#include "Ca65Writer.hpp"
#include <cstring>
#include "MacroUtils.hpp"
#include <sstream>
#include <iomanip>

std::string Ca65Writer::write(const AsmIR::Module& input) {
    const AsmIR::Module module = textAsBytes(input);
    std::stringstream out;
    // (#223: ca45 is a 45GS02 assembler: the target told the CPU, unless
    // the source names one; else it rejects ldz, (zp),z, map, ...)
    // (ca45's statements before any segment switch are in its "default"
    // segment, which it writes after code and data; ca65 would take them as
    // CODE: named DEFAULT when the program switches segments, #224)
    {
        bool content = false, switches = false;
        for (const auto& st : module.statements) {
            if (st.type == AsmIR::Statement::Type::DIRECTIVE && st.dir.type == AsmIR::DirectiveType::SEGMENT) { switches = true; break; }
            if (st.type == AsmIR::Statement::Type::INSTRUCTION || st.type == AsmIR::Statement::Type::LABEL ||
                (st.type == AsmIR::Statement::Type::DIRECTIVE && st.dir.type != AsmIR::DirectiveType::CONST &&
                 st.dir.type != AsmIR::DirectiveType::GLOBAL && st.dir.type != AsmIR::DirectiveType::EXTERN &&
                 st.dir.type != AsmIR::DirectiveType::WEAK && st.dir.type != AsmIR::DirectiveType::CPU))
                content = true;
        }
        if (content && switches) defaultSegment_ = true;
    }
    bool cpuStated = false;
    for (const auto& st : module.statements)
        if (st.type == AsmIR::Statement::Type::DIRECTIVE && st.dir.type == AsmIR::DirectiveType::CPU) cpuStated = true;
    if (!cpuStated && (module.cpu == "45GS02" || module.cpu == "45gs02")) out << ".setcpu \"4510\"\n";
    if (defaultSegment_) out << ".segment \"DEFAULT\"\n";

    // Emit imports/exports
    for (const auto& [name, sym] : module.symbols) {
        if (sym.is_extern) {
            out << ".import " << name << "\n";
        } else if (sym.is_global) {
            out << ".export " << name << "\n";
        }
    }

    if (!module.symbols.empty()) {
        out << "\n";
    }

    // (no macro definitions: the readers expand every call in place, #224)

    // Emit statements
    for (const auto& stmt : module.statements) {
        switch (stmt.type) {
            case AsmIR::Statement::Type::LABEL:
                out << stmt.label << ":\n";
                break;

            case AsmIR::Statement::Type::INSTRUCTION: {
                out << "    " << formatInstruction(stmt.instr) << (stmt.comment.empty() ? "" : " ;" + stmt.comment) << "\n";
                break;
            }

            case AsmIR::Statement::Type::DIRECTIVE: {
                // .global/.extern/.weak come from the symbol table
                if (stmt.dir.type == AsmIR::DirectiveType::GLOBAL ||
                    stmt.dir.type == AsmIR::DirectiveType::EXTERN ||
                    stmt.dir.type == AsmIR::DirectiveType::WEAK) {
                    continue;
                }
                // (in a data list ca65's * is the item's own address, ca45's the
                // line's: a label for the line in its place, #223)
                AsmIR::Directive d = stmt.dir;
                std::string kind = dataKind(d.name);
                if (kind == "byte" || kind == "word" || kind == "dword") {
                    std::string pc = "__cvt_pc_" + std::to_string(pcLabels_ + 1);
                    bool used = false;
                    for (auto& a : d.arguments) used = replacePc(a, pc) || used;
                    if (used) {
                        pcLabels_++;
                        out << pc << ":\n";
                    }
                }
                out << formatDirective(d) << (stmt.comment.empty() ? "" : " ;" + stmt.comment) << "\n";
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

std::string Ca65Writer::formatInstruction(const AsmIR::Instruction& in) {
    std::stringstream ss;
    AsmIR::Instruction instr = in;
    if (forcedAbs(instr.mnemonic)) instr.operand.text = "a:" + instr.operand.text;
    // (ca65's 4510 is the 65CE02 with MAP: no 32-bit or flat-memory instructions, #223)
    if (isQInstruction(instr.mnemonic) || !flatOperand(instr.operand.text).empty())
        addWarning("'" + instr.mnemonic + " " + instr.operand.text +
                   "' is a 45GS02 instruction ca65 (4510) does not have: kept as written");
    if (instr.mode == AsmIR::AddressingMode::RAW)
        return instr.operand.text.empty() ? instr.mnemonic : instr.mnemonic + " " + instr.operand.text;
    ss << instr.mnemonic;

    if (instr.mode != AsmIR::AddressingMode::IMPLIED) {
        ss << " " << formatAddressingMode(instr);
    }

    return ss.str();
}

std::string Ca65Writer::formatDirective(const AsmIR::Directive& dir) {
    std::stringstream ss;
    if (dir.type == AsmIR::DirectiveType::CONST && dir.arguments.size() == 2)
        return dir.arguments[0] + " = " + dir.arguments[1];
    auto args = [&](size_t from) {   // the arguments from 'from', comma-separated
        std::string r;
        for (size_t i = from; i < dir.arguments.size(); i++) r += (i > from ? ", " : "") + dir.arguments[i];
        return r;
    };
    // (a CPU by its family, as the target names it, #224)
    if (dir.type == AsmIR::DirectiveType::CPU && !dir.arguments.empty()) {
        std::string fam = cpuFamily(dir.arguments[0]);
        if (fam == "45gs02") return ".setcpu \"4510\"";
        if (fam == "6502") return ".setcpu \"6502\"";
        if (fam == "65c02") return ".setcpu \"65C02\"";
    }
    // (#223: the data directives in ca65's spelling)
    const std::string kind = dataKind(dir.name);
    const size_t n = dir.arguments.size();
    if (kind == "dword") return ".dword " + args(0);
    if (kind == "fill" && n >= 1) return ".res " + args(0);
    if (kind == "align" && n >= 1) return ".align " + args(0);
    if (kind == "fillto" && n >= 1) return ".res (" + dir.arguments[0] + ")-*" + (n > 1 ? ", " + args(1) : "");
    if (kind == "float") return noEquivalent(dir, "ca65", ".float " + args(0));
    ss << "." << formatDirectiveName(dir.name);

    for (size_t i = 0; i < dir.arguments.size(); i++) ss << (i ? ", " : " ") << dir.arguments[i];

    return ss.str();
}

std::string Ca65Writer::formatAddressingMode(const AsmIR::Instruction& instr) {
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
            ss << instr.operand.text;
            break;
        case AsmIR::AddressingMode::FRAME_RELATIVE:
            ss << instr.operand.text;
            break;
        case AsmIR::AddressingMode::INDEXED_PAIR:
            // 45GS02 specific, not applicable to ca65
            ss << instr.operand.text;
            break;
    }

    return ss.str();
}

std::string Ca65Writer::formatDirectiveName(const std::string& irName) {
    if (irName == "byte") return "byte";
    if (irName == "word") return "word";
    if (irName == "long") return "dword";
    if (irName == "extern") return "import";
    if (irName == "global") return "export";
    return irName;
}

bool Ca65Writer::replacePc(std::string& e, const std::string& label) {
    std::string out;
    bool any = false;
    char quote = 0;
    char prev = 0;   // the last non-blank character outside strings (0: the start)
    for (size_t i = 0; i < e.size(); i++) {
        char c = e[i];
        if (quote) { out += c; if (c == quote) quote = 0; continue; }
        if (c == '"' || c == '\'') { quote = c; out += c; prev = c; continue; }
        // (an operand position: the start, or after an operator or '(')
        if (c == '*' && (prev == 0 || std::strchr("(+-*/&|^<>=!,~", prev))) {
            out += label;
            any = true;
            prev = 'x';
            continue;
        }
        out += c;
        if (c != ' ' && c != '\t') prev = c;
    }
    if (any) e = out;
    return any;
}
