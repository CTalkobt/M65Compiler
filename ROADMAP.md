# M65 Compiler Suite Roadmap: v1.0 → v2.0

**Current Status:** v1.0-rc3
**Last Updated:** 2026-05-18
**Maintainer:** Craig Taylor (CTalkobt)

---

## v1.0 Release (Target: 2026-05-31)

**Status:** All blockers and should-fix items complete. In final QA.

### Readiness Checklist

- [x] All v1.0 blockers resolved
- [x] All v1.0 should-fix items completed
- [x] IR pipeline as default codegen (`--legacy-codegen` available as fallback)
- [x] Test suite: 256 assembler tests, 176 simulated ops tests, 127 linker tests, 131 objdump tests, 60 move/fill tests, mmemu integration tests
- [x] Code coverage: 80.1% line, 77.1% function, 47.4% branch
- [x] cbm.h KERNAL wrappers validated with IR pipeline (Issue #46)
- [ ] Final QA on target hardware (MEGA65 emulator)
- [ ] Release notes prepared
- [ ] Man pages generated

### Recent fixes (rc3)

- Fixed `cbm.h` inline asm parameter scoping for IR pipeline (`@_p_` convention)
- Fixed `select` simulated op segfault on immediate operands
- Added `make coverage` target for gcov/gcovr code coverage reporting

---

## v1.0.1 Patch (Target: 2026-06-30)

| Task | Category | Status |
|------|----------|--------|
| Regression test validation (diverse 45GS02 patterns) | test | todo |
| Performance microbenchmarks | test | todo |
| Documentation cleanup (remove stale .tmp/, consolidate todo.md) | doc | todo |

---

## v1.1 Minor Release (Target: 2026-07-31)

**Compiler:**

| Task | Category | Status |
|------|----------|--------|
| `inline` keyword expansion (Phase 1, `-finline-functions`) | optimizer | todo |
| Processor flag/register intrinsics (`__builtin_carry`, etc.) | codegen | todo |

**Standard Library:**

| Task | Category | Status |
|------|----------|--------|
| `math.h` integer ops (`min`, `max`, `gcd`, `lcm`) | stdlib | todo |
| `time.h` minimal (clock, time wrappers) | stdlib | todo |

**Done:**

- Flexible Array Members (C99)
- `objdump45` symbolic disassembly

---

## v1.2 Register Optimization (Target: 2026-08-31)

| Task | Category | Status |
|------|----------|--------|
| Phase 2: Selective register invalidation at call sites | optimizer | todo |
| Phase 3: Emit `.func_flags leaf` directives | codegen | done |
| Assembler-level register tracking | asm | todo |

---

## v1.3 Calling Convention Safety (Target: 2026-09-30)

| Task | Category | Status |
|------|----------|--------|
| Per-function convention attributes in `.o45` | linker, codegen | **done** |
| Convention mismatch detection | linker | **done** |
| Automatic thunk generation (stack↔ZP) | linker | **done** |
| `--no-thunks` flag and `-Wthunk` warning | linker | todo |
| Separate ZP-convention stdlib | stdlib | todo |

> Note: Core thunk infrastructure (s2z, z2s) was implemented ahead of schedule as part of the linker test coverage work. The linker now detects mismatches, generates bridge thunks in THUNK_AUTO mode, and supports THUNK_WARN and THUNK_ERROR modes.

---

## v1.4 Dynamic Linking & Banking (Target: 2026-11-30)

| Task | Category | Status |
|------|----------|--------|
| Banked segment support | linker | todo |
| Position-Independent Code (PIC) | codegen, linker | todo |
| `.import binary` assembler directive | asm | todo |

---

## v2.0 (Target: 2027-01-31)

| Task | Category | Status |
|------|----------|--------|
| `long long` / `unsigned long long` (64-bit) | parser, codegen | todo |
| Variable-Length Arrays (VLA) | parser, codegen | todo |
| Inter-TU optimization (Phase 4) | optimizer | todo |
| Assembler peephole optimizer (Phase 5) | asm | todo |
| `mega65.h` platform library | stdlib | todo |
| File I/O wrapper (`disk.h`) | stdlib | todo |
| Comprehensive validation suite (50+ test programs) | test | in progress |

---

## Post-v2.0 (Deferred)

- Floating-point support (software IEEE) — no FPU on 45GS02
- Lambda expressions
- Link-Time Code Generation (LTO)
- Static analysis & sanitizers

---

## Completed v1.0 Items

| ID | Task | Completed |
|----|------|-----------|
| V1.0-001 | `getchar()` implementation | 2026-05-05 |
| V1.0-002 | `sprintf` %ld ZP conflict fix | 2026-05-02 |
| V1.0-003 | ConstantFolder branch elimination | 2026-05-02 |
| V1.0-004 | Designated Initializers | 2026-05-02 |
| V1.0-005 | Type qualifier ordering | 2026-05-05 |
| V1.0-006 | Local `_Alignas` | 2026-05-01 |
| V1.0-007 | Address-of register vars | 2026-04-30 |
| V1.0-008 | Integer literal overflow warnings | 2026-04-30 |
| V1.0-009 | `ctype.h` completion | 2026-05-05 |
| V1.0-010 | `string.h` completion | 2026-05-05 |
| V1.0-011 | `stdlib.h` extensions | 2026-05-05 |
| V1.0-012 | `#pragma once` support | 2026-05-05 |
| V1.0-013 | `_Pragma()` operator | 2026-04-30 |
| V0.99-A | Core compilation pipeline + IR backend | 2026-05-06 |
| V0.99-B | Register clobber tracking (Phase 1) | 2026-05-06 |
| V0.99-C | Basic standard library | 2026-05-06 |
