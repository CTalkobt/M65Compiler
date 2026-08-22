# Proposed Feature Enhancements for basic45

This document outlines recommended features and capabilities that could be added to the `basic45` utility to improve user experience, streamline development, and provide robust integration with modern development workflows.

---

## 1. Compile-Time Validation & Safety (Developer Experience)

Enhancing `basic45` to catch errors at compile-time instead of runtime.

*   **Label Validation in GOTO/GOSUB/THEN:**
    *   *Current Behavior:* In `--labels` mode, referenced labels that do not exist are not validated, causing runtime syntax errors.
    *   *Improvement:* Perform a validation pass after parsing to ensure every referenced label exists. Throw a compiler error with the file and line number if an undefined label is found.
*   **Unresolved Symbol Detection:**
    *   *Current Behavior:* Symbols not found in the `--symbols` file are silently left as-is (e.g. `{unknown_sub}` remains in text), which leads to syntax errors in tokenization or execution.
    *   *Improvement:* Generate a warning or error listing all symbol placeholders that could not be resolved from the symbol file.
*   **Standardized Error Output Format:**
    *   *Improvement:* Format error and warning messages matching the GNU Standard: `filename:line:col: error: message`. This allows modern IDEs (like VS Code, Emacs, or Vim) to parse errors automatically, enabling developers to click on errors to jump directly to the code.
*   **Ascending Line Number Verification (Normal Mode):**
    *   *Improvement:* Verify that user-specified line numbers are strictly ascending and flag duplicates or out-of-order lines before compiling.
*   **Program Size and RAM Limits Warning:**
    *   *Improvement:* Calculate total compiled binary size relative to the load address (e.g. `0x0801`) and warn the user if the compiled program is likely to overflow available MEGA65 BASIC memory.

---

## 2. Preprocessor & Syntax Extensions

Enhancing code flexibility, readability, and binary size optimization.

*   **Symbol Expression Evaluation:**
    *   *Current Behavior:* Symbol substitution only supports exact name matches with hex-to-dec translation.
    *   *Improvement:* Support offset arithmetic and simple expressions (e.g., `{myroutine + 3}` or `{sprite_table + (4 * 64)}`) to reference structure members or offsets directly.
*   **Predefined Macros:**
    *   *Improvement:* Expose compiler-defined macros like `__FILE__`, `__LINE__`, `__DATE__`, and `__TIME__` for logging and build tracking.
*   **Extended Preprocessor Directives:**
    *   *Improvement:* Add support for `#elif` and `#undef` directives to make conditional compilation logic cleaner.
*   **Automatic Code Minification/Compression:**
    *   *Improvement:* Add an optimization flag (e.g., `-O` or `--minify`) that strips comments (`REM`), removes non-essential spaces (in non-quoted contexts), and merges consecutive statements onto a single line (using `:`) where possible up to the 80/255 character line limits. This reduces disk footprint and improves execution speed on the retro platform.

---

## 3. Tooling, Integration, & Workflow

Making the utility fit smoothly into modern development loops.

*   **Watch Mode (`--watch` / `-w`):**
    *   *Improvement:* Keep the compiler running and monitor the input file and all `#include` dependencies. Automatically recompile whenever any file is updated.
*   **Dry-run/Syntax Check Mode (`--check` / `-c`):**
    *   *Improvement:* Run the preprocessor, tokenizer, label-resolver, and validation checks without generating the final `.prg` file. Useful for fast IDE compilation checks.
*   **Emulator Launch Integration (`--run`):**
    *   *Improvement:* Support launching the compiled `.prg` in a configured emulator (such as XEMU / XMega65) immediately after a successful build.
*   **Source Maps Generation:**
    *   *Improvement:* Output a source map mapping locations in the tokenized `.prg` back to the preprocessed/original source lines. This can enable source-level debugging inside emulators or IDE debuggers.

---

## 4. Character Map & Encoding Enhancements

Streamlining string layout and special graphic entry.

*   **Unicode to PETSCII Transliteration:**
    *   *Improvement:* Map standard UTF-8 characters (like box drawing characters `┌ ┐ └ ┘ ─ │` or graphic block symbols) to their closest PETSCII equivalents.
