import { ParsedDiagnostic } from './diagnosticTypes';

const DIAGNOSTIC_REGEX = /^(.+?):(\d+):(\d+):\s*(error|warning|note):\s*(.*)$/;

export function parseDiagnosticLine(line: string): ParsedDiagnostic | null {
  const match = line.match(DIAGNOSTIC_REGEX);
  if (!match) {
    return null;
  }

  const [, file, lineStr, colStr, severity, message] = match;
  return {
    file,
    line: parseInt(lineStr, 10),
    column: parseInt(colStr, 10),
    severity: severity as 'error' | 'warning' | 'note',
    message,
  };
}

export function parseDiagnostics(stderr: string): ParsedDiagnostic[] {
  const diagnostics: ParsedDiagnostic[] = [];
  const lines = stderr.split('\n');

  for (const line of lines) {
    if (!line.trim()) continue;

    const parsed = parseDiagnosticLine(line);
    if (parsed) {
      diagnostics.push(parsed);
    } else if (line.trim().length > 0) {
      // Catch-all for unparseable lines (raw crashes, asserts, etc.)
      // Extract filename from line if possible, else use generic source
      const fileMatch = line.match(/^([^\s:]+)/);
      diagnostics.push({
        file: fileMatch ? fileMatch[1] : 'unknown',
        line: 1,
        column: 1,
        severity: 'error',
        message: line,
      });
    }
  }

  return diagnostics;
}
