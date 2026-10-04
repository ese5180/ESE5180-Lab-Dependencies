/* Zephyr replacement for the Arduino Crypto library's RNG.h: Curve25519 and
 * Ed25519 only call RNG.rand(), so back it with the Zephyr CSPRNG. */
#ifndef RNG_SHIM_h
#define RNG_SHIM_h
#include <stdint.h>
#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>

/* The STM32 HAL defines a macro named RNG for its hardware peripheral. */
#undef RNG

class RNGClass {
public:
    void rand(uint8_t *data, size_t len) { sys_csrand_get(data, len); }
};
extern RNGClass RNG;
#endif