*   **Hex and Binary Literals:**
    *   *Improvement:* Allow the use of modern hexadecimal (`0x45`) and binary (`%10101010`) literals in source, automatically converting them to standard decimal values at tokenization time for compatibility.

---

## 5. Documentation Generator Enhancements (`--docs`)

Expanding the utility's documentation output.

*   **Automatic Code Block Insertion:**
    *   *Improvement:* Include syntax-highlighted code blocks of the subroutines directly within the generated markdown sections.
*   **Table of Contents (TOC) & Indexing:**
    *   *Improvement:* Automatically generate a table of contents and a cross-referenced list of all labels, variables, and imported symbols.
*   **Alternative Formats:**
    *   *Improvement:* Support outputting documentation in HTML or JSON formats (e.g. `--docs-format html`) for easy integration into project websites.

---

## 6. Emulated Language Features (Syntactic Sugar & Compiler Transpilation)

These features allow developers to use modern programming constructs that are preprocessed/transpiled down to standard CBM/MEGA65 BASIC tokens, significantly simplifying code readability and structure without adding runtime performance overhead.

*   **Subroutines/Procedures with Local Scope and Parameters:**
    *   *Concept:* Allow defining named procedures with parameters and local variables:
      ```basic
      sub draw_sprite(x, y, sprite_id)
          local frame = sprite_id * 64
          # compiler generates standard BASIC setting global tmp vars and GOSUB
      end sub
      ```
    *   *Emulation:* The compiler replaces subroutine calls with variable assignments to temporary mangled registers (e.g., `__draw_sprite_x = 10`) followed by a `GOSUB`. Local variables are either stored on a simulated software stack or mapped to unique global names to prevent collisions.
*   **Compile-Time Constants and Enums:**
    *   *Concept:* Allow defining constants or enumerations that do not consume variables or RAM at runtime:
      ```basic
      const SCREEN_WIDTH = 80
      enum Color { BLACK, WHITE, RED, CYAN }
      ```
    *   *Emulation:* The compiler replaces occurrences of constants/enum values inline with their literal values (e.g., replacing `Color.RED` with `2`) during tokenization.
*   **Structures / Records (Parallel Array / Byte Offset Mapping):**
    *   *Concept:* Group related properties under a struct to avoid managing multiple parallel arrays:
      ```basic
      struct Entity
          x as integer
          y as integer
          color as integer
      end struct
      dim players(10) as Entity
      ```
    *   *Emulation:* The compiler maps `players(i).x` to a backend array `players_x%(i)`, avoiding complex manual variable naming and index arithmetic.
*   **Namespaces / Scope Modules:**
    *   *Concept:* Organize library files into namespaces to avoid name clashes when using `#include`:
      ```basic
      module Math
          sub abs(val)
              ...
          end sub
      end module
      ```
    *   *Emulation:* Variable names and labels inside a module are automatically isolated using the **Variable Name Translation** (mangling) engine described below.
*   **Variable Name Translation (Name Mangling):**
    *   *Concept:* Solve the severe limitation where Commodore/MEGA65 BASIC only considers the first two characters of a variable name and tokenizes keywords embedded in variables. Developers can write readable, descriptive variable names of any length (e.g., `player_health`, `enemy_y_velocity`, `Math.abs_val`).
    *   *Emulation:* The compiler translates every unique user-defined variable to a safe, compiler-generated 2-character name (e.g., `A0%`, `A1$`, `B9`) in the tokenized output. The translator:
        *   Preserves variable type suffixes (e.g., `%`, `$`).
        *   Keeps an internal symbol-to-mangled-name mapping table to ensure consistent translation across the program.
        *   Excludes reserved system variables (like `ST`, `TI`, `TI$`).
        *   Excludes all two-letter CBM BASIC keywords (e.g., `TO`, `IF`, `ON`, `OR`, `GO`, `FN`, `DO`, `GR`) and combinations that form other keywords to prevent tokenization conflicts.
*   **Structured Loops and Control Flow:**
    *   *Concept:* Implement cleaner control structures that are missing or restricted in older BASIC standards:
      ```basic
      switch player_state
          case STATE_IDLE: print "idle"
          case STATE_WALKING: print "walk"
          default: print "unknown"
      end switch
      ```
    *   *Emulation:* Transpiled into conditional `IF ... GOTO` jumps automatically.
