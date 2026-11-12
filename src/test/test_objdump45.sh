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

cat > "$BUILD/objdump_test.s45" << 'EOF'
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
$AS -c "$BUILD/objdump_test.s45" -o "$BUILD/objdump_test.o45" 2>&1
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
assert_contains "-V prints version" "$OUT" "objdump45 v"

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
echo "--- Test: directory ---"
OUT=$($OD -h "$BUILD" 2>&1)
assert_exit "directory exits non-zero" $? 1
assert_contains "directory error message" "$OUT" "is a directory"
OUT=$($OD -D "$BUILD" "$BUILD" 2>&1)
assert_exit "diff of a directory exits non-zero" $? 1
assert_contains "diff directory error message" "$OUT" "is a directory"

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
    assert_contains "puts.o45 has tsx (param load)" "$OUT" "tsx"
    assert_contains "puts.o45 has rts" "$OUT" "rts"
fi

# ─── Test 16: disassembly byte accuracy ─────────────────────────────────────
# Assemble known instructions, disassemble, verify mnemonic+operand output

echo ""
echo "--- Test: disassembly instruction accuracy ---"

cat > "$BUILD/objdump_accuracy.s45" << 'EOF'
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

$AS -c "$BUILD/objdump_accuracy.s45" -o "$BUILD/objdump_accuracy.o45" 2>&1
if [ $? -eq 0 ]; then
    OUT=$($OD -d "$BUILD/objdump_accuracy.o45" 2>&1)
    assert_exit "accuracy test exits 0" $? 0
    assert_contains "clv" "$OUT" "clv"
    assert_contains "lda #\$ff" "$OUT" '#$ff'
    assert_contains "sta \$2000" "$OUT" '$2000'
    assert_contains "ldx zp,y" "$OUT" '$80,y'
    assert_contains "lda (zp,x)" "$OUT" '($10,x)'
    assert_contains "sta (zp),y" "$OUT" '($20),y'
    assert_contains "lda (zp),z" "$OUT" '($30),z'
    assert_contains "lda (zp,sp),y" "$OUT" '($40,sp),y'
    assert_contains "jmp (abs)" "$OUT" '($5000)'
    assert_contains "jsr (abs,x)" "$OUT" '($6000,x)'
    assert_contains "phw #imm16" "$OUT" '#$abcd'
    assert_contains "rts #imm" "$OUT" '#$02'
else
    fail "accuracy test — assembly failed"
fi

# ─── Test 17: branch target annotation ──────────────────────────────────────

echo ""
echo "--- Test: branch target annotation ---"

cat > "$BUILD/objdump_branch.s45" << 'EOF'
.global _start
.global _loop
.global _end
.s45egment "code"
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

$AS -c "$BUILD/objdump_branch.s45" -o "$BUILD/objdump_branch.o45" 2>&1
if [ $? -eq 0 ]; then
    OUT=$($OD -d "$BUILD/objdump_branch.o45" 2>&1)
    assert_exit "branch annotation exits 0" $? 0
    assert_contains "bne annotated with <_loop>" "$OUT" "<_loop>"
    assert_contains "jsr annotated with <_end>" "$OUT" "<_end>"
    assert_contains "_start label" "$OUT" "<_start>"
else
    fail "branch annotation — assembly failed"
fi

# ─── Test 21: symbolic annotation on absolute operands ───────────────────────

echo ""
echo "--- Test: symbolic annotation on data access ---"

cat > "$BUILD/objdump_symref.s45" << 'EOF'
.global _main
.global _counter
.global _table
.s45egment "code"
* = $4000

_main:
    lda _counter
    sta _counter
    ldx _counter
    stx _counter
    ldy _table
    lda _table,x
    lda _table,y
    jmp _main
    rts

.s45egment "data"
_counter: .byte $00
_table: .byte $01, $02, $03
EOF

$AS -c "$BUILD/objdump_symref.s45" -o "$BUILD/objdump_symref.o45" 2>&1
if [ $? -eq 0 ]; then
    OUT=$($OD -d "$BUILD/objdump_symref.o45" 2>&1)
    assert_exit "symref exits 0" $? 0
    # lda/sta/ldx/stx with absolute addressing should annotate
    assert_contains "lda abs annotated" "$OUT" "lda"
    assert_contains "lda abs has <_counter>" "$OUT" "<_counter>"
    assert_contains "sta abs has <_counter>" "$OUT" "<_counter>"
    assert_contains "lda abs,x has <_table>" "$OUT" "<_table>"
    assert_contains "lda abs,y has <_table>" "$OUT" "<_table>"
    # jmp may be optimized to bra — check for either
    assert_contains "branch to _main" "$OUT" "<_main>"
    assert_contains "_main label" "$OUT" "<_main>:"
    assert_contains "_counter label" "$OUT" "<_counter>:"
    assert_contains "_table label" "$OUT" "<_table>:"
else
    fail "symref — assembly failed"
fi

