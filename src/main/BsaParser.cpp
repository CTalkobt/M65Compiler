#include "BsaParser.hpp"
#include "DialectReader.hpp"
#include "MacroUtils.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <sstream>
#include <vector>

// BSA's syntax and semantics, as bsa.c (version 03-Mar-2025) has them (#371).
// The source is turned into ca45's syntax line by line, then read by the
// shared DialectReader.

namespace {

const int32_t UNDEF = 0xff0000;   // BSA's value of an undefined symbol

std::string upper(std::string s) {
    for (auto& c : s) c = (char)std::toupper((unsigned char)c);
    return s;
}
std::string lower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}
std::string trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r");
    return s.substr(b, e - b + 1);
}
std::string hexByte(int v) {
    static const char* d = "0123456789abcdef";
    std::string s = "$";
    s += d[(v >> 4) & 15];
    s += d[v & 15];
    return s;
}

// a symbol character, as BSA's isym (and the first one may not be a digit
// unless the symbol is an nn$ label of the C65's BSO sources)
bool isSymChar(char c) { return c == '_' || c == '$' || c == '.' || std::isalnum((unsigned char)c); }
bool isSymStart(const std::string& s, size_t p) {
    if (p >= s.size()) return false;
    char c = s[p];
    if (c == '_' || c == '.' || std::isalpha((unsigned char)c)) return true;
    if (c == '@' && p + 1 < s.size() && std::isalpha((unsigned char)s[p + 1])) return true;
    return false;
}
// nn$: digits, then '$' (a BSO local label)
bool isNnd(const std::string& s, size_t p) {
    size_t q = p;
    while (q < s.size() && std::isdigit((unsigned char)s[q])) q++;
    return q > p && q < s.size() && s[q] == '$';
}

// BSA's mnemonics (its opcode tables; the 45GS02's Q forms and the
// L-prefixed long branches): a word in column 1 that is one is an
// instruction, not a label
const std::set<std::string>& mnemonics() {
    static const std::set<std::string> m = [] {
        std::set<std::string> s = {
            "ADC", "AND", "ASL", "ASR", "ASW", "AUG", "BBR", "BBS", "BCC", "BCS", "BEQ", "BIT", "BMI", "BNE",
            "BPL", "BRA", "BRK", "BRU", "BSR", "BVC", "BVS", "CLC", "CLD", "CLE", "CLI", "CLV", "CMP", "CPX",
            "CPY", "CPZ", "DEA", "DEC", "DEW", "DEX", "DEY", "DEZ", "EOM", "EOR", "INA", "INC", "INW", "INX",
            "INY", "INZ", "JMP", "JSR", "LDA", "LDX", "LDY", "LDZ", "LSR", "MAP", "MVN", "MVP", "NEG", "NOP",
            "ORA", "PHA", "PHB", "PHD", "PHK", "PHP", "PHW", "PHX", "PHY", "PHZ", "PLA", "PLB", "PLD", "PLP",
            "PLX", "PLY", "PLZ", "RMB", "ROL", "ROR", "ROW", "RTI", "RTL", "RTS", "SBC", "SEC", "SED", "SEE",
            "SEI", "SMB", "STA", "STP", "STX", "STY", "STZ", "SWA", "TAB", "TAX", "TAY", "TAZ", "TBA", "TCD",
            "TCS", "TDC", "TRB", "TSA", "TSB", "TSC", "TSX", "TSY", "TXA", "TXS", "TYA", "TYS", "TYX", "TZA",
            "WAI", "WDM", "XBA", "XCE",
            "ORQ", "ANDQ", "EORQ", "ADCQ", "STQ", "LDQ", "CMPQ", "SBCQ", "ASLQ", "ROLQ", "LSRQ", "RORQ",
            "DEQ", "INQ", "ASRQ", "BITQ", "ORAQ", "INCQ", "DECQ"};
        for (const char* b : {"BPL", "BMI", "BVC", "BVS", "BCC", "BCS", "BNE", "BEQ", "BRA"})
            s.insert(std::string("L") + b);
        for (int i = 0; i < 8; i++)
            for (const char* b : {"BBR", "BBS", "RMB", "SMB"}) s.insert(b + std::to_string(i));
        return s;
    }();
    return m;
}

// a pseudo-op BSA knows (IsPseudo matches the first letters)
bool isPseudo(const std::string& name) {
    static const char* const names[] = {"WORD", "BIGW", "HEX4", "DEC4", "WOR", "BYTE", "BYT", "PET", "DISP",
                                        "BHEX", "BITS", "LITS", "QUAD", "REAL", "FILL", "BSS", "STORE", "CPU",
                                        "BASE", "CASE", "ORG", "LOAD", "INCLUDE", "SRC", "SIZE", "SKI", "PAG",
                                        "NAM", "SUBTTL", "END"};
    for (const char* n : names)
        if (name.compare(0, std::strlen(n), n) == 0) return true;
    return false;
}

// The names BSA symbols get in ca45: a MODULE's .x / _x locals and the BSO
// nn$ labels with their scope; '$' and '@' (legal in BSA names) made into
// ca45's letters, digits and '_' (ca45 takes '.' in a name, not first)
struct Names {
    bool fold = false;          // symbols case-insensitive (.CASE -, BSO mode)
    bool bso = false;           // nn$ local labels (the C65's BSO sources)
    std::string module;         // the current MODULE
    std::string bsoScope;       // the last column-1 label (BSO mode)
    std::string map(std::string sym) const {
        if (!module.empty() && (sym[0] == '.' || sym[0] == '_')) sym = module + sym;
        if (bso && isNnd(sym, 0) && !bsoScope.empty()) sym = bsoScope + "_" + sym;
        if (fold) sym = lower(sym);
        std::string out;
        for (size_t i = 0; i < sym.size(); i++) {
            char c = sym[i];
            // (ca45 takes '.' in a name, not first: there it starts a directive)
            if (c == '.' && i == 0) out += "_";
            else if (c == '$') out += "_S";
            else if (c == '@') out += "_A";
            else out += c;
        }
        if (!out.empty() && std::isdigit((unsigned char)out[0])) out = "L" + out;
        return out;
    }
};

