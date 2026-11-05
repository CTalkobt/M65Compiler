import {
  createConnection,
  DidChangeConfigurationNotification,
  FileChangeType,
  InitializeParams,
  InitializeResult,
  ProposedFeatures,
  ServerCapabilities,
  TextDocumentSyncKind,
} from 'vscode-languageserver/node';
import { fileURLToPath, pathToFileURL } from 'url';
import * as fs from 'fs';
import * as path from 'path';

import { DocumentManager } from './documents';
import { HoverProvider } from './providers/hover';
import { CompletionProvider } from './providers/completion';
import { DefinitionProvider } from './providers/definition';
import { WorkspaceSymbolProvider } from './providers/workspaceSymbol';
import { DocumentSymbolProvider } from './providers/documentSymbol';
import { ReferencesProvider } from './providers/references';
import { asmLineFor, compileToAssembly } from './diagnostics/runTool';
import { WorkspaceIndex } from './indexer/workspaceIndex';
import { findDataDir, loadReferenceData } from './providers/data';
import { M65Settings, resolveSettings } from './config';

// The language server: stdio (--stdio), node IPC (VS Code) or a socket, as
// vscode-languageserver chooses from the command line; stdio when none is
// given (node server.js, as editors other than VS Code run it)
if (!process.argv.some((a) => /^--(stdio|node-ipc|socket|pipe|clientProcessId)/.test(a))) process.argv.push('--stdio');
const connection = createConnection(ProposedFeatures.all);
// (vscode-languageserver sends console output to the client; the server's
// progress notes go as log messages, not errors)
console.error = console.log;

let documentManager: DocumentManager;
let hoverProvider: HoverProvider;
let completionProvider: CompletionProvider;
let definitionProvider: DefinitionProvider;
let workspaceSymbolProvider: WorkspaceSymbolProvider;
let documentSymbolProvider: DocumentSymbolProvider;
let referencesProvider: ReferencesProvider;
let cc45Path = '';
let workspaceIndex: WorkspaceIndex;
let workspacePath = process.cwd();
let hasConfiguration = false;
let initialSettings: M65Settings | undefined;

async function applySettings(): Promise<void> {
  let s: M65Settings | undefined = initialSettings;
  if (hasConfiguration) {
    try {
      s = (await connection.workspace.getConfiguration('m65')) ?? s;
    } catch (err) {
      console.error(`[LSP Server] workspace/configuration: ${err}`);
    }
  }
  const r = resolveSettings(s, workspacePath);
  documentManager.setConfig(r.cc45Path, r.ca45Path, r.debounceMs);
  workspaceIndex.setExclude(r.exclude);
  cc45Path = r.cc45Path;
  workspaceIndex.setLibraryDirs(libraryDirs(r.cc45Path));
  console.error(`[LSP Server] cc45=${r.cc45Path} ca45=${r.ca45Path} debounce=${r.debounceMs}ms`);
}

/** cc45's headers: CC45_INCLUDE, else beside the compiler (bin/../lib/include, an installed bin/../include/cc45) */
function libraryDirs(cc45: string): string[] {
  const out: string[] = [];
  if (process.env.CC45_INCLUDE) out.push(process.env.CC45_INCLUDE);
  let real = cc45;
  try { real = fs.realpathSync(cc45); } catch { /* not there */ }
  for (const bin of new Set([path.dirname(cc45), path.dirname(real)])) {
    for (const d of [path.join(bin, '..', 'lib', 'include'), path.join(bin, '..', 'include', 'cc45')]) {
      if (fs.existsSync(d)) { out.push(path.resolve(d)); break; }
    }
  }
  return Array.from(new Set(out));
}

connection.onInitialize((params: InitializeParams): InitializeResult => {
  const root = params.workspaceFolders?.[0]?.uri ?? params.rootUri;
  if (root && root.startsWith('file:')) workspacePath = fileURLToPath(root);
  else if (params.rootPath) workspacePath = params.rootPath;
  hasConfiguration = !!params.capabilities.workspace?.configuration;
  const opts = params.initializationOptions as { m65?: M65Settings } | M65Settings | undefined;
  initialSettings = opts && 'm65' in opts ? opts.m65 : (opts as M65Settings | undefined);
  console.error(`[LSP Server] workspace: ${workspacePath}`);

  const data = loadReferenceData(findDataDir(__dirname));
  workspaceIndex = new WorkspaceIndex(workspacePath);
  const r = resolveSettings(initialSettings, workspacePath);
  workspaceIndex.setExclude(r.exclude);
  documentManager = new DocumentManager(connection, r.cc45Path, r.ca45Path, r.debounceMs, workspacePath);
  documentManager.onContent = (doc) => workspaceIndex.indexDocument(doc);
  documentManager.onClose = (uri) => {
    if (uri.startsWith('file:')) workspaceIndex.indexFile(fileURLToPath(uri));
  };
  const open = (uri: string) => documentManager.textDocuments.get(uri);
  hoverProvider = new HoverProvider(data, workspaceIndex, open);
  completionProvider = new CompletionProvider(data, workspaceIndex);
  definitionProvider = new DefinitionProvider(workspaceIndex);
  workspaceSymbolProvider = new WorkspaceSymbolProvider(connection, workspaceIndex);
  documentSymbolProvider = new DocumentSymbolProvider();
  referencesProvider = new ReferencesProvider(workspaceIndex, open);
  cc45Path = r.cc45Path;

  const capabilities: ServerCapabilities = {
    textDocumentSync: TextDocumentSyncKind.Full,
    hoverProvider: true,
    completionProvider: { resolveProvider: false, triggerCharacters: ['.', '>', '#', ' '] },
    definitionProvider: true,
    workspaceSymbolProvider: true,
    documentSymbolProvider: true,
    referencesProvider: true,
  };
  return { capabilities, serverInfo: { name: 'MEGA65 CC45/CA45 Language Server', version: '0.2.0' } };
});

