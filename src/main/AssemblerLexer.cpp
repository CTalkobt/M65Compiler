#include <cstring>
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
        token.file = curFile_;
        // the line's directive (.cpu 45gs02: a name, not a number, for checkNumberEnd)
        if (token.type == AssemblerTokenType::NEWLINE) lineDirective_.clear();
        else if (token.type == AssemblerTokenType::DIRECTIVE && lineDirective_.empty()) {
            lineDirective_ = token.value;
            if (!lineDirective_.empty() && lineDirective_[0] == '.') lineDirective_.erase(0, 1);
            for (auto& ch : lineDirective_) ch = (char)std::tolower((unsigned char)ch);
        }
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

    // #line N "file" from the preprocessor (#include): the next line is
    // line N; the marker itself is skipped
    if (c == '#' && (pos == 0 || source[pos - 1] == '\n') && source.compare(pos, 6, "#line ") == 0) {
        size_t p = pos + 6;
        int n = 0;
        while (p < source.length() && std::isdigit((unsigned char)source[p])) n = n * 10 + (source[p++] - '0');
        // (#line N "file": the lines that follow are that file's)
        while (p < source.length() && (source[p] == ' ' || source[p] == '\t')) p++;
        if (p < source.length() && source[p] == '"') {
            size_t e = source.find('"', p + 1);
            size_t nl = source.find('\n', p);
            if (e != std::string::npos && (nl == std::string::npos || e < nl)) {
                std::string f = source.substr(p + 1, e - p - 1);
                auto it = std::find(files_.begin(), files_.end(), f);
                curFile_ = (int)(it - files_.begin());
                if (it == files_.end()) files_.push_back(f);
            }
        }
        while (peek() != '\n' && peek() != '\0') get();
        line = n - 1;   // the NEWLINE that follows makes it n
        return nextToken();
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
        static const std::set<std::string> regs = {"A", "X", "Y", "Z", "SP", "PC", "AX", "AY", "AZ", "XY", "Q", "AXY", "AXZ", "AYZ", "AXYZ"};
        if (regs.count(upper)) return {AssemblerTokenType::REGISTER, upper, startLine, startCol};
        return {AssemblerTokenType::DIRECTIVE, name, startLine, startCol};
    }

    // P.C P.Z P.I P.D P.V P.N: a processor flag (expr / FlagNode)
    if (c == 'P' && pos + 2 < source.length() && source[pos + 1] == '.' &&
        std::strchr("CZIDVNczidvn", source[pos + 2]) &&
        !(pos + 3 < source.length() && (std::isalnum((unsigned char)source[pos + 3]) || source[pos + 3] == '_'))) {
        int startLine = line;
        int startCol = column;
        get(); // consume P
        get(); // consume .
        char flag = (char)std::toupper(get());
        return {AssemblerTokenType::FLAG, std::string(1, flag), startLine, startCol};
    }

    // ('-' is always the MINUS operator: "5-1" is a subtraction, "-1" a unary
    //  minus; lexing "-1" as a number turned "5-1" into the two numbers 5 and -1)
    if (std::isdigit(c) || c == '$' || c == '%' || c == '.') {
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
        "LDA.PARAM", "STA.PARAM", "LDX.PARAM", "STX.PARAM", "LDY.PARAM", "STY.PARAM", "LDZ.PARAM", "STZ.PARAM",
        "LDAX.PARAM", "STAX.PARAM", "LDAY.PARAM", "STAY.PARAM", "LDAZ.PARAM", "STAZ.PARAM", "LDAXYZ.PARAM", "STAXYZ.PARAM", "LEAX.PARAM",
        "LDA.LOCAL", "STA.LOCAL", "LDX.LOCAL", "STX.LOCAL", "LDY.LOCAL", "STY.LOCAL", "LDZ.LOCAL", "STZ.LOCAL",
        "LDAX.LOCAL", "STAX.LOCAL", "LDAY.LOCAL", "STAY.LOCAL", "LDAZ.LOCAL", "STAZ.LOCAL", "LDAXYZ.LOCAL", "STAXYZ.LOCAL", "LEAX.LOCAL",
        "LDA.FP", "STA.FP", "LDAX.FP", "STAX.FP", "LDAY.FP", "STAY.FP", "LDAZ.FP", "STAZ.FP", "LDAXYZ.FP", "STAXYZ.FP", "MOVE.FP",
        "INC.16F", "DEC.16F",
        "BFEXT", "BFEXT16", "BFEXT32", "BFINS", "BFINS.SP", "BFINS.IND", "BFINS16", "BFINS16.SP", "BFINS16.IND", "BFINS32", "BFINS32.SP", "BFINS32.IND",
        "STRUCT_ELEM", "STRUCT_ELEM.16", "ADDR_ELEM", "ADDR_ELEM.16"
    };

    std::string upperValue = value;
    std::transform(upperValue.begin(), upperValue.end(), upperValue.begin(), ::toupper);
    // the 45GS02's names for the 32-bit increment / decrement (incq, decq)
    if (upperValue == "INQ") upperValue = "INCQ";
    else if (upperValue == "DEQ") upperValue = "DECQ";

    if (instructions.count(upperValue)) {
        return {AssemblerTokenType::INSTRUCTION, upperValue, startLine, startCol};
    }
    // An addressing-mode suffix on a native mnemonic: lda.abs $12 (forces
    // absolute), lda.zp, .imm, .ind, .xind, .indy, .indz, .accum/.a
    {
        size_t dot = upperValue.rfind('.');
        static const std::set<std::string> suffixes = {"IMM", "ZP", "ABS", "IND", "XIND", "INDY", "INDZ", "ACCUM", "A"};
        if (dot != std::string::npos && dot > 0 && suffixes.count(upperValue.substr(dot + 1)) &&
            upperValue.find('.') == dot && instructions.count(upperValue.substr(0, dot)))
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
            // Marked with \x01 so an unprefixed string starting with s/p isn't mistaken for one
            tok.value = std::string(1, asmstr::PREFIX) + prefix + tok.value;
            return tok;
        }
    }

    return {AssemblerTokenType::IDENTIFIER, value, startLine, startCol};
}

