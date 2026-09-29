# v1.0.5 Known Bugs

## Initialized global pointers placed in BSS instead of DATA — FIXED

**Status**: Fixed (2026-09-28)
**Severity**: High

**Root cause**: `ConstantFolder::visit(TranslationUnit)` at line 80 called `decl->accept(*this)` which visited each top-level declaration but never replaced the original AST node with the folded result. The `visit(VariableDeclaration)` handler moved the initializer out of the original node via `fold(std::move(node.initializer))`, creating a new node in `lastStmt`, but since `visit(TranslationUnit)` didn't use `lastStmt` to replace, the original node lost its initializer (moved away, now null).

**Fix**: Changed `visit(TranslationUnit)` to accept-and-replace: visit each declaration, and if `lastStmt` is set (folded replacement), swap it into the AST. This ensures initializers survive constant folding.

## alloca() incompatible with cc45 calling convention

**Status**: Open
**Severity**: Medium
**Found**: 2026-09-28

**Issue**: `alloca()` in `lib/stdlib/alloca.s45` pops arguments from the hardware stack (`pla`), but cc45 passes function arguments on the software parameter stack (`__sp_base`). This causes stack corruption and BRK.

**Workaround**: Use local arrays (compiler-managed stack frames) instead of `alloca()`.

**Fix needed**: Rewrite `alloca.s45` to read size from the software parameter stack, or implement alloca as a compiler intrinsic.

## Function call with local arrays doesn't return correctly — FIXED

**Status**: Fixed (2026-09-28)
**Severity**: High

**Root cause**: Inlined function bodies that end without an explicit `return` statement left the merge block disconnected. The dead block eliminator then removed the merge block and all subsequent code (including statements after the inlined call).

**Fix**: Added fall-through BR to merge label after visiting inlined body, at both inline expansion sites in `visit(FunctionCall)` in IRBuilder.cpp.
