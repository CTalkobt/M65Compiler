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

# Macro expansion as C11 6.10.3 has it: XSTR(N) is "4", XCAT(v,N) v4, SELF
# SELF + 1 (once), placemarkers, # and ## on raw arguments (the lines gcc -E
# -P gives, in test_pp_expand.expected)
if ./bin/cp45 src/test-resources/test_pp_expand.c -o build/test/test_pp_expand.i >/dev/null 2>&1 &&
   grep '^a[0-9]' build/test/test_pp_expand.i | diff - src/test-resources/test_pp_expand.expected >/dev/null; then
    echo "SUCCESS: macro expansion (arguments expanded first, hide sets, #, ##)"
else
    echo "FAIL: macro expansion ($(grep '^a[0-9]' build/test/test_pp_expand.i 2>/dev/null | diff - src/test-resources/test_pp_expand.expected | head -4 | tr '\n' ' '))"
    failed=$((failed + 1))
fi

# _Pragma, also made by a macro (DO_PRAGMA(x) _Pragma(#x)): every #pragma
# (signed_default too), its output first on its own line (line numbers kept)
printf '#define DO_PRAGMA(x) _Pragma(#x)\nDO_PRAGMA(cc45 signed_default)\nchar c = -1;\n_Pragma("cc45 unsigned_default") char d = -1;\nint e = __LINE__;\n' > build/test/test_pragma_op.c
if ./bin/cp45 build/test/test_pragma_op.c -o build/test/test_pragma_op.i >/dev/null 2>&1 &&
   [ "$(sed -n 2p build/test/test_pragma_op.i)" = '__asm__(".signed_default");' ] &&
   [ "$(sed -n 4p build/test/test_pragma_op.i)" = '__asm__(".unsigned_default"); char d = -1;' ] &&
   grep -qx 'int e = 5;' build/test/test_pragma_op.i; then
    echo "SUCCESS: _Pragma, also from a macro"
else
    echo "FAIL: _Pragma ($(tr '\n' '|' < build/test/test_pragma_op.i 2>/dev/null))"
    failed=$((failed + 1))
fi

# #embed / #include binary (the runtime test is test_embed_xemu.c): the
# lines after one keep their numbers, an odd byte for words warns, and a
# missing file, an unknown parameter or parameters on #include binary are
# errors naming the line; in a false #if nothing is read
printf '\001\002\003' > build/test/test_embed3.bin
printf 'unsigned int w[] = {\n#include binary word "test_embed3.bin"\n};\n#if 0\n#embed "nowhere.bin"\n#endif\nint e = __LINE__;\n' > build/test/test_embed_ok.c
printf 'char a[] = {\n#embed "nowhere.bin"\n};\n' > build/test/test_embed_e1.c
printf 'char a[] = {\n\n#embed "test_embed3.bin" vendor::thing(1)\n};\n' > build/test/test_embed_e2.c
printf 'char a[] = {\n#include binary "test_embed3.bin" limit(1)\n};\n' > build/test/test_embed_e3.c
if ./bin/cp45 build/test/test_embed_ok.c -o build/test/test_embed_ok.i 2>build/test/test_embed_ok.err &&
   grep -qx '0x0201, 0x0003' build/test/test_embed_ok.i && grep -qx 'int e = 7;' build/test/test_embed_ok.i &&
   grep -q 'test_embed_ok.c:2: warning: .*3 bytes is not a whole number of words' build/test/test_embed_ok.err &&
   ./bin/cp45 build/test/test_embed_e1.c -o /dev/null 2>&1 | grep -q 'test_embed_e1.c:2: .*nowhere.bin' &&
   ./bin/cp45 build/test/test_embed_e2.c -o /dev/null 2>&1 | grep -q "test_embed_e2.c:3: #embed: unknown parameter 'vendor::thing'" &&
   ./bin/cp45 build/test/test_embed_e3.c -o /dev/null 2>&1 | grep -q 'test_embed_e3.c:2: #include binary takes no parameters'; then
    echo "SUCCESS: #embed / #include binary lines and errors"
