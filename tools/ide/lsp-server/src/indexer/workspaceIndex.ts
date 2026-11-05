import * as fs from 'fs';
import * as path from 'path';
import { pathToFileURL, fileURLToPath } from 'url';
import { TextDocument } from 'vscode-languageserver-textdocument';
import { AggregateInfo, CType, CTypes, IndexedFile, Symbol, SymbolKind } from './symbolTypes';
import { CSymbolScanner } from './cSymbolScanner';
import { AsmSymbolScanner } from './asmSymbolScanner';
import { cleanAsm, cleanC, words } from './text';

export class WorkspaceIndex {
  private indexedFiles: Map<string, IndexedFile> = new Map();
  private symbolsByName: Map<string, Symbol[]> = new Map();
  private cScanner = new CSymbolScanner();
  private asmScanner = new AsmSymbolScanner();
  private workspacePath: string;
  private libraryDirs: string[] = [];
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

  static isAsmPath(p: string): boolean {
    return !/\.(c|h)$/i.test(p);
  }

  private build(uri: string, content: string, asm: boolean, library = false): IndexedFile {
    if (asm) {
      return { uri, symbols: this.asmScanner.scanFile(uri, content), timestamp: Date.now(), words: words(cleanAsm(content)), library };
    }
    const r = this.cScanner.scan(uri, content);
    if (library) for (const s of r.symbols) s.library = true;
    return { uri, symbols: r.symbols, types: r.types, timestamp: Date.now(), words: words(cleanC(content)), library };
  }

  /** A file on disk indexed again (created or changed outside the editor) */
  indexFile(fsPath: string): void {
    const ext = path.extname(fsPath).toLowerCase();
    if (!WorkspaceIndex.EXTENSIONS.includes(ext)) return;
    const library = this.libraryDirs.some((d) => fsPath.startsWith(d + path.sep));
    if (!library && this.isExcluded(path.relative(this.workspacePath, fsPath))) return;
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
    this.indexedFiles.set(uri, this.build(uri, content, WorkspaceIndex.isAsmPath(fsPath), library));
    this.rebuildSymbolIndex();
  }

  static uriToPath(uri: string): string {
    return uri.startsWith('file:') ? fileURLToPath(uri) : uri;
  }

  /** An open document (its text as edited) */
  indexDocument(document: TextDocument): void {
    const uri = document.uri;
    const c = document.languageId === 'c' || document.languageId === 'h' || /\.(c|h)$/i.test(uri);
    const asm = document.languageId === 'm65-asm' || /\.(s45|asm|inc|s)$/i.test(uri);
    if (!c && !asm) return;
    const prev = this.indexedFiles.get(uri);
    this.indexedFiles.set(uri, this.build(uri, document.getText(), !c, prev?.library ?? false));
    this.rebuildSymbolIndex();
  }

  /** The compiler's headers (lib/include): definitions, types and macros, not the workspace's symbols */
  setLibraryDirs(dirs: string[]): void {
    const keep = dirs.filter((d) => fs.existsSync(d));
    if (keep.join('\n') === this.libraryDirs.join('\n')) return;
    for (const [uri, f] of this.indexedFiles) if (f.library) this.indexedFiles.delete(uri);
    this.libraryDirs = keep;
    for (const dir of keep) {
      const scan = (d: string) => {
        for (const e of fs.readdirSync(d, { withFileTypes: true })) {
          const p = path.join(d, e.name);
          if (e.isDirectory()) scan(p);
          else if (/\.h$/i.test(e.name)) {
            try {
              const uri = pathToFileURL(p).toString();
              // (a workspace that is the compiler's own tree has them already)
              const had = this.indexedFiles.get(uri);
              const f = this.build(uri, fs.readFileSync(p, 'utf-8'), false, !had);
              this.indexedFiles.set(uri, f);
            } catch { /* unreadable */ }
          }
        }
      };
      try { scan(dir); } catch (err) { console.error(`[WorkspaceIndex] ${dir}: ${err}`); }
    }
    this.rebuildSymbolIndex();
    console.error(`[WorkspaceIndex] library headers: ${keep.join(', ') || '(none)'}`);
  }

  /** The workspace's symbols matching a query (substring), not the library's */
  findSymbols(query: string): Symbol[] {
    const lowerQuery = query.toLowerCase();
    const results: Symbol[] = [];
    for (const [name, symbols] of this.symbolsByName) {
      if (name.toLowerCase().includes(lowerQuery)) results.push(...symbols.filter((s) => !s.library));
      if (results.length >= 200) break;
    }
    return results.slice(0, 200);
  }

