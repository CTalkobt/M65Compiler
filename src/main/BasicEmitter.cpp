#include "BasicEmitter.hpp"
#include "PETSCIIEncoder.hpp"
#include <cstring>
#include <sstream>
#include <regex>
#include <stdexcept>

BasicEmitter::BasicEmitter(uint16_t loadAddr) : loadAddress(loadAddr) {}

std::vector<uint8_t> BasicEmitter::emitBinary(const std::string& sourceCode) {
    BasicTokenizer tokenizer;
    PETSCIIEncoder petscii;

    std::vector<BasicLine> lines;
    std::vector<BasicToken> currentLineTokens;
    uint16_t currentLineNum = 0;
    bool lineNumExpected = true;

    auto tokens = tokenizer.tokenize(sourceCode);

    for (size_t i = 0; i < tokens.size(); i++) {
        const auto& token = tokens[i];

        if (token.type == BasicToken::EOL) {
            if (currentLineNum > 0 && !currentLineTokens.empty()) {
                BasicLine line;
                line.lineNumber = currentLineNum;
                line.tokens = currentLineTokens;
                lines.push_back(line);

                currentLineTokens.clear();
                currentLineNum = 0;
            }
            lineNumExpected = true;
        } else if (token.type == BasicToken::END_OF_FILE) {
            // Save the last line if it has tokens
            if (currentLineNum > 0 && !currentLineTokens.empty()) {
                BasicLine line;
                line.lineNumber = currentLineNum;
                line.tokens = currentLineTokens;
                lines.push_back(line);
            }
            break;
        } else if (lineNumExpected && token.type == BasicToken::NUMBER) {
            currentLineNum = static_cast<uint16_t>(std::stoi(token.value));
            lineNumExpected = false;
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
    uint16_t currentAddr = loadAddress + 2;  // Address where program starts (after load address bytes)

    for (size_t i = 0; i < lines.size(); i++) {
        // Calculate where the next line will start
        uint16_t nextLineAddr;
        if (i + 1 < lines.size()) {
            // Temporarily emit line to know its size
            auto tempLine = emitLine(lines[i], 0);
            nextLineAddr = currentAddr + tempLine.size();
        } else {
            // Last line: next address is 0 (program end)
            nextLineAddr = 0;
        }

        // Now emit with the correct next address
        auto lineData = emitLine(lines[i], nextLineAddr);
        programData.insert(programData.end(), lineData.begin(), lineData.end());
        currentAddr += lineData.size();
    }

    result.insert(result.end(), programData.begin(), programData.end());

    // Program end marker
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
