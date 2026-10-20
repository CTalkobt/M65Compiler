#!/bin/bash

# Simple test script for cc45 compiler

CC="./bin/cc45 -O2"
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
    "src/test-resources/test_predefined_macros.c"
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
    "src/test-resources/stdlib/test_calloc.c"
    "src/test-resources/stdlib/test_realloc.c"
    "src/test-resources/stdlib/test_strnlen.c"
    "src/test-resources/stdlib/test_strlcpy.c"
    "src/test-resources/stdlib/test_strdup.c"
    "src/test-resources/stdlib/test_strtok_r.c"
    "src/test-resources/stdlib/test_strcspn.c"
    "src/test-resources/stdlib/test_strspn.c"
    "src/test-resources/stdlib/test_strstr.c"
    "src/test-resources/stdlib/test_strpbrk.c"
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
    
    # 2. Assemble: a flat binary; a program that calls the library cannot be
    # one (ca45 rejects the unresolved .extern), so it is assembled as an object
    if ! $AS "$s_file" -o "$bin_file" > build/test/as_flat.log 2>&1; then
        if grep -q "is .extern but no linker resolves it" build/test/as_flat.log &&
           $AS -c "$s_file" -o "${bin_file%.bin}.o45" >/dev/null 2>&1; then
            :
        else
            cat build/test/as_flat.log
            echo "FAIL: Assembly failed for $f"
            failed=$((failed + 1))
            continue
        fi
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
    # (it calls the library: an object, not a flat binary)
    $AS -c build/test/test_include_flag.s45 -o build/test/test_include_flag.o45
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

echo "Testing arithmetic on a struct with no operator..."
printf 'struct S { int a; };\nint f(struct S x, struct S y) { struct S z; z = x / y; return z.a; }\n' > build/test/test_struct_op.c
out=$($CC -c build/test/test_struct_op.c -o build/test/test_struct_op.o45 2>&1)
if [ $? -ne 0 ] && grep -q "invalid operands to binary / ('struct S' has no operator/)" <<< "$out"; then
    echo "SUCCESS: struct / struct without an operator is an error"
else
    echo "FAIL: struct / struct was accepted ($(head -1 <<< "$out"))"
    failed=$((failed + 1))
fi

# A __naked function that returns a value (its asm returns: no branch to an
# epilogue it does not have), and a static one with the specifiers in either
# order: module-private in both files, so the two link together
for f in naked_s1 naked_s2; do
    printf '__naked static unsigned char *here(void) {\n    __asm__("lda #<mark");\n    __asm__("ldx #>mark");\n    __asm__("rts");\n    __asm__("mark: .byte 1");\n}\nstatic __naked void nop(void) { __asm__("rts"); }\nunsigned char *%s(void) { nop(); return here(); }\n' $f > build/test/test_$f.c
done
printf 'unsigned char *naked_s1(void);\nunsigned char *naked_s2(void);\nint main(void) { return *naked_s1() + *naked_s2() == 2 ? 0 : 1; }\n' > build/test/test_naked_main.c
if $CC -c build/test/test_naked_s1.c -o build/test/test_naked_s1.o45 >/dev/null 2>&1 &&
   $CC -c build/test/test_naked_s2.c -o build/test/test_naked_s2.o45 >/dev/null 2>&1 &&
   $CC -c build/test/test_naked_main.c -o build/test/test_naked_main.o45 >/dev/null 2>&1 &&
   ./bin/ln45 -basic -o build/test/test_naked_static.prg build/test/test_naked_main.o45 build/test/test_naked_s1.o45 build/test/test_naked_s2.o45 lib/build/c45.lib >/dev/null 2>&1; then
    echo "SUCCESS: __naked static functions returning a value"
else
    echo "FAIL: __naked static functions returning a value"
    failed=$((failed + 1))
fi

# A function-like macro call whose arguments go on over the next lines (a
# cast's parentheses on the first one): expanded whole, line numbers kept
printf '#define ADD(a, b, c) ((a) + (b) + (c))\nint f(int x) {\n    return ADD(x,\n               2, 3);\n}\nint g(int x) { return ADD(x, (int)(unsigned)x,\n  4); }\nint h = __LINE__;\n' > build/test/test_mlmacro.c
if ./bin/cp45 build/test/test_mlmacro.c -o build/test/test_mlmacro.i >/dev/null 2>&1 &&
   grep -qF "return ((x) + (2) + (3));" build/test/test_mlmacro.i &&
   grep -qF "return ((x) + ((int)(unsigned)x) + (4));" build/test/test_mlmacro.i &&
   grep -qF "int h = 8;" build/test/test_mlmacro.i &&
   $CC -c build/test/test_mlmacro.c -o build/test/test_mlmacro.o45 >/dev/null 2>&1 &&
   ! ./bin/nm45 build/test/test_mlmacro.o45 | grep -q " U _ADD"; then
    echo "SUCCESS: macro calls over several lines"
