#!/bin/bash

# Test script for objdump45 — validates all display modes against known .o45 input

AS="./bin/ca45"
OD="./bin/objdump45"
BUILD="build/test"
mkdir -p "$BUILD"

passed=0
failed=0

pass() { echo "  PASS: $1"; passed=$((passed + 1)); }
fail() { echo "  FAIL: $1"; failed=$((failed + 1)); }

assert_contains() {
    local label="$1" output="$2" expected="$3"
    if echo "$output" | grep -qF -- "$expected"; then
        pass "$label"
    else
        fail "$label — expected to find: $expected"
    fi
}

assert_not_contains() {
    local label="$1" output="$2" unexpected="$3"
    if echo "$output" | grep -qF -- "$unexpected"; then
        fail "$label — should not contain: $unexpected"
    else
        pass "$label"
    fi
}

assert_exit() {
    local label="$1" code="$2" expected="$3"
    if [ "$code" -eq "$expected" ]; then
        pass "$label"
    else
        fail "$label — exit code $code, expected $expected"
    fi
}

# ─── Build test input: a .o45 with code, data, imports, exports, relocs ──────

cat > "$BUILD/objdump_test.s" << 'EOF'
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

echo "=== Assembling test input ==="
$AS -c "$BUILD/objdump_test.s" -o "$BUILD/objdump_test.o45" 2>&1
if [ $? -ne 0 ]; then
    echo "FATAL: failed to assemble test input"
    exit 1
fi

# ─── Test 1: -V (version) ───────────────────────────────────────────────────

echo ""
echo "--- Test: version ---"
OUT=$($OD -V 2>&1)
assert_exit "-V exits 0" $? 0
assert_contains "-V prints tool name" "$OUT" "objdump45"
assert_contains "-V prints version" "$OUT" "v0.99"

# ─── Test 2: -? (help) ──────────────────────────────────────────────────────

echo ""
echo "--- Test: help ---"
OUT=$($OD -? 2>&1)
assert_exit "-? exits 0" $? 0
assert_contains "-? mentions -d" "$OUT" "-d"
assert_contains "-? mentions -f" "$OUT" "-f"
assert_contains "-? mentions disassemble" "$OUT" "Disassemble"

# ─── Test 3: no args → error ────────────────────────────────────────────────

echo ""
echo "--- Test: no args ---"
$OD 2>/dev/null
assert_exit "no args exits non-zero" $? 1

# ─── Test 4: no display flags → error ───────────────────────────────────────

echo ""
echo "--- Test: no display flags ---"
$OD "$BUILD/objdump_test.o45" 2>/dev/null
assert_exit "no display flags exits non-zero" $? 1

# ─── Test 5: bad file → error ───────────────────────────────────────────────

echo ""
echo "--- Test: bad file ---"
OUT=$($OD -f /nonexistent 2>&1)
assert_exit "missing file exits non-zero" $? 1
assert_contains "missing file error message" "$OUT" "cannot open"

echo ""
echo "--- Test: invalid format ---"
echo "not an o45 file" > "$BUILD/bad.o45"
OUT=$($OD -f "$BUILD/bad.o45" 2>&1)
assert_exit "bad format exits non-zero" $? 1

# ─── Test 6: -f (file header) ───────────────────────────────────────────────

echo ""
echo "--- Test: -f file header ---"
OUT=$($OD -f "$BUILD/objdump_test.o45" 2>&1)
assert_exit "-f exits 0" $? 0
assert_contains "-f shows format" "$OUT" "o45-32"
assert_contains "-f shows CPU" "$OUT" "45GS02"
assert_contains "-f shows SIZE32" "$OUT" "SIZE32"
assert_contains "-f shows CPUEXT" "$OUT" "CPUEXT"

# ─── Test 7: -h (section headers) ───────────────────────────────────────────

echo ""
echo "--- Test: -h section headers ---"
OUT=$($OD -h "$BUILD/objdump_test.o45" 2>&1)
assert_exit "-h exits 0" $? 0
assert_contains "-h shows TEXT" "$OUT" "TEXT"
assert_contains "-h shows DATA" "$OUT" "DATA"
assert_contains "-h shows CODE type" "$OUT" "CODE"
assert_contains "-h shows Sections" "$OUT" "Sections"

