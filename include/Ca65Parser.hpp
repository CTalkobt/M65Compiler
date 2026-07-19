#pragma once
#include "AsmParser.hpp"
#include <memory>

// ca65 parser — converts ca65 (CC65) assembly to semantic IR
// Handles ca65-specific syntax:
// - .import / .export (instead of .extern / .global)
// - Local labels with @ prefix
// - .segment "name" directives
// - Similar but sometimes different directive names
class Ca65Parser : public AsmParser {
public:
    Ca65Parser() = default;
    AsmIR::Module parse(const std::string& source) override;

private:
    enum class TokenType {
        END_OF_FILE,
        NEWLINE,
        IDENTIFIER,
        NUMBER,
        STRING,
        SYMBOL,
        IMMEDIATE,
        DOT,
        AT,
        COLON,
        COMMA,
        LPAREN,
        RPAREN,
        SEMICOLON,
        COMMENT,
        WHITESPACE
    };

    struct Token {
        TokenType type;
        std::string value;
        int line;
        int column;
    };

    // Lexer for ca65 syntax
    std::vector<Token> tokenize(const std::string& source);

    // Parser for ca65 syntax
    AsmIR::Module parseTokens(const std::vector<Token>& tokens);

    // Helpers
    AsmIR::AddressingMode inferAddressingMode(const std::string& operand);
    std::string normalizeMnemonic(const std::string& mnemonic);
};
