import {
  TextDocuments,
  TextDocument,
  TextDocumentChangeEvent,
  Connection,
  Diagnostic,
  DiagnosticSeverity,
} from 'vscode-languageserver/node';
import { TextDocument as TextDocumentType } from 'vscode-languageserver-textdocument';
import * as path from 'path';
import { runCompiler } from './diagnostics/runCompiler';
import { runAssembler } from './diagnostics/runAssembler';

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

  private getDocumentState(uri: string): DocumentState {
    let state = this.docState.get(uri);
    if (!state) {
      state = { generationId: 0, debounceTimer: null };
      this.docState.set(uri, state);
    }
    return state;
  }

  private async handleDidOpen(document: TextDocumentType): Promise<void> {
    // Immediate diagnostic on open
    await this.runDiagnostics(document);
  }

  private handleDidChange(document: TextDocumentType): void {
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

    let parsedDiags;
    if (language === 'c') {
      parsedDiags = runCompiler(this.cc45Path, filePath, this.workspacePath);
    } else if (language === 'asm') {
      parsedDiags = runAssembler(this.ca45Path, filePath, this.workspacePath);
    } else {
      // Unknown language
      return [
        {
          range: {
            start: { line: 0, character: 0 },
            end: { line: 0, character: 1 },
          },
          severity: DiagnosticSeverity.Warning,
          message: `Unknown file type: ${language}`,
        },
      ];
    }

    // Convert parsed diagnostics to LSP format
    return parsedDiags.map((diag) => ({
      range: {
        start: {
          line: Math.max(0, diag.line - 1),
          character: Math.max(0, diag.column - 1),
        },
        end: {
          line: Math.max(0, diag.line - 1),
          character: Math.max(0, diag.column),
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

  private detectLanguage(languageId: string, filePath: string): string {
    if (languageId === 'c' || languageId === 'h') {
      return 'c';
    }
    if (languageId === 'm65-asm' || filePath.endsWith('.s45') || filePath.endsWith('.asm')) {
      return 'asm';
    }
    return 'unknown';
  }

  private uriToFilePath(uri: string): string {
    // Convert file:// URI to filesystem path
    if (uri.startsWith('file://')) {
      // On Windows, file:///C:/path, on Unix, file:///path
      const pathPart = uri.substring(7);
      return decodeURIComponent(pathPart);
    }
    return uri;
  }
}
