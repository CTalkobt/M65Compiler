#pragma once
#include "AsmWriter.hpp"

// KickAssembler writer — converts semantic IR to KickAssembler assembly
// Outputs KickAssembler-compatible syntax with:
// - !byte, !word, !long directives
// - !align, !org directives
// - !import, !export for external symbols
// - Flattened labels (no namespace support in output by default)
class KickAssemblerWriter : public AsmWriter {
public:
    KickAssemblerWriter() = default;
    std::string write(const AsmIR::Module& module) override;

private:
    std::string formatInstruction(const AsmIR::Instruction& instr);
    std::string formatDirective(const AsmIR::Directive& dir);
    std::string formatAddressingMode(const AsmIR::Instruction& instr);
    std::string formatImmediateValue(uint32_t value, int width);
};
