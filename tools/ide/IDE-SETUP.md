# MEGA65 LSP Server — IDE Setup Guide

This guide covers setup of the MEGA65 C Compiler (cc45) and Assembler (ca45) Language Server Protocol (LSP) server in various IDEs.

## Prerequisites

1. **Built LSP Server & Extension** (done in repo root):
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
   - **C source**: `.c`, `.h`
   - **Assembly**: `.s45`, `.asm` (45GS02), `.s`, `.asm` (6502)

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
   npm install -g @vscode/vsce
   cd tools/ide/vscode-extension
   vsce package
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
     "m65.diagnostics.debounceMs": 500,
     "m65.indexer.exclude": ["node_modules", ".git", "bin", "obj", "build"]
   }
   ```

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

## Visual Studio 2022+

### Setup via LSP Client Extension

1. **Install LSP Client**:
   - Extensions → Manage Extensions
   - Search: "LSP Client"
   - Install "LSP Client" (by jasonwilson)

2. **Configure Extension**:
   - Tools → Options → LSP Client → LSP Client Settings
   - Add server configuration:
     ```json
     {
       "root": "${workspaceFolder}",
       "executable": "node",
       "arguments": [
         "C:\\path\\to\\m65compiler\\tools\\ide\\lsp-server\\dist\\server.js"
       ],
       "languages": [
         { "languageId": "c", "scheme": "file" },
         { "languageId": "plaintext", "scheme": "file" }
       ]
     }
     ```

3. **Or copy the server elsewhere**: `tools\ide\lsp-server\dist` holds the server and its data (one
   `server.js`, `data\`); copy the directory and point the arguments at its `server.js`:
   ```bash
   xcopy /E /I tools\ide\lsp-server\dist  %USERPROFILE%\mega65-lsp\
   ```

4. **File Associations** (Tools → Options → Text Editor → File Extensions):
   - `.s45` → C/C++
   - `.asm` → C/C++ (assembly)

### Features

- ✅ Real-time diagnostics (squiggles)
- ✅ Hover documentation
- ✅ Goto Definition (F12)
- ✅ Autocomplete (Ctrl+Space)
- ⚠️ Symbol search (limited UI)

### Known Limitations

- No dedicated syntax highlighting for 45GS02 assembly
- Limited symbol picker UI
- No multi-target debugging (use VS Code for that)

---

## CLion / JetBrains IDEs (IntelliJ, PyCharm, Rider, etc.)

### Setup via LSP Support Plugin

1. **Enable LSP Support** (Built-in as of 2023.1):
   - Settings → Languages & Frameworks → Language Server Protocol
   - Check "Enable Language Server Protocol"

2. **Configure LSP Server**:
   - Settings → Languages & Frameworks → LSP → Server Definitions
   - Click **+** to add new server:
     ```
     Name: MEGA65 LSP
     Extension: C, H, S45, ASM
     Path: node
     Arguments: ${LSP_SERVER_PATH}/dist/server.js
     Initialization Options: {}
     Trace Level: Verbose (for debugging)
     ```
   - Set `${LSP_SERVER_PATH}` = `${PROJECT_DIR}/tools/ide/lsp-server`

3. **Project Configuration**:
   - File → Project Structure → Project Settings
   - Add Run Configuration → Node.js
     ```
     Node executable: node
     JavaScript file: tools/ide/lsp-server/dist/server.js
     Working directory: ${PROJECT_DIR}
     ```

4. **File Type Associations**:
   - Settings → Editor → File Types
   - Assign `.s45`, `.asm` to "C++" type

### Features

- ✅ Real-time diagnostics (red/yellow squiggles)
- ✅ Hover documentation with "Ctrl+Q"
- ✅ Goto Definition (**Ctrl+B** or **Cmd+B**)
- ✅ Find Usages (**Alt+F7** / **Opt+F7**)
- ✅ File structure / outline (**Ctrl+F12**)
- ✅ Symbol Search (**Ctrl+Alt+O** / **Cmd+Opt+O**)
- ✅ Code Completion (**Ctrl+Space**)

### Keyboard Shortcuts

| Action | Shortcut |
|--------|----------|
| Go to Definition | **Ctrl+B** (Windows/Linux), **Cmd+B** (Mac) |
| Quick Documentation | **Ctrl+Q** (Windows/Linux), **F1** (Mac) |
| Find Symbol | **Ctrl+Alt+O** (Windows/Linux), **Cmd+Opt+O** (Mac) |
| Find Usages | **Alt+F7** |
| Go to Type | **Ctrl+Shift+B** (Windows/Linux), **Cmd+Shift+B** (Mac) |

### Debugging Integration

For C code debugging, configure native debugger:
- Settings → Build, Execution, Deployment → Debugger
- Use GDB/LLDB configured for your platform
- Set breakpoints in LSP-highlighted code
- 45GS02 debugging in xemu: VS Code's `cc45` debug type (dbg45 `--dap`, [doc/bin/dbg45.md](../../doc/bin/dbg45.md)); JetBrains IDEs can run the same adapter with a DAP plugin

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
         "filetypes": ["c", "h", "s45", "asm"],
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
    \ 'whitelist': ['c', 'h', 's45', 'asm'],
    \ })
augroup END
```

---

