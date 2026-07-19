# cvt_asm — Assembler Format Converter

**Status:** v1.0.5 (Stable)  
**Purpose:** Convert assembly source code between different 6502 assembler formats.

---

## Overview

`cvt_asm` is a format-agnostic assembler converter built on a semantic intermediate representation (IR). It allows conversion between different C64/MEGA65 assembler syntaxes while preserving the intent and structure of the code.

### Design Philosophy

- **Semantic IR**: Normalizes syntax to a common representation (instructions, directives, symbols, segments)
- **Format-Agnostic**: Each assembler is plugged in via parser/writer implementations
- **Extensible**: Adding new formats requires only implementing a parser and writer
- **Lossless (for supported features)**: Converts between assemblers without losing semantic meaning

### Current Status

**Phase 1 (Complete):**
- Semantic IR for 6502/45GS02 instructions and directives
- Generic parser/writer registry system
- ca45 parser and writer (full implementation)

**Phase 2 (Complete):**
- KickAssembler parser and writer (full implementation)
- Namespace support (!namespace / !endnamespace)
- Directive flattening and conversion
- Bidirectional conversion (ca45 ↔ kickassembler)

**Future Phases:**
- ca65 parser and writer (CC65 compatible)
- Other assemblers (Oscar, Merlin, Buddy/Power Assembler, etc.)

---

## Usage

### Basic Conversion

```bash
# Convert ca45 to ca45 (test and normalize)
cvt_asm input.s45 -o output.s45

# Convert using verbose output
cvt_asm -v input.s45

# Specify formats explicitly
cvt_asm -f ca45 -t ca45 input.s45 -o output.s45
```

### Command-Line Options

```
cvt_asm [options] <input-file>

Options:
  -f <format>      Input format (default: ca45)
  -t <format>      Target format (default: ca45)
  -o <file>        Output file (default: stdout)
  -l               List supported formats and exit
  -v               Verbose output (parse/write diagnostics)
  --help           Show usage information
  --version        Show version
```

### List Available Formats

```bash
cvt_asm -l
```

Output:
```
Supported input formats:
  ca45
  kickassembler

Supported output formats:
  ca45
  kickassembler
```

---

## Semantic IR (AsmIR)

The intermediate representation captures the essential structure of assembly code:

### Statement Types

- **INSTRUCTION**: CPU instruction (mnemonic, addressing mode, operand)
- **DIRECTIVE**: Assembler directive (.byte, .global, .align, etc.)
- **LABEL**: Symbol definition
- **COMMENT**: Comment line
- **BLANK**: Empty line

### Addressing Modes

The IR supports all 6502 and 45GS02 addressing modes:

```
IMPLIED
IMMEDIATE
ZERO_PAGE / BASE_PAGE
ZERO_PAGE_X / BASE_PAGE_X
ZERO_PAGE_Y / BASE_PAGE_Y
ABSOLUTE
ABSOLUTE_X
ABSOLUTE_Y
INDIRECT
INDIRECT_X / BASE_PAGE_X_INDIRECT
INDIRECT_Y / BASE_PAGE_INDIRECT_Y
STACK_RELATIVE
FRAME_RELATIVE
INDEXED_PAIR (45GS02)
```

### Directive Types

```
BYTE_DATA, WORD_DATA, LONG_DATA
FILL, ALIGN, ORG
SEGMENT, GLOBAL, EXTERN, WEAK, CONST
CPU
FUNCTION, ZP_USES, ZP_CLOBBERS, REG_CLOBBERS, FLAG_CLOBBERS
LOC (source location)
OTHER
```

---

## Architecture

### Parser Registry

Parsers are registered at startup:

```cpp
ParserRegistry::getInstance().registerParser("ca45", 
    []() -> AsmParser* { return new Ca45Parser(); });
```

Each parser:
- Inherits from `AsmParser` (abstract base)
- Implements `parse(const std::string& source) -> AsmIR::Module`
- Collects errors and warnings during parsing

### Writer Registry

Writers follow the same pattern:

```cpp
WriterRegistry::getInstance().registerWriter("ca45",
    []() -> AsmWriter* { return new Ca45Writer(); });
```

Each writer:
- Inherits from `AsmWriter` (abstract base)
- Implements `write(const AsmIR::Module& module) -> std::string`
- Formats and emits the IR in the target syntax

### Adding a New Format

To add support for a new assembler format (e.g., ca65):

1. **Create parser header** (`include/Ca65Parser.hpp`):
   ```cpp
   class Ca65Parser : public AsmParser {
       AsmIR::Module parse(const std::string& source) override;
   private:
       // Conversion helpers
   };
   ```

2. **Implement parser** (`src/main/Ca65Parser.cpp`):
   - Use ca65's lexer/parser to build the IR
   - Map ca65 directives to IR types
   - Preserve operand text for symbols