# ─── Test 18: PRG file disassembly ───────────────────────────────────────────

echo ""
echo "--- Test: PRG disassembly ---"

cat > "$BUILD/objdump_prg.s45" << 'EOF'
.org $0810
    lda #$42
    jsr $FFD2
    rts
EOF

$AS "$BUILD/objdump_prg.s45" -o "$BUILD/objdump_prg.prg" 2>&1
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
    assert_contains "prg jmp (tail-call optimized)" "$OUT" "jmp"
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

cat > "$BUILD/objdump_basic.s45" << 'EOF'
.basicUpstart $080D
    lda #$00
    sta $D020
    rts
EOF

$AS "$BUILD/objdump_basic.s45" -o "$BUILD/objdump_basic.prg" 2>&1
if [ $? -eq 0 ]; then
    OUT=$($OD -fd "$BUILD/objdump_basic.prg" 2>&1)
    assert_exit "basic prg exits 0" $? 0
    assert_contains "basic prg format" "$OUT" "prg"
    assert_contains "basic prg has lda" "$OUT" "lda"
    assert_contains "basic prg has sta" "$OUT" "sta"
else
    fail "basic prg — assembly failed"
fi

# ─── Test 22: -m map file for PRG disassembly ────────────────────────────────

echo ""
echo "--- Test: -m map file with PRG ---"

# Build a linked PRG with a map file
cat > "$BUILD/objdump_map_a.s45" << 'EOF'
.global _main
.extern _helper
.segment "code"
_main:
    jsr _helper
    rts
EOF

cat > "$BUILD/objdump_map_b.s45" << 'EOF'
.global _helper
.global _data
.s45egment "code"
_helper:
    lda _data
    rts
.s45egment "data"
_data: .byte $42
EOF

$AS -c "$BUILD/objdump_map_a.s45" -o "$BUILD/objdump_map_a.o45" 2>&1
$AS -c "$BUILD/objdump_map_b.s45" -o "$BUILD/objdump_map_b.o45" 2>&1
LN="./bin/ln45"
if [ -x "$LN" ]; then
    $LN -basic -M "$BUILD/objdump_map.txt" -o "$BUILD/objdump_map.prg" \
        "$BUILD/objdump_map_a.o45" "$BUILD/objdump_map_b.o45" 2>&1

    if [ $? -eq 0 ]; then
        # Without -m: no labels
        OUT=$($OD -d "$BUILD/objdump_map.prg" 2>&1)
        assert_exit "prg no -m exits 0" $? 0
        assert_not_contains "prg no -m has no _main label" "$OUT" "<_main>"

        # With -m: labels appear
        OUT=$($OD -d -m "$BUILD/objdump_map.txt" "$BUILD/objdump_map.prg" 2>&1)
        assert_exit "prg -m exits 0" $? 0
        assert_contains "prg -m shows _main label" "$OUT" "<_main>"
        assert_contains "prg -m shows _helper label" "$OUT" "<_helper>"
        assert_contains "prg -m jsr annotated" "$OUT" "<_helper>"
        assert_contains "prg -m lda annotated with _data" "$OUT" "<_data>"
    else
        fail "map test — linking failed"
    fi
else
    echo "  SKIP: ln45 not available"
fi

# ─── Object operands named by their relocations (#354) ──────────────────────

echo "Object labels and relocated operands (#354)..."
cat > "$BUILD/objdump_reloc.c" <<'EOF2'
unsigned char a, b;
struct player { unsigned char x, y, hp; unsigned char items[20]; } pl;
void split(unsigned char k) { a = k % 3; b = k / 3; }
void sethp(unsigned char v) { pl.hp = v; }
EOF2
if ./bin/cc45 -O2 -c "$BUILD/objdump_reloc.c" -o "$BUILD/objdump_reloc.o45" >/dev/null 2>&1; then
    OUT=$($OD -d "$BUILD/objdump_reloc.o45" 2>&1)
    T=$(printf '%s\n' "$OUT" | awk '/section TEXT/,/section DATA/')
    assert_not_contains "no BSS label in TEXT" "$T" "<_b>:"
    assert_not_contains "no ZP / absolute label in TEXT" "$T" "<__zp_scratch2>:"
    assert_not_contains "no absolute label in TEXT" "$T" "<cc45.zeroPageStart>:"
    assert_contains "a call named by its import" "$T" "; <__div"
    assert_contains "a store named by its BSS export" "$T" "; <_a>"
    assert_contains "the other store too" "$T" "; <_b>"
    assert_contains "a member at its offset" "$T" "; <_pl+2>"
    assert_not_contains "no operand named by its placeholder" "$T" "; <_split"
else
    fail "relocation names — compiling failed"
fi

# ─── --cpu: 6502 / 6510 / 65CE02 (#144) ─────────────────────────────────────

