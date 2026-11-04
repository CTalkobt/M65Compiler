import { Hover, HoverParams, MarkupKind } from 'vscode-languageserver/node';
import { TextDocument } from 'vscode-languageserver-textdocument';
import * as fs from 'fs';
import { ReferenceData, isAsm, opcodeMarkdown, wordAt } from './data';
import { WorkspaceIndex } from '../indexer/workspaceIndex';

export class HoverProvider {
  constructor(private data: ReferenceData, private index: WorkspaceIndex) {}

  onHover(params: HoverParams, document: TextDocument): Hover | null {
    const { position } = params;
    const line = document.getText({ start: { line: position.line, character: 0 }, end: { line: position.line + 1, character: 0 } })
      .replace(/\r?\n$/, '');
    const w = wordAt(line, position.character);
    if (!w) return null;
    const range = { start: { line: position.line, character: w.start }, end: { line: position.line, character: w.end } };
    const md = isAsm(document.languageId, document.uri) ? this.asm(w.word) : this.c(line, w.word);
    const text = md ?? this.symbol(w.word, document.uri);
    if (!text) return null;
    return { contents: { kind: MarkupKind.Markdown, value: text }, range };
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

  /** A symbol of the workspace: its defining line */
  private symbol(word: string, uri: string): string | null {
    const defs = this.index.findDefinitions(word);
    if (defs.length === 0) return null;
    const d = defs.find((s) => s.location.uri === uri) ?? defs[0];
    let text = '';
    try {
      const lines = fs.readFileSync(WorkspaceIndex.uriToPath(d.location.uri), 'utf-8').split('\n');
      text = (lines[d.location.range.start.line] || '').trim();
    } catch { /* not on disk */ }
    const file = decodeURIComponent(d.location.uri.split('/').pop() || '');
    return `${text ? '```\n' + text + '\n```\n' : `**${word}**\n\n`}${file}:${d.location.range.start.line + 1}` +
      (defs.length > 1 ? ` (and ${defs.length - 1} more)` : '');
  }
}
