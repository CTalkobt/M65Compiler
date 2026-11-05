import { Location, ReferenceParams } from 'vscode-languageserver/node';
import { TextDocument } from 'vscode-languageserver-textdocument';
import * as fs from 'fs';
import { WorkspaceIndex } from '../indexer/workspaceIndex';
import { cleanAsm, cleanC, escapeRegex, Positions } from '../indexer/text';
import { identAt, isAsm } from './data';

// References by name: the whole word in the files that use it (the index
// knows each file's identifiers), outside comments and string literals.
// An assembly @local label only in its own file. Not by scope: a local
// variable's name matches the same name elsewhere.

const MAX = 5000;

export class ReferencesProvider {
  constructor(private index: WorkspaceIndex, private open: (uri: string) => TextDocument | undefined) {}

  onReferences(params: ReferenceParams, document: TextDocument): Location[] {
    const p = params.position;
    const line = document.getText({ start: { line: p.line, character: 0 }, end: { line: p.line + 1, character: 0 } });
    const word = identAt(line.replace(/\r?\n$/, ''), p.character, isAsm(document.languageId, document.uri));
    if (!word) return [];
    const uris = word.startsWith('@') ? [document.uri] : Array.from(new Set([document.uri, ...this.index.filesWithWord(word)]));
    const re = new RegExp(`(?<![\\w@])${escapeRegex(word)}(?!\\w)`, 'g');
    const defs = params.context.includeDeclaration ? null
      : new Set(this.index.findDefinitions(word).map((d) => `${d.location.uri}:${d.location.range.start.line}:${d.location.range.start.character}`));
    const out: Location[] = [];
    for (const uri of uris) {
      let text: string;
      const doc = this.open(uri) ?? (uri === document.uri ? document : undefined);
      if (doc) text = doc.getText();
      else {
        try { text = fs.readFileSync(WorkspaceIndex.uriToPath(uri), 'utf-8'); } catch { continue; }
      }
      const clean = isAsm('', uri) ? cleanAsm(text) : cleanC(text);
      const pos = new Positions(text);
      for (const m of clean.matchAll(re)) {
        const s = pos.at(m.index ?? 0);
        if (defs?.has(`${uri}:${s.line}:${s.character}`)) continue;
        out.push({ uri, range: { start: s, end: { line: s.line, character: s.character + word.length } } });
        if (out.length >= MAX) return out;
      }
    }
    return out;
  }
}
