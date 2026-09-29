#pragma once
#include "AsmParser.hpp"
#include <memory>

// KickAssembler parser — converts KickAssembler assembly to semantic IR
// Handles KickAssembler-specific syntax:
// - Namespaces (!namespace / !endnamespace)
// - Pseudo-directives (!byte, !word, !align, etc.)
// - Block syntax (!align { code })
// - Scoped labels (namespace.label)
class KickAssemblerParser : public AsmParser {
public:
    KickAssemblerParser() = default;
    AsmIR::Module parse(const std::string& source) override;

private:
    enum class TokenType {
        END_OF_FILE,
        NEWLINE,
        IDENTIFIER,
        NUMBER,
        STRING,
        SYMBOL,
        IMMEDIATE,     // #
        DOLLAR,        // $
        PERCENT,       // %
        COMMA,
        COLON,
        LPAREN,
        RPAREN,
        LBRACE,
        RBRACE,
        SEMICOLON,
        COMMENT,
        DIRECTIVE,     // !directive
        WHITESPACE
    };

    struct Token {
        TokenType type;
        std::string value;
        int line;
        int column;
    };

    // Lexer for KickAssembler syntax
    std::vector<Token> tokenize(const std::string& source);

    // Parser for KickAssembler syntax
    AsmIR::Module parseTokens(const std::vector<Token>& tokens);

    // Helpers
    AsmIR::AddressingMode inferAddressingMode(const std::string& operand, bool hasImmediate = false);
    void flattenNamespaces(std::vector<AsmIR::Statement>& statements);
    std::string resolveScopedLabel(const std::string& label, const std::string& currentNamespace);
};
