# Release Steps — MEGA65 C Compiler Suite

Checklist for preparing and publishing a release.

---

## 0. Update Version Number

Update the version string in all locations:

| File | What to change |
|------|----------------|
| `include/Version.hpp` | `SUITE_VERSION` macro (e.g., `"1.1.0"`) |
| `CLAUDE.md` | Status line at top (e.g., `v1.1`) |
| `ROADMAP.md` | Current Status field |
| `README.md` | Version references |
| `CHANGELOG.md` | Change `[Unreleased]` header to `[1.1.0] - YYYY-MM-DD` |

Verify:
```bash
make clean && make bin/cc45
./bin/cc45 --version    # Should show new version
```

---

## 1. Clean Build

```bash
make clean && make all
make lib
```

Confirm all 7 tools build without errors: `cc45`, `ca45`, `ln45`, `ar45`, `nm45`, `objdump45`, `cp45`.

---

## 2. Core Test Suite

```bash
make test
```

This runs all unit, validation, integration, and emulator tests:
- Compiler, assembler, linker, objdump unit tests
- Const expression, register tracking, restrict, clobber, cast/fold tests
- 14 validation suites (parser, semantic, assembler, linker, segments, symbols, structs, preprocessor, directives, simops, simops-extended, addressing, const, proc)
- Segment emission tests (55 tests)
- O45 format and move/fill tests
- ZP calling convention tests
- Shadow IR tests
- mmemu execution tests (full test_mmemu.sh suite — **requires mmemu-cli**)
- objdump45 tests (129 tests)

**Gate: all tests must pass. mmemu-cli must be installed and on PATH.**

---

## 3. Stdlib Validation (requires mmemu-cli)

```bash
make test-stdlib
```

- 13 stdlib execution tests (string.h, stdlib.h, ctype.h, math.h, time.h)

**Gate: all tests must pass.**

---

## 4. Performance Benchmarks

```bash
make bench
```

Runs 15 benchmarks measuring compile time, link time, object size, PRG size, and execution cycles. Compares against saved baseline (`src/test/bench_baseline.json`).

- Size or cycle regressions >10% are flagged and fail the check
- Review any SLOWER or REGRESSION entries
- If regressions are acceptable (e.g., new feature adds code), update baseline:
  ```bash
  make bench-save
  ```

**Gate: no unexpected regressions.**

---

## 5. Code Quality

```bash
make cppcheck
make coverage
```

- `cppcheck`: Static analysis — review any new warnings
- `coverage`: Code coverage report via gcov/gcovr

**Targets: >= 80% line coverage, >= 75% function coverage.**

---

## 6. GitHub Issue Audit

```bash
# Verify all issues for the target milestone are closed
gh issue list --state open --json number,title --jq '.[] | "\(.number) \(.title)"'

# Check for any open bugs
gh issue list --state open --label bug
```

All issues tagged for this release milestone should be closed. No open bugs.

---

## 7. Documentation

```bash
make man
```

Review:
- [ ] `CHANGELOG.md` — all changes since last release listed
- [ ] `CLAUDE.md` — reflects current feature set, version updated
- [ ] `ROADMAP.md` — milestones and status up to date
- [ ] `README.md` — installation instructions, feature list current
- [ ] `doc/cc45.md`, `doc/ca45.md`, `doc/ln45.md`, `doc/stdlib.md` — tool docs current

---

## 8. Install Smoke Test

```bash
make install_local
~/.local/bin/cc45 --version
~/.local/bin/ca45 --version
~/.local/bin/ln45 --version
~/.local/bin/ar45 --version
~/.local/bin/nm45 --version
~/.local/bin/objdump45 --version
```

Verify version string matches the release.

---

## 9. Example Programs

Build each example from clean to verify end-to-end toolchain:

```bash
for dir in examples/c/hello_world examples/c/hello_linked examples/c/palette_fade \
           examples/c/unit_convert examples/c/text_scroller examples/c/game_of_life; do
    echo "=== $dir ==="
    (cd "$dir" && make clean && make) || echo "FAILED: $dir"
done
```

**Gate: all examples build without errors.**

---

## 10. Final Commit & Tag

```bash
git add -A
git commit -m "Release vX.Y.Z"
git tag -a vX.Y.Z -m "Release vX.Y.Z — <one-line summary>"
git push && git push origin vX.Y.Z
```

---

## 11. GitHub Release

```bash
gh release create vX.Y.Z --title "vX.Y.Z" --notes-file CHANGELOG_EXCERPT.md
```

Or create via GitHub UI with release notes extracted from CHANGELOG.md.

---

## Quick Reference: One-Command Validation

Full pre-release validation (requires mmemu-cli):

```bash
make clean && make all && make lib && make test && make test-stdlib && make bench
```

**All steps must exit 0. Any test failure blocks the release.**
