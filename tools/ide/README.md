# MEGA65 IDE Tools

Language Server Protocol (LSP) server and VS Code extension for the MEGA65 C compiler (cc45) and assembler (ca45).

**📖 [IDE Setup Guide](IDE-SETUP.md)** — Configure LSP in VS Code, Visual Studio, CLion, Vim, Emacs, Sublime, and more.

## Project Structure

- **`lsp-server/`** — Standalone LSP server (Node.js, TypeScript)
  - Language-agnostic, can be used by other editors
  - Communicates with cc45/ca45 via subprocess
  - Provides diagnostics, hover, completion, go-to-definition

- **`vscode-extension/`** — VS Code client extension
  - Bundles the LSP server
  - Registers language support for `.c`, `.h`, `.s45`, `.asm` files
  - Contributes syntax highlighting, configuration, command palette

## Building

```bash
npm install              # Install dependencies
npm run build           # Build both packages
npm run watch           # Continuous build
npm run lint            # Run linter
```

## Development

### Running in VS Code

1. Open the vscode-extension directory in VS Code
2. Press `F5` to launch the Extension Development Host
3. Open a `.c`, `.h`, `.s45`, or `.asm` file to activate the extension

### Debugging

Set breakpoints in the extension or LSP server code. The debugger will pause and allow inspection.

## Known Limitations (Phase 1)

- No header-only diagnostics (`.h` files alone)
- No multi-error collection (stops at first error)
- Symbol navigation is ctags-style (no semantic analysis)
- No debugger/DAP integration (issue #200)
- `#include` in assembly is broken in ca45 (known compiler bug)

## Notes

- LSP server expects cc45 and ca45 binaries at `${workspaceFolder}/bin/`
- Configurable via VS Code settings: `m65.cc45Path`, `m65.ca45Path`
- Diagnostics debounced at 500ms (configurable: `m65.diagnostics.debounceMs`)
