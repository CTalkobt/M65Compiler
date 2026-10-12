#include "LabelBasedSourceParser.hpp"
#include "BasicTokenizer.hpp"
#include <sstream>
#include <regex>
#include <algorithm>
#include <cctype>

std::string LabelBasedSourceParser::upper(const std::string& s) {
    std::string u = s;
    std::transform(u.begin(), u.end(), u.begin(), ::toupper);
    return u;
}

void LabelBasedSourceParser::parse(const std::string& sourceCode) {
    sourceLines.clear();
    labelToLineNum.clear();
    definedLabels.clear();
    definedUpper.clear();
    referencedLabels.clear();
    firstRefLine.clear();
    labelLine.clear();
    trailingLineNum = 0;
    lineNumberOverflow = false;

    std::istringstream iss(sourceCode);
    std::string line;
    int lineNum = 0;

    while (std::getline(iss, line)) {
        lineNum++;

        // Skip empty lines and comments
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '\'' || trimmed.substr(0, 3) == "REM") {
            continue;
        }

        SourceLine sl;
        sl.sourceLineNum = lineNum;
        std::string labelName, code;
        if (isLabelDefinition(trimmed, labelName, code)) {
            sl.label = labelName;
            sl.code = code;
            definedLabels.insert(labelName);
            definedUpper.insert(upper(labelName));
            labelLine.insert({upper(labelName), lineNum});
        } else {
            sl.code = trimmed;
        }
        sourceLines.push_back(sl);
    }

    // Extract all references
    extractLabelsAndReferences();
}

void LabelBasedSourceParser::forEachLabelRef(const std::vector<BasicToken>& toks,
        const std::function<bool(const std::string&)>& isLabel,
        const std::function<void(size_t, const std::string&)>& f) {
    auto isWord = [&](size_t j) {
        return j < toks.size() &&
               (toks[j].type == BasicToken::IDENTIFIER || toks[j].type == BasicToken::KEYWORD);
    };
    auto endsStatement = [&](size_t j) {
        return j >= toks.size() || toks[j].type == BasicToken::EOL ||
               toks[j].type == BasicToken::END_OF_FILE ||
               (toks[j].type == BasicToken::OPERATOR && toks[j].value == ":") ||
               (toks[j].type == BasicToken::KEYWORD && toks[j].value == "ELSE");
    };
    for (size_t i = 0; i < toks.size(); i++) {
        if (toks[i].type != BasicToken::KEYWORD) continue;
        const std::string& kw = toks[i].value;
        size_t j = i + 1;
        bool list = false;   // GOTO/GOSUB: a reference, and maybe a list of them
        if (kw == "GOTO" || kw == "GOSUB") {
            list = true;
        } else if (kw == "GO" && j < toks.size() && toks[j].type == BasicToken::KEYWORD &&
                   toks[j].value == "TO") {
            list = true;
            j++;
        } else if (kw != "THEN" && kw != "ELSE" && kw != "RESTORE" && kw != "TRAP" &&
                   kw != "RESUME" && kw != "RUN") {
            continue;
        }
        while (isWord(j)) {
            std::string name = upper(toks[j].value);
            // a keyword is a label only after GOTO/GOSUB (IF X THEN END
            // stays END even with a label "end")
            bool ref = list ? (isLabel(name) || toks[j].type == BasicToken::IDENTIFIER)
                            : (toks[j].type == BasicToken::IDENTIFIER &&
                               (isLabel(name) || endsStatement(j + 1)));
            if (!ref) break;
            f(j, name);
            if (!list || j + 2 >= toks.size() || toks[j + 1].type != BasicToken::OPERATOR ||
                toks[j + 1].value != ",")
                break;
            j += 2;   // ON X GOTO a, b, c
        }
    }
}

void LabelBasedSourceParser::extractLabelsAndReferences() {
    BasicTokenizer tokenizer;
    auto isLabel = [this](const std::string& n) { return definedUpper.count(n) > 0; };
    for (const auto& sl : sourceLines) {
        if (sl.code.empty()) continue;
        auto toks = tokenizer.tokenize(sl.code);
        int at = sl.sourceLineNum;
        forEachLabelRef(toks, isLabel, [this, at](size_t, const std::string& n) {
            referencedLabels.insert(n);
            firstRefLine.insert({n, at});
        });
    }
}

void LabelBasedSourceParser::assignLineNumbers(uint16_t startLineNum, uint16_t lineIncrement) {
    labelToLineNum.clear();
    trailingLineNum = 0;
    lineNumberOverflow = false;
    uint32_t current = startLineNum;
    std::vector<std::string> pending;   // labels waiting for a line with code

    for (auto& sl : sourceLines) {
        if (!sl.label.empty()) pending.push_back(sl.label);
        if (sl.code.empty()) { sl.basicLineNum = 0; continue; }
        if (current > 63999) lineNumberOverflow = true;
        sl.basicLineNum = (uint16_t)std::min<uint32_t>(current, 63999);
        for (const auto& l : pending) labelToLineNum[upper(l)] = sl.basicLineNum;
        pending.clear();
        current += lineIncrement;
    }
    if (!pending.empty()) {
        if (current > 63999) lineNumberOverflow = true;
        trailingLineNum = (uint16_t)std::min<uint32_t>(current, 63999);
        for (const auto& l : pending) labelToLineNum[upper(l)] = trailingLineNum;
    }
}

bool LabelBasedSourceParser::validate(std::vector<std::pair<int, std::string>>& errors) {
    bool isValid = true;

    // Check for undefined label references
    std::vector<std::pair<int, std::string>> undefinedRefs;
    for (const auto& ref : referencedLabels)
        if (definedUpper.find(ref) == definedUpper.end()) undefinedRefs.push_back({firstRefLine[ref], ref});
    std::sort(undefinedRefs.begin(), undefinedRefs.end());
    for (const auto& [at, ref] : undefinedRefs) {
        errors.push_back({at, "Undefined label: " + ref});
        isValid = false;
    }

    // Check for duplicate labels (labels are matched without regard to case)
    std::unordered_map<std::string, int> labelCount;
    std::vector<std::string> order;
    for (const auto& sl : sourceLines) {
        if (!sl.label.empty() && labelCount[upper(sl.label)]++ == 0) order.push_back(sl.label);
    }
    for (const auto& label : order) {
        int count = labelCount[upper(label)];
        if (count > 1) {
            errors.push_back({labelLine[upper(label)], "Duplicate label definition: " + label + " (" + std::to_string(count) + " times)"});
            isValid = false;
        }
    }

    if (lineNumberOverflow) {
        errors.push_back({0, "Too many lines: BASIC line numbers end at 63999 (use a smaller --increment)"});
        isValid = false;
    }

    return isValid;
}

std::string LabelBasedSourceParser::trim(const std::string& str) const {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

bool LabelBasedSourceParser::isValidLabelName(const std::string& name) const {
    if (name.empty()) return false;
    if (!std::isalpha(name[0]) && name[0] != '_') return false;
    for (char c : name) {
        if (!std::isalnum(c) && c != '_') return false;
    }
    return true;
}

bool LabelBasedSourceParser::isLabelDefinition(const std::string& line,
                                               std::string& labelName,
                                               std::string& code) const {
    // Pattern: label_name: BASIC code
    std::regex labelPattern(R"(^(\w+)\s*:\s*(.*))");
    std::smatch match;

    if (std::regex_match(line, match, labelPattern)) {
        labelName = match[1].str();
        code = match[2].str();
        return isValidLabelName(labelName);
    }

    return false;
}
