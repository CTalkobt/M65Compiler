#!/bin/bash

# Test script for ca45 assembler specific features (directives, etc.)

AS="./bin/ca45"
mkdir -p build/test

failed=0
passed=0

test_file() {
    local f=$1
    local bin_file="build/test/$(basename "$f" .s45).bin"
    
    echo "Testing $f..."
    if $AS "$f" -o "$bin_file"; then
        echo "SUCCESS: $f"
        passed=$((passed + 1))
    else
        echo "FAIL: Assembly failed for $f"
        failed=$((failed + 1))
    fi
}

# List of assembler feature tests
AS_TEST_FILES=(
    "src/test-resources/test_basic_upstart.s45"
    "src/test-resources/test_data_types.s45"
    "src/test-resources/test_expr_minus.s45"
    "src/test-resources/test_mul.s45"
    "src/test-resources/test_div.s45"
    "src/test-resources/test_expr.s45"
    "src/test-resources/test_expr_advanced.s45"
    "src/test-resources/hello_call.s45"
    "src/test-resources/test_var.s45"
    "src/test-resources/test_45gs02.s45"
    "src/test-resources/test_equates_advanced.s45"
    "src/test-resources/test_flat_z_all.s45"
    "src/test-resources/test_accumulator_labels.s45"
    "src/test-resources/test_multi_org.s45"
    "src/test-resources/test_peephole.s45"
    "src/test-resources/test_simulated_ops.s45"
    "src/test-resources/test_fill.s45"
    "src/test-resources/test_fill_advanced.s45"
    "src/test-resources/test_segments.s45"
    "src/test-resources/test_res.s45"
    "src/test-resources/test_pushpop.s45"
    "src/test-resources/test_addr_helpers.s45"
    "src/test-resources/test_shifts_16.s45"
    "src/test-resources/test_new_16bit_ops.s45"
    "src/test-resources/test_array.s45"
    "src/test-resources/test_expressions.s45"
)

for f in "${AS_TEST_FILES[@]}"; do
    test_file "$f"
done

# Specific check for basicUpstart output format
echo "Verifying .basicUpstart output bytes..."
# Expecting: 0b 08 0a 00 9e 32 30 36 31 00 00 00 (for START at $080D = 2061)
ACTUAL=$(hexdump -v -n 12 -e '1/1 "%02x "' build/test/test_basic_upstart.bin | xargs)
EXPECTED="0b 08 0a 00 9e 32 30 36 31 00 00 00"

if [ "$ACTUAL" == "$EXPECTED" ]; then
    echo "SUCCESS: .basicUpstart output matches expected format."
else
    echo "FAIL: .basicUpstart output mismatch."
    echo "  Expected start: $EXPECTED"
    echo "  Actual:         $ACTUAL"
    failed=$((failed + 1))
fi

# Expressions: subtraction, unary minus, 0x/0b, data directive expressions
echo "Verifying directive bytes (test_directives.s45)..."
$AS src/test-resources/test_directives.s45 -o build/test/test_directives.bin >/dev/null 2>&1
ACTUAL=$(hexdump -v -e '1/1 "%02x "' build/test/test_directives.bin | xargs)
EXPECTED="ea ea ea 55 55 11 11 02 09 ff ff ff 06 06 06 06 00 00 ad 12 00 bd 12 00 a5 12 1a 20"
if [ "$ACTUAL" == "$EXPECTED" ]; then
    echo "SUCCESS: directive bytes match."
else
    echo "FAIL: directive bytes mismatch."
    echo "  Expected: $EXPECTED"
    echo "  Actual:   $ACTUAL"
    failed=$((failed + 1))
fi
# .basicUpstart: 5-digit SYS for an address of 10000 or more
printf '.basicUpstart 10000\n* = 10000\n rts\n' > build/test/test_bu5.s45
$AS build/test/test_bu5.s45 -o build/test/test_bu5.prg >/dev/null 2>&1
ACTUAL=$(hexdump -v -n 15 -e '1/1 "%02x "' build/test/test_bu5.prg | xargs)
if [ "$ACTUAL" == "01 20 0c 20 0a 00 9e 31 30 30 30 30 00 00 00" ]; then
    echo "SUCCESS: .basicUpstart 10000 stub."
