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
    numbered = false;
    numberErrors.clear();

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
        if (trimmed.compare(0, 7, "#pragma") == 0) {
            sl.pragma = trim(trimmed.substr(7));
            sourceLines.push_back(sl);
            continue;
        }
        // a line number first ("10 PRINT", "20GOTO 10"): a numbered program
        // (no BASIC statement starts with a digit)
        if (std::isdigit((unsigned char)trimmed[0])) {
            size_t d = 0;
            while (d < trimmed.size() && std::isdigit((unsigned char)trimmed[d])) d++;
            std::string digits = trimmed.substr(0, d);
            unsigned long n = digits.size() > 5 ? 99999 : std::stoul(digits);
            numbered = true;
            if (n > 63999) numberErrors.push_back({lineNum, "line number " + digits + " is above 63999"});
            sl.writtenNum = (uint16_t)std::min<unsigned long>(n, 63999);
            trimmed = trim(trimmed.substr(d));
            if (trimmed.empty()) {
                numberErrors.push_back({lineNum, "line " + digits + " has no code (BASIC deletes such a line)"});
                continue;
            }
        }
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

void LabelBasedSourceParser::forEachLineNumberRef(const std::vector<BasicToken>& toks,
        const std::function<void(size_t, uint16_t)>& f) {
    auto isNumber = [&](size_t j) {
        return j < toks.size() && toks[j].type == BasicToken::NUMBER &&
               toks[j].value.find_first_not_of("0123456789") == std::string::npos && toks[j].value.size() <= 5;
    };
    for (size_t i = 0; i < toks.size(); i++) {
        if (toks[i].type != BasicToken::KEYWORD) continue;
        const std::string& kw = toks[i].value;
        size_t j = i + 1;
        bool list = false;
        if (kw == "GOTO" || kw == "GOSUB") {
            list = true;
        } else if (kw == "GO" && j < toks.size() && toks[j].type == BasicToken::KEYWORD && toks[j].value == "TO") {
            list = true;
            j++;
        } else if (kw != "THEN" && kw != "ELSE" && kw != "RESTORE" && kw != "TRAP" && kw != "RESUME" && kw != "RUN") {
            continue;
        }
        while (isNumber(j)) {
            f(j, (uint16_t)std::min(std::stoul(toks[j].value), 65535ul));
            if (!list || j + 2 >= toks.size() || toks[j + 1].type != BasicToken::OPERATOR || toks[j + 1].value != ",")
                break;
            j += 2;   // ON X GOTO 100, 200
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

    if (numbered) {
        // the lines keep their numbers, which must ascend; every line with
        // code needs one
        uint32_t previous = 0;
        bool missing = false;
        for (auto& sl : sourceLines) {
            if (!sl.label.empty()) pending.push_back(sl.label);
            if (sl.code.empty()) { sl.basicLineNum = 0; continue; }
            if (!sl.writtenNum) {
                if (!missing)
                    numberErrors.push_back({sl.sourceLineNum, "this line has no line number, but the program's "
                                            "other lines are numbered (number every line, or none)"});
                missing = true;
                sl.basicLineNum = (uint16_t)std::min<uint32_t>(previous + 1, 63999);
            } else {
                if (sl.writtenNum <= previous)
                    numberErrors.push_back({sl.sourceLineNum, "line number " + std::to_string(sl.writtenNum) +
                                            " after " + std::to_string(previous) + ": the numbers must ascend"});
                sl.basicLineNum = sl.writtenNum;
            }
            previous = std::max<uint32_t>(previous, sl.basicLineNum);
            for (const auto& l : pending) labelToLineNum[upper(l)] = sl.basicLineNum;
            pending.clear();
        }
        if (!pending.empty()) {
            trailingLineNum = (uint16_t)std::min<uint32_t>(previous + 1, 63999);
            for (const auto& l : pending) labelToLineNum[upper(l)] = trailingLineNum;
        }
        return;
    }

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

    // a numbered program: its numbers, and the lines GOTO 100 and the like
    // name must exist (?UNDEF'D STATEMENT when run)
    for (const auto& e : numberErrors) { errors.push_back(e); isValid = false; }
    if (numbered) {
        std::unordered_set<uint16_t> lineNums;
        for (const auto& sl : sourceLines) if (sl.basicLineNum) lineNums.insert(sl.basicLineNum);
        if (trailingLineNum) lineNums.insert(trailingLineNum);
        BasicTokenizer tokenizer;
        for (const auto& sl : sourceLines) {
            if (sl.code.empty()) continue;
            auto toks = tokenizer.tokenize(sl.code);
            forEachLineNumberRef(toks, [&](size_t, uint16_t n) {
                if (!lineNums.count(n)) {
                    errors.push_back({sl.sourceLineNum, "line " + std::to_string(n) + " does not exist"});
                    isValid = false;
                }
            });
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
