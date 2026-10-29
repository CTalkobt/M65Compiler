#include "AsmWriter.hpp"
#include "AssemblerLexer.hpp"
#include <cctype>
#include <set>
#include <cstdlib>
#include <cstdio>

WriterRegistry& WriterRegistry::getInstance() {
    static WriterRegistry instance;
    return instance;
}

void WriterRegistry::registerWriter(const std::string& format, WriterFactory factory) {
    writers_[format] = factory;
}

std::unique_ptr<AsmWriter> WriterRegistry::createWriter(const std::string& format) {
    auto it = writers_.find(format);
    if (it != writers_.end()) {
        return std::unique_ptr<AsmWriter>(it->second());
    }
    return nullptr;
}

std::vector<std::string> WriterRegistry::getAvailableWriters() const {
    std::vector<std::string> result;
    for (const auto& [name, _] : writers_) {
        result.push_back(name);
    }
    return result;
}

namespace {
// A data value written as a negative literal (.byte -1) as the unsigned value
// ca45 stores ($FF): ca65 rejects it out of the type's range (#223)
std::string negativeAsUnsigned(const std::string& a, int bytes) {
    size_t i = a.find_first_not_of(" \t");
    if (i == std::string::npos || a[i] != '-') return a;
    std::string num = a.substr(i + 1);
    while (!num.empty() && (num.back() == ' ' || num.back() == '\t')) num.pop_back();
    if (num.empty()) return a;
    long v;
    char* end = nullptr;
    if (num[0] == '$') v = std::strtol(num.c_str() + 1, &end, 16);
    else if (num[0] == '%') v = std::strtol(num.c_str() + 1, &end, 2);
    else v = std::strtol(num.c_str(), &end, 10);
    if (!end || *end || end == num.c_str() || v <= 0 || v > (1L << (8 * bytes - 1))) return a;
    char buf[16];
    std::snprintf(buf, sizeof buf, bytes == 1 ? "$%02lX" : "$%04lX", ((1L << (8 * bytes)) - v));
    return buf;
}

// 'c' in an expression (outside "strings") as the number ca45 gives it
std::string charsAsNumbers(const std::string& e) {
    std::string out;
    bool dq = false;
    for (size_t i = 0; i < e.size(); i++) {
        char c = e[i];
        if (c == '"') dq = !dq;
        if (!dq && c == '\'' && i + 2 < e.size() && e[i + 2] == '\'' && e[i + 1] != '\\') {
            auto b = asmstr::encode(std::string(1, e[i + 1]), "text");
            char buf[8];
            std::snprintf(buf, sizeof(buf), "$%02X", b.empty() ? 0 : b[0]);
            out += buf;
            i += 2;
            continue;
        }
        out += c;
    }
    return out;
}
}  // namespace

bool AsmWriter::isQInstruction(const std::string& mnemonic) {
    static const char* const q[] = {"ldq", "stq", "adcq", "sbcq", "andq", "orq", "eorq", "aslq", "lsrq",
                                    "rolq", "rorq", "asrq", "inq", "deq", "cpq", "bitq"};
    std::string m;
    for (char c : mnemonic) m += (char)std::tolower((unsigned char)c);
    for (const char* x : q) if (m == x) return true;
    return false;
}

std::string AsmWriter::flatOperand(const std::string& operand) {
    size_t a = operand.find_first_not_of(" \t");
    size_t b = operand.rfind(']');
    if (a == std::string::npos || operand[a] != '[' || b == std::string::npos || b < a) return "";
    std::string rest = operand.substr(b + 1);
    std::string r;
    for (char c : rest) if (c != ' ' && c != '\t') r += (char)std::tolower((unsigned char)c);
    return r == ",z" ? operand.substr(a + 1, b - a - 1) : "";
}

std::string AsmWriter::literalBytes(const AsmIR::Directive& dir) {
    auto num = [](std::string a, long& v) {
        size_t b = a.find_first_not_of(" \t"), e = a.find_last_not_of(" \t");
        if (b == std::string::npos) return false;
        a = a.substr(b, e - b + 1);
        char* end = nullptr;
        if (a[0] == '$') v = std::strtol(a.c_str() + 1, &end, 16);
        else if (a[0] == '%') v = std::strtol(a.c_str() + 1, &end, 2);
        else v = std::strtol(a.c_str(), &end, 10);
        return end && !*end && end != a.c_str();
    };
    std::vector<long> bytes;
    std::string kind = dataKind(dir.name);
    if (kind == "dword") {
        for (const auto& a : dir.arguments) {
            long v;
            if (!num(a, v)) return "";
            for (int k = 0; k < 4; k++) bytes.push_back((v >> (8 * k)) & 0xFF);
        }
    } else if (kind == "fill" && !dir.arguments.empty()) {
        long n, f = 0;
        if (!num(dir.arguments[0], n) || n < 0 || n > 256 || (dir.arguments.size() > 1 && !num(dir.arguments[1], f)))
            return "";
        for (long k = 0; k < n; k++) bytes.push_back(f & 0xFF);
    } else {
        return "";
    }
    std::string r;
    for (size_t k = 0; k < bytes.size(); k++) {
        char b[8];
        std::snprintf(b, sizeof b, "%s$%02lX", k ? ", " : "", bytes[k]);
        r += b;
    }
    return r;
}

