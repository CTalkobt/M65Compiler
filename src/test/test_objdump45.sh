#!/bin/bash
# Test objdump45 display modes - uses shared test utilities

source "$(dirname "$0")/test-lib.sh"

OD="${OD:-./bin/objdump45}"

print_section "objdump45 tests"

# Build test input: .o45 with code, data, imports, exports, relocs
mkdir -p build/test
cat > build/test/objdump_test.s << 'EOF'
.extern _printf
.global _main
.global _count
.weak _default_handler
.segment "code"
* = $2000
_main:
    lda #$41
    jsr _printf
    ldx _count
    beq _done
    dex
_done:
    rts
_default_handler:
    rti
.segment "data"
_count: .byte $05
EOF

compile_and_assemble build/test/objdump_test.s build/test/objdump_test.c 2>/dev/null || {
    $AS -c build/test/objdump_test.s -o build/test/objdump_test.o45 2>&1
}

# Test -V (version)
OUT=$($OD -V 2>&1)
echo "$OUT" | grep -q "objdump45" && print_pass "-V prints tool name" || print_fail "-V prints tool name"

# Test -f (file header)
OUT=$($OD -f build/test/objdump_test.o45 2>&1)
echo "$OUT" | grep -q "o45-32" && print_pass "-f shows format" || print_fail "-f shows format"
echo "$OUT" | grep -q "45GS02" && print_pass "-f shows CPU" || print_fail "-f shows CPU"

# Test -h (section headers)
OUT=$($OD -h build/test/objdump_test.o45 2>&1)
echo "$OUT" | grep -q "TEXT" && print_pass "-h shows TEXT" || print_fail "-h shows TEXT"
echo "$OUT" | grep -q "DATA" && print_pass "-h shows DATA" || print_fail "-h shows DATA"

# Test -t (symbol table)
OUT=$($OD -t build/test/objdump_test.o45 2>&1)
echo "$OUT" | grep -q "_main" && print_pass "-t shows _main" || print_fail "-t shows _main"
echo "$OUT" | grep -q "SYMBOL TABLE" && print_pass "-t shows header" || print_fail "-t shows header"

# Test -r (relocations)
OUT=$($OD -r build/test/objdump_test.o45 2>&1)
echo "$OUT" | grep -q "RELOCATION" && print_pass "-r shows RELOCATION" || print_fail "-r shows RELOCATION"

# Test -s (hex dump)
OUT=$($OD -s build/test/objdump_test.o45 2>&1)
echo "$OUT" | grep -q "Contents of section" && print_pass "-s shows contents" || print_fail "-s shows contents"

# Test -d (disassembly)
OUT=$($OD -d build/test/objdump_test.o45 2>&1)
echo "$OUT" | grep -q "Disassembly" && print_pass "-d shows disassembly" || print_fail "-d shows disassembly"
echo "$OUT" | grep -q "_main" && print_pass "-d shows labels" || print_fail "-d shows labels"

# Test -a (all)
OUT=$($OD -a build/test/objdump_test.o45 2>&1)
echo "$OUT" | grep -q "o45-32" && print_pass "-a includes header" || print_fail "-a includes header"
echo "$OUT" | grep -q "Disassembly" && print_pass "-a includes disasm" || print_fail "-a includes disasm"

# Test combined flags
OUT=$($OD -fth build/test/objdump_test.o45 2>&1)
echo "$OUT" | grep -q "45GS02" && print_pass "-fth combined flags" || print_fail "-fth combined flags"

# Test multiple files
OUT=$($OD -t build/test/objdump_test.o45 build/test/objdump_test.o45 2>&1)
[ $(echo "$OUT" | grep -c "_main") -ge 2 ] && print_pass "multi-file output" || print_fail "multi-file output"

# Test error cases
$OD 2>/dev/null; [ $? -ne 0 ] && print_pass "no args error" || print_fail "no args error"
$OD build/test/objdump_test.o45 2>/dev/null; [ $? -ne 0 ] && print_pass "no flags error" || print_fail "no flags error"
OUT=$($OD -f /nonexistent 2>&1); [ $? -ne 0 ] && print_pass "missing file error" || print_fail "missing file error"

test_summary
exit $?
