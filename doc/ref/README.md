# Reference Documentation

Planning and release documents for the MEGA65 C Compiler Suite.

## Planning

### v1.1 Optimization Roadmap
**File:** [`V1_1_OPTIMIZATION_ROADMAP.md`](V1_1_OPTIMIZATION_ROADMAP.md)

Frame-pointer, frame-layout, bitfield and calling-convention code-size work proposed for v1.1, with the status of each item checked against the current compiler (several are already done or turned out not to apply).

## Release Management
**File:** [`RELEASE-STEPS.md`](RELEASE-STEPS.md)

Checklist for preparing and publishing a release: version bump, clean build, the test suites, documentation, install smoke test, examples, Docker images, the tag, and the GitHub release with the prebuilt archives the Release workflow attaches (`src/release/package.sh`, `.github/workflows/release.yml`).

## Legacy Script

**File:** `run_mmemu_test.sh` — obsolete: an old runner for the mmemu emulator, which never ran a program (it only checks that the `.prg` file exists and looks like an executable). It is not used by any Makefile target or test script; runtime tests run on `xemu-xmega65` only (`make test-xemu`, that is `src/test/test_xemu.sh`).

## Historical Work

`doc/archive/` holds phase completion reports, session summaries and investigation notes. They are kept for context and are not maintained.

## Status

The suite version is `SUITE_VERSION` in `include/Version.hpp` (currently `1.1.0.dev`, after the v1.1.0 release; `cc45 --version` prints it with the git hash).
