#pragma once
#include "AsmParser.hpp"

// x65: .byte/.word/.long (dc.b/dc.w/dc.l), .org, .import/.export
// (xref/xdef), labels with ':' or in column 1
// Read by the shared DialectReader (see DialectReader.hpp)
class X65Parser : public AsmParser {
public:
    X65Parser() = default;
    AsmIR::Module parse(const std::string& source) override;
};
