#pragma once
#include "AsmParser.hpp"

// KickAssembler: .byte/.word/.dword/.text/.fill/.align, .const/.var/.label
// NAME = e, *= or .pc = for the origin, .cpu _45gs02, // and /* */
// comments, .macro name(args) { ... } called as name(args)
// Read by the shared DialectReader (see DialectReader.hpp)
class KickAssemblerParser : public AsmParser {
public:
    KickAssemblerParser() = default;
    AsmIR::Module parse(const std::string& source) override;
};
