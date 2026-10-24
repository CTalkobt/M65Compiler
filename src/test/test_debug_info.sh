#!/bin/bash
# test_debug_info.sh — debug information in objects (#202, stage 1): cc45's
# .dbg_var / .dbg_type directives, ca45's OPT_DEBUG_TYPES / OPT_DEBUG_VARS
# records (locations resolved to the object's segments), objdump45 -g; the
# program's code is the same with them
cd "$(dirname "$0")/../.." || exit 1
T=build/test/debug_info
mkdir -p $T
passed=0
failed=0
pass() { passed=$((passed + 1)); }
fail() { echo "  FAIL: $1"; failed=$((failed + 1)); }

cat > $T/prog.c <<'EOF'
struct pt { int x; signed char y; unsigned flag : 1; struct pt *next; };
union u { long l; unsigned char b[4]; };
int counter;
static signed long total;
char name[8] = "lamb";
union u word;
float ratio;
int add(int x, int y) { int result = x + y; counter += result; return result; }
int sum(struct pt *p) { static int calls; int s = 0; calls++; while (p) { s += p->x; p = p->next; } return s; }
static int first(struct pt q) { return q.x; }
int main(void) {
    struct pt a, b;
    a.x = add(1, 2); a.next = &b; b.x = 4; b.next = 0;
    total = sum(&a) + first(a);
    return (int)total;
}
EOF

# the records, both conventions: types, globals and statics, locals and
# parameters with a location each (no compiler temporaries)
for mode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r name flags <<< "$mode"
    if ! ./bin/cc45 -O2 $flags -c $T/prog.c -o $T/prog_$name.o45 >/dev/null 2>&1; then
        fail "$name: cc45 -c"; continue
    fi
    ./bin/objdump45 -g $T/prog_$name.o45 > $T/g_$name.txt
    g=$T/g_$name.txt
    grep -qE '^    struct pt \(7 bytes\)' $g && grep -qE 'flag +\+3\.0\.1 +u16' $g &&
        grep -qE 'next +\+5 +\*struct:pt' $g && grep -qE '^    union u \(4 bytes\)' $g &&
        grep -qE 'b +\+0 +\[4\]char' $g && pass || fail "$name: types: $(sed -n '/Types:/,/Variables/p' $g)"
    grep -qE 'global +- +counter +u16 +seg:bss\+[0-9]+' $g && grep -qE 'static +- +total +s32 +seg:bss' $g &&
        grep -qE 'global +- +name +\[8\]char +seg:data' $g && grep -qE 'global +- +ratio +f40 +seg:bss' $g &&
        grep -qE 'static +_sum +calls +u16 +seg:bss' $g && pass || fail "$name: globals and statics"
    grep -qE 'param +_add +x +u16 +(zp:\$10|seg:bss)' $g && grep -qE 'param +_sum +p +\*struct:pt +(zp|seg)' $g &&
        grep -qE 'param +_first__prog +q +&struct:pt' $g && grep -qE 'local +_main +a +struct:pt +seg:bss' $g &&
        grep -qE 'local +_main +b +struct:pt +seg:bss' $g && pass || fail "$name: locals and parameters: $(sed -n '/Variables/,$p' $g)"
    ! grep -qE ' __[a-z]' <(sed -n '/Variables/,$p' $g) && pass || fail "$name: a compiler temporary is listed"
done

# ca45: a location of each kind; a long layout over several records
cat > $T/hand.s45 <<'EOF'
    .extern ext
    CONST = $1234
    .code
f:  rts
    .data
d0: .byte 0
d1: .word 0
    .bss
b0: .res 4
    .dbg_var "global - here u16 sym:d1"
    .dbg_var "global - there u8 sym:b0+2"
    .dbg_var "global - far u16 sym:ext+3"
    .dbg_var "global - fixed u16 sym:CONST"
    .dbg_var "global - gone u16 sym:nowhere"
    .dbg_var "local f n u8 zp:$30"
EOF
long="struct big 120"
for i in $(seq 0 39); do long="$long member_number_$i@$((i * 3)):u16"; done
echo "    .dbg_type \"$long\"" >> $T/hand.s45
if ./bin/ca45 -c $T/hand.s45 -o $T/hand.o45 >/dev/null 2>&1; then
    ./bin/objdump45 -g $T/hand.o45 > $T/hand.txt
    grep -qE 'here +u16 +seg:data\+1$' $T/hand.txt && grep -qE 'there +u8 +seg:bss\+2$' $T/hand.txt &&
        grep -qE 'far +u16 +imp:ext\+3$' $T/hand.txt && grep -qE 'fixed +u16 +abs:\$1234$' $T/hand.txt &&
        grep -qE 'gone +u16 +none$' $T/hand.txt && grep -qE 'f +n +u8 +zp:\$30$' $T/hand.txt &&
        pass || fail "ca45 locations: $(sed -n '/Variables/,$p' $T/hand.txt)"
    [ "$(grep -c 'member_number_' $T/hand.txt)" -eq 40 ] && grep -qE 'member_number_39 +\+117 +u16' $T/hand.txt &&
        [ "$(./bin/objdump45 -f $T/hand.o45 | grep -c 'option: DEBUG_TYPES')" -ge 2 ] &&
        pass || fail "a long layout over several records"
else
    fail "ca45 -c with .dbg_var / .dbg_type"
fi
printf '    .code\n    .dbg_var 5\n' > $T/bad.s45
./bin/ca45 -c $T/bad.s45 -o $T/bad.o45 2>&1 | grep -q "dbg_var needs a string" && pass || fail ".dbg_var without a string"

# the program is the same without them: the code of the linked program
# against one assembled from the same source with the directives taken out
# (cc45 --save-temps writes temp_<pid>.s45 in the current directory: $T's)
rm -f $T/temp_*.s45
(cd $T && ../../../bin/cc45 -O2 -c prog.c -o p1.o45 --save-temps >/dev/null 2>&1)
asm=$(ls -t $T/temp_*.s45 2>/dev/null | head -1)
if [ -n "$asm" ]; then
    mv "$asm" $T/p1.s45
    grep -v '\.dbg_' $T/p1.s45 > $T/p0.s45
    ./bin/ca45 -c $T/p1.s45 -o $T/a1.o45 >/dev/null 2>&1 && ./bin/ca45 -c $T/p0.s45 -o $T/a0.o45 >/dev/null 2>&1 &&
        ./bin/ln45 -basic -o $T/a1.prg $T/a1.o45 lib/build/c45.lib >/dev/null 2>&1 &&
        ./bin/ln45 -basic -o $T/a0.prg $T/a0.o45 lib/build/c45.lib >/dev/null 2>&1 &&
        cmp -s $T/a1.prg $T/a0.prg && pass || fail "the program changes with the debug directives"
else
    fail "cc45 --save-temps kept no assembly"
fi

echo "debug info tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
