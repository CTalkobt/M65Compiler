# Linters: al45 (assembly) and cl45 (C)

Status: the shared core and al45's phases 1-2 are implemented
(`doc/bin/al45.md`); al45's phase 3 and cl45 are planned (#199, #278, #64). The assembly half was planned on 2026-10-31; the C half on
2026-11-12.

Two tools on one core:

| Tool | What it is | Parses with |
|---|---|---|
| `al45` | a symlink to ca45, chosen by program name | ca45's front end (preprocessor, `#include`, macros, pass 1); no code emitted |
| `cl45` | a symlink to cc45, chosen by program name (as `cp45` is) | cc45's front end and IR builder; no assembly emitted |

Both read `.cc45-lint.yml` (#278), report rules by stable `mega65-*` ids,
honour inline suppression and write text, JSON or SARIF.

## Shared core

`include/Lint.hpp`, `src/main/Lint*.cpp` (in `lib45`, linked by ca45 and
cc45):

- `LintRule { id, defaultSeverity, profile, summary, explanation }`, a
  registry the tools fill with their rules; `Diagnostic { file, line, col,
  severity, ruleId, message }`.
- Severities `off` / `info` / `warning` / `error`; a run exits 1 when any
  diagnostic is an error.
- **Configuration**: a YAML subset reader (no dependencies: maps, lists,
  scalars, comments, quoted strings: what #278's schema uses).
  ```yaml
  profile: cc45                      # or standalone
  c_checks:
    enabled: "mega65-*,-mega65-style-*"
    options:
      mega65-stack-frame-limit: 256
  asm_rules:
    - id: mega65-dead-store
      severity: warning
    - id: mega65-dma-trigger-order
      severity: error
      message: "DMA trigger ($D700) must be written last"
  overrides:
    mega65-flag-stale: error
  ```
  `enabled` is a clang-tidy style list: globs, `-` to remove, applied in
  order. `asm_rules` entries configure built-in rules by id (severity, an
  optional message); a `pattern:` key for user-written rules is a later
  extension and is rejected with a message for now.
- **Precedence**: inline suppression > command line (`--checks=LIST`,
  `--disable=ID[,ID]`, `--profile=`) > `./.cc45-lint.yml` (searched upward
  from the source file) > `~/.config/m65/cc45-lint.yml` > built-in defaults.
- **Suppression**: `; NOLINT`, `; NOLINT(id[,id])`, `; NOLINTNEXTLINE(id)` in
  assembly; `// NOLINT(...)`, `/* NOLINT(...) */` and
  `#pragma cc45 lint(ignore, id)` (to the end of the file, or to
  `#pragma cc45 lint(restore, id)`) in C.
- **Output**: `file:line:col: warning: message [mega65-id]` (the format of
  the tools' errors), `--format=json` (an array of diagnostics),
  `--format=sarif` (SARIF 2.1.0, one run, the rules listed with their
  summaries).
- `--list-rules` (id, default severity, profile, summary), `--explain=ID`.

**Profiles**: `cc45` (code linked with c45.lib: the parameter block, the
library temporaries, `__bp_rom` around ROM calls, declared clobbers) or
`standalone` (flat binaries, demos: CPU and hardware rules only). The
default: `cc45` for a relocatable object (`-c`: ln45 links it with crt0
and c45.lib) and for C; `standalone` for a flat binary.

The zero-page ranges the `cc45` profile uses are the ABI's
(`calling-conventions.md` 6.1): `$08`-`$0F` scratch, `$10`-`$2F` the
parameter block, `$30`-`$7F` the register pool, `$80`-`$F1` functions'
base-page blocks, `$F2`-`$F5` the 32-bit temporary, `$F6`-`$FC` the library
temporaries, `$FD`/`$FE` the frame pointer.

## al45: assembly

Primary audience: **hand-written assembly**. cc45's output is a second
corpus, not the design target.

- **No reliance on cc45 metadata.** `proc`/`endproc`, `.func_flags`,
  `.reg_clobbers` / `.flag_clobbers` / `.zp_*`, `.loc`, `.zar`, `cc45.fp`
  enrich the analysis when present. Without `proc`, a function's entry is a
  `jsr` target, an exported label or the file's start / `.org`, and its
  extent what is reachable from the entry.
- **No reliance on ca45's optimizer.** Rules that overlap `-P` passes have
  their own analysis: the passes assume cc45's code shapes and rewrite
  rather than report; hand-written code is often assembled without them.
- **Idioms are not errors.** Falling through between routines, tail jumps,
  `bit $xxxx` to skip bytes, self-modified operands, data inline after
  `jsr`: the analysis stops there (state unknown) instead of reporting.

### Phase 1: foundation: done
`ca45_main.cpp`: program name `al45` → lint mode (`Logger::setTool("al45")`),
ca45's parsing options accepted (`-D`, `-I`, `.cpu`); the Makefile's
`bin/al45` symlink, install, man page; `doc/bin/al45.md`; a
`src/test/test_al45.sh` step with fixtures in `src/test-resources/al45/`
(each with its expected diagnostics).

### Phase 2: local rules (statements and labels, no data flow): done

| Rule | Profile | Default | What |
|---|---|---|---|
| `mega65-unreachable` | all | warning | code after `jmp`/`bra`/`rts`/`rti`/`rtn` with no label |
| `mega65-unused-label` | all | info | a local label never referenced |
| `mega65-no-bp-mode` | all | warning | `ldz $xx`, `lda/sta $xx,y`, `jmp ($xx)`, `phw $00xx` (no base-page form: ca45's warning as a rule) |
| `mega65-page1-stack` | cc45 | warning | `$0100,x` stack reads under the 16-bit stack |
| `mega65-dma-trigger-order` | all | warning | `$D700`/`$D705` written before the list address bytes in the same straight-line code |
| `mega65-io-personality` | standalone | info | VIC-III/IV registers written with no `$47 $53 → $D02F` knock in the file |
| `mega65-rom-call-bp` | cc45 | warning | `jsr` to `$E000`+ / KERNAL vectors not bracketed by `__bp_rom`/`__bp_prog` or `__rom_call` |
| `mega65-unnamed-proc-attrs` | all | warning | function attributes in an unnamed `proc` |
| `mega65-style-*` | all | off | columns, comment spacing, naming (regexes in the config): later |

### Phase 3: flow analysis
A per-flag effect table for every 45GS02 opcode (Q forms and ca45's
simulated ops included: registers and flags read and written, memory,
stack delta), per-function CFGs, backward liveness (A/X/Y/Z, flags,
base-page bytes), push/pull depth. Calls: a callee's `proc` attributes,
else al45's own summary of a routine in the file, else everything
clobbered. Rules: `mega65-zp-param-block` and `mega65-lib-temps` (cc45 profile: a
value kept in `$10`-`$2F` / `$F6`-`$FC` across a call, which may change
them; a store there alone is legitimate, so these need liveness, not a
pattern), `mega65-flag-stale`, `mega65-stack-balance`,
`mega65-dead-reg-write`, `mega65-redundant-load`, `mega65-dead-store`,
`mega65-clobber-mismatch` (with `--suggest-clobbers`). Unknown control flow
(`jmp (...)`, computed targets, self-modified operands) makes the state
unknown.

## cl45: C

Rules that need what cc45 knows (types, frames, which addresses are I/O,
the call graph of the file) and that the compiler does not already handle.

### How it runs
`cc45_main.cpp`: program name `cl45` → lint mode: cc45's options (`-I`,
`-D`, `-f...`, `-O<n>`), the preprocessor and parser as for compiling, the
AST checks, then IRBuilder (and, for the rules that need it, the IR passes:
frame sizes, the call graph); no code generation. A rule is an `ASTWalker`
(`AST.hpp`) or a pass over the IR with the source locations both carry.
Diagnostics through the shared core; cc45's own warnings keep going to
stderr as now (a rule may take one over: `mega65-recursion-frame`).

### Rules

| Rule | Default | Needs | What |
|---|---|---|---|
| `mega65-dma-trigger-order` | warning | AST | a store to `$D700`/`$D705` before the list address bytes (`$D701`/`$D702`/`$D704`) in the same block |
| `mega65-io-not-volatile` | warning | AST | an access to `$D000`-`$DFFF` (a constant address cast to a pointer, or a `mega65.h` register) through a non-`volatile` pointer |
| `mega65-io-personality` | info (standalone) | AST | VIC-III/IV registers written with no knock (crt0 does it for linked programs) |
| `mega65-isr-rom-call` | warning | IR call graph | an `__interrupt` handler, or a function only it reaches, calling the ROM / KERNAL (`<cbm.h>` wrappers, `__jsr` to `$E000`+) |
| `mega65-stack-frame-limit` | warning | IR | a stack frame larger than `mega65-stack-frame-limit` (default 256) |
| `mega65-recursion-frame` | info | IR | a function on a call cycle (it gets a stack frame instead of static allocation) |
| `mega65-sac-interrupt` | warning | IR call graph | a static-allocation function reachable from an `__interrupt` handler in the same file (ln45 rejects it at link time) |
| `mega65-narrow-param` | info | IR | an `int` parameter that every call passes 0-255 (cc45's note today) |
| `mega65-wide-arith` | info | IR known bits | `long` arithmetic whose operands and result fit 16 bits |
| `mega65-signedness` | warning | AST types | a plain (unsigned) `int`/`char` compared with a negative constant or assigned one (`char c = -1` is 255), a division or `>>` whose result differs from standard C's |
| `mega65-implicit-narrowing` | info | AST types | a wider value assigned to a narrower integer without a cast |

### Not planned
- Calling-convention mismatches: ln45 checks every cross-object `JSR` and
  bridges either direction (`--no-thunks`: an error).
- Register pressure, strength reduction, unrolling, peephole suggestions:
  the optimizer's work; as advice mostly noise.
- ZP save/restore advice: cc45 places base-page variables itself.
- C-to-assembly mapping: `objdump45 --source` / `--html`, `-S` with `.loc`.

### Phases
1. The `cl45` symlink and lint mode on the shared core; the AST rules
   (`dma-trigger-order`, `io-not-volatile`, `signedness`,
   `implicit-narrowing`); `doc/bin/cl45.md`; `src/test/test_cl45.sh` with
   fixtures in `src/test-resources/cl45/`.
2. The IR rules (`stack-frame-limit`, `recursion-frame`, `sac-interrupt`,
   `isr-rom-call`, `narrow-param`, `wide-arith`).
3. `make lint-lib` over the library's C and assembly; the LSP server
   (`tools/ide/lsp-server`) running both linters.

## Acceptance
- The library lints clean at default severities; each suppression in it
  has a reason.
- Every rule has a positive and a negative fixture; al45's include a
  hand-written-style fixture without `proc` or attributes.
- Output does not change with `-v` levels (`test_logging.sh`).

## Out of scope
Cross-file analysis (ln45's), value tracking beyond "same as" for redundant
loads, a formatter (`af45`).