else
    echo "FAIL: #embed / #include binary ($(cat build/test/test_embed_ok.err 2>/dev/null; ./bin/cp45 build/test/test_embed_e2.c -o /dev/null 2>&1 | head -2))"
    failed=$((failed + 1))
fi

# __bit / __bits (#204; the runtime test is test_bits_xemu.c): a volatile
# register written with one load and one store (lda/and/ora/sta), a bit
# test without shifts (bbs7), and the errors: not an integer, a bit outside
# the operand, lo > hi, a position not constant, its address
printf '#define VIC (*(volatile unsigned char *)0xD011)\nunsigned char g, h;\nvoid w(void) { __bits(VIC, 4, 6) = 3; }\nvoid s(void) { __bit(g, 5) = 1; }\nvoid t(void) { if (__bit(g, 7)) h = 1; }\n' > build/test/test_bits_cg.c
bits_ok=1
if ./bin/cc45 -O2 -S build/test/test_bits_cg.c -o build/test/test_bits_cg.s45 >/dev/null 2>&1; then
    W=$(sed -n '/proc _w$/,/endproc/p' build/test/test_bits_cg.s45 | grep -E '^\s+(lda|and|ora|sta) ' | tr -s ' \n' ' ')
    S=$(sed -n '/proc _s$/,/endproc/p' build/test/test_bits_cg.s45 | grep -cE '^\s+(and|lsr|asl)')
    [ "$W" = " lda \$D011 and #143 ora #48 sta \$D011 " ] || bits_ok=0
    [ "$S" = 0 ] || bits_ok=0
    sed -n '/proc _t$/,/endproc/p' build/test/test_bits_cg.s45 | grep -q 'bbs7' || bits_ok=0
else
    bits_ok=0
fi
for bad in 'float f; void t(void){ __bit(f, 1) = 1; }|not an integer' \
           'unsigned char c; void t(void){ __bits(c, 4, 8) = 1; }|bit 8 is outside the operand (8 bits' \
           'unsigned char c; void t(void){ __bits(c, 5, 2) = 1; }|low bit 5 is above the high bit 2' \
           'unsigned char c; int n; void t(void){ __bit(c, n) = 1; }|bit number of __bit is not a constant' \
           'unsigned char c; void t(void){ unsigned char *p = &__bit(c, 1); }|Cannot take address of __bit'; do
    printf '%s\n' "${bad%%|*}" > build/test/test_bits_bad.c
    ./bin/cc45 -O2 -c build/test/test_bits_bad.c -o build/test/test_bits_bad.o45 2>&1 | grep -qF "${bad##*|}" || bits_ok=0
done
if [ $bits_ok = 1 ]; then
    echo "SUCCESS: __bit / __bits code and errors"
else
    echo "FAIL: __bit / __bits ($W; and/shifts in s: $S)"
    failed=$((failed + 1))
fi

# __jsr: a constant value is an immediate load, a ROM address goes through
# __rom_calli with the address after it, a result is saved after the call;
# the errors: not a register, a register twice, __jmp with a result, a C
# function of the file, a target that is neither address nor function
printf 'unsigned char f(unsigned char v) { return __jsr(0xFFD2, .A = 65, .X = v).A; }\n' > build/test/test_jsr_cg.c
jsr_ok=1
if ./bin/cc45 -O2 -S build/test/test_jsr_cg.c -o build/test/test_jsr_cg.s45 >/dev/null 2>&1; then
    # (one 8-bit register from a value: loaded straight into it, #344)
    J=$(sed -n '/proc _f\b/,/endproc/p' build/test/test_jsr_cg.s45 | grep -E '^\s*(lda #\$41|ldx \$10|ldx ___jsr_x|jsr __rom_calli|\.word \$FFD2|sta ___jsr_a)' | tr -s ' \n' ' ')
    [ "$J" = " ldx \$10 lda #\$41 jsr __rom_calli .word \$FFD2 sta ___jsr_a " ] || jsr_ok=0
