#include "BsaWriter.hpp"
#include <cstdio>
#include <cstdlib>
#include <regex>
#include <vector>
#include <cctype>
#include <cstring>
#include <sstream>

namespace {

// a ca45 expression for BSA: ca45's < and > take everything after them, BSA's
// only the next operand: what follows goes in parentheses
std::string bsaExpr(const std::string& in) {
    // ca45's 0x1F / 0b101 are $1F / %101 in BSA (which reads 0x1F as 0, then an error)
    static const std::regex hexRe(R"re(\b0[xX]([0-9A-Fa-f]+)\b)re"), binRe(R"re(\b0[bB]([01]+)\b)re");
    const std::string e = std::regex_replace(std::regex_replace(in, hexRe, "$$$1"), binRe, "%$1");
    size_t p = 0;
    while (p < e.size() && e[p] == ' ') p++;
    std::string pre = e.substr(0, p);
    if (p < e.size() && e[p] == '#') { pre += '#'; p++; }
    // (BSA's > is a shift, not masked to a byte: >$AABBCC is $AABB, an
    // immediate out of range there; ca45's is bits 8-15)
    if (p < e.size() && e[p] == '<' && p + 1 < e.size() && e[p + 1] != '(')
        return pre + "<(" + e.substr(p + 1) + ")";
    if (p < e.size() && e[p] == '>') {
        std::string rest = e.substr(p + 1);
        if (rest.empty() || rest[0] != '(') rest = "(" + rest + ")";
        return pre + ">" + rest + "&255";
    }
    return e;
}

// an operand that is an expression starting with '(': BSA would read it as
// indirect
std::string guard(const std::string& e) { return !e.empty() && e[0] == '(' ? "0+" + e : e; }

std::string join(const std::vector<std::string>& a, size_t from, const std::string& sep = ",") {
    std::string r;
    for (size_t i = from; i < a.size(); i++) r += (i > from ? sep : "") + bsaExpr(a[i]);
    return r;
}

}  // namespace

std::string BsaWriter::write(const AsmIR::Module& input) {
    const AsmIR::Module module = localsAsGlobals(textAsBytes(input));
    std::stringstream out;

    // the CPU (ca45 is a 45GS02 assembler), unless the source names one
    bool cpuStated = false, org = false, code = false;
    for (const auto& st : module.statements) {
        if (st.type == AsmIR::Statement::Type::DIRECTIVE && st.dir.type == AsmIR::DirectiveType::CPU) cpuStated = true;
        if (st.type == AsmIR::Statement::Type::DIRECTIVE && st.dir.type == AsmIR::DirectiveType::ORG && !code) org = true;
        if (st.type == AsmIR::Statement::Type::INSTRUCTION) code = true;
    }
    if (!cpuStated) out << ".CPU 45GS02\n";
    // BSA writes a binary only where the source says: the whole program,
    // from its origin to its end
    out << ".STORE __bsa_start,__bsa_end-__bsa_start,\"asmcvt45.bin\"\n";
    if (!org) {
        addWarning("no origin (the source was linked elsewhere): BSA needs one, add '* = <address>' at the start");
        out << "; * = <address>  (BSA needs an origin here: the source had none)\n";
    }

    // one program without a linker: imports and exports as comments
    bool any = false;
    for (const auto& [name, sym] : module.symbols) {
        if (sym.is_extern) { out << "; extern: " << name << " (define it before assembling)\n"; any = true; }
        else if (sym.is_global) { out << "; global: " << name << "\n"; any = true; }
        else if (sym.is_weak) { out << "; weak: " << name << "\n"; any = true; }
    }
    if (any) out << "\n";

    bool started = false;
    auto start = [&]() { if (!started) { out << "__bsa_start:\n"; started = true; } };
    if (!org) start();
    for (const auto& stmt : module.statements) {
        std::string comment = stmt.comment.empty() ? "" : " ;" + stmt.comment;
        switch (stmt.type) {
            case AsmIR::Statement::Type::LABEL:
                out << stmt.label << ":\n";
                break;
            case AsmIR::Statement::Type::INSTRUCTION:
                out << "   " << formatInstruction(stmt.instr) << comment << "\n";
                break;
            case AsmIR::Statement::Type::DIRECTIVE:
                if (stmt.dir.type == AsmIR::DirectiveType::GLOBAL || stmt.dir.type == AsmIR::DirectiveType::EXTERN ||
                    stmt.dir.type == AsmIR::DirectiveType::WEAK)
                    continue;
                out << formatDirective(stmt.dir) << comment << "\n";
                if (stmt.dir.type == AsmIR::DirectiveType::ORG) start();
                break;
            case AsmIR::Statement::Type::COMMENT:
                out << ";" << stmt.comment << "\n";
                break;
            case AsmIR::Statement::Type::BLANK:
                out << "\n";
                break;
        }
    }
    out << "__bsa_end:\n";
    return out.str();
}