// A number runs into letters, digits or '_' ($12junk, 10xyz, %1010_0101,
// $1_2): the rest is consumed and recorded, so ca45 reports it instead of
// reading the number alone (#363)
AssemblerToken AssemblerLexer::checkNumberEnd(AssemblerToken tok) {
    std::string rest;
    while (std::isalnum((unsigned char)peek()) || peek() == '_') rest += get();
    if (!rest.empty() && lineDirective_ != "cpu") {
        AssemblerToken bad = tok;
        bad.file = curFile_;
        bad.value = tok.value + rest;
        badNumbers_.push_back({bad, rest});
    }
    return tok;
}

AssemblerToken AssemblerLexer::lexNumber() {
    int startLine = line;
    int startCol = column;
    std::string value;
    char prefix = get(); 
    value += prefix;
    
    if (prefix == '$') {
        while (std::isxdigit(peek())) value += get();
        return checkNumberEnd({AssemblerTokenType::HEX_LITERAL, value, startLine, startCol});
    }
    
    if (prefix == '%') {
        while (peek() == '0' || peek() == '1') value += get();
        return checkNumberEnd({AssemblerTokenType::BINARY_LITERAL, value, startLine, startCol});
    }

    // 0x1F / 0b101: C-style hex and binary, as $1F / %101
    if (prefix == '0' && (peek() == 'x' || peek() == 'X') && pos + 1 < source.length() &&
        std::isxdigit((unsigned char)source[pos + 1])) {
        get();
        value = "$";
        while (std::isxdigit(peek())) value += get();
        return checkNumberEnd({AssemblerTokenType::HEX_LITERAL, value, startLine, startCol});
    }
    if (prefix == '0' && (peek() == 'b' || peek() == 'B') && pos + 1 < source.length() &&
        (source[pos + 1] == '0' || source[pos + 1] == '1')) {
        get();
        value = "%";
        while (peek() == '0' || peek() == '1') value += get();
        return checkNumberEnd({AssemblerTokenType::BINARY_LITERAL, value, startLine, startCol});
    }

    bool hasDot = (prefix == '.');
    while (std::isdigit(peek()) || (!hasDot && peek() == '.')) {
        if (peek() == '.') hasDot = true;
        value += get();
    }
    // a float's exponent (.float 1.0e20, 2.5E-3)
    if (hasDot && (peek() == 'e' || peek() == 'E')) {
        size_t k = pos + 1;
        if (k < source.length() && (source[k] == '+' || source[k] == '-')) k++;
        if (k < source.length() && std::isdigit((unsigned char)source[k])) {
            value += get();
            if (peek() == '+' || peek() == '-') value += get();
            while (std::isdigit(peek())) value += get();
        }
    }
    return checkNumberEnd({AssemblerTokenType::DECIMAL_LITERAL, value, startLine, startCol});
}

AssemblerToken AssemblerLexer::lexString() {
    int startLine = line;
    int startCol = column;
    get(); // skip "
    std::string value;
    while (peek() != '"' && peek() != '\0' && peek() != '\n') {
        char c = get();
        if (c != '\\' || peek() == '\0') { value += c; continue; }
        char e = get();
        int b;
        switch (e) {
            case 'n': value += asmstr::NEWLINE; continue;
            case 'r': value += asmstr::RETURN; continue;
            case 't': b = 0x09; break;
            case 'a': b = 0x07; break;
            case 'b': b = 0x08; break;
            case 'f': b = 0x0C; break;
            case 'v': b = 0x0B; break;
            case 'x': {
                b = 0; int digits = 0;
                while (std::isxdigit((unsigned char)peek())) {
                    char h = get();
                    b = b * 16 + (std::isdigit((unsigned char)h) ? h - '0' : (std::tolower(h) - 'a' + 10));
                    digits++;
                }
                if (!digits) b = 'x';
                break;
            }
            default:
                if (e >= '0' && e <= '7') {
                    b = e - '0';
                    for (int k = 0; k < 2 && peek() >= '0' && peek() <= '7'; k++) b = b * 8 + (get() - '0');
                } else {
                    b = (unsigned char)e;  // \" \\ \' and unknown escapes: the character itself
                }
        }
        value += asmstr::EXACT;
        value += (char)(unsigned char)b;
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