else
    echo "FAIL: .basicUpstart 10000 stub: $ACTUAL"
    failed=$((failed + 1))
fi

echo "Verifying expression bytes (test_expr_minus.s45)..."
ACTUAL=$(hexdump -v -e '1/1 "%02x "' build/test/test_expr_minus.bin | xargs)
EXPECTED="a9 04 a9 06 a9 04 a9 05 a9 01 a9 10 a9 05 a9 ff 03 fe 01 03 00 06 00 ff ff 3f 42 0f 00 81 80 00 00 00 c3 2d 78 eb c6"
if [ "$ACTUAL" == "$EXPECTED" ]; then
    echo "SUCCESS: expression bytes match."
else
    echo "FAIL: expression bytes mismatch."
    echo "  Expected: $EXPECTED"
    echo "  Actual:   $ACTUAL"
    failed=$((failed + 1))
fi
if $AS src/test-resources/test_expr_minus.s45 -o build/test/test_expr_minus.bin 2>&1 | grep -q "overflow"; then
    echo "FAIL: #-1 warns about an 8-bit overflow"
    failed=$((failed + 1))
fi

echo "Testing -Dcc45.zeroPageStart=\$10..."
# Use a dereference which is guaranteed to use ZP scratch $02/$03
echo -e ".var ptr = \$20\nexpr .A, *ptr" > build/test_zp_shift.s45
$AS "-Dcc45.zeroPageStart=\$10" -o build/test_zp_shift.bin build/test_zp_shift.s45
# 85 10 (STA $10) should be in there (saving eval'd ptr address to ZP scratch)
ACTUAL_ZP=$(hexdump -v -e '1/1 "%02x "' build/test_zp_shift.bin)
if [ -f build/test_zp_shift.bin ] && echo "$ACTUAL_ZP" | grep -q "85 10"; then
    echo "SUCCESS: -Dcc45.zeroPageStart correctly shifted ZP addresses."
else
    echo "FAIL: -Dcc45.zeroPageStart did not shift ZP addresses correctly."
    echo "Actual bytes: $ACTUAL_ZP"
    failed=$((failed + 1))
fi

echo "Testing .global / .extern in an object (test_global_extern.s45)..."
if $AS -c src/test-resources/test_global_extern.s45 -o build/test/test_global_extern.o45 >/dev/null 2>&1 &&
   ./bin/nm45 build/test/test_global_extern.o45 | grep -q " U _printf" &&
   ./bin/nm45 build/test/test_global_extern.o45 | grep -q " T _main" &&
   ./bin/nm45 build/test/test_global_extern.o45 | grep -q " D _count"; then
    echo "SUCCESS: .global exports and .extern imports in the object"
else
    echo "FAIL: test_global_extern.s45 as an object"
    failed=$((failed + 1))
fi

echo "Testing nm45 and ln45 with a directory as input..."
# a directory opens as a stream but throws on the first read (they aborted)
mkdir -p build/test/dirinput/c45.lib
nm_out=$(./bin/nm45 build/test/dirinput build/test/test_global_extern.o45 2>&1); nm_rc=$?
ln_out=$(./bin/ln45 -o build/test/dirinput.prg build/test/dirinput 2>&1); ln_rc=$?
lnl_out=$(./bin/ln45 -o build/test/dirinput.prg build/test/test_global_extern.o45 -l build/test/dirinput 2>&1); lnl_rc=$?
if [ $nm_rc -eq 1 ] && grep -q "dirinput: is a directory" <<< "$nm_out" && grep -q " T _main" <<< "$nm_out" &&
   [ $ln_rc -eq 1 ] && grep -q "dirinput: is a directory" <<< "$ln_out" &&
   [ $lnl_rc -eq 1 ] && grep -q "dirinput: is a directory" <<< "$lnl_out" &&
   (cd build/test/dirinput && ../../../bin/ln45 -o ../dirinput.prg ../test_global_extern.o45 -l c45.lib >/dev/null 2>&1); then
    echo "SUCCESS: a directory is reported, a library directory of the same name is skipped"
    passed=$((passed + 1))
else
    echo "FAIL: directory input (nm45 $nm_rc: $nm_out / ln45 $ln_rc: $ln_out / ln45 -l $lnl_rc: $lnl_out)"
    failed=$((failed + 1))
