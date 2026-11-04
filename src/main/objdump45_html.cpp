// objdump45_html.cpp — objdump45 --html (#201): one self-contained page that
// shows the model objdump45 --json writes (objects, .lib members, PRGs,
// binaries, overlay files): a tree of what is in each file, and its views:
// the header, the segments drawn to scale, the link sections, the symbols,
// the functions' attributes (calling convention, clobbers, base page), the
// relocations, the disassembly (branches, jumps and calls followed by a
// click, each label's callers), the data and the call graph. No external
// files: the model is in the page.
#include <string>

namespace {

const char* const kPage = R"OD45HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>objdump45: __TITLE__</title>
<style>
:root {
  --bg: #fbfbfa; --fg: #1d1f21; --muted: #6a6f75; --line: #dcdfe3; --panel: #f1f2f4; --sel: #dbe7fb;
  --accent: #2f62c9; --code: #0e5a3a; --data: #8a5a00; --bss: #7a4fb5; --zp: #b23b3b; --hit: #fff1a8;
  --mono: ui-monospace, SFMono-Regular, Menlo, Consolas, "Liberation Mono", monospace;
}
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) {
    --bg: #17191c; --fg: #e3e5e8; --muted: #9298a0; --line: #30343a; --panel: #1f2226; --sel: #24395c;
    --accent: #7aa7ff; --code: #6fd3a2; --data: #e6b45c; --bss: #c39af0; --zp: #f08a8a; --hit: #5a4f12;
  }
}
:root[data-theme="dark"] {
  --bg: #17191c; --fg: #e3e5e8; --muted: #9298a0; --line: #30343a; --panel: #1f2226; --sel: #24395c;
  --accent: #7aa7ff; --code: #6fd3a2; --data: #e6b45c; --bss: #c39af0; --zp: #f08a8a; --hit: #5a4f12;
}
* { box-sizing: border-box; }
html, body { margin: 0; height: 100%; background: var(--bg); color: var(--fg);
  font: 14px/1.45 system-ui, -apple-system, "Segoe UI", sans-serif; }
