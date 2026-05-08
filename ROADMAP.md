# M65 Compiler Suite Roadmap: v1.0 → v2.0

**Current Status:** v0.99.dev (pre-release)  
**Last Updated:** 2026-05-08  
**Maintainer:** Craig Taylor (CTalkobt)

---

## Kanban Board — Current Work

**Legend:**
- **Effort:** XS (<4h), S (4-8h), M (8-16h), L (16-32h), XL (32+h)
- **Status:** 📋 Todo | 🔄 In Progress | 🔍 Review | ✅ Done
- **Tags:** `blocker`, `stdlib`, `parser`, `codegen`, `asm`, `optimizer`, `bugfix`, `doc`

---

### 📋 TO DO — v1.0 Release (Target: 2026-05-31)

Blockers and high-priority items for v1.0 release.

**Status:** ✅ **ALL ITEMS COMPLETE** — Ready for release candidate / final QA

---

### 🔄 IN PROGRESS

| ID | Task | Effort | Category | Dependencies | Acceptance Criteria | Assignee |
|---|---|---|---|---|---|---|
| | *None currently* | | | | | |

---

### 🔍 REVIEW / TESTING

| ID | Task | Effort | Category | Dependencies | Acceptance Criteria |
|---|---|---|---|---|---|
| | *None currently* | | | | |

---

### ✅ DONE — v1.0 Release

| ID | Task | Effort | Category | Completed |
|---|---|---|---|---|
| **V1.0-001** | **`getchar()` implementation** | M | stdlib | 2026-05-05 |
| **V1.0-002** | **`sprintf` %ld ZP conflict** | L | bugfix, codegen | 2026-05-02 |
| **V1.0-003** | **ConstantFolder branch elimination** | L | bugfix, optimizer | 2026-05-02 |
| **V1.0-004** | **Designated Initializers** | L | parser | 2026-05-02 |
| **V1.0-005** | **Type qualifier ordering** | S | parser | 2026-05-05 |
| **V1.0-006** | **Local `_Alignas`** | L | parser, asm | 2026-05-01 |
| **V1.0-007** | **Address-of register vars** | S | parser | 2026-04-30 |
| **V1.0-008** | **Integer literal overflow warnings** | M | parser | 2026-04-30 |
| **V1.0-009** | **`ctype.h` completion** | M | stdlib | 2026-05-05 |
| **V1.0-010** | **`string.h` completion** | M | stdlib | 2026-05-05 |
| **V1.0-011** | **`stdlib.h` extensions** | M | stdlib | 2026-05-05 |
| **V1.0-012** | **`#pragma once` support** | S | parser | 2026-05-05 |
| **V1.0-013** | **`_Pragma()` operator** | M | parser | 2026-04-30 |
| **V0.99-A** | Core compilation pipeline | XL | codegen | 2026-05-06 |
| **V0.99-B** | Register clobber tracking (Phase 1) | L | optimizer | 2026-05-06 |
| **V0.99-C** | Basic standard library (math.h, string.h partial, etc.) | L | stdlib | 2026-05-06 |

---

## Backlog — v1.0.1 to v2.0

### v1.0.1 Patch (Target: 2026-06-30)

| ID | Task | Effort | Category | Phase Target | Dependencies |
|---|---|---|---|---|---|
| **V1.0.1-001** | Regression test validation (diverse 45GS02 patterns) | M | test | v1.0.1 | All v1.0 items done |
| **V1.0.1-002** | Performance microbenchmarks | M | test | v1.0.1 | All v1.0 items done |
| **V1.0.1-003** | Documentation updates from v1.0 testing | M | doc | v1.0.1 | Beta testing complete |

---

### ✅ DONE — v1.1 Minor Release

| ID | Task | Effort | Category | Completed |
|---|---|---|---|---|
| **V1.1-001** | **Flexible Array Members (C99)** | L | parser, codegen | 2026-05-02 |
| **V1.1-008** | **`objdump45` symbolic disassembly** | M | tool | 2026-04-28 |