fi

echo "Testing ln45's end-of-RAM check (Issue #283)..."
# a C program reaching $C000 (reads as ROM in C65 mode) fails to link with
# either stack and with a stack top above $BFFF; --ram-top moves the limit
printf 'static unsigned char big[41000];\nint main(void) { big[40999] = 1; return big[40999]; }\n' > build/test/big_bss.c
printf 'static unsigned char big[30000];\nint main(void) { big[29999] = 1; return big[29999]; }\n' > build/test/fit_bss.c
rt_ok=1
./bin/cc45 -O2 build/test/big_bss.c -o build/test/big_bss.prg >/dev/null 2>&1 && rt_ok=0
for fl in "-fno-stack16" "-fstack-top=0xFFF0"; do
    ./bin/cc45 -O2 $fl -c build/test/big_bss.c -o build/test/big_bss.o45 >/dev/null 2>&1 || rt_ok=0
    ./bin/ln45 -basic -o build/test/big_bss.prg build/test/big_bss.o45 lib/build/c45.lib >/dev/null 2>&1 && rt_ok=0
done
./bin/cc45 -O2 -fno-stack16 -c build/test/big_bss.c -o build/test/big_bss.o45 >/dev/null 2>&1
./bin/ln45 -basic --ram-top=0x10000 -o build/test/big_bss.prg build/test/big_bss.o45 lib/build/c45.lib >/dev/null 2>&1 || rt_ok=0
./bin/cc45 -O2 build/test/fit_bss.c -o build/test/fit_bss.prg >/dev/null 2>&1 || rt_ok=0
./bin/cc45 -O2 -fno-stack16 build/test/fit_bss.c -o build/test/fit_bss.prg >/dev/null 2>&1 || rt_ok=0
if [ $rt_ok -eq 1 ]; then
    echo "SUCCESS: a C program above \$C000 fails to link"
    passed=$((passed + 1))
else
    echo "FAIL: ln45 end-of-RAM check"
    failed=$((failed + 1))
fi

echo "Testing a used .extern in a flat program..."
# no linker resolves it: an error (it would be $0000); declared but unused is fine,
# and -c keeps it as an import
printf '.extern _used, _unused\n* = $2000\n    jsr _used\n    rts\n' > build/test_flat_extern.s45
if $AS build/test_flat_extern.s45 -o build/test_flat_extern.bin 2>&1 | grep -q "'_used' is .extern but no linker resolves it" &&
   ! $AS build/test_flat_extern.s45 -o build/test_flat_extern.bin >/dev/null 2>&1 &&
   ! $AS build/test_flat_extern.s45 -o build/test_flat_extern.bin 2>&1 | grep -q "_unused" &&
   $AS -c build/test_flat_extern.s45 -o build/test_flat_extern.o45 >/dev/null 2>&1; then
    echo "SUCCESS: a used .extern in a flat program is an error"
else
    echo "FAIL: a used .extern in a flat program should be an error (and only that one)"
    failed=$((failed + 1))
fi

# Unknown directives are ignored with a warning (once per line); a known one
# in upper case, #include, conditional assembly and .asciiz get a hint
cat > build/test_unknown_dir.s45 << 'EOF'
    .org $2000
    .bogus 1
    .BYTE 1
    .if 1
    .asciiz "x"
    .byte 2
EOF
OUT=$($AS build/test_unknown_dir.s45 -o build/test_unknown_dir.bin 2>&1)
if echo "$OUT" | grep -q "2:5: warning: unknown directive '.bogus' is ignored" &&
   echo "$OUT" | grep -q "directives are lower case: '.byte'" &&
   echo "$OUT" | grep -q "ca45 has no conditional assembly" &&
   echo "$OUT" | grep -q "use .ascii" &&
   [ "$(echo "$OUT" | grep -c 'unknown directive')" = "4" ] &&
   [ "$(xxd -p build/test_unknown_dir.bin)" = "02" ]; then
    echo "SUCCESS: unknown directives warn and are ignored"
    passed=$((passed + 1))
else
    echo "FAIL: unknown directive warnings ($OUT)"
    failed=$((failed + 1))
fi