header { display: flex; gap: 12px; align-items: center; padding: 8px 16px; border-bottom: 1px solid var(--line); background: var(--panel); flex-wrap: wrap; }
header h1 { font-size: 15px; margin: 0; font-weight: 600; }
header .grow { flex: 1; }
input[type=search] { font: inherit; padding: 4px 8px; border: 1px solid var(--line); border-radius: 6px; background: var(--bg); color: var(--fg); min-width: 200px; }
button { font: inherit; padding: 3px 10px; border: 1px solid var(--line); border-radius: 6px; background: var(--bg); color: var(--fg); cursor: pointer; }
button:disabled { opacity: .45; cursor: default; }
main { display: flex; height: calc(100% - 45px); }
nav { width: 290px; min-width: 200px; overflow: auto; border-right: 1px solid var(--line); padding: 6px 0; background: var(--panel); }
#view { flex: 1; overflow: auto; padding: 12px 16px 40px; }
.tree { list-style: none; margin: 0; padding-left: 14px; }
nav > .tree { padding-left: 4px; }
.node { cursor: pointer; padding: 1px 6px; border-radius: 4px; white-space: nowrap; display: block; outline: none; }
.node:hover { background: var(--sel); }
.node.sel { background: var(--sel); font-weight: 600; }
.tw { display: inline-block; width: 14px; color: var(--muted); }
.count { color: var(--muted); font-size: 12px; margin-left: 4px; }
h2 { font-size: 16px; margin: 4px 0 10px; }
.sub { color: var(--muted); margin: -6px 0 10px; }
table { border-collapse: collapse; font-size: 13px; }
th, td { text-align: left; padding: 3px 10px 3px 0; border-bottom: 1px solid var(--line); vertical-align: top; }
th { color: var(--muted); font-weight: 600; position: sticky; top: 0; background: var(--bg); cursor: pointer; }
td.n, th.n { text-align: right; }
.mono, td.mono { font-family: var(--mono); font-size: 12.5px; }
a, .link { color: var(--accent); cursor: pointer; text-decoration: none; }
a:hover, .link:hover { text-decoration: underline; }
.tag { display: inline-block; padding: 0 6px; margin-right: 3px; border-radius: 9px; font-size: 11.5px; border: 1px solid var(--line); }
.dis { font-family: var(--mono); font-size: 12.5px; white-space: pre; }
.dis .row { padding: 0 4px; }
.dis .lab { color: var(--accent); font-weight: 600; margin-top: 8px; }
.dis .src { color: var(--muted); font-style: italic; }
.dis .ad { color: var(--muted); }
.dis .by { color: var(--muted); }
.dis .mn { color: var(--code); font-weight: 600; }
.dis .nt { color: var(--muted); }
.dis .xr { color: var(--muted); font-size: 11.5px; margin-left: 2ch; }
.hit { background: var(--hit); }
.map { display: flex; height: 34px; border: 1px solid var(--line); border-radius: 6px; overflow: hidden; margin: 6px 0 14px; }
.map div { overflow: hidden; white-space: nowrap; font-size: 11.5px; padding: 2px 6px; color: #fff; min-width: 3px; }
.seg-code { background: var(--code); } .seg-data { background: var(--data); } .seg-bss { background: var(--bss); } .seg-zp { background: var(--zp); }
.kv td:first-child { color: var(--muted); padding-right: 18px; }
.hex { font-family: var(--mono); font-size: 12.5px; white-space: pre; }
.empty { color: var(--muted); }
.cols { columns: 2 320px; }
@media (max-width: 700px) { main { flex-direction: column; } nav { width: auto; max-height: 38vh; border-right: 0; border-bottom: 1px solid var(--line); } }
</style>
</head>
<body>
<header>
  <h1>objdump45 — __TITLE__</h1>
  <button id="back" title="Back (Alt+Left)" disabled>◀ Back</button>
  <span class="grow"></span>
  <input id="q" type="search" placeholder="Filter (symbols, lines, relocations…)" aria-label="Filter">
  <button id="theme" title="Light / dark">Theme</button>
</header>
<main>
  <nav id="nav" aria-label="Contents"></nav>
  <section id="view"></section>
</main>
<script id="model" type="application/json">__MODEL__</script>
<script>
(function () {
'use strict';
const MODELS = JSON.parse(document.getElementById('model').textContent);
const $ = (s, el) => (el || document).querySelector(s);
const esc = (s) => String(s == null ? '' : s).replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]));
const hx = (v, w) => '$' + Number(v).toString(16).padStart(w || 4, '0');
const view = $('#view'), nav = $('#nav'), q = $('#q'), backBtn = $('#back');
let current = null;            // the node shown
const history = [];

try { const t = localStorage.getItem('od45-theme'); if (t) document.documentElement.dataset.theme = t; } catch (e) {}
$('#theme').onclick = () => {
  const dark = matchMedia('(prefers-color-scheme: dark)').matches;
  const now = document.documentElement.dataset.theme || (dark ? 'dark' : 'light');
  const next = now === 'dark' ? 'light' : 'dark';
  document.documentElement.dataset.theme = next;
  try { localStorage.setItem('od45-theme', next); } catch (e) {}
};

// ---- per file: what the views need, derived once ------------------------
function prepare(m) {
  m.byAddr = new Map();        // a disassembly line's address: its section and row
  m.labels = new Map();        // address -> name (labels and symbols)
  m.callers = new Map();       // address -> [{from, fromName, m}]
  (m.symbols || []).forEach((s) => { if (s.value != null && s.seg !== 'U' && !m.labels.has(s.value)) m.labels.set(s.value, s.name); });
  (m.disasm || []).forEach((sec, si) => {
    let fn = null;
    sec.lines.forEach((l, li) => {
      if (l.t === 'L') { m.labels.set(l.a, l.name); fn = l.name; }
      if (!m.byAddr.has(l.a)) m.byAddr.set(l.a, { si, li });
      l.fn = fn;
    });
  });
  // (in an object an operand a relocation names goes there, not to the
  // address it holds before linking: jsr $0000 ; <__mul16>)
  (m.disasm || []).forEach((sec) => sec.lines.forEach((l) => {
    const r = l.t === 'I' && l.n && /^<(.+)>$/.exec(l.n);
    if (r && l.g != null && m.labels.get(l.g) !== r[1] && !/^(b|lb)/.test(l.m)) delete l.g;
  }));
  (m.disasm || []).forEach((sec) => sec.lines.forEach((l) => {
    if (l.t === 'I' && l.g != null) {
      if (!m.callers.has(l.g)) m.callers.set(l.g, []);
      m.callers.get(l.g).push({ from: l.a, fn: l.fn, m: l.m });
    }
  }));
  // the call graph: jsr / jmp / bsr out of each function to a label (or, in an
  // object, to the symbol a relocation names)
  m.calls = new Map();
  (m.disasm || []).forEach((sec) => sec.lines.forEach((l) => {
    if (l.t !== 'I' || !l.fn || !/^(jsr|bsr|jmp)$/.test(l.m)) return;
    let callee = null;
    if (l.g != null && m.labels.has(l.g)) callee = m.labels.get(l.g);
    else if (l.n && !/^\$|undocumented/.test(l.n)) callee = l.n.replace(/^<|>$/g, '').split(/[ +]/)[0];
    if (!callee || (callee === l.fn && l.m !== 'jsr')) return;   // (a jsr to itself: recursion)
    if (!m.calls.has(l.fn)) m.calls.set(l.fn, new Map());
    const k = m.calls.get(l.fn);
    k.set(callee, (k.get(callee) || 0) + 1);
  }));
  (m.members || []).forEach(prepare);
}
MODELS.forEach(prepare);

