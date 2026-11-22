// al45's style rules (mega65-style-*) and af45, the formatter (#611;
// doc/bin/al45.md "Style rules", doc/bin/af45.md). The defaults are the
// library's own layout (lib/stdlib_asm, lib/*.s45): labels at column 1,
// instructions after 4 spaces, one space before the operand, trailing
// comments at column 29 (28 spaces: room for the code of 99% of the
// library's commented lines), no tabs or trailing spaces, a final newline.
#include "AsmStyle.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <vector>

#include "Logger.hpp"
#include "Version.hpp"

namespace asmstyle {

namespace {

const size_t npos = std::string::npos;

// A source line, split as ca45's lexer reads it: strings ("..." with
// backslash escapes), character literals ('x, 'x'), comments (; and //, /* */)
struct Line {
    std::string text;               // the line, its tabs outside literals expanded (8 columns)
    bool block = false;             // touches a /* */ comment: left as it is
    bool tab = false;               // the original had a tab outside a literal
    size_t tabCol = 0;
    size_t indent = 0;
    size_t codeStart = npos, codeEnd = 0;   // the code: [codeStart, codeEnd), trailing spaces out
    size_t commentStart = npos;     // ; or //
    bool slash = false;             // the comment is //
    // the code
    std::string label;              // "name" of "name:" (empty: none)
    size_t labelEnd = npos;         // after its ':'
    enum Kind { None, Instr, Directive, Proc, Const, Other } kind = None;
    std::string word;               // mnemonic, directive, proc / endproc, a constant's name
    size_t wordStart = npos, wordEnd = npos;
    size_t operandStart = npos;     // the first character after the word's spaces (npos: none)
    std::string procName;           // proc <name>
};

bool identStart(char c) { return std::isalpha((unsigned char)c) || c == '_' || c == '.'; }
bool identChar(char c) { return std::isalnum((unsigned char)c) || c == '_' || c == '.'; }

// The tabs outside literals expanded; tabStop columns of 8
std::string expandTabs(const std::string& s, bool& had, size_t& firstCol) {
    std::string out;
    bool inStr = false;
    int chr = 0;   // characters of a 'x literal still to copy
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if (inStr) {
            out += c;
            if (c == '\\' && i + 1 < s.size()) out += s[++i];
            else if (c == '"') inStr = false;
            continue;
        }
        if (chr) { out += c; chr--; continue; }
        if (c == '"') inStr = true;
        else if (c == '\'') chr = 1;
        else if (c == '\t') {
            if (!had) firstCol = out.size();
            had = true;
            do out += ' '; while (out.size() % 8);
            continue;
        }
        out += c;
    }
    return out;
}

// inBlock: a /* */ comment open from an earlier line
Line split(const std::string& raw, bool& inBlock) {
    Line L;
    L.text = expandTabs(raw, L.tab, L.tabCol);
    const std::string& t = L.text;
    size_t n = t.size();
    while (n && (t[n - 1] == ' ' || t[n - 1] == '\r')) n--;
    if (inBlock) {
        L.block = true;
        if (t.find("*/") != npos) inBlock = false;
        return L;
    }
    size_t end = n;
    bool inStr = false;
    for (size_t i = 0; i < n; i++) {
        char c = t[i];
        if (inStr) {
            if (c == '\\') i++;
            else if (c == '"') inStr = false;
            continue;
        }
        if (c == '"') { inStr = true; continue; }
        if (c == '\'') {   // 'x or 'x'
            i += 1;
            if (i + 1 < n && t[i + 1] == '\'') i++;
            continue;
        }
        if (c == ';') { L.commentStart = i; end = i; break; }
        if (c == '/' && i + 1 < n && t[i + 1] == '/') { L.commentStart = i; L.slash = true; end = i; break; }
        if (c == '/' && i + 1 < n && t[i + 1] == '*') {
            L.block = true;
            size_t close = t.find("*/", i + 2);
            if (close == npos) { inBlock = true; return L; }
            i = close + 1;
        }
    }
    if (L.block) return L;
    size_t cs = 0;
    while (cs < end && t[cs] == ' ') cs++;
    L.indent = cs;
    size_t ce = end;
    while (ce > cs && t[ce - 1] == ' ') ce--;
    if (cs >= ce) return L;   // blank, or a comment alone
    L.codeStart = cs;
    L.codeEnd = ce;
    size_t p = cs;
    // a label: name: (a colon only; ca45 has no labels without one)
    {
        size_t q = p;
        if (q < ce && t[q] == '@') q++;
        if (q < ce && identStart(t[q])) {
            while (q < ce && identChar(t[q])) q++;
            if (q < ce && t[q] == ':' && (q + 1 >= ce || t[q + 1] != ':')) {
                L.label = t.substr(p, q - p);
                L.labelEnd = q + 1;
                p = q + 1;
                while (p < ce && t[p] == ' ') p++;
            }
        }
    }
    if (p >= ce) return L;
    L.wordStart = p;
    size_t q = p;
    if (identStart(t[q])) {
        while (q < ce && identChar(t[q])) q++;
    } else {
        while (q < ce && t[q] != ' ') q++;
    }
    L.wordEnd = q;
    L.word = t.substr(p, q - p);
    size_t r = q;
    while (r < ce && t[r] == ' ') r++;
    if (r < ce) L.operandStart = r;
    std::string lw = L.word;
    for (auto& c : lw) c = (char)std::tolower((unsigned char)c);
    if (r < ce && t[r] == '=' && (r + 1 >= ce || t[r + 1] != '=') && identStart(t[p]) && t[p] != '.')
        L.kind = Line::Const;
    else if (lw == "proc" || lw == "endproc") {
        L.kind = Line::Proc;
        if (lw == "proc" && r < ce) {
            size_t e2 = r;
            while (e2 < ce && identChar(t[e2])) e2++;
            L.procName = t.substr(r, e2 - r);
        }
    } else if (t[p] == '.')
        L.kind = Line::Directive;
    else if (std::isalpha((unsigned char)t[p]) || t[p] == '_')
        L.kind = Line::Instr;
    else
        L.kind = Line::Other;
    return L;
}

std::vector<std::string> lines(const std::string& text) {
    std::vector<std::string> v;
    size_t s = 0;
    while (s < text.size()) {
        size_t e = text.find('\n', s);
        if (e == npos) { v.push_back(text.substr(s)); break; }
        v.push_back(text.substr(s, e - s));
        s = e + 1;
    }
    return v;
}

// Where the line's indentation should be (its kind, no label). dataCol: the
// column of the directive on the latest label line (npos: none), where the
// directives after it may line up (a table: label: .byte ... / .byte ...)
size_t wantIndent(const Line& L, const Options& o, size_t dataCol) {
    switch (L.kind) {
        case Line::Instr: return (size_t)o.instrCol;
        case Line::Directive:
            if (L.indent == 0 || L.indent == dataCol) return L.indent;
            return (size_t)o.instrCol;
        case Line::Proc: return L.indent == 0 ? 0 : (size_t)o.instrCol;
        case Line::Const: return 0;
        default: return L.indent;
    }
}

// The directives' column after a label line (see wantIndent), kept up to
// date line by line
void trackDataCol(const Line& L, size_t& dataCol) {
    if (L.block || L.codeStart == npos) return;
    if (!L.label.empty()) {
        if (L.kind == Line::Directive) dataCol = L.wordStart;
    } else if (L.kind == Line::Instr || L.kind == Line::Proc) {
        dataCol = npos;
    }
}

// Spaces between a mnemonic ending at column wordEndCol and its operand
size_t wantOperandGap(size_t wordEndCol, const Options& o) {
    if (o.operandCol > 0 && (size_t)o.operandCol > wordEndCol) return (size_t)o.operandCol - wordEndCol;
    return 1;
}

// Where a trailing comment after code ending at codeEnd should start (gap:
// the spaces it has now, kept when the code is too long for the column)
size_t wantCommentCol(size_t codeEnd, size_t gap, const Options& o) {
    if (codeEnd + 2 <= (size_t)o.commentCol) return (size_t)o.commentCol;
    return codeEnd + std::max<size_t>(2, gap);
}

std::set<std::string> exportedNames(const std::vector<std::string>& ls) {
    std::set<std::string> names;
    static const std::regex dir(R"(^\s*\.(global|globl|weak|extern|export|exportzp|import)\s+(.*))",
                                std::regex::icase);
    for (const auto& l : ls) {
        std::smatch m;
        std::string code = l.substr(0, l.find(';'));
        if (!std::regex_match(code, m, dir)) continue;
        std::stringstream ss(m[2].str());
        std::string n;
        while (std::getline(ss, n, ',')) {
            size_t a = n.find_first_not_of(" \t"), b = n.find_last_not_of(" \t");
            if (a != npos) names.insert(n.substr(a, b - a + 1));
        }
    }
    return names;
}

}  // namespace