3. **Create writer header** (`include/Ca65Writer.hpp`):
   ```cpp
   class Ca65Writer : public AsmWriter {
       std::string write(const AsmIR::Module& module) override;
   private:
       // Formatting helpers
   };
   ```

4. **Implement writer** (`src/main/Ca65Writer.cpp`):
   - Emit IR statements in ca65 syntax
   - Handle ca65-specific directives
   - Format addresses and symbols

5. **Register in main**:
   ```cpp
   static void registerFormats() {
       ParserRegistry::getInstance().registerParser("ca65", 
           []() -> AsmParser* { return new Ca65Parser(); });
       WriterRegistry::getInstance().registerWriter("ca65",
           []() -> AsmWriter* { return new Ca65Writer(); });
   }
   ```

6. **Update Makefile**:
   ```make
   CVT_ASM_OBJECTS += $(OBJ_DIR)/Ca65Parser.o $(OBJ_DIR)/Ca65Writer.o
   ```

---

## Conversion Examples

### Simple Instruction Preservation

**Input (ca45):**
```asm
main:
    lda #$42
    sta $d020
    rts
```

**Output (ca45):**
```asm
main:
    lda #$42
    sta $d020
    rts
```

Operand text is preserved during the round-trip.

### Symbol and Label Handling

**Input:**
```asm
.global counter
.extern update_screen

loop:
    lda counter
    jsr update_screen
    bne loop
```

**Parsed IR:**
- `counter` captured as extern symbol
- `update_screen` captured as extern symbol
- `loop` captured as label with references in IR

**Output (ca45):**
```asm
.extern counter
.extern update_screen

loop:
    lda counter
    jsr update_screen
    bne loop
```

### Directive Normalization

Directives are preserved but normalized to the target format:

| ca45 | IR | KickAssembler |
|------|----|----|
| `.byte 1, 2` | BYTE_DATA | `!byte 1, 2` |
| `.word $1000` | WORD_DATA | `!word $1000` |
| `.align 256` | ALIGN | `!align 256` |
| `.global foo` | GLOBAL | `!export foo` |

### KickAssembler Namespace Flattening

KickAssembler namespaces are flattened to dot-notation labels during conversion, enabling use in other assemblers:

**Input (KickAssembler):**
```asm
!namespace game
    main:
        lda #$42
        rts
!endnamespace
```

**Output (ca45):**
```asm
game.main:
    lda #$42
    rts
```

**Output (KickAssembler, round-trip):**
```asm
game.main:
    LDA #$42
    RTS
```

Labels with dots are preserved during round-trips. Conversions between formats that don't support namespaces use flattened names.

---

## Limitations

### Macro Expansion

Macros in the source are expanded during parsing and stored as individual statements in the IR. When converting to a format that supports macros, the original macro structure is not recovered (it would require additional metadata during parsing).

**Workaround for phase 2:** Store macro definitions separately in IR, allowing lossless conversions between formats that both support macros.

### Format-Specific Features

Some formats have features not representable in the IR:

| Feature | ca45 | ca65 | KickAss |
|---------|------|------|---------|
| Macros | ✓ | ✓ | ✓ |
| Namespaces | ✗ | ✗ | ✓ |
| Block syntax | ✗ | ✗ | ✓ |
| Scope blocks | ✗ | ✗ | ✓ |

**Fallback:** When converting a format with unsupported features to one without, cvt_asm flattens the structure and emits a warning.

---

## Error Handling

Parse and write errors are collected and reported:

```bash
$ cvt_asm bad.s45
Parse error: Unknown directive: .baddir
Parse error: Unexpected token at line 5

$ cvt_asm -v bad.s45
Parsing...
Parse error: Unknown directive: .baddir
Parse error: Unexpected token at line 5
```

Use `-v` for detailed diagnostics.

---

## Technical Details

### Build

```bash
make bin/cvt_asm       # Build just cvt_asm
make -j12 all          # Build all tools including cvt_asm
```

### Dependencies

- **Existing ca45 parser**: Leverages AssemblerParser, AssemblerLexer
- **Preprocessor**: Expands macros and handles directives
- **C++17**: Semantic IR uses standard library features

### Performance

Typical conversion time for a 1000-line assembly file: <100ms.

---

## Future Work

### Phase 2: Cross-Format Conversions

- [ ] ca65 parser and writer
- [ ] Full compatibility matrix
- [ ] Format-specific optimizations (e.g., preserve ca65's local label syntax)

### Phase 3: Advanced Features

- [ ] Macro preservation (for macro-aware conversions)
- [ ] Configurable output formatting
- [ ] Batch conversion tool

### Phase 4: Integration

- [ ] IDE plugin support
- [ ] Build system integration
- [ ] Incremental conversion

---

## See Also

- `ca45.md` — ca45 assembler documentation
- `.../calling-conventions.md` — MEGA65 calling conventions
- `lib45.md` — .o45 relocatable object format
