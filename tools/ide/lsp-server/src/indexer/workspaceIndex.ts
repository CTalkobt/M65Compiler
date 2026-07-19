import * as fs from 'fs';
import * as path from 'path';
import { TextDocument } from 'vscode-languageserver-textdocument';
import { Symbol, IndexedFile } from './symbolTypes';
import { CSymbolScanner } from './cSymbolScanner';
import { AsmSymbolScanner } from './asmSymbolScanner';

export class WorkspaceIndex {
  private indexedFiles: Map<string, IndexedFile> = new Map();
  private symbolsByName: Map<string, Symbol[]> = new Map();
  private cScanner = new CSymbolScanner();
  private asmScanner = new AsmSymbolScanner();
  private workspacePath: string;
  private excludePatterns: string[] = [
    '**/node_modules/**',
    '**/.git/**',
    '**/bin/**',
    '**/obj/**',
    '**/dist/**',
  ];

  constructor(workspacePath: string) {
    this.workspacePath = workspacePath;
  }

  /**
   * Index a single document (called on open/save)
   */
  indexDocument(document: TextDocument): void {
    const uri = document.uri;
    const content = document.getText();

    let symbols: Symbol[] = [];

    // Detect file type from language ID or extension
    const ext = path.extname(uri);
    if (
      document.languageId === 'c' ||
      document.languageId === 'h' ||
      ext === '.c' ||
      ext === '.h'
    ) {
      symbols = this.cScanner.scanFile(uri, content);
    } else if (
      document.languageId === 'm65-asm' ||
      ext === '.s45' ||
      ext === '.asm'
    ) {
      symbols = this.asmScanner.scanFile(uri, content);
    }

    // Store indexed file
    this.indexedFiles.set(uri, {
      uri,
      symbols,
      timestamp: Date.now(),
    });

    // Update symbol index
    this.rebuildSymbolIndex();
  }

  /**
   * Find all symbols matching a name (substring search)
   */
  findSymbols(query: string): Symbol[] {
    const lowerQuery = query.toLowerCase();
    const results: Symbol[] = [];

    for (const [name, symbols] of this.symbolsByName) {
      if (name.toLowerCase().includes(lowerQuery)) {
        results.push(...symbols);
      }
    }

    return results.slice(0, 50); // Limit to 50 results
  }

  /**
   * Find all definitions of a symbol by exact name
   */
  findDefinitions(name: string): Symbol[] {
    return (this.symbolsByName.get(name) || []).filter((s) => s.isDefinition);
  }

  /**
   * Get all workspace symbols
   */
  getAllSymbols(): Symbol[] {
    const all: Symbol[] = [];
    for (const symbols of this.symbolsByName.values()) {
      all.push(...symbols);
    }
    return all;
  }

  /**
   * Get symbols in a specific file
   */
  getFileSymbols(uri: string): Symbol[] {
    const indexed = this.indexedFiles.get(uri);
    return indexed ? indexed.symbols : [];
  }

  /**
   * Remove index for a document (called on close)
   */
  removeDocument(uri: string): void {
    this.indexedFiles.delete(uri);
    this.rebuildSymbolIndex();
  }

  /**
   * Rebuild the symbol name→location map from all indexed files
   */
  private rebuildSymbolIndex(): void {
    this.symbolsByName.clear();

    for (const indexed of this.indexedFiles.values()) {
      for (const symbol of indexed.symbols) {
        if (!this.symbolsByName.has(symbol.name)) {
          this.symbolsByName.set(symbol.name, []);
        }
        this.symbolsByName.get(symbol.name)!.push(symbol);
      }
    }
  }

  /**
   * Perform initial workspace scan (optional, for initialization)
   */
  scanWorkspace(): void {
    console.error('[WorkspaceIndex] Starting workspace scan...');

    const startTime = Date.now();
    let fileCount = 0;

    const scanDir = (dir: string) => {
      try {
        const entries = fs.readdirSync(dir, { withFileTypes: true });

        for (const entry of entries) {
          const fullPath = path.join(dir, entry.name);
          const relPath = path.relative(this.workspacePath, fullPath);

          // Check if path matches exclude patterns
          if (this.isExcluded(relPath)) {
            continue;
          }

          if (entry.isDirectory()) {
            scanDir(fullPath);
          } else if (entry.isFile()) {
            const ext = path.extname(entry.name);
            if (['.c', '.h', '.s45', '.asm'].includes(ext)) {
              try {
                const content = fs.readFileSync(fullPath, 'utf-8');
                const uri = `file://${fullPath}`;

                let symbols: Symbol[] = [];
                if (ext === '.c' || ext === '.h') {
                  symbols = this.cScanner.scanFile(uri, content);
                } else if (ext === '.s45' || ext === '.asm') {
                  symbols = this.asmScanner.scanFile(uri, content);
                }

                this.indexedFiles.set(uri, {
                  uri,
                  symbols,
                  timestamp: Date.now(),
                });

                fileCount++;
              } catch (err) {
                console.error(`[WorkspaceIndex] Error scanning ${fullPath}: ${err}`);
              }
            }
          }
        }
      } catch (err) {
        console.error(`[WorkspaceIndex] Error reading directory ${dir}: ${err}`);
      }
    };

    scanDir(this.workspacePath);
    this.rebuildSymbolIndex();

    const elapsed = Date.now() - startTime;
    console.error(
      `[WorkspaceIndex] Scanned ${fileCount} files in ${elapsed}ms, ${this.symbolsByName.size} symbols indexed`
    );
  }

  private isExcluded(relPath: string): boolean {
    for (const pattern of this.excludePatterns) {
      // Simple glob matching: convert pattern to regex
      const regex = this.globToRegex(pattern);
      if (regex.test(relPath)) {
        return true;
      }
    }
    return false;
  }

  private globToRegex(glob: string): RegExp {
    const escaped = glob
      .replace(/\./g, '\\.')
      .replace(/\*/g, '[^/]*')
      .replace(/\*\*/g, '.*');
    return new RegExp(`^${escaped}$`);
  }
}
