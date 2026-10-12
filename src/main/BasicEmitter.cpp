#include "BasicEmitter.hpp"
#include "PETSCIIEncoder.hpp"
#include <cstring>
#include <sstream>
#include <regex>
#include <stdexcept>
#include <fstream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <iostream>

BasicEmitter::BasicEmitter(uint16_t loadAddr, uint16_t lineInc)
    : loadAddress(loadAddr), lineIncrement(lineInc) {}

std::vector<uint8_t> BasicEmitter::emitBinary(const std::string& sourceCode, bool useLabels) {
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
        } else if (token.type == BasicToken::LABEL) {
            // Label at start of line (no explicit line number)
            if (useLabels) {
                currentLineTokens.push_back(token);  // Keep label token for processing
                lineNumExpected = false;
                if (currentLineNum == 0) {
                    currentLineNum = 1;  // Placeholder - will be replaced by processLabels
                }
            }
        } else if (lineNumExpected && token.type == BasicToken::NUMBER) {
            currentLineNum = static_cast<uint16_t>(std::stoi(token.value));
            lineNumExpected = false;
        } else if (lineNumExpected && useLabels && token.type != BasicToken::LABEL) {
            // In label mode, if we see a statement without a line number or label, use placeholder
            currentLineNum = 1;
            lineNumExpected = false;
            currentLineTokens.push_back(token);
        } else {
            currentLineTokens.push_back(token);
        }
    }

    if (useLabels) {
        lines = processLabels(lines);
    }

    return emit(lines);
}

