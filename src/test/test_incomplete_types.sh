#!/bin/bash
# Pointers to a struct/union never defined in the file (an incomplete type, as
# C allows): declaring, passing and comparing them compiles; using what they
# point to, and pointer arithmetic on them, are errors. (The runtime half is
# src/test-resources/test_cast_ptrptr_xemu.c.)

CC="./bin/cc45 -O2"
T=build/test/incomplete_types
mkdir -p $T
passed=0
failed=0
pass() { echo "PASS: $1"; passed=$((passed + 1)); }
fail() { echo "FAIL: $1"; failed=$((failed + 1)); }

# compiles: $1 the description, $2 the code (after "struct never;")
ok() {
    printf 'struct never;\n%s\n' "$2" > $T/ok.c
    if $CC -c $T/ok.c -o $T/ok.o45 > $T/ok.log 2>&1 && $CC -g -fno-zpcall -c $T/ok.c -o $T/ok2.o45 >> $T/ok.log 2>&1; then
        pass "$1"
    else
        fail "$1:"; head -3 $T/ok.log
    fi
}
# an error matching $3
bad() {
    printf 'struct never;\n%s\n' "$2" > $T/bad.c
    if $CC -c $T/bad.c -o $T/bad.o45 > $T/bad.log 2>&1; then
        fail "$1 compiled"
    elif grep -q "$3" $T/bad.log; then
        pass "$1"
    else
        fail "$1: not \"$3\":"; head -3 $T/bad.log
    fi
}

ok "a member pointing to a struct never defined" 'struct t { struct never *root; int len; }; int f(struct t *t) { t->root = 0; return t->root != 0; }'
ok "a struct with methods and such a member" 'struct t { struct opaque *root; __fastcall__ int size(); }; int f(void) { struct t b; b.root = 0; return b.root == 0; }'
ok "locals, globals, parameters, returns" 'struct never *g; struct never *f(struct never *p) { struct never *q = p; g = q; return g; }'
ok "an array of them, a typedef of one" 'typedef struct h h_t; h_t *hs[3]; struct never *a[4]; int f(void) { return a[1] == 0 && hs[2] == 0; }'
ok "casts to void * and char *, comparisons" 'int f(struct never *p, struct never *q) { void *v = p; char *c = (char *)q; return v == (void *)c || p < q; }'
ok "a pointer to a pointer to one steps 2" 'struct never **f(struct never **p) { return p + 1; }'
bad "p + n" 'struct never *f(struct never *p) { return p + 3; }' "arithmetic on a pointer to an incomplete type 'struct never'"
bad "p - q" 'int f(struct never *p, struct never *q) { return p - q; }' "arithmetic on a pointer to an incomplete type"
bad "++p" 'struct never *f(struct never *p) { return ++p; }' "arithmetic on a pointer to an incomplete type"
bad "p--" 'struct never *f(struct never *p) { p--; return p; }' "arithmetic on a pointer to an incomplete type"
bad "a variable of the type" 'int f(void) { struct never x; return 0; }' "Unknown struct/union type"
bad "p->member" 'int f(struct never *p) { return p->x; }' "not found in struct"

echo ""
echo "incomplete type tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
