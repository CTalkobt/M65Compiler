import { execFile, ExecFileException } from 'child_process';
import * as fs from 'fs';
import * as os from 'os';
import * as path from 'path';
import { parseDiagnostics } from './parseDiagnostics';
import { ParsedDiagnostic } from './diagnosticTypes';

export const TIMEOUT_MS = 10000;

export type ToolKind = 'c' | 'asm';

export interface ToolRun {
  /** the tool's diagnostics for the document (line/column 1-based) */
  diagnostics: ParsedDiagnostic[];
}

function problem(file: string, message: string): ParsedDiagnostic {
  return { file, line: 1, column: 1, severity: 'error', message };
}

function exec(tool: string, args: string[], cwd: string): Promise<{ code: number | null; out: string; stdout?: string; err?: ExecFileException }> {
  return new Promise((resolve) => {
    execFile(tool, args, { cwd, timeout: TIMEOUT_MS, maxBuffer: 4 * 1024 * 1024, encoding: 'utf-8' },
      (err, stdout, stderr) => {
        const e = err as (ExecFileException & { code?: number | string }) | null;
        if (e && (e.killed || typeof e.code === 'string')) {
          resolve({ code: null, out: `${stderr}${stdout}`, err: e });
          return;
        }
        resolve({ code: e ? (typeof e.code === 'number' ? e.code : 1) : 0, out: `${stderr}\n${stdout}`, stdout });
      });
  });
}

/**
 * The diagnostics of a document's text (not only what is saved): cc45 -c or
 * ca45 -c on a copy of it.
 *
 * - C: the copy in a directory of its own, with -I the document's directory
 *   (its "..." includes still found);
 * - assembly: the document itself when the text is what is on disk, else a
 *   hidden copy beside it (.include is found only in the file's directory).
 *
 * Diagnostics in the copy are the document's; ones in other files (a header)
 * are put on its first line with their file and line.
 */
export async function runTool(kind: ToolKind, toolPath: string, filePath: string, text: string): Promise<ToolRun> {
  const name = kind === 'c' ? 'cc45' : 'ca45';
  if (!toolPath || !fs.existsSync(toolPath)) {
    return {
      diagnostics: [problem(filePath, `${name} not found${toolPath ? ` at ${toolPath}` : ''}. Set m65.${name}Path in the settings.`)],
    };
  }
  const dir = path.dirname(filePath);
  const base = path.basename(filePath);
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'm65lsp-'));
  let src = filePath;
  let sibling: string | null = null;
  try {
    const onDisk = fs.existsSync(filePath) ? fs.readFileSync(filePath, 'utf-8') : null;
    if (onDisk !== text) {
      if (kind === 'c') {
        src = path.join(tmp, base);
      } else {
        sibling = path.join(dir, `.${base}.m65lsp${path.extname(base) || '.s45'}`);
        src = sibling;
      }
      fs.writeFileSync(src, text);
    }
    const obj = path.join(tmp, 'out.o45');
    const args = kind === 'c' ? ['-c', src, '-o', obj, '-I', dir] : ['-c', src, '-o', obj];
    const r = await exec(toolPath, args, fs.existsSync(dir) ? dir : tmp);
    if (r.err) {
      const msg = r.err.code === 'ETIMEDOUT' || (r.err as { killed?: boolean }).killed
        ? `${name} timed out after ${TIMEOUT_MS} ms` : `could not run ${name}: ${r.err.message}`;
      return { diagnostics: [problem(filePath, msg)] };
    }
    const parsed = parseDiagnostics(r.out);
    const mine = (f: string) => f === '' || f === src || f === filePath ||
      path.resolve(dir, f) === path.resolve(src) || path.resolve(dir, f) === path.resolve(filePath) ||
      path.basename(f) === path.basename(src);
    const diagnostics: ParsedDiagnostic[] = [];
    for (const d of parsed) {
      if (mine(d.file)) {
        diagnostics.push({ ...d, file: filePath });
      } else if (d.file !== 'unknown' && /\.(c|h|s45|s|asm|inc)$/i.test(d.file)) {
        diagnostics.push({ ...d, file: filePath, line: 1, column: 1, message: `${path.basename(d.file)}:${d.line}: ${d.message}` });
      } else {
        diagnostics.push({ ...d, file: filePath, line: 1, column: 1 });
      }
    }
    if (diagnostics.length === 0 && r.code !== 0) {
      diagnostics.push(problem(filePath, `${name} failed (exit status ${r.code})`));
    }
    return { diagnostics };
  } finally {
    if (sibling) {
      try { fs.unlinkSync(sibling); } catch { /* gone */ }
    }
    try { fs.rmSync(tmp, { recursive: true, force: true }); } catch { /* left */ }
  }
}

