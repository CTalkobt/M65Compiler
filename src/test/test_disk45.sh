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

# REL on a D71 (directory on track 18) and a D81 (super side sector): the
# records DOS lays out (the last data sector filled), validate clean
for ext in d71 d81; do
    $DISK45 create "$TMPDIR/rel.$ext" -n "REL" >/dev/null 2>&1
    $DISK45 rel-create "$TMPDIR/rel.$ext" SCORES 20 30 2>&1 | grep -q '38 records' &&
    $DISK45 rel-write "$TMPDIR/rel.$ext" SCORES 30 '"LAST"' >/dev/null 2>&1 &&
    $DISK45 rel-read "$TMPDIR/rel.$ext" SCORES 30 2>&1 | grep -q 'Text: "LAST' &&
    $DISK45 validate "$TMPDIR/rel.$ext" 2>&1 | grep -q 'Status: OK' &&
    ! $DISK45 validate "$TMPDIR/rel.$ext" 2>&1 | grep -q 'Cross' &&
        pass "REL on $ext" || fail "REL on $ext"
done

# 8250 (D82), D9090/D9060 and CMD native REL files start with a super side
# sector ($FE at byte 2, as the 1581's; VICE), the 8050's (D80) do not
for spec in d80:39:1:no d82:39:1:yes d90:76:10:yes dnp:1:34:yes; do
    IFS=: read ext dt ds super <<< "$spec"
    rm -f "$TMPDIR/rs.$ext"
    $DISK45 create "$TMPDIR/rs.$ext" >/dev/null 2>&1
    $DISK45 rel-create "$TMPDIR/rs.$ext" SCORES 100 1000 >/dev/null 2>&1
    $DISK45 rel-write "$TMPDIR/rs.$ext" SCORES 900 '"HELLO"' >/dev/null 2>&1
    read -r st ss <<< "$($DISK45 bpeek "$TMPDIR/rs.$ext" $dt $ds 21 2)"
    MARK=$($DISK45 bpeek "$TMPDIR/rs.$ext" $((16#$st)) $((16#$ss)) 2 1)
    WANT=00; [ $super = yes ] && WANT=FE
    [ "$MARK" = $WANT ] &&
        $DISK45 rel-read "$TMPDIR/rs.$ext" SCORES 900 2>&1 | grep -q 'Text: "HELLO' &&
        $DISK45 validate "$TMPDIR/rs.$ext" 2>&1 | grep -q 'Status: OK' &&
        pass "REL on $ext (super side sector: $super)" || fail "REL on $ext (byte 2 of the first side sector: $MARK)"
done

# repair: the BAM rebuilt from the file chains. A D64 with a file sector
# marked free and an unused sector (1/0) marked used; a fresh image of every
# writable format needs no change
rm -f "$TMPDIR/rep.d64"
$DISK45 create "$TMPDIR/rep.d64" >/dev/null 2>&1
$DISK45 add "$TMPDIR/rep.d64" "$TMPDIR/d1.prg" A >/dev/null 2>&1
FIRST=$($DISK45 chain "$TMPDIR/rep.d64" A | sed -n 2p | grep -oE '[0-9]+/[0-9]+' | head -1)
python3 - "$TMPDIR/rep.d64" "$FIRST" <<'PY'
import sys
p = sys.argv[1]; t, s = map(int, sys.argv[2].split('/'))
d = bytearray(open(p, 'rb').read())
bam = sum(21 for _ in range(17)) * 256      # 18/0
def entry(tr): return bam + 4 + 4 * (tr - 1)
e = entry(t); d[e + 1 + s // 8] |= 1 << (s % 8); d[e] += 1     # file sector "free"
e = entry(1); d[e + 1] &= 0xFE; d[e] -= 1                       # 1/0 "used"
open(p, 'wb').write(d)
PY
DRY=$($DISK45 repair "$TMPDIR/rep.d64" --dry 2>&1)
SUM=$(md5sum < "$TMPDIR/rep.d64")
grep -q "Would mark used: 1 sector ($FIRST)" <<< "$DRY" && grep -q 'Would free: 1 sector (1/0)' <<< "$DRY" &&
    [ "$(md5sum < "$TMPDIR/rep.d64")" = "$SUM" ] &&
    $DISK45 repair "$TMPDIR/rep.d64" >/dev/null 2>&1 &&
    $DISK45 validate "$TMPDIR/rep.d64" 2>&1 | grep -q 'Status: OK' &&
    ! $DISK45 validate "$TMPDIR/rep.d64" 2>&1 | grep -q 'Orphaned' &&
    $DISK45 repair "$TMPDIR/rep.d64" 2>&1 | grep -q 'The BAM matches' &&
    pass "repair: --dry reports, repair rebuilds the BAM" || fail "repair ($DRY)"
bad=""
for ext in d64 d71 d81 d65 d80 d82 d90 d60 d2m dnp g64 g71 x64; do
    rm -f "$TMPDIR/rf.$ext"
    $DISK45 create "$TMPDIR/rf.$ext" >/dev/null 2>&1
    $DISK45 repair "$TMPDIR/rf.$ext" --dry 2>&1 | grep -q 'Would mark used: 0 sectors' &&
        $DISK45 repair "$TMPDIR/rf.$ext" --dry 2>&1 | grep -q 'Would free: 0 sectors' || bad="$bad $ext"
done
[ -z "$bad" ] && pass "repair: fresh images of every format match their BAM" || fail "repair on fresh images:$bad"

# diff: the files of two images (subdirectories too) by name, type, size and
# CRC-32; --sectors byte by byte. Exit status 0 when the same, 1 otherwise
rm -f "$TMPDIR/da.d81" "$TMPDIR/db.d81"
$DISK45 create "$TMPDIR/da.d81" >/dev/null 2>&1
$DISK45 add "$TMPDIR/da.d81" "$TMPDIR/d1.prg" A >/dev/null 2>&1
$DISK45 add "$TMPDIR/da.d81" "$TMPDIR/d1.prg" B >/dev/null 2>&1
$DISK45 mkdir "$TMPDIR/da.d81" SUB >/dev/null 2>&1
cp "$TMPDIR/da.d81" "$TMPDIR/db.d81"
SAME=$($DISK45 diff "$TMPDIR/da.d81" "$TMPDIR/db.d81"); SAMERC=$?
SSAME=$($DISK45 diff "$TMPDIR/da.d81" "$TMPDIR/db.d81" --sectors)
printf 'changed' > "$TMPDIR/chg.prg"
$DISK45 rm "$TMPDIR/db.d81" A >/dev/null 2>&1
$DISK45 rm "$TMPDIR/db.d81" B >/dev/null 2>&1
$DISK45 add "$TMPDIR/db.d81" "$TMPDIR/chg.prg" B >/dev/null 2>&1
$DISK45 add "$TMPDIR/db.d81" "$TMPDIR/d1.prg" SUB/Y >/dev/null 2>&1
DIFF=$($DISK45 diff "$TMPDIR/da.d81" "$TMPDIR/db.d81"); DIFFRC=$?
[ $SAMERC = 0 ] && grep -q 'The files are the same' <<< "$SAME" && grep -q 'The sectors are the same' <<< "$SSAME" &&
    [ $DIFFRC = 1 ] && grep -q '^- A (PRG' <<< "$DIFF" && grep -q '^~ B: .*crc ' <<< "$DIFF" &&
    grep -q '^+ SUB/Y (PRG' <<< "$DIFF" && grep -q '3 difference' <<< "$DIFF" &&
    $DISK45 diff "$TMPDIR/da.d81" "$TMPDIR/db.d81" --sectors | grep -q 'sector(s) differ' &&
    pass "diff: files (subdirectories too) and sectors" || fail "diff ($DIFF)"

# list --petscii: names in the C64's characters (Unicode); a name with
# graphics ($60 $61 $5C $5E $A6 $FF) patched into the directory
rm -f "$TMPDIR/pt.d64"
$DISK45 create "$TMPDIR/pt.d64" -n GAMES -i 2A >/dev/null 2>&1
$DISK45 add "$TMPDIR/pt.d64" "$TMPDIR/d1.prg" HELLO >/dev/null 2>&1
python3 - "$TMPDIR/pt.d64" <<'PY'
import sys
p = sys.argv[1]; d = bytearray(open(p, 'rb').read())
off = (17 * 21 + 1) * 256        # 18/1
d[off + 5:off + 21] = bytes([0x41, 0x42, 0x60, 0x61, 0x5C, 0x5E, 0xA6, 0xFF]) + bytes([0xA0] * 8)
open(p, 'wb').write(d)
PY
PT=$($DISK45 list "$TMPDIR/pt.d64" --petscii)
grep -q '^0 "GAMES           " 2A$' <<< "$PT" && grep -q '"AB─♠£↑▒π" *PRG' <<< "$PT" &&
    grep -q 'BLOCKS FREE' <<< "$PT" && pass "list --petscii" || fail "list --petscii ($PT)"

# basic: a BASIC program as LIST shows it; V2 ($0801), V7 ($1C01: DO, $FE
# SLEEP, LOOP UNTIL), BASIC 65 (basic45's output, on the host)
python3 - "$TMPDIR" <<'PY'
import sys
d = sys.argv[1]
def prg(load, lines):
    out = bytearray([load & 0xff, load >> 8]); addr = load
    for num, body in lines:
        nxt = addr + 5 + len(body)
        out += bytes([nxt & 0xff, nxt >> 8, num & 0xff, num >> 8]) + bytes(body) + b'\0'; addr = nxt
    return out + b'\0\0'
open(d + '/bv2.prg', 'wb').write(prg(0x0801, [(10, [0x99, 0x20, 0x22] + list(b'HELLO WORLD') + [0x22]),
                                              (20, [0x89, 0x20] + list(b'10'))]))
open(d + '/bv7.prg', 'wb').write(prg(0x1C01, [(10, [0xEB]), (20, [0xFE, 0x0B, 0x20, 0x31]),
                                              (30, [0xEC, 0x20, 0xFC, 0x20, 0x58, 0xB2, 0x31])]))
PY
rm -f "$TMPDIR/bas.d64"
$DISK45 create "$TMPDIR/bas.d64" >/dev/null 2>&1
$DISK45 add "$TMPDIR/bas.d64" "$TMPDIR/bv2.prg" HELLO >/dev/null 2>&1
$DISK45 add "$TMPDIR/bas.d64" "$TMPDIR/bv7.prg" LOOPS >/dev/null 2>&1
printf 'print "hi"\nsleep 1\n' > "$TMPDIR/b65.bas"
./bin/basic45 "$TMPDIR/b65.bas" -o "$TMPDIR/b65.prg" >/dev/null 2>&1
[ "$($DISK45 basic "$TMPDIR/bas.d64" HELLO)" = "$(printf '10 PRINT "HELLO WORLD"\n20 GOTO 10')" ] &&
    [ "$($DISK45 basic "$TMPDIR/bas.d64" LOOPS)" = "$(printf '10 DO\n20 SLEEP 1\n30 LOOP UNTIL X=1')" ] &&
    [ "$($DISK45 basic "$TMPDIR/b65.prg")" = "$(printf '10 PRINT"HI"\n20 SLEEP 1')" ] &&
    pass "basic: BASIC V2, V7 and BASIC 65 listings" ||
    fail "basic ($($DISK45 basic "$TMPDIR/bas.d64" LOOPS 2>&1 | head -3))"

# add -s/-b/-e/--track: interleave and placement of a file's sectors
head -c 2000 /dev/urandom > "$TMPDIR/pl.bin"
plchain() {   # plchain <add options...>: the first 4 sectors of the file
    rm -f "$TMPDIR/pl.d64"
    $DISK45 create "$TMPDIR/pl.d64" >/dev/null 2>&1
    $DISK45 add "$TMPDIR/pl.d64" "$TMPDIR/d1.prg" F1 >/dev/null 2>&1
    $DISK45 add "$TMPDIR/pl.d64" "$TMPDIR/pl.bin" BIG "$@" >/dev/null 2>&1
    $DISK45 extract "$TMPDIR/pl.d64" BIG "$TMPDIR/pl.out" >/dev/null 2>&1
    cmp -s "$TMPDIR/pl.out" "$TMPDIR/pl.bin" && $DISK45 validate "$TMPDIR/pl.d64" | grep -q 'Status: OK' &&
        $DISK45 chain "$TMPDIR/pl.d64" BIG | sed -n 2p | grep -oE '[0-9]+/[0-9]+' | head -4 | tr '\n' ' '
}
[ "$(plchain -s 10)" = "19/1 19/11 19/2 19/12 " ] && [ "$(plchain -s 4 -b 5)" = "19/5 19/9 19/13 19/17 " ] &&
    [ "$(plchain -e)" = "17/0 17/1 17/2 17/3 " ] && [ "$(plchain --track 3 -s 2)" = "3/0 3/2 3/4 3/6 " ] &&
    ! $DISK45 add "$TMPDIR/pl.d64" "$TMPDIR/pl.bin" X --track 99 >/dev/null 2>&1 &&
    pass "add -s/-b/-e/--track: interleave and placement" ||
    fail "add placement ($(plchain -s 10)/$(plchain -s 4 -b 5)/$(plchain -e)/$(plchain --track 3 -s 2))"

# shell: commands from stdin; an attached image is left out of them, !
# runs a host command, history lists them
rm -f "$TMPDIR/sh.d81"
SH=$(cd "$TMPDIR" && printf 'create sh.d81\nattach sh.d81\nadd d1.prg ONE\nmkdir SUB\nadd d1.prg SUB/TWO\nlist SUB/\nextract ONE sh1.prg\n!cmp -s sh1.prg d1.prg && echo SAME\nhistory\nquit\nlist\n' | "$OLDPWD/$DISK45" shell 2>&1)
grep -q '"TWO".*PRG' <<< "$SH" && grep -q '^SAME$' <<< "$SH" && grep -q '  2  attach sh.d81' <<< "$SH" &&
    [ "$(grep -c 'blocks free' <<< "$SH")" = 1 ] &&
    pass "shell: attach, commands, !, history, quit" || fail "shell ($SH)"

# edit: the sector editor's keys piped in (right, right, AB cd, down, next
# sector, ff, write, quit); without w nothing is written
rm -f "$TMPDIR/ed.d64"
$DISK45 create "$TMPDIR/ed.d64" >/dev/null 2>&1
printf '\033[C\033[CABcd\033[Bnffwq' | $DISK45 edit "$TMPDIR/ed.d64" 18 1 >/dev/null 2>&1
SUM=$(md5sum < "$TMPDIR/ed.d64")
printf '00q' | $DISK45 edit "$TMPDIR/ed.d64" 18 1 >/dev/null 2>&1
[ "$($DISK45 bpeek "$TMPDIR/ed.d64" 18 1 2 2)" = "AB CD" ] && [ "$($DISK45 bpeek "$TMPDIR/ed.d64" 18 2 20 1)" = "FF" ] &&
    [ "$(md5sum < "$TMPDIR/ed.d64")" = "$SUM" ] && ! $DISK45 edit "$TMPDIR/ed.d64" 99 0 </dev/null >/dev/null 2>&1 &&
    pass "edit: the sector editor's keys" || fail "edit"

# ============================================================================
echo "=== Directory slots ==="

# A file added into the first slot of a directory sector (after the file
# there was removed) must not cut the directory chain; D80/D82 remove
for ext in d64 d71 d81 d80 d82 d65; do
    $DISK45 create "$TMPDIR/slots.$ext" -n "SLOTS" >/dev/null 2>&1
    for i in 1 2 3 4 5 6 7 8 9 10 11 12; do $DISK45 add "$TMPDIR/slots.$ext" "$TMPDIR/d1.prg" F$i >/dev/null 2>&1; done
    $DISK45 remove "$TMPDIR/slots.$ext" F1 >/dev/null 2>&1
    $DISK45 add "$TMPDIR/slots.$ext" "$TMPDIR/d1.prg" NEW >/dev/null 2>&1
    N=$($DISK45 list "$TMPDIR/slots.$ext" 2>&1 | grep -c PRG)
    $DISK45 extract "$TMPDIR/slots.$ext" F12 "$TMPDIR/f12.prg" >/dev/null 2>&1
    [ "$N" = 12 ] && cmp -s "$TMPDIR/f12.prg" "$TMPDIR/d1.prg" &&
        $DISK45 validate "$TMPDIR/slots.$ext" 2>&1 | grep -q 'Status: OK' &&
        ! $DISK45 validate "$TMPDIR/slots.$ext" 2>&1 | grep -q 'Cross' &&
        pass "Directory slot 0 reuse ($ext)" || fail "Directory slot 0 reuse ($ext): $N files"
done

# ============================================================================
echo "=== D80/D82 layout ==="

# As https://ist.uwaterloo.ca/~schepers/formats/D80-D82.TXT gives it: the
# header links to the BAM 38/0, the BAM sectors chain to the directory 39/1,
# BLOCKS FREE excludes track 39, a D82 uses its second side
$DISK45 create "$TMPDIR/l.d80" -n SAMPLE >/dev/null 2>&1
H=$(dd if="$TMPDIR/l.d80" bs=256 skip=$((38*29)) count=1 2>/dev/null | xxd -p | head -c 6)
B=$(dd if="$TMPDIR/l.d80" bs=256 skip=$((37*29)) count=1 2>/dev/null | xxd -p | head -c 22)
B3=$(dd if="$TMPDIR/l.d80" bs=256 skip=$((37*29+3)) count=1 2>/dev/null | xxd -p | head -c 12)
[ "$H" = "260043" ] && [ "$B" = "2603430001331dffffff1f" ] && [ "$B3" = "27014300334e" ] &&
    $DISK45 list "$TMPDIR/l.d80" | grep -q '^2052 blocks free' &&
    pass "D80 header and BAM" || fail "D80 header and BAM ($H $B $B3)"
$DISK45 create "$TMPDIR/l.d82" -n SAMPLE >/dev/null 2>&1
head -c 61440 /dev/urandom > "$TMPDIR/big.prg"
for i in $(seq 1 16); do $DISK45 add "$TMPDIR/l.d82" "$TMPDIR/big.prg" BIG$i >/dev/null 2>&1; done
$DISK45 extract "$TMPDIR/l.d82" BIG16 "$TMPDIR/big16.prg" >/dev/null 2>&1
$DISK45 list "$TMPDIR/l.d82" | grep -q '^261 blocks free' && cmp -s "$TMPDIR/big16.prg" "$TMPDIR/big.prg" &&
    $DISK45 validate "$TMPDIR/l.d82" 2>&1 | grep -q 'Status: OK' &&
    pass "D82 second side" || fail "D82 second side"

# ============================================================================
echo "=== D9090/D9060, read-only formats ==="

# D9090/D9060 (VICE's layout): tracks 0-152 of 192/128 sectors; 0/0 the
# configuration block (directory 76/10, header 76/20, BAM 1/0), the BAM a
# chain of sectors with 5 bytes per track and head
for spec in d90:7520256:29162:8 d60:5013504:19441:12; do
    IFS=: read ext size free per <<< "$spec"
    rm -f "$TMPDIR/hd.$ext"
    $DISK45 create "$TMPDIR/hd.$ext" -n "HARD DISK" -i HD >/dev/null 2>&1
    SPT=$(( (size / 153) / 256 ))
    CFG=$(dd if="$TMPDIR/hd.$ext" bs=1 count=12 2>/dev/null | xxd -p)
    HDR=$(dd if="$TMPDIR/hd.$ext" bs=1 skip=$(( (76 * SPT + 20) * 256 )) count=29 2>/dev/null | xxd -p | tr -d '\n')
    BAM=$(dd if="$TMPDIR/hd.$ext" bs=1 skip=$(( SPT * 256 )) count=21 2>/dev/null | xxd -p | tr -d '\n')
    dd if=/dev/urandom of="$TMPDIR/hdbig.bin" bs=1k count=300 2>/dev/null
    [ "$(stat -c %s "$TMPDIR/hd.$ext")" = "$size" ] &&
        [ "$CFG" = "000100ff4c0a4c1401004844" ] &&
        [ "$HDR" = "4c0a0000000048415244204449534ba0a0a0a0a0a0a0a0a04844a03341" ] &&
        [ "${BAM:0:12}" = "$(printf '%02x00ffff00%02x' $((1 + per)) $per)" ] &&
        $DISK45 list "$TMPDIR/hd.$ext" | grep -q "^$free blocks free" &&
        $DISK45 add "$TMPDIR/hd.$ext" "$TMPDIR/hdbig.bin" BIG >/dev/null 2>&1 &&
        $DISK45 mkdir "$TMPDIR/hd.$ext" SUB >/dev/null 2>&1 &&
        $DISK45 add "$TMPDIR/hd.$ext" "$TMPDIR/d1.prg" SUB/X >/dev/null 2>&1 &&
        $DISK45 extract "$TMPDIR/hd.$ext" BIG "$TMPDIR/hdbig2.bin" >/dev/null 2>&1 &&
        cmp -s "$TMPDIR/hdbig.bin" "$TMPDIR/hdbig2.bin" &&
        $DISK45 validate "$TMPDIR/hd.$ext" | grep -q 'Status: OK' &&
        $DISK45 rm "$TMPDIR/hd.$ext" BIG >/dev/null 2>&1 &&
        $DISK45 list "$TMPDIR/hd.$ext" | grep -q "^$((free - 2)) blocks free" &&
        pass "$ext: VICE layout, add/extract/remove, subdirectory, validate" ||
        fail "$ext (cfg $CFG header $HDR bam $BAM)"
done
# the size tells D9090 and D9060 apart
cp "$TMPDIR/hd.d60" "$TMPDIR/hd60.d90"
$DISK45 list "$TMPDIR/hd60.d90" | grep -q '^19439 blocks free' && pass "D9060 image named .d90" || fail "D9060 as .d90"

! $DISK45 create "$TMPDIR/ro.nib" >/dev/null 2>&1 && [ ! -f "$TMPDIR/ro.nib" ] &&
    pass "NIB create refused" || fail "NIB create refused"

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

# Files in subdirectories: add, extract, rename, move, lock, remove
$DISK45 mkdir "$TMPDIR/sub.d81" GAMES/ARCADE >/dev/null 2>&1
$DISK45 add "$TMPDIR/sub.d81" "$TMPDIR/d1.prg" GAMES/ARCADE/PONG >/dev/null 2>&1 &&
    $DISK45 add "$TMPDIR/sub.d81" "$TMPDIR/d1.prg" GAMES/ >/dev/null 2>&1 &&
    $DISK45 extract "$TMPDIR/sub.d81" GAMES/ARCADE/PONG "$TMPDIR/pong.prg" >/dev/null 2>&1 &&
    cmp -s "$TMPDIR/pong.prg" "$TMPDIR/d1.prg" &&
    $DISK45 list "$TMPDIR/sub.d81" GAMES/ | grep -q '"D1"' &&
    ! $DISK45 list "$TMPDIR/sub.d81" | grep -q 'PONG\|"D1"' &&
    pass "add/extract in subdirectories" || fail "add/extract in subdirectories"
$DISK45 mv "$TMPDIR/sub.d81" GAMES/ARCADE/PONG GAMES/ARCADE/PONG2 >/dev/null 2>&1 &&
    $DISK45 mv "$TMPDIR/sub.d81" GAMES/D1 TOP >/dev/null 2>&1 &&
    $DISK45 mv "$TMPDIR/sub.d81" TOP GAMES/ARCADE/ >/dev/null 2>&1 &&
    $DISK45 lock "$TMPDIR/sub.d81" GAMES/ARCADE/TOP >/dev/null 2>&1 &&
    $DISK45 list "$TMPDIR/sub.d81" GAMES/ARCADE/ | grep -q '"PONG2".*PRG' &&
    $DISK45 list "$TMPDIR/sub.d81" GAMES/ARCADE/ | grep -q '"TOP".*PRG<' &&
    ! $DISK45 rmdir "$TMPDIR/sub.d81" GAMES/ARCADE >/dev/null 2>&1 &&
    ! $DISK45 rm "$TMPDIR/sub.d81" GAMES/ARCADE >/dev/null 2>&1 &&
    ! $DISK45 add "$TMPDIR/sub.d81" "$TMPDIR/d1.prg" NOPE/X >/dev/null 2>&1 &&
    $DISK45 rm "$TMPDIR/sub.d81" GAMES/ARCADE/PONG2 >/dev/null 2>&1 &&
    $DISK45 validate "$TMPDIR/sub.d81" | grep -q 'Status: OK' &&
    pass "rename/move/lock/remove in subdirectories" || fail "rename/move/lock/remove in subdirectories"

# ============================================================================
echo "=== PETSCII filters ==="

A2P=$(echo "Hello" | $DISK45 -a2p | xxd -p | tr -d '\n')
# H=0xC8 (shifted), e=0x45, l=0x4C, l=0x4C, o=0x4F, \n=0x0D
echo "$A2P" | grep -qi 'c8454c4c4f0d' && pass "-a2p filter" || fail "-a2p filter (got $A2P)"

P2A=$(printf '\xC8\xC5\xCC\xCC\xCF\x0D' | $DISK45 -p2a)
[ "$P2A" = "HELLO" ] && pass "-p2a filter" || fail "-p2a filter (got '$P2A')"

# #300: punctuation keeps its ASCII byte (| is $7C, not $7D like }; ~ is
# $7E), so the round trip is lossless; both ranges of capitals come back
# as A-Z; a NUL does not end the file
RT=$(printf 'a|b}c~d\\e_f{g`hZ^\n' | $DISK45 -a2p | xxd -p | tr -d '\n')
[ "$RT" = "417c427d437e445c455f467b476048da5e0d" ] &&
    [ "$(printf 'a|b}c~d\\e_f{g`hZ^\n' | $DISK45 -a2p | $DISK45 -p2a)" = 'a|b}c~d\e_f{g`hZ^' ] &&
    [ "$(printf '\x61\xC1\x00\x42' | $DISK45 -p2a | xxd -p)" = "41410062" ] &&
    pass "-a2p/-p2a: punctuation unchanged, both capital ranges, NULs kept" ||
    fail "-a2p/-p2a: punctuation, capitals, NULs (got $RT)"

# #300: BLOCKS FREE leaves out the directory track(s), as the drives do
for f in d64:664 d71:1328 d81:3160; do
    IFS=: read -r ext free <<< "$f"
    rm -f "$TMPDIR/bf.$ext"
    $DISK45 create "$TMPDIR/bf.$ext" -n BF -i BF >/dev/null 2>&1 &&
        $DISK45 list "$TMPDIR/bf.$ext" | grep -q "^$free blocks free" &&
        pass "$ext: $free blocks free when empty" || fail "$ext: blocks free (want $free)"
done

# ============================================================================
echo "=== GZ compression ==="

$DISK45 create "$TMPDIR/test.d64.gz" -n "GZIPPED" >/dev/null 2>&1
$DISK45 add "$TMPDIR/test.d64.gz" "$TMPDIR/d1.prg" GZFILE >/dev/null 2>&1
$DISK45 list "$TMPDIR/test.d64.gz" | grep -q 'GZFILE' && pass "GZ create+add+list" || fail "GZ create+add+list"

# ============================================================================
echo "=== MEGA65 SD card images ==="

# sd create lays a card out as the MEGA65's FDISK: MBR (FAT32 from sector
# 2048, system partition $41 at the end), FDISK's boot sector, the system
# partition header, the configuration sector in card sector 1
SD="$TMPDIR/card.img"
$DISK45 sd create "$SD" 64M >/dev/null 2>&1
MBR=$(dd if="$SD" bs=1 skip=446 count=32 2>/dev/null | xxd -p | tr -d '\n')
BOOT=$(dd if="$SD" bs=512 skip=2048 count=1 2>/dev/null | xxd -p | tr -d '\n')
SYS=$(dd if="$SD" bs=512 skip=67584 count=1 2>/dev/null | head -c 11)
CFG=$(dd if="$SD" bs=512 skip=1 count=1 2>/dev/null | xxd -p | head -c 12)
echo "$MBR" | grep -q '^000000000c0000000008000000000100000000004100000000080100' &&
    [ "${BOOT:0:22}" = "eb58904d45474136357231" ] && [ "${BOOT:1020:4}" = "55aa" ] &&
    [ "$SYS" = "MEGA65SYS00" ] && [ "$CFG" = "010180410001" ] &&
    pass "sd create: MBR, boot sector, system partition, configuration" ||
    fail "sd create layout (MBR $MBR, boot ${BOOT:0:22}, sys $SYS, cfg $CFG)"

head -c 819200 /dev/urandom > "$TMPDIR/sd.d81"
head -c 5000 /dev/urandom > "$TMPDIR/sd.prg"
$DISK45 sd add "$SD" "$TMPDIR/sd.d81" MEGA65.D81 >/dev/null 2>&1 &&
    $DISK45 sd add "$SD" "$TMPDIR/sd.prg" "A Long Name.prg" >/dev/null 2>&1 &&
    $DISK45 sd mkdir "$SD" GAMES >/dev/null 2>&1 &&
    $DISK45 sd add "$SD" "$TMPDIR/sd.prg" games/ >/dev/null 2>&1 &&
    $DISK45 sd extract "$SD" "a long name.prg" "$TMPDIR/sd1.prg" >/dev/null 2>&1 &&
    $DISK45 sd extract "$SD" GAMES/sd.prg "$TMPDIR/sd2.prg" >/dev/null 2>&1 &&
    $DISK45 sd extract "$SD" mega65.d81 "$TMPDIR/sd1.d81" >/dev/null 2>&1 &&
    cmp -s "$TMPDIR/sd1.prg" "$TMPDIR/sd.prg" && cmp -s "$TMPDIR/sd2.prg" "$TMPDIR/sd.prg" &&
    cmp -s "$TMPDIR/sd1.d81" "$TMPDIR/sd.d81" &&
    pass "sd add/mkdir/extract (long names, subdirectory)" || fail "sd add/mkdir/extract"
! $DISK45 sd add "$SD" "$TMPDIR/sd.prg" MEGA65.D81 >/dev/null 2>&1 &&
    $DISK45 sd add "$SD" "$TMPDIR/sd.prg" MEGA65.D81 -f >/dev/null 2>&1 &&
    $DISK45 sd list "$SD" | grep -q ' 5000  MEGA65.D81' &&
    pass "sd add refuses to replace without -f" || fail "sd add -f"
# a directory over several clusters (long names take several entries)
for i in $(seq 1 60); do $DISK45 sd add "$SD" "$TMPDIR/sd.prg" "GAMES/game number $i with a long name.prg" >/dev/null 2>&1; done
$DISK45 sd mv "$SD" "GAMES/game number 7 with a long name.prg" seven.prg >/dev/null 2>&1 &&
    $DISK45 sd rm "$SD" "GAMES/game number 9 with a long name.prg" >/dev/null 2>&1 &&
    [ "$($DISK45 sd list "$SD" GAMES | grep -c 'game number')" = "58" ] &&
    ! $DISK45 sd rm "$SD" GAMES >/dev/null 2>&1 &&
    $DISK45 sd extract "$SD" "GAMES/game number 60 with a long name.prg" "$TMPDIR/sd60.prg" >/dev/null 2>&1 &&
    cmp -s "$TMPDIR/sd60.prg" "$TMPDIR/sd.prg" && $DISK45 sd check "$SD" 2>&1 | grep -q '^OK' &&
    pass "sd mv/rm, a directory over several clusters, check" || fail "sd mv/rm/check"
if command -v fsck.vfat >/dev/null 2>&1; then
    dd if="$SD" of="$TMPDIR/p1.fs" bs=512 skip=2048 count=65536 2>/dev/null
    FSCK=$(fsck.vfat -n "$TMPDIR/p1.fs" 2>&1)
    echo "$FSCK" | grep -q '64 files' && ! echo "$FSCK" | grep -qiE 'differ|lost|cross|truncat|free cluster summary wrong' &&
        pass "sd: fsck.vfat accepts the file system" || fail "sd: fsck.vfat ($FSCK)"
fi
$DISK45 sd config "$SD" --pal --onboarded --mac 02:11:22:33:44:55 --default-disk games.d81 >/dev/null 2>&1
CFG=$(dd if="$SD" bs=512 skip=1 count=1 2>/dev/null | xxd -p | tr -d '\n')
CFG2=$(dd if="$SD" bs=512 skip=67585 count=1 2>/dev/null | xxd -p | tr -d '\n')
[ "${CFG:0:32}" = "01010041000102112233445500008080" ] && [ "$CFG" = "$CFG2" ] &&
    $DISK45 sd config "$SD" | grep -q 'Default disk: *games.d81' &&
    pass "sd config (sector 1 and the system partition copy)" || fail "sd config (${CFG:0:32})"
SUM=$(md5sum < "$SD")
! $DISK45 add "$SD" "$TMPDIR/sd.prg" X >/dev/null 2>&1 && [ "$(md5sum < "$SD")" = "$SUM" ] &&
    $DISK45 list "$SD" 2>&1 | grep -q 'is a MEGA65 SD card image' &&
    pass "disk image commands refuse an SD card image" || fail "disk image commands on an SD card image"

for ext in d64 d81; do
    $DISK45 create "$TMPDIR/relrm.$ext" >/dev/null 2>&1
    F0=$($DISK45 list "$TMPDIR/relrm.$ext" | tail -1)
    $DISK45 rel-create "$TMPDIR/relrm.$ext" big 50 400 >/dev/null 2>&1 &&
        $DISK45 remove "$TMPDIR/relrm.$ext" big >/dev/null 2>&1 &&
        [ "$($DISK45 list "$TMPDIR/relrm.$ext" | tail -1)" = "$F0" ] &&
        $DISK45 validate "$TMPDIR/relrm.$ext" 2>&1 | grep -q 'Status: OK' &&
        pass "remove frees a REL file's side sectors ($ext)" || fail "remove REL side sectors ($ext)"
done

# ============================================================================
echo "=== GEOS files ==="

# GEOS Convert files made by mkcvt.py (Schepers' CVT.TXT / cbmconvert's
# layout): a VLIR application (records of 700, 254, absent, empty, 1 and
# 5000 bytes) and a sequential data file with a lower-case name
python3 src/test/mkcvt.py "$TMPDIR"
$DISK45 create "$TMPDIR/geos.d64" -n GEOSDISK --geos >/dev/null 2>&1
FREE0=$($DISK45 list "$TMPDIR/geos.d64" | tail -1)
$DISK45 info "$TMPDIR/geos.d64" | grep -q 'GEOS: *GEOS format V1.0' &&
    [ "$(dd if="$TMPDIR/geos.d64" bs=1 skip=$((357*256+0xAD)) count=16 2>/dev/null)" = "GEOS format V1.0" ] &&
    $DISK45 validate "$TMPDIR/geos.d64" 2>&1 | grep -q 'Status: OK' &&
    pass "create --geos: signature, border sector" || fail "create --geos"
$DISK45 add "$TMPDIR/geos.d64" "$TMPDIR/vlir.cvt" >/dev/null 2>&1 &&
    $DISK45 add "$TMPDIR/geos.d64" "$TMPDIR/seq.cvt" >/dev/null 2>&1 &&
    $DISK45 list "$TMPDIR/geos.d64" | grep -q '"GeoTest App".*GEOS Application, VLIR' &&
    $DISK45 list "$TMPDIR/geos.d64" | grep -q '"a seq file".*GEOS Application Data' &&
    $DISK45 validate "$TMPDIR/geos.d64" 2>&1 | grep -q 'Status: OK' &&
    pass "add a CVT: a GEOS file (VLIR, sequential)" || fail "add a CVT"
$DISK45 extract "$TMPDIR/geos.d64" "geotest app" "$TMPDIR/g1.cvt" >/dev/null 2>&1 &&
    $DISK45 extract "$TMPDIR/geos.d64" "A SEQ FILE" "$TMPDIR/g2.cvt" >/dev/null 2>&1 &&
    cmp -s "$TMPDIR/g1.cvt" "$TMPDIR/vlir.cvt" && cmp -s "$TMPDIR/g2.cvt" "$TMPDIR/seq.cvt" &&
    pass "extract a GEOS file as its CVT (byte for byte)" || fail "extract GEOS as CVT"
$DISK45 create "$TMPDIR/geos.d81" >/dev/null 2>&1
rm -f "$TMPDIR/geos.d71"
$DISK45 copy "$TMPDIR/geos.d64" '*' "$TMPDIR/geos.d81" >/dev/null 2>&1 &&
    $DISK45 convert "$TMPDIR/geos.d64" "$TMPDIR/geos.d71" >/dev/null 2>&1 &&
    $DISK45 extract "$TMPDIR/geos.d81" "GeoTest App" "$TMPDIR/g3.cvt" >/dev/null 2>&1 &&
    $DISK45 extract "$TMPDIR/geos.d71" "a seq file" "$TMPDIR/g4.cvt" >/dev/null 2>&1 &&
    cmp -s "$TMPDIR/g3.cvt" "$TMPDIR/vlir.cvt" && cmp -s "$TMPDIR/g4.cvt" "$TMPDIR/seq.cvt" &&
    $DISK45 info "$TMPDIR/geos.d71" | grep -q 'GEOS format' &&
    $DISK45 validate "$TMPDIR/geos.d81" 2>&1 | grep -q 'Status: OK' &&
    pass "copy/convert keep GEOS files (D64 → D81, D71)" || fail "copy/convert GEOS"
$DISK45 remove "$TMPDIR/geos.d64" "GeoTest App" >/dev/null 2>&1 &&
    $DISK45 remove "$TMPDIR/geos.d64" "a seq file" >/dev/null 2>&1 &&
    [ "$($DISK45 list "$TMPDIR/geos.d64" | tail -1)" = "$FREE0" ] &&
    $DISK45 validate "$TMPDIR/geos.d64" 2>&1 | grep -q 'Status: OK' &&
    pass "remove frees a GEOS file's info block and records" || fail "remove GEOS"
$DISK45 info "$TMPDIR/vlir.cvt" | grep -q 'VLIR records: 5' &&
    $DISK45 info "$TMPDIR/vlir.cvt" | grep -q 'Class: *Test V1.0' &&
    $DISK45 extract "$TMPDIR/vlir.cvt" RECORD_5 "$TMPDIR/r5.bin" >/dev/null 2>&1 &&
    [ "$(stat -c %s "$TMPDIR/r5.bin")" = "5000" ] &&
    [ "$(tail -c 5000 "$TMPDIR/vlir.cvt" | md5sum)" = "$(md5sum < "$TMPDIR/r5.bin")" ] &&
    pass "CVT reader: records, class" || fail "CVT reader"

# ============================================================================
echo "=== GCR images (G64, G71, NIB, NBZ) ==="

# src/test/mkgcr.py encodes a D64 independently (G64.TXT, MNIB track sizes;
# nibtools' NIB with 8 KB reads that start anywhere and are not byte-aligned;
# the NIB compressed with lz.c's LZ77 as .nbz)
$DISK45 create "$TMPDIR/gcr.d64" -n "GCR TEST" -i G1 >/dev/null 2>&1
head -c 40000 /dev/urandom > "$TMPDIR/gcra.prg"
head -c 3000 /dev/urandom > "$TMPDIR/gcrb.prg"
$DISK45 add "$TMPDIR/gcr.d64" "$TMPDIR/gcra.prg" A >/dev/null 2>&1
$DISK45 add "$TMPDIR/gcr.d64" "$TMPDIR/gcrb.prg" B >/dev/null 2>&1
python3 src/test/mkgcr.py g64 "$TMPDIR/gcr.d64" "$TMPDIR/py.g64"
python3 src/test/mkgcr.py nib "$TMPDIR/gcr.d64" "$TMPDIR/py.nib"
python3 src/test/mkgcr.py nbz "$TMPDIR/gcr.d64" "$TMPDIR/py.nbz"
rm -f "$TMPDIR/dk.g64"
$DISK45 convert "$TMPDIR/gcr.d64" "$TMPDIR/dk.g64" >/dev/null 2>&1 && cmp -s "$TMPDIR/dk.g64" "$TMPDIR/py.g64" &&
    pass "G64 writer: standard tracks (as an independent encoder)" || fail "G64 writer"
for f in py.g64 py.nib py.nbz; do
    rm -f "$TMPDIR/back.d64"
    $DISK45 convert "$TMPDIR/$f" "$TMPDIR/back.d64" >/dev/null 2>&1 && cmp -s "$TMPDIR/back.d64" "$TMPDIR/gcr.d64" &&
        pass "decode $f (every sector)" || fail "decode $f"
done
# a byte in track 30's tail gap (never decoded) survives adding a file
python3 - "$TMPDIR/py.g64" "$TMPDIR/mark.g64" <<'PY'
import sys
d = bytearray(open(sys.argv[1], 'rb').read())
e = (30 - 1) * 2
off = int.from_bytes(d[12 + e * 4:16 + e * 4], 'little')
ln = int.from_bytes(d[off:off + 2], 'little')
d[off + 2 + ln - 1] = 0x56
open(sys.argv[2], 'wb').write(d)
PY
$DISK45 add "$TMPDIR/mark.g64" "$TMPDIR/gcrb.prg" C >/dev/null 2>&1 &&
    $DISK45 extract "$TMPDIR/mark.g64" C "$TMPDIR/gcrc.prg" >/dev/null 2>&1 && cmp -s "$TMPDIR/gcrc.prg" "$TMPDIR/gcrb.prg" &&
    python3 - "$TMPDIR/mark.g64" <<'PY' && pass "G64 write keeps unchanged tracks' GCR" || fail "G64 keeps unchanged tracks"
import sys
d = open(sys.argv[1], 'rb').read()
e = (30 - 1) * 2
off = int.from_bytes(d[12 + e * 4:16 + e * 4], 'little')
ln = int.from_bytes(d[off:off + 2], 'little')
sys.exit(0 if d[off + 2 + ln - 1] == 0x56 else 1)
PY
$DISK45 create "$TMPDIR/gcr.d71" -n G71TEST >/dev/null 2>&1
head -c 300000 /dev/urandom > "$TMPDIR/gcrbig.prg"
$DISK45 add "$TMPDIR/gcr.d71" "$TMPDIR/gcrbig.prg" BIG >/dev/null 2>&1
rm -f "$TMPDIR/gcr.g71" "$TMPDIR/back.d71"
$DISK45 convert "$TMPDIR/gcr.d71" "$TMPDIR/gcr.g71" >/dev/null 2>&1 &&
    $DISK45 convert "$TMPDIR/gcr.g71" "$TMPDIR/back.d71" >/dev/null 2>&1 && cmp -s "$TMPDIR/back.d71" "$TMPDIR/gcr.d71" &&
    [ "$(head -c 10 "$TMPDIR/gcr.g71" | tail -c 2 | xxd -p)" = "00a8" ] &&
    pass "G71 (168 entries, side 2) round trip" || fail "G71 round trip"
rm -f "$TMPDIR/new.g64.gz"
$DISK45 create "$TMPDIR/new.g64.gz" -n NEW >/dev/null 2>&1 && $DISK45 add "$TMPDIR/new.g64.gz" "$TMPDIR/gcrb.prg" X >/dev/null 2>&1 &&
    $DISK45 list "$TMPDIR/new.g64.gz" | grep -q '"X"' &&
    pass "create and change a .g64.gz" || fail "create .g64.gz"
! $DISK45 add "$TMPDIR/py.nib" "$TMPDIR/gcrb.prg" X >/dev/null 2>&1 &&
    pass "NIB stays read-only" || fail "NIB read-only"

# ============================================================================
echo "=== Disk images inside a .zip ==="

rm -f "$TMPDIR/game.d81.zip" "$TMPDIR/multi.zip"
$DISK45 create "$TMPDIR/game.d81.zip" -n ZIPPED >/dev/null 2>&1 &&
    $DISK45 add "$TMPDIR/game.d81.zip" "$TMPDIR/d1.prg" TOOL >/dev/null 2>&1 &&
    $DISK45 list "$TMPDIR/game.d81.zip" | grep -q '"TOOL"' &&
    { ! command -v unzip >/dev/null 2>&1 || unzip -tq "$TMPDIR/game.d81.zip" >/dev/null 2>&1; } &&
    pass "create/add/list x.d81.zip" || fail "x.d81.zip"
if command -v zip >/dev/null 2>&1 && command -v unzip >/dev/null 2>&1; then
    $DISK45 create "$TMPDIR/za.d64" -n AAA >/dev/null 2>&1
    $DISK45 create "$TMPDIR/zb.d81" -n BBB >/dev/null 2>&1
    echo hello > "$TMPDIR/zreadme.txt"
    (cd "$TMPDIR" && zip -q multi.zip za.d64 zb.d81 zreadme.txt)
    ! $DISK45 list "$TMPDIR/multi.zip" >/dev/null 2>&1 &&
        $DISK45 list "$TMPDIR/multi.zip:zb.d81" | grep -q '"BBB"' &&
        $DISK45 add "$TMPDIR/multi.zip:ZA.D64" "$TMPDIR/d1.prg" X >/dev/null 2>&1 &&
        $DISK45 list "$TMPDIR/multi.zip:za.d64" | grep -q '"X"' &&
        unzip -tq "$TMPDIR/multi.zip" >/dev/null 2>&1 &&
        [ "$(cd "$TMPDIR" && unzip -p multi.zip zreadme.txt)" = "hello" ] &&
        [ "$(cd "$TMPDIR" && unzip -p multi.zip zb.d81 | md5sum)" = "$(md5sum < "$TMPDIR/zb.d81")" ] &&
        pass "a zip with several members: zip:member, the others kept" || fail "zip:member"
fi

# ============================================================================
echo "=== CMD images (D1M/D2M/D4M, DNP) ==="

# D2M-DNP.TXT: the system partition on the last physical track (DevBlock at
# +5, partition directory at +8..11), a native partition with header 1/1,
# BAM 1/2-1/33 (sectors 0-34 of track 1 used), directory 1/34
for f in d1m:829440:3165 d2m:1658880:6365 d4m:3317760:12765 dnp:2097152:8157; do
    IFS=: read -r ext size free <<< "$f"
    rm -f "$TMPDIR/c.$ext"
    $DISK45 create "$TMPDIR/c.$ext" -n "CMD $ext" -i JO >/dev/null 2>&1 &&
        [ "$(stat -c %s "$TMPDIR/c.$ext")" = "$size" ] &&
        $DISK45 list "$TMPDIR/c.$ext" | grep -q "^$free blocks free" &&
        pass "create $ext ($size bytes, $free blocks free)" || fail "create $ext"
done
SYS=$(dd if="$TMPDIR/c.d2m" bs=1 skip=$((0x190800)) count=32 2>/dev/null | xxd -p | tr -d '\n')
P1=$(dd if="$TMPDIR/c.d2m" bs=1 skip=$((0x190820)) count=32 2>/dev/null | xxd -p | tr -d '\n')
HDR=$(dd if="$TMPDIR/c.d2m" bs=1 skip=$((0x100)) count=34 2>/dev/null | xxd -p | tr -d '\n')
BAM=$(dd if="$TMPDIR/c.d2m" bs=1 skip=$((0x200)) count=9 2>/dev/null | xxd -p)
T1=$(dd if="$TMPDIR/c.d2m" bs=1 skip=$((0x220)) count=6 2>/dev/null | xxd -p)
[ "${SYS:0:30}" = "0101ff000053595354454da0a0a0a0" ] && [ "${P1:4:2}" = "01" ] && [ "${P1:60:4}" = "0c80" ] &&
    [ "${HDR:0:8}" = "01224800" ] && [ "${HDR:44:10}" = "4a4fa03148" ] && [ "${HDR:64:4}" = "0101" ] &&
    [ "$BAM" = "000048b74a4fc00019" ] && [ "$T1" = "000000001fff" ] &&
    [ "$(dd if="$TMPDIR/c.d2m" bs=1 skip=$((0x1905f0)) count=13 2>/dev/null)" = "CMD FD SERIES" ] &&
    pass "D2M layout: system partition, header, BAM" || fail "D2M layout ($SYS / $P1 / $HDR / $BAM / $T1)"
head -c 100000 /dev/urandom > "$TMPDIR/cbig.prg"
$DISK45 add "$TMPDIR/c.d2m" "$TMPDIR/cbig.prg" BIG >/dev/null 2>&1
for i in $(seq 1 20); do $DISK45 add "$TMPDIR/c.d2m" "$TMPDIR/d1.prg" "F$i" >/dev/null 2>&1; done
$DISK45 extract "$TMPDIR/c.d2m" BIG "$TMPDIR/cbig2.prg" >/dev/null 2>&1 && cmp -s "$TMPDIR/cbig2.prg" "$TMPDIR/cbig.prg" &&
    $DISK45 list "$TMPDIR/c.d2m" | grep -q '"F20"' &&
    $DISK45 validate "$TMPDIR/c.d2m" 2>&1 | grep -q 'Status: OK' &&
    $DISK45 remove "$TMPDIR/c.d2m" BIG >/dev/null 2>&1 &&
    $DISK45 validate "$TMPDIR/c.d2m" 2>&1 | grep -q 'Status: OK' &&
    ! $DISK45 validate "$TMPDIR/c.d2m" 2>&1 | grep -q 'Orphaned' &&
    pass "D2M: add (directory over two sectors), extract, remove, validate" || fail "D2M files"
rm -f "$TMPDIR/c100.dnp"
$DISK45 create "$TMPDIR/c100.dnp" -t 100 >/dev/null 2>&1 && [ "$(stat -c %s "$TMPDIR/c100.dnp")" = "6553600" ] &&
    [ "$(dd if="$TMPDIR/c100.dnp" bs=1 skip=$((0x208)) count=1 2>/dev/null | xxd -p)" = "64" ] &&
    $DISK45 list "$TMPDIR/c100.dnp" | grep -q '^25565 blocks free' &&
    pass "DNP create -t 100" || fail "DNP -t"

# Native subdirectories (type DIR): the entry points to a header block (link
# to the directory, "H", name, ID, "1H"; $20 itself, $22 the parent header,
# $24-$26 the entry's sector and offset); the entry counts header and
# directory blocks
rm -f "$TMPDIR/cs.dnp"
$DISK45 create "$TMPDIR/cs.dnp" -i JO >/dev/null 2>&1
$DISK45 mkdir "$TMPDIR/cs.dnp" GAMES >/dev/null 2>&1
$DISK45 mkdir "$TMPDIR/cs.dnp" GAMES/ARCADE >/dev/null 2>&1
for i in $(seq 1 10); do $DISK45 add "$TMPDIR/cs.dnp" "$TMPDIR/d1.prg" "GAMES/F$i" >/dev/null 2>&1; done
$DISK45 add "$TMPDIR/cs.dnp" "$TMPDIR/d1.prg" GAMES/ARCADE/PONG >/dev/null 2>&1
ENT=$(dd if="$TMPDIR/cs.dnp" bs=1 skip=$((0x2202)) count=8 2>/dev/null | xxd -p)
SIZE=$(dd if="$TMPDIR/cs.dnp" bs=1 skip=$((0x221e)) count=2 2>/dev/null | xxd -p)
DH=$(dd if="$TMPDIR/cs.dnp" bs=1 skip=$((0x2300)) count=39 2>/dev/null | xxd -p | tr -d '\n')
[ "$ENT" = "86012347414d4553" ] && [ "$SIZE" = "0300" ] &&
    [ "$DH" = "0124480047414d4553a0a0a0a0a0a0a0a0a0a0a0a0a04a4fa03148a0a000000001230101012202" ] &&
    $DISK45 list "$TMPDIR/cs.dnp" | grep -q '"GAMES".*DIR' &&
    $DISK45 tree "$TMPDIR/cs.dnp" | grep -q '^      PONG' &&
    $DISK45 extract "$TMPDIR/cs.dnp" GAMES/ARCADE/PONG "$TMPDIR/pong2.prg" >/dev/null 2>&1 &&
    cmp -s "$TMPDIR/pong2.prg" "$TMPDIR/d1.prg" &&
    ! $DISK45 rmdir "$TMPDIR/cs.dnp" GAMES/ARCADE >/dev/null 2>&1 &&
    $DISK45 rm "$TMPDIR/cs.dnp" GAMES/ARCADE/PONG >/dev/null 2>&1 &&
    $DISK45 rmdir "$TMPDIR/cs.dnp" GAMES/ARCADE >/dev/null 2>&1 &&
    $DISK45 validate "$TMPDIR/cs.dnp" | grep -q 'Status: OK' &&
    pass "CMD native subdirectories (DIR): header block, sizes, nested files" ||
    fail "CMD native subdirectories (entry $ENT size $SIZE header $DH)"

# Partitions: create --partitions; disk.d2m:N / :NAME opens one (native in
# place, an emulated 1541/1571/1581 as its D64/D71/D81, written back)
rm -f "$TMPDIR/cp.d2m"
$DISK45 create "$TMPDIR/cp.d2m" --partitions native:GAMES:300,1581:WORK,1541:OLD,native:REST >/dev/null 2>&1
PD=$(dd if="$TMPDIR/cp.d2m" bs=1 skip=$((0x190820)) count=128 2>/dev/null | xxd -p | tr -d '\n')
$DISK45 info "$TMPDIR/cp.d2m" | grep -q '4  native.*"REST"  1536 sectors at 4652' &&
    [ "${PD:4:2}${PD:44:4}${PD:60:4}" = "0100000180" ] &&
    [ "${PD:68:2}${PD:108:4}${PD:124:4}" = "0401800640" ] &&
    $DISK45 add "$TMPDIR/cp.d2m:WORK" "$TMPDIR/d1.prg" W1 >/dev/null 2>&1 &&
    $DISK45 mkdir "$TMPDIR/cp.d2m:2" SUB >/dev/null 2>&1 &&
    $DISK45 add "$TMPDIR/cp.d2m:2" "$TMPDIR/d1.prg" SUB/W2 >/dev/null 2>&1 &&
    $DISK45 add "$TMPDIR/cp.d2m:old" "$TMPDIR/d1.prg" O1 >/dev/null 2>&1 &&
    $DISK45 add "$TMPDIR/cp.d2m:4" "$TMPDIR/d1.prg" R1 >/dev/null 2>&1 &&
    $DISK45 add "$TMPDIR/cp.d2m" "$TMPDIR/d1.prg" G1 >/dev/null 2>&1 &&
    [ "$($DISK45 list "$TMPDIR/cp.d2m" | grep -c PRG)" = 1 ] &&
    $DISK45 list "$TMPDIR/cp.d2m:WORK" | grep -q '"W1"' &&
    $DISK45 list "$TMPDIR/cp.d2m:OLD" | grep -q '"O1"' &&
    $DISK45 list "$TMPDIR/cp.d2m:REST" | grep -q '"R1"' &&
    ! $DISK45 list "$TMPDIR/cp.d2m:REST" | grep -q '"G1"\|"W1"' &&
    $DISK45 extract "$TMPDIR/cp.d2m:WORK" SUB/W2 "$TMPDIR/w2.prg" >/dev/null 2>&1 &&
    cmp -s "$TMPDIR/w2.prg" "$TMPDIR/d1.prg" &&
    dd if="$TMPDIR/cp.d2m" bs=256 skip=768 count=3200 of="$TMPDIR/work.d81" 2>/dev/null &&
    $DISK45 list "$TMPDIR/work.d81" | grep -q '"W1"' &&
    $DISK45 validate "$TMPDIR/work.d81" | grep -q 'Status: OK' &&
    $DISK45 validate "$TMPDIR/cp.d2m:4" | grep -q 'Status: OK' &&
    ! $DISK45 list "$TMPDIR/cp.d2m:9" >/dev/null 2>&1 &&
    [ "$(stat -c %s "$TMPDIR/cp.d2m")" = 1658880 ] &&
    pass "CMD partitions: create --partitions, open native and emulated ones" ||
    fail "CMD partitions ($PD)"

# ============================================================================
echo "=== FUSE mount ==="

# GEOS files appear as <name>.cvt; a .cvt copied in becomes a GEOS file when
# closed; changes reach the image (a relative image path too)
if $DISK45 2>&1 | grep -q ' mount ' && command -v fusermount3 >/dev/null 2>&1 && [ -w /dev/fuse ]; then
    FM="$TMPDIR/fmnt"
    mkdir -p "$FM"
    $DISK45 create "$TMPDIR/fuse.d64" --geos >/dev/null 2>&1
    $DISK45 add "$TMPDIR/fuse.d64" "$TMPDIR/vlir.cvt" >/dev/null 2>&1
    $DISK45 add "$TMPDIR/fuse.d64" "$TMPDIR/d1.prg" TOOL >/dev/null 2>&1
    REL=$(realpath --relative-to="$PWD" "$TMPDIR/fuse.d64")
    $DISK45 mount "$REL" "$FM" >"$TMPDIR/mount.log" 2>&1 &
    for i in $(seq 1 50); do mountpoint -q "$FM" && break; sleep 0.1; done
    if mountpoint -q "$FM"; then
        cmp -s "$FM/GeoTest App.cvt" "$TMPDIR/vlir.cvt" && [ -f "$FM/TOOL.prg" ] &&
            pass "mount: a GEOS file reads as its CVT" || fail "mount: GEOS read"
        cp "$TMPDIR/seq.cvt" "$FM/My Data.cvt"
        mv "$FM/GeoTest App.cvt" "$FM/Renamed App.cvt"
        echo hello > "$FM/note.seq"
        (cd / && fusermount3 -u "$FM")
        for i in $(seq 1 50); do mountpoint -q "$FM" || break; sleep 0.1; done
        $DISK45 list "$TMPDIR/fuse.d64" | grep -q '"My Data".*GEOS Application Data' &&
            $DISK45 list "$TMPDIR/fuse.d64" | grep -q '"Renamed App".*GEOS Application, VLIR' &&
            $DISK45 list "$TMPDIR/fuse.d64" | grep -q '"NOTE".*SEQ' &&
            $DISK45 validate "$TMPDIR/fuse.d64" 2>&1 | grep -q 'Status: OK' &&
            pass "mount: a .cvt written becomes a GEOS file; rename, new file saved" ||
            fail "mount: writes ($(cat "$TMPDIR/mount.log"))"
    else
        echo "  SKIP: mount (FUSE mount failed: $(cat "$TMPDIR/mount.log"))"
    fi

    # CBM subdirectories: listed, read and written at any depth
    $DISK45 create "$TMPDIR/fsub.d81" >/dev/null 2>&1
    $DISK45 mkdir "$TMPDIR/fsub.d81" SUB >/dev/null 2>&1
    $DISK45 mkdir "$TMPDIR/fsub.d81" SUB/INNER >/dev/null 2>&1
    $DISK45 add "$TMPDIR/fsub.d81" "$TMPDIR/d1.prg" SUB/TOOL >/dev/null 2>&1
    printf 'deep text' > "$TMPDIR/note.seq"
    $DISK45 add "$TMPDIR/fsub.d81" "$TMPDIR/note.seq" SUB/INNER/ >/dev/null 2>&1
    $DISK45 mount "$TMPDIR/fsub.d81" "$FM" >"$TMPDIR/mount.log" 2>&1 &
    for i in $(seq 1 50); do mountpoint -q "$FM" && break; sleep 0.1; done
    if mountpoint -q "$FM"; then
        [ -d "$FM/SUB" ] && [ -d "$FM/sub/INNER" ] &&
            [ "$(ls "$FM/SUB" | tr '\n' ' ')" = "INNER TOOL.prg " ] &&
            cmp -s "$FM/SUB/TOOL.prg" "$TMPDIR/d1.prg" &&
            cmp -s "$FM/SUB/INNER/NOTE.seq" "$TMPDIR/note.seq" &&
            pass "mount: subdirectories list and read" ||
            fail "mount: subdirectories ($(ls -R "$FM" 2>&1 | tr '\n' ' '))"
        echo written > "$FM/SUB/NEW.seq"
        mkdir "$FM/SUB/INNER/DEEP" && mkdir "$FM/GONE" && rmdir "$FM/GONE"
        cp "$TMPDIR/d1.prg" "$FM/SUB/INNER/DEEP/COPY.prg"
        mv "$FM/SUB/TOOL.prg" "$FM/TOOL.prg"
        mv "$FM/SUB/INNER/NOTE.seq" "$FM/SUB/NOTE.seq"
        ! rmdir "$FM/SUB/INNER" 2>/dev/null && rm "$FM/SUB/NOTE.seq" &&
            [ "$(cat "$FM/SUB/NEW.seq")" = written ] &&
            pass "mount: files and directories written in subdirectories" ||
            fail "mount: subdirectory writes ($(ls -R "$FM" 2>&1 | tr '\n' ' '))"
        (cd / && fusermount3 -u "$FM")
        for i in $(seq 1 50); do mountpoint -q "$FM" || break; sleep 0.1; done
        $DISK45 tree "$TMPDIR/fsub.d81" > "$TMPDIR/tree.txt"
        $DISK45 extract "$TMPDIR/fsub.d81" SUB/INNER/DEEP/COPY "$TMPDIR/copy.prg" >/dev/null &&
            cmp -s "$TMPDIR/copy.prg" "$TMPDIR/d1.prg" &&
            grep -q '^    NEW (SEQ' "$TMPDIR/tree.txt" && grep -q '^  TOOL (PRG' "$TMPDIR/tree.txt" &&
            grep -q '^      DEEP/' "$TMPDIR/tree.txt" && ! grep -q 'GONE\|NOTE' "$TMPDIR/tree.txt" &&
            $DISK45 validate "$TMPDIR/fsub.d81" 2>&1 | grep -q 'Status: OK' &&
            pass "mount: subdirectory changes saved" ||
            fail "mount: subdirectory changes ($(cat "$TMPDIR/tree.txt" "$TMPDIR/mount.log"))"
    fi
else
    echo "  SKIP: mount (no FUSE)"
fi

# ============================================================================
echo "=== Catalog ==="

# a collection of formats, a zip of two images, an SD card image with a D81,
# a GEOS disk; the catalog indexes every disk image (container members as
# "<container>:<member>")
CAT="$TMPDIR/coll"
CDB="$TMPDIR/cat.db"
mkdir -p "$CAT/sub"
cp "$TMPDIR/gcr.d64" "$CAT/a.d64"
cp "$TMPDIR/py.g64" "$CAT/c.g64"
cp "$TMPDIR/py.nib" "$CAT/d.nib"
cp "$TMPDIR/c100.dnp" "$CAT/e.dnp"
cp "$TMPDIR/geos.d71" "$CAT/geos.d71"
$DISK45 create "$CAT/sub/b.d81" -n BBB >/dev/null 2>&1
$DISK45 add "$CAT/sub/b.d81" "$TMPDIR/gcra.prg" A >/dev/null 2>&1
if command -v zip >/dev/null 2>&1; then
    (cd "$CAT" && zip -q pack.zip a.d64 sub/b.d81)
fi
$DISK45 sd create "$CAT/card.img" 64M --onboarded >/dev/null 2>&1
$DISK45 sd add "$CAT/card.img" "$CAT/sub/b.d81" GAMES.D81 >/dev/null 2>&1
OUT=$($DISK45 catalog build "$CAT" --db "$CDB" 2>&1)
$DISK45 catalog list --db "$CDB" > "$TMPDIR/catlist.txt"
grep -q 'a.d64 ' "$TMPDIR/catlist.txt" && grep -q 'c.g64 ' "$TMPDIR/catlist.txt" &&
    grep -q 'd.nib ' "$TMPDIR/catlist.txt" && grep -q 'e.dnp ' "$TMPDIR/catlist.txt" &&
    grep -q 'card.img:/GAMES.D81 ' "$TMPDIR/catlist.txt" &&
    { ! command -v zip >/dev/null 2>&1 || { grep -q 'pack.zip:a.d64 ' "$TMPDIR/catlist.txt" &&
                                            grep -q 'pack.zip:sub/b.d81 ' "$TMPDIR/catlist.txt"; }; } &&
    pass "catalog build: formats, zip members, SD card files" || fail "catalog build ($OUT)"
ACRC=$(python3 -c "import zlib,sys;print('%08X'%zlib.crc32(open(sys.argv[1],'rb').read()))" "$TMPDIR/gcra.prg")
GCRC=$(python3 -c "import zlib,sys;print('%08X'%zlib.crc32(open(sys.argv[1],'rb').read()))" "$TMPDIR/vlir.cvt")
$DISK45 catalog search A --db "$CDB" | grep -q "$ACRC  $CAT/card.img:/GAMES.D81" &&
    $DISK45 catalog search 'geotest*' --db "$CDB" | grep -q "$GCRC .*(GEOS Application, VLIR)" &&
    pass "catalog search: CRC of contents, GEOS files by their CVT" || fail "catalog search"
$DISK45 catalog build "$CAT" --db "$CDB" 2>&1 | grep -q 'Indexed 0 image(s), [0-9]* unchanged' &&
    pass "catalog build: unchanged images skipped" || fail "catalog rebuild"
rm "$CAT/d.nib" "$CAT/card.img"
$DISK45 catalog prune --db "$CDB" | grep -q 'Pruned 2' && ! $DISK45 catalog list --db "$CDB" | grep -q 'card.img' &&
    pass "catalog prune: deleted images and containers" || fail "catalog prune"
# a catalog made before the geos column gains it
python3 - "$TMPDIR/old.db" <<'PY'
import sqlite3, sys
c = sqlite3.connect(sys.argv[1])
c.executescript("""CREATE TABLE images (id INTEGER PRIMARY KEY AUTOINCREMENT, path TEXT UNIQUE NOT NULL,
  format TEXT, name TEXT, disk_id TEXT, mtime INTEGER, size INTEGER);
CREATE TABLE files (id INTEGER PRIMARY KEY AUTOINCREMENT, image_id INTEGER NOT NULL, name TEXT NOT NULL,
  type TEXT NOT NULL, size_bytes INTEGER, size_sectors INTEGER, crc32 TEXT);""")
c.close()
PY
$DISK45 catalog build "$CAT/geos.d71" --db "$TMPDIR/old.db" >/dev/null 2>&1 &&
    $DISK45 catalog search '*' --db "$TMPDIR/old.db" | grep -q 'GEOS' &&
    pass "catalog: an older database gains the GEOS column" || fail "catalog migration"

# A directory longer than one sector stays on the directory track (the DOS
# follows the chain there only): D81 track 40, D64 track 18
printf 'x' > "$TMPDIR/one.bin"
for fmt in "d81|40|$((39 * 40 * 256))|3" "d64|18|91392|1"; do
    IFS='|' read -r ext dtrack off first <<< "$fmt"
    img="$TMPDIR/dirchain.$ext"
    rm -f "$img"
    $DISK45 create "$img" -n DIRCHAIN >/dev/null 2>&1
    for i in $(seq 1 20); do $DISK45 add "$img" "$TMPDIR/one.bin" "F$i" >/dev/null 2>&1; done
    ok=1; n=0; sec=$first
    while [ "$n" -lt 4 ]; do
        link=$(xxd -s $((off + sec * 256)) -l 2 -p "$img")
        t=$((16#${link:0:2})); sct=$((16#${link:2:2}))
        [ "$t" -eq 0 ] && break
        [ "$t" -eq "$dtrack" ] || ok=0
        sec=$sct; n=$((n + 1))
    done
    [ "$ok" -eq 1 ] && [ "$n" -ge 2 ] && $DISK45 list "$img" | grep -q '"F20"' &&
        pass "$ext: a 20-file directory stays on track $dtrack" || fail "$ext: directory chain leaves track $dtrack"
done

# ============================================================================
echo ""
echo "disk45 tests: $passed passed, $failed failed"
[ "$failed" -eq 0 ] || exit 1
