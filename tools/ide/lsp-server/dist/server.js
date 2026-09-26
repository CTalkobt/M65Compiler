import { createConnection, TextDocumentSyncKind, DidChangeConfigurationNotification, } from 'vscode-languageserver/node';
import * as path from 'path';
import { DocumentManager } from './documents';
import { HoverProvider } from './providers/hover';
import { CompletionProvider } from './providers/completion';
import { DefinitionProvider } from './providers/definition';
import { WorkspaceSymbolProvider } from './providers/workspaceSymbol';
import { WorkspaceIndex } from './indexer/workspaceIndex';
const connection = createConnection();
let documentManager;
let hoverProvider;
let completionProvider;
let definitionProvider;
let workspaceSymbolProvider;
let workspaceIndex;
let cc45Path = 'bin/cc45';
let ca45Path = 'bin/ca45';
let debounceMs = 500;
let workspacePath = process.cwd();
connection.onInitialize((params) => {
    console.error(`[LSP Server] Initialized with workspace: ${params.rootPath || params.rootUri}`);
    // Set workspace path from initialization
    if (params.rootPath) {
        workspacePath = params.rootPath;
    }
    else if (params.rootUri) {
        workspacePath = params.rootUri.replace('file://', '');
    }
    // Initialize providers
    const dataDir = path.join(__dirname, '..', 'data');
    workspaceIndex = new WorkspaceIndex(workspacePath);
    documentManager = new DocumentManager(connection, cc45Path, ca45Path, debounceMs, workspacePath);
    hoverProvider = new HoverProvider(connection, dataDir);
    completionProvider = new CompletionProvider(connection, dataDir);
    definitionProvider = new DefinitionProvider(connection, workspaceIndex);
    workspaceSymbolProvider = new WorkspaceSymbolProvider(connection, workspaceIndex);
    const capabilities = {
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
    // Register provider handlers
    connection.onHover((params) => {
        const doc = documentManager.textDocuments.get(params.textDocument.uri);
        if (!doc)
            return null;
        return hoverProvider.onHover(params, doc);
    });
    connection.onCompletion((params) => {
        const doc = documentManager.textDocuments.get(params.textDocument.uri);
        if (!doc)
            return [];
        return completionProvider.onCompletion(params, doc);
    });
    connection.onDefinition((params) => {
        const doc = documentManager.textDocuments.get(params.textDocument.uri);
        if (!doc)
            return null;
        // Index the document first (for current file symbols)
        workspaceIndex.indexDocument(doc);
        return definitionProvider.onDefinition(params, doc);
    });
    connection.onWorkspaceSymbol((params) => {
        return workspaceSymbolProvider.onWorkspaceSymbol(params);
    });
    connection.onDidChangeWatchedFiles(() => {
        // Could implement file watcher handling here for incremental indexing
    });
    console.error('[LSP Server] All providers registered');
});
connection.onDidChangeConfiguration((change) => {
    const settings = change.settings.m65 || {};
    cc45Path = settings.cc45Path || 'bin/cc45';
    ca45Path = settings.ca45Path || 'bin/ca45';
    debounceMs = settings.diagnostics?.debounceMs || 500;
    console.error(`[LSP Server] Configuration updated: cc45=${cc45Path}, ca45=${ca45Path}, debounce=${debounceMs}ms`);
});
connection.listen();
//# sourceMappingURL=server.js.map