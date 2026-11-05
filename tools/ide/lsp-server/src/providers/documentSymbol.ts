import { DocumentSymbol, Range, SymbolKind as LspSymbolKind } from 'vscode-languageserver/node';
import { TextDocument } from 'vscode-languageserver-textdocument';
import { CSymbolScanner } from '../indexer/cSymbolScanner';
import { AsmSymbolScanner } from '../indexer/asmSymbolScanner';
import { Symbol } from '../indexer/symbolTypes';
import { isAsm } from './data';

// The outline of a document as it is being edited: C functions, prototypes,
// variables, structs and unions (their members inside), enums (their
// enumerators), typedefs, #defines; assembly procs (their labels inside),
// macros, labels and constants.

const before = (a: Range['start'], b: Range['start']) => a.line < b.line || (a.line === b.line && a.character <= b.character);

function toDocumentSymbol(s: Symbol): DocumentSymbol {
  const sel = s.location.range;
  let range: Range = s.fullRange ?? sel;
  // (the whole range must hold the name's)
  if (!before(range.start, sel.start) || !before(sel.end, range.end)) {
    range = {
      start: before(range.start, sel.start) ? range.start : sel.start,
      end: before(sel.end, range.end) ? range.end : sel.end,
    };
  }
  const d: DocumentSymbol = {
    name: s.name,
    kind: s.kind as unknown as LspSymbolKind,
    range,
    selectionRange: sel,
  };
  if (s.detail) d.detail = s.detail;
  if (s.children?.length) d.children = s.children.map(toDocumentSymbol);
  return d;
}

export class DocumentSymbolProvider {
  private c = new CSymbolScanner();
  private asm = new AsmSymbolScanner();

  onDocumentSymbol(document: TextDocument): DocumentSymbol[] {
    if (isAsm(document.languageId, document.uri)) {
      const all = this.asm.scanFile(document.uri, document.getText()).filter((s) => s.scope !== 'extern');
      const inner = new Set<Symbol>();
      for (const s of all) for (const c of s.children ?? []) inner.add(c);
      return all.filter((s) => !inner.has(s)).map(toDocumentSymbol);
    }
    return this.c.scan(document.uri, document.getText()).symbols.map(toDocumentSymbol);
  }
}
