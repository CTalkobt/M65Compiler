# Cycles of each p-code instruction's path through an interpreter's
# disassembly (objdump45 -d --cycles): from its handler to the end of the
# next dispatch (jmp (handlers,x)), following the given branch outcomes.
import re, sys
def load(path):
    ins = {}; order = []
    for l in open(path):
        m = re.match(r'\s+([0-9a-f]{4}):\t([0-9a-f ]+?)\s*\t(\S+)\t(\S+)\t?([^;\t]*)', l)
        if not m: continue
        a = int(m.group(1), 16); b = [int(x, 16) for x in m.group(2).split()]
        ins[a] = dict(bytes=b, cyc=m.group(3), mn=m.group(4), op=m.group(5).strip())
        order.append(a)
    return ins, order
def walk(ins, start, choices, stop_at_dispatch=True, limit=400):
    a = start; total = 0; ch = list(choices); stack = []; trace = []
    for _ in range(limit):
        i = ins[a]; n = len(i['bytes']); mn = i['mn']; c = i['cyc']
        if '/' in c:  # a conditional branch
            nt, t = (int(x) for x in c.split('/'))
            take = ch.pop(0) == 't'
            total += t if take else nt
            trace.append(f"{mn}{'(t)' if take else '(n)'}")
            if take: a = int(i['op'].lstrip('$'), 16); continue
            a += n; continue
        total += int(c); trace.append(mn)
        if mn == 'jmp' and i['op'].startswith('(') and ',x' in i['op']:
            return total, trace
        if mn in ('jmp', 'bra') and i['op'].startswith('$'):
            a = int(i['op'].lstrip('$'), 16); continue
        if mn == 'jsr': stack.append(a + n); a = int(i['op'].lstrip('$'), 16); continue
        if mn == 'rts':
            if not stack: return total, trace + ['(rts out)']
            a = stack.pop(); continue
        a += n
    raise RuntimeError('no end')
if __name__ == '__main__':
    ins, order = load(sys.argv[1])
    table = int(sys.argv[2], 16)
    def entry(op):
        lo = ins_bytes(table + 2 * op); hi = ins_bytes(table + 2 * op + 1)
        return lo | hi << 8
    flat = {}
    for a, i in ins.items():
        for k, b in enumerate(i['bytes']): flat[a + k] = b
    def ins_bytes(a): return flat[a]
    for spec in sys.argv[3:]:
        name, op, choices = spec.split(':')
        cyc, tr = walk(ins, entry(int(op, 16)), choices)
        print(f"{name:14} {cyc:4}")
