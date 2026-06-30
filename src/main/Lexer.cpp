#include "Lexer.hpp"
#include <cctype>
#include <map>
#include <cstdint>
#include <iostream>

Lexer::Lexer(const std::string& source) : source(source), pos(0), line(1), column(1), absLine(1), sourceFile("") {
    // Initialize with the main file context
    FileContext mainContext;
    mainContext.filename = "";  // Will be set by CodeGenerator
    mainContext.lineOffset = 1;
    mainContext.startAbsLine = 1;
    fileContextStack.push(mainContext);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        skipWhitespace();
        Token token = nextToken();
        tokens.push_back(token);
        if (token.type == TokenType::END_OF_FILE) {
            break;
        }
    }
    return tokens;
}

char Lexer::peek() const {
    if (pos >= source.length()) return '\0';
    return source[pos];
}

char Lexer::peekNext() const {
    if (pos + 1 >= source.length()) return '\0';
    return source[pos + 1];
}

char Lexer::get() {
    if (pos >= source.length()) return '\0';
    char c = source[pos++];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

void Lexer::skipWhitespace() {
    while (true) {
        if (std::isspace(peek())) {
            char c = get();
            if (c == '\n') absLine++;
        } else if (peek() == '/' && pos + 1 < source.length() && source[pos + 1] == '/') {
            // Single-line comment
            while (peek() != '\n' && peek() != '\0') {
                get();
            }
        } else if (peek() == '/' && pos + 1 < source.length() && source[pos + 1] == '*') {
            // Multi-line comment
            get(); // skip /
            get(); // skip *
            while (peek() != '\0' && !(peek() == '*' && pos + 1 < source.length() && source[pos + 1] == '/')) {
                char c = get();
                if (c == '\n') absLine++;
            }
            if (peek() == '*') {
                get(); // skip *
                get(); // skip /
            }
        } else if (peek() == '#' && pos + 5 < source.length() && source.substr(pos, 5) == "#line") {
            // #line N "file" — preprocessor line directive
            pos += 5; column += 5; // skip "#line"
            while (pos < source.length() && source[pos] == ' ') { pos++; column++; }
            // Parse line number
            int newLine = 0;
            while (pos < source.length() && std::isdigit(source[pos])) {
                newLine = newLine * 10 + (source[pos] - '0');
                pos++; column++;
            }
            // Parse filename (optional)
            std::string newFile = sourceFile;
            while (pos < source.length() && source[pos] == ' ') { pos++; column++; }
            if (pos < source.length() && source[pos] == '"') {
                pos++; column++; // skip opening quote
                newFile.clear();
                while (pos < source.length() && source[pos] != '"') {
                    newFile += source[pos++];
                    column++;
                }
                if (pos < source.length()) { pos++; column++; } // skip closing quote
            }
            // Skip rest of directive
            while (pos < source.length() && source[pos] != '\n') { pos++; column++; }

            // Record this file context mapping
            if (newLine > 0) {
                FileContext ctx;
                ctx.filename = newFile;
                ctx.lineOffset = newLine;
                ctx.startAbsLine = absLine;
                lineToFileMap[absLine] = ctx;

                handleLineDirective(newLine, newFile);
            }
        } else {
            break;
        }
    }
}

Token Lexer::nextToken() {
    char c = peek();
    if (c == '\0') {
        return {TokenType::END_OF_FILE, "", line, column, sourceFile};
    }

    if (std::isalpha(c) || c == '_') {
        return lexIdentifierOrKeyword();
    }

    if (std::isdigit(c)) {
        return lexNumber();
    }

    if (c == '@' && (peekNext() == '"' || peekNext() == '\'')) {
        get(); // consume '@'
        if (peek() == '"') return lexString(true);
        else return lexChar(true);
    }

    if (c == '"') {
        return lexString(false);
    }

    if (c == '\'') {
        return lexChar(false);
    }

    int startLine = line;
    int startCol = column;
    get();

    switch (c) {
        case '(': return {TokenType::OPEN_PAREN, "(", startLine, startCol, sourceFile};
        case ')': return {TokenType::CLOSE_PAREN, ")", startLine, startCol, sourceFile};
        case '{': return {TokenType::OPEN_BRACE, "{", startLine, startCol, sourceFile};
        case '}': return {TokenType::CLOSE_BRACE, "}", startLine, startCol, sourceFile};
        case '[': return {TokenType::OPEN_SQUARE, "[", startLine, startCol, sourceFile};
        case ']': return {TokenType::CLOSE_SQUARE, "]", startLine, startCol, sourceFile};
        case ';': return {TokenType::SEMICOLON, ";", startLine, startCol, sourceFile};
        case ':': return {TokenType::COLON, ":", startLine, startCol, sourceFile};
        case '?': return {TokenType::QUESTION_MARK, "?", startLine, startCol, sourceFile};
        case ',': return {TokenType::COMMA, ",", startLine, startCol, sourceFile};
        case '.':
            if (peek() == '.' && pos + 1 < source.length() && source[pos + 1] == '.') {
                get(); get(); return {TokenType::ELLIPSIS, "...", startLine, startCol, sourceFile};
            }
            return {TokenType::DOT, ".", startLine, startCol, sourceFile};
        case '=': 
            if (peek() == '=') { get(); return {TokenType::EQUALS_EQUALS, "==", startLine, startCol, sourceFile}; }
            return {TokenType::EQUALS, "=", startLine, startCol, sourceFile};
        case '+': 
            if (peek() == '+') { get(); return {TokenType::PLUS_PLUS, "++", startLine, startCol, sourceFile}; }
            if (peek() == '=') { get(); return {TokenType::PLUS_EQUALS, "+=", startLine, startCol, sourceFile}; }
            return {TokenType::PLUS, "+", startLine, startCol, sourceFile};
        case '-': 
            if (peek() == '>') { get(); return {TokenType::ARROW, "->", startLine, startCol, sourceFile}; }
            if (peek() == '-') { get(); return {TokenType::MINUS_MINUS, "--", startLine, startCol, sourceFile}; }
            if (peek() == '=') { get(); return {TokenType::MINUS_EQUALS, "-=", startLine, startCol, sourceFile}; }
            return {TokenType::MINUS, "-", startLine, startCol, sourceFile};
        case '*':
            if (peek() == '=') { get(); return {TokenType::STAR_EQUALS, "*=", startLine, startCol, sourceFile}; }
            return {TokenType::STAR, "*", startLine, startCol, sourceFile};
        case '/':
            if (peek() == '=') { get(); return {TokenType::SLASH_EQUALS, "/=", startLine, startCol, sourceFile}; }
            return {TokenType::SLASH, "/", startLine, startCol, sourceFile};
        case '%':
            if (peek() == '=') { get(); return {TokenType::PERCENT_EQUALS, "%=", startLine, startCol, sourceFile}; }
            return {TokenType::PERCENT, "%", startLine, startCol, sourceFile};
        case '<':
            if (peek() == '=') { get(); return {TokenType::LESS_EQUAL, "<=", startLine, startCol, sourceFile}; }
            if (peek() == '<') {
                get();
                if (peek() == '=') { get(); return {TokenType::LSHIFT_EQUALS, "<<=", startLine, startCol, sourceFile}; }
                return {TokenType::LSHIFT, "<<", startLine, startCol, sourceFile};
            }
            return {TokenType::LESS_THAN, "<", startLine, startCol, sourceFile};
        case '>':
            if (peek() == '=') { get(); return {TokenType::GREATER_EQUAL, ">=", startLine, startCol, sourceFile}; }
            if (peek() == '>') {
                get();
                if (peek() == '=') { get(); return {TokenType::RSHIFT_EQUALS, ">>=", startLine, startCol, sourceFile}; }
                return {TokenType::RSHIFT, ">>", startLine, startCol, sourceFile};
            }
            return {TokenType::GREATER_THAN, ">", startLine, startCol, sourceFile};
        case '!':
            if (peek() == '=') { get(); return {TokenType::NOT_EQUALS, "!=", startLine, startCol, sourceFile}; }
            return {TokenType::BANG, "!", startLine, startCol, sourceFile};
        case '&':
            if (peek() == '&') { get(); return {TokenType::AND, "&&", startLine, startCol, sourceFile}; }
            if (peek() == '=') { get(); return {TokenType::AMPERSAND_EQUALS, "&=", startLine, startCol, sourceFile}; }
            return {TokenType::AMPERSAND, "&", startLine, startCol, sourceFile};
        case '|':
            if (peek() == '|') { get(); return {TokenType::OR, "||", startLine, startCol, sourceFile}; }
            if (peek() == '=') { get(); return {TokenType::PIPE_EQUALS, "|=", startLine, startCol, sourceFile}; }
            return {TokenType::PIPE, "|", startLine, startCol, sourceFile};
        case '^':
            if (peek() == '=') { get(); return {TokenType::CARET_EQUALS, "^=", startLine, startCol, sourceFile}; }
            return {TokenType::CARET, "^", startLine, startCol, sourceFile};
        case '~': return {TokenType::TILDE, "~", startLine, startCol, sourceFile};
        default: return {TokenType::UNKNOWN, std::string(1, c), startLine, startCol, sourceFile};
    }
}

Token Lexer::lexIdentifierOrKeyword() {
    int startLine = line;
    int startCol = column;
    std::string value;
    while (std::isalnum(peek()) || peek() == '_') {
        value += get();
    }

    static const std::map<std::string, TokenType> keywords = {
        {"int", TokenType::INT},
        {"char", TokenType::CHAR},
        {"auto", TokenType::AUTO},
        {"unsigned", TokenType::UNSIGNED},
        {"signed", TokenType::SIGNED},
        {"return", TokenType::RETURN},
        {"void", TokenType::VOID},
        {"const", TokenType::CONST},
        {"if", TokenType::IF},
        {"else", TokenType::ELSE},
        {"while", TokenType::WHILE},
        {"for", TokenType::FOR},
        {"do", TokenType::DO},
        {"asm", TokenType::ASM},
        {"__asm__", TokenType::ASM},
        {"struct", TokenType::STRUCT},
        {"union", TokenType::UNION},
        {"volatile", TokenType::VOLATILE},
        {"_Static_assert", TokenType::STATIC_ASSERT},
        {"_Alignas", TokenType::ALIGNAS},
        {"_Alignof", TokenType::ALIGNOF},
        {"_Noreturn", TokenType::NORETURN},
        {"__attribute__", TokenType::ATTRIBUTE},
        {"__attribute", TokenType::ATTRIBUTE},
        {"__extension__", TokenType::EXTENSION},
        {"typeof", TokenType::TYPEOF},
        {"__typeof__", TokenType::TYPEOF},
        {"__typeof", TokenType::TYPEOF},
        {"break", TokenType::BREAK},
        {"continue", TokenType::CONTINUE},
        {"switch", TokenType::SWITCH},
        {"case", TokenType::CASE},
        {"default", TokenType::DEFAULT},
        {"goto", TokenType::GOTO},
        {"sizeof", TokenType::SIZEOF},
        {"_Generic", TokenType::GENERIC},
        {"typedef", TokenType::TYPEDEF},
        {"enum", TokenType::ENUM},
        {"extern", TokenType::EXTERN},
        {"static", TokenType::STATIC},
        {"_Bool", TokenType::BOOL},
        {"restrict", TokenType::RESTRICT},
        {"__restrict", TokenType::RESTRICT},
        {"__restrict__", TokenType::RESTRICT},
        {"short", TokenType::SHORT},
        {"long", TokenType::LONG},
        {"register", TokenType::REGISTER},
        {"inline", TokenType::INLINE},
        {"__inline", TokenType::INLINE},
        {"__inline__", TokenType::INLINE},
        {"__fastcall__", TokenType::FASTCALL},
        {"__interrupt", TokenType::INTERRUPT},
        {"__interrupt__", TokenType::INTERRUPT},
        {"__naked", TokenType::NAKED},
        {"__naked__", TokenType::NAKED},
        {"__unpacked", TokenType::UNPACKED},
        {"__unpacked__", TokenType::UNPACKED},
        {"repeat", TokenType::REPEAT},
        {"float", TokenType::FLOAT},
        {"double", TokenType::DOUBLE},
        {"__regparm", TokenType::REGPARM},
        {"__regparm__", TokenType::REGPARM},
        {"_Complex", TokenType::COMPLEX},
        {"__complex__", TokenType::COMPLEX},
        {"__real__", TokenType::REAL_PART},
        {"__imag__", TokenType::IMAG_PART},
        };

    auto it = keywords.find(value);
    if (it != keywords.end()) {
        return {it->second, value, startLine, startCol, sourceFile};
    }

    // Wide char/string prefix: L'x' or L"str" → downgrade to regular char/string
    if (value == "L" && (peek() == '\'' || peek() == '"')) {
        if (peek() == '"') return lexString(false);
        else return lexChar(false);
    }

    return {TokenType::IDENTIFIER, value, startLine, startCol, sourceFile};
}

Token Lexer::lexNumber() {
    int startLine = line;
    int startCol = column;
    std::string value;
    if (peek() == '0') {
        value += get();
        if (peek() == 'x' || peek() == 'X') {
            value += get();
            while (std::isxdigit(peek())) value += get();
            uint32_t val = 0;
            try {
                unsigned long long ull = std::stoull(value.substr(2), nullptr, 16);
                if (ull > 0xFFFFFFFF) {
                    std::cerr << startLine << ":" << startCol << ": warning: hex literal " << value << " overflows 32-bit integer" << std::endl;
                }
                val = static_cast<uint32_t>(ull);
            } catch (...) {
                std::cerr << startLine << ":" << startCol << ": warning: hex literal " << value << " overflows integer" << std::endl;
            }
            // Consume optional L/l/U/u suffixes
            while (peek() == 'L' || peek() == 'l' || peek() == 'U' || peek() == 'u') get();
            return {TokenType::INTEGER_LITERAL, std::to_string(val), startLine, startCol, sourceFile};
        }
    }
    while (std::isdigit(peek())) {
        value += get();
    }
    // Float literal: 0.0, 1.5f, 1.5e10, 1.5e-3, 2e5, etc.
    if (peek() == '.' && pos + 1 < source.length() && std::isdigit(source[pos + 1])) {
        value += get(); // consume '.'
        while (std::isdigit(peek())) value += get();
        // Optional exponent: e/E followed by optional sign and digits
        if (peek() == 'e' || peek() == 'E') {
            value += get();
            if (peek() == '+' || peek() == '-') value += get();
            while (std::isdigit(peek())) value += get();
        }
        // Handle suffix combinations: f, fi, if, i, l, li
        if (peek() == 'i' || peek() == 'I') {
            get(); // consume 'i'
            if (peek() == 'f' || peek() == 'F' || peek() == 'l' || peek() == 'L') get();
            return {TokenType::IMAGINARY_LITERAL, value, startLine, startCol, sourceFile};
        }
        if (peek() == 'f' || peek() == 'F' || peek() == 'l' || peek() == 'L') {
            get();
            if (peek() == 'i' || peek() == 'I') { get(); return {TokenType::IMAGINARY_LITERAL, value, startLine, startCol, sourceFile}; }
        }
        return {TokenType::FLOAT_LITERAL, value, startLine, startCol, sourceFile};
    }
    // Integer with negative exponent is float: 1e-2, 1E-3
    // Positive exponent (1e2) stays integer unless 'f' suffix
    if ((peek() == 'e' || peek() == 'E') && !value.empty() && std::isdigit(value[0])) {
        size_t savedPos2 = pos;
        char eCh = get(); // consume e/E
        bool isNeg = (peek() == '-');
        bool hasSuffix = false;
        std::string expPart;
        expPart += eCh;
        if (peek() == '+' || peek() == '-') expPart += get();
        if (std::isdigit(peek())) {
            while (std::isdigit(peek())) expPart += get();
            hasSuffix = (peek() == 'f' || peek() == 'F');
            if (hasSuffix) get();
            if (isNeg || hasSuffix) {
                // Negative exponent or explicit 'f' suffix → float
                value += expPart;
                return {TokenType::FLOAT_LITERAL, value, startLine, startCol, sourceFile};
            }
            // Positive exponent without suffix → integer
            value += expPart;
            try {
                double dval = std::stod(value);
                uint32_t ival = (uint32_t)dval;
                return {TokenType::INTEGER_LITERAL, std::to_string(ival), startLine, startCol, sourceFile};
            } catch (...) {
                return {TokenType::INTEGER_LITERAL, value, startLine, startCol, sourceFile};
            }
        }
        // No digits after e — backtrack
        pos = savedPos2;
    }
    // Check decimal literal overflow
    try {
        unsigned long long ull = std::stoull(value);
        if (ull > 0xFFFFFFFF) {
            std::cerr << startLine << ":" << startCol << ": warning: decimal literal " << value << " overflows 32-bit integer" << std::endl;
        }
    } catch (...) {
        std::cerr << startLine << ":" << startCol << ": warning: decimal literal " << value << " overflows integer" << std::endl;
    }
    // Consume optional L/l/U/u suffixes
    while (peek() == 'L' || peek() == 'l' || peek() == 'U' || peek() == 'u') get();
    // Integer imaginary literal: 200i, 1i
    if (peek() == 'i' || peek() == 'I') {
        get();
        if (peek() == 'f' || peek() == 'F') get(); // consume trailing f
        return {TokenType::IMAGINARY_LITERAL, value, startLine, startCol, sourceFile};
    }
    return {TokenType::INTEGER_LITERAL, value, startLine, startCol, sourceFile};
}

Token Lexer::lexString(bool ascii) {
    int startLine = line;
    int startCol = column;
    get(); // skip opening quote
    std::string value;
    while (peek() != '"' && peek() != '\0') {
        if (peek() == '\\') {
            get();
            char next = peek();
            switch (next) {
                case 'n': value += '\n'; get(); break;
                case 'r': value += '\r'; get(); break;
                case 't': value += '\t'; get(); break;
                default: value += get(); break;
            }
        } else {
            value += get();
        }
    }
    if (peek() == '"') get(); // skip closing quote
    if (ascii) return {TokenType::ASCII_STRING_LITERAL, value, startLine, startCol, sourceFile};
    return {TokenType::STRING_LITERAL, value, startLine, startCol, sourceFile};
}

Token Lexer::lexChar(bool ascii) {
    int startLine = line;
    int startCol = column;
    get(); // skip opening '
    char c = 0;
    if (peek() == '\\') {
        get();
        char next = get();
        switch (next) {
            case 'n': c = '\n'; break;
            case 'r': c = '\r'; break;
            case 't': c = '\t'; break;
            case '0': c = '\0'; break;
            default: c = next; break;
        }
    } else {
        c = get();
    }
    if (peek() == '\'') get(); // skip closing '
    if (!ascii) {
        // Convert to PETSCII to match .text string encoding
        uint8_t petscii = (uint8_t)c;
        if (c >= 'a' && c <= 'z') petscii = c - 32;
        else if (c >= 'A' && c <= 'Z') petscii = c + 32;
        return {TokenType::INTEGER_LITERAL, std::to_string((int)petscii), startLine, startCol, sourceFile};
    }
    // ASCII mode: preserve raw byte value
    return {TokenType::INTEGER_LITERAL, std::to_string((int)(uint8_t)c), startLine, startCol, sourceFile};
}

void Lexer::handleLineDirective(int newLine, const std::string& newFile) {
    // When we encounter a #line directive, we need to:
    // 1. Record the current context (for returning from includes)
    // 2. Update to the new file/line context

    // Check if we're returning to a previous file (newLine == 2 or similar after include)
    // or starting a new include (newLine == 1 with a different file)

    FileContext currentContext;
    currentContext.filename = newFile;
    currentContext.lineOffset = newLine;
    currentContext.startAbsLine = absLine;

    // Record this mapping for all lines in this context
    // When we encounter the next #line directive, we'll know this context is done

    sourceFile = newFile;
    line = newLine - 1;  // -1 because the next newline will increment it
}
