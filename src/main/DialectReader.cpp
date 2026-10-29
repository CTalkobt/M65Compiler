#include "DialectReader.hpp"
#include "MacroUtils.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <regex>
#include <sstream>

namespace {

std::string trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r");
    return s.substr(b, e - b + 1);
}

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

// The 6502, 65C02 and 45GS02 mnemonics: a word in column 1 that is one is an
// instruction, not a label, where labels need no colon (ACME)
bool isMnemonic(const std::string& w) {
    static const std::set<std::string> ops = {
        "adc", "and", "asl", "bcc", "bcs", "beq", "bit", "bmi", "bne", "bpl", "brk", "bvc", "bvs", "clc",
        "cld", "cli", "clv", "cmp", "cpx", "cpy", "dec", "dex", "dey", "eor", "inc", "inx", "iny", "jmp",
        "jsr", "lda", "ldx", "ldy", "lsr", "nop", "ora", "pha", "php", "pla", "plp", "rol", "ror", "rti",
        "rts", "sbc", "sec", "sed", "sei", "sta", "stx", "sty", "tax", "tay", "tsx", "txa", "txs", "tya",
        "bra", "phx", "phy", "plx", "ply", "stz", "trb", "tsb", "ina", "dea",
        "asr", "asw", "aug", "bsr", "cle", "cpz", "dew", "dez", "eom", "inw", "inz", "ldz", "map", "neg",
        "phw", "phz", "plz", "row", "rtn", "see", "tab", "taz", "tba", "tsy", "tys", "tza",
        "adcq", "andq", "aslq", "asrq", "bitq", "cmpq", "deq", "eorq", "inq", "ldq", "lsrq", "oraq",
        "rolq", "rorq", "sbcq", "stq",
        "lbcc", "lbcs", "lbeq", "lbmi", "lbne", "lbpl", "lbra", "lbvc", "lbvs"};
    return ops.count(lower(w)) > 0;
}

// The code and the comment of a line (';' outside quotes; '//' too)
void splitComment(const std::string& line, bool slash, std::string& code, std::string& comment) {
    bool dq = false;
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (dq && c == '\\') { i++; continue; }
        if (c == '"') dq = !dq;
        else if (!dq && c == '\'' && i + 2 < line.size() && line[i + 2] == '\'') i += 2;  // 'x'
        else if (!dq && (c == ';' || (slash && c == '/' && i + 1 < line.size() && line[i + 1] == '/'))) {
            code = line.substr(0, i);
            comment = line.substr(i + (c == ';' ? 1 : 2));
            return;
        }
    }
    code = line;
    comment.clear();
}

// Arguments separated by commas outside quotes and parentheses
std::vector<std::string> splitArgs(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    int depth = 0;
    bool dq = false;
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if (dq && c == '\\' && i + 1 < s.size()) { cur += c; cur += s[++i]; continue; }
        if (c == '"') dq = !dq;
        else if (!dq && c == '\'' && i + 2 < s.size() && s[i + 2] == '\'') {
            cur += s.substr(i, 3);
            i += 2;
            continue;
        } else if (!dq) {
            if (c == '(' || c == '[') depth++;
            else if ((c == ')' || c == ']') && depth > 0) depth--;
            else if (c == ',' && depth == 0) { out.push_back(trim(cur)); cur.clear(); continue; }
        }
        cur += c;
    }
    if (!trim(cur).empty() || !out.empty()) out.push_back(trim(cur));
    return out;
}

// Whether s is "(...)" with the parentheses matching each other
bool parenthesized(const std::string& s) {
    if (s.size() < 2 || s.front() != '(' || s.back() != ')') return false;
    int depth = 0;
    bool dq = false;
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if (c == '"') dq = !dq;
        if (dq) continue;
        if (c == '(') depth++;
        else if (c == ')' && --depth == 0 && i + 1 < s.size()) return false;
    }
    return depth == 0;
}

// The last comma outside quotes and parentheses (npos if none)
size_t lastTopComma(const std::string& s) {
    size_t at = std::string::npos;
    int depth = 0;
    bool dq = false;
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if (c == '"') dq = !dq;
        if (dq) continue;
        if (c == '\'' && i + 2 < s.size() && s[i + 2] == '\'') { i += 2; continue; }
        if (c == '(' || c == '[') depth++;
        else if ((c == ')' || c == ']') && depth > 0) depth--;
        else if (c == ',' && depth == 0) at = i;
    }
    return at;
}