# ─── Test 8: -t (symbol table) ──────────────────────────────────────────────

echo ""
echo "--- Test: -t symbol table ---"
OUT=$($OD -t "$BUILD/objdump_test.o45" 2>&1)
assert_exit "-t exits 0" $? 0
assert_contains "-t shows _main export" "$OUT" "_main"
assert_contains "-t shows _count export" "$OUT" "_count"
assert_contains "-t shows _printf import" "$OUT" "_printf"
assert_contains "-t shows U for undefined" "$OUT" "U _printf"
assert_contains "-t shows T for text" "$OUT" "T _main"
assert_contains "-t shows D for data" "$OUT" "D _count"
assert_contains "-t shows W for weak" "$OUT" "W _default_handler"
assert_contains "-t header" "$OUT" "SYMBOL TABLE"

# ─── Test 9: -r (relocations) ───────────────────────────────────────────────

echo ""
echo "--- Test: -r relocations ---"
OUT=$($OD -r "$BUILD/objdump_test.o45" 2>&1)
assert_exit "-r exits 0" $? 0
assert_contains "-r shows RELOCATION" "$OUT" "RELOCATION"
assert_contains "-r shows _printf reloc" "$OUT" "_printf"
assert_contains "-r shows R_WORD" "$OUT" "R_WORD"

# ─── Test 10: -s (hex dump) ─────────────────────────────────────────────────

echo ""
echo "--- Test: -s section contents ---"
OUT=$($OD -s "$BUILD/objdump_test.o45" 2>&1)
assert_exit "-s exits 0" $? 0
assert_contains "-s shows Contents of section" "$OUT" "Contents of section"
assert_contains "-s shows TEXT" "$OUT" "TEXT"
# The data section contains $05
assert_contains "-s shows data byte" "$OUT" "05"

# ─── Test 11: -d (disassembly) ──────────────────────────────────────────────

echo ""
echo "--- Test: -d disassembly ---"
OUT=$($OD -d "$BUILD/objdump_test.o45" 2>&1)
assert_exit "-d exits 0" $? 0
assert_contains "-d shows Disassembly" "$OUT" "Disassembly of section"
assert_contains "-d shows _main label" "$OUT" "<_main>"
assert_contains "-d shows lda mnemonic" "$OUT" "lda"
assert_contains "-d shows jsr mnemonic" "$OUT" "jsr"
assert_contains "-d shows rts mnemonic" "$OUT" "rts"
assert_contains "-d shows rti mnemonic" "$OUT" "rti"
assert_contains "-d shows beq mnemonic" "$OUT" "beq"
assert_contains "-d shows dex mnemonic" "$OUT" "dex"
assert_contains "-d shows #\$41 immediate" "$OUT" '#$41'
# Weak symbol should appear as label too
assert_contains "-d shows weak label" "$OUT" "<_default_handler>"

# ─── Test 12: -a (all) ──────────────────────────────────────────────────────

echo ""
echo "--- Test: -a all info ---"
OUT=$($OD -a "$BUILD/objdump_test.o45" 2>&1)
assert_exit "-a exits 0" $? 0
assert_contains "-a includes header" "$OUT" "o45-32"
assert_contains "-a includes sections" "$OUT" "Sections"
assert_contains "-a includes symbols" "$OUT" "SYMBOL TABLE"
assert_contains "-a includes relocs" "$OUT" "RELOCATION"
assert_contains "-a includes hex" "$OUT" "Contents of section"
assert_contains "-a includes disasm" "$OUT" "Disassembly"

# ─── Test 13: combined flags ────────────────────────────────────────────────

echo ""
echo "--- Test: combined flags -fth ---"
OUT=$($OD -fth "$BUILD/objdump_test.o45" 2>&1)
assert_exit "-fth exits 0" $? 0
assert_contains "-fth has header" "$OUT" "45GS02"
assert_contains "-fth has symbols" "$OUT" "_main"
assert_contains "-fth has sections" "$OUT" "Sections"
assert_not_contains "-fth has no disasm" "$OUT" "Disassembly"

# ─── Test 14: multiple files ────────────────────────────────────────────────