else
    jsr_ok=0
fi
for bad in 'void t(void){ __jsr(0xFFD2, .B = 1); }|is not a register' \
           'void t(void){ __jsr(0xFFD2, .A = 1, .AX = 2); }|register A is given twice' \
           'void t(void){ unsigned char c; __jmp(0x2000, .A -> c); }|__jmp does not return' \
           'void g(void){} void t(void){ __jsr(g); }|is a C function of this file' \
           'int v; void t(void){ __jsr(v); }|is neither'; do
    printf '%s\n' "${bad%%|*}" > build/test/test_jsr_bad.c
    ./bin/cc45 -O2 -c build/test/test_jsr_bad.c -o build/test/test_jsr_bad.o45 2>&1 | grep -qF "${bad##*|}" || jsr_ok=0
done
if [ $jsr_ok = 1 ]; then
    echo "SUCCESS: __jsr code and errors"
else
    echo "FAIL: __jsr ($J)"
    failed=$((failed + 1))
fi

# The __bit type (#73; the runtime test is test_bit_type_xemu.c): two flags
# share one byte; a flag set / cleared is tsb / trb (lda #1 first), a __zp
# one smb / rmb, a lone __zp flag tested bbs on the variable; the errors:
# extern __bit, its address, ++, an initial value that is not constant
printf '__bit ready, busy;\n__zp __bit zr;\nunsigned char h;\nvoid set(void) { ready = 1; busy = 0; zr = 1; }\nvoid clr(void) { zr = 0; }\nvoid t(void) { if (zr) h = 1; }\n' > build/test/test_bitty.c
bt_ok=1
# (in a directory of its own: other scripts may compile in build/test meanwhile)
SD=build/test/bitty.d
rm -rf $SD; mkdir -p $SD
if (cd $SD && ../../../bin/cc45 -O2 -c ../test_bitty.c -o ../test_bitty.o45 --save-temps >/dev/null 2>&1); then
    T=$(ls -t $SD/temp_*.s45 | head -1)
    SET=$(sed -n '/proc _set/,/endproc/p' "$T" | grep -E '^\s+(lda|tsb|trb|smb|rmb|sta|ora|and)' | tr -s ' \n' ' ')
    case "$SET" in *"lda #1 tsb ___bitvars0 lda #2 trb ___bitvars0 smb0 \$"*) ;; *) bt_ok=0 ;; esac
    sed -n '/proc _clr/,/endproc/p' "$T" | grep -q 'rmb0 \$' || bt_ok=0
    sed -n '/proc _t$/,/endproc/p' "$T" | grep -qE 'bbs0 \$[0-9A-F]+, ' || bt_ok=0
    ./bin/nm45 build/test/test_bitty.o45 | grep -qE ' _ready$| _busy$' && bt_ok=0
    rm -f "$T"
else
    bt_ok=0
fi
for bad in 'extern __bit f; int main(void){ return f; }|a __bit variable belongs to its file' \
           '__bit f; int main(void){ __bit *p = &f; return p != 0; }|has no address' \
           '__bit f; int main(void){ f++; return 0; }|++/-- of a __bit variable' \
           'int v; __bit f = v; int main(void){ return f; }|is not a constant'; do
    printf '%s\n' "${bad%%|*}" > build/test/test_bitty_bad.c
    ./bin/cc45 -O2 -c build/test/test_bitty_bad.c -o build/test/test_bitty_bad.o45 2>&1 | grep -qF "${bad##*|}" || bt_ok=0
done
if [ $bt_ok = 1 ]; then
    echo "SUCCESS: __bit type packing, tsb/trb/smb/rmb/bbs and errors"
else
    echo "FAIL: __bit type ($SET)"
    failed=$((failed + 1))
fi

