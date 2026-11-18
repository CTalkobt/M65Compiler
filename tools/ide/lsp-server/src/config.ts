import * as fs from 'fs';
import * as path from 'path';

/** The m65.* settings (VS Code's names; other editors send the same object) */
export interface M65Settings {
  cc45Path?: string;
  ca45Path?: string;
  cl45Path?: string;
  al45Path?: string;
  lint?: { enabled?: boolean };
  diagnostics?: { debounceMs?: number };
  indexer?: { exclude?: string[] };
}

export interface ResolvedSettings {
  cc45Path: string;
  ca45Path: string;
  /** the linters (#199): cl45 beside cc45, al45 beside ca45 unless set */
  cl45Path: string;
  al45Path: string;
  lint: boolean;
  debounceMs: number;
  exclude: string[];
}

export const DEFAULT_EXCLUDE = ['node_modules', '.git', 'bin', 'obj', 'build'];

function onPath(name: string): string | null {
  for (const d of (process.env.PATH || '').split(path.delimiter)) {
    if (!d) continue;
    const p = path.join(d, name);
    try {
      if (fs.statSync(p).isFile()) return p;
    } catch { /* not here */ }
  }
  return null;
}

/**
 * A tool's path: the setting (${workspaceFolder} and ~ expanded, relative to
 * the workspace), else the workspace's bin/<tool>, else <tool> on the PATH.
 */
export function resolveTool(setting: string | undefined, tool: string, workspace: string): string {
  const expand = (p: string) => {
    let s = p.replace(/\$\{workspaceFolder\}|\$\{workspaceRoot\}/g, workspace);
    if (s.startsWith('~/')) s = path.join(process.env.HOME || '', s.slice(2));
    return path.isAbsolute(s) ? s : path.join(workspace, s);
  };
  if (setting && setting.trim()) {
    const p = expand(setting.trim());
    if (fs.existsSync(p)) return p;
    // (the default setting names the workspace's bin/; a workspace without
    // one uses the installed tool)
    if (!/\$\{workspace(Folder|Root)\}\/bin\//.test(setting) && setting.trim() !== `bin/${tool}`) return p;
  }
  const local = path.join(workspace, 'bin', tool);
  if (fs.existsSync(local)) return local;
  return onPath(tool) || local;
}

/** A linter's path: the setting, else beside its compiler (they are links to it), else as any tool */
function resolveLinter(setting: string | undefined, tool: string, beside: string, workspace: string): string {
  if (setting && setting.trim()) return resolveTool(setting, tool, workspace);
  const p = path.join(path.dirname(beside), tool);
  return fs.existsSync(p) ? p : resolveTool(undefined, tool, workspace);
}

export function resolveSettings(s: M65Settings | undefined | null, workspace: string): ResolvedSettings {
  const d = s?.diagnostics?.debounceMs;
  const cc45Path = resolveTool(s?.cc45Path, 'cc45', workspace);
  const ca45Path = resolveTool(s?.ca45Path, 'ca45', workspace);
  return {
    cc45Path,
    ca45Path,
    cl45Path: resolveLinter(s?.cl45Path, 'cl45', cc45Path, workspace),
    al45Path: resolveLinter(s?.al45Path, 'al45', ca45Path, workspace),
    lint: s?.lint?.enabled !== false,
    debounceMs: typeof d === 'number' && d >= 0 ? d : 500,
    exclude: Array.isArray(s?.indexer?.exclude) ? s!.indexer!.exclude! : DEFAULT_EXCLUDE,
  };
}
