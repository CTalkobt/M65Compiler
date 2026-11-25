# Schemes A and B side by side (CC45_PCODE_STATS files with both)
import sys
for path in sys.argv[1:]:
    rows = []
    for l in open(path):
        p = l.split()
        if p[0] == 'F' and len(p) >= 12:
            rows.append(dict(nat=int(p[2]), a=int(p[3]), ia=int(p[4]), asm=int(p[8]), b=int(p[10]), ib=int(p[11])))
    nat = sum(r['nat'] for r in rows)
    A = sum(r['a'] for r in rows); B = sum(r['b'] for r in rows)
    ia = sum(r['ia'] for r in rows); ib = sum(r['ib'] for r in rows)
    def choose(key):
        return sum(r[key] if (r['asm'] == 0 and r[key] < r['nat']) else r['nat'] for r in rows)
    def nchosen(key): return sum(1 for r in rows if r['asm'] == 0 and r[key] < r['nat'])
    print(f"== {path.split('/')[-1]}: {len(rows)} functions, native {nat} bytes")
    print(f"   scheme A: {A} bytes ({(nat-A)*100/nat:+.0f}% saved), {ia} instructions")
    print(f"   scheme B: {B} bytes ({(nat-B)*100/nat:+.0f}% saved), {ib} instructions ({ib*100/ia:.0f}% of A's)")
    print(f"   per function, whichever smaller (inline asm native): A {choose('a')} ({(nat-choose('a'))*100/nat:.0f}% saved, {nchosen('a')} as p-code), "
          f"B {choose('b')} ({(nat-choose('b'))*100/nat:.0f}% saved, {nchosen('b')} as p-code)")
    for lo, hi in ((0, 32), (32, 128), (128, 512), (512, 10**9)):
        g = [r for r in rows if lo <= r['nat'] < hi]
        if g:
            a = sum(r['nat'] for r in g)
            print(f"   native {lo}-{hi if hi < 10**9 else '...'}: {len(g):4} functions  A {sum(r['a'] for r in g)*100/a:4.0f}%  B {sum(r['b'] for r in g)*100/a:4.0f}%")