std::string AsmWriter::cpuFamily(const std::string& arg) {
    std::string a;
    for (char c : arg) if (c != '"' && c != '_' && c != ' ') a += (char)std::tolower((unsigned char)c);
    if (a == "45gs02" || a == "m65" || a == "4510" || a == "65ce02" || a == "mega65") return "45gs02";
    if (a == "6502" || a == "6510" || a == "nmos6502") return "6502";
    if (a == "65c02" || a == "65sc02") return "65c02";
    return "";
}

std::string AsmWriter::dataKind(const std::string& name) {
    std::string n;
    for (char c : name) n += (char)std::tolower((unsigned char)c);
    if (n == "long" || n == "dword" || n == "dd") return "dword";
    if (n == "res" || n == "fill" || n == "space" || n == "ds") return "fill";
    if (n == "balign" || n == "align") return "align";
    if (n == "db") return "byte";
    if (n == "dw") return "word";
    return n;
}

AsmIR::Module AsmWriter::localsAsGlobals(const AsmIR::Module& module) {
    AsmIR::Module out = module;
    // the enclosing (non-@) label of each statement
    std::vector<std::string> scope(out.statements.size());
    std::string cur = "_top";
    for (size_t i = 0; i < out.statements.size(); i++) {
        const auto& st = out.statements[i];
        if (st.type == AsmIR::Statement::Type::LABEL && !st.label.empty() && st.label[0] != '@') cur = st.label;
        scope[i] = cur;
    }
    // (only names defined as labels here: ca45's .local / .var names such as
    // @_l_a are frame offsets and variables, not labels)
    std::set<std::string> labels;
    for (const auto& st : out.statements)
        if (st.type == AsmIR::Statement::Type::LABEL && !st.label.empty() && st.label[0] == '@') labels.insert(st.label);
    // @name as a whole word outside string literals
    auto rename = [&labels](const std::string& text, const std::string& sc) {
        std::string r;
        char quote = 0;
        for (size_t i = 0; i < text.size(); i++) {
            char c = text[i];
            if (quote) { r += c; if (c == quote) quote = 0; continue; }
            if (c == '"' || c == '\'') { quote = c; r += c; continue; }
            bool start = i == 0 || !(std::isalnum((unsigned char)text[i - 1]) || text[i - 1] == '_');
            if (c == '@' && start && i + 1 < text.size() &&
                (std::isalpha((unsigned char)text[i + 1]) || text[i + 1] == '_')) {
                size_t e = i + 1;
                while (e < text.size() && (std::isalnum((unsigned char)text[e]) || text[e] == '_')) e++;
                if (labels.count(text.substr(i, e - i))) {
                    r += sc + "__" + text.substr(i + 1, e - i - 1);
                    i = e - 1;
                    continue;
                }
            }
            r += c;
        }
        return r;
    };
    for (size_t i = 0; i < out.statements.size(); i++) {
        auto& st = out.statements[i];
        if (st.type == AsmIR::Statement::Type::LABEL && !st.label.empty() && st.label[0] == '@')
            st.label = scope[i] + "__" + st.label.substr(1);
        if (st.type == AsmIR::Statement::Type::INSTRUCTION) st.instr.operand.text = rename(st.instr.operand.text, scope[i]);
        if (st.type == AsmIR::Statement::Type::DIRECTIVE)
            for (auto& a : st.dir.arguments) a = rename(a, scope[i]);
    }
    return out;
}

AsmIR::Module AsmWriter::textAsBytes(const AsmIR::Module& module) {
    AsmIR::Module out = module;
    for (auto& st : out.statements) {
        if (st.type == AsmIR::Statement::Type::INSTRUCTION && st.instr.mode != AsmIR::AddressingMode::RAW)
            st.instr.operand.text = charsAsNumbers(st.instr.operand.text);
        if (st.type == AsmIR::Statement::Type::DIRECTIVE && st.dir.name != "text" &&
            st.dir.name != "screencode" && st.dir.name != "ascii") {
            std::string kind = dataKind(st.dir.name);
            for (auto& a : st.dir.arguments) {
                a = charsAsNumbers(a);
                if (kind == "byte") a = negativeAsUnsigned(a, 1);
                if (kind == "word") a = negativeAsUnsigned(a, 2);
            }
        }
        if (st.type != AsmIR::Statement::Type::DIRECTIVE || st.dir.arguments.size() != 1) continue;
        const std::string& name = st.dir.name;
        if (name != "text" && name != "screencode" && name != "ascii") continue;
        std::vector<AssemblerToken> toks;
        try {
            toks = AssemblerLexer(st.dir.arguments[0]).tokenize();
        } catch (...) {
            continue;
        }
        if (toks.empty() || toks[0].type != AssemblerTokenType::STRING_LITERAL) continue;
        std::vector<std::string> bytes;
        for (uint8_t b : asmstr::encode(toks[0].value, name)) {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "$%02X", b);
            bytes.push_back(buf);
        }
        std::string was = "." + name + " " + st.dir.arguments[0];
        st.comment = st.comment.empty() ? " " + was : " " + was + ";" + st.comment;
        st.dir.type = AsmIR::DirectiveType::BYTE_DATA;
        st.dir.name = "byte";
        st.dir.arguments = bytes;
    }
    return out;
}