---

### v1.1 Minor Release (Target: 2026-07-31) — In Progress

**Compiler Features:**

| ID | Task | Effort | Category | Phase Target | Dependencies |
|---|---|---|---|---|---|
| **V1.1-002** | `inline` keyword expansion (Phase 1) | L | optimizer | v1.1 | V1.0 complete; add `-finline-functions` flag |
| **V1.1-003** | Processor flag/register intrinsics (`__builtin_carry`, etc.) | L | codegen | v1.1 | V1.0 complete |

**Standard Library:**

| ID | Task | Effort | Category | Phase Target | Dependencies |
|---|---|---|---|---|---|
| **V1.1-004** | `math.h` integer ops (`min`, `max`, `gcd`, `lcm`) | M | stdlib | v1.1 | V1.0 complete |
| **V1.1-005** | `assert.h` macro | S | stdlib | v1.1 | V1.0 complete |
| **V1.1-006** | `limits.h` completion | S | stdlib | v1.1 | V1.0 complete |
| **V1.1-007** | `time.h` minimal (clock, time wrappers) | M | stdlib | v1.1 | V1.0 complete |

---

### v1.2 Register Optimization (Target: 2026-08-31)

| ID | Task | Effort | Category | Phase Target | Dependencies |
|---|---|---|---|---|---|
| **V1.2-001** | Phase 2: Selective invalidation at call sites (intra-TU) | L | optimizer | v1.2 | V1.0 complete; Phase 1 (V0.99) done |
| **V1.2-002** | Phase 3: Emit `.func_flags leaf` directives | M | codegen | v1.2 | V1.2-001 |
| **V1.2-003** | Assembler-level register tracking | L | asm | v1.2 | V1.2-002 |

---

### v1.3 Calling Convention Safety (Target: 2026-09-30)

| ID | Task | Effort | Category | Phase Target | Dependencies |
|---|---|---|---|---|---|
| **V1.3-001** | Per-function convention attributes in `.o45` | M | linker, codegen | v1.3 | V1.0 complete |
| **V1.3-002** | Convention mismatch detection | M | linker | v1.3 | V1.3-001 |
| **V1.3-003** | Automatic thunk generation (optional) | XL | linker, codegen | v1.3 | V1.3-002 |
| **V1.3-004** | Pragma support for clobber annotations | M | parser | v1.3 | V1.0 complete |
| **V1.3-005** | Separate ZP-convention stdlib | L | stdlib | v1.3 | V1.3-001 |

---

### v1.4 Dynamic Linking & Banking (Target: 2026-11-30)

| ID | Task | Effort | Category | Phase Target | Dependencies |
|---|---|---|---|---|---|
| **V1.4-001** | Banked segment support | XL | linker | v1.4 | V1.0 complete |
| **V1.4-002** | Position-Independent Code (PIC) | XL | codegen, linker | v1.4 | V1.4-001 |
| **V1.4-003** | Lazy loading infrastructure | L | linker | v1.4 | V1.4-002 |
| **V1.4-004** | `.import binary` assembler directive | M | asm | v1.4 | V1.0 complete |

---

### v2.0 Beta (Target: 2026-12-31)

