#pragma once
#include "AsmParser.hpp"

// ACME: !byte/!8, !word/!16, !32, !text, !fill, !align, !binary, !cpu (m65 is
// the 45GS02), *= for the origin, labels in column 1 (':' optional),
// !macro name { ... } called as +name
// Read by the shared DialectReader (see DialectReader.hpp)
class AcmeParser : public AsmParser {
public:
    AcmeParser() = default;
    AsmIR::Module parse(const std::string& source) override;
};
