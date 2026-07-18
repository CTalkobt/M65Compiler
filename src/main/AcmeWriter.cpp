#include "AcmeWriter.hpp"
#include <sstream>
#include <iomanip>

std::string AcmeWriter::write(const AsmIR::Module& module) {
    std::stringstream out;

    // ACME doesn't have global/extern directives in the traditional sense
    // Just emit statements directly

    for (const auto& stmt : module.statements) {
        switch (stmt.type) {
            case AsmIR::Statement::Type::LABEL:
                out << stmt.label << ":\n";
                break;

            case AsmIR::Statement::Type::INSTRUCTION: {
                out << "\t" << formatInstruction(stmt.instr) << "\n";
                break;
            }

            case AsmIR::Statement::Type::DIRECTIVE: {
                out << formatDirective(stmt.dir) << "\n";
                break;
            }

            case AsmIR::Statement::Type::COMMENT:
                out << ";" << stmt.comment << "\n";
                break;

            case AsmIR::Statement::Type::BLANK:
                out << "\n";
                break;
        }
    }

    return out.str();
}

std::string AcmeWriter::formatInstruction(const AsmIR::Instruction& instr) {
    std::stringstream ss;
    ss << instr.mnemonic;

    if (instr.mode != AsmIR::AddressingMode::IMPLIED) {
        ss << " " << formatAddressingMode(instr);
    }

    return ss.str();
}

std::string AcmeWriter::formatDirective(const AsmIR::Directive& dir) {
    std::stringstream ss;

    if (dir.type == AsmIR::DirectiveType::ORG) {
        ss << "*=";
        if (!dir.arguments.empty()) {
            ss << " " << dir.arguments[0];
        }
    } else {
        // ACME uses ! prefix for pseudo-ops
        ss << "!" << formatPseudoDirective(dir.name);
        for (const auto& arg : dir.arguments) {
            ss << " " << arg;
        }
    }

    return ss.str();
}

std::string AcmeWriter::formatAddressingMode(const AsmIR::Instruction& instr) {
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
                ss << instr.operand.text << ",x";
            } else {
                ss << "$" << std::hex << std::setw(4) << std::setfill('0') << instr.operand.value << ",x";
            }
            break;
        case AsmIR::AddressingMode::ZERO_PAGE_Y:
        case AsmIR::AddressingMode::ABSOLUTE_Y:
            if (!instr.operand.text.empty()) {
                ss << instr.operand.text << ",y";
            } else {
                ss << "$" << std::hex << std::setw(4) << std::setfill('0') << instr.operand.value << ",y";
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
        default:
            ss << instr.operand.text;
            break;
    }

    return ss.str();
}

std::string AcmeWriter::formatPseudoDirective(const std::string& irName) {
    if (irName == "byte") return "byte";
    if (irName == "word") return "word";
    if (irName == "long") return "long";
    if (irName == "align") return "align";
    return irName;
}