// ---- the tree ------------------------------------------------------------
function children(m) {
  const k = [];
  if (m.kind === 'library') {
    k.push({ label: 'Symbol index', count: (m.index || []).length, show: () => libIndex(m) });
    (m.members || []).forEach((mm) => k.push({ label: mm.file, file: mm, kids: children(mm), show: () => summary(mm) }));
    return k;
  }
  k.push({ label: 'Header', show: () => summary(m) });
  if (m.segments) k.push({ label: 'Segments', count: m.segments.length, show: () => segments(m) });
  if (m.sections && m.sections.length) k.push({ label: 'Link sections', count: m.sections.length, show: () => sections(m) });
  if (m.overlays) k.push({ label: 'Overlays', count: m.overlays.length, show: () => overlays(m) });
  if (m.symbols) k.push({ label: 'Symbols', count: m.symbols.length, show: () => symbols(m) });
  const fns = (m.symbols || []).filter((s) => s.attr);
  if (fns.length) k.push({ label: 'Functions', count: fns.length, show: () => functions(m) });
  if (m.relocs) k.push({ label: 'Relocations', count: m.relocs.length, show: () => relocs(m) });
  (m.disasm || []).forEach((sec, si) => k.push({ label: 'Disassembly: ' + sec.name, count: sec.lines.filter((l) => l.t === 'I').length,
    kids: sec.lines.filter((l) => l.t === 'L').map((l) => ({ label: l.name, addr: l.a, show: () => disasm(m, si, l.a) })),
    show: () => disasm(m, si) }));
  (m.data || []).forEach((d) => k.push({ label: 'Data: ' + d.name, count: d.hex.length / 2, show: () => data(m, d) }));
  if (m.calls && m.calls.size) k.push({ label: 'Call graph', count: m.calls.size, show: () => callGraph(m) });
  return k;
}
const ROOTS = MODELS.map((m) => ({ label: m.file, file: m, kids: children(m), open: true, show: () => summary(m) }));

