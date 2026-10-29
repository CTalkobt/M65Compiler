#include "AcmeWriter.hpp"
#include <cstdlib>
#include "MacroUtils.hpp"
#include <sstream>
#include <iomanip>

std::string AcmeWriter::write(const AsmIR::Module& input) {
    const AsmIR::Module module = textAsBytes(input);
    std::stringstream out;
    // (#223: ca45 is a 45GS02 assembler: the target told the CPU, unless
    // the source names one; else it rejects ldz, (zp),z, map, ...)
    bool cpuStated = false;
    for (const auto& st : module.statements)
        if (st.type == AsmIR::Statement::Type::DIRECTIVE && st.dir.type == AsmIR::DirectiveType::CPU) cpuStated = true;
    if (!cpuStated && (module.cpu == "45GS02" || module.cpu == "45gs02")) out << "!cpu m65\n";

    // Emit macros first
    if (!module.macros.empty()) {
        for (const auto& [name, macro] : module.macros) {
            out << MacroUtils::formatACMEMacro(macro);
        }
        out << "\n";
    }

    // ACME assembles one program, without a linker: imports and exports
    // become comments (an imported symbol must be defined before assembling)
    bool any = false;
    for (const auto& [name, sym] : module.symbols) {
        if (sym.is_extern) { out << "; extern: " << name << " (define it before assembling)\n"; any = true; }
        else if (sym.is_global) { out << "; global: " << name << "\n"; any = true; }
        else if (sym.is_weak) { out << "; weak: " << name << "\n"; any = true; }
    }
    if (any) out << "\n";

    for (const auto& stmt : module.statements) {
        switch (stmt.type) {
            case AsmIR::Statement::Type::LABEL:
                out << stmt.label << ":\n";
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
                out << formatDirective(stmt.dir) << (stmt.comment.empty() ? "" : " ;" + stmt.comment) << "\n";
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
    // (ACME 0.97's m65 has the 32-bit instructions and [zp],z, but not both together, #223)
    if (isQInstruction(instr.mnemonic) && !flatOperand(instr.operand.text).empty())
        addWarning("'" + instr.mnemonic + " " + instr.operand.text +
                   "': ACME 0.97 has no 32-bit flat-memory form: kept as written");
    if (instr.mode == AsmIR::AddressingMode::RAW)
        return instr.operand.text.empty() ? instr.mnemonic : instr.mnemonic + " " + instr.operand.text;
    ss << instr.mnemonic;

    if (instr.mode != AsmIR::AddressingMode::IMPLIED) {
        ss << " " << formatAddressingMode(instr);
    }

    return ss.str();
}

std::string AcmeWriter::formatDirective(const AsmIR::Directive& dir) {
    std::stringstream ss;
    // the 45GS02 is "m65" there
    if (dir.type == AsmIR::DirectiveType::CPU && !dir.arguments.empty() &&
        (dir.arguments[0] == "45GS02" || dir.arguments[0] == "45gs02"))
        return std::string("!") + "cpu m65";
    if (dir.type == AsmIR::DirectiveType::CONST && dir.arguments.size() == 2)
        return dir.arguments[0] + " = " + dir.arguments[1];
    auto args = [&](size_t from) {   // the arguments from 'from', comma-separated
        std::string r;
        for (size_t i = from; i < dir.arguments.size(); i++) r += (i > from ? ", " : "") + dir.arguments[i];
        return r;
    };
    // (#223: the data directives in ACME's spelling; !align takes a mask:
    // n - 1 for a power of two n, and its fill byte, else ACME pads with NOP $EA)
    const std::string kind = dataKind(dir.name);
    const size_t n = dir.arguments.size();
    if (kind == "dword") return "!32 " + args(0);
    if (kind == "fill" && n >= 1) return "!fill " + args(0);
    if (kind == "align" && n >= 1) {
        // (ca45's alignment is a literal: the mask as a number)
        const std::string& a = dir.arguments[0];
        char* end = nullptr;
        long v = a[0] == '$' ? std::strtol(a.c_str() + 1, &end, 16) : std::strtol(a.c_str(), &end, 0);
        std::string mask = (end && !*end && v > 0) ? std::to_string(v - 1) : "(" + a + ")-1";
        return "!align " + mask + ", 0, " + (n > 1 ? args(1) : "0");
    }
    if (kind == "fillto" && n >= 1) return "!fill (" + dir.arguments[0] + ")-*" + (n > 1 ? ", " + args(1) : "");
    if (kind == "incbin") return "!binary " + args(0);
    if (kind == "float") return noEquivalent(dir, "ACME", "!float " + args(0));

    if (dir.type == AsmIR::DirectiveType::ORG) {
        ss << "*=";
        if (!dir.arguments.empty()) {
            ss << " " << dir.arguments[0];
        }
    } else {
        // ACME uses ! prefix for pseudo-ops
        ss << "!" << formatPseudoDirective(dir.name);
        for (size_t i = 0; i < dir.arguments.size(); i++) ss << (i ? ", " : " ") << dir.arguments[i];
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
