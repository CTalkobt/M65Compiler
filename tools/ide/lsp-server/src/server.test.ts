import { strict as assert } from 'assert';
import { test, describe } from 'node:test';
import { spawn } from 'child_process';
import * as fs from 'fs';
import * as os from 'os';
import * as path from 'path';
import { fileURLToPath, pathToFileURL } from 'url';
import { parseDiagnostics } from './diagnostics/parseDiagnostics';
import { WorkspaceIndex } from './indexer/workspaceIndex';
import { resolveTool } from './config';
import { identAt, loadReferenceData, wordAt } from './providers/data';
import { CSymbolScanner } from './indexer/cSymbolScanner';
import { AsmSymbolScanner } from './indexer/asmSymbolScanner';
import { SymbolKind } from './indexer/symbolTypes';
import { cleanC } from './indexer/text';
import { TypeResolver } from './providers/cTypes';
import { DocumentSymbolProvider } from './providers/documentSymbol';
import { ReferencesProvider } from './providers/references';
import { HoverProvider } from './providers/hover';
import { asmLineFor } from './diagnostics/runTool';
import { TextDocument } from 'vscode-languageserver-textdocument';

const PKG = path.join(__dirname, '..');
const ROOT = path.join(PKG, '..', '..', '..');

describe('cc45 / ca45 output', () => {
  test('skips the source excerpt and suggestions; one "Compilation error" copy', () => {
    const out = [
      '/tmp/x/a.c:2:33: error: Expected \';\' after return expression.',
      ' 1 | #include "h.h"',
      ' 2 | int f(void) { return fromhdr(1) }',
      '   |                                 ^',
      '',
      'Suggestions:',
      '  • Did you forget a semicolon?',
      'Compilation error: /tmp/x/a.c:2:33: error: Expected \';\' after return expression.',
    ].join('\n');
    const d = parseDiagnostics(out);
    assert.equal(d.length, 1);
    assert.equal(d[0].file, '/tmp/x/a.c');
    assert.equal(d[0].line, 2);
    assert.equal(d[0].column, 33);
  });

  test('ca45\'s "file: line N:" form and cc45\'s bare warnings', () => {
    const d = parseDiagnostics("m.s45: line 2: 'ldq' does not support addressing mode 'abs,X'\n12: warning: unknown __attribute__((x)) ignored\n");
    assert.equal(d.length, 2);
    assert.deepEqual([d[0].file, d[0].line, d[0].severity], ['m.s45', 2, 'error']);
    assert.deepEqual([d[1].file, d[1].line, d[1].severity], ['', 12, 'warning']);
  });

  test('log and summary lines are not errors', () => {
    assert.equal(parseDiagnostics('[cc45:INFO] compiling\nAssembled to m.bin (4 bytes)\n').length, 0);
  });
});

describe('workspace index', () => {
  test('globs', () => {
    assert.ok(WorkspaceIndex.globToRegex('**/node_modules/**').test('a/node_modules/b/c.h'));
    assert.ok(WorkspaceIndex.globToRegex('**/node_modules/**').test('node_modules/b.h'));
    assert.ok(WorkspaceIndex.globToRegex('build/*.c').test('build/x.c'));
    assert.ok(!WorkspaceIndex.globToRegex('build/*.c').test('build/a/x.c'));
  });

  test('a file indexed, excluded directories not', () => {
    const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'm65idx-'));
    fs.mkdirSync(path.join(dir, 'node_modules'));
    fs.writeFileSync(path.join(dir, 'a.c'), 'int add_one(int x) { return x + 1; }\n');
    fs.writeFileSync(path.join(dir, 'node_modules', 'b.c'), 'int hidden(void) { return 0; }\n');
    fs.writeFileSync(path.join(dir, 'c.s45'), 'loop:\n    bra loop\n');
    const idx = new WorkspaceIndex(dir);
    idx.scanWorkspace();
    assert.equal(idx.findDefinitions('add_one').length, 1);
    assert.equal(idx.findDefinitions('add_one')[0].location.uri, pathToFileURL(path.join(dir, 'a.c')).toString());
    assert.equal(idx.findDefinitions('hidden').length, 0);
    assert.equal(idx.findDefinitions('loop').length, 1);
    fs.writeFileSync(path.join(dir, 'a.c'), 'int renamed(int x) { return x; }\n');
    idx.indexFile(path.join(dir, 'a.c'));
    assert.equal(idx.findDefinitions('add_one').length, 0);
    assert.equal(idx.findDefinitions('renamed').length, 1);
    fs.rmSync(dir, { recursive: true, force: true });
  });
});

