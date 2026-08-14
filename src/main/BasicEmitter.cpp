#include "BasicEmitter.hpp"
#include "PETSCIIEncoder.hpp"
#include <cstring>
#include <sstream>
#include <regex>

BasicEmitter::BasicEmitter(uint16_t loadAddr) : loadAddress(loadAddr) {}

std::vector<uint8_t> BasicEmitter::emitBinary(const std::string& sourceCode) {
    BasicTokenizer tokenizer;
    PETSCIIEncoder petscii;

    std::vector<BasicLine> lines;
    uint16_t currentLineNum = 10;
    std::vector<BasicToken> currentLineTokens;

    auto tokens = tokenizer.tokenize(sourceCode);

    for (const auto& token : tokens) {
        if (token.type == BasicToken::EOL || token.type == BasicToken::END_OF_FILE) {
            if (!currentLineTokens.empty()) {
                BasicLine line;
                line.lineNumber = currentLineNum;
                line.tokens = currentLineTokens;
                lines.push_back(line);

                currentLineTokens.clear();
                currentLineNum += 10;
            }
            if (token.type == BasicToken::END_OF_FILE) {
                break;
            }
        } else {
            currentLineTokens.push_back(token);
        }
    }

    return emit(lines);
}

std::vector<uint8_t> BasicEmitter::emit(const std::vector<BasicLine>& lines) {
    std::vector<uint8_t> result;

    result = emitLoadAddress();

    std::vector<uint8_t> programData;

    for (size_t i = 0; i < lines.size(); i++) {
        uint16_t nextLineAddr;
        if (i + 1 < lines.size()) {
            nextLineAddr = loadAddress + result.size() + programData.size() + 8;
        } else {
            nextLineAddr = 0;
        }

        auto lineData = emitLine(lines[i], nextLineAddr);
        programData.insert(programData.end(), lineData.begin(), lineData.end());
    }

    result.insert(result.end(), programData.begin(), programData.end());

    result.push_back(0x00);
    result.push_back(0x00);

    return result;
}

std::vector<uint8_t> BasicEmitter::emitLoadAddress() {
    std::vector<uint8_t> result;
    uint16_t addr = loadAddress;

    result.push_back(static_cast<uint8_t>(addr & 0xFF));
    result.push_back(static_cast<uint8_t>((addr >> 8) & 0xFF));

    return result;
}

std::vector<uint8_t> BasicEmitter::emitLine(const BasicLine& line, uint16_t nextLineAddr) {
    std::vector<uint8_t> result;

    result.push_back(static_cast<uint8_t>(nextLineAddr & 0xFF));
    result.push_back(static_cast<uint8_t>((nextLineAddr >> 8) & 0xFF));

    result.push_back(static_cast<uint8_t>(line.lineNumber & 0xFF));
    result.push_back(static_cast<uint8_t>((line.lineNumber >> 8) & 0xFF));

    for (const auto& token : line.tokens) {
        auto tokenData = emitToken(token);
        result.insert(result.end(), tokenData.begin(), tokenData.end());
    }

    result.push_back(0x00);

    return result;
}

std::vector<uint8_t> BasicEmitter::emitToken(const BasicToken& token) {
    PETSCIIEncoder petscii;
    std::vector<uint8_t> result;

    switch (token.type) {
        case BasicToken::KEYWORD:
            result.push_back(token.tokenByte);
            if (token.escapeByte != 0) {
                result.push_back(token.escapeByte);
            }
            break;

        case BasicToken::STRING: {
            result.push_back(0x22);
            auto encoded = petscii.encode(token.value);
            result.insert(result.end(), encoded.begin(), encoded.end());
            result.push_back(0x22);
            break;
        }

        case BasicToken::NUMBER: {
            auto encoded = petscii.encode(token.value);
            result.insert(result.end(), encoded.begin(), encoded.end());
            break;
        }

        case BasicToken::IDENTIFIER: {
            auto encoded = petscii.encode(token.value);
            result.insert(result.end(), encoded.begin(), encoded.end());
            break;
        }

        case BasicToken::OPERATOR: {
            if (token.value == ":") {
                result.push_back(0x3A);
            } else {
                auto encoded = petscii.encode(token.value);
                result.insert(result.end(), encoded.begin(), encoded.end());
            }
            break;
        }

        case BasicToken::EOL:
        case BasicToken::END_OF_FILE:
            break;
    }

    return result;
}