// One BSA expression, parsed by BSA's rules (EvalOperand: unary operators
// bind to the next operand, the binary ones by C-like priorities) and
// written for ca45 with the parentheses ca45's own priorities need (few:
// an operand starting with '(' is indirect there, and a leading 0+ would
// change its base-page / absolute choice); BSA's tight < and > as ca45's
// lo() and hi()
struct Val {
    std::string text;
    bool known = false;
    int32_t v = 0;
    int prec = 10;         // ca45's priority of its top operator (10: an operand)
};

// ca45's priorities (doc/bin/ca45.md: * / over + - over << >> over & ^ | over
// the comparisons over && ||)
int ca45Prec(const std::string& op) {
    if (op == "*" || op == "/") return 9;
    if (op == "+" || op == "-") return 8;
    if (op == "<<" || op == ">>") return 7;
    if (op == "&") return 6;
    if (op == "^") return 5;
    if (op == "|") return 4;
    if (op == "&&") return 2;
    if (op == "||") return 1;
    return 3;   // == != < <= > >=
}
// v's text, in parentheses when its operator binds less than `min`
std::string wrap(const Val& v, int min) { return v.prec < min ? "(" + v.text + ")" : v.text; }

struct BinOp { const char* op; int prio; };
const BinOp BINOPS[] = {{"*", 11}, {"/", 11}, {"+", 10}, {"-", 10}, {"<<", 9}, {">>", 9}, {"<=", 8},
                        {"<", 8},  {">=", 8}, {">", 8},  {"==", 7}, {"!=", 7}, {"^", 5},  {"&&", 3},
                        {"&", 6},  {"||", 2}, {"|", 4}};

class Expr {
public:
    Expr(const std::string& s, size_t p, const Names& names, const std::map<std::string, int32_t>& consts,
         bool forcedHex = false)
        : s_(s), p_(p), names_(names), consts_(consts), forcedHex_(forcedHex) {}
    Val parse() { return operand(0); }
    size_t pos() const { return p_; }
    bool forced = false;          // ^: a 16-bit operand
    std::string problem;          // a construct not converted (?sym)

private:
    const std::string& s_;
    size_t p_;
    const Names& names_;
    const std::map<std::string, int32_t>& consts_;
    bool forcedHex_;

    void skip() { while (p_ < s_.size() && (s_[p_] == ' ' || s_[p_] == '\t')) p_++; }
    static Val lit(int32_t v, const std::string& text) { Val r; r.known = true; r.v = v; r.text = text; return r; }

    Val unary(char op, int32_t (*f)(int32_t), const std::string& pre) {
        p_++;
        Val a = operand(12);
        Val r;
        // (ca45's < and > take everything after them: lo() and hi() instead)
        if (op == '<') r.text = "lo(" + a.text + ")";
        else if (op == '>') r.text = "hi(" + a.text + ")";
        else r.text = pre + wrap(a, 10);
        r.known = a.known && a.v != UNDEF;
        r.v = r.known ? f(a.v) : UNDEF;
        return r;
    }

    Val operand(int prio) {
        skip();
        Val left;
        char c = p_ < s_.size() ? s_[p_] : 0;
        if (c == '[' || c == '(') {
            p_++;
            Val a = operand(0);
            skip();
            if (p_ < s_.size() && (s_[p_] == ']' || s_[p_] == ')')) p_++;
            left = a;   // (its parentheses only where ca45's priorities need them)
        } else if (c == '+') {
            p_++;
            left = operand(12);
        } else if (c == '-') {
            left = unary('-', [](int32_t v) { return -v; }, "-");
        } else if (c == '!') {
            left = unary('!', [](int32_t v) { return (int32_t)!v; }, "!");
        } else if (c == '~') {
            left = unary('~', [](int32_t v) { return ~v; }, "~");
        } else if (c == '<') {
            left = unary('<', [](int32_t v) { return v & 0xff; }, "<");
        } else if (c == '>') {
            left = unary('>', [](int32_t v) { return v >> 8; }, ">");
        } else if (c == '*') {
            p_++;
            left.text = "*";
        } else if (c == '$') {
            p_++;
            size_t b = p_;
            while (p_ < s_.size() && std::isxdigit((unsigned char)s_[p_])) p_++;
            uint32_t v = b < p_ ? (uint32_t)std::stoul(s_.substr(b, p_ - b), nullptr, 16) : 0;
            // written without leading zeros: BSA chooses base page or absolute
            // by value, ca45 reads $00D0 as absolute
            std::ostringstream h;
            h << "$" << std::hex << v;
            left = lit((int32_t)v, h.str());
        } else if (c == '\'') {
            p_++;
            int32_t v;
            if (p_ >= s_.size() || s_[p_] == ' ') { v = ' '; }
            else if (s_[p_] == '\\') {
                p_++;
                char e = p_ < s_.size() ? s_[p_] : 0;
                v = e == 'r' ? 13 : e == 'n' ? 10 : e == 'a' ? 7 : e == 'e' ? 27 : e == '0' ? 0 : (unsigned char)e;
                p_++;
            } else {
                v = (unsigned char)s_[p_++];
            }
            if (p_ < s_.size() && s_[p_] == '\'') p_++;
            left = lit(v, std::to_string(v));
        } else if (c == '%') {
            p_++;
            int32_t r = 0;
            while (p_ < s_.size() && (s_[p_] == ' ' || s_[p_] == '1' || s_[p_] == '0' || s_[p_] == '*' || s_[p_] == '.')) {
                if (s_[p_] == '1' || s_[p_] == '*') r = (r << 1) + 1;
                if (s_[p_] == '0' || s_[p_] == '.') r <<= 1;
                p_++;
            }
            left = lit(r, std::to_string(r));
        } else if (c == '?') {
            p_++;
            size_t b = p_;
            while (p_ < s_.size() && isSymChar(s_[p_])) p_++;
            problem = "'?" + s_.substr(b, p_ - b) + "' (the length of a .BYTE line) is not converted";
            left.text = "0";
        } else if (c == '^') {
            p_++;
            forced = true;
            left = operand(12);
        } else if (forcedHex_ && std::isxdigit((unsigned char)c)) {
            size_t b = p_;
            while (p_ < s_.size() && std::isxdigit((unsigned char)s_[p_])) p_++;
            uint32_t v = (uint32_t)std::stoul(s_.substr(b, p_ - b), nullptr, 16);
            left = lit((int32_t)v, std::to_string(v));
        } else if (std::isdigit((unsigned char)c) && !isNnd(s_, p_)) {
            size_t b = p_;
            while (p_ < s_.size() && std::isdigit((unsigned char)s_[p_])) p_++;
            int32_t v = (int32_t)std::stol(s_.substr(b, p_ - b));
            left = lit(v, std::to_string(v));
        } else if (isSymStart(s_, p_) || isNnd(s_, p_)) {
            size_t b = p_;
            if (s_[p_] == '@') p_++;
            while (p_ < s_.size() && isSymChar(s_[p_])) p_++;
            std::string name = names_.map(s_.substr(b, p_ - b));
            left.text = name;
            auto it = consts_.find(name);
            if (it != consts_.end()) { left.known = true; left.v = it->second; }
            else left.v = UNDEF;
        } else {
            left.text = "";   // an empty operand (the caller reports it)
            return left;
        }

        // the binary operators, by BSA's priorities
        while (true) {
            skip();
            if (p_ >= s_.size() || !std::strchr("=*/+-<>!&^|", s_[p_])) break;
            const BinOp* op = nullptr;
            for (const auto& b : BINOPS)
                if (s_.compare(p_, std::strlen(b.op), b.op) == 0) { op = &b; break; }
            if (!op || op->prio <= prio) break;
            p_ += std::strlen(op->op);
            Val right = operand(op->prio);
            Val r;
            r.prec = ca45Prec(op->op);
            r.text = wrap(left, r.prec) + " " + op->op + " " + wrap(right, r.prec + 1);
            r.known = left.known && right.known && left.v != UNDEF && right.v != UNDEF;
            if (r.known) {
                int32_t l = left.v, rv = right.v;
                std::string o = op->op;
                if (o == "*") r.v = l * rv;
                else if (o == "/") { if (rv) r.v = l / rv; else { r.known = false; r.v = UNDEF; } }
                else if (o == "+") r.v = l + rv;
                else if (o == "-") r.v = l - rv;
                else if (o == "<<") r.v = l << rv;
                else if (o == ">>") r.v = l >> rv;
                else if (o == "<=") r.v = l <= rv;
                else if (o == "<") r.v = l < rv;
                else if (o == ">=") r.v = l >= rv;
                else if (o == ">") r.v = l > rv;
                else if (o == "==") r.v = l == rv;
                else if (o == "!=") r.v = l != rv;
                else if (o == "^") r.v = l ^ rv;
                else if (o == "&&") r.v = l && rv;
                else if (o == "&") r.v = l & rv;
                else if (o == "||") r.v = l || rv;
                else if (o == "|") r.v = l | rv;
            } else {
                r.v = UNDEF;
            }
            left = r;
        }
        return left;
    }
};

