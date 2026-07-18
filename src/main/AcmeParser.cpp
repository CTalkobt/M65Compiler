#include "AcmeParser.hpp"
#include "MacroUtils.hpp"
#include <cctype>
#include <sstream>
#include <algorithm>

AsmIR::Module AcmeParser::parse(const std::string& source) {
    AsmIR::Module module;

    try {
        // Phase 3b: Process conditional compilation directives first
        std::string processedSource = MacroUtils::processConditionals(source, module);

        auto tokens = tokenize(processedSource);
        module = parseTokens(tokens);
        module.cpu = "6502";
        module.is_o45 = false;

        // Extract macros from processed source
        MacroUtils::extractACMEMacros(processedSource, module);
    } catch (const std::exception& e) {
        addError(std::string("Parse error: ") + e.what());
    }

    return module;
}

std::vector<AcmeParser::Token> AcmeParser::tokenize(const std::string& source) {
    std::vector<Token> tokens;
    size_t pos = 0;
    int line = 1, column = 1;

    while (pos < source.length()) {
        char c = source[pos];

        // Skip whitespace (except newlines)
        if ((c == ' ' || c == '\t') && c != '\n') {
            pos++;
            column++;
            continue;
        }

        // Newline
        if (c == '\n') {
            tokens.push_back({TokenType::NEWLINE, "\\n", line, column});
            pos++;
            line++;
            column = 1;
            continue;
        }

        // Comments (semicolon or * at line start after whitespace)
        if (c == ';' || c == '*') {
            size_t end = source.find('\n', pos);
            if (end == std::string::npos) end = source.length();
            std::string comment = source.substr(pos, end - pos);
            tokens.push_back({TokenType::COMMENT, comment, line, column});
            pos = end;
            continue;
        }

        // Pseudo-ops (starting with !)
        if (c == '!' && pos + 1 < source.length() && std::isalpha(source[pos + 1])) {
            pos++;  // skip !
            size_t start = pos;
            while (pos < source.length() && (std::isalnum(source[pos]) || source[pos] == '_')) {
                pos++;
            }
            std::string name = source.substr(start, pos - start);
            tokens.push_back({TokenType::PSEUDO_OP, name, line, column});
            column += pos - start + 1;
            continue;
        }

        // Numbers (hex $xxxx, binary %xxxx, octal &xxxx, decimal)
        if (std::isdigit(c) || c == '$' || c == '%' || c == '&') {
            size_t start = pos;
            if (c == '$' && pos + 1 < source.length() && std::isxdigit(source[pos + 1])) {
                pos++;
                while (pos < source.length() && std::isxdigit(source[pos])) pos++;
            } else if (c == '%' && pos + 1 < source.length() && (source[pos + 1] == '0' || source[pos + 1] == '1')) {
                pos++;
                while (pos < source.length() && (source[pos] == '0' || source[pos] == '1')) pos++;
            } else if (c == '&' && pos + 1 < source.length() && std::isdigit(source[pos + 1])) {
                pos++;
                while (pos < source.length() && source[pos] >= '0' && source[pos] <= '7') pos++;
            } else if (std::isdigit(c)) {
                while (pos < source.length() && std::isdigit(source[pos])) pos++;
            } else {
                break;
            }
            std::string num = source.substr(start, pos - start);
            tokens.push_back({TokenType::NUMBER, num, line, column});
            column += pos - start;
            continue;
        }

        // Strings
        if (c == '"') {
            size_t start = pos++;
            while (pos < source.length() && source[pos] != '"') {
                if (source[pos] == '\\') pos++;
                pos++;
            }
            if (pos < source.length()) pos++;
            std::string str = source.substr(start, pos - start);
            tokens.push_back({TokenType::STRING, str, line, column});
            column += pos - start;
            continue;
        }

        // Single character tokens (handle before identifiers to properly detect ':' as COLON not IDENTIFIER start)
        TokenType type = TokenType::WHITESPACE;
        switch (c) {
            case '#': type = TokenType::IMMEDIATE; break;
            case '*': type = TokenType::STAR; break;
            case '=': type = TokenType::EQUALS; break;
            case ':': type = TokenType::COLON; break;
            case ',': type = TokenType::COMMA; break;
            case '(': type = TokenType::LPAREN; break;
            case ')': type = TokenType::RPAREN; break;
            case '{': type = TokenType::LBRACE; break;
            case '}': type = TokenType::RBRACE; break;
            default: break;
        }

        if (type != TokenType::WHITESPACE) {
            tokens.push_back({type, std::string(1, c), line, column});
            pos++;
            column++;
            continue;
        }

        // Identifiers (no longer starting with ':')
        if (std::isalpha(c) || c == '_') {
            size_t start = pos;
            while (pos < source.length() && (std::isalnum(source[pos]) || source[pos] == '_')) {
                pos++;
            }
            std::string ident = source.substr(start, pos - start);
            tokens.push_back({TokenType::IDENTIFIER, ident, line, column});
            column += pos - start;
            continue;
        }

        // Skip any other character
        pos++;
        column++;
    }

    tokens.push_back({TokenType::END_OF_FILE, "", line, column});
    return tokens;
}

