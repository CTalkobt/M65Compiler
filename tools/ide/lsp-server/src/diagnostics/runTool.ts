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

function exec(tool: string, args: string[], cwd: string): Promise<{ code: number | null; out: string; err?: ExecFileException }> {
  return new Promise((resolve) => {
    execFile(tool, args, { cwd, timeout: TIMEOUT_MS, maxBuffer: 4 * 1024 * 1024, encoding: 'utf-8' },
      (err, stdout, stderr) => {
        const e = err as (ExecFileException & { code?: number | string }) | null;
        if (e && (e.killed || typeof e.code === 'string')) {
          resolve({ code: null, out: `${stderr}${stdout}`, err: e });
          return;
        }
        resolve({ code: e ? (typeof e.code === 'number' ? e.code : 1) : 0, out: `${stderr}\n${stdout}` });
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
