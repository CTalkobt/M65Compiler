import * as fs from 'fs';
import * as path from 'path';

// The tables in data/ (scripts/gen-data.mjs makes all but c-keywords.json)

export interface KeywordInfo { name: string; type: string; description: string }
export interface OpcodeMode { mode: string; opcode: number; bytes: string; cycles40: number | null }
export interface OpcodeInfo {
  mnemonic: string; description: string; flags: string[]; modes: OpcodeMode[];
  simulated?: boolean; syntax?: string;
}
export interface DirectiveInfo { name: string; description: string; syntax: string; example?: string }
export interface FunctionInfo { name: string; header: string; signature: string; doc: string }
export interface PragmaInfo { name: string; syntax: string; description: string }

export interface ReferenceData {
  keywords: Map<string, KeywordInfo>;
  opcodes: Map<string, OpcodeInfo>;       // lower-case mnemonic
  directives: Map<string, DirectiveInfo>; // lower-case, without the dot
  functions: Map<string, FunctionInfo>;
  pragmas: PragmaInfo[];
}

function load<T>(dir: string, file: string, key: string): T[] {
  try {
    const p = path.join(dir, file);
    if (!fs.existsSync(p)) return [];
    const v = JSON.parse(fs.readFileSync(p, 'utf-8'))[key];
    return Array.isArray(v) ? v : [];
  } catch (err) {
    console.error(`[data] ${file}: ${err}`);
    return [];
  }
}

/** data/ beside the server (dist/data in the extension, ../data in the package) */
export function findDataDir(serverDir: string): string {
  for (const d of [path.join(serverDir, 'data'), path.join(serverDir, '..', 'data')]) {
    if (fs.existsSync(path.join(d, 'opcodes.json'))) return d;
  }
  return path.join(serverDir, '..', 'data');
}

export function loadReferenceData(dir: string): ReferenceData {
  const data: ReferenceData = {
    keywords: new Map(), opcodes: new Map(), directives: new Map(), functions: new Map(), pragmas: [],
  };
  for (const k of load<KeywordInfo>(dir, 'c-keywords.json', 'keywords')) data.keywords.set(k.name, k);
  for (const o of load<OpcodeInfo>(dir, 'opcodes.json', 'opcodes')) data.opcodes.set(o.mnemonic.toLowerCase(), o);
  for (const d of load<DirectiveInfo>(dir, 'directives.json', 'directives')) data.directives.set(d.name.toLowerCase(), d);
  for (const f of load<FunctionInfo>(dir, 'stdlib-functions.json', 'functions')) data.functions.set(f.name, f);
  data.pragmas = load<PragmaInfo>(dir, 'pragmas.json', 'pragmas');
  console.error(`[data] ${data.keywords.size} keywords, ${data.opcodes.size} instructions, ${data.directives.size} directives, ` +
    `${data.functions.size} library functions, ${data.pragmas.length} pragmas`);
  return data;
}

export function isAsm(languageId: string, uri: string): boolean {
  return languageId === 'm65-asm' || /\.(s45|asm|inc|s)$/i.test(uri);
}

/** The word at a column: identifiers, .directives, @local labels, add.16 */
export function wordAt(line: string, column: number): { word: string; start: number; end: number } | null {
  const ch = /[A-Za-z0-9_.@]/;
  let start = Math.min(column, line.length);
  let end = start;
  while (start > 0 && ch.test(line[start - 1])) start--;
  while (end < line.length && ch.test(line[end])) end++;
  if (start === end) return null;
  return { word: line.substring(start, end), start, end };
}

export function opcodeMarkdown(o: OpcodeInfo): string {
  let s = `**${o.mnemonic}**${o.simulated ? ' (ca45 simulated instruction)' : ''} — ${o.description}`;
  if (o.syntax) s += `\n\n\`${o.syntax}\``;
  if (o.flags.length) s += `\n\nFlags: ${o.flags.join(' ')}`;
  if (o.modes.length) {
    s += '\n\n| mode | bytes | cycles (40 MHz) |\n|---|---|---|\n';
    s += o.modes.map((m) => `| ${m.mode || '(implied)'} | ${m.bytes} | ${m.cycles40 ?? ''} |`).join('\n');
  }
  return s;
}