  /** All definitions of a name (the workspace's first, then the library's) */
  findDefinitions(name: string): Symbol[] {
    const all = (this.symbolsByName.get(name) || []).filter((s) => s.isDefinition);
    return [...all.filter((s) => !s.library), ...all.filter((s) => s.library)];
  }

  getAllSymbols(): Symbol[] {
    const all: Symbol[] = [];
    for (const symbols of this.symbolsByName.values()) all.push(...symbols);
    return all;
  }

  getFileSymbols(uri: string): Symbol[] {
    const indexed = this.indexedFiles.get(uri);
    return indexed ? indexed.symbols : [];
  }

  /** the indexed files that use a word (the files a reference search reads) */
  filesWithWord(word: string): string[] {
    const out: string[] = [];
    for (const f of this.indexedFiles.values()) if (f.words?.has(word)) out.push(f.uri);
    return out;
  }

  isLibrary(uri: string): boolean {
    return this.indexedFiles.get(uri)?.library ?? false;
  }

  // ---- C types (member completion): a document's own scan first, then the index

  private typesIn(first?: CTypes): CTypes[] {
    const out: CTypes[] = [];
    if (first) out.push(first);
    for (const f of this.indexedFiles.values()) if (f.types) out.push(f.types);
    return out;
  }

  findAggregate(tag: string, first?: CTypes): AggregateInfo | undefined {
    for (const t of this.typesIn(first)) {
      const a = t.aggregates.find((x) => x.tag === tag && x.members.length > 0);
      if (a) return a;
    }
    for (const t of this.typesIn(first)) {
      const a = t.aggregates.find((x) => x.tag === tag);
      if (a) return a;
    }
    return undefined;
  }

  findTypedef(name: string, first?: CTypes): CType | undefined {
    for (const t of this.typesIn(first)) {
      const v = t.typedefs.get(name);
      if (v) return v;
    }
    return undefined;
  }

  findVar(name: string, first?: CTypes): CType | undefined {
    for (const t of this.typesIn(first)) {
      const v = t.vars.get(name);
      if (v) return v;
    }
    return undefined;
  }

  findMacro(name: string, first?: CTypes): string | undefined {
    for (const t of this.typesIn(first)) {
      const v = t.macros.get(name);
      if (v !== undefined) return v;
    }
    return undefined;
  }

  removeDocument(uri: string): void {
    this.indexedFiles.delete(uri);
    this.rebuildSymbolIndex();
  }

  /** name → symbols (enumerators are names of the file too; members are not) */
  private rebuildSymbolIndex(): void {
    this.symbolsByName.clear();
    const add = (s: Symbol) => {
      let l = this.symbolsByName.get(s.name);
      if (!l) this.symbolsByName.set(s.name, (l = []));
      l.push(s);
    };
    for (const indexed of this.indexedFiles.values()) {
      for (const symbol of indexed.symbols) {
        add(symbol);
        if (symbol.kind === SymbolKind.Enum) for (const c of symbol.children ?? []) add({ ...c, library: symbol.library });
      }
    }
  }

  /** The workspace's files indexed (at start) */
  scanWorkspace(): void {
    console.error('[WorkspaceIndex] Starting workspace scan...');
    const startTime = Date.now();
    let fileCount = 0;
    const scanDir = (dir: string) => {
      let entries: fs.Dirent[];
      try {
        entries = fs.readdirSync(dir, { withFileTypes: true });
      } catch (err) {
        console.error(`[WorkspaceIndex] Error reading directory ${dir}: ${err}`);
        return;
      }
      for (const entry of entries) {
        const fullPath = path.join(dir, entry.name);
        if (this.isExcluded(path.relative(this.workspacePath, fullPath))) continue;
        if (entry.isDirectory()) {
          scanDir(fullPath);
        } else if (entry.isFile()) {
          const ext = path.extname(entry.name).toLowerCase();
          if (!WorkspaceIndex.EXTENSIONS.includes(ext) || fileCount >= WorkspaceIndex.MAX_FILES) continue;
          try {
            if (fs.statSync(fullPath).size > WorkspaceIndex.MAX_SIZE) continue;
            const uri = pathToFileURL(fullPath).toString();
            this.indexedFiles.set(uri, this.build(uri, fs.readFileSync(fullPath, 'utf-8'), WorkspaceIndex.isAsmPath(fullPath)));
            fileCount++;
          } catch (err) {
            console.error(`[WorkspaceIndex] Error scanning ${fullPath}: ${err}`);
          }
        }
      }
    };
    scanDir(this.workspacePath);
    this.rebuildSymbolIndex();
    console.error(`[WorkspaceIndex] Scanned ${fileCount} files in ${Date.now() - startTime}ms, ${this.symbolsByName.size} symbols indexed`);
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
