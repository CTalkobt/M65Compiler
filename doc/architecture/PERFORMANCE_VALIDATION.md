# Performance Validation & Benchmarking

There is no `validate_performance` Makefile target or script; earlier versions of this page described one that does not exist. This page explains how to measure code size with the current tools and records one set of measurements.

## Measuring Code Size

Build the same source with the flags to compare and look at the `.prg` size (the one-step build links with `c45.lib`, or `c45_stack.lib` under `-fno-zpcall`):

```bash
for f in "" "-O0" "-O1" "-O2" "-O3" "-fno-staticalloc" "-fno-zpcall" "-fno-zpcall -fno-staticalloc"; do
    bin/cc45 $f prog.c -o /tmp/prog.prg && echo "[$f] $(wc -c < /tmp/prog.prg) bytes"
done
```

For a per-function view, compare the assembly (`bin/cc45 -S $f prog.c -o prog.s45`) or the disassembly of the object (`bin/cc45 -c $f prog.c -o prog.o45; bin/objdump45 -d prog.o45`). The PRG size includes the CRT and every library member the program pulls in, so for small programs most of the difference between conventions comes from the program's own code and the CRT is a fixed cost.

Individual optimizations can be switched on and off with `-f<name>` / `-fno-<name>` or `#pragma cc45 optimize(...)`; see [optimizations.md](optimizations.md).

## Measurements (2026-10-11)

Toolchain: `cc45 v1.0.4 (41998c22)`, libraries in `lib/build/` at that revision. Sizes are `.prg` file sizes in bytes from `bin/cc45 <flags> <src> -o out.prg`.

| Source | default | `-O0` | `-O1` | `-O2` | `-O3` | `-fno-staticalloc` | `-fno-zpcall` | `-fno-zpcall -fno-staticalloc` |
|--------|--------:|------:|------:|------:|------:|-------------------:|--------------:|-------------------------------:|
| `fib.c` (below) | 334 | 334 | 334 | 334 | 334 | 497 | 346 | 521 |
| `src/test-resources/bench_optimizer.c` | 1374 | 1374 | 1374 | 1411 | 1411 | 2309 | 1374 | 2309 |

```c
/* fib.c */
unsigned int fib(unsigned char n) {
    unsigned int a = 0, b = 1, t;
    unsigned char i;
    for (i = 0; i < n; i++) { t = a + b; a = b; b = t; }
    return a;
}
int main(void) { return fib(20) == 6765 ? 0 : 1; }
```

Observations:
- Static allocation (SAC, on by default) is the largest size factor here: turning it off adds 163 bytes to `fib.c` and 935 bytes to `bench_optimizer.c`, because functions then build stack frames.
- The `-O` level made no difference for `fib.c`, and `-O2`/`-O3` made `bench_optimizer.c` 37 bytes *larger* than `-O0`/`-O1`/default. Do not assume a higher `-O` gives smaller code; measure.
- Compile time is not a concern at this scale: compiling and linking a ~20-line test program that uses `printf`, `sprintf`, `malloc`, `strtol`, `sqrtf`, `setjmp` and `time` (16.9 KB PRG) took 0.26 s wall time.

Execution speed was not measured. To do so, run the PRG on `xemu-xmega65` and time a loop with the CIA timers or the jiffy clock (`clock()`), comparing builds of the same program.

## Related Targets

- `make bench-optimizer` (`src/test/bench_optimizer.sh`) compiles `bench_optimizer.c` at the default level and at `-O0` to standalone assembly (`-o x.s`) and assembles it with `ca45`. It checks results with the mmemu emulator, which is no longer used for testing; the size comparison part needs only `bin/cc45` and `bin/ca45`.
- `src/test-resources/bench_stdlib_critical.c` contains library micro-benchmarks but is not wired into any target.
