#include "KickAssemblerWriter.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

std::string KickAssemblerWriter::write(const AsmIR::Module& module) {
    std::stringstream out;

    // Emit globals/exports
    for (const auto& [name, sym] : module.symbols) {
        if (sym.is_global) {
            out << "!import " << name << "\n";
        } else if (sym.is_extern) {
            out << "!export " << name << "\n";
        }
    }

    if (!module.symbols.empty()) {
        out << "\n";
    }

    // Emit statements
    for (const auto& stmt : module.statements) {
        switch (stmt.type) {
            case AsmIR::Statement::Type::LABEL:
                out << stmt.label << ":\n";
                break;

            case AsmIR::Statement::Type::INSTRUCTION: {
                out << "    " << formatInstruction(stmt.instr) << "\n";
                break;
            }

            case AsmIR::Statement::Type::DIRECTIVE: {
                out << formatDirective(stmt.dir) << "\n";
                break;
            }

            case AsmIR::Statement::Type::COMMENT:
                out << "; " << stmt.comment << "\n";
                break;

            case AsmIR::Statement::Type::BLANK:
                out << "\n";
                break;
        }
    }

    return out.str();
}

std::string KickAssemblerWriter::formatInstruction(const AsmIR::Instruction& instr) {
    std::stringstream ss;
    ss << instr.mnemonic;

    if (instr.mode != AsmIR::AddressingMode::IMPLIED) {
        ss << " " << formatAddressingMode(instr);
    }

    return ss.str();
}

std::string KickAssemblerWriter::formatDirective(const AsmIR::Directive& dir) {
    std::stringstream ss;

    // KickAssembler uses ! prefix for directives
    ss << "!" << dir.name;

    for (const auto& arg : dir.arguments) {
        ss << " " << arg;
    }

    return ss.str();
}

std::string KickAssemblerWriter::formatAddressingMode(const AsmIR::Instruction& instr) {
    std::stringstream ss;

    switch (instr.mode) {
        case AsmIR::AddressingMode::IMPLIED:
            break;
        case AsmIR::AddressingMode::IMMEDIATE:
            if (!instr.operand.text.empty()) {
                ss << "#" << instr.operand.text;
            } else {
                ss << "#$" << std::hex << std::setw(2) << std::setfill('0') << instr.operand.value;
            }
            break;
        case AsmIR::AddressingMode::ZERO_PAGE:
            if (!instr.operand.text.empty()) {
                ss << instr.operand.text;
            } else {
                ss << "$" << std::hex << std::setw(2) << std::setfill('0') << (instr.operand.value & 0xFF);
            }
            break;
        case AsmIR::AddressingMode::ZERO_PAGE_X:
            if (!instr.operand.text.empty()) {
                ss << instr.operand.text << ",x";
            } else {
                ss << "$" << std::hex << std::setw(2) << std::setfill('0') << (instr.operand.value & 0xFF) << ",x";
            }
            break;
        case AsmIR::AddressingMode::ZERO_PAGE_Y:
            if (!instr.operand.text.empty()) {
                ss << instr.operand.text << ",y";
            } else {
                ss << "$" << std::hex << std::setw(2) << std::setfill('0') << (instr.operand.value & 0xFF) << ",y";
            }
            break;
        case AsmIR::AddressingMode::ABSOLUTE:
            if (!instr.operand.text.empty()) {
                ss << instr.operand.text;
            } else {
                ss << "$" << std::hex << std::setw(4) << std::setfill('0') << (instr.operand.value & 0xFFFF);
            }
            break;
        case AsmIR::AddressingMode::ABSOLUTE_X:
            if (!instr.operand.text.empty()) {
                ss << instr.operand.text << ",x";
            } else {
                ss << "$" << std::hex << std::setw(4) << std::setfill('0') << (instr.operand.value & 0xFFFF) << ",x";
            }
            break;
        case AsmIR::AddressingMode::ABSOLUTE_Y:
            if (!instr.operand.text.empty()) {
                ss << instr.operand.text << ",y";
            } else {
                ss << "$" << std::hex << std::setw(4) << std::setfill('0') << (instr.operand.value & 0xFFFF) << ",y";
            }
            break;
        case AsmIR::AddressingMode::INDIRECT:
            ss << "(" << instr.operand.text << ")";
            break;
        case AsmIR::AddressingMode::INDIRECT_X:
            ss << "(" << instr.operand.text << ",x)";
            break;
        case AsmIR::AddressingMode::INDIRECT_Y:
            ss << "(" << instr.operand.text << "),y";
            break;
        case AsmIR::AddressingMode::STACK_RELATIVE:
            ss << instr.operand.text << ",s";
            break;
        case AsmIR::AddressingMode::FRAME_RELATIVE:
            ss << instr.operand.text;
            break;
        case AsmIR::AddressingMode::INDEXED_PAIR:
            ss << instr.operand.text << ",xy";
            break;
    }

    return ss.str();
}

std::string KickAssemblerWriter::formatImmediateValue(uint32_t value, int width) {
    std::stringstream ss;
    ss << "#$" << std::hex << std::setw(width / 4) << std::setfill('0') << value;
    return ss.str();
}
