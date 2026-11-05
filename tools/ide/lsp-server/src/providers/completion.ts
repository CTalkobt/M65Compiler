import {
  CompletionItem,
  CompletionItemKind,
  CompletionParams,
  InsertTextFormat,
  MarkupKind,
} from 'vscode-languageserver/node';
import { TextDocument } from 'vscode-languageserver-textdocument';
import { ReferenceData, isAsm, opcodeMarkdown } from './data';
import { WorkspaceIndex } from '../indexer/workspaceIndex';
import { SymbolKind } from '../indexer/symbolTypes';
import { CSymbolScanner } from '../indexer/cSymbolScanner';
import { cleanC } from '../indexer/text';
import { TypeResolver } from './cTypes';

const MAX_ITEMS = 200;

export class CompletionProvider {
  private cItems: CompletionItem[] = [];
  private asmItems: CompletionItem[] = [];
  private directiveItems: CompletionItem[] = [];
  private pragmaItems: CompletionItem[] = [];
  private scanner = new CSymbolScanner();

  constructor(data: ReferenceData, private index: WorkspaceIndex) {
    for (const k of data.keywords.values()) {
      this.cItems.push({
        label: k.name,
        kind: k.type === 'type' ? CompletionItemKind.TypeParameter : CompletionItemKind.Keyword,
        detail: k.type,
        documentation: k.description,
      });
    }
    for (const f of data.functions.values()) {
      this.cItems.push({
        label: f.name,
        kind: CompletionItemKind.Function,
        detail: f.signature,
        documentation: { kind: MarkupKind.Markdown, value: `${f.doc ? f.doc + '\n\n' : ''}<${f.header}>` },
      });
    }
    for (const o of data.opcodes.values()) {
      this.asmItems.push({
        label: o.mnemonic.toLowerCase(),
        kind: o.simulated ? CompletionItemKind.Snippet : CompletionItemKind.Keyword,
        detail: o.description.length > 80 ? o.description.slice(0, 77) + '...' : o.description,
        documentation: { kind: MarkupKind.Markdown, value: opcodeMarkdown(o) },
      });
    }
    for (const d of data.directives.values()) {
      this.directiveItems.push({
        label: `.${d.name}`,
        filterText: `.${d.name}`,
        insertText: d.name,
        kind: CompletionItemKind.Keyword,
        detail: d.syntax,
        documentation: d.description,
      });
    }
    for (const p of data.pragmas) {
      this.pragmaItems.push({
        label: p.name,
        kind: CompletionItemKind.Keyword,
        detail: p.syntax,
        documentation: p.description,
        insertTextFormat: InsertTextFormat.PlainText,
      });
    }
  }

  onCompletion(params: CompletionParams, document: TextDocument): CompletionItem[] {
    const { position } = params;
    const line = document.getText({ start: { line: position.line, character: 0 }, end: position });
    const prefix = /[A-Za-z0-9_.@]*$/.exec(line)?.[0] ?? '';
    let items: CompletionItem[];

    if (isAsm(document.languageId, document.uri)) {
      if (prefix.startsWith('.')) {
        // (the dot typed: the directive's name after it)
        items = this.directiveItems;
        return this.filter(items, prefix);
      }
      const atStart = /^\s*([A-Za-z_@][\w.]*:\s*)?[A-Za-z0-9_.]*$/.test(line);
      items = atStart ? [...this.asmItems, ...this.symbols(document.uri, true)] : this.symbols(document.uri, true);
    } else {
      const pragma = /^\s*#\s*pragma\s+([\w ]*)$/.exec(line);
      if (pragma) {
        const typed = pragma[1];
        return this.pragmaItems
          .filter((p) => p.label.startsWith(typed.trimStart()) || p.label.startsWith(`cc45 ${typed.trimStart()}`))
          .map((p) => ({ ...p, insertText: typed.includes(' ') ? p.label.slice(p.label.lastIndexOf(' ') + 1) : p.label }));
      }
      if (/^\s*#\s*\w*$/.test(line)) return [];
      // x. / p-> : the struct's members
      const text = document.getText();
      const clean = cleanC(text);
      const offset = document.offsetAt(position);
      if (/(\.|->)\s*[A-Za-z_]*\w*$/.test(clean.slice(Math.max(0, offset - 64), offset))) {
        const types = this.scanner.scan(document.uri, text).types;
        const site = new TypeResolver(this.index, types).memberSite(clean, offset);
        if (site) {
          return this.filter(site.agg.members.map((m) => ({
            label: m.name,
            kind: m.method ? CompletionItemKind.Method : CompletionItemKind.Field,
            detail: m.decl,
            documentation: `${site.agg.kind} ${site.agg.tag}`,
          })), site.prefix, false);
        }
        // (a . or -> whose struct is not known: nothing rather than everything)
        if (params.context?.triggerCharacter === '.' || params.context?.triggerCharacter === '>') return [];
      }
      if (params.context?.triggerCharacter === '>') return [];
      items = [...this.cItems, ...this.symbols(document.uri, false)];
    }
    return this.filter(items, prefix);
  }

  private filter(items: CompletionItem[], prefix: string, sort = true): CompletionItem[] {
    const p = prefix.toLowerCase();
    const seen = new Set<string>();
    const out: CompletionItem[] = [];
    for (const it of items) {
      const key = it.label.toLowerCase();
      if ((p && !key.startsWith(p)) || seen.has(key)) continue;
      seen.add(key);
      out.push(it);
    }
    if (sort) out.sort((a, b) => a.label.localeCompare(b.label));
    return out.slice(0, MAX_ITEMS);
  }

  /** The workspace's symbols of the language (this file's first) */
  private symbols(uri: string, asm: boolean): CompletionItem[] {
    const ext = asm ? /\.(s45|asm|inc|s)$/i : /\.(c|h)$/i;
    const out: CompletionItem[] = [];
    for (const s of this.index.getAllSymbols()) {
      if (!s.isDefinition || !ext.test(s.location.uri)) continue;
      if (s.scope === 'local' && s.location.uri !== uri) continue;
      const file = decodeURIComponent(s.location.uri.split('/').pop() || '');
      out.push({
        label: s.name,
        kind: s.kind === SymbolKind.Function ? CompletionItemKind.Function
          : s.kind === SymbolKind.Struct ? CompletionItemKind.Struct
          : s.kind === SymbolKind.Enum ? CompletionItemKind.Enum
          : s.kind === SymbolKind.EnumMember ? CompletionItemKind.EnumMember
          : s.kind === SymbolKind.TypeParameter ? CompletionItemKind.TypeParameter
          : s.kind === SymbolKind.Constant ? CompletionItemKind.Constant
          : CompletionItemKind.Variable,
        detail: s.detail ? `${s.detail}  (${s.library ? '<' + file + '>' : file})` : file,
        sortText: (s.location.uri === uri ? '0' : s.library ? '2' : '1') + s.name,
      });
    }
    return out;
  }
}
