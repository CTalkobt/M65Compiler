import { DefinitionParams, Location } from 'vscode-languageserver/node';
import { TextDocument } from 'vscode-languageserver-textdocument';
import { pathToFileURL } from 'url';
import * as path from 'path';
import { WorkspaceIndex } from '../indexer/workspaceIndex';
import { CSymbolScanner } from '../indexer/cSymbolScanner';
import { cleanC } from '../indexer/text';
import { identAt, isAsm } from './data';
import { TypeResolver } from './cTypes';

/** a .loc "file", line directive (cc45's assembly): the C source's place */
export function locTarget(line: string, asmPath: string): Location | null {
  const m = /^\s*\.loc\s+"([^"]+)"\s*,\s*(\d+)/.exec(line);
  if (!m) return null;
  const file = path.isAbsolute(m[1]) ? m[1] : path.resolve(path.dirname(asmPath), m[1]);
  const l = Math.max(0, parseInt(m[2], 10) - 1);
  return { uri: pathToFileURL(file).toString(), range: { start: { line: l, character: 0 }, end: { line: l, character: 0 } } };
}

export class DefinitionProvider {
  private scanner = new CSymbolScanner();

  constructor(private index: WorkspaceIndex) {}

  onDefinition(params: DefinitionParams, document: TextDocument): Location[] | null {
    const { position } = params;
    const line = document.getText({ start: { line: position.line, character: 0 }, end: { line: position.line + 1, character: 0 } })
      .replace(/\r?\n$/, '');
    const asm = isAsm(document.languageId, document.uri);
    if (asm) {
      const loc = locTarget(line, WorkspaceIndex.uriToPath(document.uri));
      if (loc) return [loc];
    }
    const word = identAt(line, position.character, asm);
    if (!word) return null;

    if (!asm) {
      // x.name / p->name: the member in its struct
      let s = Math.min(position.character, line.length);
      while (s > 0 && /\w/.test(line[s - 1])) s--;
      const text = document.getText();
      const r = new TypeResolver(this.index, this.scanner.scan(document.uri, text).types)
        .memberAt(cleanC(text), document.offsetAt({ line: position.line, character: s }), word);
      if (r) {
        const at = { line: r.member.line, character: r.member.character };
        return [{ uri: r.agg.uri, range: { start: at, end: { line: at.line, character: at.character + word.length } } }];
      }
    }

    const definitions = this.index.findDefinitions(word);
    if (definitions.length === 0) return null;
    // (an assembly @label: the one in this file)
    const own = word.startsWith('@') ? definitions.filter((d) => d.location.uri === document.uri) : definitions;
    // (a C name the workspace defines: not the library's declaration of it too)
    const work = own.filter((d) => !d.library);
    return (work.length ? work : own).map((sym) => ({ uri: sym.location.uri, range: sym.location.range }));
  }
}