# String functions on literals (#78; the runtime test is test_strlit_xemu.c):
# strlen("hello") is lda #5 (no call), strcmp(s, "") == 0 a byte compare
# (no call), "GO" too at -O3 (a call at -O2), strcmp(s, "GO") < 0 the library; -fno-builtin keeps every call;
# a file that defines its own strlen keeps it (not folded to the length)
printf '#include <string.h>\nunsigned f1(void) { return strlen("hello"); }\nint f2(const char *s) { return strcmp(s, "") == 0; }\nint f3(const char *s) { return strcmp(s, "GO") < 0; }\nint f4(const char *s) { return strcmp(s, "GO") == 0; }\n' > build/test/test_strlit_cg.c
printf 'unsigned strlen(const char *s) { return 7; }\nunsigned g(void) { return strlen("hi"); }\n' > build/test/test_strlit_own.c
sl_ok=1
proc_of() { sed -n "/proc _$1\\b/,/endproc/p" "$2"; }
if ./bin/cc45 -O2 -S build/test/test_strlit_cg.c -o build/test/test_strlit_cg.s45 >/dev/null 2>&1 &&
   ./bin/cc45 -O2 -fno-builtin -S build/test/test_strlit_cg.c -o build/test/test_strlit_nb.s45 >/dev/null 2>&1 &&
   ./bin/cc45 -O3 -S build/test/test_strlit_cg.c -o build/test/test_strlit_o3.s45 >/dev/null 2>&1 &&
   ./bin/cc45 -O2 -S build/test/test_strlit_own.c -o build/test/test_strlit_own.s45 >/dev/null 2>&1; then
    proc_of f1 build/test/test_strlit_cg.s45 | grep -q 'jsr _strlen' && sl_ok=0
    proc_of f1 build/test/test_strlit_cg.s45 | grep -q 'lda #5' || sl_ok=0
    proc_of f2 build/test/test_strlit_cg.s45 | grep -q 'jsr _strcmp' && sl_ok=0
    proc_of f3 build/test/test_strlit_cg.s45 | grep -q 'jsr _strcmp' || sl_ok=0
    proc_of f4 build/test/test_strlit_cg.s45 | grep -q 'jsr _strcmp' || sl_ok=0    # (3 bytes: the call at -O2)
    proc_of f4 build/test/test_strlit_o3.s45 | grep -q 'jsr _strcmp' && sl_ok=0    # (inline at -O3)
    proc_of f1 build/test/test_strlit_nb.s45 | grep -q 'jsr _strlen' || sl_ok=0
    proc_of f2 build/test/test_strlit_nb.s45 | grep -q 'jsr _strcmp' || sl_ok=0
    proc_of g build/test/test_strlit_own.s45 | grep -q 'lda #2' && sl_ok=0   # (its own strlen, 7: not folded to 2)
else
    sl_ok=0
fi
if [ $sl_ok = 1 ]; then
    echo "SUCCESS: string functions on literals folded / inline (-fno-builtin, own strlen)"
else
    echo "FAIL: string functions on literals"
    failed=$((failed + 1))
fi

# stridx (#79): literal candidates are a table for __stridx_table (no
# variadic call), a literal subject the index, -fno-builtin the library
printf '#include <string.h>\nint c(const char *s) { return stridx(s, "load", "save", "quit", NULL); }\nint k(void) { return stridx("save", "load", "save"); }\n' > build/test/test_stridx_cg.c
sx_ok=1
if ./bin/cc45 -O2 -S build/test/test_stridx_cg.c -o build/test/test_stridx_cg.s45 >/dev/null 2>&1 &&
   ./bin/cc45 -O2 -fno-builtin -S build/test/test_stridx_cg.c -o build/test/test_stridx_nb.s45 >/dev/null 2>&1; then
    sed -n '/proc _c\b/,/endproc/p' build/test/test_stridx_cg.s45 | grep -q 'jsr ___stridx_table' || sx_ok=0
    sed -n '/proc _c\b/,/endproc/p' build/test/test_stridx_cg.s45 | grep -q 'jsr _stridx\b' && sx_ok=0
    sed -n '/proc _k\b/,/endproc/p' build/test/test_stridx_cg.s45 | grep -q 'lda #2' || sx_ok=0
    sed -n '/proc _c\b/,/endproc/p' build/test/test_stridx_nb.s45 | grep -q 'jsr _stridx\b' || sx_ok=0