bool optionsFrom(const lint::Config& c, Options& o, std::string& err) {
    auto num = [&](const char* id, int& out) {
        auto it = c.options.find(id);
        if (it == c.options.end()) return true;
        try {
            size_t used = 0;
            int v = std::stoi(it->second, &used);
            if (used != it->second.size() || v < 0 || v > 200) throw 0;
            out = v;
            return true;
        } catch (...) {
            err = std::string(id) + ": '" + it->second + "' is not a column (0-200)";
            return false;
        }
    };
    auto re = [&](const char* id, std::string& out) {
        auto it = c.options.find(id);
        if (it == c.options.end()) return true;
        try {
            std::regex r(it->second);
            out = it->second;
            return true;
        } catch (const std::regex_error& e) {
            err = std::string(id) + ": '" + it->second + "' is not a regular expression (" + e.what() + ")";
            return false;
        }
    };
    return num("mega65-style-instruction-column", o.instrCol) && num("mega65-style-operand-column", o.operandCol) &&
           num("mega65-style-comment-column", o.commentCol) && re("mega65-style-label-name", o.labelName) &&
           re("mega65-style-local-name", o.localName) && re("mega65-style-constant-name", o.constName);
}

void registerRules(lint::Engine& e) {
    using L = lint::Level;
    e.add({"mega65-style-whitespace", L::Off, "all",
           "Style: a tab, trailing spaces, or no newline at the end of the file",
           "Spaces only (a tab outside a string or character literal; af45 expands it to 8-column stops),\n"
           "no spaces at the end of a line, and a newline after the last line."});
    e.add({"mega65-style-label", L::Off, "all",
           "Style: a label (or a constant's definition) not at column 1",
           "Labels (name:, @name:) and constants (NAME = value) start at the beginning of the line; what\n"
           "follows a label on its line is left where it is."});
    e.add({"mega65-style-indent", L::Off, "all",
           "Style: an instruction not at the instruction column, or a directive neither there nor at column 1",
           "An instruction (or a macro's name used as one) starts after mega65-style-instruction-column spaces\n"
           "(default 4). A directive, proc or endproc starts at column 1 or at the instruction column. Lines\n"
           "with a label are not checked."});
    e.add({"mega65-style-operand", L::Off, "all",
           "Style: an instruction's operand not one space after the mnemonic (or at the operand column)",
           "One space between a mnemonic and its operand, or, with mega65-style-operand-column set, the operand\n"
           "at that column (one space when the mnemonic reaches it). Directives' operands are not checked."});
    e.add({"mega65-style-comment", L::Off, "all",
           "Style: a trailing comment not at the comment column, too close to the code, or a // comment",
           "Comments start with ';'. A trailing comment starts at mega65-style-comment-column (default 28: after\n"
           "28 characters, column 29), or at least two spaces after code that reaches past it. Comments on a\n"
           "line of their own are not checked."});
    e.add({"mega65-style-label-name", L::Off, "all",
           "Style: a global label's or proc's name does not match mega65-style-label-name",
           "Global labels (name:) and proc names are matched against the regular expression\n"
           "mega65-style-label-name (default ^[A-Za-z_.][A-Za-z0-9_.]*$: any)."});
    e.add({"mega65-style-local-name", L::Off, "all",
           "Style: a local label's name does not match mega65-style-local-name",
           "Local labels (@name:) are matched against mega65-style-local-name (default ^@[A-Za-z0-9_]+$)."});
    e.add({"mega65-style-constant-name", L::Off, "all",
           "Style: a constant's name does not match mega65-style-constant-name",
           "Constants (NAME = value) are matched against mega65-style-constant-name (default: any name; for\n"
           "UPPER_SNAKE_CASE, ^[A-Z][A-Z0-9_]*$). A name the file exports or imports (.global, .weak, .extern)\n"
           "is a symbol, not a constant, and is not checked."});
}

