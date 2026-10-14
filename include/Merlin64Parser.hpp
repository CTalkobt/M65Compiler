#pragma once
#include "AsmParser.hpp"

// Merlin 64: labels in column 1 (no ':'), the opcode or pseudo-op after
// whitespace; ORG, EQU (=), DFB/DB, HEX, DW/DA, ASC, DS, ENT/EXT; '*' or ';'
// in column 1 starts a comment line; NAME MAC ... <<< (EOM) macros
// Read by the shared DialectReader (see DialectReader.hpp)
class Merlin64Parser : public AsmParser {
public:
    Merlin64Parser() = default;
    AsmIR::Module parse(const std::string& source) override;
};
