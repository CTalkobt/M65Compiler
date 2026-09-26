#pragma once
#include "AsmParser.hpp"
#include "AssemblerLexer.hpp"
#include "AssemblerParser.hpp"
#include <memory>

// ca45 parser — converts ca45 assembly to semantic IR
class Ca45Parser : public AsmParser {
public:
    Ca45Parser() = default;
    AsmIR::Module parse(const std::string& source) override;

private:
    AsmIR::AddressingMode mapAddressingMode(AddressingMode mode);
    AsmIR::DirectiveType mapDirectiveType(const std::string& name);
    void extractSymbols(const AssemblerParser& parser, AsmIR::Module& module);
    void extractStatements(const AssemblerParser& parser, AsmIR::Module& module, const std::string& source);
};