// An operand's addressing mode from its syntax, and its expression
AsmIR::AddressingMode classify(const std::string& operand, std::string& core) {
    std::string t = trim(operand);
    core = t;
    if (t.empty() || lower(t) == "a") { core.clear(); return AsmIR::AddressingMode::IMPLIED; }
    if (t[0] == '#') { core = trim(t.substr(1)); return AsmIR::AddressingMode::IMMEDIATE; }
    size_t comma = lastTopComma(t);
    if (comma != std::string::npos) {
        std::string base = trim(t.substr(0, comma)), reg = lower(trim(t.substr(comma + 1)));
        if (reg == "x" && !base.empty()) { core = base; return AsmIR::AddressingMode::ABSOLUTE_X; }
        if (reg == "y" && parenthesized(base)) {
            core = trim(base.substr(1, base.size() - 2));
            if (lastTopComma(core) == std::string::npos) return AsmIR::AddressingMode::INDIRECT_Y;
        } else if (reg == "y" && !base.empty()) {
            core = base;
            return AsmIR::AddressingMode::ABSOLUTE_Y;
        }
        core = t;
        return AsmIR::AddressingMode::RAW;
    }
    if (parenthesized(t)) {
        std::string inner = trim(t.substr(1, t.size() - 2));
        size_t c = lastTopComma(inner);
        if (c == std::string::npos) { core = inner; return AsmIR::AddressingMode::INDIRECT; }
        if (lower(trim(inner.substr(c + 1))) == "x") {
            core = trim(inner.substr(0, c));
            return AsmIR::AddressingMode::INDIRECT_X;
        }
        core = t;
        return AsmIR::AddressingMode::RAW;
    }
    if (t[0] == '[') return AsmIR::AddressingMode::RAW;   // 45GS02 [zp],z
    return AsmIR::AddressingMode::ABSOLUTE;
}

AsmIR::DirectiveType directiveType(const std::string& name) {
    if (name == "byte") return AsmIR::DirectiveType::BYTE_DATA;
    if (name == "word") return AsmIR::DirectiveType::WORD_DATA;
    if (name == "long" || name == "dword") return AsmIR::DirectiveType::LONG_DATA;
    if (name == "fill") return AsmIR::DirectiveType::FILL;
    if (name == "align") return AsmIR::DirectiveType::ALIGN;
    if (name == "org") return AsmIR::DirectiveType::ORG;
    if (name == "segment" || name == "code" || name == "data" || name == "bss") return AsmIR::DirectiveType::SEGMENT;
    if (name == "global") return AsmIR::DirectiveType::GLOBAL;
    if (name == "extern") return AsmIR::DirectiveType::EXTERN;
    if (name == "weak") return AsmIR::DirectiveType::WEAK;
    if (name == "const") return AsmIR::DirectiveType::CONST;
    if (name == "cpu") return AsmIR::DirectiveType::CPU;
    if (name == "func_flags") return AsmIR::DirectiveType::FUNCTION;
    if (name == "zp_uses") return AsmIR::DirectiveType::ZP_USES;
    if (name == "zp_clobbers") return AsmIR::DirectiveType::ZP_CLOBBERS;
    if (name == "reg_clobbers") return AsmIR::DirectiveType::REG_CLOBBERS;
    if (name == "flag_clobbers") return AsmIR::DirectiveType::FLAG_CLOBBERS;
    if (name == "loc") return AsmIR::DirectiveType::LOC;
    return AsmIR::DirectiveType::OTHER;
}

// Merlin's HEX 0102FF: the bytes as a list
std::vector<std::string> hexBytes(const std::string& args) {
    std::vector<std::string> out;
    std::string digits;
    for (char c : args)
        if (std::isxdigit((unsigned char)c)) digits += c;
    for (size_t i = 0; i + 1 < digits.size(); i += 2) out.push_back("$" + digits.substr(i, 2));
    return out;
}

