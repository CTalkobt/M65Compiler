#!/usr/bin/env python3
"""check_debug_file.py — the debug file (#202) against a program's memory.

    check_debug_file.py prog.dbg prog.mem prog.prg

test_debug_file_xemu.c's variables at the addresses prog.dbg gives them
hold the values the program left (xemu -dumpmem: the 28-bit memory from 0),
its functions' ranges hold their code (each starts inside the program and
the next begins at the previous one's end, or later), and every line
address is in a function. Prints what fails; exit status 0 when all hold.
"""
import json
import sys

dbg = json.load(open(sys.argv[1]))
mem = open(sys.argv[2], 'rb').read()
prg = open(sys.argv[3], 'rb').read()
load = prg[0] | prg[1] << 8
end = load + len(prg) - 2
bad = []


# (the program's own object: the one with _main; a one-step build's has a
# temporary name)
mine_obj = next((f['object'] for f in dbg['functions'] if f['name'] == '_main'), -1)


def var(name, scope=None):
    for v in dbg['variables']:
        if v['name'] == name and v['scope'] == scope and v['object'] == mine_obj:
            return v
    return None


def word(a): return mem[a] | mem[a + 1] << 8


def check(what, ok):
    if not ok: bad.append(what)


if mem[0xC0C0] != 0xAA: bad.append('the program did not finish')
for name, scope, size, want in [('counter', None, 2, 3), ('total', None, 4, 7 + 4)]:
    v = var(name, scope)
    check(name + ' found', v is not None and v['location'] and 'addr' in v['location'])
    if v and v['location'] and 'addr' in v['location']:
        a = v['location']['addr']
        got = int.from_bytes(mem[a:a + size], 'little')
        check('%s = %d at $%04X (got %d)' % (name, want, a, got), got == want)
v = var('name')
# (cc45's strings are PETSCII: "lamb" is LAMB's codes)
check('name', v and v['type'] == '[8]char' and mem[v['location']['addr']:v['location']['addr'] + 4] == b'LAMB')
v = var('calls', '_sum')
check('sum\'s static calls = 2', v and v['kind'] == 'static' and word(v['location']['addr']) == 2)
pt = dbg['types'].get('struct:pt')
check('struct pt layout', pt and pt['size'] == 5 and [m['offset'] for m in pt['members']] == [0, 2, 3])
a, b, sh = var('a', '_main'), var('b', '_main'), var('shared')
for v, label in [(a, 'a'), (b, 'b'), (sh, 'shared')]:
    check(label + ' has an address', v and v['location'] and 'addr' in v['location'])
if all(v and v['location'] and 'addr' in v['location'] for v in (a, b, sh)):
    aa, ba, sa = a['location']['addr'], b['location']['addr'], sh['location']['addr']
    check('a.x = 3', word(aa) == 3)
    check('a.y = -5', mem[aa + 2] == 0xFB)
    check('a.next = &b', word(aa + 3) == ba)
    check('b.x = 4, b.y = 6, b.next = 0', word(ba) == 4 and mem[ba + 2] == 6 and word(ba + 3) == 0)
    check('shared = b', mem[sa:sa + 5] == mem[ba:ba + 5])

funcs = {f['name']: f for f in dbg['functions']}
for n in ('_add', '_sum', '_main'):
    f = funcs.get(n)
    check(n + ' in the program', f and load <= f['start'] < f['end'] <= end)
spans = sorted((f['start'], f['end']) for f in dbg['functions'] if not f['overlay'])
check('functions do not overlap', all(spans[i][1] <= spans[i + 1][0] for i in range(len(spans) - 1)))
src = [i for i, f in enumerate(dbg['files']) if f.endswith('test_debug_file_xemu.c')]
check('the source file is listed', len(src) == 1)
mine = [l for l in dbg['lines'] if src and l[1] == src[0]]
check('its lines are in its functions', mine and all(
    any(funcs[n]['start'] <= l[0] < funcs[n]['end'] for n in ('_add', '_sum', '_main')) for l in mine))
check('main\'s first line with code', any(l[2] == 28 for l in mine))

for b_ in bad: print('  ' + b_)
sys.exit(1 if bad else 0)
