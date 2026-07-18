#include "Ca65Parser.hpp"
#include "MacroUtils.hpp"
#include <cctype>
#include <sstream>
#include <algorithm>

AsmIR::Module Ca65Parser::parse(const std::string& source) {
    AsmIR::Module module;

    try {
        // Phase 3b: Process conditional compilation directives first
        std::string processedSource = MacroUtils::processConditionals(source, module);

        auto tokens = tokenize(processedSource);
        module = parseTokens(tokens);
        module.cpu = "6502";  // ca65 is primarily for 6502/65C02
        module.is_o45 = false;

        // Extract macros from processed source
        MacroUtils::extractCa65Macros(processedSource, module);
    } catch (const std::exception& e) {
        addError(std::string("Parse error: ") + e.what());
    }

    return module;
}

std::vector<Ca65Parser::Token> Ca65Parser::tokenize(const std::string& source) {
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

        // Comments (; or //)
        if (c == ';' || (c == '/' && pos + 1 < source.length() && source[pos + 1] == '/')) {
            size_t end = source.find('\n', pos);
            if (end == std::string::npos) end = source.length();
            std::string comment = source.substr(pos, end - pos);
            tokens.push_back({TokenType::COMMENT, comment, line, column});
            pos = end;
            continue;
        }

        // Block comments /* */
        if (c == '/' && pos + 1 < source.length() && source[pos + 1] == '*') {
            size_t end = source.find("*/", pos + 2);
            if (end == std::string::npos) end = source.length();
            else end += 2;
            std::string comment = source.substr(pos, end - pos);
            // Count newlines in comment for line tracking
            for (char ch : comment) {
                if (ch == '\n') line++;
            }
            tokens.push_back({TokenType::COMMENT, comment, line, column});
            pos = end;
            continue;
        }

        // Numbers (hex $xxxx, decimal, binary %xxxx)
        if (std::isdigit(c) || c == '$' || (c == '%' && pos + 1 < source.length())) {
            size_t start = pos;
            if (c == '$' && pos + 1 < source.length() && std::isxdigit(source[pos + 1])) {
                pos++;
                while (pos < source.length() && std::isxdigit(source[pos])) pos++;
            } else if (c == '%' && pos + 1 < source.length() && (source[pos + 1] == '0' || source[pos + 1] == '1')) {
                pos++;
                while (pos < source.length() && (source[pos] == '0' || source[pos] == '1')) pos++;
            } else if (std::isdigit(c)) {
                while (pos < source.length() && std::isdigit(source[pos])) pos++;
            } else {
                break;  // Not a number
            }
            std::string num = source.substr(start, pos - start);
            tokens.push_back({TokenType::NUMBER, num, line, column});
            column += pos - start;
            continue;
        }

        // Directives (starting with .)
        if (c == '.' && pos + 1 < source.length() && std::isalpha(source[pos + 1])) {
            pos++;  // skip .
            size_t start = pos;
            while (pos < source.length() && (std::isalnum(source[pos]) || source[pos] == '_')) {
                pos++;
            }
            std::string name = source.substr(start, pos - start);
            tokens.push_back({TokenType::DOT, name, line, column});
            column += pos - start + 1;
            continue;
        }

        // Strings
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

        // Local labels (@label)
        if (c == '@' && pos + 1 < source.length() && (std::isalpha(source[pos + 1]) || source[pos + 1] == '_')) {
            pos++;
            size_t start = pos;
            while (pos < source.length() && (std::isalnum(source[pos]) || source[pos] == '_')) {
                pos++;
            }
            std::string label = source.substr(start, pos - start);
            tokens.push_back({TokenType::AT, label, line, column});
            column += pos - start + 1;
            continue;
        }

        // Identifiers and labels
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

        // Single character tokens
        TokenType type = TokenType::WHITESPACE;
        switch (c) {
            case ':': type = TokenType::COLON; break;
            case ',': type = TokenType::COMMA; break;
            case '(': type = TokenType::LPAREN; break;
            case ')': type = TokenType::RPAREN; break;
            case '#': type = TokenType::IMMEDIATE; break;
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

AsmIR::Module Ca65Parser::parseTokens(const std::vector<Token>& tokens) {
    AsmIR::Module module;
    size_t pos = 0;
    std::string currentSegment = "CODE";

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

        // Handle directives
        if (tok.type == TokenType::DOT) {
            std::string dirName = tok.value;
            advance();

            AsmIR::Statement stmt;
            stmt.type = AsmIR::Statement::Type::DIRECTIVE;
            stmt.source_line = tok.line;

            // Map ca65 directives to IR directives
            if (dirName == "byte") {
                stmt.dir.type = AsmIR::DirectiveType::BYTE_DATA;
                stmt.dir.name = "byte";
            } else if (dirName == "word") {
                stmt.dir.type = AsmIR::DirectiveType::WORD_DATA;
                stmt.dir.name = "word";
            } else if (dirName == "dword") {
                stmt.dir.type = AsmIR::DirectiveType::LONG_DATA;
                stmt.dir.name = "long";
            } else if (dirName == "import") {
                stmt.dir.type = AsmIR::DirectiveType::EXTERN;
                stmt.dir.name = "extern";
            } else if (dirName == "export") {
                stmt.dir.type = AsmIR::DirectiveType::GLOBAL;
                stmt.dir.name = "global";
            } else if (dirName == "segment") {
                stmt.dir.type = AsmIR::DirectiveType::SEGMENT;
                stmt.dir.name = "segment";
                if (peek().type == TokenType::STRING) {
                    currentSegment = peek().value;
                    stmt.dir.arguments.push_back(peek().value);
                    advance();
                }
                module.statements.push_back(stmt);
                continue;
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

            module.statements.push_back(stmt);
            continue;
        }

        // Handle labels (identifier followed by colon)
        if (tok.type == TokenType::IDENTIFIER && pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::COLON) {
            std::string label = tok.value;
            AsmIR::Statement stmt;
            stmt.type = AsmIR::Statement::Type::LABEL;
            stmt.label = label;
            stmt.source_line = tok.line;
            module.statements.push_back(stmt);

            advance();  // identifier
            advance();  // colon
            continue;
        }

        // Handle local labels (@label:)
        if (tok.type == TokenType::AT && pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::COLON) {
            std::string label = "@" + tok.value;
            AsmIR::Statement stmt;
            stmt.type = AsmIR::Statement::Type::LABEL;
            stmt.label = label;
            stmt.source_line = tok.line;
            module.statements.push_back(stmt);

            advance();  // @label
            advance();  // colon
            continue;
        }

        // Handle instructions
        if (tok.type == TokenType::IDENTIFIER) {
            std::string mnemonic = normalizeMnemonic(tok.value);

            AsmIR::Statement stmt;
            stmt.type = AsmIR::Statement::Type::INSTRUCTION;
            stmt.instr.mnemonic = mnemonic;
            stmt.source_line = tok.line;

            advance();

            // Parse operand
            std::string operandText;
            bool hasImmediate = false;

            while (peek().type != TokenType::NEWLINE && peek().type != TokenType::END_OF_FILE &&
                   peek().type != TokenType::COMMENT) {
                if (peek().type == TokenType::IMMEDIATE) {
                    hasImmediate = true;
                    advance();
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

        advance();
    }

    return module;
}

AsmIR::AddressingMode Ca65Parser::inferAddressingMode(const std::string& operand) {
    if (operand.empty()) return AsmIR::AddressingMode::IMPLIED;

    if (operand[0] == '$') {
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

    if (operand.find(",x") != std::string::npos || operand.find(",X") != std::string::npos)
        return AsmIR::AddressingMode::ABSOLUTE_X;
    if (operand.find(",y") != std::string::npos || operand.find(",Y") != std::string::npos)
        return AsmIR::AddressingMode::ABSOLUTE_Y;

    return AsmIR::AddressingMode::ABSOLUTE;
}

std::string Ca65Parser::normalizeMnemonic(const std::string& mnemonic) {
    std::string result = mnemonic;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}
