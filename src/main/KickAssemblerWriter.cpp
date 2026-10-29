#include "KickAssemblerWriter.hpp"
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <algorithm>

std::string KickAssemblerWriter::write(const AsmIR::Module& input) {
    // (@local labels: KickAssembler's local labels scope by braces, #223)
    const AsmIR::Module module = localsAsGlobals(textAsBytes(input));
    std::stringstream out;
    // (#223: ca45 is a 45GS02 assembler: the target told the CPU, unless
    // the source names one; else it rejects ldz, (zp),z, map, ...)
    bool cpuStated = false;
    for (const auto& st : module.statements)
        if (st.type == AsmIR::Statement::Type::DIRECTIVE && st.dir.type == AsmIR::DirectiveType::CPU) cpuStated = true;
    if (!cpuStated && (module.cpu == "45GS02" || module.cpu == "45gs02")) out << ".cpu _45gs02\n";

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
    // (the 45GS02's [zp],z is ((zp)),z in KickAssembler, #223)
    std::string flat = flatOperand(instr.operand.text);
    if (instr.mode == AsmIR::AddressingMode::RAW && !flat.empty()) return instr.mnemonic + " ((" + flat + ")),z";
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
    // (one program in source order, no segments like ca65's: a comment, #224)
    if (dir.type == AsmIR::DirectiveType::SEGMENT) {
        if (!segmentWarned_)
            addWarning("segments are not kept (the target assembles one program in source order): ." + dir.name +
                       " written as a comment");
        segmentWarned_ = true;
        return "// segment: " + dir.name + (dir.arguments.empty() ? "" : " " + dir.arguments[0]);
    }
    // (a CPU by its family, as the target names it, #224)
    if (dir.type == AsmIR::DirectiveType::CPU && !dir.arguments.empty()) {
        std::string fam = cpuFamily(dir.arguments[0]);
        if (fam == "45gs02") return ".cpu _45gs02";
        if (fam == "6502") return ".cpu _6502";
        if (fam == "65c02") return ".cpu _65c02";
    }
    if (dir.type == AsmIR::DirectiveType::CONST && dir.arguments.size() == 2)
        return ".const " + dir.arguments[0] + " = " + dir.arguments[1];

    // (#223: the data directives in KickAssembler's spelling; its .fill
    // needs the value)
    auto args = [&](size_t from) {   // the arguments from 'from', comma-separated
        std::string r;
        for (size_t i = from; i < dir.arguments.size(); i++) r += (i > from ? ", " : "") + dir.arguments[i];
        return r;
    };
    const std::string kind = dataKind(dir.name);
    const size_t n = dir.arguments.size();
    if (kind == "dword") return ".dword " + args(0);
    if (kind == "fill" && n >= 1) return ".fill " + dir.arguments[0] + ", " + (n > 1 ? args(1) : "0");
    if (kind == "align" && n == 1) return ".align " + dir.arguments[0];
    // (.align takes no fill byte: the padding as a .fill, n a power of two)
    if (kind == "align" && n > 1) {
        long a = std::strtol(dir.arguments[0].c_str(), nullptr, 0);
        if (dir.arguments[0][0] == '$') a = std::strtol(dir.arguments[0].c_str() + 1, nullptr, 16);
        if (a <= 0 || (a & (a - 1)))
            return noEquivalent(dir, "KickAssembler (.align takes no fill byte)", ".align " + dir.arguments[0]);
        std::string m = std::to_string(a - 1);
        // ((*): ca45 reads "* &" as 0, so the round trip needs the parentheses)
        return ".fill (" + std::to_string(a) + "-((*)&" + m + "))&" + m + ", " + args(1);
    }
    if (kind == "fillto" && n >= 1) return ".fill (" + dir.arguments[0] + ")-*, " + (n > 1 ? args(1) : "0");
    if (kind == "incbin") return ".import binary " + args(0);
    if (kind == "float") return noEquivalent(dir, "KickAssembler", ".float " + args(0));
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
