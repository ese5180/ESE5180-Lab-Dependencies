/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/gpio.h>
#include <errno.h>
#include <string.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#include "sec_crypto.h"

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DEFAULT_RADIO_NODE),
         "No default LoRa radio specified in DT");


#define MAX_DATA_LEN 255

/* NET-2: if nothing is heard for this long during the handshake, start over */
#define HANDSHAKE_TIMEOUT_MS 60000

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lora_encrypted_receive);


/* Explicit node label bindings for Nucleo-WL55JC discrete LEDs */
static const struct gpio_dt_spec leds[] = {
    GPIO_DT_SPEC_GET(DT_NODELABEL(blue_led_1), gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(green_led_2), gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(red_led_3), gpios)
};


static uint8_t active_led_idx = 0;


enum pairing_phase {
    INIT_ENCRYPT_PAIRING_RECV,
    INIT_ENCRYPT_PAIRING_ACK,
    LONG_ENCRYPT_PAIRING_RECV,
    LONG_ENCRYPT_PAIRING_ACK,
    SIGNATURE_PAIRING_RECV,
    SIGNATURE_PAIRING_ACK,
    PAIRED,
    PAIRING_ERROR
};

static enum pairing_phase p_phase;

/****************Start of Encryption Keys*****************/
/* Place to store encryption key(s) */
static uint8_t encrypt_key[SEC_AES_KEY_LEN];

/* Place to store signature key(s) */
static uint8_t pub_sign_key[SEC_PUBKEY_LEN];
static uint8_t priv_sign_key[SEC_PRIVKEY_LEN];

static uint8_t their_sign_key[SEC_PUBKEY_LEN];

/* Add more keys as you see fit (e.g. your Curve25519 key pair) */

/*****************End of Encryption Keys******************/


static const struct device *lora_dev;
static struct lora_modem_config config;

static uint8_t buf[MAX_DATA_LEN];


/* Packets queued by the receive callback for the state machine */
struct rx_packet {
    uint8_t data[MAX_DATA_LEN];
    uint16_t size;
    int16_t rssi;
    int8_t snr;
};

K_MSGQ_DEFINE(rx_queue, sizeof(struct rx_packet), 4, 4);


void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size,
             int16_t rssi, int8_t snr, void *user_data)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(user_data);


    /* Turn OFF current active LED */
    if (gpio_is_ready_dt(&leds[active_led_idx])) {
        gpio_pin_set_dt(&leds[active_led_idx], 0);
    }


    /* Step to next LED (Blue -> Green -> Red) */
    active_led_idx = (active_led_idx + 1) % ARRAY_SIZE(leds);


    /* Turn ON new active LED */
    if (gpio_is_ready_dt(&leds[active_led_idx])) {
        gpio_pin_set_dt(&leds[active_led_idx], 1);
    }


    LOG_INF("RX RSSI: %d dBm | SNR: %d dB | %u bytes (Active LED: %d)",
            rssi, snr, size, active_led_idx);
    LOG_HEXDUMP_INF(data, size, "Raw Bytes");

    /* Hand the packet to the state machine (drop it if the queue is full) */
    struct rx_packet pkt;

    pkt.size = MIN(size, MAX_DATA_LEN);
    memcpy(pkt.data, data, pkt.size);
    pkt.rssi = rssi;
    pkt.snr = snr;
    k_msgq_put(&rx_queue, &pkt, K_NO_WAIT);
}


/* Transmit one packet, then go back to listening */
static int radio_send(const uint8_t *pkt, uint8_t pkt_len)
{
    int ret;

    lora_recv_async(lora_dev, NULL, NULL);          /* stop listening */

    config.tx = true;
    ret = lora_config(lora_dev, &config);
    if (ret == 0) {
        ret = lora_send(lora_dev, (uint8_t *)pkt, pkt_len);
    }

    config.tx = false;                              /* back to listening */
    lora_config(lora_dev, &config);
    lora_recv_async(lora_dev, lora_receive_cb, NULL);
    return ret;
}


int main(void)
{
    int ret;

    lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);

    if (!device_is_ready(lora_dev)) {
        LOG_ERR("%s Device not ready", lora_dev->name);
        return 0;
    }


    /* Configure all 3 discrete LEDs */
    for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
        if (gpio_is_ready_dt(&leds[i])) {
            gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
        }
    }


    /* Turn ON Blue LED at boot to show RX ready state */
    if (gpio_is_ready_dt(&leds[0])) {
        gpio_pin_set_dt(&leds[0], 1);
    }


    /* --- Physical Layer Matching with 433.92 MHz TX --- */
    config.frequency = 433920000;    /* 433.92 MHz */
    config.bandwidth = BW_125_KHZ;   /* 125 kHz */
    config.datarate = SF_10;          /* SF10 */
    config.preamble_len = 8;
    config.coding_rate = CR_4_5;
    config.iq_inverted = false;
    config.public_network = false;
    config.tx_power = -15;           /* only used when replying (radio_send) */
    config.tx = false;


    ret = lora_config(lora_dev, &config);
    if (ret < 0) {
        LOG_ERR("LoRa config failed");
        return 0;
    }


    sec_sign_keygen(pub_sign_key, priv_sign_key);

    p_phase = PAIRING_ERROR; /* TODO: Set the initial phase for your system */


    LOG_INF("Listening continuously on 433.92 MHz (SF10)...");


    /* Start asynchronous reception mode */
    ret = lora_recv_async(lora_dev, lora_receive_cb, NULL);
    if (ret < 0) {
        LOG_ERR("Failed to start async receive: %d", ret);
        return 0;
    }


    /* TODO: Fill out state machine logic. Packets from the Client arrive through rx_queue. */
    while (1) {
        struct rx_packet pkt;
        bool received = (k_msgq_get(&rx_queue, &pkt, K_MSEC(100)) == 0);

        switch (p_phase) {
        case INIT_ENCRYPT_PAIRING_RECV:
            if (received) {
                /* TODO: verify the packet; if it should move you on, set the next phase */
                p_phase = PAIRING_ERROR; /* TODO: Set the next phase for your system */
            }
            break;

        case INIT_ENCRYPT_PAIRING_ACK: {
            if (received) {
                /* TODO: check whether the Client has moved on to the next stage */
                p_phase = PAIRING_ERROR; /* TODO: Set the next phase for your system */
            }
            uint8_t send_len = 0;

            /* TODO: Set buf with ACK message */
            radio_send(buf, send_len);
            break;
        }

        case LONG_ENCRYPT_PAIRING_RECV:
            p_phase = PAIRING_ERROR; /* TODO: Set the next phase for your system */
            break;

        case LONG_ENCRYPT_PAIRING_ACK:
            p_phase = PAIRING_ERROR; /* TODO: Set the next phase for your system */
            break;

        case SIGNATURE_PAIRING_RECV:
            p_phase = PAIRING_ERROR; /* TODO: Set the next phase for your system */
            break;

        case SIGNATURE_PAIRING_ACK:
            p_phase = PAIRING_ERROR; /* TODO: Set the next phase for your system */
            break;

        case PAIRED:
            /* TODO: verify, ignore duplicates, decrypt and print the Client's data (NET-4) */
            p_phase = PAIRING_ERROR; /* TODO: Set the next phase for your system */
            break;

        default:
            LOG_ERR("Error");
        }

        (void)encrypt_key;
        (void)their_sign_key;
    }
    return 0;
}
