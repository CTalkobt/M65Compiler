import sys, collections
OPNAMES = {0:'void',1:'i8',2:'i16',3:'i32',4:'iN',5:'ptr',6:'f32'}
for path in sys.argv[1:]:
    fns = []; ops = collections.defaultdict(lambda: [0, 0]); seen = set()
    for l in open(path):
        p = l.split()
        if p[0] == 'F':
            name = p[1]
            dup = name in seen; seen.add(name)
            fns.append(dict(name=name, native=int(p[2]), pc=int(p[3]), insts=int(p[4]), blocks=int(p[5]),
                            vregs=int(p[6]), asm=int(p[8]), labels=int(p[9]), dup=dup))
        elif p[0] == 'O':
            op, t = p[1].split('/')
            ops[op + '/' + OPNAMES.get(int(t), t)][0] += int(p[2]); ops[op + '/' + OPNAMES.get(int(t), t)][1] += int(p[3])
    m = [f for f in fns if f['native'] > 0]
    nat = sum(f['native'] for f in m); pc = sum(f['pc'] for f in m)
    print(f"== {path.split('/')[-1]}: {len(fns)} functions, {len(m)} with a native size")
    print(f"   native {nat} bytes, p-code {pc} bytes ({pc*100/nat:.0f}%), saving {nat-pc} ({(nat-pc)*100/nat:.0f}%)")
    print(f"   IR instructions {sum(f['insts'] for f in m)}: {nat/sum(f['insts'] for f in m):.2f} native bytes and "
          f"{pc/sum(f['insts'] for f in m):.2f} p-code bytes each")
    print(f"   functions with inline asm (native escapes needed): {sum(1 for f in m if f['asm'])}; with > 256 vregs: {sum(1 for f in m if f['vregs']>256)}")
    larger = [f for f in m if f['pc'] >= f['native']]
    print(f"   functions where p-code is not smaller: {len(larger)} ({sum(f['native'] for f in larger)} native bytes)")
    # by function size
    for lo, hi in ((0, 32), (32, 128), (128, 512), (512, 10**9)):
        g = [f for f in m if lo <= f['native'] < hi]
        if g:
            a = sum(f['native'] for f in g); b = sum(f['pc'] for f in g)
            print(f"   native {lo}-{hi if hi < 10**9 else '...'} bytes: {len(g)} functions, {a} -> {b} ({b*100/a:.0f}%)")
    tot = sum(v[1] for v in ops.values())
    print("   p-code bytes by operation (top 14):")
    for k, (c, b) in sorted(ops.items(), key=lambda kv: -kv[1][1])[:14]:
        print(f"     {k:18} {c:6} insts {b:7} bytes ({b*100/tot:4.1f}%)  {b/c:.1f}/inst")
    print("   distinct op/type opcodes:", len(ops))

# The p-code instruction set used: opcodes (operation/type), counted over all files given
allops = collections.defaultdict(lambda: [0, 0, set()])
for path in sys.argv[1:]:
    tag = path.split('/')[-1].split('.')[0]
    for l in open(path):
        p = l.split()
        if p[0] != 'O': continue
        op, t = p[1].split('/')
        k = op + '/' + OPNAMES.get(int(t), t)
        allops[k][0] += int(p[2]); allops[k][1] += int(p[3]); allops[k][2].add(tag)
total = sum(v[0] for v in allops.values())
print(f"\n== instruction set: {total} p-code instructions, {len(allops)} distinct opcodes (operation/type)")
ops_only = collections.Counter()
for k, v in allops.items(): ops_only[k.split('/')[0]] += v[0]
print(f"   {len(ops_only)} distinct operations (the type set aside)")
acc = 0
marks = {0.9: None, 0.99: None, 1.0: None}
for i, (k, v) in enumerate(sorted(allops.items(), key=lambda kv: -kv[1][0]), 1):
    acc += v[0]
    for m in marks:
        if marks[m] is None and acc >= m * total - 1e-9: marks[m] = i
print("   opcodes covering " + ", ".join(f"{int(m*100)}%: {n}" for m, n in marks.items()) + " of the instructions")
only = collections.Counter(min(v[2]) if len(v[2]) == 1 else 'both' for v in allops.values())
print("   used in:", dict(only))
print("   all opcodes, most used first (count, % of instructions, where):")
for k, v in sorted(allops.items(), key=lambda kv: -kv[1][0]):
    print(f"     {k:20} {v[0]:6}  {v[0]*100/total:5.2f}%  {'+'.join(sorted(v[2]))}")
