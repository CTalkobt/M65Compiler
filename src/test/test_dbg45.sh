#!/bin/bash
# test_dbg45.sh — dbg45, the debugger (#202 stage 3), against xemu: a
# command-line session on test_debug_file_xemu.c (breakpoints by line and
# function, start, next, step, finish, continue, backtrace, frames,
# variables, the exit), the Debug Adapter Protocol (dap_client.py: a
# session, and a pause of a program that runs on).
#
#   test_dbg45.sh [-fno-zpcall]
#
# It needs an xemu whose monitor has the breakpoint list ('k'; CTalkobt/xemu,
# branch feat/monitor-breakpoints): $DBG45_XEMU, else xemu-xmega65. Without
# one it prints SKIP and exits 2.
cd "$(dirname "$0")/../.." || exit 1
flags="$1"
name=${flags:+stack}
name=${name:-zp}
T=build/test/dbg45_$name
mkdir -p $T
XEMU=${DBG45_XEMU:-xemu-xmega65}
passed=0
failed=0
pass() { passed=$((passed + 1)); }
fail() { echo "  FAIL: $1"; failed=$((failed + 1)); }

# an xemu with the breakpoint list?
printf 'int main(void) { return 0; }\n' > $T/probe.c
./bin/cc45 -O2 -g $T/probe.c -o $T/probe.prg >/dev/null 2>&1
if ! timeout 60 ./bin/dbg45 --xemu "$XEMU" --headless --fast --timeout 20 --batch -ex run $T/probe.prg 2>&1 |
     grep -q "exited with status 0"; then
    echo "SKIP: dbg45 ($XEMU has no monitor breakpoint list: set DBG45_XEMU to an xemu built from CTalkobt/xemu's feat/monitor-breakpoints)"
    exit 2
fi

cp src/test-resources/test_debug_file_xemu.c $T/t.c
(cd $T && ../../../bin/cc45 -O2 $flags -g t.c -o t.prg >/dev/null 2>&1) || { echo "  FAIL: cc45 -g"; exit 1; }

# a command-line session and what it must print (in order; cc45 may call
# sum(&a) or sum(&b) first: sum(&a) + sum(&b))
timeout 180 ./bin/dbg45 --xemu "$XEMU" --headless --timeout 30 --batch \
    -ex "break t.c:31" -ex start -ex next -ex step -ex continue -ex "info locals" -ex next \
    -ex "print a.next->x" -ex "break sum" -ex continue -ex bt -ex "print *p" -ex "print p->y" -ex "info args" \
    -ex up -ex "print b" -ex down -ex finish -ex "delete" -ex "break 21" -ex continue -ex "print calls" \
    -ex "print s" -ex "delete" -ex continue \
    $T/t.prg > $T/session.txt 2>&1
python3 - $T/session.txt <<'EOF' || fail "the command-line session ($T/session.txt)"
import re, sys
out = open(sys.argv[1]).read()
want = [
    r'Breakpoint 1 at \$[0-9a-f]{4}: t\.c:31',
    # (the first statement: an inlined call, add's lines in the line's place)
    r'main \(\) at t\.c:28', r'28\t    a\.x = add\(1, 2\);',
    r'29\t    a\.y = -5;',
    r'30\t    a\.next = &b;',
    r'Breakpoint 1, main \(\) at t\.c:31', r'31\t    b\.x = 4;',
    r'a = \{x = 3, y = -5, next = 0x[0-9a-f]{4}\}', r'b = \{x = 0, y = 0, next = 0x0000\}',
    r'32\t    b\.y = 6;',
    r'a\.next->x = 4',
    r'Breakpoint 2 at \$[0-9a-f]{4}: sum',
    r'Breakpoint 2, sum \(p=0x[0-9a-f]{4}\) at t\.c:20',
    r'#0  sum \(p=0x[0-9a-f]{4}\) at t\.c:20', r'#1  main \(\) at t\.c:35',
    r'\*p = \{x = (3, y = -5|4, y = 6), next = 0x[0-9a-f]{4}\}',
    r'p->y = (-5|6)',
    r'p = 0x[0-9a-f]{4}',
    r'#1  main \(\) at t\.c:35',
    r'b = \{x = 4, y = 6, next = 0x0000\}',
    r'#0  sum',
    r'Run till exit from sum', r'main \(\) at t\.c:35',
    r'Breakpoint 3 at \$[0-9a-f]{4}: t\.c:21',
    r'Breakpoint 3, sum \(p=0x[0-9a-f]{4}\) at t\.c:21',
    r'calls = 1',
    r's = 0',
    r'\[program exited with status 11\]',
]
at = 0
for w in want:
    m = re.compile(w).search(out, at)
    if not m:
        print('  missing (in order): ' + w)
        sys.exit(1)
    at = m.end()
EOF
[ -s $T/session.txt ] && ! grep -q "^error:" $T/session.txt && pass || fail "errors in the session: $(grep '^error:' $T/session.txt | head -3)"

# the Debug Adapter Protocol: a session, and a pause
timeout 180 python3 src/test/dap_client.py ./bin/dbg45 $T/t.prg $T/t.c "$XEMU" && pass || fail "dbg45 --dap session"
printf 'volatile unsigned int n;\nint main(void) {\n    for (;;) n++;\n}\n' > $T/loop.c
(cd $T && ../../../bin/cc45 -O2 $flags -g loop.c -o loop.prg >/dev/null 2>&1)
timeout 60 python3 src/test/dap_client.py ./bin/dbg45 $T/loop.prg $T/loop.c "$XEMU" pause && pass || fail "dbg45 --dap pause"
# the command line's --timeout pauses a program that runs on
timeout 60 ./bin/dbg45 --xemu "$XEMU" --headless --fast --timeout 2 --batch -ex run -ex bt $T/loop.prg 2>&1 |
    grep -q "#0  main () at loop.c:3" && pass || fail "dbg45 --timeout"

echo "dbg45 tests ($name): $passed passed, $failed failed"
[ $failed -eq 0 ]