// A character in one of the dialects' encodings
uint8_t encodeChar(char c, char enc) {
    uint8_t u = (uint8_t)c;
    switch (enc) {
        case 'p':   // PETSCII, mixed case (ACME !pet, KickAssembler petscii_mixed)
            if (u >= 'a' && u <= 'z') return u - 0x20;
            if (u >= 'A' && u <= 'Z') return u + 0x80;
            return u;
        case 'P':   // PETSCII, upper case (petscii_upper)
            if (u >= 'a' && u <= 'z') return u - 0x20;
            return u;
        case 's':   // screen codes, mixed case (ACME !scr, screencode_mixed)
            if (u >= 'a' && u <= 'z') return u - 0x60;
            if (u == '@') return 0;
            if (u >= '[' && u <= '_') return u - 0x40;
            return u;
        case 'S':   // screen codes, upper case (screencode_upper)
            if (u >= 'a' && u <= 'z') return u - 0x60;
            if (u >= '@' && u <= '_') return u - 0x40;
            return u;
        default:
            return u;
    }
}

// The characters of a "string" argument (\" and \\ escapes); false if not one
bool stringChars(const std::string& a, std::string& chars) {
    if (a.size() < 2 || a.front() != '"' || a.back() != '"') return false;
    chars.clear();
    for (size_t i = 1; i + 1 < a.size(); i++) {
        if (a[i] == '\\' && i + 2 < a.size() && (a[i + 1] == '"' || a[i + 1] == '\\')) i++;
        chars += a[i];
    }
    return true;
}

std::string hexByte(uint8_t b) {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "$%02X", b);
    return buf;
}

// 'c' constants (outside "strings") as numbers in the dialect's encoding
std::string charsAsNumbers(const std::string& e, char enc) {
    std::string out;
    bool dq = false;
    for (size_t i = 0; i < e.size(); i++) {
        char c = e[i];
        if (c == '"') dq = !dq;
        if (!dq && c == '\'' && i + 2 < e.size() && e[i + 2] == '\'') {
            out += hexByte(encodeChar(e[i + 1], enc));
            i += 2;
            continue;
        }
        out += c;
    }
    return out;
}

// The CPU name ca45 uses for a dialect's
std::string cpuName(const std::string& arg) {
    std::string a = lower(trim(arg));
    if (!a.empty() && a.front() == '"' && a.back() == '"') a = a.substr(1, a.size() - 2);
    if (a == "m65" || a == "_45gs02" || a == "45gs02" || a == "4510" || a == "_4510") return "45GS02";
    if (a == "65c02" || a == "_65c02") return "65C02";
    if (a == "6502" || a == "_6502" || a == "6510" || a == "_6502nf") return "6502";
    return trim(arg);
}

struct Reader {
    const DialectSyntax& syn;
    AsmIR::Module& module;
    std::vector<std::string>& warnings;
    int lineNo = 0;
    bool warnedRaw = false;
    char kick = 's';   // KickAssembler's .encoding (screencode_mixed)

    char charEncoding() const { return syn.kickEncoding ? kick : 'r'; }

    void add(AsmIR::Statement st) {
        st.source_line = lineNo;
        module.statements.push_back(std::move(st));
    }
    void warn(const std::string& msg) { warnings.push_back("line " + std::to_string(lineNo) + ": " + msg); }

    void symbol(const std::string& name, AsmIR::DirectiveType type) {
        AsmIR::Symbol& s = module.symbols[name];
        s.name = name;
        if (type == AsmIR::DirectiveType::GLOBAL) s.is_global = true;
        if (type == AsmIR::DirectiveType::EXTERN) s.is_extern = true;
        if (type == AsmIR::DirectiveType::WEAK) s.is_weak = true;
    }

