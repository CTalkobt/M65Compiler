import { strict as assert } from 'assert';
import { test, describe } from 'node:test';
import { spawn } from 'child_process';
import * as fs from 'fs';
import * as os from 'os';
import * as path from 'path';
import { pathToFileURL } from 'url';
import { parseDiagnostics } from './diagnostics/parseDiagnostics';
import { WorkspaceIndex } from './indexer/workspaceIndex';
import { resolveTool } from './config';
import { loadReferenceData, wordAt } from './providers/data';

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
    } finally {
      srv.kill();
      fs.rmSync(ws, { recursive: true, force: true });
    }
  });
});
