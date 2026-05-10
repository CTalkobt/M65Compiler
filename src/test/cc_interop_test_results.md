# Calling Convention Interop Test Results

## Test Date
2026-05-09 (post calling convention enforcement implementation)

## Test Suite
Comprehensive tests covering:
1. Stack → ZP calls (ALLOWED)
2. ZP → Stack calls (FORBIDDEN)
3. Indirect calls through function pointers
4. Struct returns across conventions
5. Long (32-bit) returns across conventions
6. Parameter passing edge cases
7. Variadic calls from ZP functions
8. Assembly-level `.func_flags` directive

## Test Execution Status

### Passing Tests ✓

**Test 7: Function attribute verification in .o45 objects**
- ✓ ZP function correctly marked with `zp_call` flag
- ✓ Stack function correctly marked with `stack_call` flag
- All function attributes correctly embedded in `.o45` relocatable objects

### Failing Tests (Issues Identified)

**Test 1: Stack → ZP calls**
- Status: Code works correctly; test assertion needs fixing
- Issue: Grep pattern for memory output needs adjustment
- Result: Actual memory: `1E 00 9B 00 23 FF` (correct values)

**Test 2: ZP → Stack calls (CRITICAL)**
- Status: ENFORCEMENT NOT WORKING
- Root Cause: buildCallGraph() only tracks external symbol relocations (SEG_EXTERNAL), not calls within same object or to functions in TEXT segment
- Line: O45Linker.cpp:532 `if (r.segment != SEG_EXTERNAL) continue;`
- Impact: ZP→Stack calls within a single compilation unit are NOT detected and rejected
- Expected: Linker error "calling convention mismatch"
- Actual: Linker succeeded (no error)

**Test 3-6: Execution issues**
- Tests compile correctly but mmemu verification fails
- Cause: Test harness grep pattern not matching mmemu output format
- Actual Results: Programs appear to run (reach `BRK` instruction)

**Test 8: Assembly .func_flags**
- Issue: Test file uses `phw.sp` syntax which may not be correct
- Error: "Expected expression at line 22"

## Root Causes

### 1. **Critical: Incomplete Call Graph Construction**
The enforcement mechanism has a fundamental gap: it only detects calls to external symbols. Calls to functions defined in the same object file (even if exported) are not tracked in the call graph.

**Fix Required:**
- Modify buildCallGraph() to also scan TEXT relocations
- Handle both:
  - SEG_EXTERNAL: calls to symbols resolved from other objects
  - SEG_TEXT: calls to functions defined in the same object

### 2. **Test Harness Issues (Non-critical)**
- mmemu output format differs slightly from expected
- Need to adjust regex patterns or test framework
- All actual program outputs appear correct based on manual inspection

## Recommendation

**BLOCKER**: The ZP→Stack enforcement is incomplete. Before v1.0 release:

1. **Fix O45Linker::buildCallGraph()** to detect TEXT relocations
   - Impact: ~20 lines of code
   - Risk: Low (only expands existing logic)
   - Test: Re-run calling convention tests

2. **Re-run all interop tests** after fix
   - Verify ZP→Stack calls are rejected
   - Verify all execution results match expected

3. **Add integration test** to CI:
   - Compile with `-c` to object files
   - Link multiple objects with mixed conventions
   - Verify linker correctly rejects mismatches

## Files Modified

- Created: `src/test-resources/test_cc_stack_to_zp.c` (stack→ZP allowed)
- Created: `src/test-resources/test_cc_zp_to_stack.c` (ZP→stack forbidden)
- Created: `src/test-resources/test_cc_struct_return.c`
- Created: `src/test-resources/test_cc_long_return.c`
- Created: `src/test-resources/test_cc_param_edge_cases.c`
- Created: `src/test-resources/test_cc_zp_variadic.c`
- Created: `src/test-resources/test_cc_asm.s`
- Created: `src/test/test_cc_interop.sh`

## Next Steps

1. Fix buildCallGraph() to handle TEXT relocations ⚠️ URGENT
2. Fix test harness assertions
3. Re-run tests to verify all pass
4. Add automated interop tests to `make test` target
