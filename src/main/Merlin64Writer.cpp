#include "Merlin64Writer.hpp"
#include "MacroUtils.hpp"
#include <sstream>
#include <iomanip>

std::string Merlin64Writer::write(const AsmIR::Module& input) {
    // (@local labels as globals: the scoping of this dialect's own is not known here, #223)
    const AsmIR::Module module = localsAsGlobals(textAsBytes(input));
    // Merlin64 doesn't support macros, so expand them
    AsmIR::Module expandedModule = module;
    MacroUtils::processAndExpandMacros(expandedModule, "merlin64", true);

    std::stringstream out;

    // Emit symbols (globals, externs, weak) - Merlin 64 style
    for (const auto& [name, sym] : expandedModule.symbols) {
        if (sym.is_extern) {
            out << "\tEXT " << name << "\n";
        } else if (sym.is_global) {
            out << "\tENT " << name << "\n";
        }
    }

    if (!expandedModule.symbols.empty()) {
        out << "\n";
    }

    // Emit statements, skipping symbol directives
    for (const auto& stmt : expandedModule.statements) {
        switch (stmt.type) {
            case AsmIR::Statement::Type::LABEL:
                out << stmt.label << "\n";
                break;

            case AsmIR::Statement::Type::INSTRUCTION: {
                out << "\t" << formatInstruction(stmt.instr) << (stmt.comment.empty() ? "" : " ;" + stmt.comment) << "\n";
                break;
            }

            case AsmIR::Statement::Type::DIRECTIVE: {
                // .global/.extern/.weak come from the symbol table
                if (stmt.dir.type == AsmIR::DirectiveType::GLOBAL ||
                    stmt.dir.type == AsmIR::DirectiveType::EXTERN ||
                    stmt.dir.type == AsmIR::DirectiveType::WEAK) {
                    continue;
                }
                // Skip symbol directives as they're output from module.symbols
                if (stmt.dir.type == AsmIR::DirectiveType::GLOBAL ||
                    stmt.dir.type == AsmIR::DirectiveType::EXTERN ||
                    stmt.dir.type == AsmIR::DirectiveType::WEAK) {
                    continue;
                }

                out << (stmt.dir.type == AsmIR::DirectiveType::CONST ? "" : "\t") << formatDirective(stmt.dir)
                    << (stmt.comment.empty() ? "" : " ;" + stmt.comment) << "\n";
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
    if (instr.mode == AsmIR::AddressingMode::RAW)
        return instr.operand.text.empty() ? instr.mnemonic : instr.mnemonic + " " + instr.operand.text;
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
    if (dir.type == AsmIR::DirectiveType::CONST && dir.arguments.size() == 2)
        return dir.arguments[0] + " = " + dir.arguments[1];
    auto args = [&](size_t from) {   // the arguments from 'from', comma-separated
        std::string r;
        for (size_t i = from; i < dir.arguments.size(); i++) r += (i > from ? ", " : "") + dir.arguments[i];
        return r;
    };
    // (#223: data directives this writer has no checked form for are written
    // as ca45 has them, with a warning)
    const std::string kind = dataKind(dir.name);
    if (kind == "dword") return noEquivalent(dir, "Merlin 64", ".dword " + args(0));
    if (kind == "fill" || kind == "align" || kind == "fillto" || kind == "incbin" || kind == "float")
        return noEquivalent(dir, "Merlin 64", "." + dir.name + (dir.arguments.empty() ? "" : " " + args(0)));
    ss << formatDirectiveKeyword(dir.name);

    for (size_t i = 0; i < dir.arguments.size(); i++) ss << (i ? ", " : " ") << dir.arguments[i];

    return ss.str();
}

std::string Merlin64Writer::formatDirectiveKeyword(const std::string& irName) {
    // the keywords of the IR directives; another (a ca45 one) is written
    // .name, which the reader passes back unchanged
    if (irName == "byte") return "DFB";
    if (irName == "word") return "DW";
    if (irName == "org") return "ORG";
    if (irName == "text") return "ASC";
    if (irName == "res") return "DS";
    if (irName == "global") return "ENT";
    if (irName == "extern") return "EXT";
    return "." + irName;
}