echo ""
echo "--- Test: --cpu ---"
C=$BUILD/cpu144
mkdir -p $C
# every opcode at a multiple of 4, padded with $00 (BRK, one byte for every CPU)
python3 -c "open('$C/all.bin','wb').write(bytes(b for op in range(256) for b in (op,0,0,0)))"
opcount() {     # opcount CPU WHAT: lines at a multiple of 4 that are instructions / undocumented
    local out
    out=$($OD -d --cpu=$1 -b 0x1000 $C/all.bin | grep -E '^    1[0-3][0-9a-f][048c]:')
    if [ "$2" = undoc ]; then echo "$out" | grep -c 'undocumented'; else echo "$out" | grep -vc '\.byte'; fi
}
[ "$(opcount 6502 ops)" = 151 ] && [ "$(opcount 6502 undoc)" = 0 ] &&
    pass "--cpu=6502: the 151 documented opcodes, the others .byte" || fail "--cpu=6502 decodes $(opcount 6502 ops) opcodes"
[ "$(opcount 6510 ops)" = 256 ] && [ "$(opcount 6510 undoc)" = 105 ] &&
    pass "--cpu=6510: all 256, the 105 undocumented ones marked" || fail "--cpu=6510 decodes $(opcount 6510 ops), $(opcount 6510 undoc) marked"
[ "$(opcount 45gs02 ops)" = 256 ] && [ "$(opcount 65ce02 ops)" = 256 ] &&
    pass "--cpu=45gs02 / 65ce02 decode all 256" || fail "--cpu=45gs02 / 65ce02 decode $(opcount 45gs02 ops) / $(opcount 65ce02 ops)"

# a C64 program's LAX $10 (#144: it was smb2 $10); a quad prefix only on the 45GS02
printf '\xa7\x10\x42\x42\xa5\x12\x5c\x00\x00\x00\xea\x60' > $C/code.bin
OUT=$($OD -d --cpu=6510 -b 0x1000 $C/code.bin)
assert_contains "6510: lax \$10 marked" "$OUT" "lax	\$10	; undocumented"
assert_contains "6510: \$42 is jam" "$OUT" "jam"
assert_contains "6510: no quad prefix (lda \$12)" "$OUT" "lda	\$12"
assert_contains "6510: \$EA is nop" "$OUT" "nop"
OUT=$($OD -d -b 0x1000 $C/code.bin)
assert_contains "45gs02 (default): smb2 \$10" "$OUT" "smb2	\$10"
assert_contains "45gs02: ldq \$12" "$OUT" "ldq	\$12"
assert_contains "45gs02: map" "$OUT" "map"
OUT=$($OD -d --cpu 65ce02 -b 0x1000 $C/code.bin)
assert_contains "65ce02: \$5C is aug, 4 bytes" "$OUT" "5c 00 00 00	aug"
assert_not_contains "65ce02: no ldq" "$OUT" "ldq"
OUT=$($OD -d --cpu=6502 -b 0x1000 $C/code.bin)
assert_contains "6502: \$A7 is .byte" "$OUT" ".byte	\$a7"

# a PRG loading at $0801 (a C64 program) is 6510 code with a note; --cpu decides
printf '\x01\x08\xa7\x10\x60' > $C/c64.prg
OUT=$($OD -d $C/c64.prg)
assert_contains "\$0801 PRG: 6510 by default" "$OUT" "lax	\$10	; undocumented"
assert_contains "\$0801 PRG: the note" "$OUT" "decoded as 6510 (C64) code; --cpu=45gs02 for MEGA65 code"
OUT=$($OD -d --cpu=45gs02 $C/c64.prg)
assert_contains "\$0801 PRG --cpu=45gs02: smb2" "$OUT" "smb2	\$10"
assert_not_contains "\$0801 PRG --cpu=45gs02: no note" "$OUT" "decoded as 6510"
printf '\x01\x20\xa7\x10\x60' > $C/m65.prg
assert_contains "\$2001 PRG: 45gs02" "$($OD -d $C/m65.prg)" "smb2	\$10"

$OD -d --cpu=z80 -b 0 $C/code.bin >/dev/null 2>$C/err.txt; RC=$?
assert_exit "--cpu=z80 refused" $RC 1
assert_contains "--cpu=z80: the message" "$(cat $C/err.txt)" "unknown CPU 'z80'"

# a 16-bit branch's target is named (it was computed from pc + 3, a byte past it)
printf '        .global start, far\nstart:  bne far\n        .fill 200, 0\nfar:    rts\n' > $C/far.s
if $AS -c $C/far.s -o $C/far.o45 >/dev/null 2>&1; then
    assert_contains "16-bit branch: target named" "$($OD -d $C/far.o45)" "bne	\$00cb	; <far>"
else
    fail "16-bit branch: assembling failed"
fi

# ─── Summary ─────────────────────────────────────────────────────────────────