// the top of an expression for ca45: a binary operation without its outer
// parentheses; a leading '(' guarded (ca45 would read '(e)' as indirect)
std::string topText(const Val& v) {
    std::string t = v.text;
    if (!t.empty() && t[0] == '(') t = "0+" + t;
    return t;
}

// a string as BSA's ParseASCII reads it: escapes \r \n \a \e \0, a doubled
// '' for the quote, then ^ or +$80 sets the last character's bit 7
std::vector<int> parseString(const std::string& s, size_t& p) {
    std::vector<int> b;
    char delim = s[p++];
    while (p < s.size() && (s[p] != delim || (p + 1 < s.size() && s[p + 1] == s[p]))) {
        if (s[p] == '\\' && p + 1 < s.size()) {
            char e = s[++p];
            b.push_back(e == 'r' ? 13 : e == 'n' ? 10 : e == 'a' ? 7 : e == 'e' ? 27 : e == '0' ? 0 : (unsigned char)e);
            p++;
        } else if (s[p] == '\'' && p + 1 < s.size() && s[p + 1] == '\'') {
            b.push_back('\'');
            p += 2;
        } else {
            b.push_back((unsigned char)s[p++]);
        }
    }
    if (p < s.size() && s[p] == delim) p++;
    if (p < s.size() && s[p] == '^') { if (!b.empty()) b.back() |= 0x80; p++; }
    if (s.compare(p, 4, "+$80") == 0) { if (!b.empty()) b.back() |= 0x80; p += 4; }
    return b;
}

// the position after the item that starts at p (up to a ',' outside quotes
// and brackets), as BSA's SkipToComma
size_t itemEnd(const std::string& s, size_t p) {
    int depth = 0;
    char q = 0;
    for (; p < s.size(); p++) {
        char c = s[p];
        if (q) { if (c == q) q = 0; continue; }
        if (c == '"') { q = c; continue; }
        if (c == '(' || c == '[') depth++;
        else if (c == ')' || c == ']') depth--;
        else if (c == ',' && depth <= 0) break;
    }
    return p;
}

// BSA's .REAL / .REAL4 (ParseRealData): the CBM float of a decimal number,
// 5 bytes (4 with REAL4), rounded as BSA rounds it
std::vector<int> cbmReal(const std::string& arg, bool real4, bool& ok) {
    int mansize = real4 ? 3 : 4;
    unsigned char op[8] = {0};
    std::string a = trim(arg);
    ok = true;
    if (!a.empty() && a[0] == '$') {
        size_t p = 1;
        for (int i = 0; i <= mansize && p + 1 < a.size() + 1; i++, p += 2) {
            if (p + 1 >= a.size() + 1 || !std::isxdigit((unsigned char)a[p]) || !std::isxdigit((unsigned char)a[p + 1])) break;
            op[i] = (unsigned char)std::stoul(a.substr(p, 2), nullptr, 16);
        }
    } else if (!a.empty() && a[0] == '@') {
        size_t p = 0;
        for (int i = 0; i <= mansize && p + 4 <= a.size(); i++, p += 5) op[i] = (unsigned char)std::stoul(a.substr(p + 1, 3), nullptr, 8);
    } else {
        double d = std::strtod(a.c_str(), nullptr);
        if (d != 0) {
            int sign = 0, exponent;
            if (d < 0) { sign = 0x80; d = -d; }
            d = std::frexp(d, &exponent);
            exponent += 0x80;
            if (exponent < 1 || exponent > 255) { ok = false; return {}; }
            op[0] = (unsigned char)exponent;
            d *= 256;
            unsigned v = (unsigned)d;
            op[1] = (unsigned char)((v & 127) | sign);
            d -= v;
            for (int i = 2; i < 6; i++) {
                d *= 256;
                op[i] = (unsigned char)(v = (unsigned)d);
                d -= v;
            }
        }
    }
    if (op[mansize + 1] & 0x80) {
        int i;
        for (i = mansize; i > 1; --i) if (++op[i]) break;
        if (i == 1) {
            if (op[1] == 0x7f) { ++op[0]; op[1] = 0; }
            else if (op[1] == 0xff) { ++op[0]; op[1] = 0x80; }
            else ++op[1];
        }
    }
    std::vector<int> out;
    for (int i = 0; i <= mansize; i++) out.push_back(op[i]);
    return out;
}

