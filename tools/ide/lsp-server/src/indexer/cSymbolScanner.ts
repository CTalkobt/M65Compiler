import { Symbol, SymbolKind } from './symbolTypes';

export class CSymbolScanner {
  scanFile(uri: string, content: string): Symbol[] {
    const symbols: Symbol[] = [];
    const lines = content.split('\n');

    // Regex patterns for C symbols
    const functionDefPattern =
      /^\s*(?:static\s+|extern\s+)?(?:inline\s+)?(?:const\s+)?(?:volatile\s+)?[\w*\s]+\s+(\w+)\s*\([^)]*\)\s*\{/;
    const typedefPattern = /^\s*typedef\s+(?:struct|union|enum)?\s*(\w+)/;
    const structPattern = /^\s*(?:typedef\s+)?struct\s+(\w+)\s*\{?/;
    const enumPattern = /^\s*(?:typedef\s+)?enum\s+(\w+)\s*\{?/;
    const varDeclPattern = /^\s*(?:static\s+|extern\s+|const\s+|volatile\s+)*[\w*\s]+\s+(\w+)\s*[=;]/;
    const definePattern = /^\s*#define\s+(\w+)/;

    for (let lineNum = 0; lineNum < lines.length; lineNum++) {
      const line = lines[lineNum];

      // Function definitions
      const funcMatch = line.match(functionDefPattern);
      if (funcMatch) {
        symbols.push(this.createSymbol(funcMatch[1], SymbolKind.Function, uri, lineNum));
        continue;
      }

      // Typedef
      const typedefMatch = line.match(typedefPattern);
      if (typedefMatch) {
        symbols.push(this.createSymbol(typedefMatch[1], SymbolKind.Struct, uri, lineNum));
        continue;
      }

      // Struct definitions
      const structMatch = line.match(structPattern);
      if (structMatch && !line.includes('typedef')) {
        symbols.push(this.createSymbol(structMatch[1], SymbolKind.Struct, uri, lineNum));
        continue;
      }

      // Enum definitions
      const enumMatch = line.match(enumPattern);
      if (enumMatch && !line.includes('typedef')) {
        symbols.push(this.createSymbol(enumMatch[1], SymbolKind.Enum, uri, lineNum));
        continue;
      }

      // Macro defines
      const defineMatch = line.match(definePattern);
      if (defineMatch) {
        symbols.push(this.createSymbol(defineMatch[1], SymbolKind.Constant, uri, lineNum));
        continue;
      }
    }

    return symbols;
  }

  private createSymbol(name: string, kind: SymbolKind, uri: string, line: number): Symbol {
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
      isDefinition: true,
      scope: 'global',
    };
  }
}