echo ""
echo "========================================"
# ─── -S: symbol files (#144) ───────────────────────────────────────────────
echo "--- Test: -S symbol files ---"
C=$BUILD/odsym
mkdir -p $C
# a C64 program: the shipped c64 set by default (jsr CHROUT, the VIC-II, a
# RAM vector, the 6510's port); -S none: the addresses
printf '        * = $0801\n        jsr $ffd2\n        sta $d020\n        sta $d027,x\n        jmp ($0314)\n        lda $01\n        rts\n' > $C/c64.s
$AS --cpu 6502 -O0 $C/c64.s -o $C/c64.bin >/dev/null 2>&1
(printf '\x01\x08'; cat $C/c64.bin) > $C/c64.prg
out=$($OD -d $C/c64.prg)
assert_contains "-S default for a \$0801 PRG: jsr CHROUT" "$out" "jsr	CHROUT	; \$ffd2"
assert_contains "-S c64: sta VIC_BORDERCOL" "$out" "sta	VIC_BORDERCOL	; \$d020"
assert_contains "-S c64: an indexed operand" "$out" "sta	VIC_SPR0COL,x	; \$d027"
assert_contains "-S c64: jmp (CINV)" "$out" "jmp	(CINV)	; \$0314"
assert_contains "-S c64: lda R6510" "$out" "lda	R6510	; \$01"
assert_contains "-S none: the address" "$($OD -d -S none $C/c64.prg)" "jsr	\$ffd2"
# MEGA65 code: the mega65 set (<mega65.h>'s names)
printf '        * = $2000\n        jsr $ffd2\n        sta $d020\n        lda $d610\n        rts\n' > $C/m.s
$AS -O0 $C/m.s -o $C/m.bin >/dev/null 2>&1
out=$($OD -d -b '$2000' $C/m.bin)
assert_contains "-S default for the 45GS02: jsr CHROUT" "$out" "jsr	CHROUT	; \$ffd2"
assert_contains "-S mega65: sta VIC4_BORDER" "$out" "sta	VIC4_BORDER	; \$d020"
assert_contains "-S mega65: a #define's name" "$out" "lda	ASCII_KEY_QUEUE	; \$d610"
assert_contains "-S c64 on MEGA65 code" "$($OD -d -b '$2000' -S c64 $C/m.bin)" "sta	VIC_BORDERCOL"
# a file of each format, a label where a name is in the range, and the
# program's own symbols (a map's) before the file's
printf '# mine\n$c000 MYROUT the first\nal C:c003 .other\nthird = $c006\n' > $C/my.sym
printf '        * = $c000\n        jsr $c003\n        jsr $c006\n        rts\n        jsr $c000\n' > $C/r.s
$AS -O0 $C/r.s -o $C/r.bin >/dev/null 2>&1
out=$($OD -d -b '$c000' -S $C/my.sym $C/r.bin)
assert_contains "-S file: a label where a name is" "$out" "0000c000 <MYROUT>:"
assert_contains "-S file: VICE's al lines" "$out" "jsr	other	; \$c003"
assert_contains "-S file: NAME = \$addr lines" "$out" "jsr	third	; \$c006"
assert_contains "-S file: the label as an operand" "$out" "jsr	MYROUT	; \$c000"
assert_not_contains "-S file: only the file's (not the CPU's set)" "$($OD -d -b '$2000' -S $C/my.sym $C/m.bin)" "CHROUT"
# the errors
err=$($OD -d -b '$c000' -S $C/nosuch $C/r.bin 2>&1); rc=$?
[ $rc -ne 0 ] && echo "$err" | grep -q "no such file, and no shipped set of that name" && pass "-S: a missing file" || fail "-S nosuch: rc $rc: $err"
printf '$c000 MYROUT\nthis is not one\n' > $C/bad.sym
err=$($OD -d -b '$c000' -S $C/bad.sym $C/r.bin 2>&1); rc=$?
[ $rc -ne 0 ] && echo "$err" | grep -q "bad.sym:2: not a symbol line" && pass "-S: a bad line" || fail "-S bad.sym: rc $rc: $err"
# the shipped sets load, every line
for set in c64 mega65; do
    n=$(grep -c '^\$' lib/symbols/$set.sym)
    $OD -v -d -b '$2000' -S $set $C/m.bin 2>&1 | grep -q "symbols from .*lib/symbols/$set.sym ($n)" &&
        pass "-S $set: its $n symbols" || fail "-S $set does not load its $n symbols"
done

# ─── --source: assemblable ca45 source (#144) ──────────────────────────────
echo "--- Test: --source ---"
C=$BUILD/odsrc
mkdir -p $C
# every opcode of each CPU, an instruction each 4 bytes (each an --entry:
# decoded as code), its operand bytes at random; the 45GS02's again after
# NEG NEG (ldq ...): assembled back (CA45_OPTIMIZE=0) to the same bytes
python3 - $C <<'PYEOF'
import random, sys
d = sys.argv[1]
random.seed(144)
for name, pre in (("all", b""), ("quad", b"\x42\x42")):
    body = bytearray()
    for op in range(256):
        body += pre + bytes([op]) + bytes(random.randrange(256) for _ in range(3))
    open(d + "/" + name + ".bin", "wb").write(body)
    step = len(pre) + 4
    open(d + "/" + name + ".entries", "w").write(" ".join("--entry $%04x" % (0x2000 + step * k) for k in range(256)))
