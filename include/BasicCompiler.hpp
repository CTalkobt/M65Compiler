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

struct BasicCompileResult {
    bool ok = false;
    // BASIC line number (0: none), message
    std::vector<std::pair<int, std::string>> errors;
    std::shared_ptr<ir::Module> module;
};

// prg: a tokenized BASIC 65 program (its load address first)
BasicCompileResult compileBasicToIR(const std::vector<uint8_t>& prg, const std::string& sourceName);

// The module as ca45 assembly (relocatable: ca45 -c, then ln45 -basic with
// c45.lib): cc45's IR passes at -O<optLevel>, then IRCodeGen. Throws on an
// error of the code generator.
std::string basicIRToAssembly(std::shared_ptr<ir::Module> module, int optLevel, const std::string& sourceName);
