import * as vscode from 'vscode';
import * as path from 'path';
import { LanguageClient, LanguageClientOptions, ServerOptions, TransportKind } from 'vscode-languageclient/node';

let client: LanguageClient;

// The "cc45" debug type: dbg45 --dap (the Debug Adapter Protocol on its
// stdin/stdout), which runs the program in xemu (#202)
class Dbg45AdapterFactory implements vscode.DebugAdapterDescriptorFactory {
  createDebugAdapterDescriptor(session: vscode.DebugSession): vscode.ProviderResult<vscode.DebugAdapterDescriptor> {
    const folder = session.workspaceFolder ?? vscode.workspace.workspaceFolders?.[0];
    const configured = vscode.workspace.getConfiguration('m65').get<string>('dbg45Path', '${workspaceFolder}/bin/dbg45');
    const dbg45 = configured.replace('${workspaceFolder}', folder ? folder.uri.fsPath : '');
    return new vscode.DebugAdapterExecutable(dbg45, ['--dap'], { cwd: folder?.uri.fsPath });
  }
}

export async function activate(context: vscode.ExtensionContext) {
  context.subscriptions.push(
    vscode.debug.registerDebugAdapterDescriptorFactory('cc45', new Dbg45AdapterFactory())
  );

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

export function deactivate(): Promise<void> | undefined {
  if (!client) {
    return undefined;
  }
  return client.stop();
}