# A simulated instruction that can't relocate an address in a relocatable
# segment is an error with -c, not a wrong address
cat > build/test_simop_reloc.s45 << 'EOF'
        .data
_w:     .word 5
        .code
proc _f
        neg.16 _w
endproc
EOF
OUT=$($AS -c build/test_simop_reloc.s45 -o build/test_simop_reloc.o45 2>&1)
if [ $? -ne 0 ] && echo "$OUT" | grep -q "'neg.16' can't relocate '_w'"; then
    echo "SUCCESS: a simulated instruction without a relocation for its operand is an error"
    passed=$((passed + 1))
else
    echo "FAIL: simulated instruction relocation check ($OUT)"
    failed=$((failed + 1))
fi

# Sections (.function_sections / -ffunction-sections): one per proc and
# .split; merged when tied without a relocation; references where a
# relocation's value does not say which section it targets
SD=build/test_sections
mkdir -p $SD
sec_check() {   # sec_check <name> <expected nm45 -S lines (grep -F patterns)>... ; reads $SD/<name>.s45
    local n=$1; shift
    local out
    if ! out=$($AS -c $SD/$n.s45 -o $SD/$n.o45 2>&1 && ./bin/nm45 -S $SD/$n.o45 2>&1); then
        echo "FAIL: sections $n ($out)"; failed=$((failed + 1)); return
    fi
    for p in "$@"; do
        if ! grep -qF -- "$p" <<< "$out"; then
            echo "FAIL: sections $n: missing '$p'"; echo "$out" | sed 's/^/    /'; failed=$((failed + 1)); return
        fi
    done
    echo "SUCCESS: sections $n"; passed=$((passed + 1))
}
printf '        .code\nproc _a\n        lda #1\nendproc\n' > $SD/none.s45
sec_check none "No sections"
cat > $SD/base.s45 << 'EOF'
        .function_sections
        .code
proc _a
        lda #1
endproc
proc _b
        lda #2
endproc
        .align 256
proc _c
        .keep
        lda #3
endproc
EOF
sec_check base "Sections (3)" "00000000         3      1  _a" "00000003         3      1  _b" "00000100         3    256  _c  [keep]"
cat > $SD/branch.s45 << 'EOF'
        .function_sections
        .code
proc _a
        lda #1
        beq _b
        bne *+3
endproc
proc _b
        lda #2
endproc
proc _c
        lda #3
endproc
EOF
sec_check branch "Sections (2)" "00000000        10      1  _a" "_c"
cat > $SD/fall.s45 << 'EOF'
        .function_sections
        .code
        .split "x"
_x:     lda #1
        .split "y"
_y:     lda #2
        rts
        .split "z"
_z:     rts
EOF
sec_check fall "Sections (2)" "00000000         5      1  x" "00000005         1      1  z"
# a hex number in an operand with a '-' (the label-difference scan once
# stopped at the '$' and never returned)
cat > $SD/hexminus.s45 << 'EOF'
        .function_sections
        .code
proc _a
        sbc #$41 - 10
        lda #$FF-$0F
endproc
EOF
if timeout 10 $AS -c $SD/hexminus.s45 -o $SD/hexminus.o45 > /dev/null 2>&1 &&
   ./bin/nm45 -S $SD/hexminus.o45 2>&1 | grep -qF "_a"; then
    echo "SUCCESS: sections hexminus"; passed=$((passed + 1))
else
    echo "FAIL: sections hexminus (a hex number in an operand with '-': ca45 hung or failed)"; failed=$((failed + 1))
fi
# #sym of an external symbol: a low-byte relocation, as #<sym (it
# assembled as #0 without one)
printf '    .extern ext\n    .code\nproc _f\n    lda #ext\n    rts\nendproc\n' > $SD/immext.s45
if $AS -c $SD/immext.s45 -o $SD/immext.o45 >/dev/null 2>&1 &&
   ./bin/nm45 -r $SD/immext.o45 2>&1 | grep -q "0001  R_LOWX      EXTERN  ext"; then
    echo "SUCCESS: #extern immediate relocated"; passed=$((passed + 1))
else
    echo "FAIL: #extern immediate relocated ($(./bin/nm45 -r $SD/immext.o45 2>&1 | head -3))"; failed=$((failed + 1))
