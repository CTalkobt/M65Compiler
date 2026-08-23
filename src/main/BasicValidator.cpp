#include "BasicValidator.hpp"
#include <iostream>
#include <sstream>
#include <regex>

void BasicValidator::analyze(const std::string& sourceCode) {
    // Parse label-based source code
    parser.parse(sourceCode);

    // Assign auto-incrementing line numbers (10, 20, 30, ...)
    parser.assignLineNumbers(10, 10);

    // Validate label references
    std::vector<std::string> parseErrors;
    if (!parser.validate(parseErrors)) {
        for (size_t i = 0; i < parseErrors.size(); i++) {
            addError(1, 1, parseErrors[i]);
        }
    }

    // Detect unresolved symbols
    detectUnresolvedSymbols(sourceCode);

    // Estimate program size
    validateProgramSize();
}

void BasicValidator::validateLabelReferences() {
    // Validation is done in parser.validate()
}

void BasicValidator::detectUnresolvedSymbols(const std::string& sourceCode) {
    // Pattern: {symbol_name}
    std::regex symbolPattern(R"(\{([^}]+)\})");

    std::istringstream iss(sourceCode);
    std::string line;
    int lineNum = 0;

    while (std::getline(iss, line)) {
        lineNum++;

        std::string::const_iterator searchStart(line.cbegin());
        std::smatch match;

        while (std::regex_search(searchStart, line.cend(), match, symbolPattern)) {
            int col = std::distance(line.cbegin(), match[0].first) + 1;
            addError(lineNum, col, "Unresolved symbol: " + match[1].str(), true);
            searchStart = match.suffix().first;
        }
    }
}

void BasicValidator::validateProgramSize() {
    // Rough estimate of program size
    size_t estimatedSize = 2;  // Load address

    for (const auto& sl : parser.getSourceLines()) {
        estimatedSize += 4;  // Line header
        estimatedSize += sl.code.length() / 2 + 10;  // Conservative token estimate
    }

    // Warn if program likely exceeds 28KB
    if (estimatedSize > 28000) {
        addError(1, 1, "Program size warning: estimated " + std::to_string(estimatedSize) +
                " bytes (max ~28KB)", true);
    }
}

bool BasicValidator::labelExists(const std::string& label) const {
    const auto& labels = parser.getDefinedLabels();
    return labels.find(label) != labels.end();
}

const std::unordered_set<std::string>& BasicValidator::getDefinedLabels() const {
    return parser.getDefinedLabels();
}

const std::unordered_set<std::string>& BasicValidator::getReferencedLabels() const {
    return parser.getReferencedLabels();
}

int BasicValidator::getErrorCount() const {
    int count = 0;
    for (const auto& err : errors) {
        if (!err.isWarning) count++;
    }
    return count;
}

int BasicValidator::getWarningCount() const {
    int count = 0;
    for (const auto& err : errors) {
        if (err.isWarning) count++;
    }
    return count;
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