*   **Inline Assembly Integration:**
    *   *Concept:* Inject short blocks of assembly directly into the BASIC source code:
      ```basic
      asm
          lda #$01
          sta $d020
          rts
      end asm
      ```
    *   *Emulation:* The compiler auto-assembles the code, inserts it as tokenized `DATA` statements in the CBM BASIC binary, and replaces the block with a `SYS` call to the auto-calculated address.

---

## 7. Archive & Bundling Support (Disk / SFX Packaging)

It is highly practical to add archive and packaging features. Developers writing CBM/MEGA65 applications rarely distribute just a single BASIC file; their projects usually consist of the main BASIC program plus external payloads (like machine code binaries, sprite graphics, character sets, and music data). 

Adding archive/disk packaging options makes distribution and execution significantly nicer for the end user:

*   **Automatic `.d81` Disk Image Generation:**
    *   *Improvement:* Introduce a `--pack` or `-p` flag that compiles the BASIC program and bundles it, along with specified resource files (e.g., C compiler `.prg` outputs, `.bin` graphics, audio assets), directly into a formatted `.d81` disk image.
    *   *Example:*
      ```bash
      basic45 game.bas -o adventure.d81 --pack engine.prg --pack sprites.bin --pack music.sid
      ```
    *   *User Benefit:* The end user gets a single disk image they can mount in an emulator (XEMU) or write to an SD card for a real MEGA65, rather than dealing with multiple loose files.
*   **Single-File Executable Packaging (SFX / Monolithic Loader):**
    *   *Improvement:* Provide a flag (e.g., `--sfx`) that bundles external code and asset binaries directly into the *same* `.prg` file as the BASIC program.
    *   *Emulation:* The tool appends the external binary data to the end of the compiled BASIC code block. It injects a small, machine-code decruncher/copy routine at the start of the BASIC program. When the program runs, it automatically copies the appended binary assets from their loaded location to their target memory locations (e.g., relocating graphics to `$D000` or assembly to `$2000`) in RAM before launching the main BASIC loop.
    *   *User Benefit:* Eliminates runtime disk-loading latency and yields a single standalone program file.
*   **Archiving Compression Integration (Exomizer / PuCrunch):**
    *   *Improvement:* Support integrating a crunching/compression step (like Exomizer or PuCrunch) during the single-file SFX generation to compress the combined BASIC and binary assets. The program will decompress itself in RAM upon startup.
    *   *User Benefit:* Drastically reduces loading times from floppy drives/SD cards and fits larger games into RAM.

---

## 8. Link-Time Common Routine Injection (Standard Library)

Integrating a standard library mechanism at BASIC generation time allows developers to call common, highly optimized helper subroutines without manual duplication or copy-pasting.

*   **Static Routine Linking (Auto-Injection):**
    *   *Concept:* The compiler maintains a library of common BASIC routines (e.g., math functions, 16-bit PEEK/POKE helpers, fast screen clearing, double buffering setups, and joystick reading). If the compiler detects a reference to a library subroutine in the source code (e.g., `gosub {sys.read_joystick}`), it automatically appends the implementation of that routine to the end of the emitted BASIC binary, assigns it a line number, and links the GOSUB jump target correctly.
    *   *User Benefit:* Only the helper routines actually used in the code are injected (dead-code elimination), keeping program size minimal.
*   **Built-in Assembly Helpers:**
    *   *Concept:* Many common tasks (like fast block RAM memory copies, DMA operations, or audio playback initialization) are extremely slow in pure BASIC. The compiler could bundle a set of pre-compiled machine code helper routines.
    *   *Emulation:* When a developer references one of these routines, the compiler automatically injects the compiled byte sequences (via DATA statements or appending to the binary) and generates the corresponding `SYS` call in the BASIC source.
*   **Custom Library Path (`--lib`):**
    *   *Concept:* Allow developers to create their own library files containing subroutines and include them via a flag:
      ```bash
      basic45 game.bas --lib my_utility_lib.bas -o game.prg
      ```
      The compiler matches label/subroutine calls in `game.bas` with implementations in `my_utility_lib.bas`, importing only the referenced routines at generation time.



