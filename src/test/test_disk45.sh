#!/bin/bash
# test_disk45.s45h — disk45 feature tests
# Tests: create, list, add, extract, remove, rename, copy, extract-all,
#        wildcards, T64, D80, REL files, subdirectories, block ops, chain,
#        -a2p/-p2a filters, validate, lock/unlock

DISK45=bin/disk45
TMPDIR=$(mktemp -d)
passed=0
failed=0

pass() { echo "  PASS: $1"; passed=$((passed + 1)); }
fail() { echo "  FAIL: $1"; failed=$((failed + 1)); }

cleanup() { rm -rf "$TMPDIR"; }
trap cleanup EXIT

# ============================================================================
echo "=== D64 basic operations ==="

# Create
$DISK45 create "$TMPDIR/test.d64" -n "TEST DISK" -i "TD" >/dev/null 2>&1
[ -f "$TMPDIR/test.d64" ] && pass "D64 create" || fail "D64 create"

# Add file
printf '\x01\x08HELLO' > "$TMPDIR/hello.prg"
$DISK45 add "$TMPDIR/test.d64" "$TMPDIR/hello.prg" HELLO >/dev/null 2>&1
$DISK45 list "$TMPDIR/test.d64" | grep -q 'HELLO' && pass "D64 add+list" || fail "D64 add+list"

# Extract
$DISK45 extract "$TMPDIR/test.d64" HELLO "$TMPDIR/out.prg" >/dev/null 2>&1
cmp -s "$TMPDIR/hello.prg" "$TMPDIR/out.prg" && pass "D64 extract" || fail "D64 extract"

# Case-insensitive lookup
$DISK45 extract "$TMPDIR/test.d64" hello "$TMPDIR/out2.prg" >/dev/null 2>&1
cmp -s "$TMPDIR/hello.prg" "$TMPDIR/out2.prg" && pass "D64 case-insensitive" || fail "D64 case-insensitive"

# Rename
$DISK45 rename "$TMPDIR/test.d64" HELLO WORLD >/dev/null 2>&1
$DISK45 list "$TMPDIR/test.d64" | grep -q 'WORLD' && pass "D64 rename" || fail "D64 rename"

# Remove
$DISK45 remove "$TMPDIR/test.d64" WORLD >/dev/null 2>&1
$DISK45 list "$TMPDIR/test.d64" | grep -q 'WORLD' && fail "D64 remove" || pass "D64 remove"

# Info
$DISK45 info "$TMPDIR/test.d64" | grep -q 'TEST DISK' && pass "D64 info" || fail "D64 info"

# Validate
$DISK45 validate "$TMPDIR/test.d64" | grep -q 'OK\|ok\|valid' && pass "D64 validate" || fail "D64 validate"

# ============================================================================
echo "=== D81 operations ==="

$DISK45 create "$TMPDIR/test.d81" -n "D81 TEST" >/dev/null 2>&1
printf '\x01\x08DATA1' > "$TMPDIR/d1.prg"
printf '\x01\x08DATA2' > "$TMPDIR/d2.prg"
printf '\x01\x08DATA3' > "$TMPDIR/d3.prg"
$DISK45 add "$TMPDIR/test.d81" "$TMPDIR/d1.prg" FILE1 >/dev/null 2>&1
$DISK45 add "$TMPDIR/test.d81" "$TMPDIR/d2.prg" FILE2 >/dev/null 2>&1
$DISK45 add "$TMPDIR/test.d81" "$TMPDIR/d3.prg" FILE3 >/dev/null 2>&1
COUNT=$($DISK45 list "$TMPDIR/test.d81" | grep -c 'FILE')
[ "$COUNT" -eq 3 ] && pass "D81 add 3 files" || fail "D81 add 3 files (got $COUNT)"

# ============================================================================
echo "=== Wildcards ==="

WCOUNT=$($DISK45 list "$TMPDIR/test.d81" "FILE*" | grep -c 'FILE')
[ "$WCOUNT" -eq 3 ] && pass "Wildcard FILE*" || fail "Wildcard FILE* (got $WCOUNT)"

