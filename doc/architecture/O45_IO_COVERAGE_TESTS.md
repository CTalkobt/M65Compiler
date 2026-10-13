# O45 Object Format I/O Tests

The `.o45` object format code (`O45Reader.cpp`, `O45Writer.cpp`, `O45Linker.cpp`, `O45Archive.cpp`) had the lowest coverage in the 2026-07-17 gcov run (40–60%, see [COVERAGE_ANALYSIS.md](COVERAGE_ANALYSIS.md)). Two test programs target it. **Neither is part of `make test`**, so they do not contribute to `make coverage` unless run separately.

## `make test-o45` — format unit test

`src/test/test_o45.cpp`, built as `bin/test_o45` and linked directly against `O45Writer`, `O45Reader`, `O45IRSerializer`, `O45Linker` and `O45Archive`. It writes objects in memory and checks the exact bytes and the read-back (header layout, segments, symbol and relocation tables, …).

```bash
make test-o45
```

## `src/test/test_o45_io_coverage.sh` — tool round trips

A shell script that drives the built tools (`bin/ca45`, `bin/ln45`, `bin/nm45`, `bin/objdump45`) on generated assembly in `build/test/o45_io_coverage/`. It makes 10 checks:

| Group | Checks |
|-------|--------|
| Round trip | assemble a `.o45` with code, data and BSS segments (size check); assemble an object with an external reference and read it back with `objdump45 -f` |
| Linker | assemble a library object and a main object that imports from it; link them with `ln45` and check the PRG exists; assemble an object with a data-segment relocation |
| Edge cases | an object with 50+ imported symbols; a 10 KB+ code section |
| Inspection | `nm45 -n` reads the generated object |

```bash
make all
bash src/test/test_o45_io_coverage.sh
```

It prints `Passed:`/`Failed:` counts and exits 1 if any check failed. Results are not recorded here; run it against the current tools. To measure its effect on coverage, run it with a `--coverage` build (`make coverage-build`), then `make coverage-report`.

## Gaps Not Yet Covered

These were planned but are not in either test:

- reading malformed or truncated `.o45` files and error recovery;
- every relocation type (`R_LOW`, `R_HIGH`, `R_ADDR16`, `R_ADDR24`, …) applied by the linker, and objects with hundreds of relocations;
- linker error paths (undefined and duplicate symbols);
- archive operations through `ar45` (`c`, `t`, `x`, `s`, `a`, `d`) and selective member extraction by `ln45 -l`.

## Adding Checks

Add a block to `test_o45_io_coverage.sh` that calls `test_passed "<description>"` or `test_failed "<description>"`:

```bash
if ./bin/ca45 "$TEST_DIR/case.s45" -c -o "$TEST_DIR/case.o45" 2>/dev/null; then
    test_passed "My case"
else
    test_failed "My case"
fi
```

To run it with the main suite, add `@bash src/test/test_o45_io_coverage.sh` (and `@$(MAKE) test-o45`) to the `test` target in the top-level `Makefile`.