class Converter {
public:
    explicit Converter(std::vector<std::string>& warnings) : warnings_(warnings) {}
    std::string run(const std::string& source);

private:
    std::vector<std::string>& warnings_;
    Names names_;
    std::map<std::string, int32_t> consts_;
    struct Macro { std::vector<std::string> params, body; };
    std::map<std::string, Macro> macros_;   // upper-case name
    std::string bssBase = "0";              // & = expr: the BSS pointer, as base + offsets
    std::string bssOffset;
    bool storeWarned_ = false;
    int lineNo_ = 0;
    std::ostringstream out_;

    void warn(const std::string& msg) { warnings_.push_back("line " + std::to_string(lineNo_) + ": " + msg); }
    Val expr(const std::string& s, size_t& p, bool forcedHex = false, bool* forced = nullptr) {
        Expr e(s, p, names_, consts_, forcedHex);
        Val v = e.parse();
        p = e.pos();
        if (!e.problem.empty()) warn(e.problem);
        if (forced && e.forced) *forced = true;
        return v;
    }
    Val exprOf(const std::string& s, bool forcedHex = false, bool* forced = nullptr) {
        size_t p = 0;
        Val v = expr(s, p, forcedHex, forced);
        if (trim(s.substr(p)) != "") warn("'" + trim(s.substr(p)) + "' after the expression '" + trim(s.substr(0, p)) + "' is ignored");
        return v;
    }
    void emit(const std::string& line) { out_ << line << "\n"; }
    void statement(std::string body, const std::string& comment);
    std::string operand(const std::string& op, bool& forced);
    void byteData(const std::string& name, const std::string& args, const std::string& comment);
    void lines(const std::vector<std::string>& src);
};

// an instruction's operand, in ca45's syntax
std::string Converter::operand(const std::string& opIn, bool& forced) {
    std::string op = trim(opIn);
    if (op.empty()) return "";
    if (upper(op) == "A") return "a";
    if (op[0] == '#') {
        Val v = exprOf(op.substr(1), false, &forced);
        return "#" + v.text;   // (no indirect form to guard against)
    }
    // the closing bracket matching the one at 0
    auto match = [&](size_t open) {
        int d = 0;
        char q = 0;
        for (size_t i = open; i < op.size(); i++) {
            char c = op[i];
            if (q) { if (c == q) q = 0; continue; }
            if (c == '"') { q = c; continue; }
            if (c == '\'' && i + 2 < op.size() && op[i + 2] == '\'') { i += 2; continue; }
            if (c == '(' || c == '[') d++;
            else if (c == ')' || c == ']') { if (--d == 0) return i; }
        }
        return std::string::npos;
    };
    auto index = [](const std::string& rest) {   // ",x" after an operand: its register, lower case
        std::string r = lower(trim(rest));
        if (r.size() >= 2 && r[0] == ',') return trim(r.substr(1));
        return std::string();
    };
    if (op[0] == '[' || op[0] == '(') {
        size_t close = match(0);
        if (close != std::string::npos) {
            std::string inner = op.substr(1, close - 1), rest = op.substr(close + 1);
            std::string reg = index(rest);
            if (op[0] == '[' && reg == "z") return "[" + topText(exprOf(inner, false, &forced)) + "],z";
            if (op[0] == '(') {
                // (e,x), (e,sp),y
                size_t comma = itemEnd(inner, 0);
                if (comma < inner.size()) {
                    std::string r = lower(trim(inner.substr(comma + 1)));
                    std::string e = topText(exprOf(inner.substr(0, comma), false, &forced));
                    if (trim(rest).empty()) return "(" + e + "," + r + ")";
                    if (r == "sp" && reg == "y") return "(" + e + ",sp),y";
                }
                if (reg == "y" || reg == "z") return "(" + topText(exprOf(inner, false, &forced)) + ")," + reg;
                if (trim(rest).empty()) return "(" + topText(exprOf(inner, false, &forced)) + ")";
            }
        }
    }
    // e, e,x / e,y, or two operands (BBR zp,target)
    size_t comma = itemEnd(op, 0);
    if (comma >= op.size()) return topText(exprOf(op, false, &forced));
    std::string first = topText(exprOf(op.substr(0, comma), false, &forced));
    std::string second = trim(op.substr(comma + 1));
    std::string reg = lower(second);
    if (reg == "x" || reg == "y" || reg == "z") return first + "," + reg;
    return first + ", " + topText(exprOf(second, false, &forced));
}

