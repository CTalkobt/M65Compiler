#include "BasicStructured.hpp"
#include "LabelBasedSourceParser.hpp"
#include <cstring>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <sstream>
#include <iterator>

namespace fs = std::filesystem;

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

// Split at commas outside parentheses and strings
std::vector<std::string> splitTop(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    int depth = 0;
    bool inString = false;
    for (char c : s) {
        if (c == '"') inString = !inString;
        if (!inString) {
            if (c == '(') depth++;
            else if (c == ')') depth--;
            else if (c == ',' && depth == 0) { out.push_back(trim(cur)); cur.clear(); continue; }
        }
        cur += c;
    }
    if (!trim(cur).empty() || !out.empty()) out.push_back(trim(cur));
    return out;
}

bool isComment(const std::string& t) {
    return !t.empty() && (t[0] == '\'' || t.compare(0, 3, "REM") == 0 || lower(t.substr(0, 4)) == "rem ");
}

// The line without a ' comment after its statement (#583). Not in a string,
// nor after REM or DATA (their text is kept as written).
std::string stripTrailingComment(const std::string& t) {
    bool inStr = false;
    auto letter = [](char c) { return std::isalpha((unsigned char)c) != 0; };
    for (size_t i = 0; i < t.size(); i++) {
        char c = t[i];
        if (c == '"') { inStr = !inStr; continue; }
        if (inStr) continue;
        if (c == '\'') return trim(t.substr(0, i));
        if (letter(c) && (i == 0 || !(letter(t[i - 1]) || t[i - 1] == '_'))) {
            for (const char* w : {"rem", "data"}) {
                size_t n = strlen(w);
                if (lower(t.substr(i, n)) == w && (i + n >= t.size() || !(letter(t[i + n]) || t[i + n] == '_')))
                    return t;
            }
        }
    }
    return t;
}

// .name (a local label) -> __l<scope>_name, outside strings
std::string localize(const std::string& line, const std::string& scope) {
    std::string out;
    bool inString = false;
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (c == '"') inString = !inString;
        if (!inString && c == '.' && i + 1 < line.size() &&
            (std::isalpha((unsigned char)line[i + 1]) || line[i + 1] == '_') &&
            (i == 0 || !(std::isalnum((unsigned char)line[i - 1]) || line[i - 1] == '_' ||
                         line[i - 1] == '$' || line[i - 1] == '.'))) {
            size_t e = i + 1;
            while (e < line.size() && (std::isalnum((unsigned char)line[e]) || line[e] == '_')) e++;
            out += "__l" + scope + "_" + line.substr(i + 1, e - i - 1);
            i = e - 1;
            continue;
        }
        out += c;
    }
    return out;
}

bool isStringExpr(const std::string& e) {
    bool inString = false;
    for (size_t i = 0; i < e.size(); i++) {
        if (e[i] == '"') return true;
        if (!inString && e[i] == '$' && i > 0 && std::isalnum((unsigned char)e[i - 1])) return true;
    }
    return false;
}

// Whether expression e is a string (return <expr>'s ZRET$, #680): no
// comparison outside parentheses and strings (that is a number), and its first
// operand a string literal or a name ending in $ (a variable, an array, LEFT$...)
bool isStringValue(const std::string& e) {
    int depth = 0;
    bool q = false;
    for (char c : e) {
        if (c == '"') q = !q;
        if (q) continue;
        if (c == '(') depth++;
        else if (c == ')') depth--;
        else if (depth == 0 && (c == '=' || c == '<' || c == '>')) return false;
    }
    size_t i = e.find_first_not_of(" \t(");
    if (i == std::string::npos) return false;
    if (e[i] == '"') return true;
    if (!std::isalpha((unsigned char)e[i])) return false;
    size_t j = i;
    while (j < e.size() && (std::isalnum((unsigned char)e[j]) || e[j] == '_')) j++;
    return j < e.size() && e[j] == '$';
}

// Whether word (lower case) is in text as a whole word, outside strings
bool hasWord(const std::string& text, const char* word) {
    size_t n = strlen(word);
    bool inString = false;
    auto nameCh = [](char c) { return std::isalnum((unsigned char)c) || c == '_'; };
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] == '"') { inString = !inString; continue; }
        if (inString || (i > 0 && nameCh(text[i - 1])) || i + n > text.size()) continue;
        if (lower(text.substr(i, n)) == word && (i + n == text.size() || !nameCh(text[i + n]))) return true;
    }
    return false;
}

// Whether the line after this one cannot be reached by running this one:
// its last statement is a GOTO, RETURN, END, RUN or RESUME and nothing on
// the line is conditional (#605)
bool neverFallsThrough(const std::string& line) {
    if (hasWord(line, "if") || hasWord(line, "then") || hasWord(line, "else")) return false;
    size_t start = 0;
    bool inString = false;
    for (size_t i = 0; i < line.size(); i++) {
        if (line[i] == '"') inString = !inString;
        else if (!inString && line[i] == ':') start = i + 1;
    }
    std::string last = lower(trim(line.substr(start)));
    if (last == "return" || last == "end" || last == "run" || last == "resume") return true;
    // (return <expr>: a compiled proc's, #577)
    for (const char* w : {"goto ", "run ", "resume ", "return ", "return("})
        if (last.compare(0, strlen(w), w) == 0) return true;
    return false;
}

