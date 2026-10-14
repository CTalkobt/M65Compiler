#pragma once
#include "AsmParser.hpp"

// Oscar: labels in column 1 (no ':'), BYTE/DB, WORD/DW, ORG, GLOBAL, EXTERN
// Read by the shared DialectReader (see DialectReader.hpp)
class OscarParser : public AsmParser {
public:
    OscarParser() = default;
    AsmIR::Module parse(const std::string& source) override;
};
