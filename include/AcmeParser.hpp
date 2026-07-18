#pragma once
#include "AsmParser.hpp"
#include <memory>

// ACME parser — converts ACME 6502 assembly to semantic IR
// Handles ACME-specific syntax:
// - !pseudo-commands (!byte, !word, !align, !org, etc.)
// - Various number formats ($ hex, % binary, & octal, decimal)
// - Scoping with braces { }
// - *=address ORG syntax
// - Comments with ; or * at line start
class AcmeParser : public AsmParser {
public:
    AcmeParser() = default;
    AsmIR::Module parse(const std::string& source) override;

private:
    enum class TokenType {
        END_OF_FILE,
        NEWLINE,
        IDENTIFIER,
        NUMBER,
        STRING,
        PSEUDO_OP,       // !pseudo
        STAR,             // *
        EQUALS,           // =
        COLON,
        COMMA,
        LPAREN,
        RPAREN,
        LBRACE,
        RBRACE,
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

    // Lexer for ACME syntax
    std::vector<Token> tokenize(const std::string& source);

    // Parser for ACME syntax
    AsmIR::Module parseTokens(const std::vector<Token>& tokens);

    // Helpers
    AsmIR::AddressingMode inferAddressingMode(const std::string& operand);
    std::string normalizeMnemonic(const std::string& mnemonic);
};
