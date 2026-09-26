#include "LabelBasedSourceParser.hpp"
#include <sstream>
#include <regex>
#include <algorithm>
#include <cctype>

void LabelBasedSourceParser::parse(const std::string& sourceCode) {
    sourceLines.clear();
    labelToLineNum.clear();
    definedLabels.clear();
    referencedLabels.clear();

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

        std::string labelName, code;
        if (isLabelDefinition(trimmed, labelName, code)) {
            SourceLine sl;
            sl.label = labelName;
            sl.code = code;
            sl.sourceLineNum = lineNum;
            sourceLines.push_back(sl);
            definedLabels.insert(labelName);
        } else {
            // Line with no label
            SourceLine sl;
            sl.label = "";
            sl.code = trimmed;
            sl.sourceLineNum = lineNum;
            sourceLines.push_back(sl);
        }
    }

    // Extract all references
    extractLabelsAndReferences();
}

void LabelBasedSourceParser::extractLabelsAndReferences() {
    for (const auto& sl : sourceLines) {
        extractReferencesFromLine(sl.code);
    }
}

void LabelBasedSourceParser::assignLineNumbers(uint16_t startLineNum, uint16_t lineIncrement) {
    labelToLineNum.clear();
    uint16_t currentLineNum = startLineNum;

    for (const auto& sl : sourceLines) {
        if (!sl.label.empty()) {
            labelToLineNum[sl.label] = currentLineNum;
        }
        currentLineNum += lineIncrement;
    }
}

bool LabelBasedSourceParser::validate(std::vector<std::string>& errors) {
    bool isValid = true;

    // Check for undefined label references
    for (const auto& ref : referencedLabels) {
        if (definedLabels.find(ref) == definedLabels.end()) {
            errors.push_back("Undefined label: " + ref);
            isValid = false;
        }
    }

    // Check for duplicate labels
    std::unordered_map<std::string, int> labelCount;
    for (const auto& sl : sourceLines) {
        if (!sl.label.empty()) {
            labelCount[sl.label]++;
        }
    }
    for (const auto& [label, count] : labelCount) {
        if (count > 1) {
            errors.push_back("Duplicate label definition: " + label + " (" + std::to_string(count) + " times)");
            isValid = false;
        }
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

void LabelBasedSourceParser::extractReferencesFromLine(const std::string& code) {
    // Find all GOTO/GOSUB/THEN references to labels
    // Need separate patterns because THEN can be followed by GOTO/GOSUB which then has the label
    std::regex gotoPattern(R"(\b(?:GOTO|GOSUB)\s+([a-zA-Z_]\w*)\b)");
    std::regex thenGotoPattern(R"(\bTHEN\s+(?:GOTO|GOSUB)\s+([a-zA-Z_]\w*)\b)");
    std::regex thenLabelPattern(R"(\bTHEN\s+([a-zA-Z_]\w*)\b)");

    std::string::const_iterator searchStart(code.cbegin());
    std::smatch match;

    // First, try THEN GOTO/GOSUB pattern
    searchStart = code.cbegin();
    while (std::regex_search(searchStart, code.cend(), match, thenGotoPattern)) {
        std::string labelRef = match[1].str();
        referencedLabels.insert(labelRef);
        searchStart = match.suffix().first;
    }

    // Then, try plain GOTO/GOSUB pattern
    searchStart = code.cbegin();
    while (std::regex_search(searchStart, code.cend(), match, gotoPattern)) {
        std::string labelRef = match[1].str();
        // Skip if this was already matched as part of THEN GOTO
        if (labelRef != "GOTO" && labelRef != "GOSUB" && labelRef != "THEN") {
            referencedLabels.insert(labelRef);
        }
        searchStart = match.suffix().first;
    }

    // Finally, try plain THEN label pattern (handles THEN without explicit GOTO)
    searchStart = code.cbegin();
    while (std::regex_search(searchStart, code.cend(), match, thenLabelPattern)) {
        std::string labelRef = match[1].str();
        // Skip if it's a BASIC keyword (GOTO, GOSUB, etc.)
        if (labelRef != "GOTO" && labelRef != "GOSUB" && labelRef != "IF" &&
            labelRef != "THEN" && labelRef != "ELSE" && labelRef != "END") {
            referencedLabels.insert(labelRef);
        }
        searchStart = match.suffix().first;
    }
}