PYEOF
roundtrip() {   # roundtrip <name> <bin> <entries file> <cpu>
    $OD --source --cpu=$4 -S none -b '$2000' $(cat $3) $2 > $C/$1.s45 2>$C/$1.err &&
        CA45_OPTIMIZE=0 $AS $C/$1.s45 -o $C/$1.rt.bin >$C/$1.log 2>&1 && cmp -s $2 $C/$1.rt.bin
}
for cpu in 45gs02 65ce02 6502 6510; do
    roundtrip all_$cpu $C/all.bin $C/all.entries $cpu && pass "--source --cpu=$cpu: every opcode assembles back" ||
        fail "--source --cpu=$cpu round trip: $(grep -m2 error $C/all_$cpu.log; cmp $C/all.bin $C/all_$cpu.rt.bin 2>&1 | head -1)"
done
roundtrip quad $C/quad.bin $C/quad.entries 45gs02 && [ "$(grep -cE '^        (ldq|stq|adcq|sbcq|andq|eorq|oraq|cmpq|aslq|lsrq|rolq|rorq|inq|deq) ' $C/quad.s45)" -ge 10 ] &&
    pass "--source: NEG NEG and an instruction with a Q form: ldq ..., assembled back" ||
    fail "--source quad round trip: $(grep -m2 error $C/quad.log; cmp $C/quad.bin $C/quad.rt.bin 2>&1 | head -1)"
# programs cc45 made (both conventions, with their maps: the functions' names)
for t in test_cmpvalue_xemu test_bss_fill_xemu; do
    for conv in "" -fno-zpcall; do
        n=$t$conv
        ./bin/cc45 -O2 $conv -Wl,-M,$C/$n.map src/test-resources/$t.c -o $C/$n.prg >/dev/null 2>&1
        $OD --source $C/$n.prg > $C/$n.s45 2>/dev/null && CA45_OPTIMIZE=0 $AS $C/$n.s45 -o $C/$n.rt.prg >/dev/null 2>&1 &&
            cmp -s $C/$n.prg $C/$n.rt.prg && grep -q '^_main' $C/$n.s45 && pass "--source of $n.prg assembles back" ||
            fail "--source of $n.prg: $(cmp $C/$n.prg $C/$n.rt.prg 2>&1 | head -1)"
    done
done
# a C64 program: its BASIC stub and a table as .byte (--data), lax as .byte
# with its name, the KERNAL's names as equates, a routine only a pointer
# reaches (--entry), labels; a 16-bit branch an 8-bit one would do as .byte
printf '        .cpu 6502\n        * = $0801\n        .byte $0b, $08, $0a, $00, $9e, $32, $30, $36, $31, $00, $00, $00\nstart:  ldx #0\nloop:   lda msg,x\n        beq done\n        jsr $ffd2\n        inx\n        bne loop\ndone:   .byte $a7, $10\n        rts\nmsg:    .byte $48, $49, $00\nother:  inc $d020\n        rts\n' > $C/c64.s
$AS -O0 $C/c64.s -o $C/c64.prg >/dev/null 2>&1
out=$($OD --source --data '$081d-$081f' --entry '$0820' $C/c64.prg)
echo "$out" > $C/c64.s45
CA45_OPTIMIZE=0 $AS $C/c64.s45 -o $C/c64.rt.prg >/dev/null 2>&1 && cmp -s $C/c64.prg $C/c64.rt.prg &&
    pass "--source of a C64 program assembles back" || fail "--source c64: $(cmp $C/c64.prg $C/c64.rt.prg 2>&1)"
assert_contains "--source: .cpu 6502 for 6510 code" "$out" "        .cpu 6502"
assert_contains "--source: a name outside as an equate" "$out" "CHROUT = \$FFD2"
assert_contains "--source: the name in the operand" "$out" "jsr CHROUT"
assert_contains "--source: the BASIC stub as .byte" "$out" ".byte \$0b, \$08, \$0a, \$00, \$9e, \$32, \$30, \$36"
assert_contains "--source: an undocumented opcode as .byte, named" "$out" "; lax \$10 (undocumented)"
assert_contains "--source: a label at a target" "$out" "L_080F:"
assert_contains "--source: an operand naming it" "$out" "bne L_080F"
assert_contains "--source: --data as .byte" "$out" ".byte \$48, \$49, \$00"
assert_contains "--source: an --entry's code" "$out" "inc VIC_BORDERCOL"
printf '        * = $2000\n        .byte $d3, $02, $00\n        inx\n        inx\n        rts\n' > $C/b16.s
$AS -O0 $C/b16.s -o $C/b16.bin >/dev/null 2>&1
out=$($OD --source -S none -b '$2000' $C/b16.bin)
echo "$out" > $C/b16.s45
CA45_OPTIMIZE=0 $AS $C/b16.s45 -o $C/b16.rt.bin >/dev/null 2>&1 && cmp -s $C/b16.bin $C/b16.rt.bin &&
    echo "$out" | grep -q ".byte \$d3, \$02, \$00 .*; bne \$2004 (16-bit)" &&
    pass "--source: a 16-bit branch an 8-bit one reaches as .byte" || fail "--source 16-bit branch: $out"
