#include "Merlin64Writer.hpp"
#include <sstream>
#include <iomanip>

std::string Merlin64Writer::write(const AsmIR::Module& module) {
    std::stringstream out;

    // Emit symbols (globals, externs, weak) - Merlin 64 style
    for (const auto& [name, sym] : module.symbols) {
        if (sym.is_extern) {
            out << "EXTERN " << name << "\n";
        } else if (sym.is_global) {
            out << "GLOBAL " << name << "\n";
        }
    }

    if (!module.symbols.empty()) {
        out << "\n";
    }

    // Emit statements, skipping symbol directives
    for (const auto& stmt : module.statements) {
        switch (stmt.type) {
            case AsmIR::Statement::Type::LABEL:
                out << stmt.label << "\n";
                break;

            case AsmIR::Statement::Type::INSTRUCTION: {
                out << "\t" << formatInstruction(stmt.instr) << "\n";
                break;
            }

            case AsmIR::Statement::Type::DIRECTIVE: {
                // Skip symbol directives as they're output from module.symbols
                if (stmt.dir.type == AsmIR::DirectiveType::GLOBAL ||
                    stmt.dir.type == AsmIR::DirectiveType::EXTERN ||
                    stmt.dir.type == AsmIR::DirectiveType::WEAK) {
                    continue;
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

std::string Merlin64Writer::formatInstruction(const AsmIR::Instruction& instr) {
    std::stringstream ss;
    ss << instr.mnemonic;

    if (instr.mode != AsmIR::AddressingMode::IMPLIED) {
        ss << " " << formatAddressingMode(instr);
    }

    return ss.str();
}

std::string Merlin64Writer::formatAddressingMode(const AsmIR::Instruction& instr) {
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
        case AsmIR::AddressingMode::ABSOLUTE:
            if (!instr.operand.text.empty()) {
                ss << instr.operand.text;
            } else {
                ss << "$" << std::hex << std::setw(4) << std::setfill('0') << instr.operand.value;
            }
            break;
        case AsmIR::AddressingMode::ZERO_PAGE_X:
        case AsmIR::AddressingMode::ABSOLUTE_X:
            if (!instr.operand.text.empty()) {
                ss << instr.operand.text << ",X";
            } else {
                ss << "$" << std::hex << std::setw(4) << std::setfill('0') << instr.operand.value << ",X";
            }
            break;
        case AsmIR::AddressingMode::ZERO_PAGE_Y:
        case AsmIR::AddressingMode::ABSOLUTE_Y:
            if (!instr.operand.text.empty()) {
                ss << instr.operand.text << ",Y";
            } else {
                ss << "$" << std::hex << std::setw(4) << std::setfill('0') << instr.operand.value << ",Y";
            }
            break;
        case AsmIR::AddressingMode::INDIRECT:
            ss << "(" << instr.operand.text << ")";
            break;
        case AsmIR::AddressingMode::INDIRECT_X:
            ss << "(" << instr.operand.text << ",X)";
            break;
        case AsmIR::AddressingMode::INDIRECT_Y:
            ss << "(" << instr.operand.text << "),Y";
            break;
        default:
            ss << instr.operand.text;
            break;
    }

    return ss.str();
}

std::string Merlin64Writer::formatDirective(const AsmIR::Directive& dir) {
    std::stringstream ss;
    ss << formatDirectiveKeyword(dir.name);

    for (const auto& arg : dir.arguments) {
        ss << " " << arg;
    }

    return ss.str();
}

std::string Merlin64Writer::formatDirectiveKeyword(const std::string& irName) {
    // Map IR directive names to Merlin 64 keywords
    if (irName == "byte") return "BYTE";
    if (irName == "word") return "WORD";
    if (irName == "org") return "ORG";
    return irName;
}
