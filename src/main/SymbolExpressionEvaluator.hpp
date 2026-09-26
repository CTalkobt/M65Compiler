#ifndef SYMBOLEXPRESSIONEVALUATOR_HPP
#define SYMBOLEXPRESSIONEVALUATOR_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

class SymbolExpressionEvaluator {
public:
    SymbolExpressionEvaluator() = default;

    // Load symbols from file (format: hex_address symbol_name)
    void loadSymbolTable(const std::string& filename);

    // Add a symbol programmatically
    void addSymbol(const std::string& name, uint16_t value);

    // Evaluate expression: "label + 5", "sprite_base * 4", etc.
    // Returns the calculated value, or -1 on error
    int evaluateExpression(const std::string& expr);

    // Get a symbol value
    uint16_t getSymbol(const std::string& name) const;

    // Check if symbol exists
    bool hasSymbol(const std::string& name) const;

private:
    std::unordered_map<std::string, uint16_t> symbols;

    // Helper parsing methods
    struct Token {
        enum Type { NUMBER, SYMBOL, OPERATOR, LPAREN, RPAREN, END } type;
        std::string value;
    };

    class Tokenizer {
    public:
        Tokenizer(const std::string& expr) : expr(expr), pos(0) {}
        Token nextToken();

    private:
        std::string expr;
        size_t pos;
        std::string trimSpace(const std::string& s) const;
    };

    // Expression evaluation with operator precedence
    int parseExpression(const std::string& expr);
    int evaluateTokens(const std::vector<Token>& tokens);

    // Tokenization
    std::vector<Token> tokenize(const std::string& expr);
};

#endif  // SYMBOLEXPRESSIONEVALUATOR_HPP
