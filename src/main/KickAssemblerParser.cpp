#include "KickAssemblerParser.hpp"
#include "MacroUtils.hpp"
#include <cctype>
#include <sstream>
#include <algorithm>
#include <map>

AsmIR::Module KickAssemblerParser::parse(const std::string& source) {
    AsmIR::Module module;

    try {
        // Phase 3c: Process .include directives first
        std::set<std::string> processedFiles;
        std::vector<std::string> includeErrors;
        std::string withIncludes = MacroUtils::processIncludes(source, module, processedFiles, includeErrors);
        for (const auto& err : includeErrors) {
            addWarning(err);
        }

        // Phase 3b: Process conditional compilation directives
        std::string processedSource = MacroUtils::processConditionals(withIncludes, module);

        auto tokens = tokenize(processedSource);
        module = parseTokens(tokens);
        module.cpu = "45GS02";
        module.is_o45 = false;
    } catch (const std::exception& e) {
        addError(std::string("Parse error: ") + e.what());
    }

    return module;
}

std::vector<KickAssemblerParser::Token> KickAssemblerParser::tokenize(const std::string& source) {
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

        // Comments
        if (c == ';') {
            size_t end = source.find('\n', pos);
            if (end == std::string::npos) end = source.length();
            std::string comment = source.substr(pos, end - pos);
            tokens.push_back({TokenType::COMMENT, comment, line, column});
            pos = end;
            continue;
        }

        // Two-character comment
        if (c == '/' && pos + 1 < source.length() && source[pos + 1] == '/') {
            size_t end = source.find('\n', pos);
            if (end == std::string::npos) end = source.length();
            std::string comment = source.substr(pos, end - pos);
            tokens.push_back({TokenType::COMMENT, comment, line, column});
            pos = end;
            continue;
        }

        // Directives (starting with !)
        if (c == '!' && pos + 1 < source.length() && std::isalpha(source[pos + 1])) {
            pos++;  // skip !
            size_t start = pos;
            while (pos < source.length() && (std::isalnum(source[pos]) || source[pos] == '_')) {
                pos++;
            }
            std::string name = source.substr(start, pos - start);
            tokens.push_back({TokenType::DIRECTIVE, name, line, column});
            column += pos - start + 1;
            continue;
        }

        // Numbers (hex, binary, decimal)
        if (std::isdigit(c)) {
            size_t start = pos;
            while (pos < source.length() && std::isdigit(source[pos])) pos++;
            std::string num = source.substr(start, pos - start);
            tokens.push_back({TokenType::NUMBER, num, line, column});
            column += pos - start;
            continue;
        }

        // Hex numbers starting with $
        if (c == '$' && pos + 1 < source.length() && std::isxdigit(source[pos + 1])) {
            size_t start = pos;
            pos++;  // skip $
            while (pos < source.length() && std::isxdigit(source[pos])) pos++;
            std::string num = source.substr(start, pos - start);
            tokens.push_back({TokenType::NUMBER, num, line, column});
            column += pos - start;
            continue;
        }

        // Binary numbers starting with %
        if (c == '%' && pos + 1 < source.length() && (source[pos + 1] == '0' || source[pos + 1] == '1')) {
            size_t start = pos;
            pos++;  // skip %
            while (pos < source.length() && (source[pos] == '0' || source[pos] == '1')) pos++;
            std::string num = source.substr(start, pos - start);
            tokens.push_back({TokenType::NUMBER, num, line, column});
            column += pos - start;
            continue;
        }

        // Identifiers and labels
        if (std::isalpha(c) || c == '_') {
            size_t start = pos;
            while (pos < source.length() && (std::isalnum(source[pos]) || source[pos] == '_' || source[pos] == '.')) {
                pos++;
            }
            std::string ident = source.substr(start, pos - start);
            tokens.push_back({TokenType::IDENTIFIER, ident, line, column});
            column += pos - start;
            continue;
        }

        // String literals
        if (c == '"') {
            size_t start = pos++;
            while (pos < source.length() && source[pos] != '"') {
                if (source[pos] == '\\') pos++;
                pos++;
            }
            if (pos < source.length()) pos++;  // consume closing quote
            std::string str = source.substr(start, pos - start);
            tokens.push_back({TokenType::STRING, str, line, column});
            column += pos - start;
            continue;
        }

        // Single character tokens
        TokenType type = TokenType::WHITESPACE;
        switch (c) {
            case '#': type = TokenType::IMMEDIATE; break;
            case '$': type = TokenType::DOLLAR; break;
            case '%': type = TokenType::PERCENT; break;
            case ',': type = TokenType::COMMA; break;
            case ':': type = TokenType::COLON; break;
            case '(': type = TokenType::LPAREN; break;
            case ')': type = TokenType::RPAREN; break;
            case '{': type = TokenType::LBRACE; break;
            case '}': type = TokenType::RBRACE; break;
            default: break;
        }

        if (type != TokenType::WHITESPACE) {
            tokens.push_back({type, std::string(1, c), line, column});
        }

        pos++;
        column++;
    }

    tokens.push_back({TokenType::END_OF_FILE, "", line, column});
    return tokens;
}

