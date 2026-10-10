# AES_Benchmark_Zephyr (Lab 2, Part 2)

Standalone AES-128 speed/memory benchmark for the NUCLEO-WL55JC. One board, no radio.

## Variants (one per build)

| Overlay | Implementation | Where the AES tables live |
|---|---|---|
| `sw_ram.conf` | Software, PSA Crypto | 4+4 tables built at boot in **RAM** |
| `sw_rom.conf` | Software, PSA Crypto | 4+4 tables precomputed in **flash** |
| `sw_rom_fewer.conf` | Software, PSA Crypto | 1+1 tables in **flash** (more math per byte) |
| `hw.conf` | STM32WL55 AES peripheral | no tables, done in hardware |

## Build, flash, read

From your Zephyr workspace (with the venv active):

```bash
APP=<path to>/lab2_iot_security/AES_Benchmark_Zephyr
west build -p -b nucleo_wl55jc -d $APP/build_sw_ram $APP -- -DEXTRA_CONF_FILE=sw_ram.conf
west flash -d $APP/build_sw_ram
screen /dev/tty.usbmodem* 115200      # then press the black reset button
```

Repeat with `sw_rom`, `sw_rom_fewer` and `hw` (change both the overlay and the `-d` folder).

## Memory

```bash
west build -d $APP/build_sw_ram -t ram_report
west build -d $APP/build_sw_ram -t rom_report
```

The `FLASH:` / `RAM:` summary printed at the end of every build is the quickest comparison.