fi
# A relocatable address's bytes loaded, stored in zero page and compared
# (cmp.16 reads them back as immediates): the high byte's relocation keeps
# the address's low byte ($1B), which the linker needs for the carry
cat > $SD/relochi.s45 << 'EOF'
        .code
proc _f
        lda #<buf
        sta $0a
        lda #>buf
        sta $0b
        lda #0
        ldx #0
        cmp.16 .AX, $0a
        rts
endproc
        .data
        .res $1b
buf:    .byte 0
EOF
if $AS -c $SD/relochi.s45 -o $SD/relochi.o45 >/dev/null 2>&1 &&
   ./bin/nm45 -r $SD/relochi.o45 2>&1 | grep -q "R_HIGH      DATA    extra=\$1B" &&
   ! ./bin/nm45 -r $SD/relochi.o45 2>&1 | grep -q "R_HIGH      DATA    extra=\$00"; then
    echo "SUCCESS: forwarded high-byte relocation keeps the low byte"; passed=$((passed + 1))
else
    echo "FAIL: forwarded high-byte relocation keeps the low byte ($(./bin/nm45 -r $SD/relochi.o45 2>&1 | tr '\n' ' '))"; failed=$((failed + 1))
fi
# A reference from the code segment into a later text sub-segment ("far":
# cc45 __far functions) holds the target's offset in the merged text, and a
# call into another segment stays a JSR (a relative BSR would break when the
# linker places the segments apart)
printf '        .code\nproc _f\n        jsr target\n        lda #<target\n        ldx #>target\n        rts\nendproc\n        .segment "far"\ntarget: rts\n' > $SD/segref.s45
if $AS -c $SD/segref.s45 -o $SD/segref.o45 >/dev/null 2>&1 &&
   ./bin/objdump45 -d $SD/segref.o45 2>&1 | grep -q "jsr	\$0008" &&
   ./bin/objdump45 -d $SD/segref.o45 2>&1 | grep -q "lda	#\$08"; then
    echo "SUCCESS: references into a later text sub-segment"; passed=$((passed + 1))
else
    echo "FAIL: references into a later text sub-segment ($(./bin/objdump45 -d $SD/segref.o45 2>&1 | grep -E 'jsr|bsr|lda' | tr '\n' ' '))"; failed=$((failed + 1))
fi
# Link-time symbols: an .extern only in a form a relocation holds (sym,
# sym +/- n, <sym, >sym) and never where its value is needed while
# assembling; data after a return is emitted; a weak absolute symbol the code
# used cannot be given another value at link time (ln45 refuses it)
lterr=0
for body in '.res ext' '        lda #ext*3' '        .word ext/2' 'v = ext+1'; do
    printf '        .code\n.extern ext\n%s\n        rts\n' "$body" > $SD/lt.s45
    if $AS -c $SD/lt.s45 -o $SD/lt.o45 >/dev/null 2>&1; then lterr=1; echo "  (accepted: $body)"; fi
done
printf '        .code\n.global _start\n.extern ext\n_start:\n        ldx #>ext+1\n        lda #<ext\n        lda ext,x\n        rts\n        .word ext+4\n' > $SD/lt1.s45
printf '        .code\n.global ext\next = $1234\n' > $SD/lt2.s45
printf '        .code\n.global _start\n.weak wv\nwv = $12\n_start: lda #<wv\n        rts\n' > $SD/lt3.s45
printf '        .code\n.global _main2\n.weak wv\nwv = 7\n_main2: rts\n' > $SD/lt4.s45
printf '        .code\n.global wv\nwv = $3456\n' > $SD/lt5.s45
if [ $lterr = 0 ] &&
   $AS -c $SD/lt1.s45 -o $SD/lt1.o45 >/dev/null 2>&1 && $AS -c $SD/lt2.s45 -o $SD/lt2.o45 >/dev/null 2>&1 &&
   ./bin/ln45 -prg -t 2000 -o $SD/lt12.prg $SD/lt1.o45 $SD/lt2.o45 >/dev/null 2>&1 &&
   [ "$(xxd -s 2 -p $SD/lt12.prg)" = "a212a934bd3412603812" ] &&
   $AS -c $SD/lt3.s45 -o $SD/lt3.o45 >/dev/null 2>&1 && $AS -c $SD/lt4.s45 -o $SD/lt4.o45 >/dev/null 2>&1 &&
   $AS -c $SD/lt5.s45 -o $SD/lt5.o45 >/dev/null 2>&1 &&
   ! ./bin/ln45 -prg -t 2000 -o $SD/lt35.prg $SD/lt3.o45 $SD/lt5.o45 2>&1 | grep -q "^Linked" &&
   ./bin/ln45 -prg -t 2000 -o $SD/lt35.prg $SD/lt3.o45 $SD/lt5.o45 2>&1 | grep -q "is weak in .*assembled with its value there (\$12)" &&
   ./bin/ln45 -prg -t 2000 -o $SD/lt45.prg $SD/lt4.o45 $SD/lt5.o45 >/dev/null 2>&1; then
    echo "SUCCESS: link-time symbols: relocatable forms only, data after rts, weak values kept"; passed=$((passed + 1))