else
    echo "FAIL: macro calls over several lines ($(grep -v '^ *$' build/test/test_mlmacro.i 2>/dev/null | tr '\n' ' '))"
    failed=$((failed + 1))
fi

# A bitfield wider than its type is an error, also after a comma and unnamed
# (int e : 17 kept 16 bits without a word)
bf_ok=1
for decl in "int a, c : 7, e : 17;" "int : 17;" "char a : 4, b : 9;" "unsigned long long a : 40;"; do
    printf 'struct S { %s } s;\n' "$decl" > build/test/bfw.c
    $CC -c build/test/bfw.c -o build/test/bfw.o45 >/dev/null 2>&1 && bf_ok=0
done
printf 'struct S { int a, c : 7, e : 16; char d : 8; long l : 32, m : 20; int : 0; } s;\n' > build/test/bfw.c
$CC -c build/test/bfw.c -o build/test/bfw.o45 >/dev/null 2>&1 || bf_ok=0
if [ $bf_ok -eq 1 ]; then
    echo "SUCCESS: bitfield widths checked"
else
    echo "FAIL: bitfield widths checked"
    failed=$((failed + 1))
fi

# A static initializer that is not a compile-time constant is an error
# (it was emitted as zeros, #312)
nc_ok=1
for decl in "int g(void); int x = g();|initializer of 'x' is not a compile-time constant" \
            "int a; int y[] = { 1, a };|initializer element of 'y' is not a compile-time constant" \
            "int a; struct { int p, q; } s = { 1, a };|initializer element of 's' is not a compile-time constant"; do
    printf '%s\n' "${decl%%|*}" > build/test/nonconst.c
    $CC -c build/test/nonconst.c -o build/test/nonconst.o45 2>&1 | grep -qF "${decl#*|}" || nc_ok=0
done
if [ $nc_ok -eq 1 ]; then
    echo "SUCCESS: non-constant static initializers are errors"
else
    echo "FAIL: non-constant static initializers are errors"
    failed=$((failed + 1))
fi

# A case label that is not an integer constant expression is an error (it
# was taken as 0, #93)
printf 'int x;\nint g(int v) { switch (v) { case x: return 1; } return 0; }\n' > build/test/caselbl.c
if $CC -c build/test/caselbl.c -o build/test/caselbl.o45 2>&1 | grep -q "case label is not an integer constant expression"; then
    echo "SUCCESS: non-constant case labels are errors"
else
    echo "FAIL: non-constant case labels are errors"
    failed=$((failed + 1))
fi

# A bit-field width that is not a constant, and a failing _Static_assert on
# sizeof of a variable, are errors (#93)
pc_ok=1
printf 'int y;\nstruct D { unsigned b : y; } d;\n' > build/test/bfconst.c
$CC -c build/test/bfconst.c -o build/test/bfconst.o45 2>&1 | grep -q "integer constant expression for the bitfield width of 'b'" || pc_ok=0
printf 'long l;\n_Static_assert(sizeof l == 2, "long is 2");\n' > build/test/saconst.c
$CC -c build/test/saconst.c -o build/test/saconst.o45 2>&1 | grep -q "Static assertion failed.*long is 2" || pc_ok=0
if [ $pc_ok -eq 1 ]; then
    echo "SUCCESS: non-constant bit-field widths and failing static assertions are errors"
else
    echo "FAIL: non-constant bit-field widths and failing static assertions are errors"
    failed=$((failed + 1))
fi

# __zp globals (#67): base-page operands relocated against the variable
# (R_LOWX), absolute ones (ldz, lda abs,y) as its full address (R_WORD); ln45
# rejects base-page uses from code an interrupt handler reaches, a variable
# defined twice, and more than the area can hold (at most $30-$F1, 194 bytes)
zp_ok=1
printf '__zp unsigned char z;\n__zp long q;\nunsigned char f(void) { return z; }\nlong g(void) { return q; }\n' > build/test/zpg.c
if $CC -c build/test/zpg.c -o build/test/zpg.o45 >/dev/null 2>&1; then
    ./bin/nm45 -r build/test/zpg.o45 | grep -q 'R_LOWX *EXTERN *_z ' || zp_ok=0
    ./bin/nm45 -r build/test/zpg.o45 | grep -q 'R_WORD *EXTERN *_q' || zp_ok=0
    ./bin/nm45 build/test/zpg.o45 | grep -q 'A cc45.zpvar._z$' || zp_ok=0
