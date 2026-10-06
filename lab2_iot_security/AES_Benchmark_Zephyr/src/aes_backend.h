/* Common interface every AES variant implements, so main.c can time
 * them all the same way. All lengths are multiples of 16 (AES block). */
#ifndef AES_BACKEND_H
#define AES_BACKEND_H

#include <stddef.h>
#include <stdint.h>

#define AES_KEY_LEN   16
#define AES_BLOCK_LEN 16

struct aes_backend {
	const char *name;

	int (*init)(void);

	/* Key setup: everything needed before the first block can be processed */
	int (*enc_setup)(const uint8_t key[AES_KEY_LEN]);
	int (*encrypt)(uint8_t *out, const uint8_t *in, size_t len);
	void (*enc_teardown)(void);

	int (*dec_setup)(const uint8_t key[AES_KEY_LEN]);
	int (*decrypt)(uint8_t *out, const uint8_t *in, size_t len);
	void (*dec_teardown)(void);
};

extern const struct aes_backend aes_sw_psa;
extern const struct aes_backend aes_hw_stm32;

#endif /* AES_BACKEND_H */