| ID | Task | Effort | Category | Phase Target | Dependencies |
|---|---|---|---|---|---|
| **V2.0-001** | `long long` / `unsigned long long` (64-bit) | XL | parser, codegen, asm | v2.0 Beta | V1.0 complete |
| **V2.0-002** | `_Atomic` type qualifier | L | parser, codegen | v2.0 Beta | V1.0 complete |
| **V2.0-003** | Variable-Length Arrays (VLA) | L | parser, codegen | v2.0 Beta | V1.0 complete |
| **V2.0-004** | Inter-TU optimization (Phase 4) | L | optimizer | v2.0 Beta | V1.2 complete |
| **V2.0-005** | Assembler optimizer (peephole, Phase 5) | M | asm | v2.0 Beta | V1.2 complete |
| **V2.0-006** | Interprocedural constant propagation | M | optimizer | v2.0 Beta | V1.2 complete |
| **V2.0-007** | `mega65.h` platform library | L | stdlib | v2.0 Beta | V1.0 complete |
| **V2.0-008** | File I/O wrapper (`fcb.h` / `disk.h`) | L | stdlib | v2.0 Beta | V1.0 complete |
| **V2.0-009** | Comprehensive validation suite (50+ test programs) | XL | test | v2.0 Beta | All v1.x features |
| **V2.0-010** | Complete `math.h` (clz, ctz, popcount, rotl/rotr) | M | stdlib | v2.0 Beta | V1.1 complete |

---

### v2.0 Release (Target: 2027-01-31)

| ID | Task | Effort | Category | Phase Target | Dependencies |
|---|---|---|---|---|---|
| **V2.0-RELEASE-001** | API freeze (no breaking changes) | S | doc | v2.0 | All v2.0 Beta items done |
| **V2.0-RELEASE-002** | Final documentation & best practices guide | M | doc | v2.0 | All features complete |
| **V2.0-RELEASE-003** | Performance profiling (compile speed, binary size, runtime) | M | test | v2.0 | All features complete |
| **V2.0-RELEASE-004** | Community beta testing period (2 weeks) | M | test | v2.0 | v2.0 Beta complete |
| **V2.0-RELEASE-005** | Stable release & tag | S | doc | v2.0 | All items done |

---

## Post-v2.0 Backlog (v2.1+)

Deferred features explicitly excluded from v2.0:

| ID | Task | Effort | Category | Phase Target | Notes |
|---|---|---|---|---|---|
| **FUTURE-001** | Floating-point support (software IEEE) | XL | codegen, stdlib | v3.0+ | No FPU on 45GS02; 6-8 weeks est. |
| **FUTURE-002** | Lambda expressions (C++ style) | L | parser, codegen | v3.0+ | 4-6 weeks est. |
| **FUTURE-003** | Generator functions (Python style) | L | parser, codegen | v3.0+ | 8-10 weeks est. |
| **FUTURE-004** | Loop unrolling optimization | M | optimizer | v2.5+ | Small fixed iterations only |
| **FUTURE-005** | SIMD code generation | M | codegen | v2.5+ | 32-bit register patterns |
| **FUTURE-006** | Link-Time Code Generation (LTO) | L | optimizer | v3.0+ | Cross-TU inlining |
| **FUTURE-007** | Static analysis & sanitizers | L | tool | v3.0+ | UB detection, memory sanitizer |


---

## Planning Reference

### Effort by Category (v1.0 → v2.0)

| Category | Estimated Hours | Weeks | High-Effort Items |
|---|---|---|---|
| **Parser** (lang features) | 200–280 | 5–7 | Designated initializers, VLA, long long |
| **Codegen** (translation) | 240–320 | 6–8 | Inline expansion, thunks, banked code |
| **Optimizer** | 280–360 | 7–9 | Register tracking phases, inter-TU opt |
| **Assembler** | 160–200 | 4–5 | Register tracking, peephole opt |
| **Standard Library** | 120–160 | 3–4 | Platform libs (mega65.h, fcb.h) |
| **Linker** | 200–240 | 5–6 | Thunks, banking, PIC, lazy loading |
| **Testing** | 160–200 | 4–5 | Validation suite, benchmarking |
| **Documentation** | 80–120 | 2–3 | API reference, guides, migration |
| **Bug fixes & QA** | 120–160 | 3–4 | Blockers, regressions, edge cases |
| **TOTAL** | **1560–2040** | **39–51** | — |

**Notes:**
- Single developer: 26–36 weeks at 40h/week
- Parallelization (2 devs): could halve timeline
- Complex features (float, LTO) excluded from v1.0–v2.0

---

## Development Cadence & Resource Estimates

