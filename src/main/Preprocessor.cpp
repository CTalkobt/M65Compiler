#include <cstring>
#include <cstdio>
#include "Preprocessor.hpp"
#include <fstream>
#include <sstream>
#include <deque>
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

#include <iostream>
#include <ctime>
#include <iomanip>
#include <functional>

Preprocessor::Preprocessor(bool isCompiler) : isCompiler(isCompiler) {
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);

    std::stringstream dateSS;
    dateSS << "\"" << std::put_time(&tm, "%b %d %Y") << "\"";
    preprocDate = dateSS.str();

    std::stringstream timeSS;
    timeSS << "\"" << std::put_time(&tm, "%H:%M:%S") << "\"";
    preprocTime = timeSS.str();

    // Compiler identification
    macros["__cc45__"] = Macro{false, false, {}, "1"};
    macros["__CC45_VERSION__"] = Macro{false, false, {}, "10005"};  // 1.00.05 → 10005

    // Target platform identification
    macros["__MEGA65__"] = Macro{false, false, {}, "1"};
    // __noescape: a constant whose address stays in its object's code (an
    // overlay keeps it with its code; Issue #301)
    macros["__noescape"] = Macro{false, false, {}, "__attribute__((noescape))"};
    // __section("NAME"): the variable is placed in the section NAME, which
    // #pragma section("NAME", address) declares (Issue #70)
    macros["__section"] = Macro{true, false, {"n"}, "__attribute__((cc45_section(n)))"};
    // __pure: a function without side effects (Issue #76)
    macros["__pure"] = Macro{false, false, {}, "__attribute__((pure))"};
    macros["__45GS02__"] = Macro{false, false, {}, "1"};
    macros["__6502__"] = Macro{false, false, {}, "1"};

    // Standard predefined macros (C17)
    macros["__STDC__"] = Macro{false, false, {}, "1"};
    macros["__STDC_VERSION__"] = Macro{false, false, {}, "201710L"};
    macros["__STDC_EMBED_NOT_FOUND__"] = Macro{false, false, {}, "0"};
    macros["__STDC_EMBED_FOUND__"] = Macro{false, false, {}, "1"};
    macros["__STDC_EMBED_EMPTY__"] = Macro{false, false, {}, "2"};
    macros["__STDC_HOSTED__"] = Macro{false, false, {}, "0"};  // freestanding

    // C11+ optional feature-test macros (§6.10.8.3)
    macros["__STDC_NO_VLA__"] = Macro{false, false, {}, "1"};      // no variable-length arrays
    macros["__STDC_NO_THREADS__"] = Macro{false, false, {}, "1"};   // no <threads.h>
    macros["__STDC_NO_ATOMICS__"] = Macro{false, false, {}, "1"};   // no <stdatomic.h>

    // GCC builtin types
    macros["__builtin_va_list"] = Macro{false, false, {}, "void *"};

    // GCC keyword synonyms
    macros["__volatile"] = Macro{false, false, {}, "volatile"};
    macros["__volatile__"] = Macro{false, false, {}, "volatile"};
    macros["__const"] = Macro{false, false, {}, "const"};
    macros["__const__"] = Macro{false, false, {}, "const"};
    macros["__restrict"] = Macro{false, false, {}, "restrict"};
    macros["__restrict__"] = Macro{false, false, {}, "restrict"};
    macros["__inline"] = Macro{false, false, {}, "inline"};
    macros["__forceinline"] = Macro{false, false, {}, "inline __attribute__((always_inline))"};
    macros["__inline__"] = Macro{false, false, {}, "inline"};
    macros["__signed"] = Macro{false, false, {}, "signed"};
    macros["__signed__"] = Macro{false, false, {}, "signed"};

    // Parameterized integer types: __int(N) → struct __intN, __uint(N) → struct __uintN
    // (8, 16 and 32 bits: the native signed/unsigned char, int, long; see expandMacros)
    macros["__int"] = Macro{true, false, {"N"}, "struct __int##N"};
    macros["__uint"] = Macro{true, false, {"N"}, "struct __uint##N"};

    // __int128 without parens — can't use macro (breaks "unsigned __int128")
    // Handled in parser instead via identifier check


    // Decimal floating-point types (GCC extension) — map to float for now
    macros["_Decimal32"] = Macro{false, false, {}, "float"};
    macros["_Decimal64"] = Macro{false, false, {}, "float"};
    macros["_Decimal128"] = Macro{false, false, {}, "float"};

    // GCC-compatible predefined macros for target (45GS02: 8-bit char, 16-bit int/short, 32-bit long)
    macros["__CHAR_BIT__"] = Macro{false, false, {}, "8"};
    macros["__SIZEOF_CHAR__"] = Macro{false, false, {}, "1"};
    macros["__SIZEOF_SHORT__"] = Macro{false, false, {}, "2"};
    macros["__SIZEOF_INT__"] = Macro{false, false, {}, "2"};
    macros["__SIZEOF_LONG__"] = Macro{false, false, {}, "4"};
    macros["__SIZEOF_LONG_LONG__"] = Macro{false, false, {}, "0"}; // not supported
    macros["__SIZEOF_POINTER__"] = Macro{false, false, {}, "2"};
    macros["__INT_MAX__"] = Macro{false, false, {}, "32767"};
    macros["__LONG_MAX__"] = Macro{false, false, {}, "2147483647L"};
    macros["__CHAR_MAX__"] = Macro{false, false, {}, "127"};
    macros["__SCHAR_MAX__"] = Macro{false, false, {}, "127"};
    macros["__SHRT_MAX__"] = Macro{false, false, {}, "32767"};
    macros["__LONG_LONG_MAX__"] = Macro{false, false, {}, "0"}; // not supported
    macros["__ORDER_LITTLE_ENDIAN__"] = Macro{false, false, {}, "1234"};
    // Floating types: float, double and long double are all the CBM 40-bit
    // format (32-bit mantissa, exponent range 2^-128 .. 2^127); see <float.h>
    for (const char* f : {"FLT", "DBL", "LDBL"}) {
        std::string p = std::string("__") + f;
        macros[p + "_MAX__"] = Macro{false, false, {}, "1.7014118342085515e38"};
        macros[p + "_MIN__"] = Macro{false, false, {}, "2.9387358770557188e-39"};
        macros[p + "_EPSILON__"] = Macro{false, false, {}, "4.6566128730773926e-10"};
        macros[p + "_MANT_DIG__"] = Macro{false, false, {}, "32"};
        macros[p + "_DIG__"] = Macro{false, false, {}, "9"};
        macros[p + "_MIN_EXP__"] = Macro{false, false, {}, "(-127)"};
        macros[p + "_MAX_EXP__"] = Macro{false, false, {}, "127"};
        macros[p + "_MIN_10_EXP__"] = Macro{false, false, {}, "(-38)"};
        macros[p + "_MAX_10_EXP__"] = Macro{false, false, {}, "38"};
    }
    macros["__SIZEOF_FLOAT__"] = Macro{false, false, {}, "5"};
    macros["__SIZEOF_DOUBLE__"] = Macro{false, false, {}, "5"};
    macros["__SIZEOF_LONG_DOUBLE__"] = Macro{false, false, {}, "5"};
    // GCC type macros — map to cc45 native types
    macros["__SIZE_TYPE__"] = Macro{false, false, {}, "unsigned int"};
    macros["__PTRDIFF_TYPE__"] = Macro{false, false, {}, "int"};
    macros["__INTPTR_TYPE__"] = Macro{false, false, {}, "int"};
    macros["__UINTPTR_TYPE__"] = Macro{false, false, {}, "unsigned int"};
    macros["__INT8_TYPE__"] = Macro{false, false, {}, "signed char"};
    macros["__UINT8_TYPE__"] = Macro{false, false, {}, "unsigned char"};
    macros["__INT16_TYPE__"] = Macro{false, false, {}, "int"};
    macros["__UINT16_TYPE__"] = Macro{false, false, {}, "unsigned int"};
    macros["__INT32_TYPE__"] = Macro{false, false, {}, "long"};
    macros["__UINT32_TYPE__"] = Macro{false, false, {}, "unsigned long"};
    macros["__BYTE_ORDER__"] = Macro{false, false, {}, "1234"}; // little-endian (6502)
}

// Does the code name __int(N) / __uint(N) of a width other than 8, 16 or 32
// (those are the native char, int and long)?
static bool namesWideIntN(const std::string& code) {
    for (const char* p : {"__int(", "__uint("}) {
        for (size_t at = code.find(p); at != std::string::npos; at = code.find(p, at + 1)) {
            if (at > 0 && (std::isalnum((unsigned char)code[at - 1]) || code[at - 1] == '_')) continue;
            size_t a = at + std::string(p).size(), e = code.find(')', a);
            if (e == std::string::npos) return true;
            std::string n = code.substr(a, e - a);
            n.erase(0, n.find_first_not_of(" \t"));
            n.erase(n.find_last_not_of(" \t") + 1);
            if (n != "8" && n != "16" && n != "32") return true;
        }
    }
    return false;
}

