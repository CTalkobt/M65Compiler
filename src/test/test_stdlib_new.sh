#!/bin/bash
# test_stdlib_new.sh — Test new stdlib functions (memchr, isgraph, strerror, qsort, bsearch, array_decay)
# Uses xemu-xmega65 for runtime validation

CC=bin/cc45
LN=bin/ln45
CRT0=lib/build/crt0.o45
LIB=lib/build/c45.lib
XEMU=xemu-xmega65
BUILD=build/test/stdlib_new

mkdir -p $BUILD
passed=0
failed=0

pass() { echo "  PASS: $1"; passed=$((passed + 1)); }
fail() { echo "  FAIL: $1"; failed=$((failed + 1)); }

compile_and_run() {
    local name=$1
    local src=$2
    local expected=$3
    local bytes=$4

    $CC -c "$src" -o "$BUILD/$name.o45" 2>/dev/null
    if [ $? -ne 0 ]; then fail "$name (compile)"; return; fi
    $LN -basic -o "$BUILD/$name.prg" $CRT0 "$BUILD/$name.o45" $LIB 2>/dev/null
    if [ $? -ne 0 ]; then fail "$name (link)"; return; fi

    local dump_file="$BUILD/${name}_memdump.bin"
    timeout 30 $XEMU -headless -besure -prgmode 65 -prgexit -dumpmem "$dump_file" -prg "$BUILD/$name.prg" </dev/null >/dev/null 2>&1
    local exit_code=$?
    if [ $exit_code -ne 0 ] && [ $exit_code -ne 124 ]; then
        fail "$name (xemu exit code $exit_code)"
        return
    fi

    if [ ! -f "$dump_file" ]; then
        fail "$name (no memory dump)"
        return
    fi

    local addr_dec=$((16#C010))
    local actual=$(xxd -s $addr_dec -l $bytes -p "$dump_file" 2>/dev/null | \
                   sed 's/\(..\)/\1 /g' | sed 's/ $//' | tr '[:lower:]' '[:upper:]')
    local expected_upper=$(echo "$expected" | tr '[:lower:]' '[:upper:]')

    if [ "$actual" = "$expected_upper" ]; then
        pass "$name"
    else
        fail "$name (expected: $expected_upper, got: $actual)"
    fi
}

if ! command -v $XEMU &>/dev/null; then
    echo "xemu-xmega65 not found — skipping"
    exit 0
fi

echo "=== New stdlib function tests ==="

# --- memchr ---
cat > $BUILD/test_memchr.c << 'EOF'
#include <string.h>
volatile char *r = (char *)0xC010;
char buf[6] = {10, 20, 30, 40, 50, 60};
void main() {
    char *p = (char *)memchr(buf, 30, 6);
    r[0] = p ? *p : 0xFF;               // 30 = 0x1E
    r[1] = p ? 1 : 0;                   // 1 (found)
    p = (char *)memchr(buf, 99, 6);
    r[2] = p ? 1 : 0;                   // 0 (not found)
    r[3] = 0xAA;
}
EOF
compile_and_run "memchr" "$BUILD/test_memchr.c" "1E 01 00 AA" 4

# --- isgraph ---
cat > $BUILD/test_isgraph.c << 'EOF'
#include <ctype.h>
volatile char *r = (char *)0xC010;
void main() {
    r[0] = isgraph(0x41) ? 1 : 0;
    r[1] = isgraph(0x20) ? 1 : 0;
    r[2] = isgraph(0x05) ? 1 : 0;
    r[3] = isgraph(0x35) ? 1 : 0;
    r[4] = 0xAA;
}
EOF
compile_and_run "isgraph" "$BUILD/test_isgraph.c" "01 00 00 01 AA" 5

# --- strerror ---
cat > $BUILD/test_strerror.c << 'EOF'
#include <string.h>
#include <errno.h>
volatile char *r = (char *)0xC010;
void main() {
    char *s = strerror(0);
    r[0] = (s[0] != 0) ? 1 : 0;
    s = strerror(ENOMEM);
    r[1] = (s[0] != 0) ? 1 : 0;
    r[2] = 0xAA;
}
EOF
compile_and_run "strerror" "$BUILD/test_strerror.c" "01 01 AA" 3

# --- qsort: blocked by SAC codegen bug (for-loop swap after fptr call) ---
echo "  SKIP: qsort (blocked by SAC for-loop + fptr codegen bug)"

# --- bsearch: blocked by same SAC codegen issue ---
echo "  SKIP: bsearch (blocked by SAC for-loop + fptr codegen bug)"

# mktime, asctime: blocked by #179 (struct member access across linked objects)
echo "  SKIP: mktime (blocked by #179)"
echo "  SKIP: asctime (blocked by #179)"

# --- array decay fix (#176) ---
cat > $BUILD/test_array_decay.c << 'EOF'
volatile char *r = (char *)0xC010;
void check(char *s, int n) {
    r[0] = s[0]; r[1] = s[1]; r[2] = (char)n;
}
void main() {
    char buf[4];
    buf[0] = 0x41; buf[1] = 0x42; buf[2] = 0x43; buf[3] = 0x44;
    check(buf, 99);
    r[3] = 0xAA;
}
EOF
compile_and_run "array_decay" "$BUILD/test_array_decay.c" "41 42 63 AA" 4

echo ""
echo "New stdlib tests: $passed passed, $failed failed"
[ "$failed" -eq 0 ] || exit 1
