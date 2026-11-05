import { Hover, HoverParams, MarkupKind } from 'vscode-languageserver/node';
import { TextDocument } from 'vscode-languageserver-textdocument';
import * as fs from 'fs';
import { ReferenceData, identAt, isAsm, opcodeMarkdown, wordAt } from './data';
import { WorkspaceIndex } from '../indexer/workspaceIndex';
import { CSymbolScanner } from '../indexer/cSymbolScanner';
import { Symbol } from '../indexer/symbolTypes';
import { cleanC } from '../indexer/text';
import { TypeResolver } from './cTypes';

export class HoverProvider {
  private scanner = new CSymbolScanner();

  constructor(private data: ReferenceData, private index: WorkspaceIndex,
    private open: (uri: string) => TextDocument | undefined = () => undefined) {}

  onHover(params: HoverParams, document: TextDocument): Hover | null {
    const { position } = params;
    const line = document.getText({ start: { line: position.line, character: 0 }, end: { line: position.line + 1, character: 0 } })
      .replace(/\r?\n$/, '');
    const asm = isAsm(document.languageId, document.uri);
    const w = wordAt(line, position.character);
    if (!w) return null;
    const range = { start: { line: position.line, character: w.start }, end: { line: position.line, character: w.end } };
    let md: string | null = asm ? this.asm(w.word) : this.c(line, w.word);
    if (!md && !asm) md = this.member(document, position, line);
    if (!md) {
      const id = identAt(line, position.character, asm);
      if (id) md = this.symbol(id, document.uri);
    }
    if (!md) return null;
    return { contents: { kind: MarkupKind.Markdown, value: md }, range };
  }

  private asm(word: string): string | null {
    const lw = word.toLowerCase();
    if (lw.startsWith('.')) {
      const d = this.data.directives.get(lw.slice(1));
      if (d) return `**${d.syntax}**\n\n${d.description}`;
      return null;
    }
    const o = this.data.opcodes.get(lw) ?? this.data.opcodes.get(lw.replace(/\.(abs|zp|imm|ind|xind|indy|indz|accum|a)$/, ''));
    return o ? opcodeMarkdown(o) : null;
  }

  private c(line: string, word: string): string | null {
    const pragma = /^\s*#\s*pragma\s+((?:cc45\s+)?[A-Za-z_0-9]+)/.exec(line);
    if (pragma) {
      const name = pragma[1].replace(/\s+/g, ' ');
      const p = this.data.pragmas.find((x) => x.name === name || x.name === `cc45 ${name}`);
      if (p && name.endsWith(word)) return `**${p.syntax}**\n\n${p.description}`;
    }
    const k = this.data.keywords.get(word);
    if (k) return `**${k.name}** (${k.type})\n\n${k.description}`;
    const f = this.data.functions.get(word);
    if (f) return `\`\`\`c\n${f.signature}\n\`\`\`\n${f.doc ? f.doc + '\n\n' : ''}<${f.header}>`;
    return null;
  }

  /** x.name / p->name: the member's declaration */
  private member(document: TextDocument, position: { line: number; character: number }, line: string): string | null {
    let s = Math.min(position.character, line.length);
    let e = s;
    while (s > 0 && /\w/.test(line[s - 1])) s--;
    while (e < line.length && /\w/.test(line[e])) e++;
    if (s === e) return null;
    const name = line.slice(s, e);
    const text = document.getText();
    const start = document.offsetAt({ line: position.line, character: s });
    const r = new TypeResolver(this.index, this.scanner.scan(document.uri, text).types).memberAt(cleanC(text), start, name);
    if (!r) return null;
    const file = decodeURIComponent(r.agg.uri.split('/').pop() || '');
    return `\`\`\`c\n${r.member.decl}\n\`\`\`\n${r.member.method ? 'method' : 'member'} of ${r.agg.kind} ${r.agg.tag} (${file}:${r.member.line + 1})`;
  }

  /** A symbol of the workspace or the library's headers: its declaration and the comment above it */
  private symbol(word: string, uri: string): string | null {
    const defs = this.index.findDefinitions(word);
    if (defs.length === 0) return null;
    const d = defs.find((s) => s.location.uri === uri) ?? defs[0];
    const lines = this.lines(d.location.uri);
    const at = d.location.range.start.line;
    const text = d.detail ?? (lines[at] || '').trim();
    const comment = lines.length ? HoverProvider.commentAbove(lines, HoverProvider.declStart(d, lines)) : '';
    const file = decodeURIComponent(d.location.uri.split('/').pop() || '');
    const where = d.library ? `<${file}>` : `${file}:${at + 1}`;
    return `${text ? '```' + (isAsm('', d.location.uri) ? '' : 'c') + '\n' + text + '\n```\n' : `**${word}**\n\n`}` +
      `${comment ? comment + '\n\n' : ''}${where}` + (defs.length > 1 ? ` (and ${defs.length - 1} more)` : '');
  }

  private lines(uri: string): string[] {
    const doc = this.open(uri);
    if (doc) return doc.getText().split('\n');
    try {
      return fs.readFileSync(WorkspaceIndex.uriToPath(uri), 'utf-8').split('\n');
    } catch {
      return [];
    }
  }

  private static declStart(d: Symbol, lines: string[]): number {
    const l = d.fullRange?.start.line ?? d.location.range.start.line;
    return Math.min(l, d.location.range.start.line, lines.length);
  }

  /** the // or block comment just above a line (or at its end), as text */
  static commentAbove(lines: string[], line: number): string {
    const out: string[] = [];
    const trail = /\/\/\s?(.*)$|\/\*\s*(.*?)\s*\*\/\s*$/.exec(lines[line] ?? '');
    let i = line - 1;
    if (i >= 0 && /\*\/\s*$/.test(lines[i])) {
      const end = i;
      while (i >= 0 && !/\/\*/.test(lines[i])) i--;
      if (i < 0) return '';
      for (let k = i; k <= end; k++) {
        out.push(lines[k].replace(/^\s*\/\*+\s?/, '').replace(/\s*\*+\/\s*$/, '').replace(/^\s*\*\s?/, ''));
      }
    } else {
      while (i >= 0 && /^\s*\/\//.test(lines[i])) i--;
      for (let k = i + 1; k < line; k++) out.push(lines[k].replace(/^\s*\/\/+\s?/, ''));
    }
    const text = out.join('\n').trim();
    if (text) return text;
    return trail ? (trail[1] ?? trail[2] ?? '').trim() : '';
  }
}
