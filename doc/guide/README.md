# Getting Started

Tutorials for someone new to the suite. Each takes you through the tools step by step; the reference pages in
[../bin/](../bin/) have every option.

| Page | For |
|------|-----|
| [getting-started-c.md](getting-started-c.md) | Writing C for the MEGA65 with cc45: a first program, running it in xemu, on a disk and on a MEGA65, a program in several files, what differs from other C compilers, the hardware, debugging with dbg45 |
| [getting-started-basic45.md](getting-started-basic45.md) | BASIC 65 programs with basic45: labels instead of line numbers, structured blocks, disks, converting old programs, compiling to machine code, calling C |
| [getting-started-tools.md](getting-started-tools.md) | The other tools: ca45, ln45 and ar45; nm45 and objdump45; the linters and the formatter; disk45; pack45, font45, tune45; asmCvt45; dbg45 and deploy45 |
| [troubleshooting.md](troubleshooting.md) | Messages and symptoms: ca45's "has no base-page mode", ROM calls that hang, plain `int` being unsigned, ln45's heap and stack warnings, xemu without the MEGA65 ROM |
| [glossary.md](glossary.md) | The terms the docs use (SAC, ZP convention, base page, overlays, `.ov1` / `.ovd`, far and near pointers, attic RAM...), each with its reference page |
| [ide.md](ide.md) | Editors: the language server and the VS Code extension (diagnostics, lint, completion, the `m65.*` settings, debugging with the `cc45` debug type) |

The pages assume the tools are on your `PATH` (`bin/` of a release archive or of a build, or an installed
`PREFIX/bin`) and, for running programs, the xemu emulator (`xemu-xmega65`) with a MEGA65 ROM.

## Kept working

`src/test/test_guides.py` (a step of `make test`) reads each page and does what it says, in a directory of its
own (`build/test/guides/<page>/`, also its `HOME`, so no personal configuration applies; the build's `bin/` first on the
`PATH`): the files are written, the commands run (each must succeed and print the
output shown), the programs run in xemu (their screens checked). What it does with a block is said by an HTML
comment before it, which a reader does not see:

| Comment | Meaning |
|---------|---------|
| `<!-- guide: file NAME -->` | The block is the file NAME |
| `<!-- guide: run -->` | A console block: each `$ command` is run and must succeed; the lines after it must be in its output (runs of spaces count as one; `...` stands for text left out); `$ cd DIR` alone carries over to the commands after it |
| `<!-- guide: run xemu -->` | The same, skipped without xemu (dbg45 runs it) |
| `<!-- guide: fails -->` | The same, but each command must fail |
| `<!-- guide: xemu PRG [disk=IMAGE] screen=TEXT -->` | PRG is run headless in xemu, IMAGE as drive 8; its screen must show TEXT |

Blocks without a comment are only shown: commands for real hardware, xemu's window, an editor. When you change a
page, run `python3 src/test/test_guides.py doc/guide/<page>.md`.