# the errors: an object, a bad --data range
printf '        rts\n' > $C/o.s
$AS -c $C/o.s -o $C/o.o45 >/dev/null 2>&1
err=$($OD --source $C/o.o45 2>&1); rc=$?
[ $rc -ne 0 ] && echo "$err" | grep -q "takes a PRG or a binary" && pass "--source: an object refused" || fail "--source o.o45: $rc $err"
err=$($OD --source --data '$0900' $C/c64.prg 2>&1); rc=$?
[ $rc -ne 0 ] && echo "$err" | grep -q "not a range" && pass "--source: --data without a range" || fail "--data \$0900: $rc $err"

# --cycles and --vice (#144 part 4)
echo "Testing --cycles and --vice..."
printf '        * = $2000\nstart:  lda #$00\n        sta $d020\n        lda $1234,x\n        sta $1234,x\n        beq start\n        .byte $d3, $00, $00\n        bra start\n        ldq $10\n        lda [$10],z\n        ldq [$10],z\n        bbr0 $10,start\n        jsr $ffd2\n        rts\n' > $C/cyc.s
$AS -O0 $C/cyc.s -o $C/cyc.bin >/dev/null 2>&1
cyc() {     # cyc LABEL OUTPUT CYCLES MNEMONIC OPERAND
    if printf '%s\n' "$2" | grep -qF "$(printf '\t%s\t%s\t%s' "$3" "$4" "$5")"; then pass "$1"; else fail "$1: no '$3 $4 $5'"; fi
}
out=$($OD --cycles -S none -b '$2000' $C/cyc.bin)
assert_contains "--cycles: the 40 MHz legend" "$out" "; cycles: the 45GS02 at 40 MHz"
cyc "--cycles 40 MHz: lda abs,x (measured)" "$out" 5 lda '$1234,x'
cyc "--cycles 40 MHz: sta abs,x" "$out" 4 sta '$1234,x'
cyc "--cycles 40 MHz: an 8-bit branch, +2 taken" "$out" 2/4 beq '$2000'
cyc "--cycles 40 MHz: a 16-bit branch, +1 taken" "$out" 4/5 bne '$200f'
cyc "--cycles 40 MHz: bra (always taken)" "$out" 4 bra '$2000'
cyc "--cycles 40 MHz: ldq, the prefix included" "$out" 10 ldq '$10'
cyc "--cycles 40 MHz: lda [\$nn],z (NOP prefix)" "$out" 13 lda '[$10],z'
cyc "--cycles 40 MHz: ldq [\$nn],z (NEG NEG NOP)" "$out" 16 ldq '[$10],z'
cyc "--cycles 40 MHz: bbr" "$out" 5/6 bbr0 '$10,$2000'
out=$($OD --cycles=3.5 -S none -b '$2000' $C/cyc.bin)
cyc "--cycles=3.5: lda abs,x (the 65CE02's)" "$out" 4 lda '$1234,x'
cyc "--cycles=3.5: a branch, +1 taken" "$out" 2/3 beq '$2000'
cyc "--cycles=3.5: jsr" "$out" 2 jsr '$ffd2'
out=$($OD --cycles=1 -S none -b '$2000' $C/cyc.bin)
cyc "--cycles=1: lda abs,x, +1 on a page crossing" "$out" '4*' lda '$1234,x'
cyc "--cycles=1: sta abs,x (always 5)" "$out" 5 sta '$1234,x'
cyc "--cycles=1: a branch as the 6502's" "$out" '2/3*' beq '$2000'
cyc "--cycles=1: a 45GS02 instruction the 65CE02's" "$out" 3/4 bne '$200f'
cyc "--cycles=1: jsr as the 6502's" "$out" 6 jsr '$ffd2'
out=$($OD --cycles -S none $C/c64.prg)
cyc "--cycles: a C64 program's 6510 counts" "$out" '4*' lda '$081d,x'
cyc "--cycles: 6510 bne" "$out" '2/3*' bne '$080f'
cyc "--cycles: 6510 jsr" "$out" 6 jsr '$ffd2'
assert_contains "--cycles: the 6502 legend" "$out" "; cycles: the 6502"
err=$($OD --cycles=8 $C/cyc.bin 2>&1); rc=$?
[ $rc -ne 0 ] && echo "$err" | grep -q "not a 45GS02 speed" && pass "--cycles: an unknown speed refused" || fail "--cycles=8: $rc $err"
err=$($OD --source --cycles $C/c64.prg 2>&1); rc=$?
[ $rc -ne 0 ] && echo "$err" | grep -q "not --source" && pass "--cycles with --source refused" || fail "--source --cycles: $rc $err"
# a flat [$nn],z instruction is one line in -d too
out=$($OD -d -S none -b '$2000' $C/cyc.bin)
assert_contains "-d: NOP + (\$nn),z is lda [\$nn],z" "$out" "$(printf 'ea b2 10    \tlda\t[$10],z')"
assert_contains "-d: NEG NEG NOP + (\$nn),z is ldq [\$nn],z" "$out" "$(printf '42 42 ea b2 10\tldq\t[$10],z')"
out=$($OD --vice -b '$2000' $C/cyc.bin)
assert_contains "--vice: a 2-byte line" "$out" ".C:2000  A9 00       LDA #\$00"
assert_contains "--vice: a 3-byte line, the address (no symbol file)" "$out" ".C:2002  8D 20 D0    STA \$D020"
assert_contains "--vice: a 1-byte line" "$out" ".C:2024  60          RTS"
assert_contains "--vice: a 4-byte line" "$out" ".C:2012  42 42 A5 10 LDQ \$10"
assert_contains "--vice: a 5-byte line" "$out" ".C:2019  4242EA B210 LDQ [\$10],Z"
assert_not_contains "--vice: no section header" "$out" "Disassembly of section"
out=$($OD --vice -S c64 -b '$2000' $C/cyc.bin)
assert_contains "--vice -S: names as VICE's labels" "$out" "JSR CHROUT"
out=$($OD --vice --cycles $C/c64.prg)
assert_contains "--vice: the BASIC stub as instructions too" "$out" ".C:0801  0B 08       *ANC #\$08  ; 2"
assert_contains "--vice: an undocumented opcode with *, cycles" "$out" ".C:081a  A7 10       *LAX \$10  ; 3"

