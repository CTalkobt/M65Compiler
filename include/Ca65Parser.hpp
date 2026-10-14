#pragma once
#include "AsmParser.hpp"

// ca65 (cc65): .byte/.word/.dword/.res, .import/.export (also the zp forms),
// .segment and the .code/.data/.bss shortcuts, .setcpu, labels with ':' (@local),
// NAME = e / NAME := e, .macro ... .endmacro
// Read by the shared DialectReader (see DialectReader.hpp)
class Ca65Parser : public AsmParser {
public:
    Ca65Parser() = default;
    AsmIR::Module parse(const std::string& source) override;
};
