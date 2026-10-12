#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <map>
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
    std::map<std::string, std::string> variableMap;
    std::vector<EmittedLine> emittedLines;

    std::vector<uint8_t> emitLoadAddress();
    std::vector<uint8_t> emitLine(const BasicLine& line, uint16_t nextLineAddr);
    std::vector<uint8_t> emitLineWithSpaces(const BasicLine& line, uint16_t nextLineAddr);
    std::vector<uint8_t> emitToken(const BasicToken& token);
    std::vector<BasicLine> processLabels(std::vector<BasicLine>& lines);
};
