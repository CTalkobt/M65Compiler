#include "X65Parser.hpp"
#include "MacroUtils.hpp"
#include <cctype>
#include <sstream>
#include <algorithm>
#include <set>

AsmIR::Module X65Parser::parse(const std::string& source) {
    AsmIR::Module module;

    try {
        // Phase 3b: Process conditional compilation directives first
        std::string processedSource = MacroUtils::processConditionals(source, module);

        auto tokens = tokenize(processedSource);
        module = parseTokens(tokens);
        module.cpu = "6502";
        module.is_o45 = false;
    } catch (const std::exception& e) {
        addError(std::string("Parse error: ") + e.what());
    }

    return module;
}

std::vector<X65Parser::Token> X65Parser::tokenize(const std::string& source) {
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

        // Comments (semicolon anywhere)
        if (c == ';') {
            size_t end = source.find('\n', pos);
            if (end == std::string::npos) end = source.length();
            std::string comment = source.substr(pos, end - pos);
            tokens.push_back({TokenType::COMMENT, comment, line, column});
            pos = end;
            continue;
        }

        // Numbers ($xxxx hex, %binary, decimal)
        if (std::isdigit(c) || c == '$' || c == '%') {
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
                break;
            }
            std::string num = source.substr(start, pos - start);
            tokens.push_back({TokenType::NUMBER, num, line, column});
            column += pos - start;
            continue;
        }

        // Strings (double or single quoted)
        if (c == '"' || c == '\'') {
            char quote = c;
            size_t start = pos++;
            while (pos < source.length() && source[pos] != quote) {
                if (source[pos] == '\\') pos++;
                pos++;
            }
            if (pos < source.length()) pos++;
            std::string str = source.substr(start, pos - start);
            tokens.push_back({TokenType::STRING, str, line, column});
            column += pos - start;
            continue;
        }

        // Dot directives (.import, .export, etc.)
        if (c == '.' && pos + 1 < source.length() && std::isalpha(source[pos + 1])) {
            size_t start = pos++;
            while (pos < source.length() && (std::isalnum(source[pos]) || source[pos] == '_')) {
                pos++;
            }
            std::string directive = source.substr(start, pos - start);
            tokens.push_back({TokenType::DOT_DIRECTIVE, directive, line, column});
            column += pos - start;
            continue;
        }

        // Single character tokens
        TokenType type = TokenType::WHITESPACE;
        switch (c) {
            case '#': type = TokenType::IMMEDIATE; break;
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

        // Identifiers
        if (std::isalpha(c) || c == '_' || c == '@') {
            size_t start = pos;
            while (pos < source.length() && (std::isalnum(source[pos]) || source[pos] == '_')) {
                pos++;
            }
            std::string ident = source.substr(start, pos - start);
            tokens.push_back({TokenType::IDENTIFIER, ident, line, column});
            column += pos - start;
            continue;
        }

        // Skip unknown character
        pos++;
        column++;
    }

    tokens.push_back({TokenType::END_OF_FILE, "", line, column});
    return tokens;
}

AsmIR::Module X65Parser::parseTokens(const std::vector<Token>& tokens) {
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

        // Handle dot directives
        if (tok.type == TokenType::DOT_DIRECTIVE) {
            std::string dirName = tok.value;
            advance();

            AsmIR::Statement stmt;
            stmt.type = AsmIR::Statement::Type::DIRECTIVE;
            stmt.dir.type = directiveTypeFromName(dirName);
            stmt.dir.name = dirName;
            stmt.source_line = tok.line;

            // Collect arguments until newline
            while (peek().type != TokenType::NEWLINE && peek().type != TokenType::END_OF_FILE &&
                   peek().type != TokenType::COMMENT) {
                stmt.dir.arguments.push_back(peek().value);
                advance();
                if (peek().type == TokenType::COMMA) {
                    advance();
                }
            }

            module.statements.push_back(stmt);
            continue;
        }

        // Handle labels and instructions
        if (tok.type == TokenType::IDENTIFIER) {
            std::string potential_ident = tok.value;
            advance();

            // Check if it's a mnemonic
            if (isMnemonic(potential_ident)) {
                // It's an instruction
                std::string mnemonic = normalizeMnemonic(potential_ident);

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

            // Check if it's a label: followed by colon, directive, or mnemonic
            bool is_label = peek().type == TokenType::COLON ||
                           peek().type == TokenType::DOT_DIRECTIVE ||
                           peek().type == TokenType::NEWLINE ||
                           peek().type == TokenType::END_OF_FILE ||
                           (peek().type == TokenType::IDENTIFIER && isMnemonic(peek().value));

            if (is_label) {
                AsmIR::Statement stmt;
                stmt.type = AsmIR::Statement::Type::LABEL;
                stmt.label = potential_ident;
                stmt.source_line = tok.line;
                module.statements.push_back(stmt);
                if (peek().type == TokenType::COLON) {
                    advance();
                }
                continue;
            }
        }

        // Skip unknown tokens
        advance();
    }

    return module;
}

AsmIR::AddressingMode X65Parser::inferAddressingMode(const std::string& operand) {
    if (operand.empty()) return AsmIR::AddressingMode::IMPLIED;

    if (operand[0] == '$' || operand[0] == '%' || std::isdigit(operand[0])) {
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

std::string X65Parser::normalizeMnemonic(const std::string& mnemonic) {
    std::string result = mnemonic;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

bool X65Parser::isMnemonic(const std::string& word) {
    static const std::set<std::string> mnemonics = {
        "ADC", "AND", "ASL", "BCC", "BCS", "BEQ", "BIT", "BMI", "BNE", "BPL", "BRK", "BVC", "BVS",
        "CLC", "CLD", "CLI", "CLV", "CMP", "CPX", "CPY", "DEC", "DEX", "DEY", "EOR", "INC", "INX",
        "INY", "JMP", "JSR", "LDA", "LDX", "LDY", "LSR", "NOP", "ORA", "PHA", "PHP", "PLA", "PLP",
        "ROL", "ROR", "RTI", "RTS", "SBC", "SEC", "SED", "SEI", "STA", "STX", "STY", "TAX", "TAY",
        "TSX", "TXA", "TXS", "TYA"
    };
    std::string upper = word;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    return mnemonics.count(upper) > 0;
}

AsmIR::DirectiveType X65Parser::directiveTypeFromName(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == ".byte") return AsmIR::DirectiveType::BYTE_DATA;
    if (lower == ".word") return AsmIR::DirectiveType::WORD_DATA;
    if (lower == ".dword") return AsmIR::DirectiveType::LONG_DATA;
    if (lower == ".import") return AsmIR::DirectiveType::EXTERN;
    if (lower == ".export") return AsmIR::DirectiveType::GLOBAL;
    if (lower == ".org") return AsmIR::DirectiveType::ORG;
    if (lower == ".segment") return AsmIR::DirectiveType::OTHER;
    return AsmIR::DirectiveType::OTHER;
}