// .BYTE / .BYT / .PET / .DISP / .BHEX: each item's bytes, as BSA's ParseByteData
void Converter::byteData(const std::string& name, const std::string& args, const std::string& comment) {
    enum { ASCII, PETSCII, SCREENCODE } charset = name == "PET" ? PETSCII : name == "DISP" ? SCREENCODE : ASCII;
    bool forcedHex = name == "BHEX";
    std::vector<std::string> items;
    size_t p = 0;
    const std::string& s = args;
    while (p < s.size()) {
        while (p < s.size() && (s[p] == ' ' || s[p] == '\t')) p++;
        if (p >= s.size()) break;
        char d = s[p];
        if ((d == '<' || d == '>' || d == '#') && p + 5 < s.size() && s[p + 1] == '"' && s[p + 5] == '"') {
            // packed (<, >) and hashed (#) three-letter ASCII
            int c1 = (unsigned char)s[p + 2], c2 = (unsigned char)s[p + 3], c3 = (unsigned char)s[p + 4];
            if (d == '<') items.push_back(hexByte(((c2 - 63) << 6) | ((c3 - 63) << 1)));
            else if (d == '>') items.push_back(hexByte(((c1 - 63) << 3) | ((c2 - 63) >> 2)));
            else {
                int v = c3 - 64 + 27 * (c2 - 64 + 27 * (c1 - 64));
                items.push_back(hexByte(v & 0xff));
                items.push_back(hexByte((v >> 8) & 0xff));
            }
            p += 6;
        } else if (d == '"' || d == '\'') {
            for (int b : parseString(s, p)) {
                if (charset == PETSCII) {
                    if (b >= 'A' && b <= 'Z') b |= 0x80;
                    if (b >= 'a' && b <= 'z') b -= 0x20;
                }
                if (charset == SCREENCODE && b >= 'a' && b <= 'z') b -= 0x60;
                items.push_back(hexByte(b & 0xff));
            }
        } else {
            size_t q = p;
            Val v = expr(s, q, forcedHex);
            if (v.text.empty()) { warn("an empty item in ." + name); q = itemEnd(s, p); }
            if (v.known) {
                items.push_back(hexByte(v.v & 0xff));
                // a value above 255 (or below -127) is two bytes, low first
                if (d != '<' && d != '>' && (v.v > 255 || v.v < -127)) items.push_back(hexByte((v.v >> 8) & 0xff));
            } else {
                if (d != '<' && d != '>')
                    warn("." + name + " '" + trim(s.substr(p, q - p)) + "': BSA writes two bytes when its value is "
                         "above 255; written as one byte");
                items.push_back(topText(v));
            }
            p = q;
        }
        p = itemEnd(s, p);
        if (p < s.size() && s[p] == ',') p++;
    }
    std::string line = "    .byte ";
    for (size_t i = 0; i < items.size(); i++) line += (i ? ", " : "") + items[i];
    emit(line + comment);
}

// one statement (after its label): a pseudo-op, an instruction, an origin
void Converter::statement(std::string body, const std::string& comment) {
    body = trim(body);
    if (body.empty()) { if (!comment.empty()) emit(comment.substr(1)); return; }
    if (upper(body).rfind("!ADDR ", 0) == 0) body = trim(body.substr(6));
    // * = e, & = e
    if ((body[0] == '*' || body[0] == '&') && trim(body.substr(1)).rfind('=', 0) == 0) {
        std::string e = trim(trim(body.substr(1)).substr(1));
        if (body[0] == '*') {
            emit("    .org " + topText(exprOf(e)) + comment);
        } else {
            Val b = exprOf(e);
            bssBase = wrap(b, 8);
            bssOffset.clear();
            if (!comment.empty()) emit(comment.substr(1));
        }
        return;
    }
    // pseudo-ops: .NAME (or !SRC)
    if (body[0] == '.' || body[0] == '!') {
        size_t e = 1;
        while (e < body.size() && std::isalnum((unsigned char)body[e])) e++;
        std::string name = upper(body.substr(1, e - 1)), args = trim(body.substr(e));
        // BSA matches its pseudo-ops by their first letters
        auto is = [&](const char* full, size_t min) {
            return name.size() >= min && std::string(full).compare(0, name.size(), name) == 0;
        };
        if (body[0] == '!' && name == "SRC") name = "INCLUDE";
        // data turned into bytes: the BSA line in the comment
        static const std::set<std::string> asBytes = {"PET", "DISP", "BHEX", "BIGW", "QUAD", "REAL", "REAL4",
                                                       "HEX4", "DEC4", "BITS", "LITS"};
        bool strings = (name == "BYTE" || name == "BYT") && args.find_first_of("\"'") != std::string::npos;
        if ((asBytes.count(name) || strings) && comment.empty()) {
            std::string c = " ; ." + name + " " + args;
            statement(body, c);   // (the comment given, the same statement)
            return;
        }
        if (name == "BYTE" || name == "BYT" || name == "PET" || name == "DISP" || name == "BHEX") {
            byteData(name, args, comment);
        } else if (name == "WORD" || name == "WOR" || name == "BIGW") {
            std::string line = name == "BIGW" ? "    .byte " : "    .word ";
            size_t p = 0;
            bool first = true;
            while (p < args.size()) {
                size_t q = itemEnd(args, p);
                Val v = exprOf(args.substr(p, q - p));
                std::string t = topText(v);
                if (name == "BIGW") {
                    if (v.known) t = hexByte((v.v >> 8) & 0xff) + ", " + hexByte(v.v & 0xff);
                    else t = ">(" + t + "), <(" + t + ")";
                }
                line += (first ? "" : ", ") + t;
                first = false;
                p = q + 1;
            }
            emit(line + comment);
        } else if (name == "QUAD") {
            // one literal, big-endian (CBM): $hex digits as written, or decimal
            std::vector<int> b(4, 0);
            std::string a = trim(args);
            if (!a.empty() && a[0] == '$') {
                for (int i = 0; i < 4 && 1 + 2 * i + 1 < (int)a.size() + 1; i++) {
                    if (1 + 2 * i + 2 > (int)a.size()) break;
                    b[i] = (int)std::stoul(a.substr(1 + 2 * i, 2), nullptr, 16);
                }
            } else {
                long w = std::atol(a.c_str());
                for (int i = 3; i >= 0; --i) { b[i] = (int)(w & 255); w >>= 8; }
            }
            emit("    .byte " + hexByte(b[0]) + ", " + hexByte(b[1]) + ", " + hexByte(b[2]) + ", " + hexByte(b[3]) + comment);
        } else if (name == "REAL" || name == "REAL4") {
            bool ok;
            auto b = cbmReal(args, name == "REAL4", ok);
            if (!ok) { warn("." + name + " " + args + ": the exponent is out of range"); return; }
            std::string line = "    .byte ";
            for (size_t i = 0; i < b.size(); i++) line += (i ? ", " : "") + hexByte(b[i]);
            emit(line + comment);
        } else if (name == "HEX4" || name == "DEC4") {
            Val v = exprOf(args);
            if (!v.known) { warn("." + name + " needs a value known while converting: kept as written"); emit("    ; ." + name + " " + args); return; }
            char buf[16];
            if (name == "HEX4") std::snprintf(buf, sizeof buf, "%4.4X", (unsigned)(v.v & 0xffff));
            else std::snprintf(buf, sizeof buf, "%4d", (int)v.v);
            std::string line = "    .byte ";
            for (int i = 0; i < 4; i++) line += (i ? ", " : "") + hexByte((unsigned char)buf[i]);
            emit(line + comment);
        } else if (name == "BITS" || name == "LITS") {
            int v = 0;
            size_t p = 0;
            for (int i = 0; i < 8; i++) {
                while (p < args.size() && args[p] == ' ') p++;
                char c = p < args.size() ? args[p++] : '.';
                if (name == "BITS") { v <<= 1; if (c == '*') v |= 1; }
                else { v >>= 1; if (c == '*') v |= 128; }
            }
            emit("    .byte " + hexByte(v) + comment);
        } else if (name == "FILL") {
            // .FILL count (value)
            size_t p = 0;
            Val m = expr(args, p);
            while (p < args.size() && args[p] == ' ') p++;
            std::string v = "0";
            if (p < args.size() && args[p] == '(') {
                size_t q = args.rfind(')');
                Val fv = exprOf(args.substr(p + 1, (q == std::string::npos ? args.size() : q) - p - 1));
                v = fv.known ? hexByte(fv.v & 0xff) : "<(" + topText(fv) + ")";
            } else {
                warn(".FILL without a '(value)': BSA requires one");
            }
            emit("    .fill " + topText(m) + ", " + v + comment);
        } else if (name == "BSS") {
            Val n = exprOf(args);   // a .BSS without a label: the pointer moves
            bssOffset += " + " + wrap(n, 9);
            if (!comment.empty()) emit(comment.substr(1));
        } else if (name == "ORG") {
            emit("    .org " + topText(exprOf(args)) + comment);
        } else if (name == "CPU") {
            std::string c = upper(args);
            if (c == "45GS02") emit("    .cpu 45gs02" + comment);
            else if (c == "6502") emit("    .cpu 6502" + comment);
            else if (c == "65C02" || c == "65SC02") emit("    .cpu 65c02" + comment);
            else { warn(".CPU " + args + ": ca45 assembles the 6502, 65C02 and 45GS02"); emit("    ; .CPU " + args); }
        } else if (name == "LOAD" || name == "STORE") {
            // BSA writes its binary as the source says; ca45's output is chosen on its command line
            if (!storeWarned_)
                warn("." + name + ": BSA's binary output is set in the source, ca45's on its command line "
                     "(ca45 x.s45 -o x.bin; a .prg with its load address: -o x.prg): kept as a comment");
            storeWarned_ = true;
            emit("    ; ." + name + (args.empty() ? "" : " " + args) + comment);
        } else if (name == "CASE") {
            names_.fold = trim(args) == "-";
        } else if (name == "BASE") {
            warn(".BASE " + args + ": the base page is set at run time on the 45GS02 (TAB): not converted");
            emit("    ; .BASE " + args + comment);
        } else if (name == "INCLUDE") {
            emit("    .include " + args + comment);
        } else if (name == "SIZE" || is("SKIP", 3) || is("PAGE", 3) || is("NAME", 3) || name == "SUBTTL") {
            if (!comment.empty()) emit(comment.substr(1));   // listing only
        } else {
            warn("unknown pseudo-op ." + name + ": kept as written");
            emit("    " + body + comment);
        }
        return;
    }
    // an instruction
    size_t e = 0;
    while (e < body.size() && std::isalnum((unsigned char)body[e])) e++;
    std::string mn = lower(body.substr(0, e)), op = body.substr(e);
    // a mnemonic with a suffix (add.16, neg.32, lda.fp) is ca45's syntax,
    // which BSA has none of: kept as written
    if (e < body.size() && body[e] == '.') mn += "?";
    if (!mnemonics().count(upper(mn))) {
        warn("'" + body + "' is neither an instruction nor a pseudo-op BSA knows: kept as written");
        emit("    " + body + comment);
        return;
    }
    // LBNE ...: a 16-bit branch, always (BSA); ca45 has no mnemonic for it
    // (it sizes branches itself, 8-bit when in range): its bytes, the
    // opcode (the 8-bit one + 3) and the offset from the instruction's end
    static const std::map<std::string, int> longBranch = {
        {"lbpl", 0x13}, {"lbmi", 0x33}, {"lbvc", 0x53}, {"lbvs", 0x73}, {"lbcc", 0x93},
        {"lbcs", 0xb3}, {"lbne", 0xd3}, {"lbeq", 0xf3}, {"lbra", 0x83}};
    auto lb = longBranch.find(mn);
    if (lb != longBranch.end()) {
        Val t = exprOf(op);
        emit("    .byte " + hexByte(lb->second) + " ; " + mn + " " + trim(op) + comment);
        emit("    .word " + wrap(t, 8) + " - * - 1");
        return;
    }
    bool forced = false;
    std::string o = operand(op, forced);
    emit("    " + mn + (forced ? ".abs" : "") + (o.empty() ? "" : " " + o) + comment);
}

