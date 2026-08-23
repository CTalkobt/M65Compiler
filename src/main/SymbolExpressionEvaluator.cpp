#include "SymbolExpressionEvaluator.hpp"
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <stdexcept>

void SymbolExpressionEvaluator::loadSymbolTable(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Cannot open symbol table: " + filename);
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;

        std::istringstream iss(line);
        uint16_t addr;
        std::string name;

        // Format: hex_address symbol_name
        if (iss >> std::hex >> addr >> name) {
            symbols[name] = addr;
        }
    }
}

void SymbolExpressionEvaluator::addSymbol(const std::string& name, uint16_t value) {
    symbols[name] = value;
}

uint16_t SymbolExpressionEvaluator::getSymbol(const std::string& name) const {
    auto it = symbols.find(name);
    if (it == symbols.end()) {
        throw std::runtime_error("Undefined symbol: " + name);
    }
    return it->second;
}

bool SymbolExpressionEvaluator::hasSymbol(const std::string& name) const {
    return symbols.find(name) != symbols.end();
}

std::string SymbolExpressionEvaluator::Tokenizer::trimSpace(const std::string& s) const {
    size_t start = s.find_first_not_of(" \t");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
}

SymbolExpressionEvaluator::Token SymbolExpressionEvaluator::Tokenizer::nextToken() {
    // Skip whitespace
    while (pos < expr.length() && std::isspace(expr[pos])) {
        pos++;
    }

    if (pos >= expr.length()) {
        return Token{Token::END, ""};
    }

    char ch = expr[pos];

    // Parentheses
    if (ch == '(') {
        pos++;
        return Token{Token::LPAREN, "("};
    }
    if (ch == ')') {
        pos++;
        return Token{Token::RPAREN, ")"};
    }

    // Operators
    if (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '%') {
        pos++;
        return Token{Token::OPERATOR, std::string(1, ch)};
    }

    // Numbers (hex or decimal)
    if (std::isdigit(ch) || (ch == '0' && pos + 1 < expr.length() && expr[pos + 1] == 'x')) {
        size_t start = pos;
        if (ch == '0' && expr[pos + 1] == 'x') {
            // Hex number
            pos += 2;
            while (pos < expr.length() && std::isxdigit(expr[pos])) {
                pos++;
            }
        } else {
            // Decimal number
            while (pos < expr.length() && std::isdigit(expr[pos])) {
                pos++;
            }
        }
        return Token{Token::NUMBER, expr.substr(start, pos - start)};
    }

    // Symbols (identifiers)
    if (std::isalpha(ch) || ch == '_') {
        size_t start = pos;
        while (pos < expr.length() && (std::isalnum(expr[pos]) || expr[pos] == '_')) {
            pos++;
        }
        return Token{Token::SYMBOL, expr.substr(start, pos - start)};
    }

    // Unknown character
    throw std::runtime_error("Invalid character in expression: " + std::string(1, ch));
}

std::vector<SymbolExpressionEvaluator::Token> SymbolExpressionEvaluator::tokenize(const std::string& expr) {
    std::vector<Token> tokens;
    Tokenizer tokenizer(expr);

    Token token = tokenizer.nextToken();
    while (token.type != Token::END) {
        tokens.push_back(token);
        token = tokenizer.nextToken();
    }

    return tokens;
}

int SymbolExpressionEvaluator::evaluateTokens(const std::vector<Token>& tokens) {
    // Simple recursive descent parser with operator precedence
    // This is a simplified implementation - for production, consider using a proper expression parser

    if (tokens.empty()) {
        throw std::runtime_error("Empty expression");
    }

    // For now, support simple expressions like: symbol + number, number * number, etc.
    if (tokens.size() == 1) {
        const auto& token = tokens[0];
        if (token.type == Token::NUMBER) {
            return std::stoi(token.value, nullptr, 0);  // Handles both hex (0x) and decimal
        } else if (token.type == Token::SYMBOL) {
            return getSymbol(token.value);
        }
    }

    if (tokens.size() == 3) {
        // binary operation: operand1 op operand2
        int left = 0;
        if (tokens[0].type == Token::NUMBER) {
            left = std::stoi(tokens[0].value, nullptr, 0);
        } else if (tokens[0].type == Token::SYMBOL) {
            left = getSymbol(tokens[0].value);
        }

        int right = 0;
        if (tokens[2].type == Token::NUMBER) {
            right = std::stoi(tokens[2].value, nullptr, 0);
        } else if (tokens[2].type == Token::SYMBOL) {
            right = getSymbol(tokens[2].value);
        }

        const std::string& op = tokens[1].value;
        if (op == "+") return left + right;
        if (op == "-") return left - right;
        if (op == "*") return left * right;
        if (op == "/") {
            if (right == 0) throw std::runtime_error("Division by zero");
            return left / right;
        }
        if (op == "%") {
            if (right == 0) throw std::runtime_error("Modulo by zero");
            return left % right;
        }
    }

    // For more complex expressions, would need full recursive descent parser
    throw std::runtime_error("Unsupported expression format");
}

int SymbolExpressionEvaluator::evaluateExpression(const std::string& expr) {
    try {
        auto tokens = tokenize(expr);
        return evaluateTokens(tokens);
    } catch (const std::exception& e) {
        // Return error
        return -1;
    }
}

int SymbolExpressionEvaluator::parseExpression(const std::string& expr) {
    return evaluateExpression(expr);
}
