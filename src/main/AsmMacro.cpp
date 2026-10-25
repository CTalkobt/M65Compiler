// AsmMacro.cpp — ca45's multi-line macros (see AsmMacro.hpp)
#include "AsmMacro.hpp"
#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>
#include <vector>

namespace {

struct SrcLine {
    std::string text;
    std::string file;
    int line;
};

struct AsmMacroDef {
    std::vector<std::string> params;
    std::vector<std::string> body;
};

bool isIdStart(char c) { return std::isalpha((unsigned char)c) || c == '_'; }
bool isId(char c) { return std::isalnum((unsigned char)c) || c == '_'; }

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

// where a comment starts (';' outside a string or character constant), or npos
size_t commentAt(const std::string& s) {
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if (c == '"' || c == '\'') {
            for (i++; i < s.size() && s[i] != c; i++) if (s[i] == '\\') i++;
        } else if (c == ';') {
            return i;
        }
    }
    return std::string::npos;
}

std::string trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r");
    return s.substr(b, e - b + 1);
}

// ".macro" / ".endmacro" / ".endm" as the line's first word (lower case), else ""
std::string directiveOf(const std::string& code) {
    std::string t = trim(code);
    if (t.empty() || t[0] != '.') return "";
    size_t e = 1;
    while (e < t.size() && isId(t[e])) e++;
    std::string d = lower(t.substr(0, e));
    return (d == ".macro" || d == ".endmacro" || d == ".endm") ? d : "";
}

// the arguments of a use: split at the commas outside ( ) [ ] and literals
std::vector<std::string> splitArgs(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    int depth = 0;
    bool any = false;
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if (c == '"' || c == '\'') {
            size_t b = i;
            for (i++; i < s.size() && s[i] != c; i++) if (s[i] == '\\') i++;
            cur += s.substr(b, i - b + (i < s.size() ? 1 : 0));
            any = true;
            continue;
        }
        if (c == '(' || c == '[') depth++;
        else if ((c == ')' || c == ']') && depth > 0) depth--;
        if (c == ',' && depth == 0) { out.push_back(trim(cur)); cur.clear(); any = true; continue; }
        cur += c;
        if (!std::isspace((unsigned char)c)) any = true;
    }
    if (any) out.push_back(trim(cur));
    return out;
}

// a body line with the parameters' arguments put in (whole words outside
// literals and the comment) and \@ the expansion's number
std::string substitute(const std::string& line, const AsmMacroDef& m, const std::vector<std::string>& args,
                       const std::string& unique) {
    std::string out;
    size_t cut = commentAt(line);
    std::string code = line.substr(0, cut), comment = cut == std::string::npos ? "" : line.substr(cut);
    for (size_t i = 0; i < code.size();) {
        char c = code[i];
        if (c == '"' || c == '\'') {
            size_t b = i;
            for (i++; i < code.size() && code[i] != c; i++) if (code[i] == '\\') i++;
            if (i < code.size()) i++;
            out += code.substr(b, i - b);
        } else if (c == '\\' && i + 1 < code.size() && code[i + 1] == '@') {
            out += unique;
            i += 2;
        } else if (isIdStart(c) && (i == 0 || !isId(code[i - 1]))) {
            size_t b = i;
            while (i < code.size() && isId(code[i])) i++;
            std::string w = code.substr(b, i - b);
            auto p = std::find(m.params.begin(), m.params.end(), w);
            out += p == m.params.end() ? w : args[(size_t)(p - m.params.begin())];
        } else {
            out += c;
            i++;
        }
    }
    return out + comment;
}

class Expander {
public:
    std::map<std::string, AsmMacroDef> macros;
    std::vector<SrcLine> out;
    std::string err;
    int counter = 0;

