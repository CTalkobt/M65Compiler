import {
  TextDocuments,
  TextDocument,
  TextDocumentChangeEvent,
  Connection,
  Diagnostic,
  DiagnosticSeverity,
} from 'vscode-languageserver/node';
import { TextDocument as TextDocumentType } from 'vscode-languageserver-textdocument';
import { fileURLToPath } from 'url';
import { runTool } from './diagnostics/runTool';

interface DocumentState {
  generationId: number;
  debounceTimer: NodeJS.Timeout | null;
}

export class DocumentManager {
  private documents: TextDocuments<TextDocumentType>;
  private connection: Connection;
  private docState: Map<string, DocumentState> = new Map();
  private cc45Path: string;
  private ca45Path: string;
  private debounceMs: number;
  private workspacePath: string;
  /** called when a document's text changed (the workspace index) */
  onContent: ((doc: TextDocumentType) => void) | null = null;
  /** called when a document is closed (its file on disk indexed again) */
  onClose: ((uri: string) => void) | null = null;

  constructor(
    connection: Connection,
    cc45Path: string,
    ca45Path: string,
    debounceMs: number,
    workspacePath: string
  ) {
    this.documents = new TextDocuments(TextDocumentType);
    this.connection = connection;
    this.cc45Path = cc45Path;
    this.ca45Path = ca45Path;
    this.debounceMs = debounceMs;
    this.workspacePath = workspacePath;

    // Register event handlers
    this.documents.onDidOpen((evt) => this.handleDidOpen(evt.document));
    this.documents.onDidChangeContent((evt) => this.handleDidChange(evt.document));
    this.documents.onDidSave((evt) => this.handleDidSave(evt.document));
    this.documents.onDidClose((evt) => this.handleDidClose(evt.document));

    this.documents.listen(connection);
  }

  get textDocuments(): TextDocuments<TextDocumentType> {
    return this.documents;
  }

  /** New settings: the tools' paths and the debounce; open documents checked again */
  setConfig(cc45Path: string, ca45Path: string, debounceMs: number): void {
    const changed = cc45Path !== this.cc45Path || ca45Path !== this.ca45Path;
    this.cc45Path = cc45Path;
    this.ca45Path = ca45Path;
    this.debounceMs = debounceMs;
    if (changed) {
      for (const doc of this.documents.all()) {
        this.runDiagnostics(doc).catch((err) => console.error(`Error running diagnostics: ${err}`));
      }
    }
  }

  private getDocumentState(uri: string): DocumentState {
    let state = this.docState.get(uri);
    if (!state) {
      state = { generationId: 0, debounceTimer: null };
      this.docState.set(uri, state);
    }
    return state;
  }

  private async handleDidOpen(document: TextDocumentType): Promise<void> {
    this.onContent?.(document);
    // Immediate diagnostic on open
    await this.runDiagnostics(document);
  }

  private handleDidChange(document: TextDocumentType): void {
    this.onContent?.(document);
    // Debounced diagnostics on change
    const state = this.getDocumentState(document.uri);

    if (state.debounceTimer) {
      clearTimeout(state.debounceTimer);
    }

    state.debounceTimer = setTimeout(() => {
      state.debounceTimer = null;
      this.runDiagnostics(document).catch((err) => {
        console.error(`Error running diagnostics: ${err}`);
      });
    }, this.debounceMs);
  }

  private handleDidSave(document: TextDocumentType): void {
    // Immediate diagnostic on save
    const state = this.getDocumentState(document.uri);

    if (state.debounceTimer) {
      clearTimeout(state.debounceTimer);
      state.debounceTimer = null;
    }

    this.runDiagnostics(document).catch((err) => {
      console.error(`Error running diagnostics: ${err}`);
    });
  }

  private handleDidClose(document: TextDocumentType): void {
    const state = this.getDocumentState(document.uri);

    if (state.debounceTimer) {
      clearTimeout(state.debounceTimer);
      state.debounceTimer = null;
    }

    // Clear diagnostics for closed document
    this.connection.sendDiagnostics({ uri: document.uri, diagnostics: [] });
    this.docState.delete(document.uri);
    this.onClose?.(document.uri);
  }

  private async runDiagnostics(document: TextDocumentType): Promise<void> {
    const state = this.getDocumentState(document.uri);
    state.generationId++;
    const currentGeneration = state.generationId;

    const diagnostics = await this.getDiagnostics(document);

    // Discard if a newer generation has been requested
    if (currentGeneration !== state.generationId) {
      return;
    }

    this.connection.sendDiagnostics({
      uri: document.uri,
      diagnostics,
    });
  }

  private async getDiagnostics(document: TextDocumentType): Promise<Diagnostic[]> {
    const uri = document.uri;
    const filePath = this.uriToFilePath(uri);
    const language = this.detectLanguage(document.languageId, filePath);

    if (language !== 'c' && language !== 'asm') return [];
    // (a header alone is not compiled: what includes it reports its errors)
    if (language === 'c' && /\.h$/i.test(filePath)) return [];
    const run = await runTool(language, language === 'c' ? this.cc45Path : this.ca45Path, filePath, document.getText());
    const parsedDiags = run.diagnostics;

    // Convert parsed diagnostics to LSP format
    return parsedDiags.map((diag) => ({
      range: {
        start: {
          line: Math.max(0, diag.line - 1),
          character: Math.max(0, diag.column - 1),
        },
        end: {
          line: Math.max(0, diag.line - 1),
          // (to the end of the word at the column, or the line's end)
          character: this.wordEnd(document, diag.line - 1, diag.column - 1),
        },
      },
      severity:
        diag.severity === 'error'
          ? DiagnosticSeverity.Error
          : diag.severity === 'warning'
            ? DiagnosticSeverity.Warning
            : DiagnosticSeverity.Information,
      message: diag.message,
      source: language === 'c' ? 'cc45' : 'ca45',
    }));
  }

  private wordEnd(document: TextDocumentType, line: number, col: number): number {
    const text = document.getText({ start: { line: Math.max(0, line), character: 0 }, end: { line: Math.max(0, line) + 1, character: 0 } })
      .replace(/\r?\n$/, '');
    if (col >= text.length) return Math.max(col + 1, text.length);
    let e = Math.max(0, col);
    while (e < text.length && /[A-Za-z0-9_$#.]/.test(text[e])) e++;
    return e > col ? e : col + 1;
  }

  private detectLanguage(languageId: string, filePath: string): 'c' | 'asm' | 'unknown' {
    if (languageId === 'c' || languageId === 'h') {
      return 'c';
    }
    if (languageId === 'm65-asm' || filePath.endsWith('.s45') || filePath.endsWith('.asm')) {
      return 'asm';
    }
    return 'unknown';
  }

  private uriToFilePath(uri: string): string {
    return uri.startsWith('file:') ? fileURLToPath(uri) : uri;
  }
}
