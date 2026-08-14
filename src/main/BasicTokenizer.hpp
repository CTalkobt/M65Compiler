#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

struct BasicToken {
    enum Type {
        KEYWORD,
        STRING,
        NUMBER,
        IDENTIFIER,
        OPERATOR,
        EOL,
        END_OF_FILE
    };

    Type type;
    std::string value;
    uint8_t tokenByte = 0;   // For keywords (or 0xFE for escape keywords)
    uint8_t escapeByte = 0;  // For two-byte escape keywords
};

class BasicTokenizer {
public:
    BasicTokenizer();

    std::vector<BasicToken> tokenize(const std::string& source);
    uint8_t getKeywordToken(const std::string& keyword) const;
    bool isKeyword(const std::string& word) const;
    bool isEscapeKeyword(const std::string& word) const;
    uint8_t getEscapeKeywordByte(const std::string& keyword) const;
    const std::unordered_map<std::string, uint8_t>& getKeywords() const;
    const std::unordered_map<std::string, uint8_t>& getEscapeKeywords() const;

private:
    std::unordered_map<std::string, uint8_t> keywords;
    std::unordered_map<std::string, uint8_t> escapeKeywords;

    void initializeKeywords();
    BasicToken parseString(const std::string& source, size_t& pos);
    BasicToken parseNumber(const std::string& source, size_t& pos);
    BasicToken parseIdentifier(const std::string& source, size_t& pos);
};
