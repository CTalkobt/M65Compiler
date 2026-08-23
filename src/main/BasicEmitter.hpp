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
    BasicEmitter(uint16_t loadAddr = 0x0801, uint16_t lineIncrement = 10);

    std::vector<uint8_t> emit(const std::vector<BasicLine>& lines);
    std::vector<uint8_t> emitBinary(const std::string& sourceCode, bool useLabels = false);
    std::vector<uint8_t> emitFromLabels(const LabelBasedSourceParser& parser);

    const std::map<std::string, uint16_t>& getLabelMap() const;
    void outputLabelTable(const std::string& filename) const;

    void setLineIncrement(uint16_t increment) { lineIncrement = increment; }
    void setPreserveSpaces(bool preserve) { preserveSpaces = preserve; }

private:
    uint16_t loadAddress;
    uint16_t lineIncrement;
    bool preserveSpaces = false;
    std::map<std::string, uint16_t> labelMap;

    std::vector<uint8_t> emitLoadAddress();
    std::vector<uint8_t> emitLine(const BasicLine& line, uint16_t nextLineAddr);
    std::vector<uint8_t> emitLineWithSpaces(const BasicLine& line, uint16_t nextLineAddr);
    std::vector<uint8_t> emitToken(const BasicToken& token);
    std::vector<BasicLine> processLabels(std::vector<BasicLine>& lines);
};
