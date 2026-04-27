# cc45 / ca45 TODO

## ConstantFolder / CodeGenerator: Loop results eliminated

The ConstantFolder propagates constants through loop bodies sequentially, computing
final values for variables like `sum` and `i`. After folding, assignments like
`results[3] = sum` get folded to `results[3] = 10` (a constant). However, the
CodeGenerator then fails to emit code for these assignments — the source comment
appears in the .s output but no instructions are generated.

Affected patterns:
- `results[N] = localVar` after a while/do-while/for loop where localVar was
  fully constant-folded
- Likely related to `resultNeeded` flag or `isVariableUsed` optimization
  incorrectly eliminating the store

Workaround: use `-O0` flag to skip constant folding.

## CodeGenerator: ZP temp registers clobbered by function calls

When generating `results[N] = test_switch(arg)`, the CodeGenerator:
1. Computes the destination address ($4000+N) and stores it in ZP $02/$03
2. Pushes the function argument
3. Calls the function (JSR test_switch)
4. The callee clobbers ZP $02/$03 with its own temporaries
5. After return, stores result via `sta ($02),y` — but $02/$03 is wrong

Fix: save ZP destination pointer to the stack (or a different ZP location)
before the function call, and restore it after.
