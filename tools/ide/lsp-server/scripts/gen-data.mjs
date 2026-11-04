#!/usr/bin/env node
// gen-data.mjs — the LSP server's hover / completion tables, generated from
// the suite's own sources so they say what the tools do (#198):
//
//   data/opcodes.json          ca45's instructions and addressing modes
//                              (src/main/AssemblerOpcodeDatabase.cpp), the
//                              quad ones in the MEGA65 Book's modes, the
//                              40 MHz cycles (src/main/CpuCycles.cpp), and
//                              the simulated instructions (doc/bin/ca45.md)
//   data/directives.json       ca45's directives (doc/bin/ca45.md's tables)
//   data/pragmas.json          cc45's pragmas (doc/bin/cc45.md's table)
//   data/stdlib-functions.json the library's functions: the prototypes of
//                              lib/include/*.h that lib/build/c45.lib defines
//                              (bin/ar45 s), with the comment above each
//
//   node scripts/gen-data.mjs [repository root]   (npm run gen-data)
//
// data/c-keywords.json is written by hand.
import fs from 'fs';
import path from 'path';
import { execFileSync } from 'child_process';
import { fileURLToPath } from 'url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(process.argv[2] || path.join(HERE, '..', '..', '..', '..'));
const DATA = path.join(HERE, '..', 'data');
const read = (p) => fs.readFileSync(path.join(ROOT, p), 'utf-8');
const write = (name, obj) => {
  fs.writeFileSync(path.join(DATA, name), JSON.stringify(obj, null, 1) + '\n');
  console.log(`${name}: ${Object.values(obj)[1]?.length ?? ''} entries`);
};

// ---------------------------------------------------------------- opcodes

const MODE_TEXT = {
  IMPLIED: '', ACCUMULATOR: 'A', IMMEDIATE: '#imm', IMMEDIATE16: '#imm16',
  BASE_PAGE: 'zp', BASE_PAGE_X: 'zp,X', BASE_PAGE_Y: 'zp,Y',
  ABSOLUTE: 'abs', ABSOLUTE_X: 'abs,X', ABSOLUTE_Y: 'abs,Y',
  INDIRECT: '(zp)', ABSOLUTE_INDIRECT: '(abs)', ABSOLUTE_X_INDIRECT: '(abs,X)',
  BASE_PAGE_X_INDIRECT: '(zp,X)', BASE_PAGE_INDIRECT_Y: '(zp),Y', BASE_PAGE_INDIRECT_Z: '(zp),Z',
  BASE_PAGE_INDIRECT_SP_Y: '(zp,SP),Y', FLAT_INDIRECT_Z: '[zp],Z',
  RELATIVE: 'label', RELATIVE16: 'label (16-bit)', BASE_PAGE_RELATIVE: 'zp, label', STACK_RELATIVE: 'n,SP',
};

