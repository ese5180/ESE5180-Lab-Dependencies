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


/* Largest LoRa payload, not counting the radio's own header */
#define MAX_PACKET_LEN 255

/* How long to wait for the Server's reply before sending again */
#define LISTENING_INTERVAL_MS 3000

/* NET-2: if nothing is heard for this long during the handshake, start over */
#define HANDSHAKE_TIMEOUT_MS 60000

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lora_encrypted_send);


/* Explicit node label bindings for Nucleo-WL55JC discrete LEDs */
static const struct gpio_dt_spec led_blue  = GPIO_DT_SPEC_GET(DT_NODELABEL(blue_led_1), gpios);
static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(DT_NODELABEL(green_led_2), gpios);
static const struct gpio_dt_spec led_red   = GPIO_DT_SPEC_GET(DT_NODELABEL(red_led_3), gpios);


enum pairing_phase {
    INIT_ENCRYPT_PAIRING,
    LONG_ENCRYPT_PAIRING,
    SIGNATURE_PAIRING,
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
static bool radio_in_tx;

static uint8_t buf[MAX_PACKET_LEN];

/* Message to send to the Server */
char data[] = "ese5180t00-0";


/* Switch the radio between transmit and receive */
static int radio_set_mode(bool tx)
{
    config.tx = tx;
    int ret = lora_config(lora_dev, &config);

    if (ret == 0) {
        radio_in_tx = tx;
    }
    return ret;
}

/* Transmit one packet */
static int radio_send(const uint8_t *pkt, uint8_t pkt_len)
{
    int ret;

    if (!radio_in_tx) {
        ret = radio_set_mode(true);
        if (ret < 0) {
            return ret;
        }
    }

    if (gpio_is_ready_dt(&led_red)) {
        gpio_pin_set_dt(&led_red, 1);
    }

    ret = lora_send(lora_dev, (uint8_t *)pkt, pkt_len);

    if (gpio_is_ready_dt(&led_red)) {
        gpio_pin_set_dt(&led_red, 0);
    }
    return ret;
}

/* Wait up to timeout_ms for a packet; returns its length, or a negative error */
static int radio_recv(uint8_t *out, uint8_t out_size, int32_t timeout_ms)
{
    int16_t rssi;
    int8_t snr;

    if (radio_in_tx) {
        int ret = radio_set_mode(false);

        if (ret < 0) {
            return ret;
        }
    }
    return lora_recv(lora_dev, out, out_size, K_MSEC(timeout_ms), &rssi, &snr);
}


int main(void)
{
    uint32_t airtime_ms;
    int ret;

    lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);

    if (!device_is_ready(lora_dev)) {
        LOG_ERR("%s Device not ready", lora_dev->name);
        return 0;
    }


    /* Configure discrete LEDs */
    if (gpio_is_ready_dt(&led_blue))  gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_INACTIVE);
    if (gpio_is_ready_dt(&led_green)) gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
    if (gpio_is_ready_dt(&led_red))   gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);


    /* --- FCC Part 15.231 Micro-Power Settings --- */
    config.frequency = 433920000;    /* 433.92 MHz */
    config.bandwidth = BW_125_KHZ;   /* 125 kHz */
    config.datarate = SF_10;          /* SF10 */
    config.preamble_len = 8;
    config.coding_rate = CR_4_5;
    config.iq_inverted = false;
    config.public_network = false;
    config.tx_power = -15;           /* Minimum output (-15 dBm) */

    ret = radio_set_mode(true);
    if (ret < 0) {
        LOG_ERR("LoRa config failed");
        return 0;
    }


    airtime_ms = lora_airtime(lora_dev, MAX_PACKET_LEN);
    LOG_INF("Airtime of a full %d-byte packet: %u ms", MAX_PACKET_LEN, airtime_ms);


    sec_sign_keygen(pub_sign_key, priv_sign_key);

    p_phase = PAIRING_ERROR; /* TODO: Set the initial phase for your system */


    /* TODO: Fill out state machine logic */
    while (1) {
        switch (p_phase) {
        case INIT_ENCRYPT_PAIRING: {
            /* TODO: build the ID + public signing key packet in buf and send it */
            int send_len = 0;

            radio_send(buf, send_len);

            /* Wait for the Server's reply (the ACK) */
            ret = radio_recv(buf, sizeof(buf), LISTENING_INTERVAL_MS);
            if (ret > 0) {
                /* TODO: verify the packet; if it should move you on, set the next phase */
                p_phase = PAIRING_ERROR; /* TODO: Set the next phase for your system */
            }
            break;
        }

        case LONG_ENCRYPT_PAIRING:
            p_phase = PAIRING_ERROR; /* TODO: Set the next phase for your system */
            break;

        case SIGNATURE_PAIRING:
            p_phase = PAIRING_ERROR; /* TODO: Set the next phase for your system */
            break;

        case PAIRED:
            /* TODO: send `data` encrypted + signed, at least twice (NET-3) */
            p_phase = PAIRING_ERROR; /* TODO: Set the next phase for your system */
            break;

        default:
            LOG_ERR("Error");
            return 0;
        }

        (void)encrypt_key;
        (void)their_sign_key;
        k_msleep(100);
    }
    return 0;
}
