import * as fs from 'fs';
import * as path from 'path';
import { pathToFileURL, fileURLToPath } from 'url';
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
  // m65.indexer.exclude: directory names (node_modules) or globs over the
  // path from the workspace (build/**, **/*.gen.c)
  private excludePatterns: string[] = ['node_modules', '.git', 'bin', 'obj', 'build', 'dist'];
  private static readonly EXTENSIONS = ['.c', '.h', '.s45', '.asm', '.inc'];
  private static readonly MAX_FILES = 20000;
  private static readonly MAX_SIZE = 2 * 1024 * 1024;

  constructor(workspacePath: string) {
    this.workspacePath = workspacePath;
  }

  setExclude(patterns: string[]): void {
    this.excludePatterns = patterns;
  }

  private scan(uri: string, content: string, asm: boolean): Symbol[] {
    return asm ? this.asmScanner.scanFile(uri, content) : this.cScanner.scanFile(uri, content);
  }

  /** A file on disk indexed again (created or changed outside the editor) */
  indexFile(fsPath: string): void {
    const ext = path.extname(fsPath).toLowerCase();
    if (!WorkspaceIndex.EXTENSIONS.includes(ext)) return;
    if (this.isExcluded(path.relative(this.workspacePath, fsPath))) return;
    let content: string;
    try {
      const st = fs.statSync(fsPath);
      if (!st.isFile() || st.size > WorkspaceIndex.MAX_SIZE) return;
      content = fs.readFileSync(fsPath, 'utf-8');
    } catch {
      this.removeDocument(pathToFileURL(fsPath).toString());
      return;
    }
    const uri = pathToFileURL(fsPath).toString();
    this.indexedFiles.set(uri, { uri, symbols: this.scan(uri, content, ext !== '.c' && ext !== '.h'), timestamp: Date.now() });
    this.rebuildSymbolIndex();
  }

  static uriToPath(uri: string): string {
    return uri.startsWith('file:') ? fileURLToPath(uri) : uri;
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
            if (WorkspaceIndex.EXTENSIONS.includes(ext.toLowerCase()) && fileCount < WorkspaceIndex.MAX_FILES) {
              try {
                if (fs.statSync(fullPath).size > WorkspaceIndex.MAX_SIZE) continue;
                const content = fs.readFileSync(fullPath, 'utf-8');
                const uri = pathToFileURL(fullPath).toString();
                const symbols = this.scan(uri, content, ext !== '.c' && ext !== '.h');

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
    const rel = relPath.split(path.sep).join('/');
    const parts = rel.split('/');
    for (const pattern of this.excludePatterns) {
      if (!/[*?/]/.test(pattern)) {
        if (parts.includes(pattern)) return true;   // a directory or file name
      } else if (WorkspaceIndex.globToRegex(pattern).test(rel)) {
        return true;
      }
    }
    return false;
  }

  /** ** any directories, * anything but /, ? one character */
  static globToRegex(glob: string): RegExp {
    let re = '';
    for (let i = 0; i < glob.length; i++) {
      const c = glob[i];
      if (c === '*' && glob[i + 1] === '*') {
        re += '.*';
        i++;
        if (glob[i + 1] === '/') i++;
      } else if (c === '*') {
        re += '[^/]*';
      } else if (c === '?') {
        re += '[^/]';
      } else {
        re += c.replace(/[.+^${}()|[\]\\]/g, '\\$&');
      }
    }
    return new RegExp(`^${re}$`);
  }
}
