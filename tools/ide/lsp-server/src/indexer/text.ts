// Text helpers shared by the scanners and providers: source with its comments
// and literals blanked (same length, line breaks kept, so offsets and
// positions stay those of the file), positions, identifiers.

/** C: comments, string and character literals' contents as spaces */
export function cleanC(text: string): string {
  const out = text.split('');
  let i = 0;
  const n = text.length;
  const blank = (from: number, to: number) => {
    for (let k = from; k < to && k < n; k++) if (out[k] !== '\n') out[k] = ' ';
  };
  while (i < n) {
    const c = text[i];
    if (c === '/' && text[i + 1] === '/') {
      let j = i;
      while (j < n && text[j] !== '\n') j++;
      blank(i, j);
      i = j;
    } else if (c === '/' && text[i + 1] === '*') {
      const e = text.indexOf('*/', i + 2);
      const j = e < 0 ? n : e + 2;
      blank(i, j);
      i = j;
    } else if (c === '"' || c === "'") {
      let j = i + 1;
      while (j < n && text[j] !== c && text[j] !== '\n') j += text[j] === '\\' ? 2 : 1;
      blank(i + 1, j);
      i = j + 1;
    } else {
      i++;
    }
  }
  return out.join('');
}

/** Assembly: ; comments and the contents of "..." */
export function cleanAsm(text: string): string {
  const out = text.split('');
  let inStr = false;
  for (let i = 0; i < text.length; i++) {
    const c = text[i];
    if (c === '\n') { inStr = false; continue; }
    if (inStr) {
      if (c === '"') inStr = false; else out[i] = ' ';
      continue;
    }
    if (c === '"') { inStr = true; continue; }
    if (c === ';') {
      let j = i;
      while (j < text.length && text[j] !== '\n') out[j++] = ' ';
      i = j - 1;
    }
  }
  return out.join('');
}

export class Positions {
  private starts: number[] = [0];
  constructor(text: string) {
    for (let i = 0; i < text.length; i++) if (text[i] === '\n') this.starts.push(i + 1);
  }
  at(offset: number): { line: number; character: number } {
    let lo = 0;
    let hi = this.starts.length - 1;
    while (lo < hi) {
      const mid = (lo + hi + 1) >> 1;
      if (this.starts[mid] <= offset) lo = mid; else hi = mid - 1;
    }
    return { line: lo, character: offset - this.starts[lo] };
  }
  offset(line: number, character: number): number {
    return (this.starts[Math.min(line, this.starts.length - 1)] ?? 0) + character;
  }
}

/** The identifiers of (cleaned) text: @local labels and names with . too */
export function words(clean: string): Set<string> {
  const s = new Set<string>();
  for (const m of clean.matchAll(/[A-Za-z_@][A-Za-z0-9_]*(?:\.[A-Za-z_][A-Za-z0-9_]*)*/g)) {
    s.add(m[0]);
    if (m[0].includes('.')) for (const part of m[0].split('.')) s.add(part);
  }
  return s;
}

export function escapeRegex(s: string): string {
  return s.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}
