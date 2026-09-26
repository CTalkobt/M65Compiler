#pragma once
#include "AsmParser.hpp"
#include <memory>

// Oscar parser — converts Oscar (vintage Commodore 6502 assembler) to semantic IR
// Handles Oscar-specific syntax:
// - Comments with * at line start or ; anywhere
// - Labels: NAME (followed by instruction or alone on line)
// - Directives: ORG, BYTE, WORD, HEX, DEC, ASCII, etc.
// - Standard 6502 addressing modes
class OscarParser : public AsmParser {
public:
    OscarParser() = default;
    AsmIR::Module parse(const std::string& source) override;

private:
    enum class TokenType {
        END_OF_FILE,
        NEWLINE,
        IDENTIFIER,
        NUMBER,
        STRING,
        DIRECTIVE,        // ORG, BYTE, WORD, HEX, DEC, etc.
        IMMEDIATE,        // #
        EQUALS,           // =
        COLON,
        COMMA,
        LPAREN,
        RPAREN,
        COMMENT,
        WHITESPACE
    };

    struct Token {
        TokenType type;
        std::string value;
        int line;
        int column;
    };

    // Lexer for Oscar syntax
    std::vector<Token> tokenize(const std::string& source);

    // Parser for Oscar syntax
    AsmIR::Module parseTokens(const std::vector<Token>& tokens);

    // Helpers
    AsmIR::AddressingMode inferAddressingMode(const std::string& operand);
    std::string normalizeMnemonic(const std::string& mnemonic);
    bool isDirective(const std::string& word);
    bool isMnemonic(const std::string& word);
    AsmIR::DirectiveType directiveTypeFromName(const std::string& name);
};
