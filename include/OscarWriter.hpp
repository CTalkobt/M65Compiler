#pragma once
#include "AsmWriter.hpp"
#include <string>

// Oscar writer — outputs semantic IR as Oscar (vintage Commodore 6502 assembler) syntax
class OscarWriter : public AsmWriter {
public:
    OscarWriter() = default;
    std::string write(const AsmIR::Module& module) override;

private:
    std::string formatInstruction(const AsmIR::Instruction& instr);
    std::string formatAddressingMode(const AsmIR::Instruction& instr);
    std::string formatDirective(const AsmIR::Directive& dir);
    std::string formatDirectiveKeyword(const std::string& irName);
};
