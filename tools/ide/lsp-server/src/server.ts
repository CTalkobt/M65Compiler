import {
  createConnection,
  InitializeParams,
  InitializeResult,
  ServerCapabilities,
  TextDocumentSyncKind,
  DidChangeConfigurationNotification,
} from 'vscode-languageserver/node';

import { DocumentManager } from './documents';

const connection = createConnection();

let documentManager: DocumentManager;
let cc45Path = 'bin/cc45';
let ca45Path = 'bin/ca45';
let debounceMs = 500;
let workspacePath = process.cwd();

connection.onInitialize((params: InitializeParams): InitializeResult => {
  console.error(`[LSP Server] Initialized with workspace: ${params.rootPath || params.rootUri}`);

  // Set workspace path from initialization
  if (params.rootPath) {
    workspacePath = params.rootPath;
  } else if (params.rootUri) {
    workspacePath = params.rootUri.replace('file://', '');
  }

  // Initialize document manager
  documentManager = new DocumentManager(
    connection,
    cc45Path,
    ca45Path,
    debounceMs,
    workspacePath
  );

  const capabilities: ServerCapabilities = {
    textDocumentSync: TextDocumentSyncKind.Full,
    diagnosticProvider: {
      interFileDependencies: false,
      workspaceDiagnostics: false,
    },
    hoverProvider: true,
    completionProvider: {
      resolveProvider: false,
    },
    definitionProvider: true,
    workspaceSymbolProvider: true,
  };

  return {
    capabilities,
    serverInfo: {
      name: 'MEGA65 CC45/CA45 Language Server',
      version: '0.1.0',
    },
  };
});

connection.onInitialized(() => {
  console.error('[LSP Server] Initialization complete');

  // Register for configuration changes
  connection.client.register(DidChangeConfigurationNotification.type);
});

connection.onDidChangeConfiguration((change) => {
  const settings = change.settings.m65 || {};
  cc45Path = settings.cc45Path || 'bin/cc45';
  ca45Path = settings.ca45Path || 'bin/ca45';
  debounceMs = settings.diagnostics?.debounceMs || 500;

  console.error(
    `[LSP Server] Configuration updated: cc45=${cc45Path}, ca45=${ca45Path}, debounce=${debounceMs}ms`
  );
});

connection.listen();