else
    echo "FAIL: link-time symbols: relocatable forms only, data after rts, weak values kept ($(xxd -s 2 -p $SD/lt12.prg 2>/dev/null))"; failed=$((failed + 1))
fi
cat > $SD/diff.s45 << 'EOF'
        .function_sections
        .code
        .split "a"
_a:     lda #1
_a_end: rts
        .split "b"
_b:     rts
        .split "c"
_c:     rts
        .data
len:    .word _a_end - _b
EOF
sec_check diff "Sections (2)" "00000000         4      1  a" "00000004         1      1  c"
cat > $SD/ref.s45 << 'EOF'
        .function_sections
        .global _main
        .code
proc _main
        lda _t1+3
        lda _t2
endproc
        .data
        .split "t1"
_t1:    .byte 1, 2, 3
        .split "t2"
_t2:    .byte 4
EOF
sec_check ref "Sections (3)" "DATA  00000000         3      1  t1" "DATA  00000003         1      1  t2" "text reloc at 00000001 -> section 1 t1"
cat > $SD/resid.s45 << 'EOF'
        .function_sections
        .code
helper: rts
proc _a
        bra helper
endproc
proc _b
        rts
endproc
EOF
sec_check resid "Sections (1)" "00000004         1      1  _b"
cat > $SD/into.s45 << 'EOF'
        .function_sections
        .code
        lda #1
proc _a
        rts
endproc
proc _b
        rts
endproc
EOF
sec_check into "Sections (1)" "00000003         1      1  _b"
cat > $SD/dalign.s45 << 'EOF'
        .function_sections
        .data
        .byte 0
        .split "a"
_a:     .byte 1
        .align 16
        .split "v"
_v:     .word 2
        .bss
        .split "b"
_b:     .res 3
EOF
sec_check dalign "Sections (3)" "DATA  00000001        15      1  a" "DATA  00000010         2     16  v" "BSS   00000000         3      1  b"
# .ref: a section needs a symbol without a relocation; ln45 links the
# symbol's definition only when the section is kept
cat > $SD/refuser.s45 << 'EOF'
        .function_sections
        .global _a, _b
        .code
proc _a
        .ref _needed
        rts
endproc
proc _b
        rts
endproc
EOF
sec_check refuser "Sections (2)" "section 0 _a needs _needed"
printf '        .global _needed\n        .code\n_needed: rts\n' > $SD/needed.s45
printf '        .global _main\n        .extern %s\n        .code\n_main:  jsr %s\n        rts\n' _a _a > $SD/usea.s45
printf '        .global _main\n        .extern %s\n        .code\n_main:  jsr %s\n        rts\n' _b _b > $SD/useb.s45
rm -f $SD/ref.lib
if $AS -c $SD/needed.s45 -o $SD/needed.o45 >/dev/null 2>&1 &&
   $AS -c $SD/usea.s45 -o $SD/usea.o45 >/dev/null 2>&1 && $AS -c $SD/useb.s45 -o $SD/useb.o45 >/dev/null 2>&1 &&
   ./bin/ar45 c $SD/ref.lib $SD/refuser.o45 $SD/needed.o45 >/dev/null 2>&1 &&
   ./bin/ln45 -prg -t 2000 -m -o $SD/usea.prg $SD/usea.o45 $SD/ref.lib 2>&1 | grep -q '_needed' &&
   ! ./bin/ln45 -prg -t 2000 -m -o $SD/useb.prg $SD/useb.o45 $SD/ref.lib 2>&1 | grep -q '_needed'; then
    echo "SUCCESS: .ref links its symbol only with the section that needs it"
    passed=$((passed + 1))
