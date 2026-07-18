#pragma once
#include "AsmWriter.hpp"

// ACME writer — converts semantic IR to ACME 6502 assembly
// Outputs ACME-compatible syntax with:
// - !byte, !word, !long pseudo-commands
// - *=address for ORG
// - !org equivalent directives
// - Proper ACME addressing mode syntax
class AcmeWriter : public AsmWriter {
public:
    AcmeWriter() = default;
    std::string write(const AsmIR::Module& module) override;

private:
    std::string formatInstruction(const AsmIR::Instruction& instr);
    std::string formatDirective(const AsmIR::Directive& dir);
    std::string formatAddressingMode(const AsmIR::Instruction& instr);
    std::string formatPseudoDirective(const std::string& irName);
};