const flat = [];   // the visible nodes, in order (arrow keys)
function renderTree() {
  flat.length = 0;
  const build = (nodes, depth) => {
    const ul = document.createElement('ul');
    ul.className = 'tree';
    nodes.forEach((n) => {
      const li = document.createElement('li');
      const span = document.createElement('span');
      span.className = 'node' + (n === current ? ' sel' : '');
      span.tabIndex = -1;
      const hasKids = n.kids && n.kids.length;
      span.innerHTML = '<span class="tw">' + (hasKids ? (n.open ? '▾' : '▸') : '') + '</span>' + esc(n.label) +
        (n.count != null ? '<span class="count">' + n.count + '</span>' : '');
      span.onclick = (e) => {
        if (hasKids && (e.target.className === 'tw' || n === current)) { n.open = !n.open; renderTree(); }
        else select(n);
      };
      n.el = span;
      flat.push(n);
      li.appendChild(span);
      if (hasKids && n.open) li.appendChild(build(n.kids, depth + 1));
      ul.appendChild(li);
    });
    return ul;
  };
  nav.replaceChildren(build(ROOTS, 0));
  if (current && current.el) current.el.scrollIntoView({ block: 'nearest' });
}
function parentOf(target, nodes, parent) {
  for (const n of nodes || ROOTS) {
    if (n === target) return parent || null;
    const p = n.kids && parentOf(target, n.kids, n);
    if (p !== undefined && p !== null) return p;
    if (p === null && n.kids && n.kids.includes(target)) return n;
  }
  return undefined;
}
function select(n, noHistory) {
  if (current && !noHistory && current !== n) { history.push(current); backBtn.disabled = false; }
  current = n;
  q.value = '';
  n.show();
  renderTree();
  view.scrollTop = view.scrollTop;   // (a view scrolls itself to its target)
}
backBtn.onclick = () => { const n = history.pop(); backBtn.disabled = !history.length; if (n) select(n, true); };
nav.tabIndex = 0;
nav.addEventListener('keydown', (e) => {
  if (!current) return;
  const i = flat.indexOf(current);
  const hasKids = current.kids && current.kids.length;
  if (e.key === 'ArrowDown' && i + 1 < flat.length) select(flat[i + 1], true);
  else if (e.key === 'ArrowUp' && i > 0) select(flat[i - 1], true);
  else if (e.key === 'ArrowRight') { if (hasKids && !current.open) { current.open = true; renderTree(); } else if (hasKids) select(current.kids[0], true); }
  else if (e.key === 'ArrowLeft') {
    if (hasKids && current.open) { current.open = false; renderTree(); }
    else { const p = parentOf(current); if (p) select(p, true); }
  } else if (e.key === 'Home') select(flat[0], true);
  else if (e.key === 'End') select(flat[flat.length - 1], true);
  else return;
  e.preventDefault();
});
document.addEventListener('keydown', (e) => { if (e.altKey && e.key === 'ArrowLeft') { backBtn.click(); e.preventDefault(); } });

// ---- views ---------------------------------------------------------------
function page(title, sub, body) {
  view.innerHTML = '<h2>' + esc(title) + '</h2>' + (sub ? '<div class="sub">' + sub + '</div>' : '') + body;
}
// a sortable, filterable table: cols [{h, n (numeric), f(row) -> html, k(row) -> sort key}]
function table(rows, cols, empty) {
  if (!rows.length) return '<p class="empty">' + (empty || 'None.') + '</p>';
  const id = 't' + Math.random().toString(36).slice(2);
  setTimeout(() => {
    const t = document.getElementById(id);
    if (!t) return;
    t.querySelectorAll('th').forEach((th, ci) => th.onclick = () => {
      const dir = th.dataset.dir === 'a' ? 'd' : 'a';
      th.dataset.dir = dir;
      const key = cols[ci].k || ((r) => cols[ci].f(r));
      rows.sort((a, b) => { const x = key(a), y = key(b); return (x < y ? -1 : x > y ? 1 : 0) * (dir === 'a' ? 1 : -1); });
      t.tBodies[0].innerHTML = rows.map((r) => '<tr>' + cols.map((c) => '<td class="' + (c.n ? 'n ' : '') + (c.mono ? 'mono' : '') + '">' + c.f(r) + '</td>').join('') + '</tr>').join('');
      wire(t);
    });
    wire(t);
  });
  return '<table id="' + id + '" class="filterable"><thead><tr>' + cols.map((c) => '<th class="' + (c.n ? 'n' : '') + '">' + esc(c.h) + '</th>').join('') +
    '</tr></thead><tbody>' + rows.map((r) => '<tr>' + cols.map((c) => '<td class="' + (c.n ? 'n ' : '') + (c.mono ? 'mono' : '') + '">' + c.f(r) + '</td>').join('') + '</tr>').join('') +
    '</tbody></table>';
}
// links in a view: data-go="address" (the disassembly there), data-node (a tree node by label path)
function wire(root) {
  (root || view).querySelectorAll('[data-go]').forEach((a) => a.onclick = () => go(currentFile(), Number(a.dataset.go)));
  (root || view).querySelectorAll('[data-member]').forEach((a) => a.onclick = () => {
    const lib = currentFile(), node = findFileNode(lib.members[Number(a.dataset.member)]);
    if (node) { findFileNode(lib).open = true; select(node); }
  });
}
function currentFile() {
  let n = current;
  while (n && !n.file) n = parentOf(n);
  return n ? n.file : MODELS[0];
}
// the disassembly at an address
function go(m, addr) {
  const at = m.byAddr.get(addr);
  if (!at) return;
  const fileNode = findFileNode(m);
  const secNode = fileNode && fileNode.kids.find((k) => k.label === 'Disassembly: ' + m.disasm[at.si].name);
  if (!secNode) return;
  const lab = (secNode.kids || []).find((k) => k.addr === addr);
  if (lab) { secNode.open = true; fileNode.open = true; select(lab); }
  else {
    if (current) { history.push(current); backBtn.disabled = false; }
    current = secNode; secNode.open = true; disasm(m, at.si, addr); renderTree();
  }
}
function findFileNode(m, nodes) {
  for (const n of nodes || ROOTS) {
    if (n.file === m) return n;
    const f = n.kids && findFileNode(m, n.kids);
    if (f) return f;
  }
  return null;
}
const addrLink = (m, a) => m.byAddr.has(a) ? '<a data-go="' + a + '">' + hx(a) + '</a>' : hx(a);

