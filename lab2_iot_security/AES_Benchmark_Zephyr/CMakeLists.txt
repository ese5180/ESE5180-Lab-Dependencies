# SPDX-License-Identifier: Apache-2.0
#
# Lab 2, Part 2: AES speed vs memory benchmark (NUCLEO-WL55JC, Zephyr)
# Pick a variant at build time with EXTRA_CONF_FILE, e.g.
#   west build -b nucleo_wl55jc <this folder> -- -DEXTRA_CONF_FILE=sw_ram.conf

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(aes_benchmark)

target_sources(app PRIVATE src/main.c src/aes_sw_psa.c src/aes_hw_stm32.c)
