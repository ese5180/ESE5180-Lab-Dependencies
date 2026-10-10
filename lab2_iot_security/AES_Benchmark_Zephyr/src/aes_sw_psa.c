/* Software AES-128 through the PSA Crypto API (Zephyr's TF-PSA-Crypto).
 * Variants A/B/C only differ in where the AES lookup tables live,
 * which is set by Kconfig (sw_ram.conf / sw_rom.conf / sw_rom_fewer.conf). */
#include <zephyr/kernel.h>

#if defined(CONFIG_PSA_CRYPTO)

#include <psa/crypto.h>
#include "aes_backend.h"

static psa_key_id_t key_id;
static psa_cipher_operation_t op;

static int sw_init(void)
{
	return psa_crypto_init() == PSA_SUCCESS ? 0 : -EIO;
}

/* Import the raw key and start a cipher operation. PSA expands the AES key
 * schedule inside *_setup(), so this is the "key setup" cost. */
static int sw_setup(const uint8_t key[AES_KEY_LEN], bool encrypt)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t st;

	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, 128);
	psa_set_key_algorithm(&attr, PSA_ALG_ECB_NO_PADDING);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);

	st = psa_import_key(&attr, key, AES_KEY_LEN, &key_id);
	if (st != PSA_SUCCESS) {
		return -EIO;
	}

	op = psa_cipher_operation_init();
	st = encrypt ? psa_cipher_encrypt_setup(&op, key_id, PSA_ALG_ECB_NO_PADDING)
		     : psa_cipher_decrypt_setup(&op, key_id, PSA_ALG_ECB_NO_PADDING);
	return st == PSA_SUCCESS ? 0 : -EIO;
}

static int sw_enc_setup(const uint8_t key[AES_KEY_LEN]) { return sw_setup(key, true); }
static int sw_dec_setup(const uint8_t key[AES_KEY_LEN]) { return sw_setup(key, false); }

/* ECB has no chaining state, so the same operation can process many buffers */
static int sw_process(uint8_t *out, const uint8_t *in, size_t len)
{
	size_t out_len;

	return psa_cipher_update(&op, in, len, out, len, &out_len) == PSA_SUCCESS &&
		       out_len == len ? 0 : -EIO;
}

static void sw_teardown(void)
{
	psa_cipher_abort(&op);
	psa_destroy_key(key_id);
}

const struct aes_backend aes_sw_psa = {
#if defined(CONFIG_MBEDTLS_AES_FEWER_TABLES)
	.name = "SW (PSA), ROM + fewer tables",
#elif defined(CONFIG_MBEDTLS_AES_ROM_TABLES)
	.name = "SW (PSA), ROM tables",
#else
	.name = "SW (PSA), RAM tables",
#endif
	.init = sw_init,
	.enc_setup = sw_enc_setup,
	.encrypt = sw_process,
	.enc_teardown = sw_teardown,
	.dec_setup = sw_dec_setup,
	.decrypt = sw_process,
	.dec_teardown = sw_teardown,
};

#endif /* CONFIG_PSA_CRYPTO */
