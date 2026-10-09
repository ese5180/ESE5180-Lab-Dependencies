/* C wrappers around the Arduino Crypto library */
#ifndef SEC_CRYPTO_H
#define SEC_CRYPTO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SEC_PUBKEY_LEN  32   /* Ed25519 and Curve25519 public keys */
#define SEC_PRIVKEY_LEN 32
#define SEC_SIG_LEN     64   /* Ed25519 signature */
#define SEC_HASH_LEN    32   /* SHA256 digest */
#define SEC_AES_KEY_LEN 16   /* AES128 */
#define SEC_AES_BLOCK   16

/* Fill buf with hardware-backed random bytes */
void sec_random(uint8_t *buf, size_t len);

/* ---- Ed25519: sign every packet (identity + integrity) ---- */
void sec_sign_keygen(uint8_t pub[SEC_PUBKEY_LEN], uint8_t priv[SEC_PRIVKEY_LEN]);
void sec_sign(uint8_t sig[SEC_SIG_LEN], const uint8_t priv[SEC_PRIVKEY_LEN],
	      const uint8_t pub[SEC_PUBKEY_LEN], const uint8_t *msg, size_t len);
bool sec_verify(const uint8_t sig[SEC_SIG_LEN], const uint8_t pub[SEC_PUBKEY_LEN],
		const uint8_t *msg, size_t len);

/* ---- Curve25519: Diffie-Hellman key exchange ---- */
/* Creates a fresh ephemeral key pair (the private key is already clamped for Curve25519).
 * Send only pub; keep priv secret. */
void sec_dh_keygen(uint8_t pub[SEC_PUBKEY_LEN], uint8_t priv[SEC_PRIVKEY_LEN]);
/* Derives the shared secret from their public key. priv is WIPED by this call.
 * Returns false if their public key is invalid. */
bool sec_dh_shared(uint8_t secret[32], const uint8_t their_pub[SEC_PUBKEY_LEN],
		   uint8_t priv[SEC_PRIVKEY_LEN]);

/* ---- SHA256: derive keys (nothing here is sent over the air) ---- */
void sec_sha256(uint8_t out[SEC_HASH_LEN], const uint8_t *data, size_t len);

/* ---- AES128 (ECB). len must be a multiple of 16: pad shorter messages ---- */
void sec_aes128_encrypt(uint8_t *out, const uint8_t key[SEC_AES_KEY_LEN],
			const uint8_t *in, size_t len);
void sec_aes128_decrypt(uint8_t *out, const uint8_t key[SEC_AES_KEY_LEN],
			const uint8_t *in, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* SEC_CRYPTO_H */