// Whether e has no comparison or logical operator outside parentheses and
// strings: v = e, v <> e, v >= e then mean what they say
bool plainOperand(const std::string& e) {
    int depth = 0;
    bool inString = false;
    std::string top;
    for (char c : e) {
        if (c == '"') { inString = !inString; top += ' '; continue; }
        if (inString) continue;
        if (c == '(') depth++;
        else if (c == ')') depth--;
        else if (depth == 0) {
            if (c == '=' || c == '<' || c == '>') return false;
            top += c;
            continue;
        }
        top += ' ';
    }
    for (const char* w : {"and", "or", "not", "xor"})
        if (hasWord(top, w)) return false;
    return !trim(e).empty();
}

// The opposite of a block IF's condition (#605): "a < b" is "a >= b", "x"
// (true when not 0) is "x=0". Empty for a condition with AND, OR, NOT, XOR
// or more than one comparison outside parentheses: that one is "(cond)=0".
std::string oppositeCond(const std::string& cond) {
    int depth = 0, ops = 0;
    bool inString = false;
    size_t opAt = 0, opLen = 0;
    std::string top;
    for (size_t i = 0; i < cond.size(); i++) {
        char c = cond[i];
        if (c == '"') { inString = !inString; top += ' '; continue; }
        if (inString) { top += ' '; continue; }
        if (c == '(') depth++;
        else if (c == ')') depth--;
        else if (depth == 0) {
            if (c == '=' || c == '<' || c == '>') {
                if (ops && opAt + opLen == i) opLen++;
                else { ops++; opAt = i; opLen = 1; }
                top += ' ';
            } else top += c;
            continue;
        }
        top += ' ';
    }
    for (const char* w : {"and", "or", "not", "xor"})
        if (hasWord(top, w)) return "";
    if (ops == 0) return trim(cond).empty() ? "" : trim(cond) + "=0";
    if (ops != 1) return "";
    std::string op = cond.substr(opAt, opLen);
    std::string inv = op == "=" ? "<>" : op == "<>" ? "=" : op == "<" ? ">=" : op == ">" ? "<=" :
                      op == "<=" ? ">" : op == ">=" ? "<" : "";
    std::string l = trim(cond.substr(0, opAt)), r = trim(cond.substr(opAt + opLen));
    if (inv.empty() || l.empty() || r.empty()) return "";
    return l + inv + r;
}

}  // namespace

