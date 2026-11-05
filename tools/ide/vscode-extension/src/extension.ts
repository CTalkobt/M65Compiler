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
      // the m65.* settings sent to the server when they change
      configurationSection: 'm65',
      fileEvents: vscode.workspace.createFileSystemWatcher('**/*.{c,h,s45,asm,inc}'),
    },
    initializationOptions: { m65: vscode.workspace.getConfiguration('m65') },
  };

  client = new LanguageClient(
    'mega65-lsp',
    'MEGA65 Language Server',
    serverOptions,
    clientOptions
  );

  const disposable = { dispose: () => client.stop() };
  context.subscriptions.push(disposable);

  // M65: Show Assembly (a C file: cc45 -S, opened beside it at the cursor's
  // line) and M65: Show C Source (cc45's assembly: the line its .loc names)
  const place = async (method: string) => {
    const editor = vscode.window.activeTextEditor;
    if (!editor) return;
    const r = await client.sendRequest<{ uri?: string; line?: number; error?: string }>(method, {
      uri: editor.document.uri.toString(), line: editor.selection.active.line,
    });
    if (!r.uri) {
      vscode.window.showWarningMessage(r.error ?? 'nothing to show');
      return;
    }
    const doc = await vscode.workspace.openTextDocument(vscode.Uri.parse(r.uri));
    const at = new vscode.Position(r.line ?? 0, 0);
    await vscode.window.showTextDocument(doc, {
      viewColumn: method === 'm65/assembly' ? vscode.ViewColumn.Beside : vscode.ViewColumn.One,
      selection: new vscode.Range(at, at),
    });
  };
  context.subscriptions.push(
    vscode.commands.registerCommand('m65.showAssembly', () => place('m65/assembly')),
    vscode.commands.registerCommand('m65.showSource', () => place('m65/source')),
  );

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