echo ""
echo "--- Test: multiple files ---"
OUT=$($OD -t "$BUILD/objdump_test.o45" "$BUILD/objdump_test.o45" 2>&1)
assert_exit "multi-file exits 0" $? 0
# Should see _main twice (once per file)
COUNT=$(echo "$OUT" | grep -c "_main")
if [ "$COUNT" -ge 2 ]; then
    pass "multi-file shows symbols from both"
else
    fail "multi-file — expected _main at least twice, got $COUNT"
fi

# ─── Test 15: disassembly of library .o45 (if available) ────────────────────

if [ -f lib/build/puts.o45 ]; then
    echo ""
    echo "--- Test: disassembly of stdlib puts.o45 ---"
    OUT=$($OD -d lib/build/puts.o45 2>&1)
    assert_exit "puts.o45 -d exits 0" $? 0
    assert_contains "puts.o45 has _puts label" "$OUT" "<_puts>"
    assert_contains "puts.o45 has tsx (proc prologue)" "$OUT" "tsx"
    assert_contains "puts.o45 has rts" "$OUT" "rts"
fi

# ─── Test 16: disassembly byte accuracy ─────────────────────────────────────
# Assemble known instructions, disassemble, verify mnemonic+operand output

echo ""
echo "--- Test: disassembly instruction accuracy ---"

cat > "$BUILD/objdump_accuracy.s" << 'EOF'
.global _test
.segment "code"
* = $1000

_test:
    clv
    lda #$FF
    sta $2000
    ldx $80,y
    lda ($10,x)
    sta ($20),y
    lda ($30),z
    lda ($40,sp),y
    jmp ($5000)
    jsr ($6000,x)
    phw #$ABCD
    rts #2
EOF

$AS -c "$BUILD/objdump_accuracy.s" -o "$BUILD/objdump_accuracy.o45" 2>&1
if [ $? -eq 0 ]; then
    OUT=$($OD -d "$BUILD/objdump_accuracy.o45" 2>&1)
    assert_exit "accuracy test exits 0" $? 0
    assert_contains "clv" "$OUT" "clv"
    assert_contains "lda #\$FF" "$OUT" '#$FF'
    assert_contains "sta \$2000" "$OUT" '$2000'
    assert_contains "ldx zp,y" "$OUT" '$80,y'
    assert_contains "lda (zp,x)" "$OUT" '($10,x)'
    assert_contains "sta (zp),y" "$OUT" '($20),y'
    assert_contains "lda (zp),z" "$OUT" '($30),z'
    assert_contains "lda (zp,sp),y" "$OUT" '($40,sp),y'
    assert_contains "jmp (abs)" "$OUT" '($5000)'
    assert_contains "jsr (abs,x)" "$OUT" '($6000,x)'
    assert_contains "phw #imm16" "$OUT" '#$ABCD'
    assert_contains "rts #imm" "$OUT" '#$02'
else
    fail "accuracy test — assembly failed"
fi

# ─── Test 17: branch target annotation ──────────────────────────────────────

echo ""
echo "--- Test: branch target annotation ---"

cat > "$BUILD/objdump_branch.s" << 'EOF'
.global _start
.global _loop
.global _end
.segment "code"
* = $3000

_start:
    ldx #$05
_loop:
    dex
    bne _loop
    jsr _end
_end:
    rts
EOF

$AS -c "$BUILD/objdump_branch.s" -o "$BUILD/objdump_branch.o45" 2>&1
if [ $? -eq 0 ]; then
    OUT=$($OD -d "$BUILD/objdump_branch.o45" 2>&1)
    assert_exit "branch annotation exits 0" $? 0
    assert_contains "bne annotated with <_loop>" "$OUT" "<_loop>"
    assert_contains "jsr annotated with <_end>" "$OUT" "<_end>"
    assert_contains "_start label" "$OUT" "<_start>"
else
    fail "branch annotation — assembly failed"
fi

# ─── Test 18: PRG file disassembly ───────────────────────────────────────────

echo ""
echo "--- Test: PRG disassembly ---"

cat > "$BUILD/objdump_prg.s" << 'EOF'
.org $0810
    lda #$42
    jsr $FFD2
    rts
EOF

