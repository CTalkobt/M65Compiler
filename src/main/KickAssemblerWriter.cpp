#include "KickAssemblerWriter.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

std::string KickAssemblerWriter::write(const AsmIR::Module& module) {
    std::stringstream out;

    // KickAssembler assembles one program, without a linker: imports and
    // exports become comments (an imported symbol must be defined first)
    bool any = false;
    for (const auto& [name, sym] : module.symbols) {
        if (sym.is_extern) { out << "// extern: " << name << " (define it before assembling)\n"; any = true; }
        else if (sym.is_global) { out << "// global: " << name << "\n"; any = true; }
        else if (sym.is_weak) { out << "// weak: " << name << "\n"; any = true; }
    }
    if (any) out << "\n";

    // Emit statements
    for (const auto& stmt : module.statements) {
        switch (stmt.type) {
            case AsmIR::Statement::Type::LABEL:
                out << stmt.label << ":\n";
                break;

            case AsmIR::Statement::Type::INSTRUCTION: {
                out << "    " << formatInstruction(stmt.instr) << (stmt.comment.empty() ? "" : " //" + stmt.comment) << "\n";
                break;
            }

            case AsmIR::Statement::Type::DIRECTIVE: {
                // .global/.extern/.weak come from the symbol table
                if (stmt.dir.type == AsmIR::DirectiveType::GLOBAL ||
                    stmt.dir.type == AsmIR::DirectiveType::EXTERN ||
                    stmt.dir.type == AsmIR::DirectiveType::WEAK) {
                    continue;
                }
                out << formatDirective(stmt.dir) << (stmt.comment.empty() ? "" : " //" + stmt.comment) << "\n";
                break;
            }

            case AsmIR::Statement::Type::COMMENT:
                out << "//" << stmt.comment << "\n";
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
    if (instr.mode == AsmIR::AddressingMode::RAW)
        return instr.operand.text.empty() ? instr.mnemonic : instr.mnemonic + " " + instr.operand.text;
    ss << instr.mnemonic;

    if (instr.mode != AsmIR::AddressingMode::IMPLIED) {
        ss << " " << formatAddressingMode(instr);
    }

    return ss.str();
}

std::string KickAssemblerWriter::formatDirective(const AsmIR::Directive& dir) {
    std::stringstream ss;
    // the 45GS02 is "_45gs02" there
    if (dir.type == AsmIR::DirectiveType::CPU && !dir.arguments.empty() &&
        (dir.arguments[0] == "45GS02" || dir.arguments[0] == "45gs02"))
        return std::string(".") + "cpu _45gs02";
    if (dir.type == AsmIR::DirectiveType::CONST && dir.arguments.size() == 2)
        return ".const " + dir.arguments[0] + " = " + dir.arguments[1];

    // KickAssembler: *= for the origin, otherwise .name (.byte, .word, ...)
    if (dir.type == AsmIR::DirectiveType::ORG) {
        ss << "*=";
        if (!dir.arguments.empty()) ss << " " << dir.arguments[0];
        return ss.str();
    }
    ss << "." << dir.name;

    for (size_t i = 0; i < dir.arguments.size(); i++) ss << (i ? ", " : " ") << dir.arguments[i];

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
        case AsmIR::AddressingMode::RAW:
            ss << instr.operand.text;
            break;
    }

    return ss.str();
}

std::string KickAssemblerWriter::formatImmediateValue(uint32_t value, int width) {
    std::stringstream ss;
    ss << "#$" << std::hex << std::setw(width / 4) << std::setfill('0') << value;
    return ss.str();
}