void Converter::lines(const std::vector<std::string>& src) {
    for (size_t li = 0; li < src.size(); li++) {
        lineNo_++;
        std::string raw = src[li];
        // the comment: from ';' outside strings
        std::string code = raw, comment;
        {
            char q = 0;
            for (size_t i = 0; i < raw.size(); i++) {
                char c = raw[i];
                if (q) { if (c == q && !(c == '\'' && i + 1 < raw.size() && raw[i + 1] == '\'')) q = 0; else if (c == '\'' && q == '\'') i++; continue; }
                if (c == '"') { q = c; continue; }
                if (c == '\'') {
                    // a character constant 'c' or a string '...'
                    if (i + 2 < raw.size() && raw[i + 2] == '\'') { i += 2; continue; }
                    q = c;
                    continue;
                }
                if (c == ';') { code = raw.substr(0, i); comment = " ;" + raw.substr(i + 1); break; }
            }
        }
        std::string t = trim(code);
        if (t.empty()) { emit(comment.empty() ? "" : comment.substr(1)); continue; }
        std::string ut = upper(t);
        // MODULE name / ENDMOD
        if (ut.rfind("MODULE", 0) == 0 && (ut.size() == 6 || ut[6] == ' ' || ut[6] == '\t')) {
            std::string name = trim(t.substr(6));
            emit(names_.map(name) + ":" + comment);
            names_.module = name;
            continue;
        }
        if (ut.rfind("ENDMOD", 0) == 0) { names_.module.clear(); if (!comment.empty()) emit(comment.substr(1)); continue; }
        if (ut.rfind(".END", 0) == 0 && (ut.size() == 4 || !std::isalnum((unsigned char)ut[4]))) break;

        // a label, as BSA's ParseLine: after leading blanks, a word (a letter,
        // '_' or nn$ first) that is neither a mnemonic nor a macro, its ':'
        // optional; '.', '!', '*' and '&' start a pseudo-op or an assignment
        std::string label, rest = code;
        bool col1 = !code.empty() && code[0] != ' ' && code[0] != '\t';
        {
            size_t p = 0;
            while (p < code.size() && (code[p] == ' ' || code[p] == '\t')) p++;
            if (p < code.size() && (code[p] == '_' || std::isalpha((unsigned char)code[p]) || isNnd(code, p))) {
                size_t e = p;
                while (e < code.size() && isSymChar(code[e])) e++;
                std::string word = code.substr(p, e - p);
                size_t after = e;
                while (after < code.size() && (code[after] == ' ' || code[after] == '\t')) after++;
                bool isMnemonic = mnemonics().count(upper(word)) > 0;
                bool isMacroCall = macros_.count(upper(word)) > 0;
                bool colon = after < code.size() && code[after] == ':';
                // without a ':', a label only before a statement BSA reads
                // (an instruction, a pseudo-op it knows, '=', a macro, nothing):
                // else the line is kept as written (ca45's own syntax that
                // asmCvt45 wrote for BSA: add.16, proc, ...)
                bool before = colon || after >= code.size();
                if (!before) {
                    std::string r = code.substr(after);
                    if (r[0] == '=') before = true;
                    else if (r[0] == '.' || r[0] == '!') {
                        size_t n = 1;
                        while (n < r.size() && std::isalnum((unsigned char)r[n])) n++;
                        before = isPseudo(upper(r.substr(1, n - 1)));
                    } else {
                        size_t n = 0;
                        while (n < r.size() && isSymChar(r[n])) n++;
                        std::string w = upper(r.substr(0, n));
                        before = mnemonics().count(w) || macros_.count(w);
                    }
                }
                // (ca45's endproc and simulated instructions without an operand,
                // written for BSA as they were, are no labels: sxt.16, pha.all)
                if (after >= code.size()) {
                    std::string w = lower(word);
                    size_t dot = w.find('.');
                    std::string suffix = dot == std::string::npos ? "" : w.substr(dot + 1);
                    static const std::set<std::string> suffixes = {"all", "fp", "sp", "abs", "zp", "s16", "s32", "a"};
                    if (w == "endproc" || w == "proc" || w == "rtn" || (dot != std::string::npos && dot <= 12 &&
                        (suffixes.count(suffix) || (!suffix.empty() && suffix.find_first_not_of("0123456789") == std::string::npos))))
                        before = false;
                }
                // a word with a ca45 suffix (branch.16 BEQ, target) is ca45's
                // syntax, never a BSA label
                {
                    std::string w = lower(word);
                    size_t dot = w.find('.');
                    std::string suffix = dot == std::string::npos ? "" : w.substr(dot + 1);
                    if (dot != std::string::npos && dot <= 12 && !suffix.empty() &&
                        suffix.find_first_not_of("0123456789") == std::string::npos)
                        before = false;
                }
                if (!isMnemonic && !isMacroCall && before) {
                    label = word;
                    rest = code.substr(colon ? after + 1 : e);
                }
            }
        }
        std::string body = trim(rest);
        if (!label.empty()) {
            std::string name = names_.map(label);
            // (BSO mode: a column-1 label starts a scope for the nn$ labels)
            if (names_.bso && col1 && !isNnd(label, 0) && std::isalpha((unsigned char)label[0])) {
                names_.bsoScope = label;
                name = names_.map(label);
            }
            if (!body.empty() && body[0] == '=' && !(body.size() > 1 && body[1] == '=')) {
                Val v = exprOf(body.substr(1));
                if (v.known) consts_[name] = v.v;
                emit(name + " = " + topText(v) + comment);
                continue;
            }
            std::string ub = upper(body);
            if (ub.rfind(".BSS", 0) == 0) {
                // NAME .BSS n: NAME at the BSS pointer, which moves by n
                Val n = exprOf(body.substr(4));
                std::string at = bssBase + bssOffset;
                emit(name + " = " + at + comment);
                bssOffset += " + " + wrap(n, 9);
                continue;
            }
            emit(name + ":");
        }
        // a macro call: NAME(args), expanded where it is
        {
            size_t e = 0;
            while (e < body.size() && isSymChar(body[e])) e++;
            std::string word = upper(body.substr(0, e));
            size_t open = e;
            while (open < body.size() && body[open] == ' ') open++;
            if (e > 0 && macros_.count(word) && open < body.size() && body[open] == '(') {
                const Macro& m = macros_[word];
                size_t close = body.rfind(')');
                std::string argText = body.substr(open + 1, (close == std::string::npos ? body.size() : close) - open - 1);
                std::vector<std::string> args;
                for (size_t p = 0; p <= argText.size();) {
                    size_t q = itemEnd(argText, p);
                    args.push_back(trim(argText.substr(p, q - p)));
                    p = q + 1;
                    if (q >= argText.size()) break;
                }
                std::vector<std::string> expanded;
                for (std::string bl : m.body) {
                    for (size_t k = 0; k < m.params.size() && k < args.size(); k++) {
                        // the parameter as a whole word
                        const std::string& prm = m.params[k];
                        std::string r;
                        for (size_t i = 0; i < bl.size();) {
                            if (bl.compare(i, prm.size(), prm) == 0 && (i == 0 || !isSymChar(bl[i - 1])) &&
                                (i + prm.size() >= bl.size() || !isSymChar(bl[i + prm.size()]))) {
                                r += args[k];
                                i += prm.size();
                            } else {
                                r += bl[i++];
                            }
                        }
                        bl = r;
                    }
                    expanded.push_back(bl);
                }
                if (!comment.empty()) emit(comment.substr(1));
                int saved = lineNo_;
                lines(expanded);
                lineNo_ = saved;
                continue;
            }
        }
        statement(body, comment);
    }
}