W1=$($DISK45 list "$TMPDIR/test.d81" "FILE1" | grep -c 'FILE1')
[ "$W1" -eq 1 ] && pass "Wildcard exact FILE1" || fail "Wildcard exact FILE1"

W2=$($DISK45 list "$TMPDIR/test.d81" "FILE?" | grep -c 'FILE')
[ "$W2" -eq 3 ] && pass "Wildcard FILE?" || fail "Wildcard FILE? (got $W2)"

# ============================================================================
echo "=== Copy between images ==="

$DISK45 create "$TMPDIR/dst.d81" -n "DEST" >/dev/null 2>&1
$DISK45 copy "$TMPDIR/test.d81" "*" "$TMPDIR/dst.d81" >/dev/null 2>&1
DCOUNT=$($DISK45 list "$TMPDIR/dst.d81" | grep -c 'FILE')
[ "$DCOUNT" -eq 3 ] && pass "Copy all files" || fail "Copy all files (got $DCOUNT)"

# ============================================================================
echo "=== Extract-all ==="

mkdir -p "$TMPDIR/extracted"
$DISK45 extract-all "$TMPDIR/test.d81" "$TMPDIR/extracted" >/dev/null 2>&1
ECOUNT=$(ls "$TMPDIR/extracted/" | wc -l)
[ "$ECOUNT" -eq 3 ] && pass "Extract-all" || fail "Extract-all (got $ECOUNT files)"

# ============================================================================
echo "=== T64 tape container ==="

$DISK45 create "$TMPDIR/test.t64" -n "TAPE" >/dev/null 2>&1
$DISK45 add "$TMPDIR/test.t64" "$TMPDIR/d1.prg" TFILE >/dev/null 2>&1
$DISK45 list "$TMPDIR/test.t64" | grep -q 'TFILE' && pass "T64 add+list" || fail "T64 add+list"
$DISK45 extract "$TMPDIR/test.t64" TFILE "$TMPDIR/t64out.prg" >/dev/null 2>&1
[ -f "$TMPDIR/t64out.prg" ] && pass "T64 extract" || fail "T64 extract"

# ============================================================================
echo "=== D80 PET format ==="

$DISK45 create "$TMPDIR/test.d80" -n "PET" >/dev/null 2>&1
$DISK45 add "$TMPDIR/test.d80" "$TMPDIR/d1.prg" PETFILE >/dev/null 2>&1
$DISK45 extract "$TMPDIR/test.d80" PETFILE "$TMPDIR/d80out.prg" >/dev/null 2>&1
cmp -s "$TMPDIR/d1.prg" "$TMPDIR/d80out.prg" && pass "D80 add+extract" || fail "D80 add+extract"

# ============================================================================
echo "=== Block operations ==="

$DISK45 create "$TMPDIR/blk.d64" -n "BLOCK" >/dev/null 2>&1
$DISK45 bfill "$TMPDIR/blk.d64" 1 0 0xAA >/dev/null 2>&1
PEEK=$($DISK45 bpeek "$TMPDIR/blk.d64" 1 0 0 4)
[ "$PEEK" = "AA AA AA AA" ] && pass "bfill+bpeek" || fail "bfill+bpeek (got '$PEEK')"

$DISK45 bpoke "$TMPDIR/blk.d64" 1 0 0 0x42 0x43 >/dev/null 2>&1
PEEK2=$($DISK45 bpeek "$TMPDIR/blk.d64" 1 0 0 2)
[ "$PEEK2" = "42 43" ] && pass "bpoke+bpeek" || fail "bpoke+bpeek (got '$PEEK2')"

# ============================================================================
echo "=== Chain trace ==="

$DISK45 create "$TMPDIR/chain.d64" -n "CHAIN" >/dev/null 2>&1
$DISK45 add "$TMPDIR/chain.d64" "$TMPDIR/d1.prg" CHAINFILE >/dev/null 2>&1
$DISK45 chain "$TMPDIR/chain.d64" CHAINFILE 2>&1 | grep -q 'sector(s)' && pass "Chain trace" || fail "Chain trace"

