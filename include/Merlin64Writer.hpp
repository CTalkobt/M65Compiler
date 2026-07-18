#pragma once
#include "AsmWriter.hpp"
#include <string>

// Merlin 64 writer — outputs semantic IR as Merlin 64 (Commodore 6502 assembler) syntax
class Merlin64Writer : public AsmWriter {
public:
    Merlin64Writer() = default;
    std::string write(const AsmIR::Module& module) override;

private:
    std::string formatInstruction(const AsmIR::Instruction& instr);
    std::string formatAddressingMode(const AsmIR::Instruction& instr);
    std::string formatDirective(const AsmIR::Directive& dir);
    std::string formatDirectiveKeyword(const std::string& irName);
};