connection.onInitialized(() => {
  if (hasConfiguration) {
    connection.client.register(DidChangeConfigurationNotification.type, { section: 'm65' }).catch(() => undefined);
  }
  applySettings().catch((err) => console.error(`[LSP Server] settings: ${err}`));
  // the workspace's symbols for go to definition and workspace symbols, and
  // the compiler's headers
  setTimeout(() => {
    workspaceIndex.scanWorkspace();
    workspaceIndex.setLibraryDirs(libraryDirs(cc45Path));
  }, 0);
});

connection.onDidChangeConfiguration((change) => {
  if (!hasConfiguration) initialSettings = (change.settings && change.settings.m65) || initialSettings;
  applySettings().catch((err) => console.error(`[LSP Server] settings: ${err}`));
});

connection.onHover((params) => {
  const doc = documentManager.textDocuments.get(params.textDocument.uri);
  return doc ? hoverProvider.onHover(params, doc) : null;
});

connection.onCompletion((params) => {
  const doc = documentManager.textDocuments.get(params.textDocument.uri);
  return doc ? completionProvider.onCompletion(params, doc) : [];
});

connection.onDefinition((params) => {
  const doc = documentManager.textDocuments.get(params.textDocument.uri);
  return doc ? definitionProvider.onDefinition(params, doc) : null;
});

connection.onWorkspaceSymbol((params) => workspaceSymbolProvider.onWorkspaceSymbol(params));

connection.onDocumentSymbol((params) => {
  const doc = documentManager.textDocuments.get(params.textDocument.uri);
  return doc ? documentSymbolProvider.onDocumentSymbol(doc) : [];
});

connection.onReferences((params) => {
  const doc = documentManager.textDocuments.get(params.textDocument.uri);
  return doc ? referencesProvider.onReferences(params, doc) : [];
});

// m65/assembly {uri, line}: the C document compiled to assembly (cc45 -S)
// and the line of its code there — {uri, line} or {error}; the editor opens
// it ("M65: Show Assembly")
interface Place { uri?: string; line?: number; error?: string }

connection.onRequest('m65/assembly', async (p: { uri: string; line: number }): Promise<Place> => {
  const doc = documentManager.textDocuments.get(p.uri);
  const file = fileURLToPath(p.uri);
  const text = doc ? doc.getText() : fs.readFileSync(file, 'utf-8');
  const r = await compileToAssembly(cc45Path, file, text);
  if (!r.asmPath) return { error: r.error };
  return { uri: pathToFileURL(r.asmPath).toString(), line: asmLineFor(fs.readFileSync(r.asmPath, 'utf-8'), file, p.line) };
});

// m65/source {uri, line}: in cc45's assembly, the C line the code at line
// comes from (the .loc before it) — {uri, line} or {error}
connection.onRequest('m65/source', (p: { uri: string; line: number }): Place => {
  const doc = documentManager.textDocuments.get(p.uri);
  const lines = (doc ? doc.getText() : fs.readFileSync(fileURLToPath(p.uri), 'utf-8')).split('\n');
  for (let i = Math.min(p.line, lines.length - 1); i >= 0; i--) {
    const m = /^\s*\.loc\s+"([^"]+)"\s*,\s*(\d+)/.exec(lines[i]);
    if (m) return { uri: pathToFileURL(path.resolve(path.dirname(fileURLToPath(p.uri)), m[1])).toString(), line: parseInt(m[2], 10) - 1 };
  }
  return { error: 'no .loc line before here (not cc45 output, or compiled without line information)' };
});

// files changed outside the editor (a checkout, a generator): indexed again
connection.onDidChangeWatchedFiles((params) => {
  for (const c of params.changes) {
    if (!c.uri.startsWith('file:') || documentManager.textDocuments.get(c.uri)) continue;
    const p = fileURLToPath(c.uri);
    if (/\/\.[^/]*\.m65lsp\./.test(p)) continue;   // (the server's own copies)
    if (c.type === FileChangeType.Deleted) workspaceIndex.removeDocument(c.uri);
    else workspaceIndex.indexFile(p);
  }
});

connection.listen();