BasicStructuredResult lowerBasicStructured(const std::string& fullSource,
                                           const std::function<std::string(int)>& fileOf, BasicLowerMode mode) {
    BasicStructuredResult r;
    const bool unit = mode == BasicLowerMode::CompiledProcs;
    std::vector<std::string> lines;
    static const std::regex loneLabelRe(R"(^[A-Za-z_]\w*\s*:$)");
    // whether the next line can be reached from the one before it or by a
    // jump to a label in front of it
    bool reach = true;
    // the __endprocN: line just written with nothing run after it, and the
    // line of the GOTO that jumps to it: the next proc's skip is that GOTO
    int chainEnd = -1, chainGoto = -1;
    // (the compiled procs' text, #682: each line's proc)
    std::vector<std::string> lineProc;
    std::string curProc;
    auto emit = [&](const std::string& line, int from) {
        lines.push_back(line);
        r.origin.push_back(from);
        lineProc.push_back(curProc);
        std::string t = trim(line);
        if (t.empty() || isComment(t)) return;
        chainEnd = chainGoto = -1;
        if (t[0] == '#' || std::regex_match(t, loneLabelRe)) reach = true;
        else reach = !neverFallsThrough(t);
    };
    auto error = [&](int line, const std::string& msg) { r.errors.push_back({line, msg}); };

    static const std::regex labelRe(R"(^([A-Za-z_]\w*)\s*:\s*(.*)$)");
    static const std::regex ifThenRe(R"(^if\s+(.*\S)\s+then$)", std::regex::icase);
    static const std::regex procRe(R"(^proc\s+([A-Za-z_]\w*)\s*(\((.*)\))?(\s+compiled)?$)", std::regex::icase);
    static const std::regex selectRe(R"(^select\s+(.+)$)", std::regex::icase);
    static const std::regex caseRe(R"(^case\s+(.+)$)", std::regex::icase);
    static const std::regex rangeRe(R"(^(.*\S)\s+to\s+(.+)$)", std::regex::icase);
    static const std::regex isRe(R"(^is\s*(<>|<=|>=|=|<|>)\s*(.+)$)", std::regex::icase);
    static const std::regex incbinRe(R"re(^incbin\s+"([^"]+)"\s+as\s+([A-Za-z_]\w*)$)re", std::regex::icase);
    static const std::regex dataincRe(R"re(^datainc\s+"([^"]+)"(\s*,\s*(\d+))?(\s+as\s+([A-Za-z_]\w*))?$)re",
                                      std::regex::icase);
    static const std::regex asmLabelRe(R"(^([A-Za-z_]\w*):)");

    // First pass: the parameters of each proc, and the compiled ones (#577):
    // their place (1, 2, ...); in the interpreted program cut out of the
    // source (their lines left empty) and lowered as a text of their own
    std::map<std::string, std::vector<std::string>> procParams;   // lower-case name -> params
    std::map<std::string, int> compiledK;                         // lower-case name -> k
    bool usesZbuf = false;                                        // a compiled proc's string result (#680)
    std::string source;
    {
        std::istringstream in(fullSource);
        std::string raw, unitSrc;
        static const std::regex procDef(
            R"(^\s*(?:[A-Za-z_]\w*\s*:\s*)?proc\s+([A-Za-z_]\w*)\s*(\((.*)\))?(\s+compiled)?\s*$)", std::regex::icase);
        std::smatch m;
        bool inCompiled = false;
        while (std::getline(in, raw)) {
            std::string tt = stripTrailingComment(trim(raw)), t = lower(tt);
            if (std::regex_match(tt, m, procDef)) {
                std::vector<std::string> ps;
                if (m[2].matched && !trim(m[3].str()).empty()) ps = splitTop(m[3].str());
                procParams[lower(m[1].str())] = ps;
                if (m[4].matched && mode != BasicLowerMode::Compiled) {
                    int k = (int)compiledK.size() + 1;
                    compiledK[lower(m[1].str())] = k;
                    inCompiled = mode == BasicLowerMode::Interpreted;
                }
            }
            if (inCompiled) {
                unitSrc += raw + "\n";
                source += "\n";
                if (t == "endproc" || t == "end proc") inCompiled = false;
            } else {
                unitSrc += "\n";
                source += raw + "\n";
            }
        }
        if (mode == BasicLowerMode::Interpreted && !compiledK.empty()) {
            BasicStructuredResult u = lowerBasicStructured(unitSrc, fileOf, BasicLowerMode::CompiledProcs);
            r.compiledText = u.text;
            r.compiledOrigin = u.origin;
            r.compiledProcs = u.compiledProcs;
            r.compiledArgs = u.compiledArgs;
            r.errors = u.errors;
        }
        if (mode != BasicLowerMode::Interpreted) source = fullSource;
    }

    // call name[(args)] -> parameters assigned, then gosub name
    auto lowerCalls = [&](const std::string& code, int lineNum) {
        std::string res;
        bool inString = false;
        for (size_t i = 0; i < code.size(); i++) {
            char c = code[i];
            if (c == '"') inString = !inString;
            if (!inString && (i == 0 || !(std::isalnum((unsigned char)code[i - 1]) || code[i - 1] == '_')) &&
                i + 4 <= code.size() && lower(code.substr(i, 4)) == "call" &&
                (i + 4 == code.size() || code[i + 4] == ' ' || code[i + 4] == '\t')) {
                size_t n = code.find_first_not_of(" \t", i + 4);
                if (n != std::string::npos && (std::isalpha((unsigned char)code[n]) || code[n] == '_')) {
                    size_t e = n;
                    while (e < code.size() && (std::isalnum((unsigned char)code[e]) || code[e] == '_')) e++;
                    std::string name = code.substr(n, e - n);
                    std::vector<std::string> args;
                    size_t after = code.find_first_not_of(" \t", e);
                    if (after != std::string::npos && code[after] == '(') {
                        int depth = 0;
                        size_t close = after;
                        bool q = false;
                        for (; close < code.size(); close++) {
                            if (code[close] == '"') q = !q;
                            if (q) continue;
                            if (code[close] == '(') depth++;
                            else if (code[close] == ')' && --depth == 0) break;
                        }
                        args = splitTop(code.substr(after + 1, close - after - 1));
                        e = close + 1;
                    }
                    // ... to v: the proc's result (return <expr>) into v, up to
                    // the statement's end (a : or ELSE outside strings and
                    // parentheses)
                    std::string to;
                    {
                        size_t t0 = code.find_first_not_of(" \t", e);
                        if (t0 != std::string::npos && t0 + 2 < code.size() && lower(code.substr(t0, 2)) == "to" &&
                            (code[t0 + 2] == ' ' || code[t0 + 2] == '\t')) {
                            size_t v0 = t0 + 3, v1 = v0;
                            int depth = 0;
                            for (; v1 < code.size(); v1++) {
                                char d = code[v1];
                                if (d == '(') depth++;
                                else if (d == ')') depth--;
                                else if (depth == 0 && (d == ':' || (lower(code.substr(v1, 4)) == "else" &&
                                                                     (v1 == 0 || code[v1 - 1] == ' '))))
                                    break;
                            }
                            to = trim(code.substr(v0, v1 - v0));
                            e = v1;
                            if (to.empty()) error(lineNum, "call " + name + " to: a variable expected");
                        }
                    }
                    auto pp = procParams.find(lower(name));
                    auto ck = compiledK.find(lower(name));
                    if (unit && ck == compiledK.end()) {
                        error(lineNum, "call " + name + ": a compiled proc calls only compiled procs (" + name +
                                       " is not one)");
                        i = e - 1;
                        continue;
                    }
                    std::string assigns;
                    bool argsOk = false;
                    if (pp == procParams.end()) {
                        if (!args.empty()) error(lineNum, "call " + name + "(...): no proc " + name + " with parameters");
                        if (!to.empty()) error(lineNum, "call " + name + " to " + to + ": no proc " + name);
                    } else if (pp->second.size() != args.size()) {
                        error(lineNum, "call " + name + ": " + std::to_string(args.size()) + " argument(s), proc " +
                                       name + " takes " + std::to_string(pp->second.size()));
                    } else {
                        argsOk = true;
                        for (size_t k = 0; k < args.size(); k++)
                            assigns += pp->second[k] + " = " + args[k] + " : ";
                    }
                    if (unit) {
                        // a compiled proc's call of a compiled proc (#681): a
                        // GOSUB inside their program, the result in ZRET
                        std::string zr = !to.empty() && trim(to.substr(0, to.find('('))).back() == '$' ? "zret$" : "zret";
                        res += assigns + zr + (zr == "zret" ? " = 0" : " = \"\"") + " : gosub __cp_" + name;
                        if (!to.empty()) res += " : " + to + " = " + zr;
                    } else if (ck != compiledK.end() && argsOk) {
                        // a compiled proc (#577): one WPOKE of the block's
                        // words, the proc's number and the result's kind, the
                        // result's POINTER (0: none), the parameters' (a
                        // variable POINTER makes moves no scalar made before it)
                        int kind = 0;
                        if (!to.empty()) {
                            std::string base = to.substr(0, to.find('('));
                            char sfx = trim(base).back();
                            if (sfx == '&') error(lineNum, "call " + name + " to " + to +
                                                       ": a compiled proc's result is a float, % or string variable");
                            kind = sfx == '%' ? 2 : sfx == '$' ? 3 : 1;
                        }
                        // (a string result, #680: into ZBUF$'s characters, made
                        // 255 long by __mixbuf when it is not; then LEFT$)
                        if (kind == 3) { res += "gosub __mixbuf : "; usesZbuf = true; }
                        // (the block in bank 0: BANK 128, SYS's, maps ROM
                        // over the code's RAM for POKE; the SYS in BANK 128,
                        // as --link's)
                        res += assigns + "bank 0 : wpoke {__bas_psel}, " + std::to_string(ck->second + 256 * kind);
                        if (kind || !args.empty())
                            res += ", " + (kind == 3 ? std::string("pointer(zbuf$)") : kind ? "pointer(" + to + ")" : std::string("0"));
                        for (size_t k = 0; k < args.size(); k++) res += ", pointer(" + pp->second[k] + ")";
                        // (an error in the proc: its message printed, its
                        // number back in A; the program ends at __mixerr)
                        res += " : bank 128 : sys {__bas_mixed} : rreg zerr";
                        if (kind == 3) res += ", zlen";
                        res += " : on sgn(zerr) goto __mixerr";
                        if (kind == 3) res += " : " + to + " = left$(zbuf$, zlen)";
                    } else {
                        // (zret 0 first: a proc that ends without return
                        // <expr> gives 0, as a compiled one does)
                        std::string zr = !to.empty() && trim(to.substr(0, to.find('('))).back() == '$' ? "zret$" : "zret";
                        res += assigns + (to.empty() ? "" : zr + (zr == "zret" ? " = 0 : " : " = \"\" : ")) + "gosub " + name;
                        if (!to.empty()) res += " : " + to + " = " + zr;
                    }
                    i = e - 1;
                    continue;
                }
            }
            res += c;
        }
        return res;
    };

    // return <expr> (a proc's result, for call ... to v): zret = <expr> :
    // return; in a compiled proc (#577) the compiler's own RETURN <expr>
    auto lowerReturns = [&](const std::string& code, int lineNum, bool inProc) {
        std::string res;
        bool inString = false;
        for (size_t i = 0; i < code.size(); i++) {
            char c = code[i];
            if (c == '"') inString = !inString;
            // (REM and DATA take the rest of the line as text)
            if (!inString && (i == 0 || !(std::isalnum((unsigned char)code[i - 1]) || code[i - 1] == '_')))
                for (const char* w : {"rem", "data"}) {
                    size_t n = strlen(w);
                    if (lower(code.substr(i, n)) == w &&
                        (i + n == code.size() || !(std::isalnum((unsigned char)code[i + n]) || code[i + n] == '_')))
                        return res + code.substr(i);
                }
            if (!inString && (i == 0 || !(std::isalnum((unsigned char)code[i - 1]) || code[i - 1] == '_')) &&
                i + 6 < code.size() && lower(code.substr(i, 6)) == "return" &&
                (code[i + 6] == ' ' || code[i + 6] == '\t' || code[i + 6] == '(')) {
                size_t v0 = code.find_first_not_of(" \t", i + 6), v1 = v0;
                if (v0 == std::string::npos || code[v0] == ':' || lower(code.substr(v0, 4)) == "else") {
                    res += c;
                    continue;
                }
                int depth = 0;
                bool q = false;
                for (; v1 < code.size(); v1++) {
                    char d = code[v1];
                    if (d == '"') q = !q;
                    if (q) continue;
                    if (d == '(') depth++;
                    else if (d == ')') depth--;
                    else if (depth == 0 && (d == ':' || (lower(code.substr(v1, 4)) == "else" && code[v1 - 1] == ' ')))
                        break;
                }
                std::string ex = trim(code.substr(v0, v1 - v0));
                if (!inProc) error(lineNum, "return " + ex + ": a value is returned only from a proc");
                // (a compiled proc's: ZRET too, for a compiled caller's
                // GOSUB, #681; RETURN ZRET leaves to BASIC at the top)
                // (a string: ZRET$, #680)
                std::string zr = isStringValue(ex) ? "zret$" : "zret";
                res += zr + " = " + ex + " : return" + (unit ? " " + zr : "") + (v1 < code.size() ? " " : "");
                i = v1 - 1;
                continue;
            }
            res += c;
        }
        return res;
    };

    auto resolvePath = [&](const std::string& name, int lineNum) {
        fs::path p(name);
        if (p.is_absolute()) return p;
        if (fileOf) {
            fs::path base = fs::path(fileOf(lineNum)).parent_path();
            if (fs::exists(base / p)) return fs::absolute(base / p);
        }
        return fs::absolute(p);
    };

    // skip: a proc's GOTO around it (the line's index, -1: none). test: the
    // line of a select's current case test (-1: none, case else), pos: what
    // the case tests
    struct Block { char kind; int id; bool hasElse; int line; int cases; std::string var;
                   int skip = -1; int test = -1; std::string pos; };
    std::vector<Block> stack;
    int nextId = 1;

    // A case's statements on the line of its test (#605): "if v=N then
    // <statements> : goto <end>" instead of a jump over them to the next
    // case's line. Where they are plain statements (no label, no IF: its
    // THEN would take the GOTO with it; no REM or DATA, which take the rest
    // of the line; no block) and the line stays within what --minify joins.
    // last: the select's last case, which needs no GOTO.
    auto closeCase = [&](Block& b, bool last) {
        if (b.test < 0) return false;
        std::string body;
        for (size_t k = b.test + 1; k < lines.size(); k++) {
            std::string t = trim(lines[k]);
            if (t.empty() || isComment(t)) continue;
            std::smatch lm;
            if (t[0] == '#' || std::regex_match(t, loneLabelRe) ||
                (std::regex_match(t, lm, labelRe) && !basic45IsReservedWord(lm[1].str())))
                return false;
            for (const char* w : {"if", "then", "else", "rem", "data", "begin", "bend"})
                if (hasWord(t, w)) return false;
            body += (body.empty() ? "" : " : ") + t;
        }
        std::string end = "__endsel" + std::to_string(b.id);
        std::string line = "if " + b.pos + " then ";
        if (body.empty()) {
            if (last) return false;
            line += end;
        } else {
            line += body;
            if (!last && !neverFallsThrough(body)) line += " : goto " + end;
        }
        if (line.size() > 160) return false;
        for (size_t k = lines.size() - 1; k > (size_t)b.test; k--) {
            std::string t = trim(lines[k]);
            if (t.empty() || isComment(t)) continue;
            lines.erase(lines.begin() + k);
            r.origin.erase(r.origin.begin() + k);
        }
        lines[b.test] = line;
        b.test = -1;
        chainEnd = chainGoto = -1;
        reach = true;
        return true;
    };
    std::string scope = "main";          // for .local labels
    bool inAsm = false;
    int asmLine = 0;
    std::string asmBlock;
    std::vector<std::string> asmGlobals;

    std::istringstream in(source);
    std::string raw;
    int lineNum = 0;
    while (std::getline(in, raw)) {
        lineNum++;
        std::string t = trim(raw);

        // assembly lines go to the asm text
        if (inAsm) {
            if (lower(t) == "endasm") {
                inAsm = false;
                r.asmText += "; asm block (line " + std::to_string(asmLine) + ")\n.segment \"code\"\n";
                for (const auto& g : asmGlobals) r.asmText += ".global " + g + "\n";
                r.asmText += asmBlock;
                asmBlock.clear();
                asmGlobals.clear();
                continue;
            }
            std::smatch am;
            std::string at = trim(raw);
            if (std::regex_search(at, am, asmLabelRe)) {
                asmGlobals.push_back(am[1].str());
                r.asmLabels.insert(am[1].str());
            }
            asmBlock += raw + "\n";
            continue;
        }

        if (t.empty() || isComment(t) || t[0] == '#') {
            emit(raw, lineNum);
            continue;
        }

        // a ' comment after the statement: dropped (#583)
        t = stripTrailingComment(t);
        std::string line = localize(t, scope);
        std::string label, code = line;
        std::smatch m;
        if (std::regex_match(line, m, labelRe)) { label = m[1].str(); code = trim(m[2].str()); }
        std::string prefix = label.empty() ? "" : label + ": ";
        std::string lc = lower(code);
        auto labelAlone = [&]() { if (!label.empty()) emit(label + ":", lineNum); };

        if (lc == "asm") {
            labelAlone();
            inAsm = true;
            asmLine = lineNum;
        } else if (std::regex_match(code, m, incbinRe)) {
            labelAlone();
            fs::path p = resolvePath(m[1].str(), lineNum);
            std::string name = m[2].str();
            std::error_code ec;
            auto size = fs::file_size(p, ec);
            if (ec) { error(lineNum, "incbin: cannot read " + m[1].str()); continue; }
            r.asmText += "; incbin " + m[1].str() + " (line " + std::to_string(lineNum) + ")\n";
            r.asmText += ".segment \"data\"\n.global " + name + "\n.global " + name + "_size\n";
            r.asmText += name + ":\n.incbin \"" + p.string() + "\"\n";
            r.asmText += name + "_size = " + std::to_string(size) + "\n";
            r.dataLabels.insert(name);
            r.dataLabels.insert(name + "_size");
        } else if (std::regex_match(code, m, dataincRe)) {
            fs::path p = resolvePath(m[1].str(), lineNum);
            std::ifstream f(p, std::ios::binary);
            if (!f) { error(lineNum, "datainc: cannot read " + m[1].str()); continue; }
            std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            int per = m[3].matched ? std::stoi(m[3].str()) : 16;
            if (per < 1) per = 16;
            std::string first = !label.empty() ? label + ": " : (m[5].matched ? m[5].str() + ": " : "");
            if (!label.empty() && m[5].matched) emit(m[5].str() + ":", lineNum);
            if (bytes.empty()) { error(lineNum, "datainc: " + m[1].str() + " is empty"); continue; }
            for (size_t k = 0; k < bytes.size(); k += per) {
                std::string d = "data ";
                for (size_t j = k; j < bytes.size() && j < k + (size_t)per; j++)
                    d += (j > k ? "," : "") + std::to_string(bytes[j]);
                emit((k == 0 ? first : "") + d, lineNum);
            }
        } else if (std::regex_match(code, m, ifThenRe) && std::count(code.begin(), code.end(), '"') % 2 == 0) {
            // (an even number of quotes: the final THEN is not inside a string)
            int id = nextId++;
            stack.push_back(Block{'i', id, false, lineNum, 0, ""});
            std::string opp = oppositeCond(m[1].str());
            emit(prefix + "if " + (opp.empty() ? "(" + m[1].str() + ")=0" : opp) + " then __else" + std::to_string(id),
                 lineNum);
        } else if (lc == "else" && !stack.empty() && stack.back().kind == 'i') {
            Block& b = stack.back();
            if (b.hasElse) { error(lineNum, "second else in one if block"); continue; }
            b.hasElse = true;
            labelAlone();
            emit("goto __endif" + std::to_string(b.id), lineNum);
            emit("__else" + std::to_string(b.id) + ":", lineNum);
        } else if (lc == "endif" || lc == "end if") {
            if (stack.empty() || stack.back().kind != 'i') { error(lineNum, "endif without a block if"); continue; }
            Block b = stack.back();
            stack.pop_back();
            labelAlone();
            emit((b.hasElse ? "__endif" : "__else") + std::to_string(b.id) + ":", lineNum);
        } else if (std::regex_match(code, m, procRe)) {
            // (its name is a label: a keyword there would be the statement,
            // "call directory" a GOSUB of the DIRECTORY token, #584)
            if (basic45IsReservedWord(m[1].str()))
                error(lineNum, "proc name '" + m[1].str() + "' is a BASIC keyword");
            if (unit) {
                // a compiled proc (#577): its label and its parameters,
                // read by the compiled program's entry; a number each
                std::vector<std::string> ps;
                if (m[2].matched && !trim(m[3].str()).empty()) ps = splitTop(m[3].str());
                for (const auto& p : ps)
                    if (p.empty() || p.back() == '&' || p.find('(') != std::string::npos)
                        error(lineNum, "proc " + m[1].str() + " compiled: a parameter is a float, % or string variable, not " + p);
                if (!stack.empty()) error(lineNum, "proc inside a block");
                stack.push_back(Block{'p', nextId++, false, lineNum, 0, ""});
                curProc = lower(m[1].str());
                labelAlone();
                // (a label of its own: the line is reached, by the entry)
                emit("__cp_" + m[1].str() + ":", lineNum);
                std::string imp = "import";
                for (size_t k = 0; k < ps.size(); k++) imp += (k ? ", " : " ") + ps[k];
                emit(imp, lineNum);
                r.compiledProcs.push_back(m[1].str());
                r.compiledArgs = std::max(r.compiledArgs, ps.size());
                scope = "p" + lower(m[1].str());
                continue;
            }
            int id = nextId++;
            stack.push_back(Block{'p', id, false, lineNum, 0, ""});
            // The GOTO around the proc (#605): none where the line before
            // cannot run into it (a GOTO, RETURN, END; a proc that needed
            // none); right after another proc, that proc's GOTO jumps
            // around this one too
            int ce = chainEnd, cg = chainGoto;
            labelAlone();
            if (label.empty() && ce >= 0 && cg >= 0) {
                lines[cg] = "goto __endproc" + std::to_string(id);
                lines.erase(lines.begin() + ce);
                r.origin.erase(r.origin.begin() + ce);
                stack.back().skip = cg;
            } else if (!label.empty() || reach) {
                emit("goto __endproc" + std::to_string(id), lineNum);
                stack.back().skip = (int)lines.size() - 1;
            }
            emit(m[1].str() + ":", lineNum);
            r.procs.push_back({m[1].str(), lineNum});
            scope = "p" + lower(m[1].str());
        } else if (lc == "endproc" || lc == "end proc") {
            if (stack.empty() || stack.back().kind != 'p') {
                error(lineNum, "endproc without proc (or a block is still open)");
                continue;
            }
            Block b = stack.back();
            stack.pop_back();
            // (a compiled proc's end: its result 0, #577; none after a
            // RETURN that cannot fall through)
            if (!unit) emit(prefix + "return", lineNum);
            else if (!label.empty() || reach) emit(prefix + "return 0", lineNum);
            if (b.skip >= 0) {
                emit("__endproc" + std::to_string(b.id) + ":", lineNum);
                chainEnd = (int)lines.size() - 1;
                chainGoto = b.skip;
            }
            scope = "main";
        } else if (std::regex_match(code, m, selectRe) && lower(code.substr(0, 7)) == "select ") {
            int id = nextId++;
            std::string expr = m[1].str();
            std::string var = "zsel" + std::to_string(id) + (isStringExpr(expr) ? "$" : "");
            stack.push_back(Block{'s', id, false, lineNum, 0, var});
            emit(prefix + var + " = " + expr, lineNum);
        } else if (std::regex_match(code, m, caseRe) && lower(code.substr(0, 5)) == "case ") {
            if (stack.empty() || stack.back().kind != 's') { error(lineNum, "case outside select"); continue; }
            Block& b = stack.back();
            if (b.hasElse) { error(lineNum, "case after case else"); continue; }
            std::string sel = std::to_string(b.id);
            if (b.cases > 0 && !closeCase(b, false)) {
                if (reach) emit("goto __endsel" + sel, lineNum);
                emit("__case" + sel + "_" + std::to_string(b.cases + 1) + ":", lineNum);
            }
            labelAlone();
            b.cases++;
            b.test = -1;
            std::string what = trim(m[1].str());
            if (lower(what) == "else") {
                b.hasElse = true;
                continue;
            }
            // What the case tests, and its opposite (the jump to the next
            // case): v=a or v=b / v<>a and v<>b where the items are plain
            // operands (#605), else (v=a) or (v=b) / (...)=0
            std::string cond, pos, neg;
            bool plain = true;
            std::vector<std::string> items = splitTop(what);
            for (const auto& item : items) {
                std::smatch im;
                std::string c, p, n;
                if (std::regex_match(item, im, rangeRe)) {
                    c = "(" + b.var + ">=" + im[1].str() + " and " + b.var + "<=" + im[2].str() + ")";
                    p = b.var + ">=" + im[1].str() + " and " + b.var + "<=" + im[2].str();
                    n = b.var + "<" + im[1].str() + " or " + b.var + ">" + im[2].str();
                    if (items.size() > 1) n = "(" + n + ")";
                    plain = plain && plainOperand(im[1].str()) && plainOperand(im[2].str());
                } else if (std::regex_match(item, im, isRe)) {
                    std::string op = im[1].str();
                    std::string inv = op == "=" ? "<>" : op == "<>" ? "=" : op == "<" ? ">=" :
                                      op == ">" ? "<=" : op == "<=" ? ">" : "<";
                    c = "(" + b.var + op + im[2].str() + ")";
                    p = b.var + op + im[2].str();
                    n = b.var + inv + im[2].str();
                    plain = plain && plainOperand(im[2].str());
                } else {
                    c = "(" + b.var + "=" + item + ")";
                    p = b.var + "=" + item;
                    n = b.var + "<>" + item;
                    plain = plain && plainOperand(item);
                }
                cond += (cond.empty() ? "" : " or ") + c;
                pos += (pos.empty() ? "" : " or ") + p;
                neg += (neg.empty() ? "" : " and ") + n;
            }
            if (!plain) { pos = cond; neg = "(" + cond + ")=0"; }
            emit("if " + neg + " then __case" + sel + "_" + std::to_string(b.cases + 1), lineNum);
            b.test = (int)lines.size() - 1;
            b.pos = pos;
        } else if (lc == "endselect" || lc == "end select") {
            if (stack.empty() || stack.back().kind != 's') { error(lineNum, "endselect without select"); continue; }
            Block b = stack.back();
            stack.pop_back();
            bool joined = b.cases > 0 && closeCase(b, true);
            labelAlone();
            std::string sel = std::to_string(b.id);
            if (b.cases > 0 && !b.hasElse && !joined)
                emit("__case" + sel + "_" + std::to_string(b.cases + 1) + ":", lineNum);
            emit("__endsel" + sel + ":", lineNum);
        } else {
            emit(prefix + lowerReturns(lowerCalls(code, lineNum), lineNum, scope != "main"), lineNum);
        }
    }
    if (inAsm) error(asmLine, "asm without endasm");
    if (unit && !r.asmText.empty()) error(1, "asm, incbin: not in a compiled proc");
    // a compiled proc's jumps (#682): to its own labels only (its program is
    // the compiled procs alone: the interpreted program's labels are not in
    // it, another compiled proc is reached by call)
    if (unit) {
        std::map<std::string, std::string> labelProc;   // lower-case label -> its proc
        static const std::regex defRe(R"(^\s*([A-Za-z_]\w*)\s*:)");
        for (size_t k = 0; k < lines.size(); k++) {
            std::smatch dm;
            if (std::regex_search(lines[k], dm, defRe) && !basic45IsReservedWord(dm[1].str()))
                labelProc[lower(dm[1].str())] = lineProc[k];
        }
        static const std::regex jumpRe(R"(\b(goto|go\s+to|gosub|then|else|trap|restore|resume)\s+([A-Za-z_]\w*(\s*,\s*[A-Za-z_]\w*)*))",
                                       std::regex::icase);
        static const std::regex nameRe(R"([A-Za-z_]\w*)");
        for (size_t k = 0; k < lines.size(); k++) {
            // (strings and REM / DATA text left out)
            std::string t, l = lines[k];
            bool q = false;
            for (size_t c = 0; c < l.size(); c++) {
                if (l[c] == '"') q = !q;
                if (!q && (c == 0 || !std::isalnum((unsigned char)l[c - 1])) &&
                    (lower(l.substr(c, 4)) == "rem " || lower(l.substr(c, 5)) == "data "))
                    break;
                t += q ? ' ' : l[c];
            }
            std::smatch jm;
            for (size_t from = 0; from < t.size() && std::regex_search(t.cbegin() + from, t.cend(), jm, jumpRe);) {
                std::string kw = lower(jm[1].str());
                std::string list = jm[2].str();
                size_t listAt = from + jm.position(2);
                size_t after = from + jm.position(0) + jm.length(0);
                // (the next search from the list: THEN GOTO x reads GOTO x)
                from = listAt;
                // (THEN / ELSE x = ...: an assignment, not a label)
                size_t nx = t.find_first_not_of(" \t", after);
                if ((kw == "then" || kw == "else") && nx != std::string::npos && (t[nx] == '=' || t[nx] == '('))
                    continue;
                for (auto nt = std::sregex_iterator(list.begin(), list.end(), nameRe); nt != std::sregex_iterator(); ++nt) {
                    std::string name = nt->str();
                    if (basic45IsReservedWord(name)) break;   // (THEN PRINT, RESUME NEXT ...)
                    if (name.compare(0, 5, "__cp_") == 0) continue;   // (a call of a compiled proc, #681)
                    auto lp = labelProc.find(lower(name));
                    std::string shown = name.compare(0, 4, "__l") == 0 ? "." + name.substr(name.find('_', 3) + 1) : name;
                    if (lp == labelProc.end())
                        error(r.origin[k], kw + " " + shown + ": a compiled proc cannot jump out of itself (" + shown +
                                           " is not one of its labels)");
                    else if (lp->second != lineProc[k])
                        error(r.origin[k], kw + " " + shown + ": " + shown + " is in compiled proc " + lp->second +
                                           " (call it)");
                    if (kw == "then" || kw == "else" || kw == "trap" || kw == "restore" || kw == "resume") break;
                }
            }
        }
    }
    for (const auto& b : stack)
        error(b.line, b.kind == 'i' ? "if block without endif" :
                      b.kind == 's' ? "select without endselect" : "proc without endproc");
    // (the compiled procs' error exit, #577: after the program's last line)
    if (mode == BasicLowerMode::Interpreted && !compiledK.empty()) {
        lines.push_back("__mixerr: end");
        r.origin.push_back(lineNum);
        // (a string result's buffer, #680)
        // (by concatenation: in BASIC's string space, bank 1, not a literal in
        // the program's text; no RPT$, which ROM 920413's interpreter lacks)
        if (usesZbuf)
            lines.push_back("__mixbuf: if len(zbuf$) < 255 then zbuf$ = \"\" : for zi = 1 to 17 : zbuf$ = zbuf$ + \"" +
                            std::string(15, ' ') + "\" : next");
        if (usesZbuf) r.origin.push_back(lineNum);
        if (usesZbuf) lines.push_back("return");
        if (usesZbuf) r.origin.push_back(lineNum);
    }
    for (const auto& l : lines) r.text += l + "\n";
    return r;
}
