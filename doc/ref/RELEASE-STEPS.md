# Release Steps — MEGA65 C Compiler Suite

Checklist for preparing and publishing a release.

---

## 0. Update Version Number

| File | What to change |
|------|----------------|
| `include/Version.hpp` | `SUITE_VERSION` (e.g. `"1.1.0"`) — used by every tool's `--version` |
| `CLAUDE.md` | Status line at the top |
| `ROADMAP.md` | Current status |
| `README.md` | Version references |
| `CHANGELOG.md` | Change `[Unreleased]` to `[1.1.0] - YYYY-MM-DD` |

Verify:
```bash
make clean && make bin/cc45
./bin/cc45 --version    # cc45 vX.Y.Z (<git hash>)
```

---

## 1. Clean Build

```bash
make clean && make
```

`make` (`all`) builds the ten tools in `bin/` — `cc45`, `cp45`, `ca45`, `ln45`, `ar45`, `nm45`, `objdump45`, `disk45`, `asmCvt45`, `basic45` — and runs `make -C lib`, which builds `lib/build/c45.lib` (ZP convention), `c45_zp.lib` (a copy of it) and `c45_stack.lib` (stack convention).

---

## 2. Core Test Suite

```bash
make test
```

This runs, in order: the documentation check (`src/test/check_docs.py`), the compiler, assembler, const, restrict, register, cast-fold, inline-control, function-locals, string-escape, signedness, shadow-IR (cc45 vs host gcc) and clobber tests, the xemu runtime tests (`test_xemu.sh`), the parser/struct/const/semantic/preprocessor/linker validation suites, segment emission, the assembler/simops/directives/symbols/segments/proc/addressing/simops-extended validation suites, opcodes, move/fill, objdump45, disk45, the xemu stdlib tests (`test_stdlib_new.sh`) and the examples (`test_examples.sh`).

**Gate: all tests pass. `xemu-xmega65` must be on the `PATH`** (`test_xemu.sh` fails without it; the stdlib and example scripts skip).

`make bench-optimizer` (a code-size benchmark) drives the mmemu emulator and is not part of the release gate.

---

## 3. Code Coverage

```bash
make coverage
```

Rebuilds with `--coverage`, runs `make test` and writes `build/coverage/index.html` and `build/coverage/summary.txt` (needs `gcovr`; otherwise it falls back to raw `gcov`). See [COVERAGE_ANALYSIS.md](../architecture/COVERAGE_ANALYSIS.md). There is no `cppcheck` target (the name is only listed in `.PHONY`).

---

## 4. GitHub Issue Audit

```bash
gh issue list --state open --json number,title --jq '.[] | "\(.number) \(.title)"'
gh issue list --state open --label bug
```

All issues tagged for this release milestone should be closed.

---

## 5. Documentation

```bash
make man     # needs pandoc; man pages for cc45, ca45, cp45, ln45, nm45, ar45, objdump45
```

Review:
- [ ] `CHANGELOG.md` — all changes since the last release
- [ ] `CLAUDE.md` — current feature set, version
- [ ] `ROADMAP.md` — milestones and status
- [ ] `README.md` — installation instructions, feature list
- [ ] `doc/bin/*.md`, `doc/architecture/stdlib.md` — tool and library docs current
- [ ] `python3 src/test/check_docs.py` passes (also run by `make test`)

---

## 6. Install Smoke Test

```bash
make install_local          # PREFIX=~/.local
for t in cc45 cp45 ca45 ln45 ar45 nm45 objdump45 asmCvt45 disk45 basic45; do ~/.local/bin/$t --version; done
ls ~/.local/lib/cc45        # c45.lib c45_zp.lib c45_stack.lib
ls ~/.local/include/cc45    # headers
```

Verify the version strings match the release. Note that an installed `cc45` looks for headers in `PREFIX/lib/include`, not in `PREFIX/include/cc45` where `make install` puts them, so set `CC45_INCLUDE=~/.local/include/cc45` (or pass `-I`) when testing a compile from the installed tree.

---

## 7. Example Programs

`make test` already builds and runs the examples on xemu (`test_examples.sh`). To build them without the emulator:

```bash
for dir in examples/c/*/; do
    echo "=== $dir ==="
    (cd "$dir" && make clean && make) || echo "FAILED: $dir"
done
```

**Gate: all examples build without errors.**

---

## 8. Docker Build

See [DOCKER.md](../architecture/DOCKER.md). With the root `Dockerfile`:

```bash
docker build -t cc45:vX.Y.Z .
docker run --rm cc45:vX.Y.Z cc45 --version
docker run --rm cc45:vX.Y.Z ca45 --version
docker run --rm cc45:vX.Y.Z ln45 --version
docker run --rm -v $(pwd):/work cc45:vX.Y.Z cc45 /work/examples/c/hello_world/hello.c -o /work/build/hello.prg
```

The image has no `xemu-xmega65`, so `make test` inside the container fails at the xemu runtime tests.

**Gate: the image builds and the tools run.**

---

## 9. Final Commit & Tag

```bash
git add -A
git commit -m "Release vX.Y.Z"
git tag -a vX.Y.Z -m "Release vX.Y.Z — <one-line summary>"
git push && git push origin vX.Y.Z
```

---

## 10. GitHub Release

```bash
gh release create vX.Y.Z --title "vX.Y.Z" --notes-file CHANGELOG_EXCERPT.md
```

Or create it in the GitHub UI with the notes from `CHANGELOG.md`.

---

## 11. Merge to Main

```bash
git checkout main
git merge --no-ff dev_vX.Y.Z -m "Merge dev_vX.Y.Z: <one-line summary>"
git push origin main
git log --oneline -5
```

**Gate: main contains all release commits.**

---

## Quick Reference

```bash
make clean && make && make test && docker build -t cc45:latest .
```

**All steps must exit 0. Any test failure blocks the release.**
