// BasicLister — a tokenized CBM BASIC program as LIST shows it
//
// BASIC V2 (C64, $0801), V7 (C128, $1C01; its $CE xx and $FE xx tokens) and
// BASIC 65 (MEGA65, $2001; the tokens of basic45's tokenizer). Keywords and
// letters come out in upper case; in strings and REM text PETSCII control
// codes are {name} escapes (as basic45 and petcat write them), other bytes
// above $7F {$xx}.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class BasicDialect { AUTO, V2, V7, B65 };

struct BasicListing {
    std::string text;
    BasicDialect dialect = BasicDialect::AUTO;   // the one used
    std::vector<std::string> warnings;
    bool ok = false;
    std::string error;
};

// prg: the file with its 2-byte load address; AUTO picks the dialect from it
BasicListing listBasic(const std::vector<uint8_t>& prg, BasicDialect dialect = BasicDialect::AUTO);
const char* basicDialectName(BasicDialect d);
