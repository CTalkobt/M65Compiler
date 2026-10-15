#include <cstring>
#include "Preprocessor.hpp"
#include <fstream>
#include <sstream>
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
    macros["__45GS02__"] = Macro{false, false, {}, "1"};
    macros["__6502__"] = Macro{false, false, {}, "1"};

    // Standard predefined macros (C17)
    macros["__STDC__"] = Macro{false, false, {}, "1"};
    macros["__STDC_VERSION__"] = Macro{false, false, {}, "201710L"};
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

std::string Preprocessor::expandMacros(const std::string& line) {
    std::string result = line;
    
    // 0. Handle _Pragma("string") — convert to equivalent #pragma directive (C99 §6.10.9)
    size_t pragmaPos;
    while ((pragmaPos = result.find("_Pragma")) != std::string::npos) {
        // Simple whole-word check
        bool bBefore = (pragmaPos == 0 || (!std::isalnum((unsigned char)result[pragmaPos-1]) && result[pragmaPos-1] != '_'));
        if (!bBefore) { break; }

        size_t start = pragmaPos;
        pragmaPos += 7;
        while (pragmaPos < result.length() && std::isspace((unsigned char)result[pragmaPos])) pragmaPos++;
        if (pragmaPos < result.length() && result[pragmaPos] == '(') {
            pragmaPos++;
            while (pragmaPos < result.length() && std::isspace((unsigned char)result[pragmaPos])) pragmaPos++;
            if (pragmaPos < result.length() && result[pragmaPos] == '"') {
                size_t strStart = pragmaPos + 1;
                size_t strEnd = result.find('"', strStart);
                if (strEnd != std::string::npos) {
                    std::string pragmaContent = result.substr(strStart, strEnd - strStart);
                    size_t endParen = result.find(')', strEnd);
                    if (endParen != std::string::npos) {
                        // Replace _Pragma("...") with nothing and queue the pragma for processing
                        result.erase(start, endParen - start + 1);
                        // Inject as #pragma line — store for the caller to process
                        pendingPragmas_.push_back(pragmaContent);
                        continue;
                    }
                }
            }
        }
        break;
    }

    bool changed = true;
    int limit = 100; // Prevent infinite expansion

    while (changed && limit-- > 0) {
        changed = false;
        std::string nextResult;
        bool inString = false;

        for (size_t i = 0; i < result.length(); ) {
            // Skip strings
            if (result[i] == '"' && (i == 0 || result[i-1] != '\\')) {
                inString = !inString;
                nextResult += result[i++];
                continue;
            }
            if (inString) {
                nextResult += result[i++];
                continue;
            }

            // Character constants ('F', '\'') are not scanned for macros
            if (result[i] == '\'') {
                nextResult += result[i++];
                while (i < result.length() && result[i] != '\'') {
                    if (result[i] == '\\' && i + 1 < result.length()) nextResult += result[i++];
                    nextResult += result[i++];
                }
                if (i < result.length()) nextResult += result[i++];
                continue;
            }

            // A pp-number (1.0F, 0x1fUL, 1e-5) is one token: its letters are
            // not identifiers (#define F 140 must not touch 1.0F)
            if (std::isdigit((unsigned char)result[i]) ||
                (result[i] == '.' && i + 1 < result.length() && std::isdigit((unsigned char)result[i + 1]))) {
                while (i < result.length()) {
                    char ch = result[i];
                    if ((ch == '+' || ch == '-') && i > 0 && std::strchr("eEpP", result[i - 1])) {
                        nextResult += result[i++];
                    } else if (std::isalnum((unsigned char)ch) || ch == '_' || ch == '.') {
                        nextResult += result[i++];
                    } else {
                        break;
                    }
                }
                continue;
            }

            // Look for identifiers
            if (std::isalpha((unsigned char)result[i]) || result[i] == '_') {
                size_t start = i;
                while (i < result.length() && (std::isalnum((unsigned char)result[i]) || result[i] == '_')) i++;
                std::string ident = result.substr(start, i - start);

                // An encoding prefix (L'1', u"x", U'a', u8"s"; cc45/ca45 p"x", s"x", a"x") is part of the
                // literal, not a macro name
                if ((ident == "L" || ident == "u" || ident == "U" || ident == "u8" ||
                     ident == "p" || ident == "s" || ident == "a") &&
                    i < result.length() && (result[i] == '\'' || result[i] == '"')) {
                    nextResult += ident;
                    continue;
                }

                std::string replacement = "";
                bool found = false;

                // __INCLUDE_LEVEL__ — current include nesting depth
                if (ident == "__INCLUDE_LEVEL__") {
                    replacement = std::to_string(includeDepth_);
                    found = true;
                }

                // __has_include(<header>) / __has_include("header") — check if file exists
                if (!found && ident == "__has_include") {
                    size_t peek = i;
                    while (peek < result.length() && std::isspace((unsigned char)result[peek])) peek++;
                    if (peek < result.length() && result[peek] == '(') {
                        peek++;
                        while (peek < result.length() && std::isspace((unsigned char)result[peek])) peek++;
                        std::string fileName;
                        char delim = 0;
                        if (peek < result.length() && (result[peek] == '"' || result[peek] == '<')) {
                            delim = (result[peek] == '<') ? '>' : '"';
                            peek++;
                            size_t nameStart = peek;
                            while (peek < result.length() && result[peek] != delim) peek++;
                            fileName = result.substr(nameStart, peek - nameStart);
                            if (peek < result.length()) peek++; // skip closing delim
                        }
                        while (peek < result.length() && std::isspace((unsigned char)result[peek])) peek++;
                        if (peek < result.length() && result[peek] == ')') peek++;
                        // Check if file can be found
                        std::string fullPath = findIncludeFile(fileName, getDirectory(currentFile_));
                        replacement = fullPath.empty() ? "0" : "1";
                        i = peek;
                        found = true;
                    }
                }

                // __has_builtin(__builtin_X) — check if builtin is supported
                if (!found && ident == "__has_builtin") {
                    size_t peek = i;
                    while (peek < result.length() && std::isspace((unsigned char)result[peek])) peek++;
                    if (peek < result.length() && result[peek] == '(') {
                        peek++;
                        while (peek < result.length() && std::isspace((unsigned char)result[peek])) peek++;
                        size_t nameStart = peek;
                        while (peek < result.length() && (std::isalnum((unsigned char)result[peek]) || result[peek] == '_')) peek++;
                        std::string builtinName = result.substr(nameStart, peek - nameStart);
                        while (peek < result.length() && std::isspace((unsigned char)result[peek])) peek++;
                        if (peek < result.length() && result[peek] == ')') peek++;
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
                        replacement = knownBuiltins.count(builtinName) ? "1" : "0";
                        i = peek;
                        found = true;
                    }
                }

                // __has_attribute(attr) — check if attribute is supported
                if (!found && ident == "__has_attribute") {
                    size_t peek = i;
                    while (peek < result.length() && std::isspace((unsigned char)result[peek])) peek++;
                    if (peek < result.length() && result[peek] == '(') {
                        peek++;
                        while (peek < result.length() && std::isspace((unsigned char)result[peek])) peek++;
                        size_t nameStart = peek;
                        while (peek < result.length() && (std::isalnum((unsigned char)result[peek]) || result[peek] == '_')) peek++;
                        std::string attrName = result.substr(nameStart, peek - nameStart);
                        while (peek < result.length() && std::isspace((unsigned char)result[peek])) peek++;
                        if (peek < result.length() && result[peek] == ')') peek++;
                        static const std::set<std::string> knownAttrs = {
                            "always_inline", "noinline", "unused", "used", "weak",
                            "pure", "const", "cold", "hot", "packed", "aligned",
                            "deprecated", "naked", "interrupt", "noreturn",
                            "section", "visibility", "constructor", "destructor",
                            "format", "warn_unused_result",
                        };
                        replacement = knownAttrs.count(attrName) ? "1" : "0";
                        i = peek;
                        found = true;
                    }
                }

                if (macros.count(ident)) {
                    const auto& m = macros[ident];
                    if (!m.isFunctionLike) {
                        replacement = m.body;
                        found = true;
                    } else {
                        // Function-like macro: check for (
                        size_t peek = i;
                        while (peek < result.length() && std::isspace((unsigned char)result[peek])) peek++;
                        if (peek < result.length() && result[peek] == '(') {
                            peek++; // skip (
                            std::vector<std::string> args;
                            std::string currentArg;
                            int parenDepth = 0;
                            bool inString = false;
                            char strChar = 0;
                            while (peek < result.length()) {
                                char ch = result[peek];
                                // Track string/char literal state
                                if (!inString && (ch == '"' || ch == '\'')) {
                                    inString = true; strChar = ch;
                                } else if (inString && ch == strChar && (peek == 0 || result[peek-1] != '\\')) {
                                    inString = false;
                                }
                                if (!inString && ch == '(') parenDepth++;
                                else if (!inString && ch == ')') {
                                    if (parenDepth == 0) { peek++; break; }
                                    parenDepth--;
                                } else if (!inString && ch == ',' && parenDepth == 0) {
                                    if (!m.isVariadic || args.size() < m.params.size()) {
                                        args.push_back(trim(currentArg));
                                        currentArg = "";
                                        peek++;
                                        continue;
                                    }
                                }
                                currentArg += result[peek++];
                            }
                            args.push_back(trim(currentArg));

                            // Handle arguments
                            if ((!m.isVariadic && args.size() == m.params.size()) || 
                                (m.isVariadic && args.size() >= m.params.size())) {
                                
                                std::string body = m.body;
                                std::string vaArgs = "";
                                if (m.isVariadic) {
                                    for (size_t a = m.params.size(); a < args.size(); ++a) {
                                        if (!vaArgs.empty()) vaArgs += ", ";
                                        vaArgs += args[a];
                                    }
                                }

                                // 1. Handle # stringification (including #__VA_ARGS__)
                                auto doStringify = [&](std::string& b, const std::string& paramName, const std::string& argVal) {
                                    std::string pattern = "#" + paramName;
                                    size_t pos = 0;
                                    while ((pos = b.find(pattern, pos)) != std::string::npos) {
                                        if (pos > 0 && b[pos-1] == '#') { pos += pattern.length(); continue; }
                                        // Escape inner quotes and backslashes per C99 §6.10.3.2
                                        std::string escaped;
                                        for (char ch : argVal) {
                                            if (ch == '"' || ch == '\\') escaped += '\\';
                                            escaped += ch;
                                        }
                                        b.replace(pos, pattern.length(), "\"" + escaped + "\"");
                                    }
                                };

                                for (size_t p = 0; p < m.params.size(); ++p) doStringify(body, m.params[p], args[p]);
                                if (m.isVariadic) doStringify(body, "__VA_ARGS__", vaArgs);

                                // 2. Handle ## pasting (including comma removal extension ##__VA_ARGS__)
                                if (m.isVariadic) {
                                    size_t glue;
                                    // Handle GCC-style comma removal: , ##__VA_ARGS__
                                    // (the arguments go in with the parameters below:
                                    // here only the ## and, for no arguments, the comma go)
                                    while ((glue = body.find("##__VA_ARGS__")) != std::string::npos) {
                                        size_t comma = glue;
                                        while (comma > 0 && std::isspace((unsigned char)body[comma-1])) comma--;
                                        if (comma > 0 && body[comma-1] == ',' && vaArgs.empty())
                                            body.replace(comma - 1, (glue + 13) - (comma - 1), "");
                                        else
                                            body.erase(glue, 2);
                                    }
                                }

                                // 3. Replace parameters and __VA_ARGS__ in one left-to-right
                                // pass over the body's tokens, so an argument's text is never
                                // scanned for parameter names again (dma_copy_far(dst, src, len)
                                // with src = f((long)len)). Not inside string/character
                                // literals, nor an L/u/U/u8/p/s/a literal prefix; numbers are
                                // skipped whole (0x1Fu holds no identifier).
                                {
                                    std::string out;
                                    out.reserve(body.size() * 2);
                                    size_t k = 0;
                                    auto isId = [](char c) { return std::isalnum((unsigned char)c) || c == '_'; };
                                    while (k < body.size()) {
                                        char c = body[k];
                                        if (c == '"' || c == '\'') {
                                            size_t e = k + 1;
                                            while (e < body.size() && body[e] != c) e += body[e] == '\\' ? 2 : 1;
                                            if (e < body.size()) e++;
                                            out.append(body, k, e - k);
                                            k = e;
                                        } else if (std::isdigit((unsigned char)c) ||
                                                   (c == '.' && k + 1 < body.size() && std::isdigit((unsigned char)body[k + 1]))) {
                                            size_t e = k + 1;
                                            while (e < body.size() && (isId(body[e]) || body[e] == '.' ||
                                                   ((body[e] == '+' || body[e] == '-') && (body[e-1] == 'e' || body[e-1] == 'E' ||
                                                                                          body[e-1] == 'p' || body[e-1] == 'P'))))
                                                e++;
                                            out.append(body, k, e - k);
                                            k = e;
                                        } else if (isId(c)) {
                                            size_t e = k;
                                            while (e < body.size() && isId(body[e])) e++;
                                            std::string id = body.substr(k, e - k);
                                            bool litPrefix = (id == "L" || id == "u" || id == "U" || id == "u8" ||
                                                              id == "p" || id == "s" || id == "a") &&
                                                             e < body.size() && (body[e] == '\'' || body[e] == '"');
                                            const std::string* rep = nullptr;
                                            if (!litPrefix) {
                                                for (size_t p = 0; p < m.params.size(); ++p)
                                                    if (m.params[p] == id) { rep = &args[p]; break; }
                                                if (!rep && m.isVariadic && id == "__VA_ARGS__") rep = &vaArgs;
                                            }
                                            out += rep ? *rep : id;
                                            k = e;
                                        } else {
                                            out += c;
                                            k++;
                                        }
                                    }
                                    body = std::move(out);
                                }

                                // 4. Final Glue cleanup
                                size_t glue;
                                while ((glue = body.find("##")) != std::string::npos) {
                                    size_t s_p = glue;
                                    while (s_p > 0 && std::isspace((unsigned char)body[s_p-1])) s_p--;
                                    size_t e_p = glue + 2;
                                    while (e_p < body.length() && std::isspace((unsigned char)body[e_p])) e_p++;
                                    body.erase(s_p, e_p - s_p);
                                }
                                
                                replacement = body;
                                found = true;
                                i = peek;
                            }
                        }
                    }
                }

                if (found) {
                    nextResult += replacement;
                    changed = true;
                } else {
                    nextResult += ident;
                }
            } else {
                nextResult += result[i++];
            }
        }
        result = nextResult;
    }
    return result;
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
        this->macros[pair.first] = Macro{false, false, {}, pair.second};
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
    if (isCompiler && source.find("intwide.h") == std::string::npos &&
        (code.find("__int128") != std::string::npos || code.find("__int(") != std::string::npos ||
         code.find("__uint(") != std::string::npos || code.find("int64_t") != std::string::npos ||
         code.find("__int64") != std::string::npos || hasLongLong(code)) &&
        !findIncludeFile("intwide.h", getDirectory(currentFile)).empty()) {
        return processInternal("#include <intwide.h>\n#line 1 \"" + currentFile + "\"\n" + source, currentFile, 0);
    }
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
    fs::path p1 = fs::path(currentDir) / fileName;
    if (fs::exists(p1)) return p1.string();
    
    // 2. Check include paths
    for (const auto& dir : includePaths) {
        fs::path p = fs::path(dir) / fileName;
        if (fs::exists(p)) return p.string();
    }
    
    return "";
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
                        macros[name] = m;
                    } else {
                        // Object-like
                        std::string body = lineRest.substr(name.length());
                        macros[name] = Macro{false, false, {}, trim(body)};
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
                    std::cerr << "Warning: " << trim(msg) << " at " << reportedFile << ":" << (lineNum + 1) << std::endl;
                }
                output << "\n";
            } else if (cmd == "#pragma") {
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
            } else {
                output << "\n";
            }
        } else {
            if (isConditionTrue()) {
                std::string processed = expandMacros(currentLine);

                // Handle standard predefined macros if expandMacros didn't (it should have, but let's be sure about __FILE__ etc)
                // Actually expandMacros should handle macros map. Predefined ones should be in macros map.
                // Wait, __FILE__ and __LINE__ change per line!
                
                auto replaceAll = [&](std::string& s, const std::string& from, const std::string& to) {
                    size_t pos = 0;
                    bool inStr = false;
                    while ((pos = s.find(from, pos)) != std::string::npos) {
                        inStr = false;
                        for (size_t k = 0; k < pos; ++k) {
                            if (s[k] == '"' && (k == 0 || s[k-1] != '\\')) inStr = !inStr;
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

                output << processed << "\n";
                // Process any _Pragma() directives collected during macro expansion
                for (const auto& pragma : pendingPragmas_) {
                    std::istringstream ps(pragma);
                    std::string pragmaArg;
                    ps >> pragmaArg;
                    if (pragmaArg == "once") {
                        if (!currentFile.empty()) onceFiles.insert(currentFile);
                    } else if (isCompiler && pragmaArg == "weak") {
                        output << "__asm__(\".weak_next\");\n";
                    } else if (isCompiler && pragmaArg == "no_zp_save") {
                        output << "__asm__(\".no_zp_save\");\n";
                    } else if (isCompiler && pragmaArg == "cc45") {
                        std::string cc45Arg;
                        ps >> cc45Arg;
                        if (cc45Arg == "no_0100_stack") output << "__asm__(\".crt_no_0100_stack\");\n";
                        else if (cc45Arg == "exit_halt") output << "__asm__(\".crt_exit_halt\");\n";
                        else if (cc45Arg == "exit_rts") output << "__asm__(\".crt_exit_rts\");\n";
                        else if (cc45Arg == "exit_brk") output << "__asm__(\".crt_exit_brk\");\n";
                        else if (cc45Arg == "no_bssinit") output << "__asm__(\".crt_no_bssinit\");\n";
                        else if (cc45Arg == "heap") output << "__asm__(\".crt_heap\");\n";
                        else if (cc45Arg == "stdio") output << "__asm__(\".crt_stdio\");\n";
                        else if (cc45Arg == "no_zp_save") output << "__asm__(\".no_zp_save\");\n";
                        else if (cc45Arg == "no_stack16") output << "__asm__(\".no_stack16\");\n";
                        else if (cc45Arg.rfind("stack_size", 0) == 0) {
                            std::string rest = cc45Arg.substr(10), tail;
                            while (ps >> tail) rest += tail;
                            std::string n;
                            for (char ch : rest) if (std::isxdigit((unsigned char)ch) || ch == '$' || ch == 'x' || ch == 'X') n += ch;
                            output << (n.empty() ? std::string("\n") : "__asm__(\".stack_size " + n + "\");\n");
                        }
                        else if (cc45Arg.rfind("stack_top", 0) == 0) {
                            std::string rest = cc45Arg.substr(9), tail;
                            while (ps >> tail) rest += tail;
                            std::string addr;
                            for (char ch : rest) if (std::isxdigit((unsigned char)ch) || ch == '$' || ch == 'x' || ch == 'X') addr += ch;
                            output << (addr.empty() ? std::string("\n") : "__asm__(\".stack_top " + addr + "\");\n");
                        }
                        else if (cc45Arg == "set_bp") { std::string bv; ps >> bv; output << "__asm__(\".set_bp " + bv + "\");\n"; }
                        else if (cc45Arg == "weak") output << "__asm__(\".weak_next\");\n";
                        else if (cc45Arg == "recurse") output << "__asm__(\".recurse_next\");\n";
                        else if (cc45Arg.rfind("optimize", 0) == 0) output << optimizeDirective(cc45Arg, ps);
                        else if (cc45Arg == "unroll") {
                            std::string countStr;
                            if (ps >> countStr) {
                                output << "asm(\".crt_unroll " + countStr + "\");\n";
                            } else {
                                output << "asm(\".crt_unroll\");\n";
                            }
                        }
                        else if (cc45Arg == "unroll_default") {
                            std::string countStr;
                            if (ps >> countStr) {
                                output << "asm(\".crt_unroll_def " + countStr + "\");\n";
                            }
                        }
                    } else if (isCompiler && pragmaArg.substr(0, 9) == "encoding(") {
                        std::string enc = pragmaArg.substr(9);
                        if (!enc.empty() && enc.back() == ')') enc.pop_back();
                        if (enc == "ascii" || enc == "petscii" || enc == "screencode")
                            output << "__asm__(\".encoding " + enc + "\");\n";
                    } else if (isCompiler && pragmaArg == "encoding") {
                        std::string enc;
                        ps >> enc;
                        if (enc == "ascii" || enc == "petscii" || enc == "screencode")
                            output << "__asm__(\".encoding " + enc + "\");\n";
                    }
                }
                pendingPragmas_.clear();
            } else {
                output << "\n";
            }
        }
    }

    return output.str();
}