else
    sx_ok=0
fi
if [ $sx_ok = 1 ]; then
    echo "SUCCESS: stridx table / folded / -fno-builtin"
else
    echo "FAIL: stridx code"
    failed=$((failed + 1))
fi

# Sections (#70; the runtime test is test_section_xemu.c): two files'
# parts of one section, one with the address; --section overrides it; the
# initial values kept by --gc-inputs and copied by __zp_init's DMA job;
# errors: two addresses, none, the program's memory, an undeclared section;
# cc45 -Wl,... to ln45
printf '#pragma section("BUF", 0x0C00)\n__section("BUF") char a1[3] = {1, 2, 3};\n__section("BUF") char a2[2];\nextern char b1[];\nint main(void) { return a1[2] + b1[1] + a2[0]; }\n' > build/test/test_sect_a.c
printf '#pragma section("BUF")\n__section("BUF") char b1[4] = {9, 8, 7, 6};\n' > build/test/test_sect_b.c
printf '#pragma section("BUF", 0x0D00)\n__section("BUF") char c1;\n' > build/test/test_sect_c.c
printf '#pragma section("HIGH", 0x2100)\n__section("HIGH") char h[4];\nint main(void) { h[0] = 1; return 0; }\n' > build/test/test_sect_d.c
printf '__section("NONE") char n;\n' > build/test/test_sect_e.c
sc_ok=1
LIBC=lib/build/c45.lib
if ./bin/cc45 -O2 -ffunction-sections -c build/test/test_sect_a.c -o build/test/test_sect_a.o45 >/dev/null 2>&1 &&
   ./bin/cc45 -O2 -ffunction-sections -c build/test/test_sect_b.c -o build/test/test_sect_b.o45 >/dev/null 2>&1 &&
   ./bin/cc45 -O2 -c build/test/test_sect_c.c -o build/test/test_sect_c.o45 >/dev/null 2>&1; then
    SM=$(./bin/ln45 -basic --gc-inputs -m -o build/test/test_sect.prg build/test/test_sect_a.o45 build/test/test_sect_b.o45 $LIBC 2>&1)
    for want in '00000c00 _a1' '00000c03 _a2' '00000c05 _b1' 'cc45.sectinit._b1'; do
        grep -q "$want" <<< "$SM" || sc_ok=0
    done
    SM=$(./bin/ln45 -basic --section BUF=0x0E00 -m -o build/test/test_sect.prg build/test/test_sect_a.o45 build/test/test_sect_b.o45 build/test/test_sect_c.o45 $LIBC 2>&1)
    grep -q '00000e09 _c1' <<< "$SM" || sc_ok=0
    ./bin/ln45 -basic -o build/test/test_sect.prg build/test/test_sect_a.o45 build/test/test_sect_c.o45 $LIBC 2>&1 |
        grep -q 'section BUF is at \$0C00 in .* and at \$0D00' || sc_ok=0
    ./bin/ln45 -basic -o build/test/test_sect.prg build/test/test_sect_b.o45 $LIBC 2>&1 | grep -q 'section BUF has no address' || sc_ok=0
    ./bin/cc45 -O2 build/test/test_sect_d.c -o build/test/test_sect_d.prg 2>&1 | grep -q 'section HIGH (\$2100-\$2103) overlaps the program' || sc_ok=0
    ./bin/cc45 -O2 -c build/test/test_sect_e.c -o build/test/test_sect_e.o45 2>&1 | grep -q "section 'NONE' is not declared" || sc_ok=0
    # a one-step build passes -Wl options to ln45 (here its map)
    ./bin/cc45 -O2 build/test/test_sect_d.c -o build/test/test_sect_d.prg -Wl,--section,HIGH=0x0E00,-m 2>&1 |
        grep -q '00000e00 _h' || sc_ok=0