void check(lint::Engine& e, const std::string& file, const std::string& text) {
    Options o;
    std::string err;
    optionsFrom(e.config, o, err);   // (al45 checked them; the defaults otherwise)
    std::regex labelRe, localRe, constRe;
    try {
        labelRe = std::regex(o.labelName);
        localRe = std::regex(o.localName);
        constRe = std::regex(o.constName);
    } catch (...) {
        return;
    }
    std::vector<std::string> ls = lines(text);
    std::set<std::string> exported = exportedNames(ls);
    bool inBlock = false;
    size_t dataCol = npos;
    for (size_t k = 0; k < ls.size(); k++) {
        int no = (int)k + 1;
        const std::string& raw = ls[k];
        Line L = split(raw, inBlock);
        size_t dataHere = dataCol;
        trackDataCol(L, dataCol);
        // whitespace
        if (L.tab) e.report("mega65-style-whitespace", file, no, (int)L.tabCol + 1, "a tab: use spaces");
        {
            size_t n = raw.size();
            while (n && (raw[n - 1] == ' ' || raw[n - 1] == '\t' || raw[n - 1] == '\r')) n--;
            if (n < raw.size()) e.report("mega65-style-whitespace", file, no, (int)n + 1, "spaces at the end of the line");
        }
        if (k + 1 == ls.size() && !text.empty() && text.back() != '\n')
            e.report("mega65-style-whitespace", file, no, (int)raw.size() + 1, "no newline at the end of the file");
        if (L.block) continue;
        const std::string& t = L.text;
        // comments
        if (L.commentStart != npos) {
            if (L.slash)
                e.report("mega65-style-comment", file, no, (int)L.commentStart + 1, "a // comment: comments start with ';'");
            if (L.codeStart != npos) {
                size_t gap = L.commentStart - L.codeEnd;
                size_t want = wantCommentCol(L.codeEnd, gap, o);
                bool shortCode = L.codeEnd + 2 <= (size_t)o.commentCol;
                if (shortCode ? L.commentStart != want : gap < 2) {
                    std::string m = !shortCode ? "a trailing comment " + std::to_string(gap) +
                                                  (gap == 1 ? " space" : " spaces") + " after the code (at least 2)"
                                            : "a trailing comment at column " + std::to_string(L.commentStart + 1) +
                                                  ", not " + std::to_string(want + 1) + " (mega65-style-comment-column: " +
                                                  std::to_string(o.commentCol) + ")";
                    e.report("mega65-style-comment", file, no, (int)L.commentStart + 1, m);
                }
            }
        }
        if (L.codeStart == npos) continue;
        // labels and constants at column 1
        if (!L.label.empty() && L.indent > 0)
            e.report("mega65-style-label", file, no, (int)L.indent + 1,
                     "label '" + L.label + "' at column " + std::to_string(L.indent + 1) + ": labels start at column 1");
        if (L.label.empty() && L.kind == Line::Const && L.indent > 0)
            e.report("mega65-style-label", file, no, (int)L.indent + 1,
                     "constant '" + L.word + "' at column " + std::to_string(L.indent + 1) + ": definitions start at column 1");
        // indentation
        if (L.label.empty() && (L.kind == Line::Instr || L.kind == Line::Directive || L.kind == Line::Proc)) {
            size_t want = wantIndent(L, o, dataHere);
            if (L.indent != want) {
                std::string what = L.kind == Line::Instr ? "instruction '" + L.word + "'" : "'" + L.word + "'";
                std::string m = what + " at column " + std::to_string(L.indent + 1) + ", not " + std::to_string(want + 1);
                if (L.kind != Line::Instr) m += " (or 1)";
                e.report("mega65-style-indent", file, no, (int)L.indent + 1, m);
            }
        }
        // operand
        if (L.kind == Line::Instr && L.operandStart != npos) {
            size_t gap = L.operandStart - L.wordEnd, want = wantOperandGap(L.wordEnd, o);
            if (gap != want)
                e.report("mega65-style-operand", file, no, (int)L.operandStart + 1,
                         "the operand of '" + L.word + "' " + std::to_string(gap) + " spaces after it, not " + std::to_string(want));
        }
        // names
        if (!L.label.empty()) {
            bool local = L.label[0] == '@';
            if (!std::regex_match(L.label, local ? localRe : labelRe))
                e.report(local ? "mega65-style-local-name" : "mega65-style-label-name", file, no, (int)L.indent + 1,
                         std::string(local ? "local label" : "label") + " '" + L.label + "' does not match " +
                             (local ? o.localName : o.labelName));
        }
        if (L.kind == Line::Proc && !L.procName.empty() && !std::regex_match(L.procName, labelRe))
            e.report("mega65-style-label-name", file, no, (int)L.operandStart + 1,
                     "proc '" + L.procName + "' does not match " + o.labelName);
        if (L.kind == Line::Const && !exported.count(L.word) && !std::regex_match(L.word, constRe))
            e.report("mega65-style-constant-name", file, no, (int)L.wordStart + 1,
                     "constant '" + L.word + "' does not match " + o.constName);
        (void)t;
    }
}