    bool run(const std::vector<SrcLine>& lines, int depth) {
        if (depth > 64) {
            err = (lines.empty() ? std::string() : where(lines[0])) +
                  "macro expansion nested over 64 levels (a macro that uses itself?)";
            return false;
        }
        for (size_t i = 0; i < lines.size(); i++) {
            const SrcLine& l = lines[i];
            size_t cut = commentAt(l.text);
            std::string code = l.text.substr(0, cut);
            std::string d = directiveOf(code);
            if (d == ".macro") {
                if (!define(lines, i)) return false;
                continue;
            }
            if (d == ".endmacro" || d == ".endm") {
                err = where(l) + d + " without .macro";
                return false;
            }
            // [label:] NAME [arguments]
            std::string t = trim(code);
            size_t p = 0;
            std::string label;
            if (!t.empty() && (isIdStart(t[0]) || t[0] == '@')) {
                size_t e = 1;
                while (e < t.size() && isId(t[e])) e++;
                if (e < t.size() && t[e] == ':' && (e + 1 >= t.size() || t[e + 1] != '=')) {
                    label = t.substr(0, e + 1);
                    p = e + 1;
                    while (p < t.size() && std::isspace((unsigned char)t[p])) p++;
                }
            }
            size_t e = p;
            while (e < t.size() && isId(t[e])) e++;
            auto mi = e > p && isIdStart(t[p]) ? macros.find(t.substr(p, e - p)) : macros.end();
            // (NAME = value is an assignment, not a use)
            size_t after = e;
            while (after < t.size() && std::isspace((unsigned char)t[after])) after++;
            if (mi == macros.end() || (after < t.size() && t[after] == '=')) {
                out.push_back(l);
                continue;
            }
            const AsmMacroDef& m = mi->second;
            std::vector<std::string> args = splitArgs(t.substr(e));
            if (args.size() > m.params.size()) {
                err = where(l) + "macro " + mi->first + " takes " + std::to_string(m.params.size()) +
                      " argument(s), given " + std::to_string(args.size());
                return false;
            }
            args.resize(m.params.size());
            if (!label.empty()) out.push_back({label, l.file, l.line});
            std::string unique = std::to_string(++counter);
            std::vector<SrcLine> body;
            for (const auto& b : m.body) body.push_back({substitute(b, m, args, unique), l.file, l.line});
            if (!run(body, depth + 1)) return false;
        }
        return true;
    }

private:
    static std::string where(const SrcLine& l) { return l.file + ":" + std::to_string(l.line) + ": "; }

    // .macro NAME [params] ... .endmacro (nested .macro ... .endmacro inside kept)
    bool define(const std::vector<SrcLine>& lines, size_t& i) {
        const SrcLine& head = lines[i];
        std::string t = trim(head.text.substr(0, commentAt(head.text)));
        size_t p = 6;   // after ".macro"
        while (p < t.size() && std::isspace((unsigned char)t[p])) p++;
        size_t e = p;
        while (e < t.size() && isId(t[e])) e++;
        if (e == p || !isIdStart(t[p])) {
            err = where(head) + ".macro without a name";
            return false;
        }
        std::string name = t.substr(p, e - p);
        AsmMacroDef m;
        std::string rest = t.substr(e);
        std::replace(rest.begin(), rest.end(), ',', ' ');
        std::istringstream ps(rest);
        for (std::string w; ps >> w;) {
            if (!isIdStart(w[0]) || !std::all_of(w.begin(), w.end(), isId)) {
                err = where(head) + "macro " + name + ": '" + w + "' is not a parameter name";
                return false;
            }
            m.params.push_back(w);
        }
        int nest = 1;
        for (i++; i < lines.size(); i++) {
            std::string d = directiveOf(lines[i].text.substr(0, commentAt(lines[i].text)));
            if (d == ".macro") nest++;
            else if ((d == ".endmacro" || d == ".endm") && --nest == 0) break;
            m.body.push_back(lines[i].text);
        }
        if (i >= lines.size()) {
            err = where(head) + ".macro " + name + " without .endmacro";
            return false;
        }
        macros[name] = std::move(m);
        return true;
    }
};

} // namespace

bool expandAsmMacros(const std::string& source, const std::string& file, std::string& out, std::string& err) {
    // (nothing to do without a .macro: the source as it is)
    {
        bool any = false;
        std::istringstream probe(source);
        for (std::string l; std::getline(probe, l);)
            if (directiveOf(l.substr(0, commentAt(l))) == ".macro") { any = true; break; }
        if (!any) { out = source; return true; }
    }
    // the lines with their file and line (the preprocessor's #line markers)
    std::vector<SrcLine> lines;
    std::string curFile = file;
    int curLine = 1;
    std::istringstream in(source);
    for (std::string l; std::getline(in, l);) {
        std::string t = trim(l);
        if (t.rfind("#line", 0) == 0) {
            std::istringstream ls(t.substr(5));
            int n = 0;
            std::string f;
            if (ls >> n) {
                if (ls >> f && f.size() >= 2 && f.front() == '"' && f.back() == '"') curFile = f.substr(1, f.size() - 2);
                curLine = n;
            }
            continue;
        }
        lines.push_back({l, curFile, curLine++});
    }
    Expander x;
    if (!x.run(lines, 0)) { err = x.err; return false; }
    // the result, a #line marker where a line is not the one after the last
    std::ostringstream os;
    std::string atFile;
    int next = -1;
    for (const auto& l : x.out) {
        if (l.file != atFile || l.line != next) os << "#line " << l.line << " \"" << l.file << "\"\n";
        os << l.text << "\n";
        atFile = l.file;
        next = l.line + 1;
    }
    out = os.str();
    return true;
}
