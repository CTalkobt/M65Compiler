#pragma once
#include "AsmWriter.hpp"

class X65Writer : public AsmWriter {
public:
    X65Writer() = default;
    std::string write(const AsmIR::Module& module) override;

private:
    std::string formatInstruction(const AsmIR::Instruction& instr);
    std::string formatAddressingMode(const AsmIR::Instruction& instr);
    std::string formatDirective(const AsmIR::Directive& dir);
};
