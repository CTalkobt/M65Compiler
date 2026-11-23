#include "BasicFormatter.hpp"
#include "BasicTokenizer.hpp"
#include <algorithm>
#include <cctype>
#include <regex>
#include <set>
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

bool isWordChar(char c) { return std::isalnum((unsigned char)c) || c == '_' || c == '$' || c == '#'; }

// Keywords to lower case and blanks collapsed, outside strings; the text
// after REM, and DATA items, as written
std::string tidy(const std::string& code, const std::set<std::string>& keywords) {
    std::string out;
    bool inString = false;
    for (size_t i = 0; i < code.size(); i++) {
        char c = code[i];
        if (c == '"') { inString = !inString; out += c; continue; }
        if (inString) { out += c; continue; }
        if (c == ' ' || c == '\t') {
            if (!out.empty() && out.back() != ' ') out += ' ';
            continue;
        }
        if (std::isalpha((unsigned char)c) && (i == 0 || !isWordChar(code[i - 1]))) {
            size_t e = i;
            while (e < code.size() && isWordChar(code[e])) e++;
            std::string word = code.substr(i, e - i);
            std::string up = word;
            std::transform(up.begin(), up.end(), up.begin(), ::toupper);
            if (keywords.count(up)) {
                out += lower(word);
                if (up == "REM") { out += code.substr(e); return out; }
                if (up == "DATA") {
                    // the items as written, up to a ':' outside quotes
                    bool q = false;
                    size_t k = e;
                    for (; k < code.size(); k++) {
                        if (code[k] == '"') q = !q;
                        if (!q && code[k] == ':') break;
                    }
                    out += code.substr(e, k - e);
                    i = k - 1;
                    continue;
                }
            } else {
                out += word;
            }
            i = e - 1;
            continue;
        }
        out += c;
    }
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

}  // namespace

std::string formatBasic(const std::string& source) {
    std::set<std::string> keywords;
    {
        BasicTokenizer t;
        for (const auto& [k, v] : t.getKeywords()) keywords.insert(k);
        for (const auto& [k, v] : t.getEscapeKeywords()) keywords.insert(k);
        for (const auto& [k, v] : t.getCeKeywords()) keywords.insert(k);
        for (const char* k : {"ENDIF", "PROC", "ENDPROC", "CALL", "SELECT", "CASE", "ENDSELECT", "IS",
                              "ASM", "ENDASM", "INCBIN", "DATAINC", "AS"})
            keywords.insert(k);
    }
    static const std::regex labelRe(R"(^([A-Za-z_.]\w*)\s*:(\s+(.*)|$))");
    static const std::regex ifThen(R"(^if\b.*\bthen$)", std::regex::icase);

    std::ostringstream out;
    std::istringstream in(source);
    std::string raw;
    int level = 0;
    bool inAsm = false;
    while (std::getline(in, raw)) {
        std::string t = trim(raw);
        if (inAsm) {
            if (lower(t) == "endasm") { inAsm = false; out << std::string(2 + 2 * level, ' ') << "endasm\n"; }
            else out << raw << "\n";
            continue;
        }
        if (t.empty()) { out << "\n"; continue; }
        if (t[0] == '#') { out << t << "\n"; continue; }
        if (t[0] == '\'') { out << std::string(2 + 2 * level, ' ') << t << "\n"; continue; }

        std::string label, code = t;
        std::smatch m;
        if (std::regex_match(t, m, labelRe)) {
            label = m[1].str();
            code = m[3].matched ? trim(m[3].str()) : "";
        }
        code = tidy(code, keywords);
        std::string lc = lower(code);
        std::string first = lc.substr(0, lc.find(' '));

        int printLevel = level;
        if (first == "endif" || lc == "end if" || first == "endproc" || lc == "end proc" || first == "next" ||
            first == "loop" || first == "endasm") {
            level = std::max(0, level - 1);
            printLevel = level;
        } else if (first == "endselect" || lc == "end select") {
            level = std::max(0, level - 2);
            printLevel = level;
        } else if (first == "else" && lc == "else") {
            printLevel = std::max(0, level - 1);
        } else if (first == "case") {
            printLevel = std::max(0, level - 1);
        }

        if (!label.empty()) out << label << ":" << (code.empty() ? "" : " " + code) << "\n";
        else out << std::string(2 + 2 * printLevel, ' ') << code << "\n";

        // blocks opened and closed by this line's statements (split at ':'
        // outside strings): FOR / NEXT and DO / LOOP anywhere in the line,
        // "next i, j" closing two (#739: only a line starting with NEXT
        // closed a FOR, so "print i : next" left the rest indented)
        std::vector<std::string> stmts;
        {
            std::string cur;
            bool inStr = false;
            for (char ch : lc) {
                if (ch == '"') inStr = !inStr;
                if (ch == ':' && !inStr) { stmts.push_back(trim(cur)); cur.clear(); }
                else cur += ch;
            }
            stmts.push_back(trim(cur));
        }
        auto word = [](const std::string& st) { return st.substr(0, st.find_first_of(" \t")); };
        int delta = 0;
        for (size_t k = 0; k < stmts.size(); k++) {
            std::string w = word(stmts[k]);
            if (w == "for" || (w == "do" && stmts[k] == "do") || (w == "do" && stmts[k].rfind("do ", 0) == 0)) delta++;
            else if (w == "next" || w == "loop") {
                int n = 1;
                if (w == "next") n = std::max<int>(1, (int)std::count(stmts[k].begin(), stmts[k].end(), ',') + 1);
                // (the first statement's NEXT / LOOP is the dedent above)
                delta -= (k == 0) ? n - 1 : n;
            }
        }
        if (std::regex_match(lc, ifThen) && std::count(lc.begin(), lc.end(), '"') % 2 == 0) level++;
        else if (first == "proc") level++;
        else if (first == "select") level += 2;
        else if (lc == "asm") inAsm = true;
        level = std::max(0, level + delta);
    }
    return out.str();
}
