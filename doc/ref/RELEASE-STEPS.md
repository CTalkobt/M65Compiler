# Release Steps — MEGA65 C Compiler Suite

Checklist for preparing and publishing a release. The release itself is a tag `vX.Y.Z` on `main`: pushing
it starts `.github/workflows/release.yml`, which builds the prebuilt archives with
`src/release/package.sh` and attaches them to the tag's GitHub release (step 9).

---

## 0. Update Version Number

| File | What to change |
|------|----------------|
| `include/Version.hpp` | `SUITE_VERSION`: the release's version without the `.dev` suffix that development builds carry (e.g. `"1.1.0.dev"` → `"1.2.0"`) — used by every tool's `--version` |
| `CLAUDE.md` | The `**Version:**` line at the top |
| `ROADMAP.md` | Current status |
| `README.md` | The version in the title line |
| `CHANGELOG.md` | Change `[Unreleased]` to `[vX.Y.Z] - YYYY-MM-DD` |

The archives are named after the tag (the workflow passes it to `package.sh` as `VERSION`, without the `v`),
the tools' `--version` after `SUITE_VERSION`: the two must agree.

Verify:
```bash
make clean && make bin/cc45
./bin/cc45 --version    # cc45 vX.Y.Z (<git hash>)
```

---

## 1. Clean Build

```bash
make clean && make
make check-deps        # what the build found (zlib bundled, SQLite, FUSE 3)
```

`make` (`all`) builds the tools in `bin/` — `cc45`, `ca45`, `ln45`, `ar45`, `nm45`, `objdump45`, `disk45`,
`asmCvt45`, `basic45`, `pack45`, `font45`, `tune45`, `deploy45`, `dbg45`, and the links `cp45` and `cl45`
(to `cc45`) and `al45` (to `ca45`) — and runs `make -C lib`, which builds `lib/build/c45.lib` (ZP
convention), `c45_zp.lib` (a copy of it) and `c45_stack.lib` (stack convention). `third_party/zlib` (a git
submodule) must be checked out: `package.sh` refuses to build a release without it.

---

## 2. Test Suites

```bash
make test                           # about 6 minutes
make test-xemu                      # the runtime tests in xemu (src/test/test_xemu.sh)
bash src/test/run_gte_all.sh        # GCC torture tests, both calling conventions
bash src/test/test_cc_interop.sh    # calling-convention interop
make lint-lib                       # cl45 and al45 over the library (kept clean)
```

`make test` runs the steps of `src/test/test_steps.txt` in parallel (the documentation check
`src/test/check_docs.py`, the compiler, assembler, linker and tool test scripts, `bin/test_validation`, the
installed-toolchain test `test_install.sh`, …) and then the examples (`test_examples.sh`). Its steps that run
programs (`test_stdlib_new.sh`, `test_examples.sh`) are skipped without `xemu-xmega65` on the `PATH`, so run
the release's tests on a machine that has it.

**Gate: all tests pass**: `make test`; `make test-xemu` (1092 tests); the GCC torture tests (551 with each
convention, none failing: `src/test/known_failures/gte.txt` is empty); the interop script (13 checks).

`make test-distros` (needs Docker) builds the tree in a fresh container of each supported distribution with
the packages README.md lists; the CI (`.github/workflows/compile-check.yml`) runs part of it on each push.

`make bench` (the 6502 C compiler benchmarks in xemu, [benchmarks.md](../architecture/benchmarks.md)) and
`make bench-optimizer` (a code-size comparison whose runtime check uses the mmemu emulator, no longer used
for testing) are not part of the release gate.

---

## 3. Code Coverage (optional)

```bash
make coverage
```