else
    echo "FAIL: .ref"
    failed=$((failed + 1))
fi
# -ffunction-sections does the same as the directive; a sectioned object
# links to the same program as without sections (ln45 does not drop yet)
grep -v function_sections $SD/ref.s45 > $SD/ref_plain.s45
if $AS -c -ffunction-sections $SD/ref_plain.s45 -o $SD/ref_flag.o45 >/dev/null 2>&1 &&
   cmp -s $SD/ref.o45 $SD/ref_flag.o45 &&
   $AS -c $SD/ref_plain.s45 -o $SD/ref_plain.o45 >/dev/null 2>&1 &&
   ./bin/ln45 -prg -t 2000 -o $SD/ref.prg $SD/ref.o45 >/dev/null 2>&1 &&
   ./bin/ln45 -prg -t 2000 -o $SD/ref_plain.prg $SD/ref_plain.o45 >/dev/null 2>&1 &&
   cmp -s $SD/ref.prg $SD/ref_plain.prg; then
    echo "SUCCESS: -ffunction-sections as the directive; sectioned objects link unchanged"
    passed=$((passed + 1))
else
    echo "FAIL: -ffunction-sections / linking a sectioned object"
    failed=$((failed + 1))
fi

# A compare with a character literal ('0' is $30) after a load is not the
# "cmp #0" the optimiser drops
printf '    .org $2000\n    lda $10\n    cmp #'"'"'0'"'"'\n    beq done\n    lda #1\ndone:\n    rts\n' > build/test_cmp_char.s45
$AS build/test_cmp_char.s45 -o build/test_cmp_char.bin >/dev/null 2>&1
if [ "$(xxd -p build/test_cmp_char.bin | head -c 12)" = "a510c930f002" ]; then
    echo "SUCCESS: cmp #'0' after a load is kept"
    passed=$((passed + 1))
else
    echo "FAIL: cmp #'0' after a load ($(xxd -p build/test_cmp_char.bin))"
    failed=$((failed + 1))
fi

# A forced direct mode with an indirect operand (lda.zp ($10),y) is an error,
# not a silently different instruction
printf '    .org $2000\n    lda.zp ($10),y\n    rts\n' > build/test_zp_indirect.s45
if out=$($AS build/test_zp_indirect.s45 -o build/test_zp_indirect.bin 2>&1); then
    echo "FAIL: lda.zp (\$10),y assembled"
    failed=$((failed + 1))
elif grep -q "can't take an indirect operand" <<< "$out"; then
    echo "SUCCESS: lda.zp (\$10),y is an error"
    passed=$((passed + 1))
else
    echo "FAIL: lda.zp (\$10),y: unexpected error: $(head -1 <<< "$out")"
    failed=$((failed + 1))
fi

# An address written with its leading zeros ($00D0) is absolute, $D0 base
# page (the relocated base page moves only the latter); a symbol (sta (ptr,x)
# with ptr = $fb) takes the base-page indirect form
printf '.org $2000\nptr = $fb\n sta $00D0\n sta $D0\n lda $00D0,x\n ldx $00D0,y\n sta (ptr,x)\n jmp (ptr)\n' > build/test_abs00.s45
if $AS build/test_abs00.s45 -o build/test_abs00.bin >/dev/null 2>&1 &&
   [ "$(xxd -p build/test_abs00.bin)" = "8dd00085d0bdd000bed00081fb6cfb00" ]; then
    echo "SUCCESS: \$00D0 is absolute, (sym,x) base page"
    passed=$((passed + 1))
else
    echo "FAIL: \$00D0 / (sym,x) ($(xxd -p build/test_abs00.bin 2>/dev/null))"
    failed=$((failed + 1))
fi

