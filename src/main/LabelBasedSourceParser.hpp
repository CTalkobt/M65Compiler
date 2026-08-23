#ifndef LABELBASEDSOURCEPARSER_HPP
#define LABELBASEDSOURCEPARSER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>

struct SourceLine {
    std::string label;          // "" if no label
    std::string code;           // BASIC code for this line
    int sourceLineNum;          // Original line number in source
};

struct LabelInfo {
    std::string name;
    uint16_t assignedLineNumber;
    int sourceLineNum;
};

class LabelBasedSourceParser {
public:
    // Parse label-based source code
    void parse(const std::string& sourceCode);

    // Get parsed lines
    const std::vector<SourceLine>& getSourceLines() const { return sourceLines; }

    // Get label mappings
    const std::unordered_map<std::string, uint16_t>& getLabelMap() const { return labelToLineNum; }

    // Assign line numbers to labels (auto-increment by increment value)
    void assignLineNumbers(uint16_t startLineNum, uint16_t lineIncrement);

    // Validate all label references
    bool validate(std::vector<std::string>& errors);

    // Get all referenced labels
    const std::unordered_set<std::string>& getReferencedLabels() const { return referencedLabels; }

    // Get all defined labels
    const std::unordered_set<std::string>& getDefinedLabels() const { return definedLabels; }

private:
    std::vector<SourceLine> sourceLines;
    std::unordered_map<std::string, uint16_t> labelToLineNum;
    std::unordered_set<std::string> definedLabels;
    std::unordered_set<std::string> referencedLabels;

    // Helper methods
    void extractLabelsAndReferences();
    std::string trim(const std::string& str) const;
    bool isValidLabelName(const std::string& name) const;
    bool isLabelDefinition(const std::string& line, std::string& labelName, std::string& code) const;
    void extractReferencesFromLine(const std::string& code);
};

#endif  // LABELBASEDSOURCEPARSER_HPP