else
    zp_ok=0
fi
printf '__zp unsigned char ticks;\nvoid helper(void) { ticks++; }\n__interrupt void isr(void) { helper(); }\nint main(void) { return ticks; }\n' > build/test/zpisr.c
$CC build/test/zpisr.c -o build/test/zpisr.prg 2>&1 | grep -q "uses the __zp variable '_ticks' and is reachable from interrupt handler" || zp_ok=0
printf '__zp int d;\nint main(void) { return d; }\n' > build/test/zpd1.c
printf '__zp int d;\nint g(void) { return d; }\n' > build/test/zpd2.c
printf '__zp unsigned char b[98];\nint g(void) { return b[1]; }\n' > build/test/zpd3.c
printf '__zp unsigned char a[98];\nint main(void) { return a[1]; }\n' > build/test/zpd4.c
for f in zpd1 zpd2 zpd3 zpd4; do $CC -c build/test/$f.c -o build/test/$f.o45 >/dev/null 2>&1 || zp_ok=0; done
./bin/ln45 -basic -o build/test/zpd.prg build/test/zpd1.o45 build/test/zpd2.o45 lib/build/c45.lib 2>&1 | grep -q "__zp variable '_d' is defined in" || zp_ok=0
./bin/ln45 -basic -o build/test/zpd.prg build/test/zpd4.o45 build/test/zpd3.o45 lib/build/c45.lib 2>&1 | grep -q "__zp variables need 196 bytes" || zp_ok=0
printf 'extern __zp unsigned char ua[];\nint main(void) { return ua[1]; }\n' > build/test/zpu.c
$CC -c build/test/zpu.c -o build/test/zpu.o45 2>&1 | grep -q "needs a size" || zp_ok=0
printf 'int f(int a) { __zp int t = a; return t; }\n' > build/test/zpl.c
$CC -c build/test/zpl.c -o build/test/zpl.o45 2>&1 | grep -q "__zp has no effect on a local variable" || zp_ok=0
if [ $zp_ok -eq 1 ]; then
    echo "SUCCESS: __zp globals: relocations and link checks"
else
    echo "FAIL: __zp globals: relocations and link checks"
    failed=$((failed + 1))
fi

# ~ folds in the constant's width (Issue #289): ~0x8000 and ~0xFFFF are
# 16-bit constants, with no "exceeds 16-bit range" warning
printf 'unsigned int r;\nvoid f(unsigned int p) { r = p & ~0x8000; r = p & ~0xFFFF; }\n' > build/test/bnot.c
if $CC -S build/test/bnot.c -o build/test/bnot.s45 >build/test/bnot.err 2>&1 &&
   ! grep -q "exceeds 16-bit range" build/test/bnot.err; then
    echo "SUCCESS: ~ folds in the constant's width"
else
    echo "FAIL: ~ folds in the constant's width"
    failed=$((failed + 1))
fi

# Constant arguments are stored straight into the parameter block (#290),
# and a long is tested by OR-ing its bytes in place (#295)
printf 'void g(unsigned char a, unsigned char b, unsigned int n);\nvoid f(void) { g(1, 2, 300); }\nunsigned char t(unsigned long v) { if (v) return 1; return 0; }\n' > build/test/codesize.c
if $CC -S build/test/codesize.c -o build/test/codesize.s45 2>/dev/null &&
   awk '/proc _f/,/endproc/' build/test/codesize.s45 | grep -v '^ *[.;]' | tr -s ' ' | tr '\n' '|' |
       grep -q 'lda #1| sta \$10| lda #2| sta \$12| lda #44| ldx #1| sta \$14| stx \$15| jsr _g' &&
   awk '/proc _t/,/endproc/' build/test/codesize.s45 | grep -q 'ora \$13' &&
   ! awk '/proc _t/,/endproc/' build/test/codesize.s45 | grep -q '__zp_scratch'; then
    echo "SUCCESS: constant arguments and wide zero tests"
else
    echo "FAIL: constant arguments and wide zero tests"
    failed=$((failed + 1))
fi

