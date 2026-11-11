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

// A line of the program as the decompiler reads it: keywords (upper case)
// and text (numbers, names, strings in basic45's escapes, punctuation)
struct DecompiledPiece {
    bool isKw;
    std::string text;
};
struct DecompiledLine {
    uint16_t num;
    std::vector<DecompiledPiece> pieces;
    std::string label = {};     // its label when not l<num> (a converted program's procedures)
    bool cont = false;          // the rest of the line before it (split by a conversion): no number of its own
};

// The program's dialect: BASIC 65 (decompiled as it is), or another one
// converted to BASIC 65 (--convert=basic7 / basic35 / simons)
enum class BasicSource { B65, V7, V35, SIMONS, B8 };

struct BasicDecompileOptions {
    BasicSource source = BasicSource::B65;
    bool ntsc = false;          // the source machine's timing (TEMPO, PLAY): NTSC, else PAL
    bool tsbneo = false;        // SIMONS: TSBneo's forms (AT(row, column), COLOUR b: the border, PAUSE: any key)
};

struct BasicDecompileResult {
    std::string source;
    std::vector<std::string> warnings;
    bool ok = false;
    std::string error;
};

BasicDecompileResult decompileBasic(const std::vector<uint8_t>& prg,
                                    const BasicDecompileOptions& options = BasicDecompileOptions());

// BasicConvert.cpp: the program's lines from another dialect rewritten as
// BASIC 65 (keywords renamed, statements rewritten, what has no counterpart
// warned about); notes: comment lines for the top of the output
void convertBasicLines(std::vector<DecompiledLine>& lines, const BasicDecompileOptions& options,
                       std::vector<std::string>& warnings, std::vector<std::string>& notes);
