#pragma once
#include "AsmParser.hpp"

// BSA, the Bit Shift Assembler (https://codeberg.org/root42/BSA; its manual
// is the comment at the top of bsa.c), for the 6502 / 65C02 / 45GS02 (#371).
// A pre-pass (BsaParser.cpp) turns its source into ca45's syntax, which the
// shared DialectReader then reads: expressions parsed by BSA's own rules and
// written fully parenthesized; MACRO / ENDMAC expanded; #if / #ifdef resolved;
// MODULE locals and BSO-style nn$ labels given names of their own; strings,
// .PET / .DISP, .QUAD, .REAL, .BHEX, .BITS, .HEX4 ... as the bytes BSA makes;
// the BSS pointer's labels as constants.
class BsaParser : public AsmParser {
public:
    BsaParser() = default;
    AsmIR::Module parse(const std::string& source) override;
};
