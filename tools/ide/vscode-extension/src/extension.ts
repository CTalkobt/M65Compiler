import * as vscode from 'vscode';
import * as path from 'path';
import { LanguageClient, LanguageClientOptions, ServerOptions, TransportKind } from 'vscode-languageclient/node';

let client: LanguageClient;

export async function activate(context: vscode.ExtensionContext) {
  const workspaceFolder = vscode.workspace.workspaceFolders?.[0];
  if (!workspaceFolder) {
    vscode.window.showWarningMessage(
      'MEGA65 CC45/CA45 extension requires an open workspace folder'
    );
    return;
  }

  const serverModule = context.asAbsolutePath(
    path.join('dist', 'server.js')
  );

  const serverOptions: ServerOptions = {
    run: { module: serverModule, transport: TransportKind.ipc },
    debug: {
      module: serverModule,
      transport: TransportKind.ipc,
      options: { execArgv: ['--nolazy', '--inspect=6009'] },
    },
  };

  const clientOptions: LanguageClientOptions = {
    documentSelector: [
      { scheme: 'file', language: 'c' },
      { scheme: 'file', language: 'h' },
      { scheme: 'file', language: 'm65-asm' },
    ],
    synchronize: {
      fileEvents: vscode.workspace.createFileSystemWatcher('**/*.{c,h,s45,asm}'),
    },
  };

  client = new LanguageClient(
    'mega65-lsp',
    'MEGA65 Language Server',
    serverOptions,
    clientOptions
  );

  const disposable = { dispose: () => client.stop() };
  context.subscriptions.push(disposable);

  client.start().then(() => {
    console.log('[Extension] Language Server initialized');
  }).catch(err => {
    console.error(`Error starting language server: ${err}`);
  });
}

export function deactivate(): Thenable<void> | undefined {
  if (!client) {
    return undefined;
  }
  return client.stop();
}
