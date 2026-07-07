#include "AssemblerLexer.hpp"
#include <cctype>
#include <set>
#include <algorithm>

AssemblerLexer::AssemblerLexer(const std::string& source) : source(source), pos(0), line(1), column(1) {}

std::vector<AssemblerToken> AssemblerLexer::tokenize() {
    std::vector<AssemblerToken> tokens;
    while (true) {
        skipWhitespaceAndComments();
        AssemblerToken token = nextToken();
        tokens.push_back(token);
        if (token.type == AssemblerTokenType::END_OF_FILE) {
            break;
        }
    }
    return tokens;
}

char AssemblerLexer::peek() const {
    if (pos >= source.length()) return '\0';
    return source[pos];
}

char AssemblerLexer::get() {
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

void AssemblerLexer::skipWhitespaceAndComments() {
    while (true) {
        if (peek() == ' ' || peek() == '\t' || peek() == '\r') {
            get();
        } else if (peek() == ';') {
            while (peek() != '\n' && peek() != '\0') {
                get();
            }
        } else if (peek() == '/') {
            if (pos + 1 < source.length()) {
                if (source[pos + 1] == '/') {
                    while (peek() != '\n' && peek() != '\0') {
                        get();
                    }
                } else if (source[pos + 1] == '*') {
                    get(); get(); // consume /*
                    while (peek() != '\0') {
                        if (peek() == '*' && pos + 1 < source.length() && source[pos + 1] == '/') {
                            get(); get(); // consume */
                            break;
                        }
                        get();
                    }
                } else {
                    break;
                }
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

AssemblerToken AssemblerLexer::nextToken() {
    char c = peek();
    if (c == '\0') {
        return {AssemblerTokenType::END_OF_FILE, "", line, column};
    }

    if (c == '\n') {
        int startLine = line;
        int startCol = column;
        get();
        return {AssemblerTokenType::NEWLINE, "\\n", startLine, startCol};
    }

    if (c == '.') {
        // If followed by a digit, it's a decimal number
        if (pos < source.length() && std::isdigit(source[pos])) {
            return lexNumber();
        }
        int startLine = line;
        int startCol = column;
        get();
        std::string name;
        while (std::isalnum(peek()) || peek() == '_') {
            name += get();
        }
        std::string upper = name;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
        static const std::set<std::string> regs = {"A", "X", "Y", "Z", "SP", "PC", "AX", "AY", "AZ", "XY", "Q", "AXYZ"};
        if (regs.count(upper)) return {AssemblerTokenType::REGISTER, upper, startLine, startCol};
        return {AssemblerTokenType::DIRECTIVE, name, startLine, startCol};
    }

    if (c == 'P' && pos < source.length() && source[pos] == '.') {
        int startLine = line;
        int startCol = column;
        get(); // consume P
        get(); // consume .
        char flag = (char)std::toupper(get());
        return {AssemblerTokenType::FLAG, std::string(1, flag), startLine, startCol};
    }

    if (std::isdigit(c) || c == '$' || c == '%' || c == '.' || 
        (c == '-' && pos + 1 < source.length() && (std::isdigit(source[pos + 1]) || source[pos + 1] == '.'))) {
        if (c == '.' && (pos >= source.length() || !std::isdigit(source[pos]))) {
            // directive handled later
        } else {
            return lexNumber();
        }
    }

    if (std::isalpha(c) || c == '_' || c == '@') {
        return lexIdentifierOrInstruction();
    }

    if (c == '"') {
        return lexString();
    }

    if (c == '\'') {
        return lexChar();
    }

    int startLine = line;
    int startCol = column;
    get();

    switch (c) {
        case ':': return {AssemblerTokenType::COLON, ":", startLine, startCol};
        case ',': return {AssemblerTokenType::COMMA, ",", startLine, startCol};
        case '#': return {AssemblerTokenType::HASH, "#", startLine, startCol};
        case '=':
            if (peek() == '=') { get(); return {AssemblerTokenType::EQUALS_EQUALS, "==", startLine, startCol}; }
            return {AssemblerTokenType::EQUALS, "=", startLine, startCol};
        case '+':
            if (peek() == '+') { get(); return {AssemblerTokenType::INCREMENT, "++", startLine, startCol}; }
            return {AssemblerTokenType::PLUS, "+", startLine, startCol};
        case '-':
            if (peek() == '-') { get(); return {AssemblerTokenType::DECREMENT, "--", startLine, startCol}; }
            return {AssemblerTokenType::MINUS, "-", startLine, startCol};
        case '*': return {AssemblerTokenType::STAR, "*", startLine, startCol};
        case '/': return {AssemblerTokenType::SLASH, "/", startLine, startCol};
        case '<':
            if (peek() == '=') { get(); return {AssemblerTokenType::LESS_EQUAL, "<=", startLine, startCol}; }
            if (peek() == '<') { get(); return {AssemblerTokenType::LSHIFT, "<<", startLine, startCol}; }
            return {AssemblerTokenType::LESS_THAN, "<", startLine, startCol};
        case '>':
            if (peek() == '=') { get(); return {AssemblerTokenType::GREATER_EQUAL, ">=", startLine, startCol}; }
            if (peek() == '>') { get(); return {AssemblerTokenType::RSHIFT, ">>", startLine, startCol}; }
            return {AssemblerTokenType::GREATER_THAN, ">", startLine, startCol};
        case '!':
            if (peek() == '=') { get(); return {AssemblerTokenType::NOT_EQUALS, "!=", startLine, startCol}; }
            return {AssemblerTokenType::BANG, "!", startLine, startCol};
        case '&':
            if (peek() == '&') { get(); return {AssemblerTokenType::AND, "&&", startLine, startCol}; }
            return {AssemblerTokenType::AMPERSAND, "&", startLine, startCol};
        case '|':
            if (peek() == '|') { get(); return {AssemblerTokenType::OR, "||", startLine, startCol}; }
            return {AssemblerTokenType::PIPE, "|", startLine, startCol};
        case '^': return {AssemblerTokenType::CARET, "^", startLine, startCol};
        case '~': return {AssemblerTokenType::TILDE, "~", startLine, startCol};
        case '(': return {AssemblerTokenType::OPEN_PAREN, "(", startLine, startCol};
        case ')': return {AssemblerTokenType::CLOSE_PAREN, ")", startLine, startCol};
        case '[': return {AssemblerTokenType::OPEN_BRACKET, "[", startLine, startCol};
        case ']': return {AssemblerTokenType::CLOSE_BRACKET, "]", startLine, startCol};
        case '{': return {AssemblerTokenType::OPEN_CURLY, "{", startLine, startCol};
        case '}': return {AssemblerTokenType::CLOSE_CURLY, "}", startLine, startCol};
        default: return {AssemblerTokenType::UNKNOWN, std::string(1, c), startLine, startCol};
    }
}

AssemblerToken AssemblerLexer::lexIdentifierOrInstruction() {
    int startLine = line;
    int startCol = column;
    std::string value;
    while (std::isalnum(peek()) || peek() == '_' || peek() == '.' || peek() == '@') {
        value += get();
    }
    if (value.empty()) return nextToken();

    static const std::set<std::string> instructions = {
        "ADC", "AND", "ASL", "ASR", "ASW", "BCC", "BCS", "BEQ", "BIT", "BMI",
        "BNE", "BPL", "BRA", "BRK", "BSR", "BVC", "BVS", "BBR0", "BBR1", "BBR2",
        "BBR3", "BBR4", "BBR5", "BBR6", "BBR7", "BBS0", "BBS1", "BBS2", "BBS3",
        "BBS4", "BBS5", "BBS6", "BBS7", "CLC", "CLD", "CLE", "CLI", "CLV", "CMP",
        "CPX", "CPY", "CPZ", "DEC", "DEW", "DEX", "DEY", "DEZ", "EOM", "EOR",
        "INC", "INW", "INX", "INY", "INZ", "JMP", "JSR", "LDA", "LDQ", "LDX",
        "LDY", "LDZ", "LSR", "MAP", "NEG", "NOP", "ORA", "PHA", "PHP", "PHX",
        "PHY", "PHZ", "PHW", "PLA", "PLP", "PLX", "PLY", "PLZ", "RMB0", "RMB1",
        "RMB2", "RMB3", "RMB4", "RMB5", "RMB6", "RMB7", "ROL", "ROR", "ROW",
        "RTI", "RTS", "SBC", "SEC", "SED", "SEE", "SEI", "SMB0", "SMB1", "SMB2",
        "SMB3", "SMB4", "SMB5", "SMB6", "SMB7", "STA", "STQ", "STX", "STY",
        "STZ", "TAB", "TAY", "TAZ", "TBA", "TAX", "TAY", "TSX", "TSY", "TXA",
        "TXS", "TYA", "TYS", "TZA", "TRB", "TSB",
        "ADCQ", "SBCQ", "ANDQ", "ORAQ", "EORQ", "CMPQ", "INCQ", "DECQ",
        "ASLQ", "LSRQ", "ROLQ", "RORQ", "NEGQ",
        "CALL", "PROC", "ENDPROC", "RTN", "EXPR", "MUL", "MUL.8", "MUL.16", "MUL.24", "MUL.32", "DIV", "DIV.8", "DIV.16", "DIV.24", "DIV.32",
        "MUL.S16", "MUL.S32", "DIV.S16", "DIV.S32", "MOD.16", "MOD.S16", "MOD.32", "MOD.S32",
        "LDAX", "LDAY", "LDAZ", "STAX", "STAY", "STAZ",
        "ADD.16", "SUB.16", "AND.16", "ORA.16", "EOR.16", "NEG.16", "NOT.16", "CMP.16", "ABS.16", "CPW", "LDW", "STW", "SWAP", "ZERO",
        "ADD.S16", "SUB.S16", "CMP.S16", "NEG.S16", "ABS.S16", "ASR.S16", "LSL.S16", "LSR.S16", "ROL.S16", "ROR.S16", "SXT.8",
        "LSL.16", "LSR.16", "ASR.16", "ROL.16", "ROR.16",
        "ADD.32", "SUB.32", "AND.32", "ORA.32", "EOR.32", "NEG.32", "NOT.32", "CMP.32", "ABS.32",
        "ADD.S32", "SUB.S32", "CMP.S32", "NEG.S32", "ABS.S32", "ASR.S32", "LSL.S32", "LSR.S32", "ROL.S32", "ROR.S32", "SXT.16",
        "LSL.32", "LSR.32", "ASR.32", "ROL.32", "ROR.32",
        "STA.SP", "LDA.SP", "STW.SP", "LDW.SP", "PHW.SP", "FILL", "FILL.SP", "MOVE", "MOVE.SP",
        "CHKZERO.8", "CHKZERO.16", "CHKNONZERO.8", "CHKNONZERO.16", "BRANCH.16", "SELECT",
        "PTRSTACK", "PTRDEREF", "LDW.F", "STW.F", "INC.F", "DEC.F",
        "PUSH", "POP",
        "LDXY", "STXY",
        "LDA.FP", "STA.FP", "LDAX.FP", "STAX.FP", "LDAY.FP", "STAY.FP", "LDAZ.FP", "STAZ.FP", "LDAXYZ.FP", "STAXYZ.FP", "LEAX.FP", "MOVE.FP", "INC.FP", "DEC.FP", "INC.16F", "DEC.16F",
        "BFEXT", "BFEXT16", "BFEXT32", "BFINS", "BFINS.SP", "BFINS.IND", "BFINS16", "BFINS16.SP", "BFINS16.IND", "BFINS32",
        "STRUCT_ELEM", "STRUCT_ELEM.16", "ADDR_ELEM", "ADDR_ELEM.16"
    };

    std::string upperValue = value;
    std::transform(upperValue.begin(), upperValue.end(), upperValue.begin(), ::toupper);

    if (instructions.count(upperValue)) {
        return {AssemblerTokenType::INSTRUCTION, upperValue, startLine, startCol};
    }

    // Encoding prefixes for char/string literals: s'A' p'A' s"ABC" p"ABC"
    if ((value == "s" || value == "S" || value == "p" || value == "P") &&
        (peek() == '\'' || peek() == '"')) {
        char prefix = (value[0] == 's' || value[0] == 'S') ? 's' : 'p';
        if (peek() == '\'') {
            auto tok = lexChar();
            tok.value = std::string(1, prefix) + tok.value; // prepend prefix
            return tok;
        } else {
            auto tok = lexString();
            tok.value = std::string(1, prefix) + tok.value; // prepend prefix
            return tok;
        }
    }

    return {AssemblerTokenType::IDENTIFIER, value, startLine, startCol};
}

AssemblerToken AssemblerLexer::lexNumber() {
    int startLine = line;
    int startCol = column;
    std::string value;
    char prefix = get(); 
    value += prefix;
    
    if (prefix == '$') {
        while (std::isxdigit(peek())) value += get();
        return {AssemblerTokenType::HEX_LITERAL, value, startLine, startCol};
    }
    
    if (prefix == '%') {
        while (peek() == '0' || peek() == '1') value += get();
        return {AssemblerTokenType::BINARY_LITERAL, value, startLine, startCol};
    }

    bool hasDot = (prefix == '.');
    while (std::isdigit(peek()) || (!hasDot && peek() == '.')) {
        if (peek() == '.') hasDot = true;
        value += get();
    }
    return {AssemblerTokenType::DECIMAL_LITERAL, value, startLine, startCol};
}

AssemblerToken AssemblerLexer::lexString() {
    int startLine = line;
    int startCol = column;
    get(); // skip "
    std::string value;
    while (peek() != '"' && peek() != '\0') {
        value += get();
    }
    if (peek() == '"') get();
    return {AssemblerTokenType::STRING_LITERAL, value, startLine, startCol};
}

AssemblerToken AssemblerLexer::lexChar() {
    int startLine = line;
    int startCol = column;
    get(); // skip '
    std::string value;
    if (peek() != '\0' && peek() != '\'') {
        value += get();
    }
    if (peek() == '\'') get();
    return {AssemblerTokenType::CHAR_LITERAL, value, startLine, startCol};
}