/** cl45 / al45's findings as they print them with --format=json */
export function parseLintJson(out: string): ParsedDiagnostic[] {
  let list: unknown;
  try { list = JSON.parse(out); } catch { return []; }
  if (!Array.isArray(list)) return [];
  const found: ParsedDiagnostic[] = [];
  for (const f of list as Record<string, unknown>[]) {
    if (!f || typeof f.message !== 'string' || typeof f.rule !== 'string') continue;
    const sev = f.severity === 'error' ? 'error' : f.severity === 'warning' ? 'warning' : 'note';
    found.push({
      file: typeof f.file === 'string' ? f.file : '',
      line: typeof f.line === 'number' ? f.line : 1,
      column: typeof f.column === 'number' && f.column > 0 ? f.column : 1,
      severity: sev,
      message: f.message,
      rule: f.rule,
    });
  }
  return found;
}

/**
 * The linter's findings on a document's text (#199): cl45 or al45 with
 * --format=json. The text when it is not what is on disk is linted from a
 * hidden copy beside the document, so its .cc45-lint.yml (searched upward
 * from the file), "..." includes and .include files are the document's.
 *
 * A file the compiler rejects has no findings (its errors are the
 * compiler's); findings in other files (a header) are left to that file.
 * A linter that is not there gives none: the server works without them.
 */
export async function runLint(kind: ToolKind, toolPath: string, filePath: string, text: string): Promise<ToolRun> {
  if (!toolPath || !fs.existsSync(toolPath)) return { diagnostics: [] };
  const dir = path.dirname(filePath);
  if (!fs.existsSync(dir)) return { diagnostics: [] };
  const base = path.basename(filePath);
  let src = filePath;
  let sibling: string | null = null;
  try {
    const onDisk = fs.existsSync(filePath) ? fs.readFileSync(filePath, 'utf-8') : null;
    if (onDisk !== text) {
      sibling = path.join(dir, `.${base}.m65lsp.lint${path.extname(base) || (kind === 'c' ? '.c' : '.s45')}`);
      src = sibling;
      fs.writeFileSync(src, text);
    }
    const r = await exec(toolPath, ['--format=json', src], dir);
    if (r.err || r.stdout === undefined) return { diagnostics: [] };
    const diagnostics: ParsedDiagnostic[] = [];
    for (const d of parseLintJson(r.stdout)) {
      const f = path.resolve(dir, d.file);
      if (d.file === '' || f === path.resolve(src) || f === path.resolve(filePath)) diagnostics.push({ ...d, file: filePath });
    }
    return { diagnostics };
  } finally {
    if (sibling) {
      try { fs.unlinkSync(sibling); } catch { /* gone */ }
    }
  }
}

export interface AssemblyRun {
  /** the assembly file (cc45 -S), its .loc lines naming the document */
  asmPath?: string;
  error?: string;
}

/**
 * The document's text as cc45's assembly (-S, the standalone form), for
 * reading beside the C: written to <tmp>/m65-lsp-asm/<name>.s45, the .loc
 * lines naming the document (not the copy compiled).
 */
export async function compileToAssembly(toolPath: string, filePath: string, text: string): Promise<AssemblyRun> {
  if (!toolPath || !fs.existsSync(toolPath)) return { error: `cc45 not found${toolPath ? ` at ${toolPath}` : ''}` };
  const dir = path.dirname(filePath);
  const base = path.basename(filePath);
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'm65lsp-'));
  const outDir = path.join(os.tmpdir(), 'm65-lsp-asm');
  try {
    fs.mkdirSync(outDir, { recursive: true });
    const src = path.join(tmp, base);
    fs.writeFileSync(src, text);
    const out = path.join(outDir, base.replace(/\.[^.]*$/, '') + '.s45');
    const r = await exec(toolPath, ['-S', src, '-o', out, '-I', dir], fs.existsSync(dir) ? dir : tmp);
    if (r.err) return { error: `could not run cc45: ${r.err.message}` };
    if (r.code !== 0 || !fs.existsSync(out)) {
      const first = parseDiagnostics(r.out)[0];
      return { error: first ? `cc45: ${first.line}:${first.column}: ${first.message}` : `cc45 failed (exit status ${r.code})` };
    }
    const asm = fs.readFileSync(out, 'utf-8').split(`"${src}"`).join(`"${filePath}"`);
    fs.writeFileSync(out, asm);
    return { asmPath: out };
  } finally {
    try { fs.rmSync(tmp, { recursive: true, force: true }); } catch { /* left */ }
  }
}

/** the assembly line of a C line: its .loc (that line, else the next one with code, else the last before) */
export function asmLineFor(asm: string, filePath: string, cLine: number): number {
  const lines = asm.split('\n');
  let after: { asm: number; c: number } | null = null;
  let before: { asm: number; c: number } | null = null;
  for (let i = 0; i < lines.length; i++) {
    const m = /^\s*\.loc\s+"([^"]+)"\s*,\s*(\d+)/.exec(lines[i]);
    if (!m || path.resolve(m[1]) !== path.resolve(filePath)) continue;
    const c = parseInt(m[2], 10) - 1;
    if (c === cLine) return i;
    if (c > cLine && (!after || c < after.c)) after = { asm: i, c };
    if (c < cLine && (!before || c > before.c)) before = { asm: i, c };
  }
  return after?.asm ?? before?.asm ?? 0;
}
