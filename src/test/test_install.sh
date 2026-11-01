#!/bin/bash
# An installed toolchain finds its headers and libraries: `make install` into
# a temporary prefix, then the installed cc45 builds a program that includes
# system headers (PREFIX/include/cc45) and links the standard library of each
# calling convention (PREFIX/lib/cc45), with CC45_INCLUDE and CC45_LIB unset.

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WORK="$ROOT/build/tmp/test_install"
PREFIX="$WORK/prefix"

passed=0
failed=0
pass() { echo "  PASS: $1"; passed=$((passed + 1)); }
fail() { echo "  FAIL: $1"; failed=$((failed + 1)); }

unset CC45_INCLUDE CC45_LIB
rm -rf "$WORK"
mkdir -p "$WORK/src"

echo "--- Test: installed toolchain (make install PREFIX=build/tmp/test_install/prefix) ---"

if make -C "$ROOT" --no-print-directory install PREFIX="$PREFIX" >"$WORK/install.log" 2>&1; then
    pass "make install"
else
    fail "make install (see $WORK/install.log)"
    echo "Install tests: $passed passed, $failed failed"
    exit 1
fi

for f in bin/cc45 bin/cp45 bin/ln45 bin/ca45 include/cc45/stdio.h include/cc45/sys/types.h \
         lib/cc45/c45.lib lib/cc45/c45_stack.lib lib/cc45/crt0.o45 lib/cc45/crt0_zp.o45; do
    if [ -e "$PREFIX/$f" ]; then pass "installed $f"; else fail "missing $f"; fi
done

cat > "$WORK/src/hello.c" <<'EOF'
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
int main(void) {
    char buf[16];
    strcpy(buf, "hello");
    printf("%s %d\n", buf, (int)strlen(buf));
    return 0;
}
EOF

# Build from a directory with no lib/ or include/ next to it, so only the
# installed tree can supply the headers and libraries
cd "$WORK/src" || exit 1

# build <name> <flags...>: the installed cc45 builds hello.c into <name>.prg
build() {
    local name=$1; shift
    "$PREFIX/bin/cc45" -O2 "$@" hello.c -o "$name.prg" >"$name.log" 2>&1 && [ -s "$name.prg" ]
}

if "$PREFIX/bin/cp45" -O2 hello.c -o hello.i >cp45.log 2>&1 && grep -q 'printf' hello.i; then
    pass "cp45 finds <stdio.h>"
else
    fail "cp45 finds <stdio.h> (see $WORK/src/cp45.log)"
fi

if build zp; then pass "cc45 -O2 hello.c (ZP)"; else fail "cc45 -O2 hello.c (ZP): $(head -3 zp.log)"; fi
if build stack -fno-zpcall; then pass "cc45 -O2 -fno-zpcall hello.c"; else fail "cc45 -O2 -fno-zpcall hello.c: $(head -3 stack.log)"; fi

# The installed ln45 finds a library by name in PREFIX/lib/cc45
if "$PREFIX/bin/cc45" -O2 -c hello.c -o hello.o45 >c.log 2>&1 \
   && "$PREFIX/bin/ln45" -basic -m -o ln.prg hello.o45 -l c45.lib >ln.map 2>&1 \
   && grep -q '_printf' ln.map && grep -q '_main' ln.map; then
    pass "ln45 -l c45.lib (map has _main and _printf)"
else
    fail "ln45 -l c45.lib (see $WORK/src/ln.map)"
fi

# Each convention picks its own library: with c45.lib gone only the stack
# build links, with c45_stack.lib gone only the ZP build
mv "$PREFIX/lib/cc45/c45.lib" "$WORK/c45.lib"
if build stack2 -fno-zpcall; then pass "-fno-zpcall links c45_stack.lib"; else fail "-fno-zpcall without c45.lib: $(head -3 stack2.log)"; fi
if build zp2; then fail "ZP build linked without c45.lib"; else pass "ZP build needs c45.lib"; fi
mv "$WORK/c45.lib" "$PREFIX/lib/cc45/c45.lib"
mv "$PREFIX/lib/cc45/c45_stack.lib" "$WORK/c45_stack.lib"
if build zp3; then pass "ZP build links c45.lib"; else fail "ZP build without c45_stack.lib: $(head -3 zp3.log)"; fi
if build stack3 -fno-zpcall; then fail "-fno-zpcall linked without c45_stack.lib"; else pass "-fno-zpcall needs c45_stack.lib"; fi
mv "$WORK/c45_stack.lib" "$PREFIX/lib/cc45/c45_stack.lib"

# libca45 (#117): a C++ program built against only the installed header and
# archive (-I PREFIX/include, PREFIX/lib/libca45.a) assembles a 45GS02 line
cat > ca45lib.cpp <<'EOF'
#include <ca45/ca45.hpp>
#include <cstdio>
int main() {
    unsigned char b[8];
    int n = ca45::assembleLine("LDA [$6C],Z", b, sizeof b, 0x2000);
    std::printf("%d %02X %02X %02X\n", n, b[0], b[1], b[2]);
    return n == 3 && b[0] == 0xEA && b[1] == 0xB2 && b[2] == 0x6C ? 0 : 1;
}
EOF
if ${CXX:-g++} -std=c++17 -I"$PREFIX/include" ca45lib.cpp "$PREFIX/lib/libca45.a" -o ca45lib >ca45lib.log 2>&1 \
   && ./ca45lib >>ca45lib.log 2>&1; then
    pass "a program linked with the installed libca45.a"
else
    fail "libca45: $(tail -3 ca45lib.log)"
fi

if make -C "$ROOT" --no-print-directory uninstall PREFIX="$PREFIX" >"$WORK/uninstall.log" 2>&1 \
   && [ -z "$(find "$PREFIX" -type f -o -type l)" ]; then
    pass "make uninstall removes every installed file"
else
    fail "make uninstall left: $(find "$PREFIX" -type f -o -type l | head -5 | tr '\n' ' ')"
fi

echo "Install tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