std::string Converter::run(const std::string& source) {
    std::vector<std::string> src;
    {
        std::istringstream in(source);
        std::string l;
        while (std::getline(in, l)) {
            if (!l.empty() && l.back() == '\r') l.pop_back();
            src.push_back(l);
        }
    }
    // BSO mode (the C65's sources for the VAX BSO assembler, BSA's .src
    // files): nn$ local labels, symbols case-insensitive
    for (const auto& l : src) {
        size_t p = 0;
        while (p < l.size() && (l[p] == ' ' || l[p] == '\t')) p++;
        if (isNnd(l, p)) {
            size_t d = l.find('$', p);
            size_t after = d + 1;
            while (after < l.size() && l[after] == ' ') after++;
            if (after >= l.size() || l[after] == ':' || p == 0) { names_.bso = names_.fold = true; break; }
        }
    }

    // macros (MACRO NAME(params) ... ENDMAC) and conditional assembly
    // (#if e, #ifdef e: false when its value is undefined, #else, #endif,
    // #error), in source order; constants are known from where they are set
    std::vector<std::string> kept;
    std::vector<bool> skip;      // per #if level: this branch is skipped
    for (size_t i = 0; i < src.size(); i++) {
        std::string t = trim(src[i]);
        std::string u = upper(t);
        bool skipping = std::find(skip.begin(), skip.end(), true) != skip.end();
        if (!t.empty() && t[0] == '#') {
            std::string d = trim(t.substr(1)), ud = upper(d);
            lineNo_ = (int)i + 1;
            if (ud.rfind("IFDEF ", 0) == 0 || ud.rfind("IF ", 0) == 0) {
                bool isdef = ud.rfind("IFDEF ", 0) == 0;
                std::string e = d.substr(isdef ? 6 : 3);
                size_t sc = e.find(';');
                if (sc != std::string::npos) e = e.substr(0, sc);
                Val v = exprOf(e);
                bool known = v.known && v.v != UNDEF;
                if (!known && !skipping) {
                    // a symbol defined later (a label): BSA's later passes know it
                    bool laterDefined = false;
                    for (size_t k = i + 1; k < src.size() && !laterDefined; k++) {
                        std::string lt = trim(src[k]);
                        size_t e2 = 0;
                        while (e2 < lt.size() && isSymChar(lt[e2])) e2++;
                        if (e2 && names_.map(lt.substr(0, e2)) == trim(v.text) &&
                            (src[k].size() && src[k][0] != ' ' && src[k][0] != '\t')) laterDefined = true;
                    }
                    if (laterDefined)
                        warn("#" + upper(isdef ? "ifdef" : "if") + " " + trim(e) + ": a label defined later, read as "
                             "defined; check the branch");
                    if (laterDefined) known = true, v.v = 1;
                }
                skip.push_back(isdef ? !known : (!known || v.v == 0));
            } else if (ud.rfind("ELSE", 0) == 0) {
                if (!skip.empty()) skip.back() = !skip.back();
            } else if (ud.rfind("ENDIF", 0) == 0) {
                if (!skip.empty()) skip.pop_back();
            } else if (ud.rfind("ERROR", 0) == 0) {
                if (!skipping) warn("#error " + trim(d.substr(5)) + " (BSA stops here)");
            }
            continue;
        }
        if (skipping) continue;
        if (u.rfind("MACRO ", 0) == 0) {
            // MACRO NAME(params) ... ENDMAC
            std::string head = trim(t.substr(6));
            size_t open = head.find('(');
            std::string name = upper(trim(head.substr(0, open)));
            Macro m;
            if (open != std::string::npos) {
                size_t close = head.find(')', open);
                std::string ps = head.substr(open + 1, close - open - 1);
                std::istringstream pin(ps);
                std::string prm;
                while (std::getline(pin, prm, ',')) if (!trim(prm).empty()) m.params.push_back(trim(prm));
            }
            for (i++; i < src.size() && upper(trim(src[i])).rfind("ENDMAC", 0) != 0; i++) m.body.push_back(src[i]);
            macros_[name] = m;
            continue;
        }
        // constants set before an #if are known to it
        {
            size_t e = 0;
            std::string l = src[i];
            while (e < l.size() && (l[e] == ' ' || l[e] == '\t')) e++;
            size_t b = e;
            while (e < l.size() && isSymChar(l[e])) e++;
            size_t a = e;
            while (a < l.size() && l[a] == ' ') a++;
            if (e > b && a < l.size() && l[a] == '=' && !(a + 1 < l.size() && l[a + 1] == '=')) {
                std::string rhs = l.substr(a + 1);
                size_t sc = rhs.find(';');
                if (sc != std::string::npos) rhs = rhs.substr(0, sc);
                Expr ex(rhs, 0, names_, consts_);
                Val v = ex.parse();
                if (v.known) consts_[names_.map(l.substr(b, e - b))] = v.v;
            }
        }
        kept.push_back(src[i]);
    }
    lineNo_ = 0;
    lines(kept);
    return out_.str();
}

