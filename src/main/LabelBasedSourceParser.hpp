#ifndef LABELBASEDSOURCEPARSER_HPP
#define LABELBASEDSOURCEPARSER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <cstdint>

struct BasicToken;

struct SourceLine {
    std::string label;          // "" if no label
    std::string code;           // BASIC code for this line ("" for a label alone)
    std::string pragma;         // a #pragma line ("encoding petscii"), no code
    int sourceLineNum;          // Original line number in source
    uint16_t basicLineNum = 0;  // its BASIC line number (0: no line of its own)
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

    // Label (upper case) -> BASIC line number, after assignLineNumbers
    const std::unordered_map<std::string, uint16_t>& getLabelMap() const { return labelToLineNum; }

    // Number the lines that have code: startLineNum, then + lineIncrement.
    // A label alone on its line names the next line with code; labels at the
    // end of the source name an extra line (trailingLineNum, 0 if none).
    void assignLineNumbers(uint16_t startLineNum, uint16_t lineIncrement);
    uint16_t getTrailingLineNum() const { return trailingLineNum; }

    // Validate all label references (and the line numbers): errors as
    // (source line, message), source line 0 when there is none
    bool validate(std::vector<std::pair<int, std::string>>& errors);

    // Get all referenced labels (upper case)
    const std::unordered_set<std::string>& getReferencedLabels() const { return referencedLabels; }

    // Get all defined labels (as written)
    const std::unordered_set<std::string>& getDefinedLabels() const { return definedLabels; }

    // The label references in one line's tokens: the word after GOTO, GOSUB,
    // GO TO, THEN, ELSE, RESTORE, TRAP, RESUME and RUN (and each further
    // item of an ON ... GOTO/GOSUB list). isLabel(name) says whether a word
    // (upper case) is a defined label; f(tokenIndex, upperName) is called for
    // each reference. A word after GOTO/GOSUB is always a reference (also a
    // keyword that is a label name: GOTO loop); after the others only a name
    // that is a defined label or ends the statement (not a keyword).
    static void forEachLabelRef(const std::vector<BasicToken>& tokens,
                                const std::function<bool(const std::string&)>& isLabel,
                                const std::function<void(size_t, const std::string&)>& f);

    static std::string upper(const std::string& s);

private:
    std::vector<SourceLine> sourceLines;
    std::unordered_map<std::string, uint16_t> labelToLineNum;
    std::unordered_set<std::string> definedLabels;
    std::unordered_set<std::string> definedUpper;
    std::unordered_set<std::string> referencedLabels;
    std::unordered_map<std::string, int> firstRefLine;   // label -> first source line using it
    std::unordered_map<std::string, int> labelLine;      // label -> its first definition's source line
    uint16_t trailingLineNum = 0;
    bool lineNumberOverflow = false;

    // Helper methods
    void extractLabelsAndReferences();
    std::string trim(const std::string& str) const;
    bool isValidLabelName(const std::string& name) const;
    bool isLabelDefinition(const std::string& line, std::string& labelName, std::string& code) const;
};

#endif  // LABELBASEDSOURCEPARSER_HPP
