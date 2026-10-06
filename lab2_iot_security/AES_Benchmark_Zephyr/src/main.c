/*
 * Lab 2, Part 2: Encryption speed and memory tradeoffs (NUCLEO-WL55JC)
 *
 * Measures one AES-128 variant per build:
 *   - correctness against the FIPS-197 test vector
 *   - key setup time (first call "cold", then averaged "warm")
 *   - encrypt / decrypt time for one full LoRa payload, and per byte
 *
 * The variant is picked by the overlay passed with -DEXTRA_CONF_FILE=...
 */
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/timing/timing.h>

#include "aes_backend.h"

#if defined(CONFIG_CRYPTO_STM32)
static const struct aes_backend *const aes = &aes_hw_stm32;
#elif defined(CONFIG_PSA_CRYPTO)
static const struct aes_backend *const aes = &aes_sw_psa;
#else
#error "No AES variant selected: build with -DEXTRA_CONF_FILE=sw_ram.conf (or sw_rom, sw_rom_fewer, hw)"
#endif

/* Largest encrypted payload in one LoRa packet:
 * 255 B max packet - 64 B Ed25519 signature = 191 B, rounded down to 16 B blocks */
#define MSG_LEN    176
#define ITERATIONS 200

/* FIPS-197 Appendix C.1 (AES-128) known-answer test */
static const uint8_t kat_key[16] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
static const uint8_t kat_pt[16] = {
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
static const uint8_t kat_ct[16] = {
	0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
	0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a};

static uint8_t msg[MSG_LEN], ct[MSG_LEN], pt[MSG_LEN];

static inline uint64_t cycles_between(timing_t *a, timing_t *b)
{
	return timing_cycles_get(a, b);
}

/* Print a cycle count as "<cycles> cyc  <ns>.<2 digits> ns" divided by div */
static void print_time(const char *label, uint64_t cycles, uint32_t div)
{
	uint64_t ns_x100 = timing_cycles_to_ns(cycles * 100U) / div;

	printk("  %-28s %8llu cyc  %8llu.%02llu ns\n", label, cycles / div,
	       ns_x100 / 100U, ns_x100 % 100U);
}

static int known_answer_test(void)
{
	uint8_t out[16];

	if (aes->enc_setup(kat_key) || aes->encrypt(out, kat_pt, 16)) {
		return -1;
	}
	aes->enc_teardown();
	if (memcmp(out, kat_ct, 16) != 0) {
		return -2;
	}

	if (aes->dec_setup(kat_key) || aes->decrypt(out, kat_ct, 16)) {
		return -3;
	}
	aes->dec_teardown();
	return memcmp(out, kat_pt, 16) ? -4 : 0;
}

int main(void)
{
	timing_t t0, t1;
	uint64_t cold_setup, setup_sum = 0, enc_sum, dec_sum;
	uint8_t key[16];

	for (int i = 0; i < MSG_LEN; i++) {
		msg[i] = (uint8_t)(i * 7 + 3);
	}
	memcpy(key, kat_key, sizeof(key));

	timing_init();
	timing_start();

	printk("\n=== AES-128 benchmark: %s ===\n", aes->name);
	printk("CPU timer: %u MHz, message: %d B, iterations: %d\n",
	       timing_freq_get_mhz(), MSG_LEN, ITERATIONS);

	if (aes->init()) {
		printk("init FAILED\n");
		return 0;
	}

	/* 1. Cold key setup: the very first call (may build tables, wake HW, ...) */
	t0 = timing_counter_get();
	aes->enc_setup(key);
	t1 = timing_counter_get();
	aes->enc_teardown();
	cold_setup = cycles_between(&t0, &t1);

	/* 2. Correctness */
	int kat = known_answer_test();

	printk("FIPS-197 known-answer test: %s\n", kat ? "FAIL" : "PASS");
	if (kat) {
		printk("  (error %d), not timing a broken implementation\n", kat);
		return 0;
	}

	/* 3. Warm key setup, averaged */
	for (int i = 0; i < ITERATIONS; i++) {
		t0 = timing_counter_get();
		aes->enc_setup(key);
		t1 = timing_counter_get();
		aes->enc_teardown();
		setup_sum += cycles_between(&t0, &t1);
	}

	/* 4. Encrypt one full payload, many times, with the key already set up */
	aes->enc_setup(key);
	t0 = timing_counter_get();
	for (int i = 0; i < ITERATIONS; i++) {
		aes->encrypt(ct, msg, MSG_LEN);
	}
	t1 = timing_counter_get();
	aes->enc_teardown();
	enc_sum = cycles_between(&t0, &t1);

	/* 5. Decrypt it back */
	aes->dec_setup(key);
	t0 = timing_counter_get();
	for (int i = 0; i < ITERATIONS; i++) {
		aes->decrypt(pt, ct, MSG_LEN);
	}
	t1 = timing_counter_get();
	aes->dec_teardown();
	dec_sum = cycles_between(&t0, &t1);

	timing_stop();

	printk("Round trip of %d B payload: %s\n", MSG_LEN,
	       memcmp(pt, msg, MSG_LEN) ? "MISMATCH" : "OK");

	printk("Results (average per call):\n");
	print_time("key setup, first call", cold_setup, 1);
	print_time("key setup", setup_sum, ITERATIONS);
	print_time("encrypt 176 B payload", enc_sum, ITERATIONS);
	print_time("decrypt 176 B payload", dec_sum, ITERATIONS);
	print_time("encrypt per byte", enc_sum, ITERATIONS * MSG_LEN);
	print_time("decrypt per byte", dec_sum, ITERATIONS * MSG_LEN);

	/* One line to paste into a spreadsheet */
	printk("CSV,%s,%llu,%llu,%llu,%llu\n", aes->name, cold_setup,
	       setup_sum / ITERATIONS, enc_sum / (ITERATIONS * MSG_LEN),
	       dec_sum / (ITERATIONS * MSG_LEN));
	printk("    (variant, cold setup cyc, setup cyc, enc cyc/B, dec cyc/B)\n");

	return 0;
}
