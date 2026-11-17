#!/bin/bash
# An array's size (#602): a constant below 0 is an error wherever a size is
# written (the compile-time assert "typedef char check[(cond) ? 1 : -1]"
# relies on it); 0 is a warning, but not for a struct's member (the old
# flexible array member); a size above 0 says nothing.

CC="./bin/cc45 -O2"
T=build/test/array_size
mkdir -p $T
passed=0
failed=0
pass() { echo "PASS: $1"; passed=$((passed + 1)); }
fail() { echo "FAIL: $1"; failed=$((failed + 1)); }

# compiles without a message: $1 the description, $2 the code
ok() {
    printf '%s\n' "$2" > $T/ok.c
    if $CC -c $T/ok.c -o $T/ok.o45 > $T/ok.log 2>&1 && ! grep -q 'size of an array' $T/ok.log &&
       $CC -fsigned-default -c $T/ok.c -o $T/ok2.o45 >> $T/ok.log 2>&1; then
        pass "$1"
    else
        fail "$1:"; head -3 $T/ok.log
    fi
}
# an error "the size of an array is negative", with plain int unsigned and signed
bad() {
    printf '%s\n' "$2" > $T/bad.c
    for f in "" "-fsigned-default"; do
        if $CC $f -c $T/bad.c -o $T/bad.o45 > $T/bad.log 2>&1; then
            fail "$1 compiled ($f)"; return
        elif ! grep -q "error: the size of an array is negative" $T/bad.log; then
            fail "$1 ($f): another message:"; head -3 $T/bad.log; return
        fi
    done
    pass "$1"
}
# compiles, with the warning "the size of an array is 0"
warns() {
    printf '%s\n' "$2" > $T/warn.c
    if $CC -c $T/warn.c -o $T/warn.o45 > $T/warn.log 2>&1 && grep -q "warning: the size of an array is 0" $T/warn.log; then
        pass "$1"
    else
        fail "$1:"; head -3 $T/warn.log
    fi
}

bad "a typedef" 'typedef char t[-1];'
bad "a variable" 'char a[-4];'
bad "a struct member" 'struct s { char m[-2]; };'
bad "a second member of a declaration" 'struct s { char a[2], m[-2]; };'
bad "a local" 'int f(void) { char l[-3]; return 0; }'
bad "a second dimension" 'char m[2][-1];'
bad "a parameter" 'void g(int a[-1]);'
bad "a pointer to an array" 'int (*p)[-2];'
bad "sizeof a type name" 'int f(void) { return sizeof(char[-1]); }'
bad "a cast" 'int k(void *v) { return ((char (*)[-1])v) != 0; }'
bad "an enum constant" 'enum { E = -3 }; char e[E];'
bad "the assert idiom, false" 'char big[10]; typedef char check[(sizeof(big) <= 4) ? 1 : -1];'
ok "the assert idiom, true" 'char big[10]; typedef char check[(sizeof(big) <= 10) ? 1 : -1]; int f(void) { return sizeof(check); }'
ok "sizes above 0, an unsized array, a large one" 'char a[1], b[] = "xy", c[40000]; int f(void) { return sizeof a + sizeof b; }'
ok "a struct member of size 0" 'struct f { int n; char tail[0]; }; int f(struct f *p) { return p->tail[1]; }'
warns "a variable of size 0" 'char zero[0];'
warns "a typedef of size 0" 'typedef char z[0];'

echo ""
echo "array size tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