describe('settings', () => {
  test('${workspaceFolder} expanded; a missing default falls back', () => {
    const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'm65cfg-'));
    fs.mkdirSync(path.join(dir, 'bin'));
    fs.writeFileSync(path.join(dir, 'bin', 'cc45'), '');
    assert.equal(resolveTool('${workspaceFolder}/bin/cc45', 'cc45', dir), path.join(dir, 'bin', 'cc45'));
    assert.equal(resolveTool('bin/cc45', 'cc45', dir), path.join(dir, 'bin', 'cc45'));
    assert.equal(resolveTool('/no/such/cc45', 'cc45', dir), '/no/such/cc45');   // (an explicit path is kept: its error names it)
    fs.rmSync(dir, { recursive: true, force: true });
  });
});

describe('reference data', () => {
  const data = loadReferenceData(path.join(PKG, 'data'));
  test('the quad instructions in the book\'s modes only', () => {
    const ldq = data.opcodes.get('ldq')!;
    assert.deepEqual(ldq.modes.map((m) => m.mode), ['zp', 'abs', '(zp),Z', '[zp],Z']);
    assert.ok(data.opcodes.get('asrq')!.modes.every((m) => m.bytes.startsWith('42 42 ')));
    assert.equal(data.opcodes.get('lda')!.modes.find((m) => m.mode === '#imm')!.bytes, 'A9');
  });
  test('library functions, directives, pragmas', () => {
    assert.match(data.functions.get('printf')!.signature, /printf\(/);
    assert.equal(data.functions.get('dma_copy_far')!.header, 'dma.h');
    assert.ok(data.directives.has('byte') && data.directives.has('segment'));
    assert.ok(data.pragmas.some((p) => p.name === 'cc45 overlay'));
    assert.match(data.keywords.get('int')!.description, /Unsigned unless/);
  });
  test('words', () => {
    assert.equal(wordAt('    lda.abs $12', 6)!.word, 'lda.abs');
    assert.equal(wordAt('  .byte 1', 4)!.word, '.byte');
  });
});

const C_SRC = `#include <stdio.h>
#define MAX 10
#define vic ((volatile struct regs *)0xD000)
/* a node of the list */
struct node {
    struct node *next;
    int v, w[3];
    unsigned char flags : 3;
    union { int i; char c; };          /* anonymous: i and c are node's */
    struct inner { int deep; } in;
    int (*cmp)(const void *, const void *);
    __fastcall__ int size();
};
typedef struct { int x, y; } point_t;
typedef struct node node_t, *node_p;
struct regs { unsigned char border, bg; };
enum colour { RED, GREEN = 5, BLUE };
static struct node head, *tail;
extern int count;
int proto(int a);
// adds one
static int add(int a, int b) {
    struct node *n = &head;
    point_t pt;
    return a + b; /* { not a brace } */
}
`;

describe('C scanner', () => {
  const r = new CSymbolScanner().scan('file:///t.c', C_SRC);
  const by = (n: string) => r.symbols.find((s) => s.name === n);
  test('functions, prototypes, variables, macros, enums', () => {
    assert.equal(by('add')!.kind, SymbolKind.Function);
    assert.equal(by('add')!.isDefinition, true);
    assert.equal(by('add')!.location.range.start.line, 21);
    assert.equal(by('add')!.fullRange!.end.line, 25);
    assert.equal(by('proto')!.isDefinition, false);
    assert.equal(by('count')!.scope, 'extern');
    assert.equal(by('head')!.kind, SymbolKind.Variable);
    assert.equal(by('tail')!.kind, SymbolKind.Variable);
    assert.equal(by('MAX')!.kind, SymbolKind.Constant);
    assert.deepEqual(by('colour')!.children!.map((c) => c.name), ['RED', 'GREEN', 'BLUE']);
    assert.ok(!by('n') && !by('pt'));   // (locals are not the file's)
  });
  test('structs: members, methods, anonymous members, typedefs', () => {
    const node = r.types.aggregates.find((a) => a.tag === 'node')!;
    assert.deepEqual(node.members.map((m) => m.name), ['next', 'v', 'w', 'flags', 'i', 'c', 'in', 'cmp', 'size']);
    assert.deepEqual(node.members.find((m) => m.name === 'next'), { ...node.members.find((m) => m.name === 'next')!, base: 'struct node', ptr: 1 });
    assert.equal(node.members.find((m) => m.name === 'size')!.method, true);
    assert.equal(node.members.find((m) => m.name === 'in')!.base, 'struct inner');
    assert.ok(r.types.aggregates.find((a) => a.tag === 'inner'));
    assert.ok(r.types.aggregates.find((a) => a.tag === 'point_t'));
    assert.deepEqual(r.types.typedefs.get('point_t'), { base: 'struct point_t', ptr: 0 });
    assert.deepEqual(r.types.typedefs.get('node_p'), { base: 'struct node', ptr: 1 });
    assert.deepEqual(r.types.vars.get('tail'), { base: 'struct node', ptr: 1 });
    assert.equal(by('node')!.children!.length, 9);
  });
});

describe('scanner speed', () => {
  test('a file of #defines and long blank runs scans in linear time', () => {
    const src = Array.from({ length: 4000 }, (_, i) => `#define G_${i} (BASE + ${i})`).join('\n') +
      '\n' + ' '.repeat(20000) + '\nint x;\n';
    const t = Date.now();
    const r = new CSymbolScanner().scan('file:///g.h', src);
    assert.equal(r.symbols.length, 4001);
    assert.ok(Date.now() - t < 1000, `${Date.now() - t} ms`);
  });
});

describe('assembly scanner', () => {
  test('procs with their labels, macros, constants, externs', () => {
    const src = '.extern _malloc\nSIZE = 8 ; bytes\nproc _f\n@loop:\n    dex\n    bne @loop\n    rts\nendproc\n.macro mov16 d, s\n.endmacro\ntop:\n';
    const syms = new AsmSymbolScanner().scanFile('file:///t.s45', src);
    const f = syms.find((s) => s.name === '_f')!;
    assert.deepEqual(f.children!.map((c) => c.name), ['@loop']);
    assert.equal(f.fullRange!.end.line, 7);
    assert.equal(syms.find((s) => s.name === 'SIZE')!.kind, SymbolKind.Constant);
    assert.ok(syms.find((s) => s.name === 'mov16'));
    assert.equal(syms.find((s) => s.name === '_malloc')!.isDefinition, false);
    const outline = new DocumentSymbolProvider().onDocumentSymbol(TextDocument.create('file:///t.s45', 'm65-asm', 1, src));
    assert.deepEqual(outline.map((d) => d.name), ['SIZE', '_f', 'mov16', 'top']);
  });
});

describe('member completion', () => {
  const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'm65types-'));
  fs.writeFileSync(path.join(dir, 't.c'), C_SRC);
  const idx = new WorkspaceIndex(dir);
  idx.scanWorkspace();
  const site = (text: string) => {
    const doc = C_SRC + text;
    const types = new CSymbolScanner().scan('file:///x.c', doc).types;
    return new TypeResolver(idx, types).memberSite(cleanC(doc), doc.length);
  };
  test('variables, parameters, chains, typedefs, casts, macros', () => {
    assert.equal(site('void g(struct node *p) { p->')!.agg.tag, 'node');
    assert.equal(site('void g(node_t *p) { p->ne')!.prefix, 'ne');
    assert.equal(site('void g(node_p p) { p->next->')!.agg.tag, 'node');
    assert.equal(site('void g(struct node *p) { p->in.')!.agg.tag, 'inner');
    assert.equal(site('void g(void) { head.next[1].')!.agg.tag, 'node');
    assert.equal(site('void g(void) { point_t q; q.')!.agg.tag, 'point_t');
    assert.equal(site('void g(void *v) { ((struct regs *)v)->')!.agg.tag, 'regs');
    assert.equal(site('void g(struct node **pp) { (*pp)->')!.agg.tag, 'node');
    assert.equal(site('void g(void) { vic->')!.agg.tag, 'regs');
    assert.equal(site('void g(void) { int k; k.'), null);
    assert.equal(site('void g(void) { x = 1.'), null);
    const m = new TypeResolver(idx, new CSymbolScanner().scan('file:///x.c', C_SRC).types);
    const doc = C_SRC + 'void g(struct node *p) { p->flags = 1; }';
    assert.equal(m.memberAt(cleanC(doc), doc.lastIndexOf('flags'), 'flags')!.member.decl, 'unsigned char flags : 3');
  });
  test('references by name, not in comments or strings; hover comments', () => {
    fs.writeFileSync(path.join(dir, 'u.c'), 'int use(void) { /* add */ return add(1, 2) + add(3, 4); } char *s = "add";\n');
    idx.scanWorkspace();
    const doc = TextDocument.create(pathToFileURL(path.join(dir, 't.c')).toString(), 'c', 1, C_SRC);
    const refs = new ReferencesProvider(idx, () => undefined).onReferences(
      { textDocument: { uri: doc.uri }, position: { line: 21, character: 12 }, context: { includeDeclaration: true } }, doc);
    assert.equal(refs.length, 3, JSON.stringify(refs));
    assert.equal(refs.filter((r) => r.uri.endsWith('u.c')).length, 2);
    const lines = C_SRC.split('\n');
    assert.equal(HoverProvider.commentAbove(lines, 4), 'a node of the list');
    assert.equal(HoverProvider.commentAbove(lines, 21), 'adds one');
    fs.rmSync(dir, { recursive: true, force: true });
  });
  test('identifiers', () => {
    assert.equal(identAt('a.b', 0, false), 'a');
    assert.equal(identAt('  jmp @loop', 8, true), '@loop');
    assert.equal(identAt('  lda cc45.bptop', 9, true), 'cc45.bptop');
  });
});