## Emacs

### Setup via lsp-mode

1. **Install lsp-mode**:
   ```lisp
   ;; Using use-package (in ~/.emacs.d/init.el)
   (use-package lsp-mode
     :ensure t
     :hook ((c-mode . lsp)
            (asm-mode . lsp))
     :commands lsp)
   ```

2. **Configure LSP Server**:
   ```lisp
   (use-package lsp-clients
     :ensure nil
     :after lsp-mode
     :config
     (lsp-register-client
       (make-lsp-client
         :new-connection (lsp-stdio-connection '("node" "~/projects/mega65/tools/ide/lsp-server/dist/server.js"))
         :major-modes '(c-mode asm-mode)
         :server-id 'mega65-lsp)))
   ```

3. **Key Bindings** (in lsp-mode):
   ```lisp
   ;; These are defaults, customize as needed:
   (define-key lsp-mode-map (kbd "M-.") 'lsp-find-definition)
   (define-key lsp-mode-map (kbd "M-,") 'pop-tag-mark)
   (define-key lsp-mode-map (kbd "C-M-<tab>") 'completion-at-point)
   (define-key lsp-mode-map (kbd "K") 'lsp-describe-thing-at-point)
   ```

### Features

- ✅ Real-time diagnostics (flymake / flycheck)
- ✅ Hover documentation (**M-x lsp-describe-thing-at-point**)
- ✅ Goto Definition (**M-.**)
- ✅ Find References (**M-x lsp-find-references**)
- ✅ Outline (**M-x imenu** / lsp-treemacs-symbols)
- ✅ Autocomplete (company-lsp)

### Installation Notes

- Install company-mode for better completion UI:
  ```lisp
  (use-package company
    :ensure t
    :hook (lsp-mode . company-mode))
  ```

---

## Sublime Text 4+

### Setup via LSP Package

1. **Install LSP Package**:
   - Command Palette (**Ctrl+Shift+P**)
   - "Package Control: Install Package"
   - Search "LSP" → Install "LSP"

2. **Configure LSP** (Preferences → Package Settings → LSP → Settings):
   ```json
   {
     "clients": {
       "mega65_lsp": {
         "enabled": true,
         "command": ["node", "${project_dir}/tools/ide/lsp-server/dist/server.js"],
         "languages": [
           {
             "languageId": "c",
             "scopes": ["source.c"],
             "syntaxes": ["Packages/C++/C.sublime-syntax"]
           },
           {
             "languageId": "m65-asm",
             "scopes": ["source.asm"],
             "syntaxes": ["Packages/Assembly/x86-64.sublime-syntax"]
           }
         ]
       }
     }
   }
   ```

3. **File Associations** (Preferences → Settings):
   ```json
   {
     "file_type_mappings": {
       "c": "C",
       "h": "C Header",
       "s45": "x86-64 Assembly",
       "asm": "x86-64 Assembly"
     }
   }
   ```

4. **Key Bindings** (Preferences → Key Bindings):
   ```json
   [
     { "keys": ["f12"], "command": "lsp_symbol_definition" },
     { "keys": ["ctrl+k", "ctrl+i"], "command": "lsp_hover" },
     { "keys": ["ctrl+shift+r"], "command": "lsp_rename" },
     { "keys": ["ctrl+t"], "command": "lsp_symbol_search" }
   ]
   ```

### Features

- ✅ Real-time diagnostics (gutter icons)
- ✅ Hover documentation (**Ctrl+K Ctrl+I**)
- ✅ Goto Definition (**F12**)
- ✅ Symbol Search (**Ctrl+T**)
- ✅ Find References (**Shift+F12**), outline (**Ctrl+R**)
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
2. Run manually to test: `./bin/cc45 test.c -o test.s45`
3. Check IDE console for subprocess errors

**Hover/completion not working:**
1. Verify data files exist: `ls tools/ide/lsp-server/data/*.json`
2. Check IDE language ID matches: `c`, `h`, `m65-asm`, `asm`
3. Ensure static data files were built: `npm run build`

---

## IDE Feature Comparison

| Feature | VS Code | Visual Studio | CLion | Vim | Emacs | Sublime |
|---------|---------|---------------|-------|-----|-------|---------|
| Diagnostics | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Hover Docs | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Go to Definition | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Autocomplete | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Symbol Search | ✅ | ⚠️ | ✅ | ✅ | ✅ | ✅ |
| Find References | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Outline (document symbols) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Member completion (`.`, `->`) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| C ↔ assembly (Show Assembly) | ✅ | ❌ | ❌ | ⚠️ (`m65/assembly`) | ⚠️ (`m65/assembly`) | ❌ |
| Problem matchers (build tasks) | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ |
| Rename | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| Syntax Highlighting (Assembly) | ✅ | ⚠️ | ⚠️ | ⚠️ | ⚠️ | ⚠️ |
| Integrated Debugging | ✅ | ✅ | ✅ | ❌ | ❌ | ❌ |
| Setup Difficulty | Easy | Medium | Medium | Hard | Hard | Medium |

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
- [Compiler Configuration](../../../doc/bin/cc45.md)
- [Assembler Guide](../../../doc/bin/ca45.md)
- [Standard Library](../../../doc/architecture/stdlib.md)
