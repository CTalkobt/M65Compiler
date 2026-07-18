#include "Merlin64Parser.hpp"
#include <cctype>
#include <sstream>
#include <algorithm>
#include <set>

AsmIR::Module Merlin64Parser::parse(const std::string& source) {
    AsmIR::Module module;

    try {
        auto tokens = tokenize(source);
        module = parseTokens(tokens);
        module.cpu = "6502";
        module.is_o45 = false;
    } catch (const std::exception& e) {
        addError(std::string("Parse error: ") + e.what());
    }

    return module;
}

std::vector<Merlin64Parser::Token> Merlin64Parser::tokenize(const std::string& source) {
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

        // Comments (semicolon anywhere on line)
        if (c == ';') {
            size_t end = source.find('\n', pos);
            if (end == std::string::npos) end = source.length();
            std::string comment = source.substr(pos, end - pos);
            tokens.push_back({TokenType::COMMENT, comment, line, column});
            pos = end;
            continue;
        }

        // Numbers (hex $xxxx, decimal)
        if (std::isdigit(c) || (c == '$' && pos + 1 < source.length() && std::isxdigit(source[pos + 1]))) {
            size_t start = pos;
            if (c == '$') {
                pos++;
                while (pos < source.length() && std::isxdigit(source[pos])) pos++;
            } else {
                while (pos < source.length() && std::isdigit(source[pos])) pos++;
            }
            std::string num = source.substr(start, pos - start);
            tokens.push_back({TokenType::NUMBER, num, line, column});
            column += pos - start;
            continue;
        }

        // Strings (single or double quoted)
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

        // Single character tokens
        TokenType type = TokenType::WHITESPACE;
        switch (c) {
            case '#': type = TokenType::IMMEDIATE; break;
            case '=': type = TokenType::EQUALS; break;
            case ',': type = TokenType::COMMA; break;
            case '(': type = TokenType::LPAREN; break;
            case ')': type = TokenType::RPAREN; break;
            default: break;
        }

        if (type != TokenType::WHITESPACE) {
            tokens.push_back({type, std::string(1, c), line, column});
            pos++;
            column++;
            continue;
        }

        // Identifiers (keywords, mnemonics, labels)
        if (std::isalpha(c) || c == '_') {
            size_t start = pos;
            while (pos < source.length() && (std::isalnum(source[pos]) || source[pos] == '_')) {
                pos++;
            }
            std::string ident = source.substr(start, pos - start);

            // Check if it's a directive
            if (isDirective(ident)) {
                tokens.push_back({TokenType::DIRECTIVE, ident, line, column});
            } else {
                tokens.push_back({TokenType::IDENTIFIER, ident, line, column});
            }
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

AsmIR::Module Merlin64Parser::parseTokens(const std::vector<Token>& tokens) {
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

        // Handle directives
        if (tok.type == TokenType::DIRECTIVE) {
            std::string dirName = tok.value;
            advance();

            AsmIR::Statement stmt;
            stmt.type = AsmIR::Statement::Type::DIRECTIVE;
            stmt.dir.type = directiveTypeFromName(dirName);
            stmt.dir.name = dirName;
            stmt.source_line = tok.line;

            // Collect arguments until end of line
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

        // Handle labels and instructions (identifiers)
        if (tok.type == TokenType::IDENTIFIER) {
            std::string potential_ident = tok.value;
            advance();

            // Check if this identifier is itself a mnemonic (instruction)
            if (isMnemonic(potential_ident)) {
                // It's an instruction, not a label
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

            // Check if this is a label: followed by directive, mnemonic, newline, or EOF
            bool is_label = peek().type == TokenType::DIRECTIVE ||
                           peek().type == TokenType::NEWLINE ||
                           peek().type == TokenType::END_OF_FILE ||
                           (peek().type == TokenType::IDENTIFIER && isMnemonic(peek().value));

            if (is_label) {
                AsmIR::Statement stmt;
                stmt.type = AsmIR::Statement::Type::LABEL;
                stmt.label = potential_ident;
                stmt.source_line = tok.line;
                module.statements.push_back(stmt);
                continue;
            }

        }

        // Skip unknown tokens
        advance();
    }

    return module;
}

AsmIR::AddressingMode Merlin64Parser::inferAddressingMode(const std::string& operand) {
    if (operand.empty()) return AsmIR::AddressingMode::IMPLIED;

    if (operand[0] == '$' || std::isdigit(operand[0])) {
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

std::string Merlin64Parser::normalizeMnemonic(const std::string& mnemonic) {
    std::string result = mnemonic;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

bool Merlin64Parser::isDirective(const std::string& word) {
    static const std::set<std::string> directives = {
        "ORG", "org",
        "BYTE", "byte",
        "WORD", "word",
        "HEX", "hex",
        "DEC", "dec",
        "ASCII", "ascii",
        "ASCIIZ", "asciiz",
        "GLOBAL", "global",
        "EXTERN", "extern",
        "WEAK", "weak"
    };
    return directives.count(word) > 0;
}

bool Merlin64Parser::isMnemonic(const std::string& word) {
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

AsmIR::DirectiveType Merlin64Parser::directiveTypeFromName(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "org") return AsmIR::DirectiveType::ORG;
    if (lower == "byte") return AsmIR::DirectiveType::BYTE_DATA;
    if (lower == "word") return AsmIR::DirectiveType::WORD_DATA;
    if (lower == "hex") return AsmIR::DirectiveType::BYTE_DATA;
    if (lower == "dec") return AsmIR::DirectiveType::OTHER;
    if (lower == "ascii") return AsmIR::DirectiveType::OTHER;
    if (lower == "asciiz") return AsmIR::DirectiveType::OTHER;
    if (lower == "global") return AsmIR::DirectiveType::GLOBAL;
    if (lower == "extern") return AsmIR::DirectiveType::EXTERN;
    if (lower == "weak") return AsmIR::DirectiveType::WEAK;
    return AsmIR::DirectiveType::OTHER;
}
