# Completed Tasks

## Correctness
- [x] ConstantFolder: Implement constant state merging in `IfStatement` to correctly invalidate constants reassigned in branches.
- [x] Standard Library: Fix `sprintf` `%ld` output by resolving ZP conflict and hardware math races.
- [x] ConstantFolder: Preserve `long` type during constant propagation to prevent variadic argument corruption.
- [x] `limits.h` missing from `lib/include/` (documented as done in stdlib.md but file absent)
- [x] Stack-relative 32-bit `++`/`--` falls through to generic path (CodeGenerator.cpp:3207) — incorrect for `long` locals on stack
- [x] `stddef.h` missing `ptrdiff_t` and `offsetof`

## Documentation
- [x] `doc/stdlib.md` Section 11 stale — claims `static`, `const`, `extern`, function pointers not available (all implemented)

## Fine-Grained Register Invalidation
- [x] Phase 3: Emit accurate directives - Emit `.reg_clobbers` / `.flag_clobbers` with tracked values (both conventions)

## Standard Library (minimal C subset)
- [x] `limits.h`: `CHAR_BIT`, `CHAR_MAX`, `INT_MAX`, `INT_MIN`, `UINT_MAX`, `LONG_MAX`, etc.
- [x] `stddef.h`: `ptrdiff_t`, `offsetof`

## Testing
- [x] Wire up orphaned test: `test_preprocessor.c` (added to `test_compiler.sh`)
- [x] Wire up orphaned test: `test_expr_reentrant.c` (added to `test_compiler.sh`)

## From Previous Plan (restored from truncation)
- [x] ConstantFolder / CodeGenerator: Loop results eliminated
- [x] CodeGenerator: ZP temp registers clobbered by function calls
