# Cross-Module Optimization (Phases 91, 99, 100) — Status

**Related:** `optimizations.md`, `calling-conventions.md`

## Summary

The source tree contains code for interprocedural optimization (Phase 91),
cross-module far-variable bank assignment (Phase 99) and link-time
optimization coordination (Phase 100). **None of it changes the code that
cc45 or ln45 produces.**

- All of it is in `lib45-phases.a`, which no tool links: the phase classes,
  and the phase code that used to sit in live files (O45Linker's analysis
  passes in `O45LinkerPhases.cpp`, `IRCodeGenPhases.cpp`,
  `O45WriterPhases.cpp`, `OptimizeStagePhases.cpp`, `CodegenStagePhases.cpp`).
  `make all` still builds it, so it keeps compiling.
- What used to run (function profiling in cc45, the analysis passes and
  `CrossModuleOptimizer` in ln45) only collected data nobody read or printed
  messages; those calls were removed (2026-10-12), which took 0.4 MB off cc45
  and 0.8 MB off ln45 and objdump45.
- The environment variables `CC45_IPO`, `CC45_BANK_ANALYSIS`, `CC45_LTCO`,
  `CC45_LTCO_STRATEGY` are not read anywhere.
- The pragmas `#pragma cc45 no_ipo`, `no_bank_opt`, `no_ltco` are dropped by
  the preprocessor without a diagnostic (like any unknown `#pragma cc45`).
- There is no `-O` level or `-f` option that enables any of it.

Optimizations that really cross function boundaries are done **inside one
translation unit** by cc45 (inlining, removal of unused `static` functions,
devirtualization — see `optimizations.md`). At link time ln45 does
calling-convention bridge thunks and SAC safety checks (see
`calling-conventions.md`), not optimization.

## Component status

| Component | Files | Linked? (before 2026-10-12) | Effect on output |
|---|---|---|---|
| `IPOProfiler` | `IPOProfiler.cpp/.hpp` | No (cc45) | Ran; data was discarded (below) |
| `GlobalFunctionDatabase` | `GlobalFunctionDatabase.cpp/.hpp` | No (cc45, filled by `IPOProfiler`) | None — nothing read it |
| `IPOAnalyzer` | `IPOAnalyzer.cpp/.hpp` | No | None |
| `SpecializationCodeGenerator`, `SpecializationOptimizer`, `IRSpecializationGenerator` | same names | No | None |
| `IRCodeGen::collectIPOHints()`, the IPO hints option of `O45Writer` | `IRCodeGenPhases.cpp`, `O45WriterPhases.cpp` | No (cc45) | Never called; `.o45` files carry no IPO hints section |
| `IRCodeGen::setDeadCodeFunctions` / inline-only set | `IRCodeGen.hpp` | cc45 | Never called; the sets are always empty |
| `OptimizeStage::performCrossModuleOptimization`, `CodegenStage::applyIPOSpecialization` | `OptimizeStagePhases.cpp`, `CodegenStagePhases.cpp` | No (cc45 at `-O2`+) | Counted leaf/small functions and printed them with `-vv`; nothing else |
| `O45Linker` analysis passes: `analyzeIPOHints`, `applyIPOHints`, `analyzeConstantParameters`, `analyzeIRMetadata`, `analyzeSpecializations`, `analyzeCallRouting`, `analyzeInlining`, `generateDispatchers`, `integrateDispatcherAssembly`, `emitDispatcherAssemblyOutput`, and the 46 report/benchmark/regression/tuning/profile functions only they reached | `O45LinkerPhases.cpp` | No (ln45, objdump45) | Analysis and log lines only; no code was added, removed or redirected |
| `CrossModuleOptimizer` (Phase 91.4) | `CrossModuleOptimizer.cpp/.hpp` | No (ln45) | Ran in `ln45_main.cpp` **after** `linker.link()` had produced the binary, on a copy of the linker; only printed `[Phase 91.4] ...` lines to stderr |
| Phase 99: `CrossModuleVariableDatabase`, `MemoryBankAssigner`, `BankLayoutGenerator`, `BankSetupOptimizer`, `BankAwareCodeGenerator` | `src/main/*.cpp/.hpp` | No | None |
| Phase 100: `OptimizationHintCollector`, `OptimizationConstraintResolver`, `LinkTimeOptimizationCoordinator`, `OptimizationDependencyAnalyzer`, `LinkTimeOptimizationCodeGenerator`, `LinkTimeOptimizationValidator` | `src/main/*.cpp/.hpp` | No | None |

## What used to run, in detail

The following describes the code as it was linked before 2026-10-12; the calls are gone, the code is in `lib45-phases.a`.

### cc45: function profiling

