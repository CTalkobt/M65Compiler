# TODO

## Known Bugs (from root todo.md)

- [x] **Assembler simulated op size drift**: Fixed in commits 3e1e5a0 and f38808e.

## Calling Convention Safety

### Per-function convention attribute in .o45
- [ ] Add bit 2 to `O45FuncAttr.flags`: 0=stack, 1=zpcall
- [ ] Compiler: set bit 2 for zpcall/fastcall functions
- [ ] Extend `.extern` with optional convention hint (import-side attribute)
- [ ] Linker: check convention match during symbol resolution
- [ ] Linker: emit warning/error on mismatch

### Linker-generated convention thunks
- [ ] On convention mismatch, generate proxy thunk instead of erroring
- [ ] Stack-to-ZP thunk: pop stack args into ZP slots, JSR real function, RTS
- [ ] ZP-to-stack thunk: read ZP slots, push onto stack, JSR real function
- [ ] Emit thunk into text segment with synthetic symbol (`_func$thunk`)
- [ ] Patch caller's relocation to point to thunk
- [ ] Handle return conventions (AX for int, AXYZ for long, hidden-ptr for struct)
- [ ] `--no-thunks` flag for hard errors instead
- [ ] `-Wthunk` to warn when thunks are generated (performance diagnostic)

## Fine-Grained Register Invalidation

### Phase 2: Selective invalidation at call sites (intra-TU)
- [ ] Replace `invalidateRegs()` after `jsr` with `invalidateFromClobbers()`
- [ ] Merge transitive clobbers (caller inherits callee's masks)
- [ ] Conservative fallback for forward refs / extern / indirect calls

### Phase 3: Emit accurate directives
- [ ] Emit `.func_flags leaf` for leaf functions
- [ ] Add `.func_flags` directive to assembler

### Phase 4: Inter-TU optimization
- [ ] Header annotations (`#pragma clobbers reg(...) flag(...)`)
- [ ] Or: auto-generated clobber headers from .o45 (`ln45 -emit-clobbers`)

### Phase 5: Assembler optimizer
- [ ] `AssemblerOptimizer::optimize()` uses `ProcContext::regClobbersMask` at JSR

---

## 1.0 Blockers

### Documentation
- [ ] `getchar()` listed in stdlib.md without implementation status; absent from stdio.h

## 1.0 Should-Fix

### Language
- [ ] Designated initializers: `{.x=1, .y=2}` for structs, `{[2]=3}` for arrays
- [ ] `_Alignas` for local (stack) variables
- [x] Type qualifier ordering: all orderings now accepted (globals, locals, params, return types, typedefs)
- [ ] Enforce `&register_var` as compile error (C standard requirement)
- [ ] Integer literal overflow checking (Lexer.cpp:246)

### Preprocessor
- [x] `#pragma once` support (replaced non-standard `#pragma include_once`)
- [ ] `_Pragma()` currently silently discarded — should convert to `#pragma`

### Standard Library (minimal C subset)
- [ ] `ctype.h`: `isupper`, `islower`, `isxdigit`, `ispunct`, `isblank`, `iscntrl`
- [ ] `string.h`: `strstr`, `strtok`, `strncat`, `strpbrk`, `strspn`, `strcspn`
- [ ] `stdlib.h`: `strtol`, `strtoul`
- [ ] `stdio.h`: `getchar`

## Future Optimizations

- [ ] Dead code elimination after unconditional infinite loops (`while(1){}` makes subsequent `return` unreachable)

## 1.0 Won't-Fix (Documented Limitations)

- `float` / `double` types — no FPU on 45GS02, software float is a post-1.0 feature
- `inline` function expansion — accepted as keyword, no-op (calls normally via JSR)
- `void *` arithmetic — must cast to concrete pointer first
- Fine-grained invalidation phases 2-5 — optimization, not correctness
