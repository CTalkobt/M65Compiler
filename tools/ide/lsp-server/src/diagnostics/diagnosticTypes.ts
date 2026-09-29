export interface ParsedDiagnostic {
  file: string;
  line: number;
  column: number;
  severity: 'error' | 'warning' | 'note';
  message: string;
}

export interface DiagnosticRunResult {
  uri: string;
  diagnostics: ParsedDiagnostic[];
  generationId: number;
}