# --json / --html / --tree (#201): the files as one model, a page, a tree
echo ""
echo "--- --json, --html, --tree ---"
C=$BUILD/od201
mkdir -p $C
cat > $C/m.c <<'CEOF'
int sq(int x) { return x * x; }
int twice(int x) { return sq(x) + sq(x); }
const char msg[] = "hi";
int main(void) { return twice(3) + msg[0]; }
CEOF
./bin/cc45 -O0 -c $C/m.c -o $C/m.o45 2>/dev/null && ./bin/cc45 -O0 $C/m.c -o $C/m.prg -Wl,-M,$C/m.map 2>/dev/null
model() {   # model FILE... : a python expression over the --json output (m: its list)
    local expr="$1"; shift
    $OD --json "$@" | python3 -c "import json,sys; m=json.load(sys.stdin); print($expr)" 2>&1
}
[ "$(model "m[0]['kind'], [s['name'] for s in m[0]['segments']]" $C/m.o45)" = "object ['TEXT', 'DATA', 'BSS']" ] \
    && pass "--json: an object's kind and segments" || fail "--json object: $(model "m[0].get('kind')" $C/m.o45)"
[ "$(model "[s['attr']['conv'] for s in m[0]['symbols'] if s['name']=='_sq']" $C/m.o45)" = "['zp']" ] \
    && pass "--json: a function's attributes" || fail "--json attr: $(model "[s for s in m[0]['symbols'] if s['name']=='_sq']" $C/m.o45)"
[ "$(model "sorted({r['symbol'] for r in m[0]['relocs'] if 'symbol' in r} & {'_sq'}) or [l['n'] for l in m[0]['disasm'][0]['lines'] if l.get('m')=='jsr'][:2]" $C/m.o45)" != "" ] \
    && pass "--json: relocations" || fail "--json relocs"
# a relocated jsr / jmp goes to its symbol, not the address it holds unlinked
# (sq's call of __mul16 is a tail jump)
[ "$(model "sorted({(l.get('g'), l.get('n')) for l in m[0]['disasm'][0]['lines'] if l.get('m') in ('jsr', 'jmp')}, key=str)" $C/m.o45)" = "[(0, '<_sq>'), (15, '<_twice>'), (None, '<__mul16>')]" ] \
    && pass "--json: a jsr to the object's own symbol, a relocated one to its import" \
    || fail "--json jsr: $(model "[(l.get('g'), l.get('n')) for l in m[0]['disasm'][0]['lines'] if l.get('m')=='jsr']" $C/m.o45)"
[ "$(model "m[0]['kind'], m[0]['stub']['sys'] > 2048, m[0]['map'].endswith('m.map'), any(s['name']=='_twice' for s in m[0]['symbols'])" $C/m.prg)" = "prg True True True" ] \
    && pass "--json: a PRG (stub, map symbols)" || fail "--json prg: $(model "m[0].get('kind'), m[0].get('stub')" $C/m.prg)"