// A call of a function-like macro that the line leaves open: its arguments
// go on over the following lines (the caller joins them before expanding)
bool Preprocessor::hasOpenMacroCall(const std::string& line) const {
    size_t i = 0, n = line.size();
    while (i < n) {
        char c = line[i];
        if (c == '"' || c == '\'') {                 // a literal: skipped
            char q = c;
            for (i++; i < n && line[i] != q; i++) if (line[i] == '\\') i++;
            i++;
            continue;
        }
        if (!(std::isalpha((unsigned char)c) || c == '_')) { i++; continue; }
        size_t b = i;
        while (i < n && (std::isalnum((unsigned char)line[i]) || line[i] == '_')) i++;
        auto m = macros.find(line.substr(b, i - b));
        if (m == macros.end() || !m->second.isFunctionLike) continue;
        size_t k = i;
        while (k < n && std::isspace((unsigned char)line[k])) k++;
        if (k >= n || line[k] != '(') continue;
        int depth = 0;
        for (; k < n; k++) {
            char d = line[k];
            if (d == '"' || d == '\'') {
                char q = d;
                for (k++; k < n && line[k] != q; k++) if (line[k] == '\\') k++;
                continue;
            }
            if (d == '(') depth++;
            else if (d == ')' && --depth == 0) break;
        }
        if (k >= n) return true;                    // still open at the end
        i = k + 1;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Macro expansion (C11 6.10.3), on tokens. Each token carries a hide set: the
// macros whose expansion produced it, which do not expand it again (#define
// SELF SELF + 1). A macro's replacement is rescanned with the rest of the
// line (a function-like macro's name at its end takes the '(' that follows);
// an argument is expanded on its own before it is substituted, except as an
// operand of # or ## (XSTR(N) with #define XSTR(x) STR(x) is "4").
// ---------------------------------------------------------------------------

// The pp-tokens of a line, each with the white space before it
std::vector<Preprocessor::PPTok> Preprocessor::lexPP(const std::string& text) {
    std::vector<PPTok> out;
    size_t i = 0, n = text.size();
    auto isId = [](char c) { return std::isalnum((unsigned char)c) || c == '_'; };
    while (i < n) {
        PPTok t;
        size_t w = i;
        while (i < n && std::isspace((unsigned char)text[i])) i++;
        t.ws = text.substr(w, i - w);
        if (i >= n) {
            if (!t.ws.empty() && !out.empty()) { PPTok e; e.ws = t.ws; e.kind = 'm'; out.push_back(e); }
            break;
        }
        size_t b = i;
        char c = text[i];
        auto literal = [&](size_t q) {               // a string or character literal from q
            char d = text[q];
            for (i = q + 1; i < n && text[i] != d; i++) if (text[i] == '\\' && i + 1 < n) i++;
            if (i < n) i++;
        };
        if (std::isalpha((unsigned char)c) || c == '_') {
            while (i < n && isId(text[i])) i++;
            std::string id = text.substr(b, i - b);
            // an encoding prefix is part of its literal (L'1', u8"s"; cc45's p"x", s"x", a"x")
            if ((id == "L" || id == "u" || id == "U" || id == "u8" || id == "p" || id == "s" || id == "a") &&
                i < n && (text[i] == '"' || text[i] == '\'')) {
                literal(i);
                t.kind = 's';
            } else {
                t.kind = 'i';
            }
        } else if (std::isdigit((unsigned char)c) || (c == '.' && i + 1 < n && std::isdigit((unsigned char)text[i + 1]))) {
            // a pp-number (1.0F, 0x1fUL, 1e-5) is one token
            i++;
            while (i < n && (isId(text[i]) || text[i] == '.' ||
                             ((text[i] == '+' || text[i] == '-') && std::strchr("eEpP", text[i - 1]))))
                i++;
            t.kind = 'n';
        } else if (c == '"' || c == '\'') {
            literal(i);
            t.kind = 's';
        } else {
            static const char* ops[] = {"...", "<<=", ">>=", "##", "->", "++", "--", "<<", ">>", "<=", ">=", "==", "!=",
                                        "&&", "||", "*=", "/=", "%=", "+=", "-=", "&=", "^=", "|=", "::"};
            size_t len = 1;
            for (const char* op : ops) {
                size_t l = std::strlen(op);
                if (text.compare(i, l, op) == 0) { len = l; break; }
            }
            i += len;
            t.kind = 'p';
        }
        t.text = text.substr(b, i - b);
        out.push_back(std::move(t));
    }
    return out;
}

// The tokens as text: their white space kept; a space where two tokens of a
// replacement would otherwise run together into another token (- -, x y)
std::string Preprocessor::renderPP(const std::vector<PPTok>& toks) {
    std::string out;
    const PPTok* prev = nullptr;
    for (const auto& t : toks) {
        if (t.kind == 'm') { out += t.ws; continue; }
        if (prev && t.ws.empty() && (t.expanded || prev->expanded) && lexPP(prev->text + t.text).size() < 2)
            out += ' ';
        out += t.ws;
        out += t.text;
        prev = &t;
    }
    return out;
}

// The replacement of a macro: its body's tokens with the arguments put in
// (expanded, or raw next to # and ##), # and ## done, the hide set hs added
std::vector<Preprocessor::PPTok> Preprocessor::substPP(const std::vector<PPTok>& body, const Macro& m,
                                                         const std::vector<std::vector<PPTok>>& args,
                                                         const std::vector<PPTok>& va, bool haveVa,
                                                         const std::set<std::string>& hs, int depth) {
    auto param = [&](const PPTok& t) -> const std::vector<PPTok>* {
        if (t.kind != 'i') return nullptr;
        for (size_t p = 0; p < m.params.size(); p++)
            if (m.params[p] == t.text) return &args[p];
        if (m.isVariadic && t.text == "__VA_ARGS__") return &va;
        return nullptr;
    };
    auto isOp = [](const PPTok& t, const char* op) { return t.kind == 'p' && t.text == op; };
    auto placemarker = [](const std::string& ws) { PPTok p; p.kind = 'm'; p.ws = ws; return p; };
    // # x: the argument's spelling as a string literal (white space between
    // its tokens one space; " and \ in its literals escaped)
    auto stringize = [](const std::vector<PPTok>& a, const std::string& ws) {
        std::string s = "\"";
        bool first = true;
        for (const auto& t : a) {
            if (t.kind == 'm') continue;
            if (!first && !t.ws.empty()) s += ' ';
            first = false;
            if (t.kind == 's') {
                for (char c : t.text) { if (c == '"' || c == '\\') s += '\\'; s += c; }
            } else {
                s += t.text;
            }
        }
        PPTok r;
        r.ws = ws;
        r.text = s + "\"";
        r.kind = 's';
        return r;
    };
    // a ## b: one token made of the two (a placemarker on either side: the other)
    auto glue = [&](std::vector<PPTok>& os, const PPTok& rhs) {
        if (rhs.kind == 'm') return;
        if (os.empty() || os.back().kind == 'm') {
            std::string ws = os.empty() ? rhs.ws : os.back().ws;
            if (!os.empty()) os.pop_back();
            PPTok r = rhs;
            r.ws = ws;
            os.push_back(r);
            return;
        }
        PPTok lhs = os.back();
        os.pop_back();
        auto toks = lexPP(lhs.text + rhs.text);
        for (size_t k = 0; k < toks.size(); k++) {
            if (toks[k].kind == 'm') continue;
            toks[k].ws = k == 0 ? lhs.ws : toks[k].ws;
            for (const auto& h : lhs.hide) if (rhs.hide.count(h)) toks[k].hide.insert(h);
            os.push_back(toks[k]);
        }
    };
    std::vector<PPTok> os;
    for (size_t i = 0; i < body.size(); i++) {
        const PPTok& t = body[i];
        if (t.kind == 'm') continue;
        // # parameter
        if (isOp(t, "#") && i + 1 < body.size() && param(body[i + 1])) {
            os.push_back(stringize(*param(body[i + 1]), t.ws));
            i++;
            continue;
        }
        // ## operand
        if (isOp(t, "##") && i + 1 < body.size()) {
            const PPTok& r = body[i + 1];
            i++;
            const auto* a = param(r);
            // GNU: , ## __VA_ARGS__ drops the comma when there are no variadic arguments
            if (a == &va && !os.empty() && isOp(os.back(), ",")) {
                if (!haveVa || va.empty()) os.pop_back();
                else for (size_t k = 0; k < va.size(); k++) { PPTok x = va[k]; if (k == 0) x.ws = t.ws; os.push_back(x); }
                continue;
            }
            if (a) {
                if (a->empty()) { glue(os, placemarker(r.ws)); continue; }
                glue(os, (*a)[0]);
                for (size_t k = 1; k < a->size(); k++) os.push_back((*a)[k]);
            } else {
                glue(os, r);
            }
            continue;
        }
        if (const auto* a = param(t)) {
            bool pasted = i + 1 < body.size() && isOp(body[i + 1], "##");
            std::vector<PPTok> v = pasted ? *a : expandPP(*a, depth + 1);
            // (the parameter's place keeps its white space)
            size_t first = 0;
            while (first < v.size() && v[first].kind == 'm') first++;
            if (first >= v.size()) {
                os.push_back(placemarker(t.ws));
                continue;
            }
            for (size_t k = first; k < v.size(); k++) {
                PPTok x = v[k];
                if (k == first) x.ws = t.ws;
                os.push_back(x);
            }
            continue;
        }
        os.push_back(t);
    }
    std::vector<PPTok> out;
    for (auto& t : os) {
        if (t.kind == 'm') continue;
        t.hide.insert(hs.begin(), hs.end());
        t.expanded = true;
        out.push_back(std::move(t));
    }
    return out;
}

// The tokens with their macros expanded (the rest of the line rescanned with
// each replacement; a token never expands a macro its hide set holds)
std::vector<Preprocessor::PPTok> Preprocessor::expandPP(std::vector<PPTok> in, int depth) {
    std::vector<PPTok> out;
    if (depth > 200) return in;                   // (a guard only: hide sets end every recursion)
    std::deque<PPTok> q(std::make_move_iterator(in.begin()), std::make_move_iterator(in.end()));
    long budget = 2000000;
    auto number = [](const std::string& ws, const std::string& v) { PPTok t; t.ws = ws; t.text = v; t.kind = 'n'; t.expanded = true; return t; };
    // the tokens of a builtin's (...) after the name, taken off q: their text
    auto takeParens = [&](std::string& inside) {
        size_t k = 0;
        while (k < q.size() && q[k].kind == 'm') k++;
        if (k >= q.size() || q[k].text != "(") return false;
        int d = 0;
        size_t e = k;
        for (; e < q.size(); e++) {
            if (q[e].kind == 'p' && q[e].text == "(") d++;
            else if (q[e].kind == 'p' && q[e].text == ")" && --d == 0) break;
        }
        if (e >= q.size()) return false;
        for (size_t x = k + 1; x < e; x++) inside += q[x].text;
        q.erase(q.begin(), q.begin() + (long)e + 1);
        return true;
    };
    while (!q.empty()) {
        if (--budget < 0) { for (auto& t : q) out.push_back(t); break; }
        PPTok t = std::move(q.front());
        q.pop_front();
        if (t.kind != 'i' || t.hide.count(t.text)) { out.push_back(std::move(t)); continue; }
        const std::string& id = t.text;
        // builtins
        if (id == "__INCLUDE_LEVEL__") { out.push_back(number(t.ws, std::to_string(includeDepth_))); continue; }
        if (id == "__LINE__" && expandLine_ > 0) { out.push_back(number(t.ws, std::to_string(expandLine_))); continue; }
        if (id == "__FILE__" && !expandFile_.empty()) {
            PPTok s; s.ws = t.ws; s.text = "\"" + expandFile_ + "\""; s.kind = 's'; s.expanded = true;
            out.push_back(s);
            continue;
        }
        if (id == "__has_include" || id == "__has_embed" || id == "__has_builtin" || id == "__has_attribute" ||
            id == "_Pragma") {
            std::string inside;
            if (takeParens(inside)) {
                if (id == "_Pragma") {
                    // _Pragma("x"): #pragma x (the string's \" and \\ undone; C11 6.10.9)
                    std::string body = inside;
                    if (!body.empty() && body[0] == 'L') body = body.substr(1);
                    if (body.size() >= 2 && body.front() == '"' && body.back() == '"') body = body.substr(1, body.size() - 2);
                    std::string un;
                    for (size_t k = 0; k < body.size(); k++) {
                        if (body[k] == '\\' && k + 1 < body.size() && (body[k + 1] == '"' || body[k + 1] == '\\')) k++;
                        un += body[k];
                    }
                    pendingPragmas_.push_back(un);
                    continue;
                }
                std::string val;
                if (id == "__has_embed") {
                    // __STDC_EMBED_NOT_FOUND__ 0 (or a parameter not supported),
                    // __STDC_EMBED_FOUND__ 1, __STDC_EMBED_EMPTY__ 2
                    EmbedSpec spec;
                    std::string err, path;
                    std::vector<unsigned char> bytes;
                    if (!parseEmbed(inside, spec, err)) {
                        if (!err.empty()) throw std::runtime_error("__has_embed: " + err);
                        val = "0";
                    } else {
                        val = !readEmbed(spec, bytes, path) ? "0" : bytes.empty() ? "2" : "1";
                    }
                } else if (id == "__has_include") {
                    std::string f = inside;
                    if (f.size() >= 2 && ((f.front() == '"' && f.back() == '"') || (f.front() == '<' && f.back() == '>')))
                        f = f.substr(1, f.size() - 2);
                    val = findIncludeFile(f, getDirectory(currentFile_)).empty() ? "0" : "1";
                } else if (id == "__has_builtin") {
                    static const std::set<std::string> knownBuiltins = {
                        "__builtin_va_list", "__builtin_va_start", "__builtin_va_end", "__builtin_va_arg",
                        "__builtin_constant_p", "__builtin_offsetof", "__builtin_expect", "__builtin_trap", "__builtin_choose_expr",
                        "__builtin_unreachable", "__builtin_conjf",
                        "__builtin_printf", "__builtin_sprintf", "__builtin_puts", "__builtin_putstr", "__builtin_putchar",
                        "__builtin_malloc", "__builtin_free", "__builtin_calloc", "__builtin_realloc",
                        "__builtin_strlen", "__builtin_strcmp", "__builtin_strcpy", "__builtin_strncpy",
                        "__builtin_strcat", "__builtin_strchr", "__builtin_strstr",
                        "__builtin_memcpy", "__builtin_memset", "__builtin_memmove", "__builtin_memcmp",
                        "__builtin_abs", "__builtin_labs", "__builtin_exit", "__builtin_abort",
                        "__builtin_alloca",
                        "__builtin_clz", "__builtin_ctz", "__builtin_ffs", "__builtin_popcount",
                        "__builtin_bswap16",
                    };
                    val = knownBuiltins.count(inside) ? "1" : "0";
                } else {
                    static const std::set<std::string> knownAttrs = {
                        "always_inline", "noinline", "unused", "used", "weak",
                        "pure", "const", "cold", "hot", "packed", "aligned",
                        "deprecated", "naked", "interrupt", "noreturn",
                        "section", "visibility", "constructor", "destructor",
                        "format", "warn_unused_result",
                    };
                    val = knownAttrs.count(inside) ? "1" : "0";
                }
                out.push_back(number(t.ws, val));
                continue;
            }
        }
        auto mi = macros.find(id);
        if (mi == macros.end()) { out.push_back(std::move(t)); continue; }
        const Macro& m = mi->second;
        std::set<std::string> hs = t.hide;
        hs.insert(id);
        std::vector<PPTok> repl;
        if (!m.isFunctionLike) {
            repl = substPP(lexPP(m.body), m, {}, {}, false, hs, depth);
        } else {
            // a call: '(' next (else the name stays a name), the arguments to its ')'
            size_t k = 0;
            while (k < q.size() && q[k].kind == 'm') k++;
            if (k >= q.size() || !(q[k].kind == 'p' && q[k].text == "(")) { out.push_back(std::move(t)); continue; }
            std::vector<std::vector<PPTok>> args(1);
            std::vector<PPTok> va;
            bool inVa = m.isVariadic && m.params.empty();
            int d = 0;
            size_t e = k + 1;
            bool closed = false;
            for (; e < q.size(); e++) {
                const PPTok& x = q[e];
                if (x.kind == 'p' && x.text == "(") d++;
                else if (x.kind == 'p' && x.text == ")") {
                    if (d == 0) { closed = true; break; }
                    d--;
                } else if (x.kind == 'p' && x.text == "," && d == 0 && !inVa) {
                    if (m.isVariadic && args.size() == m.params.size()) { inVa = true; continue; }
                    args.emplace_back();
                    continue;
                }
                if (inVa) va.push_back(x);
                else args.back().push_back(x);
            }
            if (!closed) { out.push_back(std::move(t)); continue; }
            bool haveVa = inVa;
            // F() of a macro without parameters: no argument
            if (m.params.empty() && args.size() == 1 && args[0].empty()) args.clear();
            if (m.isVariadic && m.params.empty()) args.clear();
            // (an argument's leading white space is not part of it)
            for (auto& a : args) if (!a.empty()) a[0].ws.clear();
            if (!va.empty()) va[0].ws.clear();
            if (args.size() != m.params.size()) { out.push_back(std::move(t)); continue; }
            const PPTok& rp = q[e];
            std::set<std::string> h2;
            for (const auto& h : t.hide) if (rp.hide.count(h)) h2.insert(h);
            h2.insert(id);
            q.erase(q.begin(), q.begin() + (long)e + 1);
            std::string body = m.body;
            // __int(8/16/32) / __uint(8/16/32) are the native integers (char,
            // int, long), not wide-integer structs
            if ((id == "__int" || id == "__uint") && args.size() == 1 && body == "struct " + id + "##N") {
                std::string a = renderPP(args[0]);
                const char* sg = id == "__int" ? "signed " : "unsigned ";
                if (a == "8") body = std::string(sg) + "char";
                else if (a == "16") body = std::string(sg) + "int";
                else if (a == "32") body = std::string(sg) + "long";
            }
            repl = substPP(lexPP(body), m, args, va, haveVa, h2, depth);
        }
        // (the replacement in the name's place: its white space)
        if (!repl.empty()) repl[0].ws = t.ws;
        else if (!t.ws.empty() && !q.empty() && q.front().ws.empty()) q.front().ws = t.ws;
        for (auto it = repl.rbegin(); it != repl.rend(); ++it) q.push_front(std::move(*it));
    }
    return out;
}

std::string Preprocessor::expandMacros(const std::string& line) {
    return renderPP(expandPP(lexPP(line)));
}


// Simple recursive descent evaluator for #if expressions
struct ExprPath {
    std::string s;
    size_t pos = 0;
    
    void skipWhitespace() {
        while (pos < s.length() && std::isspace((unsigned char)s[pos])) pos++;
    }
    
    std::string nextToken() {
        skipWhitespace();
        if (pos >= s.length()) return "";
        if (std::isalpha((unsigned char)s[pos]) || s[pos] == '_') {
            size_t start = pos;
            while (pos < s.length() && (std::isalnum((unsigned char)s[pos]) || s[pos] == '_')) pos++;
            return s.substr(start, pos - start);
        }
        if (std::isdigit((unsigned char)s[pos])) {
            // A whole pp-number: 0x7fffffff, 100UL
            size_t start = pos;
            while (pos < s.length() && (std::isalnum((unsigned char)s[pos]) || s[pos] == '_')) pos++;
            return s.substr(start, pos - start);
        }
        if (pos + 1 < s.length()) {
            std::string op2 = s.substr(pos, 2);
            if (op2 == "&&" || op2 == "||" || op2 == "==" || op2 == "!=" || op2 == "<=" || op2 == ">=") {
                pos += 2;
                return op2;
            }
        }
        return std::string(1, s[pos++]);
    }
    
    std::string peekToken() {
        size_t oldPos = pos;
        std::string t = nextToken();
        pos = oldPos;
        return t;
    }
};

long Preprocessor::evaluateExpression(const std::string& expr) {
    // 1. Handle defined(SYMBOL)
    std::string e = expr;
    auto replaceDefined = [&](std::string& s) {
        size_t pos = 0;
        while ((pos = s.find("defined", pos)) != std::string::npos) {
            // Ensure whole word
            bool bBefore = (pos == 0 || (!std::isalnum((unsigned char)s[pos-1]) && s[pos-1] != '_'));
            bool bAfter = (pos + 7 == s.length() || (!std::isalnum((unsigned char)s[pos+7]) && s[pos+7] != '_'));
            if (!bBefore || !bAfter) { pos += 7; continue; }
            
            size_t startPos = pos;
            pos += 7;
            while (pos < s.length() && std::isspace((unsigned char)s[pos])) pos++;
            
            bool hasParen = false;
            if (pos < s.length() && s[pos] == '(') {
                hasParen = true;
                pos++;
                while (pos < s.length() && std::isspace((unsigned char)s[pos])) pos++;
            }
            
            size_t symStart = pos;
            while (pos < s.length() && (std::isalnum((unsigned char)s[pos]) || s[pos] == '_')) pos++;
            std::string sym = s.substr(symStart, pos - symStart);
            
            if (hasParen) {
                while (pos < s.length() && std::isspace((unsigned char)s[pos])) pos++;
                if (pos < s.length() && s[pos] == ')') pos++;
            }
            
            std::string val = (macros.find(sym) != macros.end()) ? "1" : "0";
            s.replace(startPos, pos - startPos, val);
            pos = startPos + val.length();
        }
    };
    replaceDefined(e);

    // 1b. Expand __has_include, __has_builtin, __has_attribute, and macros
    e = expandMacros(e);

    // Replace any remaining undefined identifiers with 0 (C standard: undefined = 0 in #if)
    {
        std::string cleaned;
        for (size_t i = 0; i < e.length(); ) {
            if (std::isdigit((unsigned char)e[i])) {
                // Numbers stay as written (0x7fffffff, 10UL)
                while (i < e.length() && (std::isalnum((unsigned char)e[i]) || e[i] == '_')) cleaned += e[i++];
            } else if (std::isalpha((unsigned char)e[i]) || e[i] == '_') {
                size_t start = i;
                while (i < e.length() && (std::isalnum((unsigned char)e[i]) || e[i] == '_')) i++;
                std::string ident = e.substr(start, i - start);
                // Unknown identifiers in #if evaluate to 0 per C standard
                cleaned += "0";
            } else {
                cleaned += e[i++];
            }
        }
        e = cleaned;
    }

    // 2. Simple Recursive Descent (Logical OR -> AND -> Relational -> Additive -> Multiplicative -> Unary -> Primary)
    ExprPath p{e};
    
    std::function<long()> parseOr;
    std::function<long()> parseAnd;
    std::function<long()> parseRel;
    std::function<long()> parseAdd;
    std::function<long()> parseMul;
    std::function<long()> parseUnary;
    std::function<long()> parsePrimary;

    parsePrimary = [&]() -> long {
        std::string t = p.nextToken();
        if (t == "(") {
            long val = parseOr();
            if (p.nextToken() != ")") {} // Error
            return val;
        }
        if (t.empty()) return 0;
        if (isdigit((unsigned char)t[0])) {
            // Integer constant: decimal, octal or hex, U/L suffixes ignored
            std::string d = t;
            while (!d.empty() && std::strchr("uUlL", d.back())) d.pop_back();
            try { return (long)std::stoull(d, nullptr, 0); } catch (...) { return 0; }
        }
        // Symbols not in defined() but in #if are treated as 0
        if (macros.count(t)) {
            try { return std::stol(macros[t].body, nullptr, 0); } catch(...) { return 0; }
        }
        return 0;
    };

    parseUnary = [&]() -> long {
        std::string t = p.peekToken();
        if (t == "!") { p.nextToken(); return !parseUnary(); }
        if (t == "-") { p.nextToken(); return -parseUnary(); }
        if (t == "+") { p.nextToken(); return parseUnary(); }
        return parsePrimary();
    };

    parseMul = [&]() -> long {
        long left = parseUnary();
        while (true) {
            std::string t = p.peekToken();
            if (t == "*") { p.nextToken(); left *= parseUnary(); }
            else if (t == "/") { p.nextToken(); long right = parseUnary(); left = right ? left / right : 0; }
            else break;
        }
        return left;
    };

    parseAdd = [&]() -> long {
        long left = parseMul();
        while (true) {
            std::string t = p.peekToken();
            if (t == "+") { p.nextToken(); left += parseMul(); }
            else if (t == "-") { p.nextToken(); left -= parseMul(); }
            else break;
        }
        return left;
    };

    parseRel = [&]() -> long {
        long left = parseAdd();
        while (true) {
            std::string t = p.peekToken();
            if (t == "==") { p.nextToken(); left = (left == parseAdd()); }
            else if (t == "!=") { p.nextToken(); left = (left != parseAdd()); }
            else if (t == "<") { p.nextToken(); left = (left < parseAdd()); }
            else if (t == ">") { p.nextToken(); left = (left > parseAdd()); }
            else if (t == "<=") { p.nextToken(); left = (left <= parseAdd()); }
            else if (t == ">=") { p.nextToken(); left = (left >= parseAdd()); }
            else break;
        }
        return left;
    };

    parseAnd = [&]() -> long {
        long left = parseRel();
        while (p.peekToken() == "&&") { p.nextToken(); left = (parseRel() && left); }
        return left;
    };

    parseOr = [&]() -> long {
        long left = parseAnd();
        while (p.peekToken() == "||") { p.nextToken(); left = (parseAnd() || left); }
        return left;
    };

    return parseOr();
}

// The source with comments and string/character literals blanked
static std::string stripCommentsAndStrings(const std::string& s) {
    std::string r(s);
    for (size_t i = 0; i < r.size(); i++) {
        if (r[i] == '/' && i + 1 < r.size() && r[i + 1] == '/') {
            while (i < r.size() && r[i] != '\n') r[i++] = ' ';
        } else if (r[i] == '/' && i + 1 < r.size() && r[i + 1] == '*') {
            r[i] = r[i + 1] = ' ';
            for (i += 2; i < r.size() && !(r[i] == '*' && i + 1 < r.size() && r[i + 1] == '/'); i++)
                if (r[i] != '\n') r[i] = ' ';
            if (i < r.size()) { r[i] = ' '; if (i + 1 < r.size()) r[++i] = ' '; }
        } else if (r[i] == '"' || r[i] == '\'') {
            char q = r[i];
            for (i++; i < r.size() && r[i] != q && r[i] != '\n'; i++) {
                if (r[i] == '\\' && i + 1 < r.size()) r[i++] = ' ';
                r[i] = ' ';
            }
        }
    }
    return r;
}

// #pragma cc45 optimize(a, no-b) / optimize a,no-b: the directive for
// IRBuilder (".optimize a,no-b"), or "" when the list is empty. `first` is the
// word that starts with "optimize"; the rest of the line comes from `in`.
static std::string optimizeDirective(const std::string& first, std::istream& in) {
    std::string rest = first.substr(8), tail;
    while (in >> tail) rest += " " + tail;
    std::string list;
    for (char ch : rest) {
        if (std::isalnum((unsigned char)ch) || ch == '-' || ch == '_') list += ch;
        else if ((ch == ',' || ch == ' ') && !list.empty() && list.back() != ',') list += ',';
    }
    while (!list.empty() && list.back() == ',') list.pop_back();
    return list.empty() ? std::string() : "__asm__(\".optimize " + list + "\");\n";
}

// Does the source spell "long long" (struct __int64)?
static bool hasLongLong(const std::string& s) {
    for (size_t p = s.find("long"); p != std::string::npos; p = s.find("long", p + 4)) {
        if (p > 0 && (std::isalnum((unsigned char)s[p - 1]) || s[p - 1] == '_')) continue;
        size_t q = p + 4;
        if (q >= s.size() || !std::isspace((unsigned char)s[q])) continue;
        while (q < s.size() && std::isspace((unsigned char)s[q])) q++;
        if (s.compare(q, 4, "long") == 0 && (q + 4 >= s.size() || !(std::isalnum((unsigned char)s[q + 4]) || s[q + 4] == '_')))
            return true;
    }
    return false;
}

// The same definition by C11 6.10.3p2: both object-like or both
// function-like with the same parameters (names too), and replacement
// lists the same but for the amount of white space between tokens
bool Preprocessor::sameDefinition(const Macro& a, const Macro& b) {
    if (a.isFunctionLike != b.isFunctionLike || a.isVariadic != b.isVariadic || a.params != b.params) return false;
    auto norm = [](const std::string& s) {
        std::string r;
        bool space = false;
        for (char c : s) {
            if (std::isspace((unsigned char)c)) { space = !r.empty(); continue; }
            if (space) { r += ' '; space = false; }
            r += c;
        }
        return r;
    };
    return norm(a.body) == norm(b.body);
}

void Preprocessor::defineMacro(const std::string& name, Macro m, const std::string& file, int line) {
    m.defFile = file;
    m.defLine = line;
    auto it = macros.find(name);
    if (it != macros.end() && !sameDefinition(it->second, m)) {
        std::cerr << file << ":" << line << ": warning: macro '" << name << "' redefined differently" << std::endl;
        const Macro& old = it->second;
        if (old.defFile.empty())
            std::cerr << file << ":" << line << ": note: '" << name << "' is predefined (as '" << old.body << "')" << std::endl;
        else if (old.defLine == 0)
            std::cerr << file << ":" << line << ": note: '" << name << "' was defined on the command line (as '" << old.body << "')" << std::endl;
        else
            std::cerr << old.defFile << ":" << old.defLine << ": note: the previous definition of '" << name << "'" << std::endl;
    }
    macros[name] = std::move(m);
}

// An LL constant a signed 32-bit long cannot hold (4102790424LL): a long
// long, so the unit needs <intwide.h>
static bool hasWideLLLiteral(const std::string& code) {
    for (size_t i = 0; i < code.size(); i++) {
        if (!std::isdigit((unsigned char)code[i]) || (i > 0 && (std::isalnum((unsigned char)code[i - 1]) || code[i - 1] == '_' || code[i - 1] == '.')))
            continue;
        size_t j = i;
        bool hex = code[i] == '0' && i + 1 < code.size() && (code[i + 1] == 'x' || code[i + 1] == 'X');
        if (hex) j += 2;
        while (j < code.size() && (hex ? std::isxdigit((unsigned char)code[j]) : std::isdigit((unsigned char)code[j]))) j++;
        size_t k = j;
        int ls = 0;
        while (k < code.size() && std::strchr("lLuU", code[k])) { if (code[k] == 'l' || code[k] == 'L') ls++; k++; }
        if (ls >= 2) {
            unsigned long long v = 0;
            try { v = std::stoull(code.substr(i, j - i), nullptr, 0); } catch (...) { v = 0; }
            if (v > 0x7FFFFFFFULL) return true;
        }
        i = k;
    }
    return false;
}

std::string Preprocessor::process(const std::string& source,
                                  const std::map<std::string, std::string>& initialSymbols,
                                  const std::vector<std::string>& includePaths,
                                  const std::string& currentFile) {
    // Preserve predefined macros from constructor, then add initialSymbols
    std::map<std::string, Macro> saved = std::move(this->macros);
    this->macros.clear();
    // Restore predefined macros
    for (auto& [k, v] : saved) this->macros[k] = std::move(v);
    // Add/override with initialSymbols
    for (const auto& pair : initialSymbols) {
        Macro m{false, false, {}, pair.second};
        m.defFile = "<command line>";
        this->macros[pair.first] = m;
    }
    this->includePaths = includePaths;
    this->includedFiles.clear();
    this->onceFiles.clear();
    this->stateStack.clear();
    
    // Default initial state: everything is active.
    stateStack.push_back({true, true, false});
    
    // __int128 / __int(N) / __uint(N) / int64_t are the structs of
    // <intwide.h>: a unit that uses them without the header gets it (as
    // _Complex gets its types)
    // (the code only: a comment or string that names them does not count)
    std::string code = isCompiler ? stripCommentsAndStrings(source) : std::string();
    // (an #include of it in the code, not a comment naming it: the header's
    // guard keeps a quoted one from counting twice)
    std::string prefix;
    if (isCompiler && code.find("intwide.h") == std::string::npos &&
        (code.find("__int128") != std::string::npos || namesWideIntN(code) || code.find("int64_t") != std::string::npos ||
         code.find("__int64") != std::string::npos || hasLongLong(code) || hasWideLLLiteral(code)) &&
        !findIncludeFile("intwide.h", getDirectory(currentFile)).empty())
        prefix += "#include <intwide.h>\n";
    // __fixed8_8 / __fixed16_16 (#72): <fixed.h>, their routines and helpers
    if (isCompiler && code.find("fixed.h") == std::string::npos && code.find("__fixed") != std::string::npos &&
        !findIncludeFile("fixed.h", getDirectory(currentFile)).empty())
        prefix += "#include <fixed.h>\n";
    if (!prefix.empty())
        return processInternal(prefix + "#line 1 \"" + currentFile + "\"\n" + source, currentFile, 0);
    return processInternal(source, currentFile, 0);
}

bool Preprocessor::isConditionTrue() {
    for (const auto& s : stateStack) {
        if (!s.active) return false;
    }
    return true;
}

std::string Preprocessor::trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string Preprocessor::getDirectory(const std::string& filePath) {
    return fs::path(filePath).parent_path().string();
}

std::string Preprocessor::findIncludeFile(const std::string& fileName, const std::string& currentDir) {
    // 1. Check current directory
    // (a directory of that name is not the header)
    fs::path p1 = fs::path(currentDir) / fileName;
    if (fs::exists(p1) && !fs::is_directory(p1)) return p1.string();
    
    // 2. Check include paths
    for (const auto& dir : includePaths) {
        fs::path p = fs::path(dir) / fileName;
        if (fs::exists(p) && !fs::is_directory(p)) return p.string();
    }
    
    return "";
}

bool Preprocessor::parseEmbed(const std::string& text, EmbedSpec& spec, std::string& err) {
    std::string t = trim(text);
    if (t.empty() || (t[0] != '"' && t[0] != '<')) { err = "expected \"file\" or <file>"; return false; }
    char close = t[0] == '"' ? '"' : '>';
    size_t e = t.find(close, 1);
    if (e == std::string::npos) { err = std::string("missing closing ") + close; return false; }
    spec.file = t.substr(1, e - 1);
    if (spec.file.empty()) { err = "empty file name"; return false; }
    size_t i = e + 1;
    auto skipWs = [&] { while (i < t.size() && std::isspace((unsigned char)t[i])) i++; };
    for (skipWs(); i < t.size(); skipWs()) {
        // a parameter: name or prefix::name, then (balanced tokens)
        size_t b = i;
        while (i < t.size() && (std::isalnum((unsigned char)t[i]) || t[i] == '_' || t[i] == ':')) i++;
        std::string name = t.substr(b, i - b);
        if (name.empty()) { err = "unexpected '" + t.substr(i, 1) + "'"; return false; }
        skipWs();
        std::string arg;
        bool hasArg = false;
        if (i < t.size() && t[i] == '(') {
            size_t ab = i + 1;
            int depth = 0;
            for (; i < t.size(); i++) {
                char c = t[i];
                if (c == '"' || c == '\'') {
                    for (i++; i < t.size() && t[i] != c; i++) if (t[i] == '\\') i++;
                } else if (c == '(') {
                    depth++;
                } else if (c == ')' && --depth == 0) {
                    break;
                }
            }
            if (i >= t.size()) { err = "missing ')' after " + name; return false; }
            arg = trim(t.substr(ab, i - ab));
            i++;
            hasArg = true;
        }
        // (__limit__ is limit, cc45::__offset__ is cc45::offset: C23 6.10.4.1)
        std::string vendor, n = name;
        size_t cc = n.rfind("::");
        if (cc != std::string::npos) { vendor = n.substr(0, cc); n = n.substr(cc + 2); }
        if (n.size() > 4 && n.compare(0, 2, "__") == 0 && n.compare(n.size() - 2, 2, "__") == 0)
            n = n.substr(2, n.size() - 4);
        auto number = [&](long& v) {
            if (!hasArg || arg.empty()) { err = name + " needs a value: " + name + "(N)"; return false; }
            v = evaluateExpression(arg);
            if (v < 0) { err = name + "(" + arg + ") is negative"; return false; }
            return true;
        };
        if (vendor.empty() && n == "limit") {
            if (!number(spec.limit)) return false;
        } else if ((vendor == "cc45" || vendor == "clang" || vendor == "gnu") && n == "offset") {
            if (!number(spec.offset)) return false;
        } else if (vendor.empty() && (n == "prefix" || n == "suffix" || n == "if_empty")) {
            if (!hasArg) { err = name + " needs its tokens in ( )"; return false; }
            if (n == "prefix") { spec.hasPrefix = true; spec.prefix = arg; }
            else if (n == "suffix") { spec.hasSuffix = true; spec.suffix = arg; }
            else { spec.hasIfEmpty = true; spec.ifEmpty = arg; }
        } else {
            err.clear();   // (an unknown parameter: not supported)
            spec.file = name;
            return false;
        }
    }
    return true;
}

bool Preprocessor::readEmbed(const EmbedSpec& spec, std::vector<unsigned char>& bytes, std::string& path) {
    path = findIncludeFile(spec.file, getDirectory(currentFile_));
    if (path.empty()) return false;
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    bytes.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    size_t off = std::min(bytes.size(), (size_t)spec.offset);
    bytes.erase(bytes.begin(), bytes.begin() + (long)off);
    if (spec.limit >= 0 && bytes.size() > (size_t)spec.limit) bytes.resize((size_t)spec.limit);
    return true;
}

std::string Preprocessor::embedList(const std::vector<unsigned char>& bytes, int width) {
    std::string out;
    size_t perLine = 16 / (size_t)width, n = 0;
    char buf[16];
    for (size_t i = 0; i < bytes.size(); i += (size_t)width) {
        unsigned long v = 0;
        for (int k = width - 1; k >= 0; k--)
            v = (v << 8) | (i + (size_t)k < bytes.size() ? bytes[i + (size_t)k] : 0);
        std::snprintf(buf, sizeof buf, "0x%0*lx", width * 2, v);
        if (n) out += n % perLine ? ", " : ",\n";
        out += buf;
        n++;
    }
    return out;
}

std::string Preprocessor::stripComments(const std::string& line, bool& inBlockComment) {
    std::string result;
    bool inString = false;
    bool inChar = false;
    
    for (size_t i = 0; i < line.length(); ++i) {
        if (inBlockComment) {
            if (i + 1 < line.length() && line[i] == '*' && line[i+1] == '/') {
                inBlockComment = false;
                i++;
                result += ' ';
            }
            continue;
        }
        if (inString) {
            result += line[i];
            if (line[i] == '\\' && i + 1 < line.length()) {
                result += line[i+1];
                i++;
            } else if (line[i] == '"') {
                inString = false;
            }
            continue;
        }
        if (inChar) {
            result += line[i];
            if (line[i] == '\\' && i + 1 < line.length()) {
                result += line[i+1];
                i++;
            } else if (line[i] == '\'') {
                inChar = false;
            }
            continue;
        }
        
        if (i + 1 < line.length() && line[i] == '/' && line[i+1] == '*') {
            inBlockComment = true;
            i++;
            result += ' ';
        } else if (i + 1 < line.length() && line[i] == '/' && line[i+1] == '/') {
            result += ' ';
            break;
        } else if (line[i] == '"') {
            inString = true;
            result += '"';
        } else if (line[i] == '\'') {
            inChar = true;
            result += '\'';
        } else {
            result += line[i];
        }
    }
    return result;
}

// A #pragma's text (after "#pragma"), from a #pragma line or _Pragma("..."):
// cc45's pragmas become __asm__(".directive") lines on output (else an empty
// line), others act here (once, push_macro / pop_macro)
void Preprocessor::handlePragma(std::istream& ss, std::ostream& output, const std::string& currentFile) {
    std::string pragmaArg;
    ss >> pragmaArg;
    if (pragmaArg.rfind("push_macro", 0) == 0 || pragmaArg.rfind("pop_macro", 0) == 0) {
        // #pragma push_macro("NAME") / pop_macro("NAME")
        std::string rest;
        std::getline(ss, rest);
        std::string all = pragmaArg + rest;
        size_t q1 = all.find('"'), q2 = q1 == std::string::npos ? q1 : all.find('"', q1 + 1);
        if (isConditionTrue() && q2 != std::string::npos) {
            std::string mname = all.substr(q1 + 1, q2 - q1 - 1);
            auto& stack = pushedMacros_[mname];
            if (pragmaArg[1] == 'u') {
                auto it = macros.find(mname);
                stack.push_back(it != macros.end() ? std::make_pair(true, it->second)
                                                   : std::make_pair(false, Macro{}));
            } else if (!stack.empty()) {
                if (stack.back().first) macros[mname] = stack.back().second;
                else macros.erase(mname);
                stack.pop_back();
            }
        }
        output << "\n";
    } else if (pragmaArg == "once") {
        if (!currentFile.empty()) onceFiles.insert(currentFile);
        output << "\n";
    } else if (isCompiler && pragmaArg.rfind("section", 0) == 0 &&
               (pragmaArg.size() == 7 || pragmaArg[7] == '(')) {
        std::string rest = pragmaArg.substr(7), tail;
        while (std::getline(ss, tail)) rest += tail;
        output << sectionDirective(rest) << "\n";
    } else if (isCompiler && pragmaArg == "weak") {
        output << "__asm__(\".weak_next\");\n";
    } else if (isCompiler && pragmaArg == "no_zp_save") {
        output << "__asm__(\".no_zp_save\");\n";
    } else if (isCompiler && pragmaArg == "cc45") {
        std::string cc45Arg;
        ss >> cc45Arg;
        if (cc45Arg == "no_0100_stack") {
            output << "__asm__(\".crt_no_0100_stack\");\n";
        } else if (cc45Arg == "exit_halt") {
            output << "__asm__(\".crt_exit_halt\");\n";
        } else if (cc45Arg == "exit_rts") {
            output << "__asm__(\".crt_exit_rts\");\n";
        } else if (cc45Arg == "exit_brk") {
            output << "__asm__(\".crt_exit_brk\");\n";
        } else if (cc45Arg == "no_bssinit") {
            output << "__asm__(\".crt_no_bssinit\");\n";
        } else if (cc45Arg == "heap") {
            output << "__asm__(\".crt_heap\");\n";
        } else if (cc45Arg == "stdio") {
            output << "__asm__(\".crt_stdio\");\n";
        } else if (cc45Arg == "no_zp_save") {
            output << "__asm__(\".no_zp_save\");\n";
        } else if (cc45Arg == "no_stack16") {
            output << "__asm__(\".no_stack16\");\n";
        } else if (cc45Arg.rfind("section", 0) == 0 && (cc45Arg.size() == 7 || cc45Arg[7] == '(')) {
            std::string rest = cc45Arg.substr(7), tail;
            while (std::getline(ss, tail)) rest += tail;
            output << sectionDirective(rest) << "\n";
        } else if (cc45Arg.rfind("overlay", 0) == 0) {
            // "overlay(name)" / "overlay name": this file's code is in
            // the code overlay name (ln45 loads it on demand)
            std::string rest = cc45Arg.substr(7), tail;
            while (ss >> tail) rest += tail;
            std::string n;
            for (char ch : rest) if (std::isalnum((unsigned char)ch) || ch == '_') n += ch;
            output << (n.empty() ? std::string("\n") : "__asm__(\".overlay " + n + "\");\n");
        } else if (cc45Arg.rfind("stack_size", 0) == 0) {
            // Accepts "stack_size N" and "stack_size(N)"
            std::string rest = cc45Arg.substr(10), tail;
            while (ss >> tail) rest += tail;
            std::string n;
            for (char ch : rest) if (std::isxdigit((unsigned char)ch) || ch == '$' || ch == 'x' || ch == 'X') n += ch;
            output << (n.empty() ? std::string("\n") : "__asm__(\".stack_size " + n + "\");\n");
        } else if (cc45Arg.rfind("stack_top", 0) == 0) {
            // Accepts "stack_top ADDR" and "stack_top(ADDR)"
            std::string rest = cc45Arg.substr(9), tail;
            while (ss >> tail) rest += tail;
            std::string addr;
            for (char ch : rest) if (std::isxdigit((unsigned char)ch) || ch == '$' || ch == 'x' || ch == 'X') addr += ch;
            output << (addr.empty() ? std::string("\n") : "__asm__(\".stack_top " + addr + "\");\n");
        } else if (cc45Arg == "set_bp") {
            std::string bpVal;
            ss >> bpVal;
            output << "__asm__(\".set_bp " + bpVal + "\");\n";
        } else if (cc45Arg == "weak") {
            output << "__asm__(\".weak_next\");\n";
        } else if (cc45Arg == "recurse") {
            output << "__asm__(\".recurse_next\");\n";
        } else if (cc45Arg == "unroll") {
            std::string countStr;
            if (ss >> countStr) {
                output << "asm(\".crt_unroll " + countStr + "\");\n";
            } else {
                output << "asm(\".crt_unroll\");\n";
            }
        } else if (cc45Arg == "unroll_default") {
            std::string countStr;
            if (ss >> countStr) {
                output << "asm(\".crt_unroll_def " + countStr + "\");\n";
            } else {
                output << "\n";
            }
        } else if (cc45Arg == "noescape_strings" || cc45Arg == "escape_strings") {
            output << "__asm__(\"." + cc45Arg + "\");\n";
        } else if (cc45Arg == "signed_default") {
            output << "__asm__(\".signed_default\");\n";
        } else if (cc45Arg == "unsigned_default") {
            output << "__asm__(\".unsigned_default\");\n";
        } else if (cc45Arg.rfind("max_unroll_bytes", 0) == 0) {
            // Accepts "max_unroll_bytes N" and "max_unroll_bytes(N)"
            std::string rest = cc45Arg.substr(16), tail;
            while (ss >> tail) rest += tail;
            std::string digits;
            for (char ch : rest) if (std::isdigit((unsigned char)ch)) digits += ch;
            if (!digits.empty())
                output << "__asm__(\".max_unroll_bytes " + digits + "\");\n";
            else
                output << "\n";
        } else if (cc45Arg.rfind("optimize", 0) == 0) {
            std::string d = optimizeDirective(cc45Arg, ss);
            output << (d.empty() ? std::string("\n") : d);
        } else if (cc45Arg.rfind("max_inline_ir_size", 0) == 0) {
            // Accepts "max_inline_ir_size N" and "max_inline_ir_size(N)"
            std::string rest = cc45Arg.substr(18), tail;
            while (ss >> tail) rest += tail;
            std::string digits;
            for (char ch : rest) if (std::isdigit((unsigned char)ch)) digits += ch;
            if (!digits.empty())
                output << "__asm__(\".max_inline_ir_size " + digits + "\");\n";
            else
                output << "\n";
        } else {
            output << "\n";
        }
    } else if (isCompiler && pragmaArg.substr(0, 9) == "encoding(") {
        std::string enc = pragmaArg.substr(9);
        if (!enc.empty() && enc.back() == ')') enc.pop_back();
        if (enc == "ascii" || enc == "petscii" || enc == "screencode")
            output << "__asm__(\".encoding " + enc + "\");\n";
        else output << "\n";
    } else if (isCompiler && pragmaArg == "encoding") {
        std::string enc;
        ss >> enc;
        if (enc == "ascii" || enc == "petscii" || enc == "screencode")
            output << "__asm__(\".encoding " + enc + "\");\n";
        else output << "\n";
    } else {
        output << "\n";
    }
}

// #pragma section("NAME"[, address]) (also #pragma cc45 section): declares
// a section variables can be placed in with __section("NAME") (Issue #70);
// without an address ln45 --section NAME=address gives it. The parser
// reads the directive: .cc45_section NAME [address]
std::string Preprocessor::sectionDirective(const std::string& args) {
    auto fail = [&](const std::string& why) -> std::string {
        throw std::runtime_error(currentFile_ + ":" + std::to_string(pragmaLine_) + ": error: #pragma section: " + why +
                                 " (#pragma section(\"NAME\", address))");
    };
    std::string a = args;
    auto trim = [](std::string& t) {
        t.erase(0, t.find_first_not_of(" \t"));
        t.erase(t.find_last_not_of(" \t\r") + 1);
    };
    trim(a);
    if (a.size() < 2 || a.front() != '(' || a.back() != ')') return fail("expected (\"NAME\"[, address])");
    a = a.substr(1, a.size() - 2);
    trim(a);
    if (a.empty() || a[0] != '"') return fail("the name is a string");
    size_t q = a.find('"', 1);
    if (q == std::string::npos) return fail("the name is a string");
    std::string name = a.substr(1, q - 1);
    bool ok = !name.empty() && !std::isdigit((unsigned char)name[0]);
    for (char c : name) ok = ok && (std::isalnum((unsigned char)c) || c == '_');
    if (!ok) return fail("'" + name + "' is not a name (letters, digits and _)");
    std::string addr = a.substr(q + 1);
    trim(addr);
    std::string out = "__asm__(\".cc45_section " + name;
    if (!addr.empty()) {
        if (addr[0] != ',') return fail("expected ',' after the name");
        addr = addr.substr(1);
        trim(addr);
        unsigned long v = 0;
        try {
            size_t used = 0;
            if (!addr.empty() && addr[0] == '$') { v = std::stoul(addr.substr(1), &used, 16); used++; }
            else v = std::stoul(addr, &used, 0);
            if (used != addr.size()) throw std::invalid_argument(addr);
        } catch (...) {
            return fail("the address '" + addr + "' is not a number");
        }
        if (v > 0xFFFF) return fail("the address '" + addr + "' is beyond $FFFF (variables are reached with 16-bit addresses)");
        out += " " + std::to_string(v);
    }
    return out + "\");";
}

std::string Preprocessor::processInternal(const std::string& source, const std::string& currentFile, int depth) {
    if (depth > 16) {
        throw std::runtime_error("Maximum include depth exceeded (circular dependency?)");
    }
    includeDepth_ = depth;
    currentFile_ = currentFile;

    if (!currentFile.empty() && onceFiles.count(currentFile)) {
        return "";
    }

    std::stringstream input(source);
    std::stringstream output;
    std::string line;
    int lineNum = 0;
    std::string reportedFile = currentFile;
    std::string accumulatedLine = "";
    int joinedLines = 0;   // lines of an open macro call joined so far
    bool inBlockComment = false;
    bool hasSeenNonWhitespace = false;

    while (std::getline(input, line)) {
        lineNum++;
        
        // Handle line continuation (Phase 2)
        std::string trimmedRaw = trim(line);
        if (!trimmedRaw.empty() && trimmedRaw.back() == '\\') {
            accumulatedLine += line.substr(0, line.find_last_of('\\'));
            output << "\n"; // Maintain line counts
            continue;
        }
        
        std::string currentLineRaw = accumulatedLine + line;
        accumulatedLine = "";
        
        // Strip comments (Phase 3)
        std::string currentLine = stripComments(currentLineRaw, inBlockComment);
        std::string trimmed = trim(currentLine);
        // "# define X": white space may separate '#' from the directive name
        if (trimmed.size() > 1 && trimmed[0] == '#' && std::isspace((unsigned char)trimmed[1])) {
            size_t k = 1;
            while (k < trimmed.size() && std::isspace((unsigned char)trimmed[k])) k++;
            trimmed = "#" + trimmed.substr(k);
        }

        if (!hasSeenNonWhitespace && !trimmed.empty()) {
            if (trimmed.substr(0, 1) != "#") {
                hasSeenNonWhitespace = true;
            } else {
                // Check if it's #pragma once
                std::stringstream ss(trimmed);
                std::string hash, cmd, sub;
                ss >> cmd; // cmd will be #pragma or #something
                if (cmd == "#pragma") {
                    ss >> sub;
                    if (sub == "once") {
                        if (!currentFile.empty()) onceFiles.insert(currentFile);
                        output << "\n";
                        continue;
                    }
                }
                // Other directives don't count as non-whitespace content for #pragma once purposes
                // but we should still handle them.
            }
        }
        
        // Handle compiler pass-through #!
        if (isCompiler && trimmed.substr(0, 2) == "#!") {
            if (isConditionTrue()) {
                std::string directive = "#" + trimmed.substr(2);
                output << "__asm__(\"" << directive << "\");\n";
            } else {
                output << "\n"; // Maintain line numbers
            }
            continue;
        }

        if (!trimmed.empty() && trimmed[0] == '#') {
            std::stringstream ss(trimmed);
            std::string cmd;
            ss >> cmd;

            // Handle #if( without space: split #if from the expression
            if (cmd.length() > 3 && cmd.substr(0, 3) == "#if" && cmd[3] == '(') {
                std::string rest = cmd.substr(3);
                std::string remaining;
                std::getline(ss, remaining);
                ss.clear();
                ss.str(rest + remaining);
                cmd = "#if";
            }

            if (cmd == "#define") {
                if (isConditionTrue()) {
                    std::string lineRest;
                    std::getline(ss, lineRest);
                    lineRest = trim(lineRest);
                    
                    std::string name;
                    size_t i = 0;
                    while (i < lineRest.length() && (std::isalnum((unsigned char)lineRest[i]) || lineRest[i] == '_')) {
                        name += lineRest[i++];
                    }
                    
                    if (i < lineRest.length() && lineRest[i] == '(') {
                        // Function-like
                        size_t endParen = lineRest.find(')', i);
                        std::string paramStr = lineRest.substr(i + 1, endParen - i - 1);
                        std::string body = lineRest.substr(endParen + 1);
                        
                        Macro m;
                        m.isFunctionLike = true;
                        m.body = trim(body);
                        
                        std::stringstream pss(paramStr);
                        std::string p;
                        while (std::getline(pss, p, ',')) {
                            p = trim(p);
                            if (p == "...") {
                                m.isVariadic = true;
                            } else if (p.size() > 3 && p.compare(p.size() - 3, 3, "...") == 0) {
                                // GNU named variadic parameter (args...): the
                                // body's uses of the name become __VA_ARGS__
                                std::string vn = trim(p.substr(0, p.size() - 3));
                                m.isVariadic = true;
                                std::string nb;
                                for (size_t k = 0; k < m.body.size();) {
                                    bool startOk = k == 0 || !(std::isalnum((unsigned char)m.body[k - 1]) || m.body[k - 1] == '_');
                                    size_t e = k + vn.size();
                                    if (startOk && m.body.compare(k, vn.size(), vn) == 0 &&
                                        (e >= m.body.size() || !(std::isalnum((unsigned char)m.body[e]) || m.body[e] == '_'))) {
                                        nb += "__VA_ARGS__";
                                        k = e;
                                    } else {
                                        nb += m.body[k++];
                                    }
                                }
                                m.body = nb;
                            } else {
                                m.params.push_back(p);
                            }
                        }
                        defineMacro(name, m, reportedFile, lineNum);
                    } else {
                        // Object-like
                        std::string body = lineRest.substr(name.length());
                        defineMacro(name, Macro{false, false, {}, trim(body)}, reportedFile, lineNum);
                    }
                }
                output << "\n";
            } else if (cmd == "#undef") {
                if (isConditionTrue()) {
                    std::string name;
                    ss >> name;
                    macros.erase(name);
                }
                output << "\n";
            } else if (cmd == "#ifdef") {
                std::string name;
                ss >> name;
                bool exists = (macros.find(name) != macros.end());
                bool parentActive = isConditionTrue();
                stateStack.push_back({parentActive && exists, exists, false});
                output << "\n";
            } else if (cmd == "#ifndef") {
                std::string name;
                ss >> name;
                bool exists = (macros.find(name) != macros.end());
                bool parentActive = isConditionTrue();
                stateStack.push_back({parentActive && !exists, !exists, false});
                output << "\n";
            } else if (cmd == "#if") {
                std::string expr;
                std::getline(ss, expr);
                bool val = evaluateExpression(expr);
                bool parentActive = isConditionTrue();
                stateStack.push_back({parentActive && val, val, false});
                output << "\n";
            } else if (cmd == "#elif") {
                if (stateStack.size() <= 1) throw std::runtime_error("Unexpected #elif");
                State& top = stateStack.back();
                if (top.inElse) throw std::runtime_error("Unexpected #elif after #else");
                
                bool parentActive = true;
                for (size_t i = 0; i < stateStack.size() - 1; ++i) {
                    if (!stateStack[i].active) { parentActive = false; break; }
                }

                std::string expr;
                std::getline(ss, expr);
                bool val = evaluateExpression(expr);

                if (!top.hasSeenTrueBranch && parentActive && val) {
                    top.active = true;
                    top.hasSeenTrueBranch = true;
                } else {
                    top.active = false;
                }
                output << "\n";
            } else if (cmd == "#else") {
                if (stateStack.size() <= 1) throw std::runtime_error("Unexpected #else");
                State& top = stateStack.back();
                if (top.inElse) throw std::runtime_error("Multiple #else for one #ifdef");
                
                bool parentActive = true;
                for (size_t i = 0; i < stateStack.size() - 1; ++i) {
                    if (!stateStack[i].active) { parentActive = false; break; }
                }

                top.active = parentActive && !top.hasSeenTrueBranch;
                top.inElse = true;
                output << "\n";
            } else if (cmd == "#endif") {
                if (stateStack.size() <= 1) throw std::runtime_error("Unexpected #endif");
                stateStack.pop_back();
                output << "\n";
            } else if (cmd == "#embed" || (cmd == "#include" && isConditionTrue() && [&] {
                           // #include binary [char|word|long] "file"
                           std::string w;
                           auto at = ss.tellg();
                           bool bin = (ss >> w) && w == "binary";
                           ss.clear();
                           ss.seekg(at);
                           return bin;
                       }())) {
                if (isConditionTrue()) {
                    std::string where = reportedFile + ":" + std::to_string(lineNum) + ": ";
                    std::string lineRest;
                    std::getline(ss, lineRest);
                    lineRest = trim(lineRest);
                    int width = 1;
                    if (cmd == "#include") {
                        std::istringstream ws(lineRest);
                        std::string w, kind;
                        ws >> w;   // binary
                        size_t at = lineRest.find("binary") + 6;
                        ws >> kind;
                        if (kind == "char" || kind == "word" || kind == "long") {
                            width = kind == "char" ? 1 : kind == "word" ? 2 : 4;
                            at = lineRest.find(kind, at) + kind.size();
                        }
                        lineRest = trim(lineRest.substr(at));
                    }
                    if (!lineRest.empty() && lineRest[0] != '"' && lineRest[0] != '<') lineRest = trim(expandMacros(lineRest));
                    EmbedSpec spec;
                    std::string err, path;
                    std::vector<unsigned char> bytes;
                    if (!parseEmbed(lineRest, spec, err)) {
                        if (err.empty()) throw std::runtime_error(where + cmd + ": unknown parameter '" + spec.file + "'");
                        throw std::runtime_error(where + cmd + ": " + err);
                    }
                    if (cmd == "#include" && (spec.limit >= 0 || spec.offset || spec.hasPrefix || spec.hasSuffix || spec.hasIfEmpty))
                        throw std::runtime_error(where + "#include binary takes no parameters (use #embed)");
                    if (!readEmbed(spec, bytes, path)) throw std::runtime_error(where + "Could not find embed file: " + spec.file);
                    if (bytes.size() % (size_t)width)
                        std::cerr << where << "warning: " << path << ": " << bytes.size() << " bytes is not a whole number of "
                                  << (width == 2 ? "words" : "longs") << "; the last one is padded with zero bytes\n";
                    std::string text;
                    if (bytes.empty()) {
                        if (spec.hasIfEmpty) text = expandMacros(spec.ifEmpty);
                    } else {
                        if (spec.hasPrefix) text = expandMacros(spec.prefix) + " ";
                        text += embedList(bytes, width);
                        if (spec.hasSuffix) text += " " + expandMacros(spec.suffix);
                    }
                    output << text << "\n";
                    if (text.find('\n') != std::string::npos)
                        output << "#line " << (lineNum + 1) << " \"" << reportedFile << "\"\n";
                } else {
                    output << "\n";
                }
            } else if (cmd == "#include") {
                if (isConditionTrue()) {
                    std::string lineRest;
                    std::getline(ss, lineRest);
                    lineRest = trim(lineRest);
                    
                    if (!lineRest.empty() && lineRest[0] != '"' && lineRest[0] != '<') {
                        lineRest = expandMacros(lineRest);
                    }

                    std::string fileName;
                    size_t firstQuote = lineRest.find('"');
                    size_t lastQuote = lineRest.rfind('"');
                    if (firstQuote != std::string::npos && lastQuote != std::string::npos && firstQuote < lastQuote) {
                        fileName = lineRest.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                    } else {
                        size_t firstAngle = lineRest.find('<');
                        size_t lastAngle = lineRest.rfind('>');
                        if (firstAngle != std::string::npos && lastAngle != std::string::npos && firstAngle < lastAngle) {
                            fileName = lineRest.substr(firstAngle + 1, lastAngle - firstAngle - 1);
                        }
                    }

                    if (fileName.empty()) throw std::runtime_error("Invalid #include directive: " + lineRest);

                    std::string fullPath = findIncludeFile(fileName, getDirectory(currentFile));
                    if (fullPath.empty()) throw std::runtime_error("Could not find include file: " + fileName);
                    
                    std::ifstream includeFile(fullPath);
                    if (!includeFile.is_open()) throw std::runtime_error("Failed to open include file: " + fullPath);
                    
                    std::stringstream buffer;
                    buffer << includeFile.rdbuf();
                    // Emit #line directive to mark the start of the included file
                    output << "#line 1 \"" << fullPath << "\"\n";
                    output << processInternal(buffer.str(), fullPath, depth + 1);
                    currentFile_ = currentFile;
                    includeDepth_ = depth;
                    // Restore line numbering after include
                    output << "#line " << (lineNum + 1) << " \"" << currentFile << "\"\n";
                } else {
                    output << "\n";
                }
            } else if (cmd == "#line") {
                if (isConditionTrue()) {
                    int newLineNum;
                    if (ss >> newLineNum) {
                        lineNum = newLineNum - 1;
                        std::string newFile;
                        if (ss >> newFile) {
                            if (newFile.size() >= 2 && newFile.front() == '"' && newFile.back() == '"') {
                                reportedFile = newFile.substr(1, newFile.size() - 2);
                            }
                        }
                        // Pass through #line directive to output for lexer to process
                        output << "#line " << newLineNum;
                        if (!newFile.empty()) output << " " << newFile;
                        output << "\n";
                    } else {
                        output << "\n";
                    }
                } else {
                    output << "\n";
                }
            } else if (cmd == "#error") {
                if (isConditionTrue()) {
                    std::string msg;
                    std::getline(ss, msg);
                    throw std::runtime_error("#error: " + trim(msg));
                }
                output << "\n";
            } else if (cmd == "#warning") {
                if (isConditionTrue()) {
                    std::string msg;
                    std::getline(ss, msg);
                    std::cerr << "Warning: " << trim(msg) << " at " << reportedFile << ":" << lineNum << std::endl;
                }
                output << "\n";
            } else if (cmd == "#pragma") {
                pragmaLine_ = lineNum;
                handlePragma(ss, output, currentFile);
            } else {
                output << "\n";
            }
        } else {
            // a macro call whose arguments go on over the next lines: joined
            // first (a blank line out for each, as for a '\\' continuation)
            if (isConditionTrue() && joinedLines < 64 && hasOpenMacroCall(currentLine)) {
                accumulatedLine = currentLine + " ";
                joinedLines++;
                output << "\n";
                continue;
            }
            joinedLines = 0;
            if (isConditionTrue()) {
                expandLine_ = lineNum;
                expandFile_ = reportedFile;
                std::string processed = expandMacros(currentLine);
                expandLine_ = 0;
                expandFile_.clear();

                // Handle standard predefined macros if expandMacros didn't (it should have, but let's be sure about __FILE__ etc)
                // Actually expandMacros should handle macros map. Predefined ones should be in macros map.
                // Wait, __FILE__ and __LINE__ change per line!
                
                auto replaceAll = [&](std::string& s, const std::string& from, const std::string& to) {
                    size_t pos = 0;
                    bool inStr = false;
                    while ((pos = s.find(from, pos)) != std::string::npos) {
                        inStr = false;
                        for (size_t k = 0; k < pos; ++k) {
                            if (inStr && s[k] == '\\') ++k;   // an escaped character
                            else if (s[k] == '"') inStr = !inStr;
                        }
                        if (inStr) { pos += from.length(); continue; }
                        bool boundaryBefore = (pos == 0 || (!std::isalnum((unsigned char)s[pos-1]) && s[pos-1] != '_'));
                        bool boundaryAfter = (pos + from.length() == s.length() || (!std::isalnum((unsigned char)s[pos + from.length()]) && s[pos + from.length()] != '_'));
                        if (boundaryBefore && boundaryAfter) {
                            s.replace(pos, from.length(), to);
                            pos += to.length();
                        } else {
                            pos += from.length();
                        }
                    }
                };

                replaceAll(processed, "__FILE__", "\"" + reportedFile + "\"");
                replaceAll(processed, "__LINE__", std::to_string(lineNum));
                replaceAll(processed, "__DATE__", preprocDate);
                replaceAll(processed, "__TIME__", preprocTime);

                // _Pragma("x") (also from a macro): #pragma x, its output put
                // first on this line (the line count kept)
                std::string pragmaOut;
                for (const auto& pragma : pendingPragmas_) {
                    std::istringstream ps(pragma);
                    std::ostringstream po;
                    pragmaLine_ = lineNum;
                    handlePragma(ps, po, currentFile);
                    for (char ch : po.str()) pragmaOut += ch == '\n' ? ' ' : ch;
                }
                pendingPragmas_.clear();
                while (!pragmaOut.empty() && pragmaOut.back() == ' ') pragmaOut.pop_back();
                if (!pragmaOut.empty() && !processed.empty() && !std::isspace((unsigned char)processed[0]))
                    pragmaOut += ' ';
                output << pragmaOut << processed << "\n";
            } else {
                output << "\n";
            }
        }
    }

    return output.str();
}