# ============================================================================
echo "=== REL files ==="

$DISK45 create "$TMPDIR/rel.d64" -n "REL" >/dev/null 2>&1
$DISK45 rel-create "$TMPDIR/rel.d64" MYREL 10 5 >/dev/null 2>&1
$DISK45 list "$TMPDIR/rel.d64" | grep -q 'MYREL.*REL' && pass "REL create+list" || fail "REL create+list"

$DISK45 rel-write "$TMPDIR/rel.d64" MYREL 1 0x41 0x42 0x43 >/dev/null 2>&1
RELREAD=$($DISK45 rel-read "$TMPDIR/rel.d64" MYREL 1 2>&1)
echo "$RELREAD" | grep -q '41 42 43' && pass "REL write+read" || fail "REL write+read"

RELLIST=$($DISK45 rel-list "$TMPDIR/rel.d64" MYREL 2>&1)
echo "$RELLIST" | grep -q '#  1' && pass "REL list" || fail "REL list"

# ============================================================================
echo "=== Subdirectories ==="

$DISK45 create "$TMPDIR/sub.d81" -n "SUBDIR" >/dev/null 2>&1
$DISK45 mkdir "$TMPDIR/sub.d81" GAMES >/dev/null 2>&1
$DISK45 list "$TMPDIR/sub.d81" | grep -q 'GAMES.*CBM' && pass "mkdir+list CBM" || fail "mkdir+list CBM"

$DISK45 mkdir "$TMPDIR/sub.d81" GAMES/ARCADE >/dev/null 2>&1
TREE=$($DISK45 tree "$TMPDIR/sub.d81" 2>&1)
echo "$TREE" | grep -q 'ARCADE' && pass "Nested mkdir+tree" || fail "Nested mkdir+tree"

$DISK45 list "$TMPDIR/sub.d81" "GAMES/" 2>&1 | grep -q 'ARCADE' && pass "List subdir" || fail "List subdir"

$DISK45 rmdir "$TMPDIR/sub.d81" GAMES/ARCADE >/dev/null 2>&1
TREE2=$($DISK45 tree "$TMPDIR/sub.d81" 2>&1)
echo "$TREE2" | grep -q 'ARCADE' && fail "rmdir" || pass "rmdir"

# dir-ensure
$DISK45 dir-ensure "$TMPDIR/sub.d81" 20 GAMES 2>&1 | grep -q 'Added' && pass "dir-ensure" || fail "dir-ensure"

# dir-shrink
$DISK45 dir-shrink "$TMPDIR/sub.d81" GAMES 2>&1 | grep -q 'Reclaimed\|No empty' && pass "dir-shrink" || fail "dir-shrink"

# ============================================================================
echo "=== PETSCII filters ==="

A2P=$(echo "Hello" | $DISK45 -a2p | xxd -p | tr -d '\n')
# H=0xC8 (shifted), e=0x45, l=0x4C, l=0x4C, o=0x4F, \n=0x0D
echo "$A2P" | grep -qi 'c8454c4c4f0d' && pass "-a2p filter" || fail "-a2p filter (got $A2P)"

P2A=$(printf '\xC8\xC5\xCC\xCC\xCF\x0D' | $DISK45 -p2a)
[ "$P2A" = "HELLO" ] && pass "-p2a filter" || fail "-p2a filter (got '$P2A')"

# ============================================================================
echo "=== GZ compression ==="

$DISK45 create "$TMPDIR/test.d64.gz" -n "GZIPPED" >/dev/null 2>&1
$DISK45 add "$TMPDIR/test.d64.gz" "$TMPDIR/d1.prg" GZFILE >/dev/null 2>&1
$DISK45 list "$TMPDIR/test.d64.gz" | grep -q 'GZFILE' && pass "GZ create+add+list" || fail "GZ create+add+list"

# ============================================================================
echo ""
echo "disk45 tests: $passed passed, $failed failed"
[ "$failed" -eq 0 ] || exit 1
