#!/bin/bash

# Simple test script for cc45 compiler

CC="./bin/cc45"
AS="./bin/ca45"

TEST_FILES=(
    "src/test-resources/hello.c"
    "src/test-resources/test_proc.c"
    "src/test-resources/test_control_flow.c"
    "src/test-resources/test_for.c"
    "src/test-resources/test_do_while.c"
    "src/test-resources/test_char.c"
    "src/test-resources/test_ops.c"
    "src/test-resources/test_recursion.c"
    "src/test-resources/test_complex_math.c"
    "src/test-resources/primes.c"
    "src/test-resources/hanoi.c"
    "src/test-resources/guess.c"
    "src/test-resources/hello_world.c"
    "src/test-resources/test_ptr_arith.c"
    "src/test-resources/test_constant_folding.c"
    "src/test-resources/test_ptr_ptr.c"
    "src/test-resources/test_inc_dec.c"
    "src/test-resources/test_char_flow.c"
    "src/test-resources/test_opt_struct.c"
    "src/test-resources/test_preprocessor_ext.c"
    "src/test-resources/test_preproc_final.c"
    "src/test-resources/test_preproc_macros.c"
    "src/test-resources/test_preproc_v3.c"
    "src/test-resources/test_static_assert_pass.c"
    "src/test-resources/test_break_continue.c"
    "src/test-resources/test_switch.c"
    "src/test-resources/test_switch_continue.c"
    "src/test-resources/test_case_range.c"
    "src/test-resources/test_packed_struct.c"
    "src/test-resources/test_interrupt.c"
    "src/test-resources/test_naked.c"
    "src/test-resources/test_i8_arith.c"
    "src/test-resources/test_optimizations.c"
    "src/test-resources/test_dma.c"
    "src/test-resources/test_literal_promotion.c"
    "src/test-resources/test_shift_edge.c"
    "src/test-resources/test_compound_types.c"
    "src/test-resources/test_repeat.c"
    "src/test-resources/test_regparm.c"
    "src/test-resources/test_global_vars.c"
    "src/test-resources/test_anon_struct.c"
    "src/test-resources/test_union.c"
    "src/test-resources/test_arrays.c"
    "src/test-resources/test_ternary.c"
    "src/test-resources/test_global_res.c"
    "src/test-resources/test_generic.c"
    "src/test-resources/test_sub.c"
    "src/test-resources/test_compound.c"
    "src/test-resources/test_for_decl.c"
    "src/test-resources/test_modulo.c"
    "src/test-resources/test_typedef.c"
    "src/test-resources/test_enum.c"
    "src/test-resources/test_zp_clobber.c"
    "src/test-resources/test_dead_store.c"
    "src/test-resources/test_char_local.c"
    "src/test-resources/test_func_args.c"
    "src/test-resources/test_global_struct.c"
    "src/test-resources/test_multi_call.c"
    "src/test-resources/test_mixed_types.c"
    "src/test-resources/test_nested_struct.c"
    "src/test-resources/test_volatile_dse.c"
    "src/test-resources/test_struct_param.c"
    "src/test-resources/test_inline_asm.c"
    "src/test-resources/test_short_circuit.c"
    "src/test-resources/test_type_promotion.c"
    "src/test-resources/test_string_literal.c"
    "src/test-resources/test_operator_precedence.c"
    "src/test-resources/test_cast.c"
    "src/test-resources/test_narrowing_warn.c"
    "src/test-resources/test_func_identifier.c"
    "src/test-resources/test_many_params_locals.c"
    "src/test-resources/test_ptr_precedence.c"
    "src/test-resources/test_bool.c"
    "src/test-resources/test_opt_load_store.c"
    "src/test-resources/test_restrict.c"
    "src/test-resources/test_pragma_heap.c"
    "src/test-resources/test_malloc.c"
    "src/test-resources/test_static.c"
    "src/test-resources/test_register.c"
    "src/test-resources/test_array_init.c"
    "src/test-resources/test_funcptr.c"
    "src/test-resources/test_variadic.c"
    "src/test-resources/test_short.c"
    "src/test-resources/test_struct_return.c"
    "src/test-resources/test_bitfield.c"
    "src/test-resources/test_compound_literal.c"
    "src/test-resources/test_long.c"
    "src/test-resources/test_preprocessor.c"
    "src/test-resources/test_expr_reentrant.c"
)

mkdir -p build/test

failed=0

for f in "${TEST_FILES[@]}"; do
    if [ ! -f "$f" ]; then
        echo "Skip $f (not found)"
        continue
    fi
    
    filename=$(basename "$f")
    s_file="build/test/${filename%.c}.s45"
    bin_file="build/test/${filename%.c}.bin"
    
    echo "Testing $f..."
    
    # 1. Compile
    $CC -v "$f" -o "$s_file"
    if [ $? -ne 0 ]; then
        echo "FAIL: Compilation failed for $f"
        failed=$((failed + 1))
        continue
    fi
    
    # 2. Assemble
    $AS "$s_file" -o "$bin_file"
    if [ $? -ne 0 ]; then
        echo "FAIL: Assembly failed for $f"
        failed=$((failed + 1))
        continue
    fi
    
    echo "SUCCESS: $f"
done

# --- Test -I flag with space-separated path ---
echo "Testing -I flag (space-separated)..."
$CC -v -I lib/include src/test-resources/test_include_flag.c -o build/test/test_include_flag.s45
if [ $? -ne 0 ]; then
    echo "FAIL: -I lib/include (space) failed"
    failed=$((failed + 1))
else
    $AS build/test/test_include_flag.s45 -o build/test/test_include_flag.bin
    if [ $? -ne 0 ]; then
        echo "FAIL: Assembly failed for test_include_flag.s45 (-I space)"
        failed=$((failed + 1))
    else
        echo "SUCCESS: -I flag (space-separated)"
    fi
fi

echo "Testing -I flag (no space)..."
$CC -v -Ilib/include src/test-resources/test_include_flag.c -o build/test/test_include_flag_nospace.s45
if [ $? -ne 0 ]; then
    echo "FAIL: -Ilib/include (no space) failed"
    failed=$((failed + 1))
else
    echo "SUCCESS: -I flag (no space)"
fi

if [ $failed -eq 0 ]; then
    echo "All tests passed!"
    exit 0
else
    echo "$failed tests failed."
    exit 1
fi
