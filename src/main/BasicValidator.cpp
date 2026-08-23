#include "BasicValidator.hpp"
#include <sstream>
#include <regex>
#include <algorithm>
#include <iomanip>
#include <cstdint>
#include <cctype>

void BasicValidator::analyze(const std::string& sourceCode) {
    this->sourceCode = sourceCode;

    // Split into lines
    std::istringstream iss(sourceCode);
    std::string line;
    while (std::getline(iss, line)) {
        sourceLines.push_back(line);
    }

    // Collect labels and references
    collectDefinedLabels();
    collectReferencedLabels();
}

void BasicValidator::validate() {
    validateLabelReferences();
    validateLineNumberOrder();
    detectUnresolvedSymbols();
    validateProgramSize(0x0801);
}

void BasicValidator::collectDefinedLabels() {
    // Pattern: line_number at start of line (line numbers ARE the labels in BASIC)
    std::regex lineNumPattern(R"(^(\d+)\s+)");

    for (size_t i = 0; i < sourceLines.size(); i++) {
        const auto& line = sourceLines[i];
        if (line.empty() || std::isspace(line[0])) continue;

        std::smatch match;
        if (std::regex_search(line, match, lineNumPattern)) {
            std::string lineNum = match[1].str();
            definedLabels.insert(lineNum);  // Line numbers are valid jump targets
        }
    }
}

void BasicValidator::collectReferencedLabels() {
    // Patterns for label references:
    // GOTO 100, GOSUB 500, IF ... THEN 200
    std::regex gotoPattern(R"(\b(?:GOTO|GOSUB)\s+(\d+))");
    std::regex thenPattern(R"(\bTHEN\s+(\d+))");

    for (size_t i = 0; i < sourceLines.size(); i++) {
        const auto& line = sourceLines[i];
        if (line.empty()) continue;

        // GOTO/GOSUB references
        std::string::const_iterator searchStart(line.cbegin());
        std::smatch match;
        while (std::regex_search(searchStart, line.cend(), match, gotoPattern)) {
            std::string labelRef = match[1].str();
            referencedLabels.insert(labelRef);
            searchStart = match.suffix().first;
        }

        // THEN references
        searchStart = line.cbegin();
        while (std::regex_search(searchStart, line.cend(), match, thenPattern)) {
            std::string labelRef = match[1].str();
            referencedLabels.insert(labelRef);
            searchStart = match.suffix().first;
        }
    }
}

void BasicValidator::validateLabelReferences() {
    // Find undefined label references
    std::regex gotoPattern(R"(\b(?:GOTO|GOSUB)\s+(\d+))");
    std::regex thenPattern(R"(\bTHEN\s+(\d+))");

    for (size_t i = 0; i < sourceLines.size(); i++) {
        const auto& line = sourceLines[i];
        if (line.empty()) continue;

        // Check GOTO/GOSUB references
        std::string::const_iterator searchStart(line.cbegin());
        std::smatch match;
        while (std::regex_search(searchStart, line.cend(), match, gotoPattern)) {
            std::string labelRef = match[1].str();

            if (definedLabels.find(labelRef) == definedLabels.end()) {
                int col = std::distance(line.cbegin(), match[0].first) + 1;
                addError(i + 1, col, "Undefined label: " + labelRef);
            }

            searchStart = match.suffix().first;
        }

        // Check THEN references
        searchStart = line.cbegin();
        while (std::regex_search(searchStart, line.cend(), match, thenPattern)) {
            std::string labelRef = match[1].str();

            if (definedLabels.find(labelRef) == definedLabels.end()) {
                int col = std::distance(line.cbegin(), match[0].first) + 1;
                addError(i + 1, col, "Undefined label: " + labelRef);
            }

            searchStart = match.suffix().first;
        }
    }
}

