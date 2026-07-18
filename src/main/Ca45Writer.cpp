#include "Ca45Writer.hpp"
#include <sstream>
#include <iomanip>

std::string Ca45Writer::write(const AsmIR::Module& module) {
    std::stringstream out;

    // Emit CPU directive if not default
    if (module.cpu != "45GS02") {
        out << ".cpu " << module.cpu << "\n";
    }

    // Emit symbols (globals, externs, weak)
    for (const auto& [name, sym] : module.symbols) {
        if (sym.is_extern) {
            out << ".extern " << name << "\n";
        } else if (sym.is_global) {
            out << ".global " << name << "\n";
        } else if (sym.is_weak) {
            out << ".weak " << name << "\n";
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
                if (!stmt.instr.label.empty()) {
                    out << stmt.instr.label << ":\n";
                }
                out << "  " << formatInstruction(stmt.instr) << "\n";
                break;
            }

            case AsmIR::Statement::Type::DIRECTIVE: {
                if (!stmt.label.empty()) {
                    out << stmt.label << ":\n";
                }
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

std::string Ca45Writer::formatInstruction(const AsmIR::Instruction& instr) {
    std::stringstream ss;
    ss << instr.mnemonic;

    if (instr.mode != AsmIR::AddressingMode::IMPLIED) {
        ss << " " << formatAddressingMode(instr);
    }

    return ss.str();
}

std::string Ca45Writer::formatAddressingMode(const AsmIR::Instruction& instr) {
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
                ss << instr.operand.text << ", X";
            } else {
                ss << "$" << std::hex << std::setw(2) << std::setfill('0') << (instr.operand.value & 0xFF) << ", X";
            }
            break;
        case AsmIR::AddressingMode::ZERO_PAGE_Y:
            if (!instr.operand.text.empty()) {
                ss << instr.operand.text << ", Y";
            } else {
                ss << "$" << std::hex << std::setw(2) << std::setfill('0') << (instr.operand.value & 0xFF) << ", Y";
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
                ss << instr.operand.text << ", X";
            } else {
                ss << "$" << std::hex << std::setw(4) << std::setfill('0') << (instr.operand.value & 0xFFFF) << ", X";
            }
            break;
        case AsmIR::AddressingMode::ABSOLUTE_Y:
            if (!instr.operand.text.empty()) {
                ss << instr.operand.text << ", Y";
            } else {
                ss << "$" << std::hex << std::setw(4) << std::setfill('0') << (instr.operand.value & 0xFFFF) << ", Y";
            }
            break;
        case AsmIR::AddressingMode::INDIRECT:
            ss << "(" << instr.operand.text << ")";
            break;
        case AsmIR::AddressingMode::INDIRECT_X:
            ss << "(" << instr.operand.text << ", X)";
            break;
        case AsmIR::AddressingMode::INDIRECT_Y:
            ss << "(" << instr.operand.text << "), Y";
            break;
        case AsmIR::AddressingMode::STACK_RELATIVE:
            ss << instr.operand.text << ", S";
            break;
        case AsmIR::AddressingMode::FRAME_RELATIVE:
            ss << ".fp " << instr.operand.text;
            break;
        case AsmIR::AddressingMode::INDEXED_PAIR:
            ss << instr.operand.text << ", XY";
            break;
    }

    return ss.str();
}

std::string Ca45Writer::formatDirective(const AsmIR::Directive& dir) {
    std::stringstream ss;
    ss << "." << dir.name;

    for (const auto& arg : dir.arguments) {
        ss << " " << arg;
    }

    return ss.str();
}
