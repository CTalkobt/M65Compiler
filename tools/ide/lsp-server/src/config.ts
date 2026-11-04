import * as fs from 'fs';
import * as path from 'path';

/** The m65.* settings (VS Code's names; other editors send the same object) */
export interface M65Settings {
  cc45Path?: string;
  ca45Path?: string;
  diagnostics?: { debounceMs?: number };
  indexer?: { exclude?: string[] };
}

export interface ResolvedSettings {
  cc45Path: string;
  ca45Path: string;
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

export function resolveSettings(s: M65Settings | undefined | null, workspace: string): ResolvedSettings {
  const d = s?.diagnostics?.debounceMs;
  return {
    cc45Path: resolveTool(s?.cc45Path, 'cc45', workspace),
    ca45Path: resolveTool(s?.ca45Path, 'ca45', workspace),
    debounceMs: typeof d === 'number' && d >= 0 ? d : 500,
    exclude: Array.isArray(s?.indexer?.exclude) ? s!.indexer!.exclude! : DEFAULT_EXCLUDE,
  };
}