# File-scope redeclarations must agree (Issue #287): a different type or
# array size, static after non-static (and the reverse), a second
# initializer are errors; tentative definitions, extern + definition, a[]
# completed, static then extern are not
rd_ok=1
for src in 'unsigned int a[27];\nstatic unsigned int a;' 'unsigned int a[27];\nunsigned int a;' \
           'extern unsigned int a[27];\nstatic unsigned int a;' 'int a[3];\nint a[4];' \
           'extern int a;\nstatic int a;' 'static int a;\nint a;' 'int a = 1;\nint a = 2;'; do
    printf "$src\n" > build/test/redecl.c
    $CC -c build/test/redecl.c -o build/test/redecl.o45 >/dev/null 2>&1 && rd_ok=0
done
for src in 'int a;\nint a;\nint a = 3;' 'extern int a;\nint a = 1;' 'extern int a[];\nint a[27];' \
           'static int a;\nextern int a;' 'typedef unsigned char u8;\nextern u8 b[4];\nunsigned char b[4];' \
           'extern const char *s;\nconst char *s = "hi";' 'extern int (*fp)(int);\nint (*fp)(int);'; do
    printf "$src\n" > build/test/redecl.c
    $CC -c build/test/redecl.c -o build/test/redecl.o45 >/dev/null 2>&1 || rd_ok=0
done
if [ $rd_ok -eq 1 ]; then
    echo "SUCCESS: conflicting redeclarations of a global are errors"
else
    echo "FAIL: conflicting redeclarations of a global are errors"
    failed=$((failed + 1))
fi

# The constants of byte operations are bytes (Issue #288): p & ~0x80 on an
# unsigned char is and #127, with no "8-bit operand overflow" from ca45
printf 'unsigned char r;\nvoid f(unsigned char p) { r = p & ~0x80; r = p - 300; }\n' > build/test/byteimm.c
if $CC -S build/test/byteimm.c -o build/test/byteimm.s45 2>/dev/null &&
   grep -q "and #127" build/test/byteimm.s45 && ! grep -qE "#-|#300" build/test/byteimm.s45 &&
   ! $CC -c build/test/byteimm.c -o build/test/byteimm.o45 2>&1 | grep -q "operand overflow"; then
    echo "SUCCESS: byte operations have byte constants"
else
    echo "FAIL: byte operations have byte constants"
    failed=$((failed + 1))
fi

# __unlikely / __likely (Issue #75): the branch not expected to run comes
# after the function's rts, the expected path falls through; -O0 and
# -fno-reorder-blocks keep the source order
cat > build/test/likely.c <<'EOF'
void fail(void);
unsigned char n;
void f(unsigned char x) {
    if (__unlikely(x == 0)) fail();
    n = x;
}
EOF
cold_after_rts() {   # does the jsr _fail come after the function's rts?
    awk '/rts/ { r = 1 } /jsr _fail/ { print (r ? "after" : "before"); exit }' "$1"
}
if $CC -S build/test/likely.c -o build/test/likely.s45 2>/dev/null &&
   [ "$(cold_after_rts build/test/likely.s45)" = after ] &&
   $CC -O0 -S build/test/likely.c -o build/test/likely0.s45 2>/dev/null &&
   [ "$(cold_after_rts build/test/likely0.s45)" = before ] &&
   $CC -fno-reorder-blocks -S build/test/likely.c -o build/test/likelyn.s45 2>/dev/null &&
   [ "$(cold_after_rts build/test/likelyn.s45)" = before ]; then
    echo "SUCCESS: __unlikely code after the function's return"
else
    echo "FAIL: __unlikely code after the function's return"
    failed=$((failed + 1))
fi

# Character constants (Issue #300): capitals are $C1-$DA ('S' $D3), and a
# constant is its byte read as a char: negative from $80 up when plain char
# is signed (-fsigned-default without -funsigned-char), as in GCC
cc_ok=1
printf '_Static_assert('"'"'S'"'"' == 0xD3 && '"'"'s'"'"' == 0x53 && '"'"'\\xff'"'"' == 255, "u");\nint x;\n' > build/test/charconst.c
$CC -c build/test/charconst.c -o build/test/charconst.o45 >/dev/null 2>&1 || cc_ok=0
$CC -fsigned-default -funsigned-char -c build/test/charconst.c -o build/test/charconst.o45 >/dev/null 2>&1 || cc_ok=0
printf '_Static_assert('"'"'S'"'"' == -45 && '"'"'s'"'"' == 0x53 && '"'"'\\xff'"'"' == -1, "s");\nint x;\n' > build/test/charconst.c
$CC -fsigned-default -c build/test/charconst.c -o build/test/charconst.o45 >/dev/null 2>&1 || cc_ok=0
if [ $cc_ok -eq 1 ]; then
    echo "SUCCESS: character constants: capitals \$C1-\$DA, signed with a signed char"
