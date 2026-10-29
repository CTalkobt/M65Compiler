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
        LABEL,
        RAW,        // text stored as typed: the rest of a REM, a DATA item list
        EOL,
        END_OF_FILE
    };

    Type type;
    std::string value;
    uint8_t tokenByte = 0;   // For keywords (or 0xFE for escape keywords)
    uint8_t escapeByte = 0;  // For two-byte escape keywords
    uint8_t spaceBefore = 0; // blanks before it in the source (--preserve-spaces)
    char prefix = 0;         // STRING: 'p' PETSCII, 's' screen codes, 'a' ASCII (p"..", s"..", a"..", @".."), 0 none
};

// A disk command parameter: in a statement of a BASIC 65 disk command
// (DOPEN, DLOAD, BLOAD, RECORD, ...), a letter and digits like D0, U8, L20,
// P8192 (drive, unit, record length, address) is not a variable.
// statementStart: the index of the statement's first token
bool isDiskParameter(const std::vector<BasicToken>& tokens, size_t statementStart, size_t index);

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
    // BASIC 65's functions with the $CE prefix (POT, MOD, LOG10, ...)
    const std::unordered_map<std::string, uint8_t>& getCeKeywords() const { return ceKeywords; }

private:
    std::unordered_map<std::string, uint8_t> keywords;
    std::unordered_map<std::string, uint8_t> escapeKeywords;
    std::unordered_map<std::string, uint8_t> ceKeywords;

    void initializeKeywords();
    BasicToken parseString(const std::string& source, size_t& pos);
    BasicToken parseNumber(const std::string& source, size_t& pos);
    BasicToken parseIdentifier(const std::string& source, size_t& pos);
};