else
    sc_ok=0
fi
if [ $sc_ok = 1 ]; then
    echo "SUCCESS: sections placed by ln45 (two files, --section, --gc-inputs, errors, -Wl)"
else
    echo "FAIL: sections"
    failed=$((failed + 1))
fi

# CSE across stores through pointers and calls: k * m (variables whose
# address is never taken) is reused; with &k taken it is computed again
printf 'signed int *gp;\nvoid nothing(void);\nsigned int f(signed int k, signed int m) {\n    signed int a, b;\n    a = k * m;\n    *gp = 3;\n    nothing();\n    b = k * m;\n    return a + b;\n}\nsigned int g(signed int k, signed int m) {\n    signed int a, b;\n    gp = &k;\n    a = k * m;\n    *gp = 3;\n    b = k * m;\n    return a + b;\n}\n' > build/test/test_cse_slots_cg.c
CS=$(./bin/cc45 -O2 -Roptir -c build/test/test_cse_slots_cg.c -o build/test/test_cse_slots_cg.o45 2>&1)
if grep -q '\[cse\] _f .*reuses' <<< "$CS" && ! grep -q '\[cse\] _g .*reuses' <<< "$CS"; then
    echo "SUCCESS: CSE keeps variables whose address is not taken across stores and calls"
else
    echo "FAIL: CSE across stores and calls ($(grep -c cse <<< "$CS") reuses)"
    failed=$((failed + 1))
fi

# restrict (#77; the runtime test is test_restrict_xemu.c): loads through
# restrict parameters reused across stores into a global and through the
# other one (CSE), and hoisted out of a loop (LICM); without restrict none
printf 'unsigned char g;\nvoid f(unsigned char *restrict d, const unsigned char *restrict s) {\n    d[0] = s[0] + s[1];\n    g = 1;\n    d[1] = s[0] - s[1];\n}\nvoid sc(signed int *restrict o, const signed int *restrict in, const signed int *restrict k, unsigned char n) {\n    unsigned char i;\n    for (i = 0; i < n; i++) o[i] = in[i] * *k + k[1];\n}\n' > build/test/test_restrict_cg.c
sed 's/restrict//g' build/test/test_restrict_cg.c > build/test/test_restrict_none.c
RS=$(./bin/cc45 -O2 -Roptir -c build/test/test_restrict_cg.c -o build/test/test_restrict_cg.o45 2>&1)
RN=$(./bin/cc45 -O2 -Roptir -c build/test/test_restrict_none.c -o build/test/test_restrict_none.o45 2>&1)
# (without restrict, s + 1 is still reused -- s itself does not change --
# but not the loads through it: at least two reuses more with it)
if [ "$(grep -c '\[cse\] _f .*reuses' <<< "$RS")" -ge $(( $(grep -c '\[cse\] _f .*reuses' <<< "$RN") + 2 )) ] &&
   [ "$(grep -c '\[licm\] _sc' <<< "$RS")" -ge 3 ] && [ "$(grep -c '\[licm\] _sc' <<< "$RN")" -lt 3 ]; then
    echo "SUCCESS: restrict: loads reused and hoisted only with restrict"
else
    echo "FAIL: restrict (with: $(grep -cE 'cse|licm' <<< "$RS") changes, without: $(grep -cE 'cse|licm' <<< "$RN"))"
    failed=$((failed + 1))
fi

