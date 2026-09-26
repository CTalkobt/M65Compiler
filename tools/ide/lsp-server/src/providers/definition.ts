import {
  Connection,
  DefinitionParams,
  Location,
  TextDocument,
} from 'vscode-languageserver/node';
import { WorkspaceIndex } from '../indexer/workspaceIndex';

export class DefinitionProvider {
  constructor(private connection: Connection, private index: WorkspaceIndex) {}

  async onDefinition(params: DefinitionParams, document: TextDocument): Promise<Location[] | null> {
    const { position } = params;
    const line = document.getText().split('\n')[position.line] || '';

    // Get word at cursor position
    const word = this.getWordAtPosition(line, position.character);
    if (!word) {
      return null;
    }

    // Look up symbol definitions
    const definitions = this.index.findDefinitions(word);

    if (definitions.length === 0) {
      return null;
    }

    // Convert to LSP Location format
    return definitions.map((sym) => ({
      uri: sym.location.uri,
      range: sym.location.range,
    }));
  }

  private getWordAtPosition(line: string, column: number): string | null {
    const wordStart = Math.max(0, column - 1);
    let start = wordStart;
    let end = column;

    // Find start of word (alphanumeric, underscore, dot for labels)
    while (start > 0 && /[a-zA-Z0-9_.:@]/.test(line[start - 1])) {
      start--;
    }

    // Find end of word
    while (end < line.length && /[a-zA-Z0-9_]/.test(line[end])) {
      end++;
    }

    const word = line.substring(start, end);
    return word.length > 0 ? word : null;
  }
}
