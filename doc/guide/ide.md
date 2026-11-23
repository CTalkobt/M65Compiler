# Editors and IDEs

`tools/ide` has a language server for any editor that speaks the Language Server Protocol, and a VS Code
extension that bundles it with syntax highlighting, the `m65.*` settings and a debugger. Their own pages:
[tools/ide/README.md](../../tools/ide/README.md) (what each part does, building) and
[tools/ide/IDE-SETUP.md](../../tools/ide/IDE-SETUP.md) (setting up VS Code, Visual Studio, CLion, Vim, Emacs and
Sublime).

## What they do

The language server (`tools/ide/lsp-server`, Node.js 18 or newer; one file, `dist/server.js`, with its `data/`):

- **Diagnostics** as you type, saved or not: `cc45 -c` on C files, `ca45 -c` on assembly (`.s45`, `.asm`), on a
  copy of the text, after `m65.diagnostics.debounceMs` (500 ms). cc45 stops at its first error, so one error at
  a time; a header is not compiled alone: an error in it is shown on the first line of a C file that includes
  it, with the header's name and line.
- **Lint findings** beside them (`m65.lint.enabled`, on by default): cl45 on C, al45 on assembly, with the
  rule as the diagnostic's code (`mega65-io-not-volatile`) and the project's `.cc45-lint.yml` applied
  ([cl45](../bin/cl45.md), [al45](../bin/al45.md)).
- **Hover and completion**: C keywords and cc45's extensions, the library's functions (from `lib/include/*.h`),
  `#pragma`s; ca45's instructions (modes, bytes, cycles at 40 MHz), simulated instructions and directives; the
  workspace's symbols and the library headers', with the comment above each declaration; members after `.` and
  `->` (including `<mega65.h>`'s register structs, `vic4->`).
- **Navigation**: go to definition, find references (by name), the file's outline, workspace symbols; the
  workspace's C and assembly files and cc45's headers are indexed (`m65.indexer.exclude` leaves directories out).
- **C and assembly side by side**: the request `m65/assembly` compiles the document with `cc45 -S` and gives
  the line of the cursor's code; `m65/source` (and go to definition on a `.loc` line) goes back to the C.

No rename; references are by name, not by scope.

The VS Code extension (`tools/ide/vscode-extension`) adds:

- the languages `c`, `h` and `m65-asm` (`.s45`, `.asm`), with a grammar for 45GS02 assembly;
- the commands **M65: Show Assembly** (a C file's `cc45 -S` beside it, at the cursor's line) and
  **M65: Show C Source** (in cc45's assembly);
- problem matchers for build tasks: `$cc45`, `$ca45`, `$ca45-line` and `$m65` (any of the tools);
- the debug type `cc45`: `dbg45 --dap` runs the program in xemu (below).

## Installing

Build both packages and the `.vsix` (needs Node.js and npm; `npm ci` fetches the dependencies):

```bash
cd tools/ide
npm ci
npm run build                        # type check, then the bundles: lsp-server/dist, vscode-extension/dist
cd vscode-extension
npx vsce package --no-dependencies   # mega65-cc45-ca45-0.1.0.vsix
code --install-extension mega65-cc45-ca45-0.1.0.vsix
```

To try the extension without installing it: open `tools/ide/vscode-extension` in VS Code and press F5 (an
Extension Development Host). It needs an open workspace folder.

Other editors run the server themselves: `node tools/ide/lsp-server/dist/server.js` talks LSP on stdio
(`--node-ipc` and `--socket=N` also work). `lsp-server/dist` can be copied anywhere. The settings below are
sent as the `initializationOptions` or answered for the `workspace/configuration` section `m65`; examples
for each editor are in [IDE-SETUP.md](../../tools/ide/IDE-SETUP.md).

## Settings

| Setting | Default | Meaning |
|---------|---------|---------|
| `m65.cc45Path` | `${workspaceFolder}/bin/cc45` | cc45. `${workspaceFolder}` and `~` are expanded; when the default names a `bin/cc45` that is not there, cc45 on the `PATH` is used |
| `m65.ca45Path` | `${workspaceFolder}/bin/ca45` | ca45, found the same way |
| `m65.lint.enabled` | `true` | Run cl45 and al45 with the compiler |
| `m65.cl45Path` | (empty: beside cc45) | cl45 (a link to cc45) |
| `m65.al45Path` | (empty: beside ca45) | al45 (a link to ca45) |
| `m65.diagnostics.debounceMs` | `500` | Delay after a change before the tools run |
| `m65.indexer.exclude` | `["node_modules", ".git", "bin", "obj", "build"]` | Left out of the workspace index |
| `m65.dbg45Path` | `${workspaceFolder}/bin/dbg45` | dbg45, for the `cc45` debug type (VS Code only; used as given, no `PATH` fallback: set it outside the compiler's tree) |

The tools' own options come from their configuration files (`~/.config/m65/cc45.conf`, `ca45.conf`,
`.ca45rc`: [CONFIGURATION.md](../bin/CONFIGURATION.md)). In a workspace that is not the compiler's tree, with
the tools installed, the defaults find cc45, ca45 and the linters on the `PATH`; with a release archive unpacked elsewhere, set the
paths (`"m65.cc45Path": "~/m65compiler/bin/cc45"`).

## Debugging in VS Code

The debug type `cc45` runs [dbg45](../bin/dbg45.md) with `--dap`, which starts xemu with the program and drives
it: breakpoints in C (and assembly) lines, stepping, the call stack, variables by type. It needs:

- the program built with debug information, so `prog.dbg` is beside `prog.prg`: `cc45 -O2 -g main.c -o prog.prg`
  (or `ln45 -g`);
- an xemu whose monitor has the breakpoint list (`k`): CTalkobt/xemu, branch `cc45-dev` (the Docker images build
  it), and the MEGA65 ROM ([troubleshooting](troubleshooting.md#xemu-and-the-mega65-rom)).

`.vscode/launch.json`:

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "type": "cc45",
      "request": "launch",
      "name": "Debug in xemu",
      "program": "${workspaceFolder}/prog.prg",
      "stopOnEntry": true
    }
  ]
}
```

| Attribute | Meaning |
|-----------|---------|
| `program` | The `.prg` (required); its `.dbg` next to it |
| `xemu` | The xemu to run (default `xemu-xmega65`) |
| `xemuArgs` | More of xemu's options, e.g. `["-8", "disk.d81"]` |
| `headless` | xemu without a window (default `false`) |
| `fast` | xemu as fast as it can, `-sleepless` (default `false`) |
| `stopOnEntry` | Stop at `main` (default `true`) |

## Building with a task

`.vscode/tasks.json`, the tools' messages in the Problems pane:

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "build",
      "type": "shell",
      "command": "cc45 -O2 -g main.c -o prog.prg",
      "group": "build",
      "problemMatcher": ["$m65"]
    }
  ]
}
```

## Developing the tools

```bash
cd tools/ide
npm test                         # the server's tests (an end-to-end one needs ../../bin/cc45: make)
npm run lint
npm run gen-data -w lsp-server   # lsp-server/data/*.json again from the suite's sources (needs make, make lib)
```

`gen-data` rebuilds the hover and completion data from ca45's opcode table, the cycle table, `doc/bin/ca45.md`,
`doc/bin/cc45.md` and the library headers: run it after changing any of them.
