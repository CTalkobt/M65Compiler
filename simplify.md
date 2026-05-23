# Project Simplification and Duplication Reduction Plan

This document outlines areas in the `ccomp` project where code can be simplified, duplication reduced, and architectural improvements made.

## 1. Unified Lexer Base
**Observation:** `Lexer.cpp` (C lexer) and `AssemblerLexer.cpp` (Assembly lexer) share significant logic for:
- Character consumption (`get`, `peek`, `peekNext`)
- Position tracking (`line`, `column`)
- Whitespace and comment skipping (C-style `//` and `/* */` are supported by both)

**Recommendation:** Create a `LexerBase` class that handles these common tasks. Both `Lexer` and `AssemblerLexer` should inherit from this base class or use it as a utility.

## 2. Unification of Code Generation Pipelines
**Observation:** The project currently maintains two code generation paths:
- Legacy `CodeGenerator` (AST → Assembly)
- Modern `IRBuilder` + `IRCodeGen` (AST → IR → Assembly)

In `cc45_main.cpp`, the legacy `CodeGenerator` is still used as a "validator" even when IR codegen is enabled because it catches certain semantic errors (struct/type errors) that `IRBuilder` doesn't yet fully validate.

**Recommendation:** 
- Port all validation logic from `CodeGenerator` to `IRBuilder` or a dedicated `SemanticAnalyzer`.
- Retire the legacy `CodeGenerator` once `IRBuilder` is feature-complete and self-validating.
- This will remove thousands of lines of duplicated logic for type checking and struct layout.

## 3. Unified Type System
**Observation:** Types are currently handled using `std::string typeName` and `int pointerLevel` throughout the compiler. This leads to repetitive string comparisons (e.g., `if (typeName == "int")`) and manual pointer level tracking.

**Recommendation:** Implement a robust `Type` class or structure that can represent:
- Base types (int, char, void, etc.)
- Pointer levels
- Array dimensions
- Struct/Union references
- Signedness
- Const/Volatile qualifiers

Using a proper type system will simplify `IRBuilder`, `CodeGenerator`, and `ConstantFolder`.

## 4. Shared Struct/Union Layout Logic
**Observation:** Both `CodeGenerator` and `IRBuilder` maintain their own internal maps and logic for computing struct layouts (`StructInfo` vs `IRStructInfo`).

**Recommendation:** Extract struct/union layout computation into a shared `TypeDatabase` or `TypeSystem` component that both pipelines can use.

## 5. Centralized Numeric Literal Parsing [COMPLETED]
**Observation:** The function `parseNumericLiteral` was duplicated in:
- `AssemblerParser.cpp`
- `AssemblerSimulatedOps.cpp`
- `AssemblerGenerator.cpp`

**Recommendation:** Move this and other common string utility functions to a shared utility header (e.g., `StringUtil.hpp`).

## 6. Constant Definitions for Hardware Addresses [COMPLETED]
**Observation:** `AssemblerSimulatedOps.cpp` and `AssemblerExpression.cpp` contained hardcoded memory addresses for MEGA65 hardware features (e.g., `$D700` for the math accelerator).

**Recommendation:** Create a header file (e.g., `Mega65Registers.hpp`) with named constants for these addresses to improve readability and maintainability.

## 7. Refactor `cc45_main.cpp`
**Observation:** The `main` function in `cc45_main.cpp` is extremely large and handles preprocessing, lexing, parsing, validation, IR generation, IR dump, and assembly emission in a single block.

**Recommendation:** Refactor the compilation pipeline into a `Compiler` class with distinct stages. This will make the driver logic easier to follow and test.

## 8. Assembler and C Parser Utilities
**Observation:** While the languages differ, `AssemblerParser` and `Parser` share logic for token matching and error reporting.

**Recommendation:** Consider a `ParserBase` class to provide standard `match`, `expect`, and `error` methods.

## 10. Direct CPU Register Usage Optimization [COMPLETED]
**Observation:** Initially, the compiler snapshotted CPU registers into virtual registers.

**Recommendation:** Implemented direct instruction selection (LDX, LDY, SEC, etc.) for register and flag assignments. This ensures efficient, clobber-free code for sequences like `__cpu.X = 5; __flags.Carry = 1;`.


