#pragma once
#include "AsmWriter.hpp"

// ca45 writer — converts semantic IR to ca45 assembly
class Ca45Writer : public AsmWriter {
public:
    Ca45Writer() = default;
    std::string write(const AsmIR::Module& module) override;

private:
    std::string formatInstruction(const AsmIR::Instruction& instr);
    std::string formatDirective(const AsmIR::Directive& dir);
    std::string formatAddressingMode(const AsmIR::Instruction& instr);
};
