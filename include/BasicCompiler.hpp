#pragma once
// basic45 --compile (#369): a tokenized BASIC 65 program to cc45's IR
// (ir::Module), which cc45's IR passes and IRCodeGen make into ca45 assembly.
// The program's statements become the function main; its variables globals
// (floats, 16-bit integers, strings); BASIC's arithmetic the float runtime of
// the library (the BASIC 65 ROM's routines); PRINT, the strings and the
// errors lib/stdlib_asm/basic_rt.s45 and lib/stdlib/basic_str.c.
// See doc/bin/basic45.md (Compiling).

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ir { struct Module; }

// Chaining (DLOAD / LOAD, #369): REM PRAGMA IMPORT's map file when the
// pragma names none (basic45 --import-map), and where the pragma's names
// are found (the source's directory)
struct BasicChainOptions {
    std::string importMap;
    std::string sourceDir;
    // --line-numbers: each line that can raise an error records its number
    // (the messages' " IN line", EL); off: neither (#483)
    bool lineNumbers = false;
    // --checks: the checks of the program's own values and flow (?ILLEGAL
    // QUANTITY, ?BAD SUBSCRIPT, ?SYNTAX ERROR when run, RETURN without GOSUB,
    // ...); off: none, undefined behaviour instead (#489)
    bool checks = false;
};

struct BasicCompileResult {
    bool ok = false;
    // REM PRAGMA EXPORT: the map file's text (empty: no export)
    std::string exportMap;
    // BASIC line number (0: none), message
    std::vector<std::pair<int, std::string>> errors;
    // BASIC line number, message: what compiles but differs from the interpreter
    std::vector<std::pair<int, std::string>> warnings;
    std::shared_ptr<ir::Module> module;
};

// prg: a tokenized BASIC 65 program (its load address first). speed
// (--speed): + - * / and comparisons of floats by the RAM routines
// (float_fast.s45), not the ROM's
BasicCompileResult compileBasicToIR(const std::vector<uint8_t>& prg, const std::string& sourceName,
                                    bool speed = false, const BasicChainOptions* chain = nullptr);

// The module as ca45 assembly (relocatable: ca45 -c, then ln45 -basic with
// c45.lib): cc45's IR passes at -O<optLevel>, then IRCodeGen. Throws on an
// error of the code generator.
std::string basicIRToAssembly(std::shared_ptr<ir::Module> module, int optLevel, const std::string& sourceName);
