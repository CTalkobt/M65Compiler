#include "BasicStructured.hpp"
#include <algorithm>
#include <cctype>
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

// Replace the word "call" (outside strings) followed by a name with "gosub"
std::string lowerCalls(const std::string& code) {
    std::string out;
    bool inString = false;
    for (size_t i = 0; i < code.size(); i++) {
        char c = code[i];
        if (c == '"') inString = !inString;
        if (!inString && (i == 0 || !std::isalnum((unsigned char)code[i - 1])) &&
            i + 4 <= code.size() && lower(code.substr(i, 4)) == "call" &&
            (i + 4 == code.size() || code[i + 4] == ' ' || code[i + 4] == '\t')) {
            size_t n = code.find_first_not_of(" \t", i + 4);
            if (n != std::string::npos && (std::isalpha((unsigned char)code[n]) || code[n] == '_')) {
                out += "gosub";
                i += 3;
                continue;
            }
        }
        out += c;
    }
    return out;
}

}  // namespace

BasicStructuredResult lowerBasicStructured(const std::string& source) {
    BasicStructuredResult r;
    std::ostringstream out;
    auto emit = [&](const std::string& line, int from) {
        out << line << "\n";
        r.origin.push_back(from);
    };

    // a leading label ("name: ...") is kept on the lowered line
    static const std::regex labelRe(R"(^([A-Za-z_]\w*)\s*:\s*(.*)$)");
    static const std::regex ifThenRe(R"(^if\s+(.*\S)\s+then$)", std::regex::icase);
    static const std::regex procRe(R"(^proc\s+([A-Za-z_]\w*)$)", std::regex::icase);

    struct Block { char kind; int id; bool hasElse; int line; };   // 'i' if, 'p' proc
    std::vector<Block> stack;
    int nextId = 1;

    std::istringstream in(source);
    std::string raw;
    int lineNum = 0;
    while (std::getline(in, raw)) {
        lineNum++;
        std::string line = trim(raw);
        std::string label, code = line;
        std::smatch m;
        if (std::regex_match(line, m, labelRe)) { label = m[1].str(); code = m[2].str(); }
        std::string prefix = label.empty() ? "" : label + ": ";
        std::string lc = lower(code);

        // (an even number of quotes: the final THEN is not inside a string)
        if (std::regex_match(code, m, ifThenRe) && std::count(code.begin(), code.end(), '"') % 2 == 0) {
            int id = nextId++;
            stack.push_back({'i', id, false, lineNum});
            emit(prefix + "if (" + m[1].str() + ")=0 then __else" + std::to_string(id), lineNum);
        } else if (lc == "else" && !stack.empty() && stack.back().kind == 'i' && label.empty()) {
            Block& b = stack.back();
            if (b.hasElse) {
                r.errors.push_back({lineNum, "second else in one if block"});
                continue;
            }
            b.hasElse = true;
            emit("goto __endif" + std::to_string(b.id), lineNum);
            emit("__else" + std::to_string(b.id) + ":", lineNum);
        } else if (lc == "endif" || lc == "end if") {
            if (stack.empty() || stack.back().kind != 'i') {
                r.errors.push_back({lineNum, "endif without a block if"});
                continue;
            }
            Block b = stack.back();
            stack.pop_back();
            if (!label.empty()) emit(label + ":", lineNum);
            emit((b.hasElse ? "__endif" : "__else") + std::to_string(b.id) + ":", lineNum);
        } else if (std::regex_match(code, m, procRe)) {
            int id = nextId++;
            stack.push_back({'p', id, false, lineNum});
            if (!label.empty()) emit(label + ":", lineNum);
            emit("goto __endproc" + std::to_string(id), lineNum);
            emit(m[1].str() + ":", lineNum);
        } else if (lc == "endproc") {
            if (stack.empty() || stack.back().kind != 'p') {
                r.errors.push_back({lineNum, "endproc without proc (or an if block is still open)"});
                continue;
            }
            Block b = stack.back();
            stack.pop_back();
            emit(prefix + "return", lineNum);
            emit("__endproc" + std::to_string(b.id) + ":", lineNum);
        } else {
            emit(label.empty() ? lowerCalls(raw) : prefix + lowerCalls(code), lineNum);
        }
    }
    for (const auto& b : stack)
        r.errors.push_back({b.line, b.kind == 'i' ? "if block without endif" : "proc without endproc"});
    r.text = out.str();
    return r;
}