# __zp __bit flags tested several times in a function that leaves them
# alone: bbs on the byte itself, no load of it (a copy)
printf '__zp __bit zr, zb;\nunsigned char h;\nvoid t(void) { if (zr) h = 1; if (zr) h = 2; if (!zb) h = 3; }\n' > build/test/test_zpalias.c
# (in a directory of its own: other scripts may compile in build/test meanwhile)
SD=build/test/zpalias.d
rm -rf $SD; mkdir -p $SD
if (cd $SD && ../../../bin/cc45 -O2 -c ../test_zpalias.c -o ../test_zpalias.o45 --save-temps >/dev/null 2>&1); then
    T=$(ls -t $SD/temp_*.s45 | head -1)
    ZA=$(sed -n '/proc _t$/,/endproc/p' "$T")
    rm -f "$T"
    if [ "$(grep -cE '^\s+bbs[01] \$' <<< "$ZA")" = 3 ] && ! grep -qE '^\s+lda \$[0-9A-F]{2}$' <<< "$ZA"; then
        echo "SUCCESS: __zp __bit tests bbs on the byte (no copy)"
    else
        echo "FAIL: __zp __bit tests ($(tr '\n' ' ' <<< "$ZA"))"
        failed=$((failed + 1))
    fi
else
    echo "FAIL: __zp __bit tests (compilation)"
    failed=$((failed + 1))
fi

# Fixed point (#72; the runtime test is test_fixed_xemu.c): a constant is
# its scaled integer ($0180), 8.8 * inline (no call), 16.16 * __fx32_mul;
# % and &, two 32-bit types mixed, a bad __fixed(I, F) are errors, a
# constant out of range a warning
printf '__fixed8_8 g = 1.5;\n__fixed8_8 m(__fixed8_8 a, __fixed8_8 b) { return a * b; }\n__fixed16_16 n(__fixed16_16 a, __fixed16_16 b) { return a * b; }\n' > build/test/test_fixed_cg.c
fx_ok=1
if ./bin/cc45 -O2 -S build/test/test_fixed_cg.c -o build/test/test_fixed_cg.s45 >/dev/null 2>&1; then
    grep -A1 '^_g:' build/test/test_fixed_cg.s45 | grep -qE '\.word (384|\$0180)' || fx_ok=0
    sed -n '/proc _m\b/,/endproc/p' build/test/test_fixed_cg.s45 | grep -q 'jsr' && fx_ok=0
    sed -n '/proc _n\b/,/endproc/p' build/test/test_fixed_cg.s45 | grep -q 'jsr ___fx32_mul' || fx_ok=0
else
    fx_ok=0
fi
for bad in '__fixed8_8 a; int f(void){ return a % 2; }|'"'"'%'"'"' is not defined on fixed-point values' \
           '__fixed8_8 a; int f(void){ return a & 1; }|'"'"'&'"'"' is not defined on fixed-point values' \
           '__fixed(4,28) a; __fixed(16,16) b; int f(void){ return a + b > 0; }|__fixed(4, 28) and __fixed(16, 16) in one operation: cast one' \
           '__fixed(3, 3) a;|I + F 16 or 32' \
           '__fixed(1,15) a = 1.0;|does not fit __fixed(1, 15)'; do
    printf '%s\n' "${bad%%|*}" > build/test/test_fixed_bad.c
    ./bin/cc45 -O2 -c build/test/test_fixed_bad.c -o build/test/test_fixed_bad.o45 2>&1 | grep -qF "${bad##*|}" || fx_ok=0
done
if [ $fx_ok = 1 ]; then
    echo "SUCCESS: fixed point: constants scaled, 8.8 * inline, 16.16 * the library, errors"
else
    echo "FAIL: fixed point code"
    failed=$((failed + 1))
fi

