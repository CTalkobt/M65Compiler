#ifndef BASICVALIDATOR_HPP
#define BASICVALIDATOR_HPP

#include <string>
#include <vector>
#include <unordered_set>
#include "LabelBasedSourceParser.hpp"

struct ValidationError {
    std::string filename;
    int lineNumber;
    int column;
    std::string message;
    bool isWarning;
};

class BasicValidator {
public:
    BasicValidator(const std::string& filename) : sourceFile(filename) {}

    // Parse and validate label-based source code
    void analyze(const std::string& sourceCode);

    // Get all collected errors/warnings
    const std::vector<ValidationError>& getErrors() const { return errors; }
    bool hasErrors() const { return !errors.empty(); }
    int getErrorCount() const;
    int getWarningCount() const;

    // Query methods
    bool labelExists(const std::string& label) const;
    const std::unordered_set<std::string>& getDefinedLabels() const;
    const std::unordered_set<std::string>& getReferencedLabels() const;
    const LabelBasedSourceParser& getParser() const { return parser; }

    // Print errors in GNU format
    void printErrors() const;

private:
    std::string sourceFile;
    std::vector<ValidationError> errors;
    LabelBasedSourceParser parser;

    // Validation methods
    void validateLabelReferences();
    void detectUnresolvedSymbols(const std::string& sourceCode);
    void validateProgramSize();

    // Helper methods
    void addError(int lineNum, int col, const std::string& msg, bool isWarning = false);
};

#endif  // BASICVALIDATOR_HPP
