#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include "BasicTokenizer.hpp"

struct BasicLine {
    uint16_t lineNumber;
    std::vector<BasicToken> tokens;
};

class BasicEmitter {
public:
    BasicEmitter(uint16_t loadAddr = 0x0801);

    std::vector<uint8_t> emit(const std::vector<BasicLine>& lines);
    std::vector<uint8_t> emitBinary(const std::string& sourceCode);

private:
    uint16_t loadAddress;

    std::vector<uint8_t> emitLoadAddress();
    std::vector<uint8_t> emitLine(const BasicLine& line, uint16_t nextLineAddr);
    std::vector<uint8_t> emitToken(const BasicToken& token);
};
