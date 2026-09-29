import { Symbol, SymbolKind } from './symbolTypes';

export class AsmSymbolScanner {
  scanFile(uri: string, content: string): Symbol[] {
    const symbols: Symbol[] = [];
    const lines = content.split('\n');

    // Regex patterns for assembly symbols
    const labelPattern = /^([a-zA-Z_@][a-zA-Z0-9_:]*)\s*:/;
    const globalPattern = /\.global\s+(\w+)/;
    const externPattern = /\.extern\s+(\w+)/;
    const equatePattern = /^(\w+)\s*=/;

    let globalSymbols = new Set<string>();
    let externSymbols = new Set<string>();

    // First pass: collect global and extern declarations
    for (const line of lines) {
      const globalMatch = line.match(globalPattern);
      if (globalMatch) {
        globalSymbols.add(globalMatch[1]);
      }

      const externMatch = line.match(externPattern);
      if (externMatch) {
        externSymbols.add(externMatch[1]);
      }
    }

    // Second pass: collect all symbols
    for (let lineNum = 0; lineNum < lines.length; lineNum++) {
      const line = lines[lineNum];

      // Labels
      const labelMatch = line.match(labelPattern);
      if (labelMatch) {
        const name = labelMatch[1];
        const scope = globalSymbols.has(name)
          ? 'global'
          : name.startsWith('@')
            ? 'local'
            : 'global';
        symbols.push(this.createSymbol(name, SymbolKind.Function, uri, lineNum, scope));
        continue;
      }

      // Equates (NAME = value)
      const equateMatch = line.match(equatePattern);
      if (equateMatch && !line.includes('.')) {
        symbols.push(
          this.createSymbol(equateMatch[1], SymbolKind.Constant, uri, lineNum, 'global')
        );
      }
    }

    // Add extern symbols
    for (const name of externSymbols) {
      symbols.push(this.createSymbol(name, SymbolKind.Function, uri, 0, 'extern'));
    }

    return symbols;
  }

  private createSymbol(
    name: string,
    kind: SymbolKind,
    uri: string,
    line: number,
    scope: 'local' | 'global' | 'extern'
  ): Symbol {
    return {
      name,
      kind,
      location: {
        uri,
        range: {
          start: { line, character: 0 },
          end: { line, character: 80 },
        },
      },
      isDefinition: scope !== 'extern',
      scope,
    };
  }
}