$AS "$BUILD/objdump_prg.s" -o "$BUILD/objdump_prg.prg" 2>&1
if [ $? -eq 0 ]; then
    # Test -f on PRG (shows format and base address)
    OUT=$($OD -f "$BUILD/objdump_prg.prg" 2>&1)
    assert_exit "prg -f exits 0" $? 0
    assert_contains "prg format detected" "$OUT" "prg"
    assert_contains "prg base address" "$OUT" '$0810'

    # Test -d on PRG (auto-detects load address)
    OUT=$($OD -d "$BUILD/objdump_prg.prg" 2>&1)
    assert_exit "prg -d exits 0" $? 0
    assert_contains "prg disassembly header" "$OUT" "Disassembly"
    assert_contains "prg lda #\$42" "$OUT" '#$42'
    assert_contains "prg jsr" "$OUT" "jsr"
    assert_contains "prg rts" "$OUT" "rts"
    assert_contains "prg address at 0810" "$OUT" "0810:"

    # Test -s hex dump on PRG
    OUT=$($OD -s "$BUILD/objdump_prg.prg" 2>&1)
    assert_exit "prg -s exits 0" $? 0
    assert_contains "prg hex dump header" "$OUT" "Contents of section"

    # Test -b override on PRG
    OUT=$($OD -d -b '$0000' "$BUILD/objdump_prg.prg" 2>&1)
    assert_exit "prg -b override exits 0" $? 0
    assert_contains "prg -b overrides base" "$OUT" "0000:"
else
    fail "prg test — assembly failed"
fi

# ─── Test 19: raw .bin file disassembly ──────────────────────────────────────

echo ""
echo "--- Test: raw binary disassembly ---"

# Strip the PRG header to create a raw .bin
if [ -f "$BUILD/objdump_prg.prg" ]; then
    dd if="$BUILD/objdump_prg.prg" of="$BUILD/objdump_raw.bin" bs=1 skip=2 2>/dev/null

    # .bin with -b flag
    OUT=$($OD -d -b '$2000' "$BUILD/objdump_raw.bin" 2>&1)
    assert_exit "bin -d exits 0" $? 0
    assert_contains "bin disassembly header" "$OUT" "Disassembly"
    assert_contains "bin address at 2000" "$OUT" "2000:"
    assert_contains "bin lda" "$OUT" "lda"

    # .bin with -f shows format info
    OUT=$($OD -f -b '$2000' "$BUILD/objdump_raw.bin" 2>&1)
    assert_exit "bin -f exits 0" $? 0
    assert_contains "bin format" "$OUT" "raw binary"
    assert_contains "bin base" "$OUT" '$2000'

    # .bin with -b 0 (default)
    OUT=$($OD -d -b 0 "$BUILD/objdump_raw.bin" 2>&1)
    assert_exit "bin -b 0 exits 0" $? 0
    assert_contains "bin -b 0 starts at 0000" "$OUT" "0000:"

    # .bin without -b defaults to base $0000
    OUT=$($OD -d "$BUILD/objdump_raw.bin" 2>&1)
    assert_exit "bin without -b defaults ok" $? 0
    assert_contains "bin without -b starts at 0000" "$OUT" "0000:"
else
    fail "bin test — PRG file not available"
fi

# ─── Test 20: PRG with BASIC upstart ────────────────────────────────────────

echo ""
echo "--- Test: BASIC upstart PRG ---"

cat > "$BUILD/objdump_basic.s" << 'EOF'
.basicUpstart $080D
    lda #$00
    sta $D020
    rts
EOF

$AS "$BUILD/objdump_basic.s" -o "$BUILD/objdump_basic.prg" 2>&1
if [ $? -eq 0 ]; then
    OUT=$($OD -fd "$BUILD/objdump_basic.prg" 2>&1)
    assert_exit "basic prg exits 0" $? 0
    assert_contains "basic prg format" "$OUT" "prg"
    assert_contains "basic prg has lda" "$OUT" "lda"
    assert_contains "basic prg has sta" "$OUT" "sta"
else
    fail "basic prg — assembly failed"
fi

# ─── Summary ─────────────────────────────────────────────────────────────────

echo ""
echo "========================================"
echo "objdump45 tests: $passed passed, $failed failed"
echo "========================================"

if [ $failed -eq 0 ]; then
    exit 0
else
    exit 1
fi
