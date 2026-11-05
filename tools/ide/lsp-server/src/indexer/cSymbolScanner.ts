import { AggregateInfo, CType, CTypes, MemberInfo, Range, Symbol, SymbolKind } from './symbolTypes';
import { cleanC, Positions } from './text';

// A C file's declarations, found without a compiler: the text with comments
// and literals blanked is walked by braces and semicolons at file scope.
// Functions (with their bodies' extent), prototypes, file-scope variables,
// structs / unions with their members (and cc45's struct methods), enums
// with their enumerators, typedefs and #defines; and the types: what each
// struct holds and what type each variable, typedef and member has, for
// member completion.

const NOT_NAMES = new Set([
  'if', 'while', 'for', 'switch', 'return', 'sizeof', 'do', 'else', 'case', 'default', 'goto',
  'break', 'continue', '_Alignof', 'typedef', 'struct', 'union', 'enum', '_Static_assert',
  'static_assert', '__builtin_offsetof', 'offsetof', 'repeat',
]);
const QUALIFIERS = /\b(static|extern|register|auto|inline|__inline__|__inline|const|volatile|restrict|__restrict|__restrict__|_Thread_local|__zp|__interrupt|__naked|__fastcall__|__regparm|__noescape|__pure|_Noreturn|__far|__near|__striped|signed_default|virtual|final|override|__unpacked)\b/g;
// specifiers with an argument list that say nothing about the type here
const ATTRIBUTE_CALLS = /\b(__attribute__|_Alignas|__section|__bank|__asm__|__declspec|__cpu|__flags)\s*\(/g;

interface Piece { text: string; map: number[] }

/** blank NAME(...) (balanced) for the attribute-like specifiers */
function blankCalls(t: string): string {
  const out = t.split('');
  for (const m of t.matchAll(ATTRIBUTE_CALLS)) {
    let i = (m.index ?? 0) + m[0].length - 1;
    const start = m.index ?? 0;
    let depth = 0;
    for (; i < t.length; i++) {
      if (t[i] === '(') depth++;
      else if (t[i] === ')' && --depth === 0) break;
    }
    for (let k = start; k <= i && k < t.length; k++) if (out[k] !== '\n') out[k] = ' ';
  }
  return out.join('');
}

function matchBrace(t: string, open: number): number {
  let depth = 0;
  for (let i = open; i < t.length; i++) {
    if (t[i] === '{') depth++;
    else if (t[i] === '}' && --depth === 0) return i;
  }
  return t.length - 1;
}

const oneLine = (s: string, max = 160) => {
  const r = s.replace(/\s+/g, ' ').trim();
  return r.length > max ? r.slice(0, max - 3) + '...' : r;
};

/** the function a header ends with: name(params) at its end */
function functionName(header: string): { name: string; index: number } | null {
  const h = header.trimEnd();   // (not /\s+$/: quadratic on long blank runs)
  if (!h.endsWith(')')) return null;
  // the parameter list's opening parenthesis
  let depth = 0;
  let i = h.length - 1;
  for (; i >= 0; i--) {
    if (h[i] === ')') depth++;
    else if (h[i] === '(' && --depth === 0) break;
  }
  if (i <= 0) return null;
  const m = /([A-Za-z_]\w*)$/.exec(h.slice(0, i).trimEnd());
  if (!m || NOT_NAMES.has(m[1])) return null;
  const before = h.slice(0, m.index);
  // (a function pointer variable: (*name)(...) — not a function)
  if (/\(\s*\*?$/.test(before.trimEnd())) return null;
  if (/[=]/.test(before)) return null;
  return { name: m[1], index: m.index };
}

/** split at top-level commas */
function splitTop(p: Piece, sep: string): Piece[] {
  const out: Piece[] = [];
  let depth = 0;
  let from = 0;
  for (let i = 0; i <= p.text.length; i++) {
    const c = p.text[i];
    if (c === '(' || c === '[' || c === '{') depth++;
    else if (c === ')' || c === ']' || c === '}') depth--;
    if (i === p.text.length || (c === sep && depth === 0)) {
      out.push({ text: p.text.slice(from, i), map: p.map.slice(from, i) });
      from = i + 1;
    }
  }
  return out;
}

interface Declarator { name: string; nameAt: number; ptr: number; array: boolean; fnptr: boolean }

/** one declarator: *name[3] = ..., (*fp)(int), name : 3 */
function declarator(p: Piece): { decl: Declarator; before: string } | null {
  let t = p.text;
  // (an initializer, a bit-field width)
  let depth = 0;
  for (let i = 0; i < t.length; i++) {
    const c = t[i];
    if (c === '(' || c === '[' || c === '{') depth++;
    else if (c === ')' || c === ']' || c === '}') depth--;
    else if ((c === '=' || c === ':') && depth === 0) { t = t.slice(0, i); break; }
  }
  const fp = /\(\s*(\*+)\s*([A-Za-z_]\w*)\s*\)/.exec(t);
  if (fp) {
    return {
      decl: { name: fp[2], nameAt: p.map[fp.index + fp[0].indexOf(fp[2])], ptr: fp[1].length, array: false, fnptr: true },
      before: t.slice(0, fp.index),
    };
  }
  let array = false;
  let s = t.trimEnd();
  while (s.endsWith(']')) {
    const o = s.lastIndexOf('[');
    if (o < 0) break;
    s = s.slice(0, o).trimEnd();
    array = true;
  }
  const m = /([A-Za-z_]\w*)$/.exec(s);
  if (!m) return null;
  const before = s.slice(0, m.index);
  return {
    decl: { name: m[1], nameAt: p.map[m.index], ptr: (before.match(/\*/g) || []).length, array, fnptr: false },
    before,
  };
}

function baseOf(before: string): string {
  return before.replace(/\*/g, ' ').replace(QUALIFIERS, ' ').replace(/\s+/g, ' ').trim();
}

/** a declaration's declarators (base: the specifiers when given, else read from the first) */
function declarators(p: Piece, base?: string): { base: string; list: Declarator[] } {
  const parts = splitTop(p, ',');
  const list: Declarator[] = [];
  let b = base ?? '';
  parts.forEach((part, k) => {
    const d = declarator(part);
    if (!d) return;
    if (k === 0 && base === undefined) {
      b = baseOf(d.before);
      if (!b) return;   // (no type: not a declaration)
    }
    if (NOT_NAMES.has(d.decl.name) || /^(int|char|long|short|unsigned|signed|void|float|double)$/.test(d.decl.name)) return;
    list.push(d.decl);
  });
  return { base: b, list };
}

export interface CScan {
  symbols: Symbol[];
  types: CTypes;
}

export class CSymbolScanner {
  scanFile(uri: string, content: string): Symbol[] {
    return this.scan(uri, content).symbols;
  }

  scan(uri: string, content: string): CScan {
    const pos = new Positions(content);
    const types: CTypes = { aggregates: [], typedefs: new Map(), vars: new Map(), macros: new Map() };
    const symbols: Symbol[] = [];
    let t = cleanC(content);

    // #define (and other directives, blanked)
    {
      const out = t.split('');
      const lines = t.split('\n');
      let off = 0;
      for (let li = 0; li < lines.length; li++) {
        const line = lines[li];
        if (/^\s*#/.test(line)) {
          let end = off + line.length;
          let text = line;
          let lj = li;
          while (/\\\s*$/.test(lines[lj]) && lj + 1 < lines.length) {
            lj++;
            text += '\n' + lines[lj];
            end += 1 + lines[lj].length;
          }
          const d = /^(\s*#\s*define\s+)([A-Za-z_]\w*)(\()?/.exec(text);
          if (d) {
            const nameOff = off + d[1].length;
            const body = text.slice(d[0].length - (d[3] ? 1 : 0)).replace(/\\\n/g, ' ');
            types.macros.set(d[2], body.trim());
            symbols.push(this.sym(d[2], d[3] ? SymbolKind.Function : SymbolKind.Constant, uri, pos, nameOff, {
              detail: oneLine(`#define ${d[2]}${body}`), full: [off, end],
            }));
          }
          for (let k = off; k < end; k++) if (out[k] !== '\n') out[k] = ' ';
          off = end + 1;
          li = lj;
          continue;
        }
        off += line.length + 1;
      }
      t = out.join('');
    }
    t = blankCalls(t);

    let stmt: Piece = { text: '', map: [] };
    let start = -1;
    let pending: { sym: Symbol; agg?: AggregateInfo; kind: string; tag: string } | null = null;
    const reset = () => { stmt = { text: '', map: [] }; start = -1; pending = null; };

    for (let i = 0; i < t.length; i++) {
      const c = t[i];
      if (start < 0) {
        if (c === ' ' || c === '\n' || c === '\t' || c === '\r') continue;   // (nothing before a statement)
        start = i;
      }
      if (c === '{') {
        const end = matchBrace(t, i);
        const fn = pending ? null : functionName(stmt.text);
        if (fn && !/^\s*typedef\b/.test(stmt.text)) {
          const header = stmt.text;
          symbols.push(this.sym(fn.name, SymbolKind.Function, uri, pos, stmt.map[fn.index], {
            detail: oneLine(header), full: [start, end + 1],
          }));
          // (a static function's name is the file's own)
          reset();
          i = end;
          continue;
        }
        const agg = /\b(struct|union|enum)(?:\s+([A-Za-z_]\w*))?$/.exec(stmt.text.trimEnd());
        if (agg && !pending) {
          const tagAt = agg[2] ? stmt.map[agg.index + agg[0].lastIndexOf(agg[2])] : i;
          pending = this.aggregate(agg[1], agg[2] ?? '', t, i, end, uri, pos, tagAt, start, types);
        }
        for (let k = 0; k < 4; k++) { stmt.text += k === 1 ? '{' : k === 2 ? '}' : ' '; stmt.map.push(i); }
        i = end;
        continue;
      }
      if (c === ';') {
        this.declaration(stmt, pending, uri, pos, start, i + 1, symbols, types);
        reset();
        continue;
      }
      if (c === '}') { reset(); continue; }
      stmt.text += c;
      stmt.map.push(i);
    }
    if (pending) this.declaration(stmt, pending, uri, pos, start, t.length, symbols, types);
    return { symbols, types };
  }

  /** struct/union/enum NAME { ... }: its symbol and members */
  private aggregate(kind: string, tag: string, t: string, open: number, close: number, uri: string, pos: Positions,
    tagAt: number, start: number, types: CTypes): { sym: Symbol; agg?: AggregateInfo; kind: string; tag: string } {
    const children: Symbol[] = [];
    let agg: AggregateInfo | undefined;
    if (kind === 'enum') {
      const body: Piece = { text: t.slice(open + 1, close), map: [] };
      for (let k = open + 1; k < close; k++) body.map.push(k);
      for (const part of splitTop(body, ',')) {
        const m = /^\s*([A-Za-z_]\w*)/.exec(part.text);
        if (m) {
          children.push(this.sym(m[1], SymbolKind.EnumMember, uri, pos, part.map[m.index + m[0].indexOf(m[1])], {
            detail: oneLine(part.text),
          }));
        }
      }
    } else {
      agg = { tag, kind: kind as 'struct' | 'union', members: [], uri, line: pos.at(tagAt).line };
      this.members(t, open + 1, close, agg, children, uri, pos, types);
      if (tag) types.aggregates.push(agg);
    }
    const sym = this.sym(tag || `(anonymous ${kind})`, kind === 'enum' ? SymbolKind.Enum : SymbolKind.Struct, uri, pos, tagAt, {
      detail: `${kind} ${tag}`.trim(), full: [start < 0 ? open : start, close + 1], children,
    });
    return { sym, agg, kind, tag };
  }

  /** the members of a struct/union body (nested ones: anonymous members hoisted) */
  private members(t: string, from: number, to: number, agg: AggregateInfo, children: Symbol[], uri: string,
    pos: Positions, types: CTypes): void {
    let cur: Piece = { text: '', map: [] };
    let nested: { agg?: AggregateInfo; sym: Symbol; kind: string; tag: string } | null = null;
    const flush = () => {
      const text = cur.text;
      if (text.trim()) {
        const fn = functionName(text);
        if (fn && !nested) {
          // a struct method (cc45): name(params)
          const before = text.slice(0, fn.index);
          const m: MemberInfo = {
            name: fn.name, base: baseOf(before), ptr: (before.match(/\*/g) || []).length, method: true,
            decl: oneLine(text), ...pos.at(cur.map[fn.index]),
          };
          agg.members.push(m);
          children.push(this.sym(fn.name, SymbolKind.Method, uri, pos, cur.map[fn.index], { detail: m.decl }));
        } else if (nested) {
          const rest: Piece = { text: text.slice(text.lastIndexOf('}') + 1), map: cur.map.slice(text.lastIndexOf('}') + 1) };
          const ntag = nested.tag || `${agg.tag}.${nested.kind}@${nested.sym.location.range.start.line}`;
          if (nested.agg && !nested.tag) { nested.agg.tag = ntag; types.aggregates.push(nested.agg); }
          const { list } = declarators(rest, `${nested.kind} ${ntag}`);
          if (list.length === 0 && nested.agg) {
            // (an anonymous struct/union member: its members are the outer one's)
            agg.members.push(...nested.agg.members);
            children.push(...(nested.sym.children ?? []));
          }
          for (const d of list) this.member(d, `${nested.kind} ${ntag}`, text, agg, children, uri, pos);
        } else {
          const { base, list } = declarators(cur);
          for (const d of list) this.member(d, base, text, agg, children, uri, pos);
        }
      }
      cur = { text: '', map: [] };
      nested = null;
    };
    for (let i = from; i < to; i++) {
      const c = t[i];
      if (c === '{') {
        const end = matchBrace(t, i);
        const fn = nested ? null : functionName(cur.text);
        if (fn) {
          // a method with its body
          flush();
          i = end;
          continue;
        }
        const m = /\b(struct|union|enum)(?:\s+([A-Za-z_]\w*))?$/.exec(cur.text.trimEnd());
        if (m && !nested) {
          const tagAt = m[2] ? cur.map[m.index + m[0].lastIndexOf(m[2])] : i;
          nested = this.aggregate(m[1], m[2] ?? '', t, i, end, uri, pos, tagAt, i, types);
        }
        cur.text += ' {} ';
        for (let k = 0; k < 4; k++) cur.map.push(i);
        i = end;
        continue;
      }
      if (c === ';') { flush(); continue; }
      cur.text += c;
      cur.map.push(i);
    }
    flush();
  }

  private member(d: Declarator, base: string, text: string, agg: AggregateInfo, children: Symbol[], uri: string, pos: Positions): void {
    const m: MemberInfo = {
      name: d.name, base, ptr: d.ptr + (d.array ? 1 : 0), method: false,
      decl: oneLine(text.includes('{}') ? `${base} ${'*'.repeat(d.ptr)}${d.name}${d.array ? '[]' : ''}` : text),
      ...pos.at(d.nameAt),
    };
    agg.members.push(m);
    children.push(this.sym(d.name, SymbolKind.Field, uri, pos, d.nameAt, { detail: m.decl }));
  }

  /** a file-scope declaration ending at ; */
  private declaration(stmt: Piece, pending: { sym: Symbol; agg?: AggregateInfo; kind: string; tag: string } | null,
    uri: string, pos: Positions, start: number, end: number, symbols: Symbol[], types: CTypes): void {
    const text = stmt.text;
    const isTypedef = /^\s*typedef\b/.test(text);
    const extern = /^\s*extern\b/.test(text);
    let rest: Piece = stmt;
    let base: string | undefined;
    if (pending) {
      const k = text.indexOf('{}');
      rest = { text: text.slice(k + 2), map: stmt.map.slice(k + 2) };
      base = `${pending.kind} ${pending.tag}`;
    }
    if (isTypedef) {
      if (!pending) rest = { text: text.replace(/^\s*typedef\b/, (m) => ' '.repeat(m.length)), map: stmt.map };
      const { base: b, list } = declarators(rest, base);
      if (pending && !pending.tag && list.length > 0) {
        // typedef struct { ... } name: the struct known by the typedef's name
        pending.tag = list[0].name;
        if (pending.agg) { pending.agg.tag = list[0].name; types.aggregates.push(pending.agg); }
        pending.sym.name = list[0].name;
      }
      if (pending) symbols.push(pending.sym);
      const typeBase = pending ? `${pending.kind} ${pending.tag}` : b;
      for (const d of list) {
        types.typedefs.set(d.name, { base: typeBase, ptr: d.ptr });
        if (pending && d.name === pending.tag && d.ptr === 0) continue;   // (the struct's own symbol names it)
        symbols.push(this.sym(d.name, pending && pending.kind !== 'enum' ? SymbolKind.Struct : SymbolKind.TypeParameter, uri, pos, d.nameAt, {
          detail: oneLine(text.replace('{}', '{ ... }')), full: [start, end],
        }));
      }
      return;
    }
    if (pending) symbols.push(pending.sym);
    if (!pending) {
      const fn = functionName(text);
      if (fn) {
        symbols.push(this.sym(fn.name, SymbolKind.Function, uri, pos, stmt.map[fn.index], {
          detail: oneLine(text), full: [start, end], definition: false,
        }));
        return;
      }
    }
    const { base: b, list } = declarators(rest, base);
    for (const d of list) {
      types.vars.set(d.name, { base: b, ptr: d.ptr + (d.array ? 1 : 0) });
      symbols.push(this.sym(d.name, SymbolKind.Variable, uri, pos, d.nameAt, {
        detail: oneLine(pending ? `${b} ${'*'.repeat(d.ptr)}${d.name}` : text), full: [start, end],
        definition: !extern, scope: extern ? 'extern' : 'global',
      }));
    }
  }

  private sym(name: string, kind: SymbolKind, uri: string, pos: Positions, at: number,
    o: { detail?: string; full?: [number, number]; children?: Symbol[]; definition?: boolean; scope?: 'global' | 'extern' } = {}): Symbol {
    const s = pos.at(at);
    const range: Range = { start: s, end: { line: s.line, character: s.character + name.length } };
    const sym: Symbol = {
      name, kind, location: { uri, range }, isDefinition: o.definition ?? true, scope: o.scope ?? 'global',
    };
    if (o.detail) sym.detail = o.detail;
    if (o.full) sym.fullRange = { start: pos.at(o.full[0]), end: pos.at(Math.max(o.full[0], o.full[1])) };
    if (o.children && o.children.length) sym.children = o.children;
    return sym;
  }
}

/** a type's aggregate: typedefs followed to a struct/union */
export function aggregateOf(type: CType, find: (tag: string) => AggregateInfo | undefined,
  typedef: (name: string) => CType | undefined): { agg: AggregateInfo; ptr: number } | null {
  let base = type.base;
  let ptr = type.ptr;
  for (let k = 0; k < 8; k++) {
    const m = /^(struct|union)\s+(\S+)$/.exec(base);
    if (m) {
      const a = find(m[2]);
      return a ? { agg: a, ptr } : null;
    }
    const td = typedef(base.split(' ').pop() || base);
    if (!td) return null;
    base = td.base;
    ptr += td.ptr;
  }
  return null;
}
