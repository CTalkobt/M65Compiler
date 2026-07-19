import {
  Connection,
  HoverParams,
  Hover,
  TextDocument,
} from 'vscode-languageserver/node';
import * as fs from 'fs';
import * as path from 'path';

interface HoverReference {
  name: string;
  description: string;
  details?: string;
}

export class HoverProvider {
  private cKeywords: Map<string, HoverReference> = new Map();
  private opcodes: Map<string, HoverReference> = new Map();
  private directives: Map<string, HoverReference> = new Map();
  private connection: Connection;

  constructor(connection: Connection, dataDir: string) {
    this.connection = connection;
    this.loadReferences(dataDir);
  }

  private loadReferences(dataDir: string): void {
    try {
      // Load C keywords
      const ckeywordsPath = path.join(dataDir, 'c-keywords.json');
      if (fs.existsSync(ckeywordsPath)) {
        const data = JSON.parse(fs.readFileSync(ckeywordsPath, 'utf-8'));
        for (const kw of data.keywords) {
          this.cKeywords.set(kw.name.toLowerCase(), {
            name: kw.name,
            description: kw.description,
            details: `Type: ${kw.type}`,
          });
        }
      }

      // Load opcodes
      const opcodesPath = path.join(dataDir, 'opcodes.json');
      if (fs.existsSync(opcodesPath)) {
        const data = JSON.parse(fs.readFileSync(opcodesPath, 'utf-8'));
        for (const op of data.opcodes) {
          this.opcodes.set(op.mnemonic.toLowerCase(), {
            name: op.mnemonic,
            description: op.description,
            details: `Modes: ${op.modes.join(', ')}\nFlags: ${op.flags.join(', ') || 'none'}\nCycles: ${op.cycles}`,
          });
        }
      }

      // Load directives
      const directivesPath = path.join(dataDir, 'directives.json');
      if (fs.existsSync(directivesPath)) {
        const data = JSON.parse(fs.readFileSync(directivesPath, 'utf-8'));
        for (const dir of data.directives) {
          this.directives.set(dir.name.toLowerCase(), {
            name: `.${dir.name}`,
            description: dir.description,
            details: `Syntax: ${dir.syntax}\nExample: ${dir.example}`,
          });
        }
      }

      console.error(
        `[HoverProvider] Loaded ${this.cKeywords.size} C keywords, ${this.opcodes.size} opcodes, ${this.directives.size} directives`
      );
    } catch (err) {
      console.error(`[HoverProvider] Error loading references: ${err}`);
    }
  }

  async onHover(params: HoverParams, document: TextDocument): Promise<Hover | null> {
    const { position } = params;
    const line = document.getText().split('\n')[position.line] || '';

    // Get word at cursor position
    const word = this.getWordAtPosition(line, position.character);
    if (!word) {
      return null;
    }

    // Determine language and find reference
    const language = this.detectLanguage(document.languageId);
    let reference: HoverReference | undefined;

    if (language === 'c') {
      reference = this.cKeywords.get(word.toLowerCase());
    } else if (language === 'asm') {
      // Check if it's a directive (starts with .)
      if (word.startsWith('.')) {
        reference = this.directives.get(word.substring(1).toLowerCase());
      } else {
        // Check if it's an opcode
        reference = this.opcodes.get(word.toLowerCase());
      }
    }

    if (!reference) {
      return null;
    }

    // Build hover content
    const contents = this.buildHoverContent(reference);
    return {
      contents,
      range: undefined,
    };
  }

  private getWordAtPosition(line: string, column: number): string | null {
    const wordStart = Math.max(0, column - 1);
    let start = wordStart;
    let end = column;

    // Find start of word (alphanumeric, underscore, or dot for directives)
    while (start > 0 && /[a-zA-Z0-9_.]/.test(line[start - 1])) {
      start--;
    }

    // Find end of word
    while (end < line.length && /[a-zA-Z0-9_]/.test(line[end])) {
      end++;
    }

    const word = line.substring(start, end);
    return word.length > 0 ? word : null;
  }

  private detectLanguage(languageId: string): 'c' | 'asm' {
    if (languageId === 'm65-asm' || languageId === 's45' || languageId === 'asm') {
      return 'asm';
    }
    return 'c';
  }

  private buildHoverContent(reference: HoverReference): string {
    let content = `**${reference.name}**\n\n`;
    content += reference.description;
    if (reference.details) {
      content += `\n\n${reference.details}`;
    }
    return content;
  }
}
