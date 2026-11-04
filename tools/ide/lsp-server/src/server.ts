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
import { fileURLToPath } from 'url';

import { DocumentManager } from './documents';
import { HoverProvider } from './providers/hover';
import { CompletionProvider } from './providers/completion';
import { DefinitionProvider } from './providers/definition';
import { WorkspaceSymbolProvider } from './providers/workspaceSymbol';
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
  console.error(`[LSP Server] cc45=${r.cc45Path} ca45=${r.ca45Path} debounce=${r.debounceMs}ms`);
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
  hoverProvider = new HoverProvider(data, workspaceIndex);
  completionProvider = new CompletionProvider(data, workspaceIndex);
  definitionProvider = new DefinitionProvider(connection, workspaceIndex);
  workspaceSymbolProvider = new WorkspaceSymbolProvider(connection, workspaceIndex);

  const capabilities: ServerCapabilities = {
    textDocumentSync: TextDocumentSyncKind.Full,
    hoverProvider: true,
    completionProvider: { resolveProvider: false, triggerCharacters: ['.', '#', ' '] },
    definitionProvider: true,
    workspaceSymbolProvider: true,
  };
  return { capabilities, serverInfo: { name: 'MEGA65 CC45/CA45 Language Server', version: '0.2.0' } };
});

connection.onInitialized(() => {
  if (hasConfiguration) {
    connection.client.register(DidChangeConfigurationNotification.type, { section: 'm65' }).catch(() => undefined);
  }
  applySettings().catch((err) => console.error(`[LSP Server] settings: ${err}`));
  // the workspace's symbols for go to definition and workspace symbols
  setTimeout(() => workspaceIndex.scanWorkspace(), 0);
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
