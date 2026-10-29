#pragma once
// Conditional assembly in a dialect's own syntax, resolved for asmCvt45's
// readers (#224): ca45 has no conditional assembly, so the readers keep the
// lines of the branches taken and drop the others.
//
//   ca65:          .if e / .ifdef s / .ifndef s / .elseif e / .else / .endif
//   ACME:          !if e {   !ifdef s {   !ifndef s {   } else {   }
//   KickAssembler: .if (e) {   } else {   } else .if (e) {   }
//
// A condition is an integer expression of numbers ($hex, %bin, decimal,
// 'c'), the constants defined above it in the file (NAME = e, NAME := e,
// .const / .var NAME = e, .define NAME e), unary - ! ~ < >, the binary
// operators * / % + - << >> & ^ | and comparisons, && ||, parentheses and
// defined(NAME) / .defined(NAME). One it cannot evaluate (a label's address,
// a function) leaves that conditional as written, with a warning.

#include <cctype>
#include <map>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace dialectcond {

class Eval {
public:
    Eval(const std::string& s, const std::map<std::string, long>& c) : s_(s), c_(c) {}
    std::optional<long> run() {
        auto v = orExpr();
        skip();
        if (!v || p_ != s_.size()) return std::nullopt;
        return v;
    }

private:
    const std::string& s_;
    const std::map<std::string, long>& c_;
    size_t p_ = 0;

    void skip() { while (p_ < s_.size() && std::isspace((unsigned char)s_[p_])) p_++; }
    bool eat(const char* op) {
        skip();
        size_t n = std::char_traits<char>::length(op);
        if (s_.compare(p_, n, op) != 0) return false;
        // (not the start of a longer operator: "<" of "<<" or "<=", "&" of "&&")
        if (n == 1 && p_ + 1 < s_.size()) {
            char a = s_[p_], b = s_[p_ + 1];
            if ((a == '<' || a == '>') && (b == a || b == '=')) return false;
            if ((a == '&' || a == '|') && b == a) return false;
            if ((a == '=' || a == '!') && b == '=') return false;
        }
        p_ += n;
        return true;
    }
    std::optional<long> orExpr() {
        auto a = andExpr();
        while (a && (eat("||") || eat(".or"))) { auto b = andExpr(); if (!b) return b; a = (*a || *b) ? 1 : 0; }
        return a;
    }
    std::optional<long> andExpr() {
        auto a = cmpExpr();
        while (a && (eat("&&") || eat(".and"))) { auto b = cmpExpr(); if (!b) return b; a = (*a && *b) ? 1 : 0; }
        return a;
    }
    std::optional<long> cmpExpr() {
        auto a = bitOr();
        while (a) {
            int op = eat("==") ? 1 : eat("!=") ? 2 : eat("<>") ? 2 : eat("<=") ? 3 : eat(">=") ? 4 :
                     eat("<") ? 5 : eat(">") ? 6 : eat("=") ? 1 : 0;
            if (!op) break;
            auto b = bitOr();
            if (!b) return b;
            long x = *a, y = *b;
            a = (op == 1 ? x == y : op == 2 ? x != y : op == 3 ? x <= y : op == 4 ? x >= y : op == 5 ? x < y : x > y) ? 1 : 0;
        }
        return a;
    }
    std::optional<long> bitOr() {
        auto a = bitXor();
        while (a && eat("|")) { auto b = bitXor(); if (!b) return b; a = *a | *b; }
        return a;
    }
    std::optional<long> bitXor() {
        auto a = bitAnd();
        while (a && eat("^")) { auto b = bitAnd(); if (!b) return b; a = *a ^ *b; }
        return a;
    }
    std::optional<long> bitAnd() {
        auto a = shift();
        while (a && eat("&")) { auto b = shift(); if (!b) return b; a = *a & *b; }
        return a;
    }
    std::optional<long> shift() {
        auto a = add();
        while (a) {
            if (eat("<<")) { auto b = add(); if (!b) return b; a = *a << *b; }
            else if (eat(">>")) { auto b = add(); if (!b) return b; a = *a >> *b; }
            else break;
        }
        return a;
    }
    std::optional<long> add() {
        auto a = mul();
        while (a) {
            if (eat("+")) { auto b = mul(); if (!b) return b; a = *a + *b; }
            else if (eat("-")) { auto b = mul(); if (!b) return b; a = *a - *b; }
            else break;
        }
        return a;
    }
    std::optional<long> mul() {
        auto a = unary();
        while (a) {
            if (eat("*")) { auto b = unary(); if (!b) return b; a = *a * *b; }
            else if (eat("/")) { auto b = unary(); if (!b || !*b) return std::nullopt; a = *a / *b; }
            else if (eat("%")) { auto b = unary(); if (!b || !*b) return std::nullopt; a = *a % *b; }
            else break;
        }
        return a;
    }
    std::optional<long> unary() {
        if (eat("-")) { auto v = unary(); return v ? std::optional<long>(-*v) : v; }
        if (eat("!")) { auto v = unary(); return v ? std::optional<long>(!*v) : v; }
        if (eat("~")) { auto v = unary(); return v ? std::optional<long>(~*v) : v; }
        if (eat("<")) { auto v = unary(); return v ? std::optional<long>(*v & 0xFF) : v; }
        if (eat(">")) { auto v = unary(); return v ? std::optional<long>((*v >> 8) & 0xFF) : v; }
        return atom();
    }
    std::optional<long> atom() {
        skip();
        if (p_ >= s_.size()) return std::nullopt;
        if (eat("(")) {
            auto v = orExpr();
            if (!eat(")")) return std::nullopt;
            return v;
        }
        char c = s_[p_];
        auto digits = [&](int base) -> std::optional<long> {
            size_t st = p_;
            long v = 0;
            while (p_ < s_.size()) {
                char d = (char)std::tolower((unsigned char)s_[p_]);
                int k = std::isdigit((unsigned char)d) ? d - '0' : (d >= 'a' && d <= 'f') ? d - 'a' + 10 : 99;
                if (k >= base) break;
                v = v * base + k;
                p_++;
            }
            if (p_ == st) return std::nullopt;
            return v;
        };
        if (c == '$') { p_++; return digits(16); }
        if (c == '%') { p_++; return digits(2); }
        if (std::isdigit((unsigned char)c)) {
            if (c == '0' && p_ + 1 < s_.size() && (s_[p_ + 1] == 'x' || s_[p_ + 1] == 'X')) { p_ += 2; return digits(16); }
            return digits(10);
        }
        if (c == '\'' && p_ + 2 < s_.size() && s_[p_ + 2] == '\'') { long v = (unsigned char)s_[p_ + 1]; p_ += 3; return v; }
        if (std::isalpha((unsigned char)c) || c == '_' || c == '.') {
            size_t st = p_++;
            while (p_ < s_.size() && (std::isalnum((unsigned char)s_[p_]) || s_[p_] == '_')) p_++;
            std::string name = s_.substr(st, p_ - st);
            if (name == "defined" || name == ".defined" || name == ".def") {
                if (!eat("(")) return std::nullopt;
                skip();
                size_t ns = p_;
                while (p_ < s_.size() && (std::isalnum((unsigned char)s_[p_]) || s_[p_] == '_')) p_++;
                std::string sym = s_.substr(ns, p_ - ns);
                if (!eat(")")) return std::nullopt;
                return c_.count(sym) ? 1 : 0;
            }
            if (name == "true") return 1;
            if (name == "false") return 0;
            auto it = c_.find(name);
            if (it == c_.end()) return std::nullopt;
            return it->second;
        }
        return std::nullopt;
    }
};

