#pragma once
#include "AsmParser.hpp"
#include <memory>

// x65 parser — converts x65 (alternative ca65 assembler) to semantic IR
// Handles x65-specific syntax:
// - Comments: ; anywhere on line
// - Directives: .import, .export, .segment, etc.
// - Local labels: @label syntax
// - Standard 6502 addressing modes
class X65Parser : public AsmParser {
public:
    X65Parser() = default;
    AsmIR::Module parse(const std::string& source) override;

private:
    enum class TokenType {
        END_OF_FILE,
        NEWLINE,
        IDENTIFIER,
        NUMBER,
        STRING,
        DOT_DIRECTIVE,    // .import, .export, etc.
        IMMEDIATE,        // #
        COLON,
        COMMA,
        LPAREN,
        RPAREN,
        LBRACE,
        RBRACE,
        COMMENT,
        WHITESPACE
    };

    struct Token {
        TokenType type;
        std::string value;
        int line;
        int column;
    };

    // Lexer for x65 syntax
    std::vector<Token> tokenize(const std::string& source);

    // Parser for x65 syntax
    AsmIR::Module parseTokens(const std::vector<Token>& tokens);

    // Helpers
    AsmIR::AddressingMode inferAddressingMode(const std::string& operand);
    std::string normalizeMnemonic(const std::string& mnemonic);
    bool isMnemonic(const std::string& word);
    AsmIR::DirectiveType directiveTypeFromName(const std::string& name);
};
