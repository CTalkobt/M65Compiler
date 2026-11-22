# MEGA65 LSP Server — IDE Setup Guide

This guide covers setup of the MEGA65 C Compiler (cc45) and Assembler (ca45) Language Server Protocol (LSP) server in various IDEs.

## Prerequisites

1. **Built LSP Server & Extension** (from the repository root):
   ```bash
   cd tools/ide
   npm install
   npm run build
   ```

2. **Compiler binaries** at `${workspaceRoot}/bin/`:
   ```bash
   ./bin/cc45 --version
   ./bin/ca45 --version
   ```
   (Or configure paths in IDE settings)

3. **File types** recognized:
   - **C source**: `.c`, `.h` (by extension, or a document whose language ID is `c` or `h`)
   - **Assembly** (ca45, 45GS02): `.s45`, `.asm` (by extension, or the language ID `m65-asm`)
   - `.s` and `.inc` files get hover, completion and the outline as assembly, but no diagnostics
     (they are often another assembler's source, or fragments that do not assemble alone)

---

## Visual Studio Code (Recommended)

### Quick Start (Development Mode)

1. Open `tools/ide/vscode-extension/` in VS Code
2. Press **F5** → Extension Development Host launches
3. Create/open a `.c` or `.s45` file
4. Watch for diagnostics, hover for docs, press **F12** for go-to-definition

### Installation (Production)

1. **Package the extension**:
   ```bash
   cd tools/ide/vscode-extension
   npm run package          # npx @vscode/vsce package (after npm run build)
   # Creates: mega65-cc45-ca45-0.1.0.vsix
   ```

2. **Install in VS Code**:
   - Via UI: `Extensions > ... > Install from VSIX`
   - Via CLI: `code --install-extension ./mega65-cc45-ca45-0.1.0.vsix`

3. **Configuration** (File → Preferences → Settings, search `m65`):
   ```json
   {
     "m65.cc45Path": "${workspaceFolder}/bin/cc45",
     "m65.ca45Path": "${workspaceFolder}/bin/ca45",
     "m65.lint.enabled": true,
     "m65.cl45Path": "",
     "m65.al45Path": "",
     "m65.diagnostics.debounceMs": 500,
     "m65.indexer.exclude": ["node_modules", ".git", "bin", "obj", "build"],
     "m65.dbg45Path": "${workspaceFolder}/bin/dbg45"
   }
   ```
   (`cl45Path` / `al45Path` empty: the linters beside cc45 and ca45. `dbg45Path` is for the `cc45` debug
   type, [doc/bin/dbg45.md](../../doc/bin/dbg45.md).)

### Keyboard Shortcuts

| Action | Shortcut |
|--------|----------|
| Go to Definition | **F12** |
| Peek Definition | **Ctrl+Shift+F10** |
| Find All References | **Shift+F12** |
| Go to Symbol in File (outline) | **Ctrl+Shift+O** |
| Go to Symbol in Workspace | **Ctrl+T** |
| Hover for Documentation | Hover mouse |
| Trigger Autocomplete | **Ctrl+Space** |
| M65: Show Assembly (a C file's cc45 -S, beside it, at the cursor's line) | Command Palette / editor menu |
| M65: Show C Source (in cc45's assembly: the line its `.loc` names) | Command Palette / editor menu |

### Build Tasks

The extension contributes problem matchers, so a task's cc45, ca45 and ln45 errors land in the Problems pane:
`$cc45` (`file:line:col: error: …`), `$ca45`, `$ca45-line` (`file: line N: …`) and `$m65` (any of the tools).

```json
{
  "label": "build",
  "type": "shell",
  "command": "make",
  "problemMatcher": ["$m65"]
}
```

---

## Other editors

The setups below follow each editor's current documentation for adding a language server; they have not
been tried in every editor here. All of them start the same server, `node tools/ide/lsp-server/dist/server.js`
(stdio; `--stdio` is assumed when no transport is given), and the paths in them are placeholders for your
checkout. A C/C++ server the editor already runs (clangd, the IDE's own) works beside this one, or takes
over C files, depending on the editor.

## Visual Studio 2022+

Visual Studio has no general LSP client to point at a server: a language server is added by a VSIX
extension of its own (an `ILanguageClient` from `Microsoft.VisualStudio.LanguageServer.Client` that starts
`node <path>\tools\ide\lsp-server\dist\server.js --stdio` for `.c`, `.h`, `.s45`, `.asm`). None is
provided in this repository; use VS Code, or one of the editors below.

---

## CLion / JetBrains IDEs (IntelliJ, PyCharm, Rider, etc.)

### Setup via the LSP4IJ plugin

JetBrains IDEs have no settings page for adding a language server by hand; the LSP4IJ plugin (by Red Hat,
from the Marketplace) provides one.

1. **Install LSP4IJ**: Settings → Plugins → Marketplace → "LSP4IJ" → Install, restart.

2. **Add the server**: Settings → Languages & Frameworks → Language Servers → **+**
   - *Server* tab: Name `MEGA65 LSP`; Command
     ```
     node /path/to/m65compiler/tools/ide/lsp-server/dist/server.js --stdio
     ```
   - *Mappings* tab → *File name patterns*: `*.c;*.h` with language ID `c`, `*.s45;*.asm` with language
     ID `m65-asm`
   - *Configuration* tab: the `m65` settings below (General LSP Configuration), as JSON

3. **Assembly files**: without a file type for `.s45`, map it to *Text* (Settings → Editor → File Types)
   so the IDE opens it; the server's features do not depend on it.

### Features

LSP4IJ maps the server to the IDE's own actions: diagnostics, Quick Documentation (**Ctrl+Q**, Mac **F1**),
Go to Declaration (**Ctrl+B** / **Cmd+B**), Find Usages (**Alt+F7**), File Structure (**Ctrl+F12**),
Go to Symbol (**Ctrl+Alt+Shift+N** / **Cmd+Opt+O**) and completion (**Ctrl+Space**). LSP4IJ's log of the
server's messages is in the *Language Servers* tool window.

### Debugging Integration

45GS02 debugging in xemu is VS Code's `cc45` debug type (dbg45 `--dap`, [doc/bin/dbg45.md](../../doc/bin/dbg45.md)).
LSP4IJ also has a DAP client that could run `dbg45 --dap`; that has not been tried.

---

## Vim / Neovim

### Setup via coc.nvim (Conqueror of Completion)

1. **Install coc.nvim**:
   ```bash
   # Using vim-plug
   Plug 'neoclide/coc.nvim', {'branch': 'release'}
   :PlugInstall
   ```

2. **Configure LSP Server** (`~/.config/nvim/coc-settings.json` or `:CocConfig`):
   ```json
   {
     "languageserver": {
       "mega65-lsp": {
         "command": "node",
         "args": ["${workspaceFolder}/tools/ide/lsp-server/dist/server.js"],
         "filetypes": ["c", "cpp", "asm"],
         "rootPatterns": ["Makefile", ".git", "package.json"],
         "initializationOptions": {}
       }
     }
   }
   ```

3. **Key Mappings** (add to `~/.config/nvim/init.vim`):
   ```vim
   " Go to definition
   nmap <buffer> gd <Plug>(coc-definition)
   
   " Go to references
   nmap <buffer> gr <Plug>(coc-references)
   
   " Outline of the file
   nmap <buffer> <leader>o :CocList outline<CR>
   
   " Show hover documentation
   nnoremap <buffer> K :call CocActionAsync('doHover')<CR>
   
   " Trigger autocomplete
   inoremap <buffer> <silent><expr> <C-space> coc#refresh()
   ```

   (`cpp`: Vim opens `.h` files as C++ unless `g:c_syntax_for_h` is set; `.s45` is `asm` by the line below.)

4. **Syntax Highlighting** (optional, for 45GS02 assembly):
   ```bash
   # Install assembly syntax plugin
   Plug 'vim-scripts/asm-syntax'  # or similar
   
   # Then add to init.vim:
   au BufRead,BufNewFile *.s45 set filetype=asm
   ```

### Features

- ✅ Real-time diagnostics (quickfix list)
- ✅ Hover documentation (**K**)
- ✅ Goto Definition (**gd**)
- ✅ Find References (**gr**)
- ✅ Outline (**:CocList outline**)
- ✅ Autocomplete (**Ctrl+Space**)

### Alternative: vim-lsp

```vim
" Using vim-lsp plugin
Plug 'prabirshrestha/vim-lsp'

augroup lsp_install
  au!
  autocmd User lsp_setup call lsp#register_server({
    \ 'name': 'mega65-lsp',
    \ 'cmd': {srv -> ['node', $HOME . '/.local/mega65-lsp/dist/server.js']},
    \ 'allowlist': ['c', 'cpp', 'asm'],
    \ })
augroup END
```

### Alternative: Neovim's own client (0.11+)

```lua
vim.filetype.add({ extension = { s45 = 'asm' } })
vim.lsp.config('mega65', {
  cmd = { 'node', vim.fn.expand('~/m65compiler/tools/ide/lsp-server/dist/server.js'), '--stdio' },
  filetypes = { 'c', 'cpp', 'asm' },
  root_markers = { 'Makefile', '.git' },
  settings = { m65 = { cc45Path = vim.fn.expand('~/m65compiler/bin/cc45') } },
})
vim.lsp.enable('mega65')
```

---

## Emacs

### Setup via eglot (built in since Emacs 29)

```lisp
(add-to-list 'auto-mode-alist '("\\.s45\\'" . asm-mode))
(with-eval-after-load 'eglot
  (add-to-list 'eglot-server-programs
               `((c-mode c-ts-mode asm-mode)
                 . ("node" ,(expand-file-name "~/m65compiler/tools/ide/lsp-server/dist/server.js") "--stdio"))))
```

`M-x eglot` in a `.c` or `.s45` buffer starts it (or `(add-hook 'c-mode-hook #'eglot-ensure)`). Settings go in
`eglot-workspace-configuration`: `'(:m65 (:cc45Path "/path/to/bin/cc45"))`.

### Setup via lsp-mode

```lisp
(add-to-list 'auto-mode-alist '("\\.s45\\'" . asm-mode))
(with-eval-after-load 'lsp-mode
  (add-to-list 'lsp-language-id-configuration '(asm-mode . "m65-asm"))
  (lsp-register-client
   (make-lsp-client
    :new-connection (lsp-stdio-connection
                     (list "node" (expand-file-name "~/m65compiler/tools/ide/lsp-server/dist/server.js") "--stdio"))
    :activation-fn (lsp-activate-on "c" "m65-asm")
    :priority 1                       ; before clangd in C buffers
    :server-id 'mega65-lsp)))
(add-hook 'c-mode-hook #'lsp-deferred)
(add-hook 'asm-mode-hook #'lsp-deferred)
```

(`expand-file-name`: the command is not run by a shell, so a bare `~` is not expanded.)

### Features

- ✅ Real-time diagnostics (flymake; flycheck with lsp-mode)
- ✅ Hover documentation (eldoc; `M-x eldoc-doc-buffer`, lsp-mode `M-x lsp-describe-thing-at-point`)
- ✅ Goto Definition (**M-.**, back with **M-,**)
- ✅ Find References (**M-?**)
- ✅ Outline (**M-x imenu**)
- ✅ Autocomplete (`completion-at-point`, **C-M-i**; company or corfu show it as you type)

---

## Sublime Text 4+

### Setup via the LSP package

1. **Install LSP**: Command Palette (**Ctrl+Shift+P**) → "Package Control: Install Package" → "LSP".

2. **Configure** (Preferences → Package Settings → LSP → Settings):
   ```json
   {
     "clients": {
       "mega65-lsp": {
         "enabled": true,
         "command": ["node", "/path/to/m65compiler/tools/ide/lsp-server/dist/server.js", "--stdio"],
         "selector": "source.c | source.asm",
         "settings": {
           "m65": { "cc45Path": "/path/to/m65compiler/bin/cc45", "ca45Path": "/path/to/m65compiler/bin/ca45" }
         }
       }
     }
   }
   ```

3. **Assembly files**: Sublime has no assembly syntax of its own. Install one from Package Control whose
   scope is `source.asm…` (a 6502 or other assembly package), open a `.s45` file and choose View → Syntax →
   *Open all with current extension as…* that syntax. `.c` and `.h` are `source.c` already.

4. **Key Bindings**: the LSP package's defaults are **F12** (definition), **Shift+F12** (references),
   **F2** (rename, which this server does not provide), hover by the mouse; Preferences → Package Settings →
   LSP → Key Bindings lists them.

### Features

- ✅ Real-time diagnostics (gutter icons, the diagnostics panel)
- ✅ Hover documentation
- ✅ Goto Definition (**F12**)
- ✅ Symbol Search (Command Palette: "LSP: Goto Symbol in Project")
- ✅ Find References (**Shift+F12**), outline ("LSP: Goto Symbol")
- ✅ Autocomplete

---

## General LSP Configuration

### Common Settings (all IDEs)

After connecting, configure through the IDE's settings for the server (VS Code: the `m65.*` settings; other
editors: the server's `initializationOptions` or `workspace/configuration` section `m65`):

```json
{
  "m65": {
    "cc45Path": "${workspaceFolder}/bin/cc45",
    "ca45Path": "${workspaceFolder}/bin/ca45",
    "lint": {
      "enabled": true
    },
    "diagnostics": {
      "debounceMs": 500
    },
    "indexer": {
      "exclude": ["node_modules", ".git", "bin", "obj", "build"]
    }
  }
}
```

### Compiler Configuration

For diagnostics the server runs, on a copy of the text being edited:
- **C files**: `cc45 -c <copy> -o <temp>.o45 -I <the file's directory>`
- **Assembly files**: `ca45 -c <copy> -o <temp>.o45` (the copy beside the file, so `.include` finds its neighbours)

With `lint.enabled` (the default) the linters run on the same text, beside the compiler:
- **C files**: `cl45 --format=json <file>`
- **Assembly files**: `al45 --format=json <file>`

Their findings are diagnostics with the rule as the code (`mega65-io-not-volatile`) and the source
`cl45` / `al45`; a warning the compiler gives that a rule takes over is shown once, as the rule, and a
file with errors shows only the errors. Unsaved text is linted from a hidden copy beside the file, so
the rules, levels and suppressions are those of the project's `.cc45-lint.yml`
([doc/bin/cl45.md](../../doc/bin/cl45.md), [doc/bin/al45.md](../../doc/bin/al45.md)). The linters are
found beside cc45 and ca45 (they are links to them); `cl45Path` / `al45Path` name others. Without them
the server works as before.

Options come from the tools' own configuration: `~/.config/m65/cc45.conf`, `ca45.conf` / `.ca45rc`
([doc/bin/CONFIGURATION.md](../../doc/bin/CONFIGURATION.md)).

The library's headers (`lib/include`, found beside cc45: `bin/../lib/include`, or an installed
`bin/../include/cc45`; `CC45_INCLUDE` first) are indexed with the workspace: go to definition reaches
`<stdio.h>` and `<mega65.h>`, hover shows a macro such as `VREG_BORDER` with the comment above it, and
`vic4->` completes the members of `struct vic4_regs`.

