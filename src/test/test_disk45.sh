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
echo "=== Read-only formats ==="

# D90/D60/CMD: create gives a blank image; changing it is refused and leaves it
# unchanged. G64/NIB/Zipcode cannot be created
for ext in d90 d1m; do
    $DISK45 create "$TMPDIR/ro.$ext" -n "RO" >/dev/null 2>&1
    SUM=$(md5sum < "$TMPDIR/ro.$ext")
    ! $DISK45 add "$TMPDIR/ro.$ext" "$TMPDIR/d1.prg" X >/dev/null 2>&1 &&
        [ "$(md5sum < "$TMPDIR/ro.$ext")" = "$SUM" ] &&
        pass "Read-only $ext: add refused" || fail "Read-only $ext: add refused"
done
! $DISK45 create "$TMPDIR/ro.g64" >/dev/null 2>&1 && [ ! -f "$TMPDIR/ro.g64" ] &&
    pass "G64 create refused" || fail "G64 create refused"

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
echo ""
echo "disk45 tests: $passed passed, $failed failed"
[ "$failed" -eq 0 ] || exit 1
