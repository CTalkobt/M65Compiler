#pragma once
#include "AsmWriter.hpp"

// BSA (the Bit Shift Assembler) writer (#371): .CPU first and a .STORE of
// the program (BSA writes no binary without one); .BYTE / .WORD / .FILL n (v)
// / .REAL; ca45's < and > (which take the rest of the expression) in
// parentheses, BSA's take the next operand only; @locals as label__name (BSA's
// @names are global); imports, exports and segments as comments (BSA
// assembles one program, without a linker).
class BsaWriter : public AsmWriter {
public:
    BsaWriter() = default;
    std::string write(const AsmIR::Module& module) override;

private:
    std::string formatInstruction(const AsmIR::Instruction& instr);
    std::string formatDirective(const AsmIR::Directive& dir);
    bool segmentWarned_ = false;
};
