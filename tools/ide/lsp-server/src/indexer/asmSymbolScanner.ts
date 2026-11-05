import { Symbol, SymbolKind } from './symbolTypes';
import { cleanAsm } from './text';

// An assembly file's symbols: labels (@local ones are the file's own),
// proc ... endproc (a function, its labels its children for the outline),
// .macro, NAME = value and .equ / .set, and the .global / .extern names.

export class AsmSymbolScanner {
  scanFile(uri: string, content: string): Symbol[] {
    const symbols: Symbol[] = [];
    const lines = cleanAsm(content).split('\n');
    const rawLines = content.split('\n');
    const globals = new Set<string>();
    const externs: { name: string; line: number; col: number }[] = [];
    for (let i = 0; i < lines.length; i++) {
      for (const m of lines[i].matchAll(/\.(global|globl|export|extern|import)\s+([^\n]*)/gi)) {
        const names = m[2].split(',').map((s) => s.trim()).filter((s) => /^[A-Za-z_][\w.]*$/.test(s));
        for (const n of names) {
          if (/global|globl|export/i.test(m[1])) globals.add(n);
          else externs.push({ name: n, line: i, col: lines[i].indexOf(n, (m.index ?? 0) + m[1].length + 1) });
        }
      }
    }

    let proc: Symbol | null = null;
    // (inside a proc: its child in the outline, and a symbol of the file)
    const add = (s: Symbol) => {
      if (proc) (proc.children ??= []).push(s);
      symbols.push(s);
    };

    for (let i = 0; i < lines.length; i++) {
      const line = lines[i];
      const raw = rawLines[i] ?? '';
      let m: RegExpExecArray | null;
      if ((m = /^\s*proc\s+([A-Za-z_][\w.]*)/i.exec(line))) {
        const col = line.indexOf(m[1], m[0].length - m[1].length);
        proc = this.sym(m[1], SymbolKind.Function, uri, i, col, 'global', raw.trim());
        proc.fullRange = { start: { line: i, character: 0 }, end: { line: i, character: line.length } };
        symbols.push(proc);
        continue;
      }
      if (/^\s*endproc\b/i.test(line)) {
        if (proc && proc.fullRange) proc.fullRange.end = { line: i, character: line.length };
        proc = null;
        continue;
      }
      if ((m = /^\s*\.macro\s+([A-Za-z_][\w.]*)/i.exec(line))) {
        const s = this.sym(m[1], SymbolKind.Function, uri, i, line.indexOf(m[1]), 'global', raw.trim());
        let j = i;
        while (j + 1 < lines.length && !/^\s*\.endmacro\b|^\s*\.endm\b/i.test(lines[j])) j++;
        s.fullRange = { start: { line: i, character: 0 }, end: { line: j, character: (lines[j] || '').length } };
        symbols.push(s);
        continue;
      }
      // label: (at the start of the line, or indented and followed by a colon)
      if ((m = /^\s*([A-Za-z_@][\w.]*)\s*:(?!=)/.exec(line))) {
        const name = m[1];
        const scope = name.startsWith('@') ? 'local' : 'global';
        add(this.sym(name, SymbolKind.Function, uri, i, line.indexOf(name), globals.has(name) || scope === 'global' ? scope : 'local', raw.trim()));
        continue;
      }
      // NAME = value, NAME .equ value, .set NAME, value
      if ((m = /^\s*([A-Za-z_][\w.]*)\s*(?:=|\.equ\b|\.set\b|equ\b)/i.exec(line)) && !/^\s*\./.test(line)) {
        add(this.sym(m[1], SymbolKind.Constant, uri, i, line.indexOf(m[1]), 'global', raw.trim()));
        continue;
      }
      if ((m = /^\s*\.(?:var|set|equ|define)\s+([A-Za-z_][\w.]*)/i.exec(line))) {
        add(this.sym(m[1], SymbolKind.Constant, uri, i, line.indexOf(m[1], line.indexOf('.') + 2), 'global', raw.trim()));
      }
    }

    for (const e of externs) {
      symbols.push(this.sym(e.name, SymbolKind.Function, uri, e.line, Math.max(0, e.col), 'extern', `.extern ${e.name}`));
    }
    return symbols;
  }

  private sym(name: string, kind: SymbolKind, uri: string, line: number, col: number,
    scope: 'local' | 'global' | 'extern', detail: string): Symbol {
    return {
      name,
      kind,
      location: { uri, range: { start: { line, character: col }, end: { line, character: col + name.length } } },
      isDefinition: scope !== 'extern',
      scope,
      detail,
    };
  }
}