AsmIR::Module AcmeParser::parseTokens(const std::vector<Token>& tokens) {
    AsmIR::Module module;
    size_t pos = 0;

    auto peek = [&]() -> const Token& {
        return pos < tokens.size() ? tokens[pos] : tokens.back();
    };

    auto advance = [&]() {
        if (pos < tokens.size()) pos++;
    };

    auto match = [&](TokenType type) {
        if (peek().type == type) {
            advance();
            return true;
        }
        return false;
    };

    while (peek().type != TokenType::END_OF_FILE) {
        const Token& tok = peek();

        // Skip newlines and comments
        if (match(TokenType::NEWLINE) || match(TokenType::COMMENT)) {
            continue;
        }

        // Handle ORG (*=address)
        if (tok.type == TokenType::STAR && pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::EQUALS) {
            advance();  // *
            advance();  // =

            AsmIR::Statement stmt;
            stmt.type = AsmIR::Statement::Type::DIRECTIVE;
            stmt.dir.type = AsmIR::DirectiveType::ORG;
            stmt.dir.name = "org";
            stmt.source_line = tok.line;

            if (peek().type == TokenType::NUMBER) {
                stmt.dir.arguments.push_back(peek().value);
                advance();
            }

            module.statements.push_back(stmt);
            continue;
        }

        // Handle pseudo-ops
        if (tok.type == TokenType::PSEUDO_OP) {
            std::string opName = tok.value;
            advance();

            AsmIR::Statement stmt;
            stmt.type = AsmIR::Statement::Type::DIRECTIVE;
            stmt.source_line = tok.line;

            if (opName == "byte") {
                stmt.dir.type = AsmIR::DirectiveType::BYTE_DATA;
                stmt.dir.name = "byte";
            } else if (opName == "word") {
                stmt.dir.type = AsmIR::DirectiveType::WORD_DATA;
                stmt.dir.name = "word";
            } else if (opName == "long") {
                stmt.dir.type = AsmIR::DirectiveType::LONG_DATA;
                stmt.dir.name = "long";
            } else if (opName == "align") {
                stmt.dir.type = AsmIR::DirectiveType::ALIGN;
                stmt.dir.name = "align";
            } else {
                stmt.dir.type = AsmIR::DirectiveType::OTHER;
                stmt.dir.name = opName;
            }

            // Collect arguments until end of line
            while (peek().type != TokenType::NEWLINE && peek().type != TokenType::END_OF_FILE) {
                if (peek().type != TokenType::COMMENT) {
                    stmt.dir.arguments.push_back(peek().value);
                }
                advance();
            }

            module.statements.push_back(stmt);
            continue;
        }

        // Handle labels (identifier not followed by instruction mnemonic)
        if (tok.type == TokenType::IDENTIFIER) {
            // Lookahead to see if this is a label or instruction
            std::string potential_label = tok.value;
            advance();

            // If followed by colon, it's definitely a label
            if (peek().type == TokenType::COLON) {
                AsmIR::Statement stmt;
                stmt.type = AsmIR::Statement::Type::LABEL;
                stmt.label = potential_label;
                stmt.source_line = tok.line;
                module.statements.push_back(stmt);
                advance();  // colon
                continue;
            }

            // Otherwise, treat as instruction
            std::string mnemonic = normalizeMnemonic(potential_label);

            AsmIR::Statement stmt;
            stmt.type = AsmIR::Statement::Type::INSTRUCTION;
            stmt.instr.mnemonic = mnemonic;
            stmt.source_line = tok.line;

            // Parse operand
            std::string operandText;
            bool hasImmediate = false;
            while (peek().type != TokenType::NEWLINE && peek().type != TokenType::END_OF_FILE &&
                   peek().type != TokenType::COMMENT) {
                if (peek().type == TokenType::IMMEDIATE) {
                    hasImmediate = true;
                    advance();  // skip # token
                    continue;
                }
                operandText += peek().value;
                advance();
                if (peek().type == TokenType::COMMA) {
                    operandText += ",";
                    advance();
                }
            }

            stmt.instr.operand.text = operandText;
            stmt.instr.mode = inferAddressingMode(operandText);
            if (hasImmediate) {
                stmt.instr.mode = AsmIR::AddressingMode::IMMEDIATE;
            }
            module.statements.push_back(stmt);
            continue;
        }

        // Skip unknown tokens
        advance();
    }

    return module;
}

AsmIR::AddressingMode AcmeParser::inferAddressingMode(const std::string& operand) {
    if (operand.empty()) return AsmIR::AddressingMode::IMPLIED;

    if (operand[0] == '$' || operand[0] == '%' || operand[0] == '&' || std::isdigit(operand[0])) {
        if (operand.find(",x") != std::string::npos || operand.find(",X") != std::string::npos)
            return AsmIR::AddressingMode::ABSOLUTE_X;
        if (operand.find(",y") != std::string::npos || operand.find(",Y") != std::string::npos)
            return AsmIR::AddressingMode::ABSOLUTE_Y;
        return AsmIR::AddressingMode::ABSOLUTE;
    }

    if (operand[0] == '(') {
        if (operand.find(",x)") != std::string::npos)
            return AsmIR::AddressingMode::INDIRECT_X;
        if (operand.find("),y") != std::string::npos)
            return AsmIR::AddressingMode::INDIRECT_Y;
        return AsmIR::AddressingMode::INDIRECT;
    }

    return AsmIR::AddressingMode::ABSOLUTE;
}

std::string AcmeParser::normalizeMnemonic(const std::string& mnemonic) {
    std::string result = mnemonic;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}
