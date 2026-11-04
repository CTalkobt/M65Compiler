import { ParsedDiagnostic } from './diagnosticTypes';

// file:line:col: severity: message (cc45 and ca45's form), after an optional
// "Compilation error: " (cc45's one-line summary of the first error)
const DIAGNOSTIC_REGEX = /^(?:Compilation error:\s*)?(.+?):(\d+):(\d+):\s*(error|warning|note):\s*(.*)$/;
// ca45's older form: "file: line 12: message" (an error)
const LINE_REGEX = /^(?:Compilation error:\s*)?(.+?): line (\d+):\s*(?:(error|warning|note):\s*)?(.*)$/;
// cc45's warnings without a file: "12: warning: message"
const BARE_REGEX = /^(\d+):\s*(error|warning|note):\s*(.*)$/;

// what the tools print around a diagnostic, not one itself: the source
// excerpt (" 2 | text", "   |   ^"), cc45's "Suggestions:" and its bullets,
// -v log lines ("[cc45:INFO] ..."), ca45's "Assembled to ..." / "Object file: ..."
const CONTEXT_REGEX = /^\s*(\d+\s*)?\|(\s|$)|^Suggestions:\s*$|^\s*[•*-]\s|^\[[A-Za-z0-9_]+:[A-Z]+\]|^(Assembled to|Object file:|Linked \d+|Map file:)/;

export function parseDiagnosticLine(line: string): ParsedDiagnostic | null {
  let match = line.match(DIAGNOSTIC_REGEX);
  if (match) {
    const [, file, lineStr, colStr, severity, message] = match;
    return {
      file,
      line: parseInt(lineStr, 10),
      column: parseInt(colStr, 10),
      severity: severity as 'error' | 'warning' | 'note',
      message,
    };
  }
  match = line.match(LINE_REGEX);
  if (match && !/\s/.test(match[1].trim()) ) {
    const [, file, lineStr, severity, message] = match;
    return {
      file,
      line: parseInt(lineStr, 10),
      column: 1,
      severity: (severity || 'error') as 'error' | 'warning' | 'note',
      message,
    };
  }
  match = line.match(BARE_REGEX);
  if (match) {
    const [, lineStr, severity, message] = match;
    return {
      file: '',
      line: parseInt(lineStr, 10),
      column: 1,
      severity: severity as 'error' | 'warning' | 'note',
      message,
    };
  }
  return null;
}

export function isContextLine(line: string): boolean {
  return CONTEXT_REGEX.test(line);
}

export function parseDiagnostics(stderr: string): ParsedDiagnostic[] {
  const diagnostics: ParsedDiagnostic[] = [];
  const lines = stderr.split('\n');

  for (const line of lines) {
    if (!line.trim()) continue;

    const parsed = parseDiagnosticLine(line);
    if (parsed) {
      diagnostics.push(parsed);
    } else if (isContextLine(line)) {
      continue;
    } else {
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

  // cc45 repeats its first error as "Compilation error: <the same>": once
  const seen = new Set<string>();
  return diagnostics.filter((d) => {
    const key = `${d.file}:${d.line}:${d.column}:${d.severity}:${d.message}`;
    if (seen.has(key)) return false;
    seen.add(key);
    return true;
  });
}
