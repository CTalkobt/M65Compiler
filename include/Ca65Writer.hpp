#pragma once
#include "AsmWriter.hpp"

// ca65 writer — converts semantic IR to ca65 (CC65) assembly
// Outputs ca65-compatible syntax with:
// - .import / .export for external symbols
// - .segment directives
// - .byte, .word, .long directives
// - Local labels with @ prefix (reconstructed from flattened names where possible)
class Ca65Writer : public AsmWriter {
public:
    Ca65Writer() = default;
    std::string write(const AsmIR::Module& module) override;

private:
    std::string formatInstruction(const AsmIR::Instruction& instr);
    std::string formatDirective(const AsmIR::Directive& dir);
    std::string formatAddressingMode(const AsmIR::Instruction& instr);
    std::string formatDirectiveName(const std::string& irName);
};
