#ifndef BASICVALIDATOR_HPP
#define BASICVALIDATOR_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <cstdint>

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

    // Parse source code and collect validation data
    void analyze(const std::string& sourceCode);

    // Validate all aspects
    void validate();

    // Get all collected errors/warnings
    const std::vector<ValidationError>& getErrors() const { return errors; }
    bool hasErrors() const { return !errors.empty(); }

    // Query methods
    bool labelExists(const std::string& label) const;
    const std::unordered_set<std::string>& getDefinedLabels() const { return definedLabels; }
    const std::unordered_set<std::string>& getReferencedLabels() const { return referencedLabels; }
    const std::unordered_set<std::string>& getUnresolvedSymbols() const { return unresolvedSymbols; }

    // Print errors in GNU format
    void printErrors() const;

private:
    std::string sourceFile;
    std::string sourceCode;
    std::vector<std::string> sourceLines;
    std::vector<ValidationError> errors;

    // Validation data
    std::unordered_set<std::string> definedLabels;
    std::unordered_set<std::string> referencedLabels;
    std::unordered_set<std::string> unresolvedSymbols;
    std::vector<uint16_t> lineNumbers;  // Line numbers from source

    // Validation methods
    void collectDefinedLabels();
    void collectReferencedLabels();
    void validateLabelReferences();
    void validateLineNumberOrder();
    void detectUnresolvedSymbols();
    void validateProgramSize(uint16_t loadAddress);

    // Helper methods
    std::string trim(const std::string& str) const;
    bool isValidLabelName(const std::string& name) const;
    int findLineNumber(const std::string& line) const;
    void addError(int lineNum, int col, const std::string& msg, bool isWarning = false);
};

#endif  // BASICVALIDATOR_HPP
