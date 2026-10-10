/* AES-128 on the STM32WL55 hardware AES peripheral, through Zephyr's
 * crypto driver API (driver: crypto_stm32, devicetree node "aes"). */
#include <zephyr/kernel.h>

#if defined(CONFIG_CRYPTO_STM32)

#include <zephyr/device.h>
#include <zephyr/crypto/crypto.h>
#include "aes_backend.h"

static const struct device *const aes_dev = DEVICE_DT_GET(DT_NODELABEL(aes));
static struct cipher_ctx ctx;
static uint8_t key_copy[AES_KEY_LEN]; /* driver keeps a pointer, not a copy */

static int hw_init(void)
{
	return device_is_ready(aes_dev) ? 0 : -ENODEV;
}

/* "Key setup" here = open a driver session (configures the peripheral) */
static int hw_setup(const uint8_t key[AES_KEY_LEN], enum cipher_op dir)
{
	memcpy(key_copy, key, AES_KEY_LEN);
	ctx = (struct cipher_ctx){
		.keylen = AES_KEY_LEN,
		.key.bit_stream = key_copy,
		.flags = CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS,
	};
	return cipher_begin_session(aes_dev, &ctx, CRYPTO_CIPHER_ALGO_AES,
				    CRYPTO_CIPHER_MODE_ECB, dir);
}

static int hw_enc_setup(const uint8_t key[AES_KEY_LEN])
{
	return hw_setup(key, CRYPTO_CIPHER_OP_ENCRYPT);
}

static int hw_dec_setup(const uint8_t key[AES_KEY_LEN])
{
	return hw_setup(key, CRYPTO_CIPHER_OP_DECRYPT);
}

/* The driver only accepts ONE 16-byte block per ECB call, so loop */
static int hw_process(uint8_t *out, const uint8_t *in, size_t len)
{
	for (size_t i = 0; i < len; i += AES_BLOCK_LEN) {
		struct cipher_pkt pkt = {
			.in_buf = (uint8_t *)(in + i),
			.in_len = AES_BLOCK_LEN,
			.out_buf = out + i,
			.out_buf_max = AES_BLOCK_LEN,
		};
		int ret = cipher_block_op(&ctx, &pkt);

		if (ret) {
			return ret;
		}
	}
	return 0;
}

static void hw_teardown(void)
{
	cipher_free_session(aes_dev, &ctx);
}

const struct aes_backend aes_hw_stm32 = {
	.name = "HW (STM32 AES peripheral)",
	.init = hw_init,
	.enc_setup = hw_enc_setup,
	.encrypt = hw_process,
	.enc_teardown = hw_teardown,
	.dec_setup = hw_dec_setup,
	.decrypt = hw_process,
	.dec_teardown = hw_teardown,
};

#endif /* CONFIG_CRYPTO_STM32 */