Rebuilds with `--coverage` (after a `make clean`), runs `make test` and writes `build/coverage/index.html`
and `build/coverage/summary.txt` (needs `gcovr`; otherwise it falls back to raw `gcov`). See
[COVERAGE_ANALYSIS.md](../architecture/COVERAGE_ANALYSIS.md). Rebuild without coverage afterwards
(`make clean && make`). There is no `cppcheck` target (the name is only listed in `.PHONY`).

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
make man     # needs pandoc; a man page per tool from doc/bin/*.md, in man/
```

Review:
- [ ] `CHANGELOG.md` — all changes since the last release
- [ ] `CLAUDE.md` — current feature set, version
- [ ] `ROADMAP.md` — milestones and status
- [ ] `README.md` — installation instructions, tool list, the Download section
- [ ] `doc/bin/*.md`, `doc/architecture/stdlib.md` — tool and library docs current
- [ ] `python3 src/test/check_docs.py` passes (also run by `make test`)

---

## 6. Install Smoke Test

`make test` already installs into `build/tmp` and builds a program with the installed tools
(`src/test/test_install.sh`). By hand:

```bash
make install_local          # PREFIX=~/.local
for t in cc45 cp45 ca45 al45 cl45 ln45 ar45 nm45 objdump45 asmCvt45 disk45 basic45 pack45 font45 tune45 deploy45 dbg45; do ~/.local/bin/$t --version; done
ls ~/.local/lib/cc45        # c45.lib c45_zp.lib c45_stack.lib crt0*.o45 tuneplay.o45 symbols/
ls ~/.local/include/cc45    # headers
(cd /tmp && ~/.local/bin/cc45 -O2 hello.c -o hello.prg)   # with a hello.c there: no -I, no CC45_INCLUDE
```

Verify the version strings match the release. The installed tools find the headers (`bin/../include/cc45`)
and libraries (`bin/../lib/cc45`) relative to their own location.

---

## 7. Example Programs

`make test` already builds the examples and, with xemu, runs them (`test_examples.sh`). To build them
without the emulator:

```bash
make -C examples
make -C examples/disk       # the two examples disks
```

**Gate: all examples build without errors.**

---

## 8. Docker Build

See [DOCKER.md](../architecture/DOCKER.md). With the root `Dockerfile` (from a checkout that has the zlib
submodule):

```bash
docker build -t cc45:vX.Y.Z .
docker run --rm cc45:vX.Y.Z cc45 --version
docker run --rm cc45:vX.Y.Z ca45 --version
docker run --rm cc45:vX.Y.Z ln45 --version
docker run --rm -v $(pwd):/work --user $(id -u):$(id -g) cc45:vX.Y.Z cc45 -O2 /work/examples/c/hello_world/hello.c -o /work/build/hello.prg
make docker             # the Alpine image, toolchain only
make docker-runtime     # the Alpine image with xemu
```

The root image and the runtime image contain `xemu-xmega65` but not the MEGA65 ROM: `make test-xemu` inside
a container needs yours mounted at `/rom/MEGA65.ROM`.

**Gate: the images build and the tools run.**

---

## 9. Tag, Archives and GitHub Release

Try the archive build before tagging, either locally or in the CI:

```bash
bash src/release/package.sh native build/release          # this machine's compiler and libraries
bash src/release/package.sh linux-static build/release    # static (musl), in an Alpine container: needs Docker
gh workflow run release.yml                               # the three archives as the run's artifacts; nothing is released
```

`package.sh` builds the files git tracks (with the bundled zlib), packs the `make install` tree as
`m65compiler-<version>-<os>-<arch>.tar.gz` and checks the archive: unpacked in a directory of its own, its
`cc45` builds a program and its `disk45` writes disk images.

Then, with the release's changes merged into `main`:

```bash
git checkout main && git pull
git tag -a vX.Y.Z -m "Release vX.Y.Z — <one-line summary>"
git push origin vX.Y.Z
```

The tag starts the Release workflow, which:

1. builds `m65compiler-X.Y.Z-linux-x86_64.tar.gz` and `m65compiler-X.Y.Z-linux-arm64.tar.gz` (statically
   linked, `package.sh linux-static`) and `m65compiler-X.Y.Z-macos-arm64.tar.gz` (`package.sh native`);
2. writes `SHA256SUMS` for the three;
3. creates the tag's GitHub release as a **draft** when there is none yet (title: the tag; a placeholder
   note), and uploads the archives and `SHA256SUMS` to it (replacing files of the same name).

Finish by hand: write the release notes from `CHANGELOG.md` into the draft and publish it.

```bash
gh run watch                                   # the Release workflow
gh release view vX.Y.Z                         # the three archives and SHA256SUMS are attached
gh release edit vX.Y.Z --notes-file <notes.md> --draft=false
```

**Gate: the workflow is green, the release has the three archives and `SHA256SUMS`, and it is published.**

---

## 10. After the Release

- `include/Version.hpp`: `SUITE_VERSION` back to a `.dev` version, and the `CLAUDE.md` version line with it.
- `CHANGELOG.md`: a new `[Unreleased]` heading.
- `ROADMAP.md`: the status line.

---

## Quick Reference

```bash
make clean && make && make test && make test-xemu && bash src/test/run_gte_all.sh && bash src/test/test_cc_interop.sh
git tag -a vX.Y.Z -m "Release vX.Y.Z" && git push origin vX.Y.Z      # then publish the draft release
```

**All steps must exit 0. Any test failure blocks the release.**
