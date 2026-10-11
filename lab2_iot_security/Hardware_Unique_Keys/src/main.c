/* You'll need libraries for flash + NVS setup, as well as crypto functions */
#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/kvss/nvs.h>
#include <psa/crypto.h>
#include <soc.h>
#include <string.h>

#define NVS_APP_KEY_ID 1
#define AES_KEY_SIZE 16
#define LORAWAN_KEY_SIZE 16

/* Sample Raw Secret (LoRaWAN AppKey) */
uint8_t raw_lora_app_key[16] = {
    0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
    0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
};

static struct nvs_fs fs;
static const uint8_t app_salt[] = "ESE5180_LAB2_SECTION3_2_SALT";

/* Read hardware 96-bit Unique Device ID (UID) directly from STM32 silicon */
static void get_stm32_hardware_uid(uint8_t uid_out[12])
{
    /* STM32WL55JC 96-bit Unique ID Register Base Address */
    #ifndef UID_BASE
    #define UID_BASE 0x1FFF7580
    #endif

    uint32_t *uid_registers = (uint32_t *)UID_BASE;
    uint32_t word0 = uid_registers[0];
    uint32_t word1 = uid_registers[1];
    uint32_t word2 = uid_registers[2];

    memcpy(&uid_out[0], &word0, 4);
    memcpy(&uid_out[4], &word1, 4);
    memcpy(&uid_out[8], &word2, 4);
}

/* Store a secret in plaintext in NVS */
static int store_unencrypted_secret(const uint8_t *plain, size_t len)
{
    /*  TODO: 3.1 Storing Information in Plaintext
        Write to non-volatile storage the LoRaWAN AppKey in plaintext.
        Return 0 if successful, the return code if failed
    */
}


/* Derive a 128-bit AES Key using PSA Crypto HKDF mixed with Hardware UID */
// AES = Advanced Encryption Standard, symmetric key crypto
// PSA = latform Security Architecture, security framework by ARM
// HKDF = HMAC-based Extract-and-Expand Key Derivation Function
static psa_status_t derive_huk_aes_key(psa_key_id_t *key_handle)
{
    uint8_t hw_uid[12];
    get_stm32_hardware_uid(hw_uid);

    /*  TODO: 3.2 Hardware Key Derivation & AES-GCM
        Implement the following steps to derive an AES key using a Hardware Unique Key
        psa_set_key_*, psa_import_key, & psa_key_derivation_setup functions will be helpful here
    */

    /* 1. Import raw HW UID into a temporary PSA key slot */


    /* 2. Configure target derived key attributes */


    /* 3. Setup HKDF Derivation Operation */


        /* HKDF Step 1: Salt */

        /* HKDF Step 2: Secret (Pass key ID via psa_key_derivation_input_key) */

        /* HKDF Step 3: Info Label */


    /* 4. Derive AES key handle */


    return status;
}

/* Encrypt and store secret in NVS */
static int store_encrypted_secret(psa_key_id_t key_handle, const uint8_t *plain, size_t len)
{
    uint8_t cipher_text[32] = {0};
    size_t out_len = 0;

    /*  TODO: 3.2 Hardware Key Derivation & AES-GCM
        Leverage psa_cipher_encrypt and the HUK derived key to encrypt
        the plaintext and store in NVS.
    */

    /* Write encrypted payload to NVS */
    int rc = nvs_write(&fs, NVS_APP_KEY_ID, cipher_text, out_len);
    if (rc < 0) {
        printk("[NVS_ERR] Flash write failed: %d\n", rc);
        return rc;
    }

    printk("[SUCCESS] Encrypted key written to NVS (%d bytes)\n", rc);
    return 0;
}

/* Read encrypted payload from NVS and decrypt using derived HUK key */
static int read_and_decrypt_secret(psa_key_id_t key_handle)
{
    printk("\n--- NVS Read & PSA Decrypt Stage ---\n");

    uint8_t cipher_text[32] = {0};
    uint8_t decrypted_plain[16] = {0};
    size_t out_len = 0;

    /*  TODO: 3.2 Hardware Key Derivation & AES-GCM
        Implement the following steps to read out the
        encrypted key from NVS, then decrypt it &
        validate with the plaintext key.
    */

    /* 1. Read encrypted payload from NVS */


    /* 2. Decrypt payload using PSA Crypto */
    /* Note: psa_cipher_decrypt automatically parses the 16-byte IV prepended by psa_cipher_encrypt */


    /* 3. Print decrypted key in Hex */


    /* 4. Verify against original raw key */


    return 0;
}


int main(void)
{
    printk("\n--- Lab 2, Section 3: Leveraging Hardware Unique Keys (HUKs) ---\n");

    /* Initialize NVS File System */
    const struct device *flash_dev = PARTITION_DEVICE(storage_partition);
    fs.flash_device = flash_dev;
    fs.offset = PARTITION_OFFSET(storage_partition);
    
    struct flash_pages_info info;
    flash_get_page_info_by_offs(flash_dev, fs.offset, &info);

    fs.sector_size = 2048; /* STM32WL55 flash page size is 2KB */
    fs.sector_count = 2;   /* 2 sectors = 4KB total partition size */

    if (nvs_mount(&fs) != 0) {
        printk("[CRITICAL] NVS Mount Failed!\n");
        return 0;
    }

    /* ********** Section 3.1: Storing Information in Plaintext ********** */
    // // Use for Section 3.1, then comment out for Section 3.2
    // store_unencrypted_secret(raw_lora_app_key, sizeof(raw_lora_app_key));

    /* ********** Section 3.2: Hardware Key Derivation & AES-GCM ********** */
    // Comment out this section when working on Section 3.1

    /* Initialize PSA Crypto Subsystem */
    if (psa_crypto_init() != PSA_SUCCESS) {
        printk("[CRITICAL] PSA Crypto Init Failed!\n");
        return 0;
    }

    /* Display Hardware Silicon ID */
    uint8_t uid[12];
    get_stm32_hardware_uid(uid);
    printk("STM32 Silicon UID: %02X%02X%02X%02X-%02X%02X%02X%02X-%02X%02X%02X%02X\n",
           uid[0], uid[1], uid[2], uid[3], uid[4], uid[5],
           uid[6], uid[7], uid[8], uid[9], uid[10], uid[11]);

    /* Derive HUK Key */
    psa_key_id_t huk_key_handle;
    if (derive_huk_aes_key(&huk_key_handle) != PSA_SUCCESS) {
        printk("[CRITICAL] HUK Derivation Failed!\n");
        return 0;
    }

    /* Encrypt and write to NVS */
    store_encrypted_secret(huk_key_handle, raw_lora_app_key, sizeof(raw_lora_app_key));

    /* Read back from NVS and decrypt using the same derived HUK key */
    read_and_decrypt_secret(huk_key_handle);

    /* Clean up key handle from memory */
    psa_destroy_key(huk_key_handle);

    // Done!
    while(1); 
}