std::string format(const std::string& text, const Options& o) {
    std::vector<std::string> ls = lines(text);
    std::string out;
    bool inBlock = false;
    // a trailing comment's column before and after: a comment alone on the
    // next line that starts where it did continues it, and moves with it
    size_t contFrom = npos, contTo = npos, dataCol = npos;
    for (const auto& raw : ls) {
        bool wasBlock = inBlock;
        Line L = split(raw, inBlock);
        size_t dataHere = dataCol;
        trackDataCol(L, dataCol);
        size_t prevFrom = contFrom, prevTo = contTo;
        contFrom = contTo = npos;
        if (L.block || wasBlock) {
            // (a /* */ comment: as it is, but for the spaces at the end)
            std::string r = raw;
            while (!r.empty() && (r.back() == ' ' || r.back() == '\t' || r.back() == '\r')) r.pop_back();
            out += r + "\n";
            continue;
        }
        const std::string& t = L.text;
        std::string line;
        if (L.codeStart != npos) {
            if (!L.label.empty()) {
                line = t.substr(L.codeStart, L.labelEnd - L.codeStart);
                if (L.wordStart != npos) {
                    // (what follows the label stays its distance after it)
                    size_t gap = std::max<size_t>(1, L.wordStart - L.labelEnd);
                    line += std::string(gap, ' ');
                }
            } else {
                line = std::string(wantIndent(L, o, dataHere), ' ');
            }
            if (L.wordStart != npos) {
                if (L.kind == Line::Instr && L.operandStart != npos) {
                    line += L.word;
                    line += std::string(wantOperandGap(line.size(), o), ' ');
                    line += t.substr(L.operandStart, L.codeEnd - L.operandStart);
                } else {
                    line += t.substr(L.wordStart, L.codeEnd - L.wordStart);
                }
            }
        } else if (L.commentStart != npos) {
            // (a comment alone: where it is, or under the one it continues)
            size_t at = L.commentStart == prevFrom ? prevTo : L.commentStart;
            line = std::string(at, ' ');
            if (L.commentStart == prevFrom) { contFrom = prevFrom; contTo = prevTo; }
        }
        if (L.commentStart != npos) {
            std::string c = t.substr(L.commentStart);
            while (!c.empty() && (c.back() == ' ' || c.back() == '\r')) c.pop_back();
            if (L.slash) c = ";" + c.substr(2);
            if (L.codeStart != npos) {
                size_t gap = L.commentStart - L.codeEnd;
                size_t col = wantCommentCol(line.size(), gap, o);
                line += std::string(col - line.size(), ' ');
                contFrom = L.commentStart;
                contTo = col;
            }
            line += c;
        }
        out += line + "\n";
    }
    return out;
}

