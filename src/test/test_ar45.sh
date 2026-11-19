#!/bin/bash
# ar45: archives of .o45 objects. A file given to c / a that is not an .o45
# object is stored, not indexed, and ar45 says so (#638).
cd "$(dirname "$0")/../.."
AR=./bin/ar45
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
passed=0; failed=0
pass() { echo "  PASS: $1"; passed=$((passed + 1)); }
fail() { echo "  FAIL: $1"; failed=$((failed + 1)); }

printf 'int add(int a, int b) { return a + b; }\n' > $T/addm.c
./bin/cc45 -O2 -c $T/addm.c -o $T/addm.o45 || fail "addm.c does not compile"
echo "just some notes" > $T/notes.txt
echo "more notes" > $T/more.txt

# c: an object and a text file
$AR c $T/t.lib $T/addm.o45 $T/notes.txt > $T/c.out 2> $T/c.err; rc=$?
[ $rc = 0 ] && pass "c with a non-object exits 0" || fail "c with a non-object: exit $rc"
grep -q "^ar45: warning: $T/notes.txt: not an .o45 object: stored, not indexed" $T/c.err &&
    pass "c warns about the non-object" || fail "c: no warning ($(head -1 $T/c.err))"
grep -q "addm.o45" $T/c.err && fail "c warns about the object" || pass "c says nothing about the object"
$AR t $T/t.lib | grep -q "^notes.txt " && pass "c stores the non-object" || fail "c did not store notes.txt"
$AR s $T/t.lib | grep -q "^_add \[addm.o45\]" && pass "the object is indexed" || fail "_add not in the index"
$AR s $T/t.lib | grep -q "notes.txt" && fail "the non-object is indexed" || pass "the non-object is not indexed"

# a: another one
$AR a $T/t.lib $T/more.txt > $T/a.out 2> $T/a.err; rc=$?
[ $rc = 0 ] && pass "a with a non-object exits 0" || fail "a with a non-object: exit $rc"
grep -q "^ar45: warning: $T/more.txt: not an .o45 object: stored, not indexed" $T/a.err &&
    pass "a warns about the non-object" || fail "a: no warning ($(head -1 $T/a.err))"
grep -q "notes.txt" $T/a.err && fail "a warns about a member already there" || pass "a warns only about the files given"
$AR t $T/t.lib | grep -q "^more.txt " && pass "a stores the non-object" || fail "a did not store more.txt"

# objects alone: no warning
$AR c $T/o.lib $T/addm.o45 > /dev/null 2> $T/o.err
[ -s $T/o.err ] && fail "c with objects only writes to stderr: $(head -1 $T/o.err)" || pass "no warning for objects"

echo "ar45 tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
