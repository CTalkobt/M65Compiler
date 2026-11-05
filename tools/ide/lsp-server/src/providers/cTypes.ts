import { AggregateInfo, CType, CTypes, MemberInfo } from '../indexer/symbolTypes';
import { aggregateOf } from '../indexer/cSymbolScanner';
import { WorkspaceIndex } from '../indexer/workspaceIndex';
import { escapeRegex } from '../indexer/text';

// The struct before a . or ->, without a compiler: the expression's chain
// (a.b->c[i].) read backwards; its head's type from the declarations before
// it in the file (locals, parameters), the file's or the workspace's
// variables, or a macro (<mega65.h>'s vic4 is ((volatile struct vic4_regs
// *)0xD000)); each member's type from its struct. Casts ((struct T *)p)->
// and (*p). work; calls do not (their return type is not looked up).

export interface MemberSite {
  agg: AggregateInfo;
  /** the name typed so far after the . or -> */
  prefix: string;
}

export class TypeResolver {
  constructor(private index: WorkspaceIndex, private doc: CTypes) {}

  private aggOf(t: CType | null): AggregateInfo | null {
    if (!t) return null;
    const r = aggregateOf(t, (tag) => this.index.findAggregate(tag, this.doc), (n) => this.index.findTypedef(n, this.doc));
    return r ? r.agg : null;
  }

  /** the struct whose members complete at offset (after "x." / "p->" and a partial name) */
  memberSite(clean: string, offset: number): MemberSite | null {
    // (the end of the text only: the regexes are anchored at its end)
    const from = Math.max(0, offset - 256);
    const before = clean.slice(from, offset);
    const m = /(\.|->)[ \t]*([A-Za-z_]\w*)?$/.exec(before);
    if (!m) return null;
    // (a number's point, 1.5, is not a member access)
    const lead = before.slice(0, m.index).trimEnd();
    if (m[1] === '.' && /\d$/.test(lead) && !/[A-Za-z_]\w*$/.test(lead)) return null;
    const agg = this.aggOf(this.exprType(clean, from + m.index));
    return agg ? { agg, prefix: m[2] ?? '' } : null;
  }

  /** the member a name at [start, end) names, when it follows . or -> */
  memberAt(clean: string, start: number, name: string): { agg: AggregateInfo; member: MemberInfo } | null {
    const site = this.memberSite(clean, start);
    if (!site) return null;
    const member = site.agg.members.find((x) => x.name === name);
    return member ? { agg: site.agg, member } : null;
  }

  /** the type of the expression ending at end (exclusive) */
  exprType(clean: string, end: number): CType | null {
    let i = end;
    const skipWs = () => { while (i > 0 && /\s/.test(clean[i - 1])) i--; };
    const chain: string[] = [];
    let head: CType | null = null;
    for (let guard = 0; guard < 64; guard++) {
      skipWs();
      const c = clean[i - 1];
      if (c === ']') {
        let depth = 0;
        for (i--; i >= 0; i--) {
          if (clean[i] === ']') depth++;
          else if (clean[i] === '[' && --depth === 0) break;
        }
        continue;
      }
      if (c === ')') {
        let depth = 0;
        let j = i - 1;
        for (; j >= 0; j--) {
          if (clean[j] === ')') depth++;
          else if (clean[j] === '(' && --depth === 0) break;
        }
        const inner = clean.slice(j + 1, i - 1);
        // (a call: name(...) — its type is not known)
        if (/[A-Za-z_]\w*$/.test(clean.slice(Math.max(0, j - 256), j).trimEnd())) return null;
        head = this.parenType(inner, clean, j + 1);
        break;
      }
      const id = /([A-Za-z_]\w*)$/.exec(clean.slice(0, i));
      if (!id) return null;
      i -= id[1].length;
      skipWs();
      if (clean.slice(0, i).endsWith('->')) { chain.push(id[1]); i -= 2; continue; }
      if (clean[i - 1] === '.') { chain.push(id[1]); i -= 1; continue; }
      head = this.identType(id[1], clean, i);
      break;
    }
    let t = head;
    for (let k = chain.length - 1; k >= 0 && t; k--) {
      const agg = this.aggOf(t);
      const mem = agg?.members.find((x) => x.name === chain[k]);
      t = mem ? { base: mem.base, ptr: mem.ptr } : null;
    }
    return t;
  }

  /** (cast)expr, (*p), (expr) */
  private parenType(inner: string, clean: string, at: number): CType | null {
    const cast = /^\s*\(\s*((?:struct|union)\s+[A-Za-z_]\w*|[A-Za-z_]\w*)((?:\s|\*|\bconst\b|\bvolatile\b)*)\)/.exec(inner);
    if (cast) {
      const base = cast[1].replace(/\s+/g, ' ');
      return { base, ptr: (cast[2].match(/\*/g) || []).length };
    }
    const deref = /^\s*\*+\s*/.exec(inner);
    const t = this.exprType(clean, at + inner.trimEnd().length);
    if (!t) return null;
    return deref ? { base: t.base, ptr: Math.max(0, t.ptr - (deref[0].match(/\*/g) || []).length) } : t;
  }

  /** a name's declared type: the last declaration before here, a variable, a macro */
  identType(name: string, clean: string, at: number): CType | null {
    const re = new RegExp(
      `(?:\\b(struct|union)\\s+([A-Za-z_]\\w*)|\\b([A-Za-z_]\\w*))((?:\\s|\\*|\\bconst\\b|\\bvolatile\\b|\\b__far\\b|\\b__near\\b|\\brestrict\\b)*)\\b${escapeRegex(name)}\\b(?=\\s*(?:\\[|=|;|,|\\)))`, 'g');
    let found: CType | null = null;
    for (const m of clean.slice(0, at).matchAll(re)) {
      const t: CType = { base: m[1] ? `${m[1]} ${m[2]}` : m[3], ptr: (m[4].match(/\*/g) || []).length };
      if (this.aggOf(t)) found = t;
    }
    if (found) return found;
    const v = this.index.findVar(name, this.doc);
    if (v && this.aggOf(v)) return v;
    const body = this.index.findMacro(name, this.doc);
    if (body) {
      const deref = /^\s*\(?\s*\*\s*\(/.test(body) ? 1 : 0;
      const s = /\b(struct|union)\s+([A-Za-z_]\w*)\s*(\**)/.exec(body);
      if (s) return { base: `${s[1]} ${s[2]}`, ptr: Math.max(0, s[3].length - deref) };
      const a = /\(\s*(?:(?:volatile|const)\s+)*([A-Za-z_]\w*)\s*(\*+)\s*\)/.exec(body);
      if (a && this.aggOf({ base: a[1], ptr: 0 })) return { base: a[1], ptr: Math.max(0, a[2].length - deref) };
    }
    return null;
  }
}
