import { createConnection, TextDocuments, TextDocumentSyncKind, } from 'vscode-languageserver/node';
import { TextDocument } from 'vscode-languageserver-textdocument';
const connection = createConnection();
const documents = new TextDocuments(TextDocument);
connection.onInitialize((params) => {
    console.error(`[LSP Server] Initialized with ${params.processId}`);
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
});
documents.listen(connection);
connection.listen();
//# sourceMappingURL=server.js.map