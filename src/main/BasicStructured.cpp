#include "BasicStructured.hpp"
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

}  // namespace

BasicStructuredResult lowerBasicStructured(const std::string& source,
                                           const std::function<std::string(int)>& fileOf) {
    BasicStructuredResult r;
    std::ostringstream out;
    auto emit = [&](const std::string& line, int from) {
        out << line << "\n";
        r.origin.push_back(from);
    };
    auto error = [&](int line, const std::string& msg) { r.errors.push_back({line, msg}); };

    static const std::regex labelRe(R"(^([A-Za-z_]\w*)\s*:\s*(.*)$)");
    static const std::regex ifThenRe(R"(^if\s+(.*\S)\s+then$)", std::regex::icase);
    static const std::regex procRe(R"(^proc\s+([A-Za-z_]\w*)\s*(\((.*)\))?$)", std::regex::icase);
    static const std::regex selectRe(R"(^select\s+(.+)$)", std::regex::icase);
    static const std::regex caseRe(R"(^case\s+(.+)$)", std::regex::icase);
    static const std::regex rangeRe(R"(^(.*\S)\s+to\s+(.+)$)", std::regex::icase);
    static const std::regex isRe(R"(^is\s*(<>|<=|>=|=|<|>)\s*(.+)$)", std::regex::icase);
    static const std::regex incbinRe(R"re(^incbin\s+"([^"]+)"\s+as\s+([A-Za-z_]\w*)$)re", std::regex::icase);
    static const std::regex dataincRe(R"re(^datainc\s+"([^"]+)"(\s*,\s*(\d+))?(\s+as\s+([A-Za-z_]\w*))?$)re",
                                      std::regex::icase);
    static const std::regex asmLabelRe(R"(^([A-Za-z_]\w*):)");

    // First pass: the parameters of each proc
    std::map<std::string, std::vector<std::string>> procParams;   // lower-case name -> params
    {
        std::istringstream in(source);
        std::string raw;
        static const std::regex procDef(R"(^\s*(?:[A-Za-z_]\w*\s*:\s*)?proc\s+([A-Za-z_]\w*)\s*(\((.*)\))?\s*$)",
                                        std::regex::icase);
        std::smatch m;
        while (std::getline(in, raw))
            if (std::regex_match(raw, m, procDef)) {
                std::vector<std::string> ps;
                if (m[2].matched && !trim(m[3].str()).empty()) ps = splitTop(m[3].str());
                procParams[lower(m[1].str())] = ps;
            }
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
                    auto pp = procParams.find(lower(name));
                    std::string assigns;
                    if (pp == procParams.end()) {
                        if (!args.empty()) error(lineNum, "call " + name + "(...): no proc " + name + " with parameters");
                    } else if (pp->second.size() != args.size()) {
                        error(lineNum, "call " + name + ": " + std::to_string(args.size()) + " argument(s), proc " +
                                       name + " takes " + std::to_string(pp->second.size()));
                    } else {
                        for (size_t k = 0; k < args.size(); k++)
                            assigns += pp->second[k] + " = " + args[k] + " : ";
                    }
                    res += assigns + "gosub " + name;
                    i = e - 1;
                    continue;
                }
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

    struct Block { char kind; int id; bool hasElse; int line; int cases; std::string var; };
    std::vector<Block> stack;
    int nextId = 1;
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
            stack.push_back({'i', id, false, lineNum, 0, ""});
            emit(prefix + "if (" + m[1].str() + ")=0 then __else" + std::to_string(id), lineNum);
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
            int id = nextId++;
            stack.push_back({'p', id, false, lineNum, 0, ""});
            labelAlone();
            emit("goto __endproc" + std::to_string(id), lineNum);
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
            emit(prefix + "return", lineNum);
            emit("__endproc" + std::to_string(b.id) + ":", lineNum);
            scope = "main";
        } else if (std::regex_match(code, m, selectRe) && lower(code.substr(0, 7)) == "select ") {
            int id = nextId++;
            std::string expr = m[1].str();
            std::string var = "zsel" + std::to_string(id) + (isStringExpr(expr) ? "$" : "");
            stack.push_back({'s', id, false, lineNum, 0, var});
            emit(prefix + var + " = " + expr, lineNum);
        } else if (std::regex_match(code, m, caseRe) && lower(code.substr(0, 5)) == "case ") {
            if (stack.empty() || stack.back().kind != 's') { error(lineNum, "case outside select"); continue; }
            Block& b = stack.back();
            if (b.hasElse) { error(lineNum, "case after case else"); continue; }
            std::string sel = std::to_string(b.id);
            if (b.cases > 0) {
                emit("goto __endsel" + sel, lineNum);
                emit("__case" + sel + "_" + std::to_string(b.cases + 1) + ":", lineNum);
            }
            labelAlone();
            b.cases++;
            std::string what = trim(m[1].str());
            if (lower(what) == "else") {
                b.hasElse = true;
                continue;
            }
            std::string cond;
            for (const auto& item : splitTop(what)) {
                std::smatch im;
                std::string c;
                if (std::regex_match(item, im, rangeRe))
                    c = "(" + b.var + ">=" + im[1].str() + " and " + b.var + "<=" + im[2].str() + ")";
                else if (std::regex_match(item, im, isRe))
                    c = "(" + b.var + im[1].str() + im[2].str() + ")";
                else
                    c = "(" + b.var + "=" + item + ")";
                cond += (cond.empty() ? "" : " or ") + c;
            }
            emit("if (" + cond + ")=0 then __case" + sel + "_" + std::to_string(b.cases + 1), lineNum);
        } else if (lc == "endselect" || lc == "end select") {
            if (stack.empty() || stack.back().kind != 's') { error(lineNum, "endselect without select"); continue; }
            Block b = stack.back();
            stack.pop_back();
            labelAlone();
            std::string sel = std::to_string(b.id);
            if (b.cases > 0 && !b.hasElse) emit("__case" + sel + "_" + std::to_string(b.cases + 1) + ":", lineNum);
            emit("__endsel" + sel + ":", lineNum);
        } else {
            emit(prefix + lowerCalls(code, lineNum), lineNum);
        }
    }
    if (inAsm) error(asmLine, "asm without endasm");
    for (const auto& b : stack)
        error(b.line, b.kind == 'i' ? "if block without endif" :
                      b.kind == 's' ? "select without endselect" : "proc without endproc");
    r.text = out.str();
    return r;
}