    // A directive by its dialect name; false if the dialect has none so named
    bool directive(const std::string& word, const std::string& args, const std::string& comment, bool prefixed) {
        std::string w = lower(word);
        auto it = syn.directives.find(w);
        if (syn.handled.count(w) && (prefixed || syn.keywords || syn.bare.count(w))) return true;
        if (syn.ignored.count(w) && (prefixed || syn.keywords || syn.bare.count(w))) {
            warn("'" + word + "' has no ca45 equivalent and is left out");
            return true;
        }
        if (!prefixed && (it == syn.directives.end() || (!syn.keywords && !syn.bare.count(w)))) return false;
        if (syn.kickEncoding && w == "encoding") {
            std::string e = lower(trim(args));
            if (e.size() >= 2 && e.front() == '"') e = e.substr(1, e.size() - 2);
            kick = e == "petscii_mixed" ? 'p' : e == "petscii_upper" ? 'P' : e == "screencode_upper" ? 'S'
                 : e == "ascii" ? 'r' : 's';
            return true;
        }
        std::string name = it != syn.directives.end() ? it->second : word;   // passed on as written
        auto enc = syn.strings.find(w);
        if (w != "hex" && (enc != syn.strings.end() || directiveType(name) == AsmIR::DirectiveType::BYTE_DATA)) {
            stringData(splitArgs(args), enc != syn.strings.end() ? (enc->second == 'k' ? kick : enc->second) : charEncoding(),
                       comment);
            return true;
        }
        AsmIR::Statement st;
        st.type = AsmIR::Statement::Type::DIRECTIVE;
        st.dir.name = name;
        st.dir.type = directiveType(name);
        st.comment = comment;
        if (w == "hex") {
            st.dir.arguments = hexBytes(args);
        } else if (st.dir.type == AsmIR::DirectiveType::CONST) {
            // .const name = value (KickAssembler .const/.var/.label)
            static const std::regex constRe(R"(^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.+)$)");
            std::smatch m;
            if (std::regex_match(args, m, constRe)) st.dir.arguments = {m[1].str(), trim(m[2].str())};
            else st.dir.arguments = splitArgs(args);
        } else {
            st.dir.arguments = splitArgs(args);
            for (auto& x : st.dir.arguments) x = charsAsNumbers(x, charEncoding());
        }
        if (st.dir.type == AsmIR::DirectiveType::ORG && !st.dir.arguments.empty() && st.dir.arguments[0][0] == '=')
            st.dir.arguments[0] = trim(st.dir.arguments[0].substr(1));   // .pc = $2000
        if (st.dir.type == AsmIR::DirectiveType::CPU && !st.dir.arguments.empty())
            st.dir.arguments[0] = cpuName(st.dir.arguments[0]);
        if (st.dir.type == AsmIR::DirectiveType::GLOBAL || st.dir.type == AsmIR::DirectiveType::EXTERN ||
            st.dir.type == AsmIR::DirectiveType::WEAK)
            for (auto& a : st.dir.arguments) symbol(a, st.dir.type);
        add(st);
        return true;
    }

    // Data with strings: ASCII strings of printable characters as .ascii,
    // everything else as .byte, in order
    void stringData(const std::vector<std::string>& args, char enc, const std::string& comment) {
        std::vector<std::string> bytes;
        bool first = true;
        auto flushBytes = [&]() {
            if (bytes.empty()) return;
            AsmIR::Statement st;
            st.type = AsmIR::Statement::Type::DIRECTIVE;
            st.dir.type = AsmIR::DirectiveType::BYTE_DATA;
            st.dir.name = "byte";
            st.dir.arguments = bytes;
            if (first) st.comment = comment;
            first = false;
            add(st);
            bytes.clear();
        };
        for (const auto& a : args) {
            std::string chars;
            if (!stringChars(a, chars)) {
                bytes.push_back(charsAsNumbers(a, charEncoding()));
                continue;
            }
            bool plain = enc == 'r' && !chars.empty() &&
                         std::all_of(chars.begin(), chars.end(), [](char c) {
                             return c >= 0x20 && c < 0x7F && c != '"' && c != '\\' && c != '\'';
                         });
            if (!plain) {
                for (char c : chars) bytes.push_back(hexByte(encodeChar(c, enc)));
                continue;
            }
            flushBytes();
            AsmIR::Statement st;
            st.type = AsmIR::Statement::Type::DIRECTIVE;
            st.dir.type = AsmIR::DirectiveType::OTHER;
            st.dir.name = "ascii";
            st.dir.arguments = {"\"" + chars + "\""};
            if (first) st.comment = comment;
            first = false;
            add(st);
        }
        flushBytes();
    }

    // A macro invocation: its expansion read in its place
    bool macroCall(const std::string& name, const std::string& args, int depth) {
        auto it = module.macros.find(name);
        if (it == module.macros.end()) return false;
        if (depth > 16) { warn("macro '" + name + "' nests too deeply"); return true; }
        for (const auto& l : MacroUtils::expandMacro(it->second, splitArgs(args))) line(l, depth + 1);
        return true;
    }