// af45 ------------------------------------------------------------------------

namespace {

void usage(std::ostream& os) {
    os << "Usage: af45 [options] <file.s45>...\n"
          "The assembly formatter: lays a file out as al45's style rules describe (whitespace only:\n"
          "what ca45 assembles does not change). To standard output, or with -w the files rewritten.\n"
          "  -w, --write          Rewrite the files in place\n"
          "  --check              Change nothing; list the files that would change (exit status 1)\n"
          "  -o FILE              Write the one file's result to FILE\n"
          "  --config=FILE        This configuration, not ~/.config/m65/cc45-lint.yml and .cc45-lint.yml\n"
          "  --option=ID=VALUE    A rule option (mega65-style-comment-column=24, ...)\n"
          "  -v ... -vvvv         Log to stderr\n"
          "  -?, -h, --help       This help\n"
          "  -V, --version        The version\n"
          "Options (the configuration's options:): mega65-style-instruction-column (4),\n"
          "mega65-style-operand-column (0: one space), mega65-style-comment-column (28).\n"
          "See doc/bin/af45.md.\n";
}

std::string dirOf(const std::string& f) {
    size_t s = f.find_last_of('/');
    return s == npos ? "." : (s == 0 ? "/" : f.substr(0, s));
}

}  // namespace

