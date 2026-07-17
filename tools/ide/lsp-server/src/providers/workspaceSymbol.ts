import {
  Connection,
  WorkspaceSymbolParams,
  SymbolInformation,
  SymbolKind as LspSymbolKind,
} from 'vscode-languageserver/node';
import { WorkspaceIndex } from '../indexer/workspaceIndex';
import { SymbolKind } from '../indexer/symbolTypes';

export class WorkspaceSymbolProvider {
  constructor(private connection: Connection, private index: WorkspaceIndex) {}

  async onWorkspaceSymbol(params: WorkspaceSymbolParams): Promise<SymbolInformation[]> {
    const { query } = params;

    // Search for symbols matching query
    const symbols = this.index.findSymbols(query);

    // Convert to LSP SymbolInformation format
    return symbols.map((sym) => ({
      name: sym.name,
      kind: this.convertSymbolKind(sym.kind),
      location: {
        uri: sym.location.uri,
        range: sym.location.range,
      },
      containerName: this.getContainerName(sym.location.uri),
    }));
  }

  private convertSymbolKind(kind: SymbolKind): LspSymbolKind {
    // Map our SymbolKind to LSP SymbolKind
    const map: Record<SymbolKind, LspSymbolKind> = {
      [SymbolKind.File]: LspSymbolKind.File,
      [SymbolKind.Module]: LspSymbolKind.Module,
      [SymbolKind.Namespace]: LspSymbolKind.Namespace,
      [SymbolKind.Package]: LspSymbolKind.Package,
      [SymbolKind.Class]: LspSymbolKind.Class,
      [SymbolKind.Method]: LspSymbolKind.Method,
      [SymbolKind.Property]: LspSymbolKind.Property,
      [SymbolKind.Field]: LspSymbolKind.Field,
      [SymbolKind.Constructor]: LspSymbolKind.Constructor,
      [SymbolKind.Enum]: LspSymbolKind.Enum,
      [SymbolKind.Interface]: LspSymbolKind.Interface,
      [SymbolKind.Function]: LspSymbolKind.Function,
      [SymbolKind.Variable]: LspSymbolKind.Variable,
      [SymbolKind.Constant]: LspSymbolKind.Constant,
      [SymbolKind.String]: LspSymbolKind.String,
      [SymbolKind.Number]: LspSymbolKind.Number,
      [SymbolKind.Boolean]: LspSymbolKind.Boolean,
      [SymbolKind.Array]: LspSymbolKind.Array,
      [SymbolKind.Object]: LspSymbolKind.Object,
      [SymbolKind.Key]: LspSymbolKind.Key,
      [SymbolKind.Null]: LspSymbolKind.Null,
      [SymbolKind.EnumMember]: LspSymbolKind.EnumMember,
      [SymbolKind.Struct]: LspSymbolKind.Struct,
      [SymbolKind.Event]: LspSymbolKind.Event,
      [SymbolKind.Operator]: LspSymbolKind.Operator,
      [SymbolKind.TypeParameter]: LspSymbolKind.TypeParameter,
    };
    return map[kind] || LspSymbolKind.Variable;
  }

  private getContainerName(uri: string): string {
    // Extract filename from URI for container name
    const parts = uri.split('/');
    return parts[parts.length - 1] || uri;
  }
}