DialectSyntax syntax() {
    DialectSyntax s;
    s.name = "bsa";
    s.directives = {{"byte", "byte"}, {"word", "word"}, {"fill", "fill"}, {"org", "org"}, {"cpu", "cpu"}};
    return s;
}

}  // namespace

AsmIR::Module BsaParser::parse(const std::string& source) {
    AsmIR::Module module;
    try {
        // .INCLUDE "file" / !SRC "file": read first, as ca45 reads includes
        std::string withIncludes;
        {
            std::istringstream in(source);
            std::ostringstream o;
            std::string l;
            while (std::getline(in, l)) {
                std::string t = trim(l), u = upper(t);
                if (u.rfind(".INCLUDE", 0) == 0) o << ".include" << t.substr(8) << "\n";
                else if (u.rfind("!SRC", 0) == 0) o << ".include" << t.substr(4) << "\n";
                else o << l << "\n";
            }
            std::set<std::string> processedFiles;
            std::vector<std::string> includeErrors;
            withIncludes = MacroUtils::processIncludes(o.str(), module, processedFiles, includeErrors);
            for (const auto& err : includeErrors) addWarning(err);
        }
        Converter conv(warnings_);
        std::string ca45 = conv.run(withIncludes);
        static const DialectSyntax syn = syntax();
        std::vector<std::string> readWarnings;
        readDialect(ca45, syn, module, readWarnings);
        // (zp),z and [zp],z are kept as written: here they are ca45's own syntax
        for (const auto& w : readWarnings)
            if (!(w.find("is kept as written") != std::string::npos && w.find("),z'") != std::string::npos) &&
                !(w.find("is kept as written") != std::string::npos && w.find("],z'") != std::string::npos))
                warnings_.push_back(w);
        module.cpu = "45GS02";   // ca45's: a 6502 program assembles unchanged
        module.is_o45 = false;
    } catch (const std::exception& e) {
        addError(std::string("Parse error: ") + e.what());
    }
    return module;
}