### Requests for other editors

Besides the LSP's own, the server answers two requests (VS Code's "M65: Show Assembly" / "Show C Source"):

- `m65/assembly` `{ uri, line }` (a C document, a 0-based line): the document compiled with `cc45 -S` to
  `<tmp>/m65-lsp-asm/<name>.s45`; the result `{ uri, line }` is that file and the line of the C line's code
  (its `.loc`), or `{ error }` (cc45's first message)
- `m65/source` `{ uri, line }` (cc45's assembly): the C place the `.loc` before that line names, `{ uri, line }`
  or `{ error }`

Go to definition on a `.loc` line goes to the C line too.

### Troubleshooting

**Server not starting:**
1. Check node.js is installed: `node --version`
2. Verify LSP server path: `ls -la tools/ide/lsp-server/dist/server.js`
3. Check IDE LSP logs for errors

**Diagnostics not appearing:**
1. Ensure cc45/ca45 binaries exist: `ls -la bin/cc45 bin/ca45`
2. Run what the server runs: `./bin/cc45 -c test.c -o test.o45` (assembly: `./bin/ca45 -c test.s45 -o test.o45`)
3. Check IDE console for subprocess errors

**Hover/completion not working:**
1. Verify data files exist: `ls tools/ide/lsp-server/dist/data/*.json` (copied from `lsp-server/data` by the build)
2. Check the file is one the server takes: `.c`/`.h` (or language ID `c`/`h`), `.s45`/`.asm` (or `m65-asm`)
3. Ensure static data files were built: `npm run build`

---

## IDE Feature Comparison

| Feature | VS Code | JetBrains (LSP4IJ) | Vim / Neovim | Emacs | Sublime |
|---------|---------|--------------------|--------------|-------|---------|
| Diagnostics | ✅ | ✅ | ✅ | ✅ | ✅ |
| Hover Docs | ✅ | ✅ | ✅ | ✅ | ✅ |
| Go to Definition | ✅ | ✅ | ✅ | ✅ | ✅ |
| Autocomplete | ✅ | ✅ | ✅ | ✅ | ✅ |
| Symbol Search | ✅ | ✅ | ✅ | ✅ | ✅ |
| Find References | ✅ | ✅ | ✅ | ✅ | ✅ |
| Outline (document symbols) | ✅ | ✅ | ✅ | ✅ | ✅ |
| Member completion (`.`, `->`) | ✅ | ✅ | ✅ | ✅ | ✅ |
| C ↔ assembly (Show Assembly) | ✅ | ❌ | ⚠️ (`m65/assembly`) | ⚠️ (`m65/assembly`) | ❌ |
| Problem matchers (build tasks) | ✅ | ❌ | ❌ | ❌ | ❌ |
| Rename | ❌ | ❌ | ❌ | ❌ | ❌ |
| Syntax Highlighting (Assembly) | ✅ | ❌ | ⚠️ | ⚠️ | ⚠️ (a package) |
| Integrated Debugging (dbg45) | ✅ | ⚠️ (LSP4IJ's DAP, untried) | ❌ | ❌ | ❌ |
| Setup Difficulty | Easy | Medium | Medium | Medium | Medium |

Visual Studio: none of these without an extension of its own (see above).

---

## Next Steps

- **Test in your IDE**: Open a sample project from `examples/c/` or `examples/asm/`
- **Report issues**: https://github.com/CTalkobt/M65Compiler/issues
- **Future work**:
  - Rename and scope-aware references (symbols are found by patterns, not by the compiler)
  - Multi-error collection in the compiler (cc45 stops at its first error)

---

## Related Documentation

- [LSP Server Architecture](README.md)
- [Compiler Configuration](../../doc/bin/cc45.md)
- [Assembler Guide](../../doc/bin/ca45.md)
- [Standard Library](../../doc/architecture/stdlib.md)
