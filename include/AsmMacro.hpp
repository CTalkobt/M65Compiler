// AsmMacro.hpp — ca45's multi-line macros (.macro / .endmacro)
//
//   .macro push_regs            ; a name, then parameters (commas or spaces)
//       pha
//       phx
//   .endmacro                   ; or .endm
//   .macro store val, addr
//       lda #(val)
//       sta addr
//   .endmacro
//       push_regs               ; a use: the body in its place
//       store 5, $d020          ; arguments: split at the commas outside ( ) and literals
//
// The expansion runs after the C preprocessor (#define, #include) and before
// the lexer. A parameter is replaced where it is a whole word (not inside a
// string, a character constant or a comment); \@ is a number of its own in
// each expansion (labels that a macro used twice can have:
// @loop\@). An expansion is expanded again, so macros use and define other
// macros; one that goes on using itself stops at 64 levels with an error.
// Every expanded line is marked with the line of its use (#line), so the
// assembler's messages point there.
#pragma once
#include <string>

// The source with its macros expanded; false with err set (a .macro without
// .endmacro, the wrong number of arguments, a recursion too deep)
bool expandAsmMacros(const std::string& source, const std::string& file, std::string& out, std::string& err);