else
    echo "FAIL: character constants: capitals \$C1-\$DA, signed with a signed char"
    failed=$((failed + 1))
fi

# Function pointer parameters with attributes after the name and array
# suffixes (int *p __attribute__((noescape)), const int a[]): accepted
printf 'void (*cb)(int *p __attribute__((noescape)));\nint (*sum)(const int a[], int n __attribute__((unused)));\nint x;\n' > build/test/fpattr.c
if $CC -c build/test/fpattr.c -o build/test/fpattr.o45 >/dev/null 2>&1; then
    echo "SUCCESS: function pointer parameters with attributes and array suffixes"
else
    echo "FAIL: function pointer parameters with attributes and array suffixes"
    failed=$((failed + 1))
fi

# Size (Issues #296, #297): no ldx #0 (the high byte of a char) before a
# ZP-convention call, whose arguments are in $10-$2F; no inx / dex
# restoring the flags of a dropped load at a label whose code sets them
cat > build/test/szchk.c <<'SRC'
unsigned int buf_len(unsigned int l);
void put(unsigned char c);
unsigned int cur_col, cur_line;
unsigned char k;
void f(unsigned char c) { if (c) cur_col = buf_len(cur_line); cur_line = 0; put(k); }
SRC
if $CC -S build/test/szchk.c -o build/test/szchk.s45 2>/dev/null &&
   ! grep -qE '^\s*inx$' build/test/szchk.s45 &&
   grep -A1 'lda _k' build/test/szchk.s45 | grep -q 'sta \$10'; then
    echo "SUCCESS: no dead ldx before ZP calls, no dead flag restores"
else
    echo "FAIL: no dead ldx before ZP calls, no dead flag restores"
    failed=$((failed + 1))
fi

# Purity inference (-fipa-pure-const): a static function computing only from
# its argument is const, so sq(a) + sq(a) calls it once; not with
# -fno-ipa-pure-const, nor for one that reads a global
printf '__attribute__((noinline)) static int sq(int x) { return x * x; }\nunsigned char g;\n__attribute__((noinline)) static int rd(int x) { return x + g; }\nint f(int a) { return sq(a) + sq(a) + rd(a) + rd(a); }\n' > build/test/pinfer.c
if $CC -Roptir -c build/test/pinfer.c -o build/test/pinfer.o45 2>&1 | grep -q "reuses the result of a call of _sq" &&
   ! $CC -Roptir -c build/test/pinfer.c -o build/test/pinfer.o45 2>&1 | grep -q "reuses the result of a call of _rd" &&
   ! $CC -fno-ipa-pure-const -Roptir -c build/test/pinfer.c -o build/test/pinfer.o45 2>&1 | grep -q "reuses the result"; then
    echo "SUCCESS: purity inference"
else
    echo "FAIL: purity inference"
    failed=$((failed + 1))
fi

# break N (Issue #71): more levels than the loops/switches around it, 0 or
# a level over 255 are errors; break 1 and a switch as a level are not
bn_ok=1
for src in 'int f(void){ int i; for(i=0;i<3;i++) break 2; return i; }' \
           'int f(int x){ int i; for(i=0;i<3;i++) switch(x){ case 1: break 3; } return i; }' \
           'int f(void){ int i; for(i=0;i<3;i++) break 0; return i; }' \
           'int f(void){ int i; for(i=0;i<3;i++) break 256; return i; }' \
           'int f(void){ int i; for(i=0;i<3;i++){ int g(void){ break 2; return 0; } g(); } return i; }'; do
    printf '%s\n' "$src" > build/test/breakn.c
    $CC -c build/test/breakn.c -o build/test/breakn.o45 >/dev/null 2>&1 && bn_ok=0
done
for src in 'int f(void){ int i; for(i=0;i<3;i++) break 1; return i; }' \
           'int f(int x){ int i; for(i=0;i<3;i++) switch(x){ case 1: break 2; } return i; }' \
           'int f(void){ int i, j; for(i=0;i<3;i++) for(j=0;j<3;j++) break 2; return i + j; }'; do
    printf '%s\n' "$src" > build/test/breakn.c
    $CC -c build/test/breakn.c -o build/test/breakn.o45 >/dev/null 2>&1 || bn_ok=0