// what each instruction does and the flags it sets (the MEGA65 Book)
const INFO = {
  adc: ['Add memory to A with carry', 'N V Z C'], and: ['AND memory with A', 'N Z'],
  asl: ['Arithmetic shift left', 'N Z C'], asr: ['Arithmetic shift right', 'N Z C'],
  asw: ['Arithmetic shift left a word in memory', 'N Z C'], bbr: ['Branch if a bit of a base-page byte is 0', ''],
  bbs: ['Branch if a bit of a base-page byte is 1', ''], bcc: ['Branch if carry clear', ''],
  bcs: ['Branch if carry set', ''], beq: ['Branch if equal (Z set)', ''], bit: ['Test bits of memory with A', 'N V Z'],
  bmi: ['Branch if minus (N set)', ''], bne: ['Branch if not equal (Z clear)', ''], bpl: ['Branch if plus (N clear)', ''],
  bra: ['Branch always', ''], brk: ['Break: software interrupt', 'B I'], bsr: ['Branch to subroutine (relative, 16-bit)', ''],
  bvc: ['Branch if overflow clear', ''], bvs: ['Branch if overflow set', ''], clc: ['Clear carry', 'C'],
  cld: ['Clear decimal mode', 'D'], cle: ['Clear the stack-extend flag: 16-bit stack pointer', 'E'],
  cli: ['Clear interrupt disable', 'I'], clv: ['Clear overflow', 'V'], cmp: ['Compare memory with A', 'N Z C'],
  cpx: ['Compare memory with X', 'N Z C'], cpy: ['Compare memory with Y', 'N Z C'], cpz: ['Compare memory with Z', 'N Z C'],
  dec: ['Decrement memory or A', 'N Z'], dew: ['Decrement a word in memory', 'N Z'], dex: ['Decrement X', 'N Z'],
  dey: ['Decrement Y', 'N Z'], dez: ['Decrement Z', 'N Z'], eom: ['End of mapping sequence (no operation; before (zp),Z: [zp],Z)', ''],
  eor: ['Exclusive-OR memory with A', 'N Z'], inc: ['Increment memory or A', 'N Z'], inw: ['Increment a word in memory', 'N Z'],
  inx: ['Increment X', 'N Z'], iny: ['Increment Y', 'N Z'], inz: ['Increment Z', 'N Z'], jmp: ['Jump', ''],
  jsr: ['Jump to subroutine', ''], lda: ['Load A', 'N Z'], ldx: ['Load X', 'N Z'], ldy: ['Load Y', 'N Z'], ldz: ['Load Z', 'N Z'],
  lsr: ['Logical shift right', 'N Z C'], map: ['Set the memory map from A, X, Y, Z', ''], neg: ['Negate A (two\'s complement)', 'N Z'],
  ora: ['OR memory with A', 'N Z'], pha: ['Push A', ''], php: ['Push the processor status', ''], phw: ['Push a word', ''],
  phx: ['Push X', ''], phy: ['Push Y', ''], phz: ['Push Z', ''], pla: ['Pull A', 'N Z'], plp: ['Pull the processor status', 'all'],
  plx: ['Pull X', 'N Z'], ply: ['Pull Y', 'N Z'], plz: ['Pull Z', 'N Z'], rmb: ['Reset (clear) a bit of a base-page byte', ''],
  rol: ['Rotate left through carry', 'N Z C'], ror: ['Rotate right through carry', 'N Z C'], row: ['Rotate a word in memory left', 'N Z C'],
  rti: ['Return from interrupt', 'all'], rts: ['Return from subroutine (rts #n: also n bytes off the stack)', ''],
  sbc: ['Subtract memory from A with borrow', 'N V Z C'], sec: ['Set carry', 'C'], sed: ['Set decimal mode', 'D'],
  see: ['Set the stack-extend flag: 8-bit stack pointer', 'E'], sei: ['Set interrupt disable', 'I'],
  smb: ['Set a bit of a base-page byte', ''], sta: ['Store A', ''], stx: ['Store X', ''], sty: ['Store Y', ''], stz: ['Store Z', ''],
  tab: ['Transfer A to the base-page register B', ''], tax: ['Transfer A to X', 'N Z'], tay: ['Transfer A to Y', 'N Z'],
  taz: ['Transfer A to Z', 'N Z'], tba: ['Transfer B to A', 'N Z'], trb: ['Test and reset memory bits with A', 'Z'],
  tsb: ['Test and set memory bits with A', 'Z'], tsx: ['Transfer the stack pointer (low) to X', 'N Z'],
  tsy: ['Transfer the stack pointer (high) to Y', 'N Z'], txa: ['Transfer X to A', 'N Z'], txs: ['Transfer X to the stack pointer (low)', ''],
  tya: ['Transfer Y to A', 'N Z'], tys: ['Transfer Y to the stack pointer (high)', ''], tza: ['Transfer Z to A', 'N Z'],
  ldq: ['Load Q (A, X, Y, Z: 32 bits; NEG NEG prefix)', 'N Z'], stq: ['Store Q (32 bits)', ''],
  adcq: ['Add memory to Q with carry (32 bits)', 'N V Z C'], sbcq: ['Subtract memory from Q with borrow (32 bits)', 'N V Z C'],
  andq: ['AND memory with Q (32 bits)', 'N Z'], orq: ['OR memory with Q (32 bits)', 'N Z'], eorq: ['Exclusive-OR memory with Q (32 bits)', 'N Z'],
  cmpq: ['Compare memory with Q (32 bits)', 'N Z C'], bitq: ['Test bits of memory with Q (32 bits)', 'N V Z'],
  aslq: ['Arithmetic shift left Q or memory (32 bits)', 'N Z C'], lsrq: ['Logical shift right Q or memory (32 bits)', 'N Z C'],
  rolq: ['Rotate Q or memory left (32 bits)', 'N Z C'], rorq: ['Rotate Q or memory right (32 bits)', 'N Z C'],
  asrq: ['Arithmetic shift right Q or memory (32 bits)', 'N Z C'], inq: ['Increment Q or memory (32 bits)', 'N Z'],
  deq: ['Decrement Q or memory (32 bits)', 'N Z'], negq: ['Negate Q (32 bits)', 'N Z'],
};

