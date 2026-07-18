#pragma once
#include "AsmParser.hpp"
#include <memory>

// Merlin 64 parser — converts Merlin 64 (Commodore 6502 assembler) to semantic IR
// Handles Merlin 64-specific syntax:
// - Comments with ; (anywhere on line)
// - Labels: at line start, followed by space or instruction
// - Directives: ORG, BYTE, WORD, HEX, DEC, etc.
// - Standard 6502 addressing modes
class Merlin64Parser : public AsmParser {
public:
    Merlin64Parser() = default;
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

    // Lexer for Merlin 64 syntax
    std::vector<Token> tokenize(const std::string& source);

    // Parser for Merlin 64 syntax
    AsmIR::Module parseTokens(const std::vector<Token>& tokens);

    // Helpers
    AsmIR::AddressingMode inferAddressingMode(const std::string& operand);
    std::string normalizeMnemonic(const std::string& mnemonic);
    bool isDirective(const std::string& word);
    bool isMnemonic(const std::string& word);
    AsmIR::DirectiveType directiveTypeFromName(const std::string& name);
};
