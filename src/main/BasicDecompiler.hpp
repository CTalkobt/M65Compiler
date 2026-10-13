// BasicDecompiler — a tokenized BASIC 65 program (.prg) to basic45 source
//
// Each line becomes a source line; a line that is the target of GOTO, GOSUB,
// GO TO, ON ... GOTO/GOSUB, THEN, ELSE, RESTORE, TRAP, RESUME or RUN gets a
// label l<number>, and the references use it. Keywords and names come out
// in lower case; strings keep their PETSCII control codes as {name} escapes
// ({$xx} for a byte with no name).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct BasicDecompileResult {
    std::string source;
    std::vector<std::string> warnings;
    bool ok = false;
    std::string error;
};

BasicDecompileResult decompileBasic(const std::vector<uint8_t>& prg);