# A local label nothing refers to (left by branch inversion) is reached
# only by falling through: the reload after it goes; a label a branch
# reaches is a merge point: the load after it stays. cmp.16 keeps A/X. (The
# lda #0 sets the flags again: the reload's own N/Z must not reach the rts)
printf '.org $2000\n lda $32\n ldx $33\n cmp.16 .AX, #8192\n bcc @b\n bra @e\n@b:\n lda $32\n ldx $33\n sta $40\n stx $41\n sta $42\n stx $43\n@e:\n lda #0\n rts\n' > build/test_unref_label.s45
printf '.org $2000\n lda #1\n beq @x\n lda #2\n@x:\n lda #2\n sta $40\n rts\n' > build/test_merge_label.s45
$AS build/test_unref_label.s45 -o build/test_unref_label.bin >/dev/null 2>&1
$AS build/test_merge_label.s45 -o build/test_merge_label.bin >/dev/null 2>&1
U=$(xxd -p build/test_unref_label.bin | tr -d '\n'); M=$(xxd -p build/test_merge_label.bin | tr -d '\n')
if [ "$U" = "a532a633e020d002c900b0088540864185428643a90060" ] && [ "$M" = "a901f002a902a902854060" ]; then
    echo "SUCCESS: unreferenced labels keep register knowledge, merge points reset it"
    passed=$((passed + 1))
else
    echo "FAIL: label knowledge (unref $U, merge $M)"
    failed=$((failed + 1))
fi

# #<(sym+2) / #>(sym+2) in an object: relocated like #<sym+2 / #>sym+2
printf '    .segment "bss"\nbuf: .res 300\n    .segment "code"\n    lda #<(buf+2)\n    adc #>(buf+2)\n' > build/test_paren_reloc.s45
R=$($AS -c build/test_paren_reloc.s45 -o build/test_paren_reloc.o45 >/dev/null 2>&1 && ./bin/nm45 -r build/test_paren_reloc.o45)
if grep -q '0001  R_LOWX      BSS' <<< "$R" && grep -q '0003  R_HIGH      BSS     extra=\$02' <<< "$R"; then
    echo "SUCCESS: #<(sym+n) / #>(sym+n) are relocated"
    passed=$((passed + 1))
else
    echo "FAIL: parenthesized lo/hi relocations ($R)"
    failed=$((failed + 1))
fi

# -ffunction-sections: a proc with a SeqExtract routine (BSRs inserted by the
# optimizer) is a section of its own, so ln45 can drop it
cat > build/test_seqx_sec.s45 <<'END'
    .segment "code"
    proc f1
    lda #$11
    sta $3000
    lda #$22
    sta $3001
    lda #$11
    sta $3000
    lda #$22
    sta $3001
    lda #$11
    sta $3000
    lda #$22
    sta $3001
    endproc
    proc f2
    rts
    endproc
END
S=$($AS -c -ffunction-sections build/test_seqx_sec.s45 -o build/test_seqx_sec.o45 >/dev/null 2>&1 && ./bin/nm45 -S build/test_seqx_sec.o45)
if grep -q ' f1$' <<< "$S" && grep -q ' f2$' <<< "$S"; then
    echo "SUCCESS: a proc with a SeqExtract routine is its own section"
    passed=$((passed + 1))
else
    echo "FAIL: SeqExtract proc sections ($S)"
    failed=$((failed + 1))
fi

# The dead LDX #imm pass: Q instructions read X (Q is A:X:Y:Z): ldx #252 /
# stq $10 / ldx #0 keeps the 252 (it was removed: -1000 passed as $FFFF0018)
cat > build/test_deadldx.s45 << 'END'
    proc f
    lda #24
    ldx #252
    ldy #255
    ldz #255
    stq $10
    lda #70
    ldx #0
    sta $14
    rts
    endproc
END
D=$($AS -c build/test_deadldx.s45 -o build/test_deadldx.o45 >/dev/null 2>&1 && ./bin/objdump45 -d build/test_deadldx.o45)
if grep -q 'ldx.*#\$fc' <<< "$D"; then
    echo "SUCCESS: dead LDX: a Q store reads X"
    passed=$((passed + 1))
else
    echo "FAIL: dead LDX removed the ldx before stq ($D)"
    failed=$((failed + 1))
fi

if [ $failed -eq 0 ]; then
    echo "All assembler feature tests passed!"
    exit 0
else
    echo "$failed tests failed."
    exit 1
fi
