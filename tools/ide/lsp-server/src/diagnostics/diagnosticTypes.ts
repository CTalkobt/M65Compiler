export interface ParsedDiagnostic {
  file: string;
  line: number;
  column: number;
  severity: 'error' | 'warning' | 'note';
  message: string;
  /** a linter's finding: its rule (mega65-...) */
  rule?: string;
}

export interface DiagnosticRunResult {
  uri: string;
  diagnostics: ParsedDiagnostic[];
  generationId: number;
}
