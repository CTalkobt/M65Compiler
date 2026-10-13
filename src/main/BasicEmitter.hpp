#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <set>
#include "BasicTokenizer.hpp"
#include "LabelBasedSourceParser.hpp"

struct BasicLine {
    uint16_t lineNumber;
    std::vector<BasicToken> tokens;
};

class BasicEmitter {
public:
    BasicEmitter(uint16_t loadAddr = 0x2001, uint16_t lineIncrement = 10);

    std::vector<uint8_t> emit(const std::vector<BasicLine>& lines);
    std::vector<uint8_t> emitBinary(const std::string& sourceCode, bool useLabels = false);
    std::vector<uint8_t> emitFromLabels(const LabelBasedSourceParser& parser);

    const std::map<std::string, uint16_t>& getLabelMap() const;
    void outputLabelTable(const std::string& filename) const;

    void setLineIncrement(uint16_t increment) { lineIncrement = increment; }
    // Encoding of strings without a prefix: 'u' (default: PETSCII, letters
    // folded to $41-$5A), 'p' (mixed case), 's' (screen codes), 'a' (ASCII);
    // #pragma encoding <upper|petscii|screencode|ascii> changes it in the source
    void setEncoding(char e) { defaultEncoding = e; }
    static char encodingFromName(const std::string& name);   // 0 if unknown

    // Messages found while emitting: the parser's source line (1-based)
    struct Diagnostic { int sourceLine; std::string message; bool error; };
    const std::vector<Diagnostic>& getDiagnostics() const { return diagnostics; }

    // For the memory report
    size_t getScalarCount() const { return scalarCount; }
    size_t getArrayCount() const { return arrayCount; }
    size_t getLongestLine() const { return longestLine; }           // listed characters
    uint16_t getLongestLineNumber() const { return longestLineNumber; }

    // Rename variables longer than two characters (BasicVarRenamer; default on)
    void setRenameVariables(bool on) { renameVariables = on; }
    const std::map<std::string, std::string>& getVariableMap() const { return variableMap; }

    // The emitted lines: BASIC line number and the parser's source line
    // index (1-based line of the text given to the parser; 0 for the
    // extra line of trailing labels), in program order
    struct EmittedLine { uint16_t lineNumber; int sourceLine; uint16_t address; };
    const std::vector<EmittedLine>& getEmittedLines() const { return emittedLines; }
    void setPreserveSpaces(bool preserve) { preserveSpaces = preserve; }

private:
    uint16_t loadAddress;
    uint16_t lineIncrement;
    bool preserveSpaces = false;
    std::map<std::string, uint16_t> labelMap;      // label (as written) -> line number
    std::map<uint16_t, uint16_t> lineAddress;       // line number -> address of the line
    bool renameVariables = true;
    char defaultEncoding = 'u';
    std::vector<Diagnostic> diagnostics;
    size_t scalarCount = 0, arrayCount = 0, longestLine = 0;
    uint16_t longestLineNumber = 0;
    std::map<std::string, std::string> variableMap;
    std::vector<EmittedLine> emittedLines;

    std::vector<uint8_t> emitLoadAddress();
    std::vector<uint8_t> emitLine(const BasicLine& line, uint16_t nextLineAddr);
    std::vector<uint8_t> emitLineWithSpaces(const BasicLine& line, uint16_t nextLineAddr);
    std::vector<uint8_t> emitToken(const BasicToken& token);
    std::vector<BasicLine> processLabels(std::vector<BasicLine>& lines);
    // Warnings: variables read but never assigned / assigned but never read,
    // code after an unconditional jump, lines too long to edit
    void checkVariables(const std::vector<std::vector<BasicToken>>& lines, const std::vector<int>& sourceOf);
    void checkReachable(const std::vector<BasicLine>& lines, const std::vector<int>& sourceOf,
                        const std::set<uint16_t>& targets);
    size_t listedLength(const std::vector<uint8_t>& lineData) const;
};