# Far pointers (#68; the runtime tests are test_far_ptr_xemu.c and
# test_near_ptr_xemu.c): *p reads with [bp],z, an address initializer fills
# the high word with 0, a __far function is unchanged; near pointers read
# base-page indexed, initialized with an address's low byte, converted with
# B; a struct copied through one, a float and a bit-field through one are
# errors
printf 'char buf[2];\nchar __far *g = buf;\nchar __far *p;\nchar f(void){ return *p; }\n__zp char z[2];\nchar __near *n = &z[1];\nchar h(void){ return *n; }\nchar *k(void){ return (char *)n; }\n' > build/test/test_far_cg.c
fp_ok=1
if ./bin/cc45 -O2 -S build/test/test_far_cg.c -o build/test/test_far_cg.s45 >/dev/null 2>&1; then
    grep -A2 '^_g:' build/test/test_far_cg.s45 | tr '\n' ' ' | grep -qE '\.word _buf +\.word 0' || fp_ok=0
    grep -q 'lda \[\$[0-9A-F]*\],z' build/test/test_far_cg.s45 || fp_ok=0
    grep -A1 '^_n:' build/test/test_far_cg.s45 | grep -qF '.byte <(_z+1)' || fp_ok=0
    grep -q 'lda \$00,x' build/test/test_far_cg.s45 || fp_ok=0
    grep -q 'tba' build/test/test_far_cg.s45 || fp_ok=0
else
    fp_ok=0
fi
for bad in 'struct S { int a; }; struct S __far *p; struct S s; void f(void){ s = *p; }|a struct or union through a far pointer is not copied' \
           'struct S { int a; }; struct S __far *p; struct S s; void f(void){ *p = s; }|a struct or union through a far pointer is not copied' \
           'float __far *p; float f(void){ return *p; }|only integers and pointers can be read through a far pointer' \
           'struct B { int a : 3; }; struct B __far *p; int f(void){ return p->a; }|a bit-field through a far pointer is not supported'; do
    printf '%s\n' "${bad%%|*}" > build/test/test_far_bad.c
    ./bin/cc45 -O2 -c build/test/test_far_bad.c -o build/test/test_far_bad.o45 2>&1 | grep -qF "${bad##*|}" || fp_ok=0
done
if [ $fp_ok = 1 ]; then
    echo "SUCCESS: far and near pointers: [bp],z / \$00,x access, address initializers, errors"
else
    echo "FAIL: far pointer code"
    failed=$((failed + 1))
fi

# pack45 (#345): round trips (with the in-place check pack45 makes itself)
# of random, zero, text and code inputs, unpacked again with -x
pk_ok=1
head -c 3000 /dev/urandom > build/test/pack_rand.bin
head -c 9000 /dev/zero > build/test/pack_zero.bin
head -c 40000 doc/bin/cc45.md > build/test/pack_text.bin
head -c 30000 bin/ca45 > build/test/pack_code.bin
for f in rand zero text code; do
    ./bin/pack45 -d 0x40000 build/test/pack_$f.bin build/test/pack_$f.pak >/dev/null 2>&1 &&
    ./bin/pack45 -x build/test/pack_$f.pak build/test/pack_$f.out >/dev/null 2>&1 &&
    cmp -s build/test/pack_$f.bin build/test/pack_$f.out || pk_ok=0
done
# (text packs to under 60%)
[ $(stat -c %s build/test/pack_text.pak) -lt 24000 ] || pk_ok=0
if [ $pk_ok = 1 ]; then
    echo "SUCCESS: pack45 round trips"
else
    echo "FAIL: pack45 round trips"
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

# Elements of global arrays at a byte index as name+off,y, no address built
# in a pointer (#318): an extern array, a 600-byte one, read-modify-write,
# a constant offset (a byte widened for the index)
printf 'extern unsigned char ext[40];\nunsigned char big[600];\nvoid f(unsigned char i, unsigned char v) { ext[i]--; big[i] = v; big[i + 5] += 3; }\n' > build/test/elemy.c
if $CC -S build/test/elemy.c -o build/test/elemy.s45 >/dev/null 2>&1 &&
   [ "$(grep -cE '(_ext|_big)(\+5)?,y' build/test/elemy.s45)" -eq 5 ] &&
   ! grep -qE 'adc #<\(?(_ext|_big)' build/test/elemy.s45; then
    echo "SUCCESS: byte-table elements as name+off,y"
else
    echo "FAIL: byte-table elements as name+off,y"
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