    void line(const std::string& text, int depth = 0) {
        std::string code, comment;
        if (syn.starComments && !text.empty() && text[0] == '*') {
            code.clear();
            comment = text.substr(1);
        } else {
            splitComment(text, syn.slashComments, code, comment);
        }
        if (trim(code).empty()) {
            // the symbols ACME and KickAssembler output names in comments
            static const std::regex symRe(R"(^\s*(global|extern|weak):\s*([A-Za-z_@.][A-Za-z0-9_@.]*).*$)");
            std::smatch m;
            AsmIR::Statement st;
            if (std::regex_match(comment, m, symRe)) {
                std::string kind = m[1].str();
                st.type = AsmIR::Statement::Type::DIRECTIVE;
                st.dir.name = kind;
                st.dir.type = directiveType(kind);
                st.dir.arguments = {m[2].str()};
                symbol(m[2].str(), st.dir.type);
            } else if (!comment.empty()) {
                // the writers put a space after ';'
                st.type = AsmIR::Statement::Type::COMMENT;
                st.comment = comment[0] == ' ' ? comment.substr(1) : comment;
            } else {
                st.type = AsmIR::Statement::Type::BLANK;
            }
            add(st);
            return;
        }

        static const std::regex constRe(R"(^\s*([A-Za-z_][A-Za-z0-9_]*)\s*(?:=|:=|\s[Ee][Qq][Uu]\s)\s*(.+)$)");
        static const std::regex labelRe(R"(^\s*([A-Za-z_@.!:\]][A-Za-z0-9_@.]*)\s*:(?!=)(.*)$)");
        static const std::regex columnRe(R"(^([A-Za-z_@.:\]][A-Za-z0-9_@.]*)(\s.*)?$)");
        static const std::regex orgRe(R"(^\s*\*\s*=\s*(.+)$)");
        static const std::regex wordRe(R"(^\s*([.!+]?)([A-Za-z_0-9][A-Za-z0-9_]*(?:\.[A-Za-z0-9_]+)*)\b\s*(.*)$)");
        std::smatch m;

        // a constant: NAME = e, NAME := e, NAME EQU e
        if (std::regex_match(code, m, constRe)) {
            AsmIR::Statement st;
            st.type = AsmIR::Statement::Type::DIRECTIVE;
            st.dir.type = AsmIR::DirectiveType::CONST;
            st.dir.name = "const";
            st.dir.arguments = {m[1].str(), trim(m[2].str())};
            st.comment = comment;
            add(st);
            return;
        }
        // a label: NAME: (any dialect), or a word in column 1
        std::string label;
        if (std::regex_match(code, m, labelRe)) {
            label = m[1].str();
            code = m[2].str();
        } else if (syn.columnLabels && std::regex_match(code, m, columnRe)) {
            std::string w = m[1].str();
            bool directiveWord = ((syn.keywords || syn.bare.count(lower(w))) &&
                                  (syn.directives.count(lower(w)) || syn.ignored.count(lower(w)))) ||
                                 w[0] == syn.prefix;   // .org in column 1 (x65, ca65)
            if (w[0] != syn.prefix &&
                (syn.columnLabelsAlways || (!isMnemonic(w) && !directiveWord && !module.macros.count(w)))) {
                label = w;
                code = m[2].matched ? m[2].str() : "";
            }
        }
        if (!label.empty()) {
            // Merlin: LABEL EQU e, LABEL = e
            static const std::regex equRe(R"(^\s*(?:=|[Ee][Qq][Uu]\s)\s*(.+)$)");
            if (std::regex_match(code, m, equRe)) {
                AsmIR::Statement st;
                st.type = AsmIR::Statement::Type::DIRECTIVE;
                st.dir.type = AsmIR::DirectiveType::CONST;
                st.dir.name = "const";
                st.dir.arguments = {label, trim(m[1].str())};
                st.comment = comment;
                add(st);
                return;
            }
            AsmIR::Statement st;
            st.type = AsmIR::Statement::Type::LABEL;
            st.label = label;
            add(st);
            if (trim(code).empty()) {
                if (!comment.empty()) {
                    AsmIR::Statement c;
                    c.type = AsmIR::Statement::Type::COMMENT;
                    c.comment = comment;
                    add(c);
                }
                return;
            }
        }
        if (std::regex_match(code, m, orgRe)) {
            AsmIR::Statement st;
            st.type = AsmIR::Statement::Type::DIRECTIVE;
            st.dir.type = AsmIR::DirectiveType::ORG;
            st.dir.name = "org";
            st.dir.arguments = {trim(m[1].str())};
            st.comment = comment;
            add(st);
            return;
        }
        if (!std::regex_match(code, m, wordRe)) {
            AsmIR::Statement st;
            st.type = AsmIR::Statement::Type::INSTRUCTION;
            st.instr.mnemonic = trim(code);
            st.instr.mode = AsmIR::AddressingMode::RAW;
            st.comment = comment;
            warn("'" + trim(code) + "' is kept as written");
            add(st);
            return;
        }
        std::string pre = m[1].str(), word = m[2].str(), rest = trim(m[3].str());
        // directives: the dialect's prefix, '.', or a keyword
        if ((pre == std::string(1, syn.prefix) || pre == ".") && directive(word, rest, comment, true)) return;
        if (pre.empty() && directive(word, rest, comment, false)) return;
        // macros: +name (ACME), name(args) (KickAssembler), name args
        if (pre == "+" && syn.plusMacros && macroCall(word, rest, depth)) return;
        if (pre.empty() && syn.callMacros && !rest.empty() && rest[0] == '(' && rest.back() == ')' &&
            macroCall(word, rest.substr(1, rest.size() - 2), depth))
            return;
        if (pre.empty() && macroCall(word, rest, depth)) return;

        AsmIR::Statement st;
        st.type = AsmIR::Statement::Type::INSTRUCTION;
        st.comment = comment;
        if (!pre.empty()) {
            st.instr.mnemonic = trim(code);
            st.instr.mode = AsmIR::AddressingMode::RAW;
            warn("'" + trim(code) + "' is kept as written");
            add(st);
            return;
        }
        st.instr.mnemonic = lower(word);
        std::string core;
        AsmIR::AddressingMode mode = classify(rest, core);
        // an operand of another instruction (ca45's simulated ones: push a,
        // fill dst, #n, add.16) is kept as written
        if (!isMnemonic(word)) mode = AsmIR::AddressingMode::RAW;
        st.instr.mode = mode;
        st.instr.operand.text = mode == AsmIR::AddressingMode::RAW ? rest : charsAsNumbers(core, charEncoding());
        if (mode == AsmIR::AddressingMode::RAW && !warnedRaw) {
            warn("'" + trim(code) + "' is kept as written");
            warnedRaw = true;
        }
        add(st);
    }
};