function summary(m) {
  if (m.kind === 'library') {
    const n = (m.members || []).length;
    page(m.file, 'library: ' + n + ' members, ' + (m.index || []).length + ' symbols in its index',
      table(m.members.slice(), [
        { h: 'Member', f: (r) => '<a data-member="' + m.members.indexOf(r) + '">' + esc(r.file) + '</a>', k: (r) => r.file },
        { h: 'Code', n: 1, f: (r) => seg(r, 'TEXT'), k: (r) => segSize(r, 'TEXT') },
        { h: 'Data', n: 1, f: (r) => seg(r, 'DATA'), k: (r) => segSize(r, 'DATA') },
        { h: 'BSS', n: 1, f: (r) => seg(r, 'BSS'), k: (r) => segSize(r, 'BSS') },
        { h: 'Exports', n: 1, f: (r) => (r.symbols || []).filter((s) => s.seg !== 'U').length },
        { h: 'Imports', n: 1, f: (r) => (r.symbols || []).filter((s) => s.seg === 'U').length },
      ]));
    wire();
    return;
  }
  if (m.kind === 'unreadable') { page(m.file, '', '<p>Not an object: ' + esc(m.error) + '</p>'); return; }
  const rows = [['Kind', m.kind]];
  if (m.format) rows.push(['Format', m.format], ['CPU', m.cpu], ['Mode', hx(m.mode)]);
  if (m.base != null) rows.push(['Loaded at', hx(m.base)], ['Size', m.size + ' bytes']);
  if (m.stub) rows.push(['BASIC stub', m.stub.line + ' SYS ' + m.stub.sys + ' (' + hx(m.stub.sys) + ')']);
  if (m.map) rows.push(['Symbols from', m.map]);
  if (m.region != null) rows.push(['Overlays run at', hx(m.region)], ['Loaded from', hx(m.loadAddress, 5)], ['Kind', m.mapped ? 'mapped' : 'copied']);
  (m.options || []).forEach((o) => rows.push(['Option ' + o.name, o.text != null ? o.text : o.size + ' bytes']));
  let body = '<table class="kv">' + rows.map((r) => '<tr><td>' + esc(r[0]) + '</td><td class="mono">' + esc(r[1]) + '</td></tr>').join('') + '</table>';
  if (m.segments) body += '<h2 style="margin-top:18px">Segments</h2>' + segMap(m);
  page(m.file, '', body);
}
const segSize = (m, n) => ((m.segments || []).find((s) => s.name === n) || { size: 0 }).size;
const seg = (m, n) => segSize(m, n) || '';
function segMap(m) {
  const segs = (m.segments || []).filter((s) => s.size);
  const total = segs.reduce((a, s) => a + s.size, 0) || 1;
  return '<div class="map">' + segs.map((s) => '<div class="seg-' + s.type + '" style="flex:' + s.size + '" title="' + s.name + ': ' + s.size + ' bytes at ' + hx(s.base) + '">' +
    s.name + ' ' + s.size + '</div>').join('') + '</div>';
}
function segments(m) {
  page('Segments', m.file, segMap(m) + table(m.segments.slice(), [
    { h: 'Name', f: (r) => esc(r.name) }, { h: 'Type', f: (r) => r.type },
    { h: 'Base', n: 1, mono: 1, f: (r) => hx(r.base), k: (r) => r.base }, { h: 'Size', n: 1, f: (r) => r.size, k: (r) => r.size },
    { h: 'End', n: 1, mono: 1, f: (r) => hx(r.base + r.size), k: (r) => r.base + r.size }, { h: 'Relocations', n: 1, f: (r) => r.relocs, k: (r) => r.relocs },
  ]));
}
function sections(m) {
  page('Link sections', m.file + ' — ln45 keeps or drops each as a whole', table(m.sections.slice(), [
    { h: 'Name', f: (r) => esc(r.name) }, { h: 'Segment', f: (r) => r.seg },
    { h: 'Offset', n: 1, mono: 1, f: (r) => hx(r.offset), k: (r) => r.offset }, { h: 'Size', n: 1, f: (r) => r.size, k: (r) => r.size },
    { h: 'Align', n: 1, f: (r) => r.align, k: (r) => r.align }, { h: '', f: (r) => r.keep ? '<span class="tag">keep</span>' : '' },
  ]));
}
function overlays(m) {
  page('Overlays', m.file, table(m.overlays.slice(), [
    { h: 'Name', f: (r) => esc(r.name) }, { h: 'Size', n: 1, f: (r) => r.size, k: (r) => r.size },
    { h: 'Stored at', n: 1, mono: 1, f: (r) => hx(r.at, 5), k: (r) => r.at }, { h: 'Runs at', n: 1, mono: 1, f: () => hx(m.region) },
  ]));
}
function attrTags(a) {
  return '<span class="tag">' + a.conv + '</span>' + a.flags.map((f) => '<span class="tag">' + f + '</span>').join('');
}
function symbols(m) {
  page('Symbols', m.file, table(m.symbols.slice(), [
    { h: 'Name', f: (r) => r.seg !== 'U' && m.byAddr.has(r.value) ? '<a data-go="' + r.value + '">' + esc(r.name) + '</a>' : esc(r.name), k: (r) => r.name },
    { h: 'Value', n: 1, mono: 1, f: (r) => r.value != null ? hx(r.value) : '', k: (r) => r.value == null ? -1 : r.value },
    { h: 'Segment', f: (r) => r.seg === 'U' ? 'undefined (import)' : esc(r.seg) + (r.weak ? ' <span class="tag">weak</span>' : '') },
    { h: 'Function', f: (r) => r.attr ? attrTags(r.attr) : '', k: (r) => r.attr ? r.attr.conv : '' },
    { h: 'Called from', n: 1, f: (r) => (m.callers.get(r.value) || []).length || '', k: (r) => (m.callers.get(r.value) || []).length },
  ]));
}
function functions(m) {
  const rows = m.symbols.filter((s) => s.attr);
  page('Functions', m.file + ' — their attribute records ($F9): calling convention, what they clobber', table(rows, [
    { h: 'Function', f: (r) => m.byAddr.has(r.value) ? '<a data-go="' + r.value + '">' + esc(r.name) + '</a>' : esc(r.name), k: (r) => r.name },
    { h: 'Convention', f: (r) => attrTags(r.attr), k: (r) => r.attr.conv + r.attr.flags.join() },
    { h: 'Registers', mono: 1, f: (r) => r.attr.regs || '–' }, { h: 'Flags', mono: 1, f: (r) => r.attr.cflags || '–' },
    { h: 'Params', n: 1, f: (r) => r.attr.params, k: (r) => r.attr.params }, { h: 'Frame', n: 1, f: (r) => r.attr.frame, k: (r) => r.attr.frame },
    { h: 'Base page: uses / clobbers / releases', mono: 1, f: (r) => esc([r.attr.zpUses, r.attr.zpClobbers, r.attr.zpRelease].map((x) => x || '–').join(' / ')) },
    { h: 'Calls', f: (r) => callees(m, r.name) },
  ]));
}
function callees(m, name) {
  const c = m.calls && m.calls.get(name);
  if (!c) return '';
  return [...c.keys()].map((n) => { const a = [...m.labels].find(([, v]) => v === n); return a && m.byAddr.has(a[0]) ? '<a data-go="' + a[0] + '">' + esc(n) + '</a>' : esc(n); }).join(', ');
}
function relocs(m) {
  page('Relocations', m.file, table(m.relocs.slice(), [
    { h: 'Segment', f: (r) => r.seg },
    { h: 'Offset', n: 1, mono: 1, f: (r) => { const s = (m.segments || []).find((x) => x.name === r.seg); const a = (s ? s.base : 0) + r.offset; return r.seg === 'TEXT' ? addrLink(m, instrAt(m, a)) + (instrAt(m, a) !== a ? '+' + (a - instrAt(m, a)) : '') : hx(a); }, k: (r) => r.offset },
    { h: 'Type', f: (r) => r.type }, { h: 'Against', f: (r) => r.symbol ? esc(r.symbol) : '<span class="empty">' + r.target + '</span>', k: (r) => r.symbol || r.target },
    { h: 'Extra', n: 1, mono: 1, f: (r) => r.extra != null ? hx(r.extra, 2) : '' },
  ]));
}
function instrAt(m, a) {   // the instruction holding the byte at a
  for (let x = a; x > a - 6; x--) { const at = m.byAddr.get(x); if (at && m.disasm[at.si].lines[at.li].t !== 'L') return x; }
  return a;
}
function disasm(m, si, focus) {
  const sec = m.disasm[si];
  const out = [];
  sec.lines.forEach((l) => {
    if (l.t === 'L') {
      const cs = m.callers.get(l.a) || [];
      out.push('<div class="row lab" id="a' + l.a + '">' + hx(l.a) + ' &lt;' + esc(l.name) + '&gt;:' +
        (cs.length ? '<span class="xr">called / reached from ' + cs.slice(0, 12).map((c) => '<a data-go="' + c.from + '">' + (c.fn && c.fn !== l.name ? esc(c.fn) + ' ' : '') + hx(c.from) + '</a>').join(', ') + (cs.length > 12 ? ' … (' + cs.length + ')' : '') + '</span>' : '') + '</div>');
    } else if (l.t === 'S') {
      out.push('<div class="row src">; ' + esc(l.file) + ':' + l.line + '  ' + esc(l.text) + '</div>');
    } else if (l.t === 'B') {
      out.push('<div class="row" id="a' + l.a + '"><span class="ad">  ' + hx(l.a).slice(1) + ':</span>  <span class="by">' + l.b.padEnd(16) + '</span><span class="mn">.byte</span> $' + l.b + '</div>');
    } else {
      let op = esc(l.o || '');
      if (l.g != null && m.byAddr.has(l.g)) {
        const name = m.labels.get(l.g);
        op = '<a data-go="' + l.g + '">' + op + '</a>' + (name ? ' <span class="nt">&lt;' + esc(name) + '&gt;</span>' : '');
      }
      const note = l.n && !(l.g != null && m.labels.get(l.g) && l.n === '<' + m.labels.get(l.g) + '>') ? '  <span class="nt">; ' + esc(l.n) + '</span>' : '';
      out.push('<div class="row" id="a' + l.a + '"><span class="ad">  ' + hx(l.a).slice(1) + ':</span>  <span class="by">' + l.b.padEnd(16) + '</span>' +
        (l.c ? '<span class="nt">' + l.c.padEnd(6) + '</span>' : '') + '<span class="mn">' + esc(l.m).padEnd(6) + '</span>' + op + note + (l.u ? ' <span class="tag">undocumented</span>' : '') + '</div>');
    }
  });
  page('Disassembly: ' + sec.name, esc(m.file) + ' — ' + esc(sec.cpu) + ', ' + sec.size + ' bytes at ' + hx(sec.base) + (sec.note ? ' — ' + esc(sec.note) : ''),
    '<div class="dis filterable-lines">' + out.join('') + '</div>');
  wire();
  if (focus != null) {
    const el = document.getElementById('a' + focus);
    if (el) { el.classList.add('hit'); el.scrollIntoView({ block: 'center' }); }
  } else view.scrollTop = 0;
}
function data(m, d) {
  const bytes = d.hex.match(/../g) || [];
  const lab = new Map((d.labels || []).map((l) => [l.a, l.name]));
  const rel = new Map((d.relocNames || []).map((l) => [l.a, l.name]));
  const out = [];
  for (let i = 0; i < bytes.length; i += 16) {
    const row = bytes.slice(i, i + 16);
    for (let j = 0; j < row.length; j++) if (lab.has(d.base + i + j)) out.push('<div class="lab" style="color:var(--accent)">' + hx(d.base + i + j) + ' &lt;' + esc(lab.get(d.base + i + j)) + '&gt;:</div>');
    const asc = row.map((b) => { const c = parseInt(b, 16); return c >= 32 && c < 127 ? String.fromCharCode(c) : '.'; }).join('');
    const names = row.map((b, j) => rel.get(d.base + i + j)).filter(Boolean);
    out.push('<div class="row">' + hx(d.base + i).slice(1) + '  ' + row.map((b, j) => (j && j % 4 === 0 ? ' ' : '') + b).join(' ').padEnd(51) + ' ' + esc(asc) +
      (names.length ? '  <span class="nt">; ' + esc(names.join(', ')) + '</span>' : '') + '</div>');
  }
  page('Data: ' + d.name, esc(m.file) + ' — ' + bytes.length + ' bytes at ' + hx(d.base), '<div class="hex filterable-lines">' + out.join('') + '</div>');
}
function callGraph(m) {
  const fns = [...m.calls.keys()].sort();
  const callersOf = new Map();
  m.calls.forEach((k, f) => k.forEach((n, c) => { if (!callersOf.has(c)) callersOf.set(c, []); callersOf.get(c).push(f); }));
  // functions on a cycle (recursion: a SAC function on one needs a frame)
  const onCycle = new Set();
  const state = new Map();
  const visit = (f, stack) => {
    if (state.get(f) === 1) { stack.slice(stack.indexOf(f)).forEach((x) => onCycle.add(x)); return; }
    if (state.get(f) === 2) return;
    state.set(f, 1); stack.push(f);
    (m.calls.get(f) || new Map()).forEach((n, c) => visit(c, stack));
    stack.pop(); state.set(f, 2);
  };
  fns.forEach((f) => visit(f, []));
  const link = (n) => { const a = [...m.labels].find(([, v]) => v === n); return a && m.byAddr.has(a[0]) ? '<a data-go="' + a[0] + '">' + esc(n) + '</a>' : esc(n); };
  const rows = fns.map((f) => ({ f }));
  page('Call graph', esc(m.file) + ' — jsr / jmp / bsr out of each function (a relocated call by its symbol)' + (onCycle.size ? '; on a call cycle: ' + [...onCycle].map(esc).join(', ') : ''),
    table(rows, [
      { h: 'Function', f: (r) => link(r.f) + (onCycle.has(r.f) ? ' <span class="tag">cycle</span>' : ''), k: (r) => r.f },
      { h: 'Calls', f: (r) => [...m.calls.get(r.f).entries()].map(([c, n]) => link(c) + (n > 1 ? ' ×' + n : '')).join(', ') },
      { h: 'Called by', f: (r) => (callersOf.get(r.f) || []).map(link).join(', '), k: (r) => (callersOf.get(r.f) || []).length },
    ]));
}
function libIndex(m) {
  page('Symbol index', m.file, table(m.index.slice(), [
    { h: 'Symbol', f: (r) => esc(r.name) }, { h: 'Member', f: (r) => esc((m.members[r.member] || {}).file || r.member), k: (r) => r.member },
  ]));
}

