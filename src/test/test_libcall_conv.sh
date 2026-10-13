#!/bin/bash
# Calls to the library's ZP assembly routines from stack-convention code
# (doc/architecture/calling-conventions.md §3.4):
#  1. cc45's list of those routines (IRBuilder::isLibraryZpRoutine), used for
#     calls without a declaration, matches the zp_call functions the library
#     builds from lib/stdlib_asm/*.s45
#  2. a stack-mode file calling them with no declaration (__builtin_alloca,
#     __builtin_setjmp, implicit alloca...) links with ln45 --no-thunks
#  3. a __fastcall__ declaration after a stack-convention call to the same
#     function is an error (ln45 could not bridge the earlier call)

CC="./bin/cc45 -O2"
LD="./bin/ln45"
NM="./bin/nm45"
T=build/test/libcall_conv
mkdir -p $T

passed=0
failed=0
pass() { echo "PASS: $1"; passed=$((passed + 1)); }
fail() { echo "FAIL: $1"; failed=$((failed + 1)); }

# 1. the routine list (key_pressed is __regparm: its argument is in A, so a
# call without its declaration cannot work in either convention)
$NM -f lib/build/asm_*.o45 2>/dev/null | awk '($2 == "T" || $2 == "W") && /zp_call/ {print $3}' |
    sed 's/^_//' | grep -v '^_' | grep -vx 'key_pressed' | sort -u > $T/lib.txt
sed -n '/^bool IRBuilder::isLibraryZpRoutine/,/};/p' src/main/IRBuilder.cpp | grep -o '"[^"]*"' | tr -d '"' |
    sort -u > $T/cc45.txt
if [ -s $T/lib.txt ] && diff $T/lib.txt $T/cc45.txt > $T/diff.txt; then
    pass "isLibraryZpRoutine lists the library's ZP assembly functions ($(wc -l < $T/lib.txt))"
else
    fail "isLibraryZpRoutine differs from lib/build/asm_*.o45 (< library, > cc45):"
    cat $T/diff.txt
fi

# 2. no thunks for undeclared calls in a stack-mode program
if $CC -c -fno-zpcall -fimplicit-function-declaration src/test-resources/test_libcall_conv_xemu.c -o $T/lc.o45 >/dev/null 2>&1 &&
   $LD --no-thunks -basic -o $T/lc.prg lib/build/crt0.o45 $T/lc.o45 lib/build/c45_stack.lib > $T/lc.log 2>&1; then
    pass "stack-mode __builtin_alloca/setjmp/longjmp/abs and implicit alloca/strlen link with --no-thunks"
else
    fail "stack-mode undeclared library calls need thunks:"
    grep -i "mismatch\|error" $T/lc.log | head -5
fi

# 3. __fastcall__ after a stack call
cat > $T/late.c <<'SRC'
int g(int a, int b);
int t1(void) { return g(1, 2); }
__fastcall__ int g(int a, int b);
int t2(void) { return g(3, 4); }
SRC
if ! $CC -c -fno-zpcall $T/late.c -o $T/late.o45 > $T/late.log 2>&1 &&
   grep -q "declared __fastcall__ after it was called" $T/late.log; then
    pass "late __fastcall__ declaration after a stack call is an error"
else
    fail "late __fastcall__ declaration after a stack call is not reported"
fi
# ... but not in the ZP convention
if $CC -c $T/late.c -o $T/late_zp.o45 > /dev/null 2>&1; then
    pass "late __fastcall__ declaration is fine in the ZP convention"
else
    fail "late __fastcall__ declaration rejected in the ZP convention"
fi

echo ""
echo "libcall convention tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