void BasicValidator::validateLineNumberOrder() {
    // Extract explicit line numbers and check ascending order
    std::regex lineNumPattern(R"(^(\d+)\s+)");

    uint16_t prevLineNum = 0;
    bool hasExplicitLineNumbers = false;
    int lineCount = 0;

    for (size_t i = 0; i < sourceLines.size(); i++) {
        const auto& line = sourceLines[i];
        if (line.empty() || std::isspace(line[0])) continue;

        std::smatch match;
        if (std::regex_search(line, match, lineNumPattern)) {
            hasExplicitLineNumbers = true;
            uint16_t lineNum = static_cast<uint16_t>(std::stoi(match[1].str()));
            lineNumbers.push_back(lineNum);
            lineCount++;

            // Check for duplicates
            if (lineNum == prevLineNum && prevLineNum != 0) {
                addError(i + 1, 1, "Duplicate line number: " + std::to_string(lineNum));
            }

            // Check ascending order
            if (lineNum < prevLineNum) {
                addError(i + 1, 1, "Line numbers not in ascending order (prev: " +
                        std::to_string(prevLineNum) + ", current: " + std::to_string(lineNum) + ")");
            }

            prevLineNum = lineNum;
        }
    }

    if (!hasExplicitLineNumbers) {
        addError(1, 1, "No explicit line numbers found. Use numbered lines (e.g., 10 PRINT \"HELLO\")", true);
    }
}

void BasicValidator::detectUnresolvedSymbols() {
    // Pattern: {symbol_name} that doesn't look like a valid substitution target
    std::regex symbolPattern(R"(\{([^}]+)\})");

    for (size_t i = 0; i < sourceLines.size(); i++) {
        const auto& line = sourceLines[i];
        if (line.empty()) continue;

        std::string::const_iterator searchStart(line.cbegin());
        std::smatch match;

        while (std::regex_search(searchStart, line.cend(), match, symbolPattern)) {
            std::string symbolName = match[1].str();

            // Simple heuristic: if it's still in {} after processing, it's likely unresolved
            // This is a warning because symbol resolution depends on external symbol files
            int col = std::distance(line.cbegin(), match[0].first) + 1;
            addError(i + 1, col, "Unresolved symbol: " + symbolName, true);
            unresolvedSymbols.insert(symbolName);

            searchStart = match.suffix().first;
        }
    }
}

void BasicValidator::validateProgramSize(uint16_t loadAddress) {
    // Rough estimate of program size (conservative estimate)
    // Each line: 4 bytes header + ~1.5x source length (tokens are usually shorter but have overhead)
    // This is a warning, not an error

    size_t estimatedSize = 0;
    estimatedSize += 2;  // Load address itself

    for (const auto& line : sourceLines) {
        if (!line.empty() && !std::isspace(line[0])) {
            estimatedSize += 4;  // Line header (address, line number)
            estimatedSize += line.length() / 2 + 10;  // Conservative token estimate
        }
    }

    // MEGA65 BASIC typically has ~32KB available (0x0801 to 0x8000)
    // Warn if program exceeds 28KB (leaving 4KB buffer)
    const uint16_t warnThreshold = 0x7000 - loadAddress;
    if (estimatedSize > warnThreshold) {
        addError(1, 1, "Program size warning: estimated " + std::to_string(estimatedSize) +
                " bytes (max " + std::to_string(warnThreshold) + ")", true);
    }
}

bool BasicValidator::labelExists(const std::string& label) const {
    return definedLabels.find(label) != definedLabels.end();
}

std::string BasicValidator::trim(const std::string& str) const {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

bool BasicValidator::isValidLabelName(const std::string& name) const {
    if (name.empty()) return false;
    if (!std::isalpha(name[0]) && name[0] != '_') return false;
    for (char c : name) {
        if (!std::isalnum(c) && c != '_') return false;
    }
    return true;
}

int BasicValidator::findLineNumber(const std::string& line) const {
    std::regex lineNumPattern(R"(^(\d+))");
    std::smatch match;
    if (std::regex_search(line, match, lineNumPattern)) {
        return std::stoi(match[1].str());
    }
    return -1;
}

void BasicValidator::addError(int lineNum, int col, const std::string& msg, bool isWarning) {
    ValidationError err;
    err.filename = sourceFile;
    err.lineNumber = lineNum;
    err.column = col;
    err.message = msg;
    err.isWarning = isWarning;
    errors.push_back(err);
}

void BasicValidator::printErrors() const {
    for (const auto& err : errors) {
        std::string type = err.isWarning ? "warning" : "error";
        std::cerr << err.filename << ":" << err.lineNumber << ":" << err.column
                  << ": " << type << ": " << err.message << std::endl;
    }
}