function parseArray(src, name) {
  const m = new RegExp(`${name}\\[256\\]\\s*=\\s*\\{([^}]*)\\}`).exec(src);
  if (!m) throw new Error(`${name} not found`);
  return m[1].replace(/\/\/[^\n]*/g, '').split(',').map((x) => x.trim()).filter(Boolean).map(Number);
}

function opcodes() {
  const db = read('src/main/AssemblerOpcodeDatabase.cpp');
  const fast = parseArray(read('src/main/CpuCycles.cpp'), 'k45Fast');
  const byM = new Map();
  for (const m of db.matchAll(/\{\{"([a-z0-9]+)",\s*AddressingMode::([A-Z0-9_]+)\},\s*0x([0-9A-Fa-f]{2})\}/g)) {
    const [, mn, mode, hex] = m;
    if (!byM.has(mn)) byM.set(mn, []);
    byM.get(mn).push({ mode, opcode: parseInt(hex, 16) });
  }
  // the quad instructions: NEG NEG and the 8-bit one, in the book's modes
  // (ca45's AssemblerOpcodeDatabase quadModeAllowed)
  const q = (base, modes) => (byM.get(base) || []).filter((e) => modes.includes(e.mode));
  const MEM = ['BASE_PAGE', 'ABSOLUTE', 'BASE_PAGE_INDIRECT_Z', 'FLAT_INDIRECT_Z'];
  const SHIFT = ['ACCUMULATOR', 'BASE_PAGE', 'ABSOLUTE', 'BASE_PAGE_X', 'ABSOLUTE_X'];
  const quads = {
    adcq: q('adc', MEM), sbcq: q('sbc', MEM), andq: q('and', MEM), orq: q('ora', MEM), eorq: q('eor', MEM), cmpq: q('cmp', MEM),
    bitq: q('bit', ['BASE_PAGE', 'ABSOLUTE']), aslq: q('asl', SHIFT), lsrq: q('lsr', SHIFT), rolq: q('rol', SHIFT), rorq: q('ror', SHIFT),
    inq: q('inc', SHIFT), deq: q('dec', SHIFT), asrq: q('asr', ['ACCUMULATOR', 'BASE_PAGE', 'BASE_PAGE_X']),
  };
  const out = [];
  const entry = (mn, list, prefix) => {
    const base = mn.replace(/[0-7]$/, '');
    const [description, flags] = INFO[mn] || INFO[base] || ['', ''];
    const seen = new Set();
    const modes = [];
    for (const e of list) {
      const text = MODE_TEXT[e.mode] ?? e.mode;
      if (seen.has(text)) continue;
      seen.add(text);
      modes.push({ mode: text, opcode: e.opcode, bytes: prefix + e.opcode.toString(16).toUpperCase().padStart(2, '0'),
        cycles40: fast[e.opcode] || null });
    }
    const order = Object.values(MODE_TEXT);
    modes.sort((a, b) => order.indexOf(a.mode) - order.indexOf(b.mode));
    out.push({ mnemonic: mn.toUpperCase(), description, flags: flags ? flags.split(' ') : [], modes });
  };
  for (const [mn, list] of [...byM.entries()].sort()) {
    if (mn === 'ldq' || mn === 'stq') entry(mn, list.filter((e) => MEM.includes(e.mode)), '42 42 ');
    else entry(mn, list, '');
  }
  for (const [mn, list] of Object.entries(quads)) entry(mn, list, '42 42 ');
  out.push({ mnemonic: 'NEGQ', description: INFO.negq[0], flags: ['N', 'Z'], modes: [] });
  // the simulated instructions (ca45.md's "Simulated Instructions" tables)
  const md = read('doc/bin/ca45.md');
  const sim = md.slice(md.indexOf('## Simulated Instructions'), md.indexOf('## Register References'));
  const simulated = [];
  for (const line of sim.split('\n')) {
    const m = /^\| (`[^|]+`(?:\s*(?:\/|,|or)\s*`[^|]+`)*[^|]*)\|(.*)\|\s*$/.exec(line);
    if (!m || /^-+$/.test(m[1].trim())) continue;
    const names = [...m[1].matchAll(/`([a-z][a-z0-9.]*)[^`]*`/g)].map((x) => x[1]);
    const cells = m[2].split('|').map((c) => c.trim()).filter(Boolean);
    for (const n of names) {
      if (simulated.some((s) => s.mnemonic === n.toUpperCase()) || INFO[n]) continue;
      simulated.push({ mnemonic: n.toUpperCase(), description: cells.join(' — ').replace(/`/g, ''), flags: [], modes: [],
        simulated: true, syntax: m[1].trim().replace(/`/g, '') });
    }
  }
  return { generatedFrom: 'src/main/AssemblerOpcodeDatabase.cpp, src/main/CpuCycles.cpp, doc/bin/ca45.md', opcodes: [...out, ...simulated] };
}

// ------------------------------------------------------------- directives

function directives() {
  const md = read('doc/bin/ca45.md');
  const out = [];
  const seen = new Set();
  for (const line of md.split('\n')) {
    const m = /^\| (`\.[^|]+)\|(.*)\|\s*$/.exec(line);
    if (!m) continue;
    const forms = m[1].trim();
    const description = m[2].split('|').map((c) => c.trim()).filter(Boolean).join(' — ').replace(/`/g, '');
    for (const x of forms.matchAll(/`\.([a-zA-Z_][a-zA-Z0-9_]*)([^`]*)`/g)) {
      const name = x[1].toLowerCase();
      if (seen.has(name)) continue;
      seen.add(name);
      out.push({ name, description, syntax: `.${x[1]}${x[2]}`.trim(), example: forms.replace(/`/g, '') });
    }
  }
  return { generatedFrom: 'doc/bin/ca45.md', directives: out };
}

// ---------------------------------------------------------------- library

function libraryExports() {
  const ar = path.join(ROOT, 'bin', 'ar45');
  const lib = path.join(ROOT, 'lib', 'build', 'c45.lib');
  if (!fs.existsSync(ar) || !fs.existsSync(lib)) {
    throw new Error('bin/ar45 and lib/build/c45.lib are needed (make && make lib)');
  }
  const set = new Set();
  for (const line of execFileSync(ar, ['s', lib], { encoding: 'utf-8' }).split('\n')) {
    const m = /^(\S+)\s+\[/.exec(line);
    if (m) set.add(m[1]);
  }
  return set;
}

function stdlib() {
  const exported = libraryExports();
  const dir = path.join(ROOT, 'lib', 'include');
  const out = [];
  const seen = new Set();
  for (const header of fs.readdirSync(dir).filter((f) => f.endsWith('.h')).sort()) {
    const src = fs.readFileSync(path.join(dir, header), 'utf-8').replace(/\r/g, '');
    // a declaration at the start of a line, up to its ';' (several lines), and
    // a /* ... */ or // comment just above it
    const re = /((?:\/\*(?:[^*]|\*(?!\/))*\*\/[ \t]*\n|(?:\/\/[^\n]*\n)+)?)^([A-Za-z_][^;{}()#=]*?\b([A-Za-z_]\w*)\s*\(([^;{}]*?)\)[^;{}]*;)/gm;
    for (const m of src.matchAll(re)) {
      const [, comment, decl, name] = m;
      if (seen.has(name) || /^\s*(typedef|return|if|while|for|switch|static\s+inline)\b/.test(decl)) continue;
      if (!exported.has('_' + name) && !exported.has(name)) continue;
      seen.add(name);
      const doc = (comment || '').replace(/^\s*\/\*+\s?|\s*\*+\/\s*$/g, '').replace(/^\s*\/\/\s?/gm, '')
        .split('\n').map((l) => l.replace(/^\s*\*\s?/, '')).join(' ').replace(/\s+/g, ' ').trim();
      out.push({ name, header, signature: decl.replace(/\s+/g, ' ').replace(/\s*;$/, '').trim(), doc });
    }
  }
  // the names programs call: #define unpack __unpack, #define dma_copy_far(d, s, n) __dma_copy_far(...)
  const byName = new Map(out.map((f) => [f.name, f]));
  for (const header of fs.readdirSync(dir).filter((f) => f.endsWith('.h')).sort()) {
    const src = fs.readFileSync(path.join(dir, header), 'utf-8');
    for (const m of src.matchAll(/^#define\s+([A-Za-z_]\w*)(\([^)]*\))?\s+\(?\s*([A-Za-z_]\w*)\b/gm)) {
      const [, alias, params, target] = m;
      const f = byName.get(target);
      if (!f || seen.has(alias)) continue;
      seen.add(alias);
      const sig = f.signature.replace(new RegExp(`\\b${target}\\b`), alias);
      out.push({ name: alias, header, signature: params ? `${sig}  /* macro ${alias}${params} */` : sig, doc: f.doc });
    }
  }
  out.sort((a, b) => a.name.localeCompare(b.name));
  return { generatedFrom: 'lib/include/*.h, lib/build/c45.lib', functions: out };
}

// ---------------------------------------------------------------- pragmas

function pragmas() {
  const md = read('doc/bin/cc45.md');
  const sec = md.slice(md.indexOf('## Pragmas'), md.indexOf('## Type System'));
  const out = [];
  for (const line of sec.split('\n')) {
    const m = /^\| (`#pragma[^|]*?)\s*\|(.*)\|\s*$/.exec(line.replace(/\\\|/g, '/'));
    if (!m) continue;
    const description = m[2].trim().replace(/`/g, '').replace(/\[([^\]]+)\]\([^)]*\)/g, '$1');
    // `#pragma cc45 a` / `b`: b is #pragma cc45 b
    let prefix = '';
    for (const t of m[1].matchAll(/`([^`]*)`/g)) {
      let f = /^#pragma ((?:cc45 )?)([a-z_0-9]+)(.*)$/.exec(t[1]);
      if (f) prefix = f[1];
      else f = /^()([a-z_0-9]+)(.*)$/.exec(t[1]);
      if (!f) continue;
      const name = (f[1] || prefix) + f[2];
      if (out.some((p) => p.name === name)) continue;
      out.push({ name, syntax: `#pragma ${name}${f[3]}`.trim(), description });
    }
  }
  const noEffect = /Accepted but currently without effect[^:]*:([^.]*)\./.exec(sec);
  if (noEffect) {
    for (const x of noEffect[1].matchAll(/`([a-z_0-9]+)[^`]*`/g)) {
      const name = `cc45 ${x[1]}`;
      if (!out.some((p) => p.name === name)) out.push({ name, syntax: `#pragma ${name}`, description: 'Accepted, without effect.' });
    }
  }
  return { generatedFrom: 'doc/bin/cc45.md', pragmas: out };
}

fs.mkdirSync(DATA, { recursive: true });
write('pragmas.json', pragmas());
write('opcodes.json', opcodes());
write('directives.json', directives());
write('stdlib-functions.json', stdlib());
