#!/usr/bin/env python3
"""opsize.py <cc45 tree> <out.json> <cc45 flags...> -- <C files...>: where the
code bytes go, by IR operation. Compiles each file with CC45_OPMARK=1 -S
(cc45 writes '; @op <op> <type> <operands>' before each IR instruction's
code), assembles it with ca45 --source-map, and totals the bytes after each
marker (a function's code before its first marker: 'frame'). Operands: v a
vreg, i an immediate, g a global, l a label, - none. The JSON has the
totals ("total") and each file's bytes ("files"); compare two runs to see
what a change saved. Example (the library's C part):

    cd lib && python3 ../src/test/opsize.py .. /tmp/lib.json -O2 -fsigned-default \
        -funsigned-char -fzp-locals-max=8 -Iinclude -- stdlib/*.c
"""
import json, os, re, subprocess, sys, tempfile, collections

tree, out = sys.argv[1], sys.argv[2]
rest = sys.argv[3:]
sep = rest.index("--")
flags, files = rest[:sep], rest[sep + 1:]
cc45, ca45 = os.path.join(tree, "bin", "cc45"), os.path.join(tree, "bin", "ca45")
tot = collections.Counter()
per_file = {}
tmp = tempfile.mkdtemp(dir=os.path.dirname(os.path.abspath(out)))
env = dict(os.environ, CC45_OPMARK="1")
for f in files:
    base = os.path.join(tmp, os.path.basename(f))
    s45, smap = base + ".s45", base + ".map"
    r = subprocess.run([cc45] + flags + ["-S", f, "-o", s45], env=env, capture_output=True, text=True)
    if r.returncode or not os.path.exists(s45):
        print("skip", f, r.stderr.strip()[-120:], file=sys.stderr)
        continue
    r = subprocess.run([ca45, "-c", s45, "-o", base + ".o45", "--source-map", smap], capture_output=True, text=True)
    if not os.path.exists(smap):
        print("noasm", f, r.stderr.strip()[-120:], file=sys.stderr)
        continue
    src = open(s45).read().split("\n")
    # the marker in force at each source line
    tag, cur = {}, "frame"
    for i, line in enumerate(src, 1):
        t = line.strip()
        m = re.match(r";\s*@op (\S+) (\S+) (\S+)", t)
        if m:
            cur = m.group(1) + " " + m.group(2) + " " + m.group(3)
        elif re.match(r"proc\b", t) or t.startswith("endproc"):
            cur = "frame"
        tag[i] = cur
    rows = []
    for line in open(smap):
        m = re.match(r"([0-9a-f]{8})\s+Line (\d+)\s+(.*)", line)
        if m:
            rows.append((int(m.group(1), 16), int(m.group(2)), m.group(3).strip()))
    fc = collections.Counter()
    for k, (a, ln, text) in enumerate(rows):
        if text.startswith("."):
            continue                    # data, reservations
        nxt = rows[k + 1][0] if k + 1 < len(rows) else a
        n = nxt - a
        if n <= 0 or n > 64:
            continue
        fc[tag.get(ln, "frame")] += n
    per_file[f] = sum(fc.values())
    tot.update(fc)
json.dump({"total": tot, "files": per_file}, open(out, "w"), indent=0)
print("files", len(per_file), "bytes", sum(tot.values()))
