#include "X65Writer.hpp"
#include "MacroUtils.hpp"
#include <sstream>
#include <iomanip>

std::string X65Writer::write(const AsmIR::Module& module) {
    // X65 doesn't support macros, so expand them
    AsmIR::Module expandedModule = module;
    MacroUtils::processAndExpandMacros(expandedModule, "x65", true);

    std::stringstream out;

    // Emit symbols (.import/.export)
    for (const auto& [name, sym] : expandedModule.symbols) {
        if (sym.is_extern) {
            out << ".import " << name << "\n";
        } else if (sym.is_global) {
            out << ".export " << name << "\n";
        }
    }

    if (!expandedModule.symbols.empty()) {
        out << "\n";
    }

    // Emit statements, skipping symbol directives
    for (const auto& stmt : expandedModule.statements) {
        switch (stmt.type) {
            case AsmIR::Statement::Type::LABEL:
                out << stmt.label << ":\n";
                break;

            case AsmIR::Statement::Type::INSTRUCTION: {
                out << "  " << formatInstruction(stmt.instr) << "\n";
                break;
            }

            case AsmIR::Statement::Type::DIRECTIVE: {
                // Skip symbol directives
                if (stmt.dir.type == AsmIR::DirectiveType::GLOBAL ||
                    stmt.dir.type == AsmIR::DirectiveType::EXTERN) {
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

std::string X65Writer::formatInstruction(const AsmIR::Instruction& instr) {
    std::stringstream ss;
    ss << instr.mnemonic;

    if (instr.mode != AsmIR::AddressingMode::IMPLIED) {
        ss << " " << formatAddressingMode(instr);
    }

    return ss.str();
}

std::string X65Writer::formatAddressingMode(const AsmIR::Instruction& instr) {
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
                ss << instr.operand.text << ", X";
            } else {
                ss << "$" << std::hex << std::setw(4) << std::setfill('0') << instr.operand.value << ", X";
            }
            break;
        case AsmIR::AddressingMode::ZERO_PAGE_Y:
        case AsmIR::AddressingMode::ABSOLUTE_Y:
            if (!instr.operand.text.empty()) {
                ss << instr.operand.text << ", Y";
            } else {
                ss << "$" << std::hex << std::setw(4) << std::setfill('0') << instr.operand.value << ", Y";
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
        default:
            ss << instr.operand.text;
            break;
    }

    return ss.str();
}

std::string X65Writer::formatDirective(const AsmIR::Directive& dir) {
    std::stringstream ss;
    ss << "." << dir.name;

    for (const auto& arg : dir.arguments) {
        ss << " " << arg;
    }

    return ss.str();
}