std::vector<uint8_t> BasicEmitter::emit(const std::vector<BasicLine>& lines) {
    std::vector<uint8_t> result;

    result = emitLoadAddress();

    // First pass: emit all lines with dummy address (0) to get their exact sizes
    std::vector<std::vector<uint8_t>> lineDataList;
    for (size_t i = 0; i < lines.size(); i++) {
        auto lineData = emitLineWithSpaces(lines[i], 0);  // Use dummy address, with spaces
        lineDataList.push_back(lineData);
    }

    // Line addresses: the first line is at the load address (the PRG's
    // 2-byte header is not loaded); each link points at the next line, the
    // last one at the $00 $00 that ends the program
    std::vector<uint16_t> lineAddrs;
    uint16_t currentAddr = loadAddress;
    lineAddress.clear();
    for (size_t i = 0; i < lines.size(); i++) {
        lineAddrs.push_back(currentAddr);
        lineAddress[lines[i].lineNumber] = currentAddr;
        currentAddr += lineDataList[i].size();
    }

    // Second pass: emit lines with correct addresses
    std::vector<uint8_t> programData;
    for (size_t i = 0; i < lines.size(); i++) {
        uint16_t nextLineAddr = (i + 1 < lines.size()) ? lineAddrs[i + 1] : currentAddr;
        auto lineData = emitLineWithSpaces(lines[i], nextLineAddr);
        programData.insert(programData.end(), lineData.begin(), lineData.end());
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

        case BasicToken::RAW: {
            auto encoded = petscii.encode(token.value);
            result.insert(result.end(), encoded.begin(), encoded.end());
            break;
        }

        case BasicToken::OPERATOR: {
            if (token.tokenByte != 0) {
                result.push_back(token.tokenByte);   // + - * / ^ > = <
            } else if (token.value == ":") {
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

        case BasicToken::LABEL:
            // Labels are not emitted in the binary
            break;
    }

    return result;
}

std::vector<uint8_t> BasicEmitter::emitLineWithSpaces(const BasicLine& line, uint16_t nextLineAddr) {
    std::vector<uint8_t> result;

    result.push_back(static_cast<uint8_t>(nextLineAddr & 0xFF));
    result.push_back(static_cast<uint8_t>((nextLineAddr >> 8) & 0xFF));

    result.push_back(static_cast<uint8_t>(line.lineNumber & 0xFF));
    result.push_back(static_cast<uint8_t>((line.lineNumber >> 8) & 0xFF));

    for (size_t i = 0; i < line.tokens.size(); i++) {
        const auto& token = line.tokens[i];
        auto tokenData = emitToken(token);
        result.insert(result.end(), tokenData.begin(), tokenData.end());

        // Add space after keywords when followed by identifier, number, or variable
        if (token.type == BasicToken::KEYWORD && i + 1 < line.tokens.size()) {
            const auto& nextToken = line.tokens[i + 1];
            if (token.value.back() == '(') {
                // TAB( SPC(: the parenthesis is part of the token
            } else if (nextToken.type == BasicToken::RAW) {
                // REM text keeps its own spacing; DATA items follow a space
                if (token.value == "DATA") result.push_back(0x20);
            } else if (nextToken.type == BasicToken::IDENTIFIER ||
                nextToken.type == BasicToken::NUMBER ||
                nextToken.type == BasicToken::OPERATOR) {
                // Don't add space before certain operators like (),:
                if (nextToken.type == BasicToken::OPERATOR &&
                    (nextToken.value == "(" || nextToken.value == "," || nextToken.value == ":")) {
                    // Skip space for these operators
                } else {
                    result.push_back(0x20);  // Space character
                }
            }
        }
    }

    result.push_back(0x00);

    return result;
}

std::vector<BasicLine> BasicEmitter::processLabels(std::vector<BasicLine>& lines) {
    std::vector<BasicLine> processedLines;
    labelMap.clear();

    uint16_t nextLineNum = lineIncrement;

    // First pass: collect labels and build mapping
    for (size_t idx = 0; idx < lines.size(); idx++) {
        auto& line = lines[idx];
        if (!line.tokens.empty() && line.tokens[0].type == BasicToken::LABEL) {
            labelMap[line.tokens[0].value] = nextLineNum;
        }
        nextLineNum += lineIncrement;
    }

    // Second pass: process tokens and generate line numbers
    nextLineNum = lineIncrement;
    for (auto& line : lines) {
        BasicLine newLine;
        newLine.lineNumber = nextLineNum;

        // Skip label token if present, process remaining tokens
        size_t startIdx = 0;
        if (!line.tokens.empty() && line.tokens[0].type == BasicToken::LABEL) {
            startIdx = 1;
        }

        // Process tokens, resolving label references
        for (size_t i = startIdx; i < line.tokens.size(); i++) {
            const auto& token = line.tokens[i];

            // Check if this is a GOTO/GOSUB followed by a label reference
            if ((token.type == BasicToken::KEYWORD &&
                 (token.value == "GOTO" || token.value == "GOSUB")) &&
                i + 1 < line.tokens.size() &&
                line.tokens[i + 1].type == BasicToken::IDENTIFIER) {

                // Add the GOTO/GOSUB token
                newLine.tokens.push_back(token);

                // Replace the label reference with its line number
                const std::string& labelName = line.tokens[i + 1].value;
                auto it = labelMap.find(labelName);
                if (it != labelMap.end()) {
                    BasicToken lineNumToken;
                    lineNumToken.type = BasicToken::NUMBER;
                    lineNumToken.value = std::to_string(it->second);
                    newLine.tokens.push_back(lineNumToken);
                    i++;  // Skip the label reference token
                } else {
                    // Undefined label - keep as-is (will cause runtime error)
                    newLine.tokens.push_back(line.tokens[i + 1]);
                    i++;
                }
            } else {
                newLine.tokens.push_back(token);
            }
        }

        processedLines.push_back(newLine);
        nextLineNum += lineIncrement;
    }

    return processedLines;
}

const std::map<std::string, uint16_t>& BasicEmitter::getLabelMap() const {
    return labelMap;
}

void BasicEmitter::outputLabelTable(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file) {
        std::cerr << "Warning: Could not open label table file: " << filename << std::endl;
        return;
    }

    file << "Label\tLine Number\tMemory Address\n";
    // in line order
    std::vector<std::pair<uint16_t, std::string>> rows;
    for (const auto& [label, lineNum] : labelMap) rows.push_back({lineNum, label});
    std::sort(rows.begin(), rows.end());
    for (const auto& [lineNum, label] : rows) {
        auto it = lineAddress.find(lineNum);
        uint16_t memAddr = it != lineAddress.end() ? it->second : 0;
        file << label << "\t" << lineNum << "\t0x" << std::hex << std::setw(4)
             << std::setfill('0') << memAddr << std::dec << "\n";
    }
}

std::vector<uint8_t> BasicEmitter::emitFromLabels(const LabelBasedSourceParser& parser) {
    // Each source line with code is one BASIC line, numbered by the parser
    // (assignLineNumbers); label references become line numbers
    BasicTokenizer tokenizer;
    std::vector<BasicLine> lines;

    const auto& labels = parser.getLabelMap();   // upper-case label -> line number
    auto isLabel = [&](const std::string& n) { return labels.count(n) > 0; };
    labelMap.clear();

    for (const auto& srcLine : parser.getSourceLines()) {
        if (!srcLine.label.empty()) {
            auto it = labels.find(LabelBasedSourceParser::upper(srcLine.label));
            if (it != labels.end()) labelMap[srcLine.label] = it->second;
        }
        if (srcLine.code.empty()) continue;

        auto tokens = tokenizer.tokenize(srcLine.code);
        LabelBasedSourceParser::forEachLabelRef(tokens, isLabel,
            [&](size_t idx, const std::string& name) {
                auto it = labels.find(name);
                if (it == labels.end()) return;   // undefined: the validator reports it
                tokens[idx].type = BasicToken::NUMBER;
                tokens[idx].value = std::to_string(it->second);
            });

        BasicLine line;
        line.lineNumber = srcLine.basicLineNum;
        for (const auto& token : tokens) {
            if (token.type != BasicToken::EOL && token.type != BasicToken::END_OF_FILE &&
                token.type != BasicToken::LABEL)
                line.tokens.push_back(token);
        }
        if (!line.tokens.empty()) lines.push_back(line);
    }

    // Labels after the last line of code name a line of their own (":")
    if (parser.getTrailingLineNum() != 0) {
        BasicLine line;
        line.lineNumber = parser.getTrailingLineNum();
        BasicToken colon;
        colon.type = BasicToken::OPERATOR;
        colon.value = ":";
        line.tokens.push_back(colon);
        lines.push_back(line);
    }

    // Emit the lines
    return emit(lines);
}