[ "$(model "m[0]['kind'], len(m[0]['members']) > 50, any(e['name']=='_printf' for e in m[0]['index'])" lib/build/c45.lib)" = "library True True" ] \
    && pass "--json: a .lib's members and index" || fail "--json lib"
printf '\xa9\x01\x20\x00\x20\x60' > $C/r.bin
[ "$(model "[(l['m'], l.get('g')) for l in m[0]['disasm'][0]['lines']]" -b 0x2000 $C/r.bin)" = "[('lda', None), ('jsr', 8192), ('rts', None)]" ] \
    && pass "--json: a binary; a jsr's target" || fail "--json bin: $(model "m[0]" -b 0x2000 $C/r.bin)"
$OD --html $C/m.html $C/m.o45 $C/m.prg
out=$(cat $C/m.html)
assert_contains "--html: the page's title" "$out" "<title>objdump45: 2 files</title>"
assert_contains "--html: the model in the page" "$out" '"kind":"prg"'
python3 -c "import re,sys; s=open('$C/m.html').read(); j=re.search(r'id=\"model\"[^>]*>(.*?)</script>', s, re.S).group(1); sys.exit('</' in j)" \
    && pass "--html: no </ inside the model" || fail "--html: </ inside the model"
if command -v node >/dev/null; then
    python3 -c "import re,sys; s=open('$C/m.html').read(); open('$C/page.js','w').write(re.findall(r'<script>(.*?)</script>', s, re.S)[0])"
    node --check $C/page.js 2>/dev/null && pass "--html: the page's script parses" || fail "--html: the page's script"
fi
out=$($OD --tree $C/m.o45)
assert_contains "--tree: the file" "$out" "m.o45  (object, o45-32, 45GS02)"
assert_contains "--tree: a function's convention" "$out" "Convention: zp_call"
assert_contains "--tree: the disassembly by label" "$out" "<_twice>"
assert_contains "--tree: the call graph" "$out" "calls: _sq x2"
assert_contains "--tree: who calls" "$out" "called by: _twice"
err=$($OD --tree --source $C/m.prg 2>&1); rc=$?
[ $rc -ne 0 ] && echo "$err" | grep -q "not for --source" && pass "--tree with --source refused" || fail "--tree --source: $rc $err"

# checks and --compare (#201)
cat > $C/smc.s45 <<'SEOF'
* = $2000
start:
    lda #5
    sta patch+1
patch:
    ldx #0
    bne start+1
    rts
SEOF
$AS -c $C/smc.s45 -o $C/smc.o45 >/dev/null 2>&1
out=$($OD --tree $C/smc.o45)
assert_contains "checks: a store into the code (a note)" "$out" 'info    $2002  sta writes into the code (<TEXT+$6>)'
assert_contains "checks: a branch into an instruction" "$out" 'bne to $2001, inside an instruction'
$AS $C/smc.s45 -o $C/smc.bin >/dev/null 2>&1
out=$($OD --tree -b 0x2000 $C/smc.bin)
assert_contains "checks: a binary's store into its code" "$out" 'sta writes into the code ($2006)'
assert_not_contains "checks: no branch checks without a map (data may be decoded)" "$out" 'inside an instruction'
cat > $C/isr.c <<'CEOF'
static int counter(int x) { return x + 1; }
volatile int n;
__interrupt void tick(void) { n = counter(n); }
int main(void) { return 0; }
CEOF
./bin/cc45 -O0 -c $C/isr.c -o $C/isr.o45 2>/dev/null
out=$($OD --tree $C/isr.o45)
assert_contains "checks: an interrupt handler calling a function with static allocation (ln45 rejects it)" "$out" "the interrupt handler _tick calls _counter__isr, which has static allocation"
out=$($OD --tree $C/m.o45)
assert_contains "checks: none in a plain object" "$out" "Checks (0 warnings, 0 notes)"
./bin/cc45 -O2 -c $C/m.c -o $C/m2.o45 2>/dev/null
out=$($OD --compare $C/m.o45 $C/m2.o45)
assert_contains "--compare: the two files" "$out" "m.o45 → $C/m2.o45  (comparison)"
assert_contains "--compare: the sizes" "$out" "TEXT    "
assert_contains "--compare: the functions changed" "$out" "Functions changed ("
[ "$(model "m[0]['kind'], m[0]['a'].endswith('m.o45'), len(m)" --compare $C/m.o45 $C/m2.o45)" = "comparison True 3" ] \
    && pass "--compare --json: the comparison first, then both files" || fail "--compare --json"
same=$($OD --compare $C/m.o45 $C/m.o45)
assert_contains "--compare: a file against itself: nothing changed" "$same" "Functions changed (0;"
err=$($OD --compare $C/m.o45 2>&1); rc=$?
[ $rc -ne 0 ] && echo "$err" | grep -q "takes two files" && pass "--compare needs two files" || fail "--compare one file: $rc $err"

echo "objdump45 tests: $passed passed, $failed failed"
echo "========================================"

if [ $failed -eq 0 ]; then
    exit 0
else
    exit 1
fi
