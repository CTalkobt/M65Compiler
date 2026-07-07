#!/bin/bash
# test_stdlib_new.sh — Test new stdlib functions (qsort, bsearch, memchr, etc.)

CC=bin/cc45
AS=bin/ca45
LN=bin/ln45
LIB=lib/build/c45.lib
MMEMU=mmemu-cli
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
    $LN "$BUILD/$name.o45" $LIB -o "$BUILD/$name.prg" 2>/dev/null
    if [ $? -ne 0 ]; then fail "$name (link)"; return; fi
    local output=$(echo -e "load $BUILD/$name.prg\nsetpc \$2000\nstep 10000000\nm \$4000 $bytes\nq" | $MMEMU -m rawMega65 2>/dev/null | grep "4000:")
    if echo "$output" | grep -qi "$expected"; then
        pass "$name"
    else
        fail "$name (expected: $expected, got: $output)"
    fi
}

if ! command -v $MMEMU &>/dev/null; then
    echo "mmemu-cli not found — skipping"
    exit 0
fi

echo "=== New stdlib function tests ==="

# --- memchr ---
cat > $BUILD/test_memchr.c << 'EOF'
#include <string.h>
volatile char *r = (char *)0x4000;
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
volatile char *r = (char *)0x4000;
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
volatile char *r = (char *)0x4000;
void main() {
    char *s = strerror(0);
    r[0] = (s[0] != 0) ? 1 : 0;
    s = strerror(ENOMEM);
    r[1] = (s[0] != 0) ? 1 : 0;
    r[2] = 0xAA;
}
EOF
compile_and_run "strerror" "$BUILD/test_strerror.c" "01 01 AA" 3

# --- qsort (global array) ---
cat > $BUILD/test_qsort.c << 'EOF'
#include <stdlib.h>
volatile char *r = (char *)0x4000;
signed int arr[5] = {50, 10, 40, 20, 30};
signed int cmp(const void *a, const void *b) { return *(signed int*)a - *(signed int*)b; }
void main() {
    qsort(arr, 5, sizeof(signed int), cmp);
    r[0] = (char)arr[0];
    r[1] = (char)arr[2];
    r[2] = (char)arr[4];
    r[3] = 0xAA;
}
EOF
compile_and_run "qsort" "$BUILD/test_qsort.c" "0A 1E 32 AA" 4

# --- bsearch (global array) ---
cat > $BUILD/test_bsearch.c << 'EOF'
#include <stdlib.h>
volatile char *r = (char *)0x4000;
signed int sorted[5] = {10, 20, 30, 40, 50};
signed int cmp(const void *a, const void *b) { return *(signed int*)a - *(signed int*)b; }
void main() {
    signed int key = 30;
    signed int *found = (signed int *)bsearch(&key, sorted, 5, sizeof(signed int), cmp);
    r[0] = found ? 1 : 0;
    key = 25;
    found = (signed int *)bsearch(&key, sorted, 5, sizeof(signed int), cmp);
    r[1] = found ? 1 : 0;
    r[2] = 0xAA;
}
EOF
compile_and_run "bsearch" "$BUILD/test_bsearch.c" "01 00 AA" 3

# mktime, asctime: blocked by #179 (struct member access across linked objects)
echo "  SKIP: mktime (blocked by #179)"
echo "  SKIP: asctime (blocked by #179)"

# --- array decay fix (#176) ---
cat > $BUILD/test_array_decay.c << 'EOF'
volatile char *r = (char *)0x4000;
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