### Proposed Release Schedule
- **v1.0**: May–June 2026 (4–6 weeks from current state)
- **v1.1**: July 2026 (+3–4 weeks after v1.0)
- **v1.2**: August 2026 (+4–5 weeks)
- **v1.3**: September 2026 (+3–4 weeks)
- **v1.4**: October–November 2026 (+6–8 weeks)
- **v2.0 Beta**: December 2026 (+8–12 weeks)
- **v2.0 Release**: January 2027 (+4 weeks)

**Total: ~10–12 months from v0.99 → v2.0**

### Effort Estimates (Single Developer)
| Phase | Feature Count | Estimated Hours | Weeks |
|-------|---------------|-----------------|-------|
| 1.0 Release | 10 items | 80–120 | 2–3 |
| 1.1 | 12 items | 120–160 | 3–4 |
| 1.2 | 3 items (reg. opt.) | 160–200 | 4–5 |
| 1.3 | 5 items (calling conv.) | 120–160 | 3–4 |
| 1.4 | 5 items (dynamic link) | 240–320 | 6–8 |
| 2.0 Beta | 10+ items | 320–480 | 8–12 |
| **Total** | **~45 features** | **~1040–1440 hrs** | **26–36 weeks** |

**Notes:**
- Assumes continuous development with minimal external interruptions
- Includes testing, documentation, and bug fixes
- Parallelization (2+ developers) could halve timeline
- Complex features (float, LTO) have wider uncertainty ranges

---

## Success Metrics & Release Criteria

### v1.0 Readiness Checklist
- [x] All 1.0 blockers resolved (V1.0-001, V1.0-002, V1.0-003)
- [x] All 1.0 should-fix items completed (V1.0-004 through V1.0-013)
- [ ] Regression test suite passes 100%
- [ ] Documentation complete and reviewed
- [ ] Man pages generated and installed
- [ ] No critical bugs in alpha/beta testing (7+ days)

### v2.0 Readiness Checklist
- [ ] All v1.0–v1.4 features complete
- [ ] C99 compatibility achieved (except float, VLA)
- [ ] Comprehensive test coverage (50+ test programs)
- [ ] Performance benchmarks documented
- [ ] Community feedback period (2 weeks public beta)
- [ ] Stable API (no breaking changes to cc45/ca45 flags or library headers)

---

## Risk & Mitigation

### High-Risk Items
1. **Float support** — software implementation on 8-bit target (deferred to v3.0)
2. **Dynamic linking** — complex relocation and loading logic (Phase 5)
3. **Banked segments** — requires hardware banking knowledge (Phase 5)

**Mitigations:**
- Prioritize simpler features first (stdlib, optimizations)
- Validate each phase with extensive testing before next
- Maintain escape hatch: deferred features have clear exit criteria

### Technical Debt
- **Known bugs** tracked in root `todo.md` (sprintf %ld, ConstantFolder branches)
  - Must fix before v1.0 — blockers for real-world C code
- **Assembler size drift** in complex programs — Phase 1 passthrough overhead
  - Monitor with stress tests; fix if >10KB programs affected

---

## Version History & Stability Guarantees

### API Stability
- **v1.0–v1.x**: Stable compiler/assembler command-line flags, library API
  - Minor additions allowed (new stdlib functions, new `-f` flags)
  - No breaking changes to existing flags/headers
- **v2.0+**: Major version only on breaking changes

### Deprecation Policy
- Announce deprecation 1 version in advance
- Remove in version N+2
- Example: deprecate in v1.2, remove in v1.4

---

## How to Use This Roadmap

1. **For release planning**: Refer to the phase durations and success criteria
2. **For priority setting**: Focus on 1.0 blockers first, then should-fix items
3. **For community communication**: Reference phase numbers and target dates
4. **For risk assessment**: Check high-risk items and mitigations

**Last Updated:** 2026-05-08 (all V1.0 items validated complete)  
**Maintainer:** Craig Taylor (CTalkobt)