// Lines of a macro definition: they are in module.macros, not statements
bool macroStart(const std::string& t, const DialectSyntax& syn) {
    std::string l = lower(t);
    if (l.rfind(".macro", 0) == 0 || l.rfind("!macro", 0) == 0) return true;
    // Merlin: NAME MAC
    static const std::regex macRe(R"(^\S+\s+[Mm][Aa][Cc]\b.*$)");
    return syn.columnLabelsAlways && std::regex_match(t, macRe);
}

bool macroEnd(const std::string& t) {
    std::string l = lower(trim(t));
    return l.rfind(".endmacro", 0) == 0 || l.rfind(".endmac", 0) == 0 || l == "}" ||
           l.rfind("<<<", 0) == 0 || l == "eom" || l.rfind("\teom", 0) == 0;
}

}  // namespace

void readDialect(const std::string& source, const DialectSyntax& syntax, AsmIR::Module& module,
                 std::vector<std::string>& warnings) {
    std::string src = source;
    if (syntax.slashComments) {
        // /* ... */ comments: blanked, their newlines kept
        for (size_t at = src.find("/*"); at != std::string::npos; at = src.find("/*", at)) {
            size_t end = src.find("*/", at + 2);
            end = end == std::string::npos ? src.size() : end + 2;
            for (size_t i = at; i < end; i++)
                if (src[i] != '\n') src[i] = ' ';
        }
    }
    Reader r{syntax, module, warnings};
    std::stringstream ss(src);
    std::string text;
    bool inMacro = false;
    while (std::getline(ss, text)) {
        r.lineNo++;
        if (!text.empty() && text.back() == '\r') text.pop_back();
        if (inMacro) {
            if (macroEnd(text)) inMacro = false;
            continue;
        }
        if (macroStart(trim(text), syntax) || macroStart(text, syntax)) {
            inMacro = !(trim(text).find('}') != std::string::npos && trim(text).back() == '}');
            continue;
        }
        r.line(text);
    }
}