AsmIR::Module KickAssemblerParser::parseTokens(const std::vector<Token>& tokens) {
    AsmIR::Module module;
    std::string currentNamespace;
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

        // Handle namespace
        if (tok.type == TokenType::DIRECTIVE && tok.value == "namespace") {
            advance();
            if (peek().type == TokenType::IDENTIFIER) {
                currentNamespace = peek().value;
                advance();
            }
            continue;
        }

        if (tok.type == TokenType::DIRECTIVE && tok.value == "endnamespace") {
            advance();
            currentNamespace.clear();
            continue;
        }

        // Handle labels
        if (tok.type == TokenType::IDENTIFIER && pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::COLON) {
            std::string label = tok.value;
            if (!currentNamespace.empty()) {
                label = currentNamespace + "." + label;
            }

            AsmIR::Statement stmt;
            stmt.type = AsmIR::Statement::Type::LABEL;
            stmt.label = label;
            stmt.source_line = tok.line;
            module.statements.push_back(stmt);

            advance();  // identifier
            advance();  // colon
            continue;
        }

        // Handle instructions
        if (tok.type == TokenType::IDENTIFIER) {
            std::string mnemonic = tok.value;
            std::transform(mnemonic.begin(), mnemonic.end(), mnemonic.begin(), ::toupper);

            AsmIR::Statement stmt;
            stmt.type = AsmIR::Statement::Type::INSTRUCTION;
            stmt.instr.mnemonic = mnemonic;
            stmt.source_line = tok.line;

            advance();

            // Parse operand
            std::string operandText;
            bool hasImmediate = false;

            while (peek().type != TokenType::NEWLINE && peek().type != TokenType::END_OF_FILE &&
                   peek().type != TokenType::COMMENT && peek().type != TokenType::SEMICOLON) {
                if (peek().type == TokenType::IMMEDIATE) {
                    hasImmediate = true;
                    advance();  // skip # token itself
                    continue;
                }
                if (!operandText.empty() && peek().type != TokenType::LPAREN && peek().type != TokenType::RPAREN) {
                    // Add separator for non-parentheses
                    if (peek().type == TokenType::COMMA) {
                        operandText += ",";
                        advance();
                        continue;
                    }
                }
                operandText += peek().value;
                advance();
            }

            stmt.instr.operand.text = operandText;
            stmt.instr.mode = inferAddressingMode(operandText, hasImmediate);
            module.statements.push_back(stmt);
            continue;
        }

        // Handle directives
        if (tok.type == TokenType::DIRECTIVE) {
            std::string dirName = tok.value;
            advance();

            AsmIR::Statement stmt;
            stmt.type = AsmIR::Statement::Type::DIRECTIVE;

            // Map KickAssembler directives to IR directives
            if (dirName == "byte") {
                stmt.dir.type = AsmIR::DirectiveType::BYTE_DATA;
                stmt.dir.name = "byte";
            } else if (dirName == "word") {
                stmt.dir.type = AsmIR::DirectiveType::WORD_DATA;
                stmt.dir.name = "word";
            } else if (dirName == "long") {
                stmt.dir.type = AsmIR::DirectiveType::LONG_DATA;
                stmt.dir.name = "long";
            } else if (dirName == "fill") {
                stmt.dir.type = AsmIR::DirectiveType::FILL;
                stmt.dir.name = "fill";
            } else if (dirName == "align") {
                stmt.dir.type = AsmIR::DirectiveType::ALIGN;
                stmt.dir.name = "align";
            } else if (dirName == "org") {
                stmt.dir.type = AsmIR::DirectiveType::ORG;
                stmt.dir.name = "org";
            } else if (dirName == "import" || dirName == "export") {
                stmt.dir.type = AsmIR::DirectiveType::GLOBAL;
                stmt.dir.name = dirName;
            } else {
                stmt.dir.type = AsmIR::DirectiveType::OTHER;
                stmt.dir.name = dirName;
            }

            // Collect arguments until end of line
            while (peek().type != TokenType::NEWLINE && peek().type != TokenType::END_OF_FILE) {
                if (peek().type != TokenType::COMMENT) {
                    stmt.dir.arguments.push_back(peek().value);
                }
                advance();
            }

            stmt.source_line = tok.line;
            module.statements.push_back(stmt);
            continue;
        }

        advance();
    }

    return module;
}

AsmIR::AddressingMode KickAssemblerParser::inferAddressingMode(const std::string& operand, bool hasImmediate) {
    if (operand.empty()) return AsmIR::AddressingMode::IMPLIED;

    if (hasImmediate) return AsmIR::AddressingMode::IMMEDIATE;

    if (operand[0] == '$') {
        if (operand.find(",x") != std::string::npos || operand.find(",X") != std::string::npos)
            return AsmIR::AddressingMode::ABSOLUTE_X;
        if (operand.find(",y") != std::string::npos || operand.find(",Y") != std::string::npos)
            return AsmIR::AddressingMode::ABSOLUTE_Y;
        return AsmIR::AddressingMode::ABSOLUTE;
    }

    if (operand.find("($") != std::string::npos && operand.back() == ')')
        return AsmIR::AddressingMode::INDIRECT;

    if (operand.find(",x") != std::string::npos || operand.find(",X") != std::string::npos)
        return AsmIR::AddressingMode::ABSOLUTE_X;
    if (operand.find(",y") != std::string::npos || operand.find(",Y") != std::string::npos)
        return AsmIR::AddressingMode::ABSOLUTE_Y;

    return AsmIR::AddressingMode::ABSOLUTE;
}
