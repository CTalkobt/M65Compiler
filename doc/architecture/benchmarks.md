# Benchmarks: cc45 beside the other 6502 C compilers

`make bench` (or `python3 src/bench/run_bench.py`) builds the 6502 C compiler
benchmarks with cc45, runs them in xemu, checks their results and prints cc45's
program sizes and times beside the published results of cc65, llvm-mos, Oscar64,
sdcc, vbcc and Calypsi (#129).

The benchmarks are gglabs' bench8 ([6502 C Compilers
Comparison](https://gglabs.us/node/2293), 2022) as kept up to date by
[c-bench-64](https://github.com/thred/c-bench-64), which also publishes the
other compilers' results. They are not in this repository (they carry no
licence): `run_bench.py` downloads c-bench-64 at a pinned commit
(`SUITE_COMMIT`, checked against `SUITE_SHA256`) into `build/bench/`, once.

| Benchmark | What it exercises |
|---|---|
| `aes256` | AES-256-CBC of 8 KB: table lookups, arrays, bit operations |
| `crc8`, `crc16`, `crc32` | a CRC of the 8 KB at `$E000`: shifts of `char`, `int`, `long` |
| `dhrystone` | Dhrystone 2.1, 1000 runs: structs, `strcpy`/`strcmp`, calls |
| `fact` | recursion |
| `pi` | 160 digits of π: `long` multiply and divide |
| `pow` | recursion and `float` |
| `puff2` | zlib's inflate (puff) of a deflated file |
| `sieve`, `sieve_bit` | the sieve of Eratosthenes, in bytes and in bits |

## Running

```bash
make bench                                    # all, at 1 MHz, cc45 -O2
make bench BENCH_ARGS="--speed 40"            # at the MEGA65's 40 MHz
python3 src/bench/run_bench.py --cflags=-O3 pi fact
python3 src/bench/run_bench.py --json r.json  # the results as JSON too
```

| Option | |
|---|---|
| `--speed 1\|3\|40` | the CPU's speed while the benchmark runs (`$D031`/`$D054`). 1 (default) compares with the published results, all measured on a C64. |
| `--cflags=OPTS` | cc45's options (default `-O2`; `-fsigned-default` is always added: the sources are standard C). `-fno-zpcall` links `c45_stack.lib`. |
| `--xemu PATH` | the emulator (default `xemu-xmega65`, or `XEMU`) |
| `--rom PATH` | the MEGA65 ROM xemu runs (default `MEGA65_ROM`, `/rom/MEGA65.ROM`, xemu's own) |
| `--timeout S` | seconds per run (default 900) |
| `--json FILE` | write the results |
| `--fetch-only` | download the suite and stop |

It needs `bin/cc45`, `bin/ln45`, the library (`make lib`) and xemu; `openssl`
for `aes256`'s check. The exit status is 1 when a benchmark does not build,
does not finish or gives a wrong result.

## The harness

Each program is the benchmark's file, compiled with cc45, and cc45's own
`src/bench/main45.c` and `out45.c` in place of the suite's `main.c`,
`cia_timer.c` and `out.c`:

- **Time**: CIA 2's timer A counts phi2 (1 MHz on the MEGA65 at every CPU
  speed) and timer B counts its underflows: 32 bits of ticks, read as
  microseconds. The suite times with the TOD clock (tenths of a second), which
  does not run in step with a headless xemu (`-sleepless`).
- **Output**: `print`, `print_int`, `print_hex` keep the text at `$C100`
  instead of putting it on the screen; `main` writes `benchmark_check()`'s
  result and the ticks at `$C0C0` and prints one line, `CHECK n, $ticks TICKS`.
  (A screen that scrolls keeps xemu's `-prgexit` from seeing `READY.`.)
- **Checks**: a benchmark passes when its check returns 0 and it printed no
  `[FAIL]` (Dhrystone's check always returns 1: it passes on its `[OK]`s).
  The CRC benchmarks read the KERNAL at `$E000`, and their expected values are
  those of the C64's KERNAL; in C65 mode `$E000` is the C65 KERNAL (ROM
  `$3E000`), so `run_bench.py` computes the CRCs of those bytes from the ROM and
  compares them with the printed ones. `aes256` copies `$E000` with `memcpy`,
  which in cc45 is a DMA copy: the RAM there, not the ROM; its expected value is
  that RAM (from xemu's memory dump) encrypted by `openssl`.
- **Size**: the `.prg` file (with the BASIC stub), as the published sizes.

## Results

cc45 (2026-11-04, `-O2`) in xemu at 1 MHz, beside the published results of
each compiler's performance build (bytes / seconds; `!` a wrong result):

| Benchmark | cc45 | cc65 | llvm-mos | Oscar64 | sdcc | vbcc | Calypsi |
|---|---|---|---|---|---|---|---|
| aes256 | 8608 / 44.3 | 9491 / 203.7 | 10595 / 10.4 | 3585 / 10.4 | 5825 / 62.5 | 13573 / 38.6 | 6757 / - |
| crc8 | 1902 / 2.4 | 3014 / 3.2 | 7627 / 1.0 | 2097 / 0.4 | 2053 / 1.2 | 5490 / 1.2 | 4229 / 1.9 |
| crc16 | 1976 / 5.4 | 3056 / 4.3 | 7812 / 1.7 | 2145 / 1.6 | 2113 / 2.6 | 5715 / 2.6 | 4281 / 2.5 |
| crc32 | 2184 / 11.9 | 3409 / 40.0 | 8047 / 3.6 | 2244 / 2.5 | 2254 / 4.2 | 6026 / 5.0 | 4379 / 4.7 |
| dhrystone | 6250 / 5.6 | 6303 / 8.4 | 8289 / 1.9 | 3483 / 1.6 | 4680 / 5.6 | 11479 / 2.3 | 7028 / 10.5 |
| fact | 2602 / 45.8 | 3537 / 237.1 | 8958 / 43.6 | 2872 / 0.0 | 2761 / 112.3 | 7013 / 187.4 | 4666 / 209.0 |
| pi | 2815 / 9.7 | 3700 / 105.1 | 9403 / 63.1 | 3335 / 31.5 | 2843 / 81.1 | 5970 / 96.3 | 4788 / 122.0 |
| pow | 6816 / 8.2 | - | 33661 / 19.9 | 6713 / 3.3 | 8216 / 8.0 | 16235 / 23.6 | 13850 / 13.3 |
| puff2 | 11743 / 29.9 | 12845 / 47.7 | 26045 / 16.0 | 15831 / 7.3 | 11537 / 16.5 | 25411 / 19.8 | 12269 / 25.0! |
| sieve | 2121 / 18.0 | 3235 / 20.5 | 7217 / 18.8 | 2600 / 8.7 | 2291 / 20.8 | 5948 / 13.5 | 4285 / 21.2 |
| sieve_bit | 2250 / 25.0 | 3437 / 51.2 | 7912 / 20.4 | 2556 / 15.3 | 2414 / 29.5 | 6194 / 20.3 | 4406 / 27.7 |

At 40 MHz (`--speed 40`) the times are about 1/34 of these (crc8 0.07 s, fact
1.1 s, puff2 0.74 s).

**What the comparison is not**: the others ran on a PAL C64 in VICE (0.985 MHz,
the VIC-II taking cycles on badlines); cc45 runs on xemu's MEGA65 at 1 MHz,
with the 45GS02's instructions (`inw`, `asw`, 16-bit stack, the Z register,
`ldq`…) and its cycle counts at that speed, and the C65 KERNAL's bytes for the
CRCs (a CRC's time depends a little on its data). Read the times as a guide to
where cc45 stands, not to a tenth of a second.

## Changing the suite

To move to a newer c-bench-64, set `SUITE_COMMIT` and `SUITE_SHA256` in
`run_bench.py` (the hash of
`https://codeload.github.com/thred/c-bench-64/tar.gz/<commit>`) and remove
`build/bench/`. A new benchmark goes in `BENCHMARKS` (its name in the results:
its source file).

## Compiler bugs the benchmarks found

Fixed with #129: a `typedef` with several declarators (`typedef struct {...}
Rec_Type, *Rec_Pointer;`, Dhrystone) did not parse; the second subscript of
`(*p)[i][j]` through a pointer to a 2-D array stepped by the size of a pointer
(tiny-AES's `state_t *`); a function whose multiply by a constant calls
`__mul16y` did not count Y among its clobbered registers, so a caller kept an
index in Y across the call (AES's MixColumns at `-O0`).
