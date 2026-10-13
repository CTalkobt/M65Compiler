# M65 Compiler Code Coverage

Line/function coverage of the C++ toolchain sources (`src/main/`) is measured with gcov.

## How to Measure

```bash
make coverage
```

The `coverage` target (top-level `Makefile`):

1. `coverage-build`: `make clean`, then rebuilds `all lib` with `EXTRA_CXXFLAGS=--coverage`;
2. runs `make test` with the instrumented binaries (failures do not stop the report);
3. `coverage-report`: `gcovr --root . --filter 'src/main/' --exclude '.*_main\.cpp'` writes `build/coverage/index.html` (per-file HTML) and `build/coverage/summary.txt`, and prints a summary. Without `gcovr` (`pip install gcovr`) it falls back to raw `gcov` output for the first files.

`make coverage-clean` removes `build/coverage/` and the `.gcda`/`.gcno` files. Note that `make test` includes the xemu runtime tests; without `xemu-xmega65` on the `PATH` those steps fail and the code they exercise shows as uncovered.

What is and is not measured:
- only `src/main/` is reported, and the tools' `*_main.cpp` command-line drivers are excluded;
- the C library (`lib/`) and generated 45GS02 code are not covered by gcov.

## Historical Snapshot (2026-07-17)

The figures below are from a gcov run on 2026-07-17. They have **not** been reproduced against the current tree — the sources have grown considerably since (for example `IRBuilder.cpp` is now ~8,800 lines, `O45Linker.cpp` ~4,650, `IRCodeGen.cpp` ~5,950) — so treat them only as a rough baseline and rerun `make coverage` for current numbers. The "Lines" column is the number of instrumented lines gcov reported at the time.

| Component | Line coverage | Lines |
|-----------|---------------|-------|
| Lexer.cpp | 83.0% | 719 |
| Parser.cpp | 92.5% | 493 |
| Preprocessor.cpp | 90.8% | 249 |
| AST.cpp | 95.6% | 45 |
| IRBuilder.cpp | 74.9% | 3285 |
| IRCodeGen.cpp | 78.5% | 2616 |
| IROptimizer.cpp | 84.3% | 730 |
| M65Emitter.cpp | 86.1% | 158 |
| AssemblerLexer.cpp | 78.9% | 194 |
| AssemblerParser.cpp | 62.7% | 1521 |
| AssemblerGenerator.cpp | 85.8% | 415 |
| AssemblerOptimizer.cpp | 72.9% | 802 |
| AssemblerExpression.cpp | 54.8% | 609 |
| AssemblerSimulatedOps.cpp | 59.3% | 2543 |
| AssemblerOpcodeDatabase.cpp | 14.5% | 207 |
| O45Emitter.cpp | 62.8% | 374 |
| O45Reader.cpp | 52.6% | 171 |
| O45Writer.cpp | 53.6% | 714 |
| O45Linker.cpp | 58.8% | 2503 |
| O45Archive.cpp | 40.6% | 663 |
| TypeInfo.cpp | 91.5% | 201 |
| ConstantFolder.cpp | 76.0% | 100 |
| LoopOptimizer.cpp | 90.4% | 73 |
| VRegAllocator.cpp | 86.1% | 158 |
| OpEffect.cpp | 93.3% | 15 |

At that time the weakest areas were the object-file and archive code (`O45Reader`, `O45Writer`, `O45Linker`, `O45Archive`: 40–60%) and the assembler expression/simulated-op code (55–60%). [O45_IO_COVERAGE_TESTS.md](O45_IO_COVERAGE_TESTS.md) describes a test script aimed at the O45 modules.

The release checklist ([../ref/RELEASE-STEPS.md](../ref/RELEASE-STEPS.md)) runs `make coverage` before a release.