int formatterMain(int argc, char** argv) {
    Logger::setTool("af45");
    argc = Logger::takeFlags(argc, argv);
    std::vector<std::string> args(argv + 1, argv + argc);
    bool write = false, checkOnly = false;
    std::string outFile, configFile;
    std::vector<std::pair<std::string, std::string>> setOpts;
    std::vector<std::string> files;
    for (size_t i = 0; i < args.size(); i++) {
        const std::string& a = args[i];
        if (a == "-?" || a == "-h" || a == "--help") { usage(std::cout); return 0; }
        if (a == "-V" || a == "--version") { std::cout << suiteVersionString("af45") << std::endl; return 0; }
        if (a == "-w" || a == "--write") write = true;
        else if (a == "--check") checkOnly = true;
        else if (a == "-o" && i + 1 < args.size()) outFile = args[++i];
        else if (a.rfind("--config=", 0) == 0) configFile = a.substr(9);
        else if (a.rfind("--option=", 0) == 0) {
            std::string kv = a.substr(9);
            size_t eq = kv.find('=');
            if (eq == npos || eq == 0) {
                std::cerr << "af45: --option=ID=VALUE, not '" << kv << "'" << std::endl;
                return 2;
            }
            setOpts.push_back({kv.substr(0, eq), kv.substr(eq + 1)});
        } else if (!a.empty() && a[0] == '-' && a != "-") {
            std::cerr << "af45: unknown option '" << a << "' (af45 --help)" << std::endl;
            return 2;
        } else
            files.push_back(a);
    }
    if (files.empty()) {
        usage(std::cerr);
        return 2;
    }
    if (!outFile.empty() && (files.size() != 1 || write || checkOnly)) {
        std::cerr << "af45: -o takes one file, without -w or --check" << std::endl;
        return 2;
    }
    if (files.size() > 1 && !write && !checkOnly) {
        std::cerr << "af45: several files: -w (rewrite them) or --check" << std::endl;
        return 2;
    }
    int status = 0;
    for (const auto& f : files) {
        lint::Config cfg;
        std::string err;
        bool ok = configFile.empty() ? cfg.loadDefaults(dirOf(f), err) : cfg.loadFile(configFile, err);
        for (const auto& [k, v] : setOpts) cfg.options[k] = v;
        Options o;
        if (!ok || !optionsFrom(cfg, o, err)) {
            std::cerr << "af45: error: " << err << std::endl;
            return 2;
        }
        for (const auto& s : cfg.sources) LOG_INFO("af45", "configuration " << s);
        std::ifstream in(f, std::ios::binary);
        if (!in) {
            std::cerr << "af45: cannot open " << f << std::endl;
            return 2;
        }
        std::stringstream ss;
        ss << in.rdbuf();
        std::string text = ss.str(), result = format(text, o);
        if (checkOnly) {
            if (result != text) { std::cout << f << std::endl; status = 1; }
            continue;
        }
        if (write || !outFile.empty()) {
            const std::string& dest = write ? f : outFile;
            if (write && result == text) continue;
            std::ofstream of(dest, std::ios::binary);
            if (!of) {
                std::cerr << "af45: cannot write " << dest << std::endl;
                return 2;
            }
            of << result;
            LOG_INFO("af45", "wrote " << dest);
            continue;
        }
        std::cout << result;
    }
    return status;
}

}  // namespace asmstyle