// ---- the filter -----------------------------------------------------------
q.addEventListener('input', () => {
  const t = q.value.toLowerCase();
  view.querySelectorAll('table.filterable tbody tr').forEach((tr) => { tr.style.display = !t || tr.textContent.toLowerCase().includes(t) ? '' : 'none'; });
  view.querySelectorAll('.filterable-lines > div').forEach((d) => {
    const hit = t && d.textContent.toLowerCase().includes(t);
    d.classList.toggle('hit', !!hit);
  });
  if (t) { const first = view.querySelector('.filterable-lines > .hit'); if (first) first.scrollIntoView({ block: 'center' }); }
});

select(ROOTS[0], true);
nav.focus();
})();
</script>
</body>
</html>
)OD45HTML";

void replaceAll(std::string& s, const std::string& from, const std::string& to) {
    for (size_t at = s.find(from); at != std::string::npos; at = s.find(from, at + to.size()))
        s.replace(at, from.size(), to);
}

}  // namespace

std::string objdump45HtmlPage(const std::string& title, const std::string& modelJson) {
    std::string t;
    for (char c : title) {
        if (c == '<') t += "&lt;";
        else if (c == '>') t += "&gt;";
        else if (c == '&') t += "&amp;";
        else if (c == '"') t += "&quot;";
        else t += c;
    }
    // (the model in a <script>: no "</" in it, no "<!--")
    std::string j = modelJson;
    replaceAll(j, "</", "<\\/");
    replaceAll(j, "<!--", "<\\u0021--");
    std::string page = kPage;
    replaceAll(page, "__TITLE__", t);
    size_t m = page.find("__MODEL__");
    page.replace(m, 9, j);
    return page;
}