inline std::optional<long> evaluate(const std::string& e, const std::map<std::string, long>& consts) {
    return Eval(e, consts).run();
}

// The source with its conditionals resolved; dialect "ca65", "acme" or
// "kickassembler". Warnings name what was kept as written.
inline std::string resolve(const std::string& source, const std::string& dialect, std::vector<std::string>& warnings) {
    std::map<std::string, long> consts;
    // a frame: whether its lines are kept, whether a branch was taken, and
    // (ACME / Kick) whether it is a brace block of a conditional
    struct Frame { bool active, taken, kept; bool brace; };
    std::vector<Frame> st;
    std::vector<bool> otherBraces;   // (ACME / Kick: a '{' not of a conditional)
    auto active = [&]() { for (auto& f : st) if (!f.active) return false; return true; };
    auto trim = [](std::string x) {
        size_t a = x.find_first_not_of(" \t"), b = x.find_last_not_of(" \t\r");
        return a == std::string::npos ? std::string() : x.substr(a, b - a + 1);
    };
    // the code of a line (the comment cut off: ';' and, for Kick, '//')
    auto codeOf = [&](const std::string& l) {
        char q = 0;
        for (size_t i = 0; i < l.size(); i++) {
            if (q) { if (l[i] == q) q = 0; continue; }
            if (l[i] == '"' || l[i] == '\'') { q = l[i]; continue; }
            if (l[i] == ';') return l.substr(0, i);
            if (dialect == "kickassembler" && l[i] == '/' && i + 1 < l.size() && l[i + 1] == '/') return l.substr(0, i);
        }
        return l;
    };
    static const std::regex constRe(R"re(^\s*(?:\.const\s+|\.var\s+|\.label\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*:?=\s*(.+)$)re");
    static const std::regex defineRe(R"re(^\s*\.define\s+([A-Za-z_][A-Za-z0-9_]*)\s+(.+)$)re");
    std::istringstream in(source);
    std::ostringstream out;
    std::string line;
    int lineNo = 0;
    auto keepRaw = [&](const std::string& what) {
        warnings.push_back("line " + std::to_string(lineNo) + ": '" + what +
                           "' cannot be evaluated here (a label's address, a function?): kept as written");
    };
    while (std::getline(in, line)) {
        lineNo++;
        std::string code = trim(codeOf(line));
        std::smatch m;
        std::string low;
        for (char ch : code) low += (char)std::tolower((unsigned char)ch);
        bool handled = false;
        if (dialect == "ca65") {
            auto cond = [&](const std::string& rest, bool def, bool neg) -> std::optional<bool> {
                if (def) { bool d = consts.count(trim(rest)) > 0; return neg ? !d : d; }
                auto v = evaluate(rest, consts);
                if (!v) return std::nullopt;
                return *v != 0;
            };
            if (std::regex_match(low, std::regex(R"re(^\.if(def|ndef)?\b.*)re"))) {
                bool def = low.rfind(".ifdef", 0) == 0, ndef = low.rfind(".ifndef", 0) == 0;
                std::string rest = code.substr(def ? 6 : ndef ? 7 : 3);
                auto c = cond(rest, def || ndef, ndef);
                if (!c) { keepRaw(code); st.push_back({true, true, true, false}); out << line << "\n"; }
                else st.push_back({*c, *c, false, false});
                handled = true;
            } else if (low.rfind(".elseif", 0) == 0 && !st.empty() && !st.back().kept) {
                auto c = cond(code.substr(7), false, false);
                Frame& f = st.back();
                if (!c) { keepRaw(code); f.active = false; }
                else { f.active = !f.taken && *c; f.taken = f.taken || f.active; }
                handled = true;
            } else if (low == ".else" && !st.empty() && !st.back().kept) {
                Frame& f = st.back(); f.active = !f.taken; f.taken = true; handled = true;
            } else if (low == ".endif" && !st.empty()) {
                if (st.back().kept) out << line << "\n";
                st.pop_back(); handled = true;
            } else if ((low == ".else" || low.rfind(".elseif", 0) == 0) && !st.empty() && st.back().kept) {
                out << line << "\n"; handled = true;
            }
        } else {
            // ACME: !if e {   Kick: .if (e) {   ...   } else {   } else .if (e) {   }
            static const std::regex acmeIf(R"re(^!(if|ifdef|ifndef)\s+(.*?)\s*\{$)re", std::regex::icase);
            static const std::regex kickIf(R"re(^\.if\s*\((.*)\)\s*\{$)re", std::regex::icase);
            static const std::regex elseRe(R"re(^\}\s*else\s*\{$)re", std::regex::icase);
            static const std::regex elseIfRe(R"re(^\}\s*else\s*(?:\.if\s*\((.*)\)|!if\s+(.*?))\s*\{$)re", std::regex::icase);
            bool isIf = false;
            std::optional<bool> c;
            if (dialect == "acme" && std::regex_match(code, m, acmeIf)) {
                isIf = true;
                std::string kind = m[1].str();
                for (auto& ch : kind) ch = (char)std::tolower((unsigned char)ch);
                if (kind == "if") { auto v = evaluate(m[2].str(), consts); if (v) c = *v != 0; }
                else c = (consts.count(trim(m[2].str())) > 0) == (kind == "ifdef");
            } else if (dialect == "kickassembler" && std::regex_match(code, m, kickIf)) {
                isIf = true;
                auto v = evaluate(m[1].str(), consts);
                if (v) c = *v != 0;
            }
            if (isIf) {
                if (!c) { keepRaw(code); st.push_back({true, true, true, true}); out << line << "\n"; }
                else st.push_back({*c, *c, false, true});
                handled = true;
            } else if (std::regex_match(code, m, elseIfRe) && !st.empty() && st.back().brace) {
                Frame& f = st.back();
                if (f.kept) out << line << "\n";
                else {
                    auto v = evaluate(m[1].matched ? m[1].str() : m[2].str(), consts);
                    if (!v) { keepRaw(code); f.active = false; }
                    else { f.active = !f.taken && *v != 0; f.taken = f.taken || f.active; }
                }
                handled = true;
            } else if (std::regex_match(code, elseRe) && !st.empty() && st.back().brace) {
                Frame& f = st.back();
                if (f.kept) out << line << "\n";
                else { f.active = !f.taken; f.taken = true; }
                handled = true;
            } else if (code == "}" && otherBraces.empty() && !st.empty() && st.back().brace) {
                if (st.back().kept) out << line << "\n";
                st.pop_back();
                handled = true;
            } else if (!code.empty() && code.back() == '{') {
                otherBraces.push_back(true);   // (a macro, a namespace, ...: its '}' is not ours)
            } else if (code == "}" && !otherBraces.empty()) {
                otherBraces.pop_back();
            }
        }
        if (handled) continue;
        if (!active()) continue;
        // the constants defined so far (a value it cannot evaluate: not known)
        if (std::regex_match(code, m, defineRe) || std::regex_match(code, m, constRe)) {
            auto v = evaluate(m[2].str(), consts);
            if (v) consts[m[1].str()] = *v; else consts.erase(m[1].str());
        }
        out << line << "\n";
    }
    if (!st.empty()) warnings.push_back("a conditional is not closed at the end of the file");
    return out.str();
}

}  // namespace dialectcond