std::string BsaWriter::formatInstruction(const AsmIR::Instruction& in) {
    AsmIR::Instruction instr = in;
    if (forcedAbs(instr.mnemonic)) instr.operand.text = "^" + instr.operand.text;
    const std::string& o = instr.operand.text;
    std::string op;
    switch (instr.mode) {
        case AsmIR::AddressingMode::IMPLIED: break;
        case AsmIR::AddressingMode::IMMEDIATE: op = "#" + bsaExpr(o); break;
        case AsmIR::AddressingMode::ZERO_PAGE:
        case AsmIR::AddressingMode::ABSOLUTE: op = guard(bsaExpr(o)); break;
        case AsmIR::AddressingMode::ZERO_PAGE_X:
        case AsmIR::AddressingMode::ABSOLUTE_X: op = guard(bsaExpr(o)) + ",X"; break;
        case AsmIR::AddressingMode::ZERO_PAGE_Y:
        case AsmIR::AddressingMode::ABSOLUTE_Y: op = guard(bsaExpr(o)) + ",Y"; break;
        case AsmIR::AddressingMode::INDIRECT: op = "(" + o + ")"; break;
        case AsmIR::AddressingMode::INDIRECT_X: op = "(" + o + ",X)"; break;
        case AsmIR::AddressingMode::INDIRECT_Y: op = "(" + o + "),Y"; break;
        case AsmIR::AddressingMode::STACK_RELATIVE: op = "(" + o + ",SP),Y"; break;
        default: op = o; break;   // RAW: (zp),z, [zp],z (as BSA writes them), bbr zp,target, ...
    }
    return op.empty() ? instr.mnemonic : instr.mnemonic + " " + op;
}

std::string BsaWriter::formatDirective(const AsmIR::Directive& dir) {
    if (dir.type == AsmIR::DirectiveType::SEGMENT) {
        if (!segmentWarned_)
            addWarning("segments are not kept (BSA assembles one program in source order): ." + dir.name +
                       " written as a comment");
        segmentWarned_ = true;
        return "; segment: " + dir.name + (dir.arguments.empty() ? "" : " " + dir.arguments[0]);
    }
    if (dir.type == AsmIR::DirectiveType::CPU && !dir.arguments.empty()) {
        std::string fam = cpuFamily(dir.arguments[0]);
        if (fam == "45gs02") return ".CPU 45GS02";
        if (fam == "6502") return ".CPU 6502";
        if (fam == "65c02") return ".CPU 65C02";
    }
    if (dir.type == AsmIR::DirectiveType::CONST && dir.arguments.size() == 2)
        return dir.arguments[0] + " = " + bsaExpr(dir.arguments[1]);
    if (dir.type == AsmIR::DirectiveType::ORG) return "* = " + (dir.arguments.empty() ? "" : bsaExpr(dir.arguments[0]));
    const std::string kind = dataKind(dir.name);
    const size_t n = dir.arguments.size();
    const std::string fill = n > 1 ? bsaExpr(dir.arguments[1]) : "0";
    if (kind == "byte") return "   .BYTE " + join(dir.arguments, 0);
    if (kind == "word") return "   .WORD " + join(dir.arguments, 0);
    if (kind == "dword") {
        // .QUAD is one big-endian literal: the bytes, little-endian as ca45's
        std::string lit = literalBytes(dir);
        if (!lit.empty()) return "   .BYTE " + lit;
        std::string r;
        for (size_t i = 0; i < n; i++) {
            const std::string e = "(" + dir.arguments[i] + ")";
            r += std::string(i ? "," : "") + "<" + e + ",>" + e + ",<(" + e + ">>16),>(" + e + ">>16)";
        }
        return "   .BYTE " + r;
    }
    if (kind == "fill" && n >= 1) return "   .FILL " + bsaExpr(dir.arguments[0]) + " (" + fill + ")";
    if (kind == "align" && n >= 1) {
        const std::string& a = dir.arguments[0];
        char* end = nullptr;
        long v = a[0] == '$' ? std::strtol(a.c_str() + 1, &end, 16) : std::strtol(a.c_str(), &end, 0);
        std::string m = (end && !*end && v > 0) ? std::to_string(v - 1) : "(" + a + ")-1";
        return "   .FILL ((" + m + ")+1-(*&(" + m + ")))&(" + m + ") (" + fill + ")";
    }
    if (kind == "fillto" && n >= 1) return "   .FILL (" + dir.arguments[0] + ")-* (" + fill + ")";
    if (kind == "float") {
        std::string r;
        for (size_t i = 0; i < n; i++) r += std::string(i ? "\n" : "") + "   .REAL " + dir.arguments[i];
        return r;
    }
    std::string written = "   ." + dir.name + (n ? " " + join(dir.arguments, 0, ", ") : "");
    return noEquivalent(dir, "BSA", written);
}
