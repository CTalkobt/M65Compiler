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

        // blocks opened by this line
        bool hasNext = std::regex_search(lc, std::regex(R"(\bnext\b)"));
        bool hasLoop = std::regex_search(lc, std::regex(R"(\bloop\b)"));
        if (std::regex_match(lc, ifThen) && std::count(lc.begin(), lc.end(), '"') % 2 == 0) level++;
        else if (first == "proc") level++;
        else if (first == "select") level += 2;
        else if (first == "for" && !hasNext) level++;
        else if (first == "do" && !hasLoop) level++;
        else if (lc == "asm") inAsm = true;
    }
    return out.str();
}
