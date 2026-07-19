import {
  Connection,
  CompletionParams,
  CompletionItem,
  CompletionItemKind,
  InsertTextFormat,
  TextDocument,
} from 'vscode-languageserver/node';
import * as fs from 'fs';
import * as path from 'path';

export class CompletionProvider {
  private cKeywords: CompletionItem[] = [];
  private opcodes: CompletionItem[] = [];
  private directives: CompletionItem[] = [];
  private connection: Connection;

  constructor(connection: Connection, dataDir: string) {
    this.connection = connection;
    this.loadCompletions(dataDir);
  }

  private loadCompletions(dataDir: string): void {
    try {
      // Load C keywords
      const ckeywordsPath = path.join(dataDir, 'c-keywords.json');
      if (fs.existsSync(ckeywordsPath)) {
        const data = JSON.parse(fs.readFileSync(ckeywordsPath, 'utf-8'));
        for (const kw of data.keywords) {
          this.cKeywords.push({
            label: kw.name,
            kind: this.getCompletionKindForKeyword(kw.type),
            detail: kw.type,
            documentation: kw.description,
            insertText: kw.name,
            insertTextFormat: InsertTextFormat.PlainText,
          });
        }
      }

      // Load opcodes
      const opcodesPath = path.join(dataDir, 'opcodes.json');
      if (fs.existsSync(opcodesPath)) {
        const data = JSON.parse(fs.readFileSync(opcodesPath, 'utf-8'));
        for (const op of data.opcodes) {
          this.opcodes.push({
            label: op.mnemonic,
            kind: CompletionItemKind.Method,
            detail: `Modes: ${op.modes.join(', ')}`,
            documentation: op.description,
            insertText: op.mnemonic.toLowerCase(),
            insertTextFormat: InsertTextFormat.PlainText,
          });
        }
      }

      // Load directives
      const directivesPath = path.join(dataDir, 'directives.json');
      if (fs.existsSync(directivesPath)) {
        const data = JSON.parse(fs.readFileSync(directivesPath, 'utf-8'));
        for (const dir of data.directives) {
          this.directives.push({
            label: `.${dir.name}`,
            kind: CompletionItemKind.Keyword,
            detail: dir.syntax,
            documentation: dir.description,
            insertText: `.${dir.name}`,
            insertTextFormat: InsertTextFormat.PlainText,
          });
        }
      }

      console.error(
        `[CompletionProvider] Loaded ${this.cKeywords.length} C completions, ${this.opcodes.length} opcodes, ${this.directives.length} directives`
      );
    } catch (err) {
      console.error(`[CompletionProvider] Error loading completions: ${err}`);
    }
  }

  async onCompletion(
    params: CompletionParams,
    document: TextDocument
  ): Promise<CompletionItem[]> {
    const { position } = params;
    const line = document.getText().split('\n')[position.line] || '';
    const character = position.character;

    // Get the word being completed (partial word)
    const partialWord = this.getPartialWordAtPosition(line, character);

    // Determine language and return appropriate completions
    const language = this.detectLanguage(document.languageId);
    let completions: CompletionItem[] = [];

    if (language === 'c') {
      completions = this.cKeywords;
    } else if (language === 'asm') {
      // For assembly, offer both opcodes and directives
      // Check if cursor is after a dot (directive context)
      const beforeCursor = line.substring(0, character);
      const lastDotIndex = beforeCursor.lastIndexOf('.');

      if (lastDotIndex !== -1 && character - lastDotIndex <= 20) {
        // In directive context
        completions = this.directives;
      } else {
        // Mix both opcodes and directives
        completions = [...this.opcodes, ...this.directives];
      }
    }

    // Filter by partial word if provided
    if (partialWord) {
      const lowerPartial = partialWord.toLowerCase();
      completions = completions.filter((item) =>
        item.label.toLowerCase().startsWith(lowerPartial)
      );
    }

    // Sort by relevance (exact prefix match first, then alphabetical)
    completions.sort((a, b) => {
      const aLower = a.label.toLowerCase();
      const bLower = b.label.toLowerCase();
      return aLower.localeCompare(bLower);
    });

    return completions.slice(0, 50); // Limit to 50 results
  }

  private getPartialWordAtPosition(line: string, column: number): string {
    let start = column - 1;

    // Find start of word (skip backwards over alphanumeric/underscore/dot)
    while (start >= 0 && /[a-zA-Z0-9_.]/.test(line[start])) {
      start--;
    }
    start++;

    // Word is from start to column (we don't go forward, just use current position)
    return line.substring(start, column);
  }

  private detectLanguage(languageId: string): 'c' | 'asm' {
    if (languageId === 'm65-asm' || languageId === 's45' || languageId === 'asm') {
      return 'asm';
    }
    return 'c';
  }

  private getCompletionKindForKeyword(
    type: string
  ): CompletionItemKind {
    if (type === 'type') {
      return CompletionItemKind.Class;
    }
    if (type === 'qualifier') {
      return CompletionItemKind.Keyword;
    }
    return CompletionItemKind.Keyword;
  }
}