done
if [ $bn_ok -eq 1 ]; then
    echo "SUCCESS: break N levels are checked"
else
    echo "FAIL: break N levels are checked"
    failed=$((failed + 1))
fi

# A local or parameter that shadows a const global is writable (Issue #285);
# the global is read-only again when the local's scope ends
sh_ok=1
for src in "const char cr = 13; unsigned char f(unsigned char c) { unsigned char cr; cr = c; cr++; return cr; }" \
           "extern const char cr; unsigned char f(unsigned char cr) { cr = 1; return cr; }" \
           "const char i = 3; int f(void) { int s = 0; for (int i = 0; i < 4; i++) s += i; return s; }"; do
    printf '%s\n' "$src" > build/test/cshadow.c
    $CC -c build/test/cshadow.c -o build/test/cshadow.o45 >/dev/null 2>&1 || sh_ok=0
done
for src in "const char cr = 13; void f(void) { { char cr; cr = 2; } cr = 1; }" \
           "const char cr = 13; void g(char cr) { cr = 1; } void f(void) { cr = 1; }" \
           "const char i = 3; void f(void) { for (int i = 0; i < 4; i++) ; i = 1; }"; do
    printf '%s\n' "$src" > build/test/cshadow.c
    $CC -c build/test/cshadow.c -o build/test/cshadow.o45 2>&1 | grep -q "read-only location" || sh_ok=0
done
if [ $sh_ok -eq 1 ]; then
    echo "SUCCESS: a local shadowing a const global is writable"
else
    echo "FAIL: a local shadowing a const global is writable"
    failed=$((failed + 1))
fi

# A directory as input: an error (it was read as an empty file); a directory
# with a header's name is skipped by the include search
mkdir -p build/test/dirinput_c/stdio.h
printf '#include "stdio.h"\nint main(void) { printf("x"); return 0; }\n' > build/test/dirinput_c/t.c
dir_out=$($CC build/test/dirinput_c 2>&1)
if ! $CC build/test/dirinput_c >/dev/null 2>&1 && grep -q "is a directory" <<< "$dir_out" &&
   $CC -c build/test/dirinput_c/t.c -o build/test/dirinput_c.o45 >/dev/null 2>&1; then
    echo "SUCCESS: a directory as input"
else
    echo "FAIL: a directory as input ($dir_out)"
    failed=$((failed + 1))
fi

# A macro redefined differently (#311): a warning at the redefinition and a
# note at the previous one; none for an identical redefinition (white space
# aside); parameter names count; -D and predefined macros too; #warning's
# line is its own
mkdir -p build/test/macroredef
mr=build/test/macroredef/m.c
printf '#define REG_SMALL 2\n#define REG_SMALL 1024\n#define SAME 5\n#define SAME  5\n#define F(a) a\n#define F(b) b\n#define G(a) (a +  1)\n#define G(a) (a + 1)\n#define DX 3\n#define __STDC_HOSTED__ 1\n#warning on line 11\nint f(void) { return REG_SMALL + SAME + F(1) + G(2) + DX; }\n' > $mr
mr_out=$($CC -O2 -DDX=4 -c $mr -o build/test/macroredef/m.o45 2>&1)
if grep -q "m.c:2: warning: macro 'REG_SMALL' redefined differently" <<< "$mr_out" &&
   grep -q "m.c:1: note: the previous definition of 'REG_SMALL'" <<< "$mr_out" &&
   grep -q "m.c:6: warning: macro 'F' redefined" <<< "$mr_out" && grep -q "m.c:5: note: the previous definition of 'F'" <<< "$mr_out" &&
   grep -q "m.c:9: warning: macro 'DX' redefined" <<< "$mr_out" && grep -q "'DX' was defined on the command line" <<< "$mr_out" &&
   grep -q "m.c:10: warning: macro '__STDC_HOSTED__' redefined" <<< "$mr_out" && grep -q "is predefined" <<< "$mr_out" &&
   [ "$(grep -c 'redefined differently' <<< "$mr_out")" = 4 ] && grep -q "on line 11 at .*m.c:11" <<< "$mr_out"; then
    echo "SUCCESS: a macro redefined differently is warned about (both places); identical ones are not"
    passed=$((passed + 1))
else
    echo "FAIL: macro redefinition warnings ($mr_out)"
    failed=$((failed + 1))
fi

if [ $failed -eq 0 ]; then
    echo "All tests passed!"
    exit 0
else
    echo "$failed tests failed."
    exit 1
fi