`IRBuilder` owns an `IPOProfiler`. For each function definition it calls
`recordFunctionDefinition(name, paramCount, localVarCount, hasNoCalls,
isLeafCandidate, module)` and for each call `recordFunctionCall(caller, callee,
argCount, hasConstantArgs)`. At the end of `IRBuilder::generate()`
`finalizeProfiles()` stores one `GlobalFunctionProfile` per function in the
`GlobalFunctionDatabase` singleton:

- `codeSize` is an estimate, `20 + 10*params + 5*locals + 10*calls made`, not a
  measured size;
- `isLeaf` = makes no calls and has no locals;
- `callSites` = the distinct callers in this file; `hasExternalCaller` is always
  `true` and `isDeadCode` always `false`.

The process then ends without reading the database.

### ln45: analysis and messages

During `O45Linker::link()` the passes listed in the table look at function
attributes (`.func_flags`, `.param_sizes`), the call graph and any IPO hints
section (there never is one, so every link reports
`DEBUG: IPO Hints: Aggregated 0 functions from N objects`). `generateDispatchers`
and the two dispatcher-assembly steps only build text and log
`ln45: No dispatcher assembly to integrate/emit`; the comments in
`integrateDispatcherAssembly` state that assembling and linking the dispatchers
is not implemented.

After the link, `ln45_main.cpp` runs `CrossModuleOptimizer`
(`analyzeSpecializations`, `generateCallRouting`, `createDispatchers` on a copy
of the linker). The reports (`getSpecializationReport`,
`getCallRoutingReport`) are behind `if (false)`. The output file is written
from the binary returned by `link()` before any of this.

These messages appeared on stderr on every link, including cc45's one-step build (no longer):

```
DEBUG: IPO Hints: Aggregated 0 functions from 3 objects
INFO: IPO Hints Applied: 0 candidates, 0 leaf functions, 0 validated, 0 mismatches
ln45: No dispatcher assembly to integrate
ln45: No dispatcher assembly to emit
[Phase 91.4] Analyzing cross-module specialization opportunities...
[Phase 91.4] Creating dispatcher stubs for multi-specialization...
```

## Earlier claims that do not hold

| Claim | Actual |
|---|---|
| "19% code reduction on `test_phase91_validation.c`", "11.8% on `test_phase91_tuning.c`" | One-step PRG sizes with the current toolchain: validation 526 bytes at `-O0`/`-O1`, 571 at `-O2`; tuning 1249 at `-O0`/`-O1`, 1610 at `-O2`. The difference comes from ordinary inlining, which makes these programs larger |
| Three-tier inlining heuristics (single caller < 20 bytes, leaf < 10, tiny with ≤ 3 call sites) | Exists only in the unlinked `IPOAnalyzer`. cc45 inlining uses statement count (≤ 20) and IR instruction count (≤ `max_inline_ir_size`, default 11), see `optimizations.md` |
| Specialized variants such as `_compute_spec_5_3` and dispatcher stubs | Never generated |
| Cross-module dead-code elimination | Not done. cc45 removes unused `static` functions within one file; ln45 pulls library members on demand but does not drop unused functions from linked objects (it warns `unused global symbol`) |
| Bank assignment, co-location, bank-setup hoisting (Phase 99) | Not connected to cc45 or ln45 |
| Link-time hint coordination, synergy bonuses (Phase 100) | Not connected to cc45 or ln45 |
| Memory/time overhead figures, threshold-tuning results, "production-ready" | Not reproducible; the code does not run |

## How to check

Scratch files below are in any empty directory.

```bash
# None of the phase code is in the binaries
nm -C bin/cc45 bin/ln45 bin/ca45 | grep -E 'IPOAnalyzer|IPOProfiler|CrossModuleOptimizer|generateDispatchers|MemoryBankAssigner'
# (prints nothing; ar t lib45/lib45-phases.a lists it)

# The env vars and pragmas are not read
grep -rn 'CC45_IPO\|CC45_LTCO\|CC45_BANK_ANALYSIS\|no_ipo\|no_ltco\|no_bank_opt' src/main include

# The pragmas do not change the output: put them on lines 1-3 of a/prog.c and
# leave lines 1-3 empty in b/prog.c (same file name and line numbers), then
(cd a && cc45 -S prog.c -o ../a.s) ; (cd b && cc45 -S prog.c -o ../b.s) ; cmp a.s b.s   # identical

# Size at different levels (one-step build)
cc45 -O0 src/test-resources/test_phase91_validation.c -o v0.prg
cc45 -O2 src/test-resources/test_phase91_validation.c -o v2.prg
ls -l v0.prg v2.prg
```

The design notes for the unconnected code (phases 92, 99 and 100) were
removed; they described intended behaviour, not what the tools do. They are
in the git history.