describe('C to assembly', () => {
  test('a C line\'s .loc: that line, else the next, else the one before', () => {
    const asm = '.loc "/w/a.c", 3\n lda #1\n.loc "/w/a.c", 7\n rts\n.loc "/w/b.c", 5\n';
    assert.equal(asmLineFor(asm, '/w/a.c', 2), 0);
    assert.equal(asmLineFor(asm, '/w/a.c', 4), 2);
    assert.equal(asmLineFor(asm, '/w/a.c', 9), 2);
  });
});

// the server itself, over stdio (needs bin/cc45, bin/ca45 and npm run build)
describe('language server', () => {
  const server = path.join(PKG, 'dist', 'server.js');
  const cc45 = path.join(ROOT, 'bin', 'cc45');
  const skip = !fs.existsSync(server) || !fs.existsSync(cc45) ? 'needs npm run build and bin/cc45' : false;

  test('diagnostics of unsaved text, hover, completion, definition', { skip, timeout: 30000 }, async () => {
    const ws = fs.mkdtempSync(path.join(os.tmpdir(), 'm65lsp-'));
    fs.writeFileSync(path.join(ws, 'h.h'), 'int helper(int a);\n');
    fs.writeFileSync(path.join(ws, 'a.c'), '#include "h.h"\nint helper(int a) { return a + 1; }\n');
    fs.writeFileSync(path.join(ws, 'b.s45'), 'start:\n    lda #1\n    bra start\n');
    const srv = spawn('node', [server, '--stdio'], { cwd: ws });
    let buf = Buffer.alloc(0);
    let id = 0;
    const pending = new Map<number, (r: unknown) => void>();
    const diags = new Map<string, { range: { start: { line: number } }; message: string }[]>();
    srv.stdout.on('data', (d: Buffer) => {
      buf = Buffer.concat([buf, d]);
      for (;;) {
        const h = buf.indexOf('\r\n\r\n');
        if (h < 0) break;
        const len = Number(/Content-Length: (\d+)/.exec(buf.subarray(0, h).toString())![1]);
        if (buf.length < h + 4 + len) break;
        const msg = JSON.parse(buf.subarray(h + 4, h + 4 + len).toString());
        buf = buf.subarray(h + 4 + len);
        if (msg.id !== undefined && pending.has(msg.id)) { pending.get(msg.id)!(msg.result); pending.delete(msg.id); }
        else if (msg.id !== undefined) send({ jsonrpc: '2.0', id: msg.id, result: [{ cc45Path: cc45, ca45Path: path.join(ROOT, 'bin', 'ca45') }] });
        else if (msg.method === 'textDocument/publishDiagnostics') diags.set(msg.params.uri, msg.params.diagnostics);
      }
    });
    const send = (m: object) => { const s = JSON.stringify(m); srv.stdin.write(`Content-Length: ${Buffer.byteLength(s)}\r\n\r\n${s}`); };
    const req = <T>(method: string, params: object) => new Promise<T>((r) => { const i = ++id; pending.set(i, r as (x: unknown) => void); send({ jsonrpc: '2.0', id: i, method, params }); });
    const uri = (f: string) => pathToFileURL(path.join(ws, f)).toString();
    const until = async (f: () => boolean) => { for (let i = 0; i < 100 && !f(); i++) await new Promise((r) => setTimeout(r, 100)); };
    try {
      await req('initialize', { processId: process.pid, rootUri: pathToFileURL(ws).toString(), capabilities: { workspace: { configuration: true } } });
      send({ jsonrpc: '2.0', method: 'initialized', params: {} });
      // the buffer differs from a.c on disk: a missing ';' on line 3
      const text = '#include "h.h"\nint helper(int a) { return a + 1; }\nint main(void) { int x = helper(2) return x; }\n';
      send({ jsonrpc: '2.0', method: 'textDocument/didOpen', params: { textDocument: { uri: uri('a.c'), languageId: 'c', version: 1, text } } });
      send({ jsonrpc: '2.0', method: 'textDocument/didOpen', params: { textDocument: { uri: uri('b.s45'), languageId: 'm65-asm', version: 1, text: fs.readFileSync(path.join(ws, 'b.s45'), 'utf-8') } } });
      await until(() => (diags.get(uri('a.c'))?.length ?? 0) > 0 && diags.has(uri('b.s45')));
      const ad = diags.get(uri('a.c'))!;
      assert.equal(ad.length, 1, JSON.stringify(ad));
      assert.equal(ad[0].range.start.line, 2);
      assert.deepEqual(diags.get(uri('b.s45')), []);
      assert.deepEqual(fs.readdirSync(ws).sort(), ['a.c', 'b.s45', 'h.h']);   // (no copies left)

      const hov = await req<{ contents: { value: string } }>('textDocument/hover', { textDocument: { uri: uri('b.s45') }, position: { line: 1, character: 5 } });
      assert.match(hov.contents.value, /\*\*LDA\*\*/);
      const comp = await req<{ label: string }[]>('textDocument/completion', { textDocument: { uri: uri('a.c') }, position: { line: 2, character: 27 } });
      assert.ok(comp.some((c) => c.label === 'helper'));
      const def = await req<{ uri: string; range: { start: { line: number } } }[]>('textDocument/definition', { textDocument: { uri: uri('a.c') }, position: { line: 2, character: 28 } });
      assert.ok(def.some((d) => d.uri === uri('a.c') && d.range.start.line === 1), JSON.stringify(def));

      // the outline, references, members of a library struct, the assembly
      const syms = await req<{ name: string }[]>('textDocument/documentSymbol', { textDocument: { uri: uri('a.c') } });
      assert.deepEqual(syms.map((x) => x.name), ['helper', 'main']);
      const refs = await req<{ uri: string }[]>('textDocument/references', { textDocument: { uri: uri('a.c') }, position: { line: 1, character: 5 }, context: { includeDeclaration: true } });
      assert.ok(refs.some((r) => r.uri === uri('h.h')) && refs.filter((r) => r.uri === uri('a.c')).length === 2, JSON.stringify(refs));
      const vtext = '#include <mega65.h>\nvoid f(void) { vic4->bor }\n';
      send({ jsonrpc: '2.0', method: 'textDocument/didOpen', params: { textDocument: { uri: uri('v.c'), languageId: 'c', version: 1, text: vtext } } });
      // (the library's headers are indexed after the workspace)
      let members: { label: string }[] = [];
      for (let i = 0; i < 50 && members.length === 0; i++) {
        members = await req<{ label: string }[]>('textDocument/completion', { textDocument: { uri: uri('v.c') }, position: { line: 1, character: 24 }, context: { triggerKind: 1 } });
        if (members.length === 0) await new Promise((r) => setTimeout(r, 100));
      }
      assert.ok(members.some((x) => x.label === 'border') && members.every((x) => x.label.startsWith('bor')), JSON.stringify(members));
      const vh = await req<{ contents: { value: string } }>('textDocument/hover', { textDocument: { uri: uri('v.c') }, position: { line: 1, character: 17 } });
      assert.match(vh.contents.value, /struct vic4_regs/);
      assert.match(vh.contents.value, /<mega65\.h>/);
      // (with the error: cc45's message; corrected: the assembly)
      const bad = await req<{ error?: string }>('m65/assembly', { uri: uri('a.c'), line: 1 });
      assert.match(bad.error ?? '', /Expected ';'/);
      send({ jsonrpc: '2.0', method: 'textDocument/didChange', params: { textDocument: { uri: uri('a.c'), version: 2 },
        contentChanges: [{ text: text.replace('helper(2) return', 'helper(2); return') }] } });
      const asm = await req<{ uri: string; line: number }>('m65/assembly', { uri: uri('a.c'), line: 1 });
      assert.ok(asm.uri && asm.uri.endsWith('/a.s45'), JSON.stringify(asm));
      const asmLines = fs.readFileSync(fileURLToPath(asm.uri), 'utf-8').split('\n');
      assert.match(asmLines[asm.line], /\.loc ".*a\.c", 2/);
      const back = await req<{ uri: string; line: number }>('m65/source', { uri: asm.uri, line: asm.line + 1 });
      assert.deepEqual(back, { uri: uri('a.c'), line: 1 });
    } finally {
      srv.kill();
      fs.rmSync(ws, { recursive: true, force: true });
    }
  });
});
