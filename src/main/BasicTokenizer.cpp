#include "BasicTokenizer.hpp"
#include <cctype>
#include <algorithm>

BasicTokenizer::BasicTokenizer() {
    initializeKeywords();
}

void BasicTokenizer::initializeKeywords() {
    keywords = {
        {"END", 0x80},
        {"FOR", 0x81},
        {"NEXT", 0x82},
        {"DATA", 0x83},
        {"INPUT", 0x84},
        {"DIM", 0x85},
        {"READ", 0x86},
        {"LET", 0x87},
        {"GOTO", 0x88},
        {"RUN", 0x89},
        {"IF", 0x8A},
        {"RESTORE", 0x8B},
        {"GOSUB", 0x8C},
        {"RETURN", 0x8D},
        {"REM", 0x8E},
        {"STOP", 0x8F},
        {"ON", 0x90},
        {"WAIT", 0x91},
        {"LOAD", 0x92},
        {"SAVE", 0x93},
        {"VERIFY", 0x94},
        {"DEF", 0x95},
        {"POKE", 0x96},
        {"PRINT", 0x99},
        {"CONT", 0x9A},
        {"LIST", 0x9B},
        {"CLR", 0x9C},
        {"CMD", 0x9D},
        {"SYS", 0x9E},
        {"OPEN", 0x9F},
        {"CLOSE", 0xA0},
        {"NEW", 0xA1},
        {"APPEND", 0xA2},
        {"DLOAD", 0xA3},
        {"GET", 0xA4},
        {"GETKEY", 0xA5},
        {"BANK", 0xA6},
        {"BSAVE", 0xA7},
        {"BLOAD", 0xA8},
        {"RECORD", 0xA9},
        {"CONCAT", 0xAA},
        {"DVERIFY", 0xAB},
        {"DCLEAR", 0xAC},
        {"PALETTE", 0xAD},
        {"BVERIFY", 0xAE},
        {"DPATCH", 0xAF},
        {"FETCH", 0xB0},
        {"FIRST", 0xB1},
        {"CARTRIDGE", 0xB2},
        {"ROLLOVER", 0xB3},
        {"RESET", 0xB4},
        {"FLUSHBUFFER", 0xB5},
        {"KEYBOARDCODING", 0xB6},
        {"ELSE", 0xB7},
        {"QUIT", 0xB8},
        {"MID", 0xC6},
        {"LEN", 0xC7},
        {"STR", 0xC8},
        {"VAL", 0xC9},
        {"ASC", 0xCA},
        {"CHR", 0xCB},
        {"PEEK", 0xCC},
        {"ABS", 0xCD},
        {"INT", 0xCE},
        {"SQR", 0xCF},
        {"RND", 0xD0},
        {"SIN", 0xD1},
        {"COS", 0xD2},
        {"TAN", 0xD3},
        {"ATN", 0xD4},
        {"EXP", 0xD5},
        {"LOG", 0xD6},
        {"SGN", 0xD7},
        {"TAB", 0xD8},
        {"THEN", 0xD9},
        {"TO", 0xDA},
        {"AND", 0xDB},
        {"OR", 0xDC},
        {"NOT", 0xDD},
        {"STEP", 0xDE},
        {"UNTIL", 0xDF},
        {"WHILE", 0xE0},
        {"REPEAT", 0xE1},
        {"DO", 0xE2},
        {"LOOP", 0xE3},
        {"EXIT", 0xE4},
        {"CALL", 0xE5},
        {"TYPE", 0xE6},
        {"MOD", 0xE7},
        {"EXOR", 0xE8},
        {"BAND", 0xE9},
        {"BOR", 0xEA},
        {"LSHIFT", 0xEB},
        {"RSHIFT", 0xEC},
        {"FRACT", 0xED},
        {"PDUMP", 0xEE},
        {"PLOAD", 0xEF},
        {"PSAVE", 0xF0},
        {"PTEST", 0xF1},
        {"MULTI", 0xF2},
        {"MONITOR", 0xF3},
        {"UNTIL2", 0xF4},
        {"HEX", 0xF5},
        {"DOKE", 0xF6},
        {"DPEEK", 0xF7},
        {"FORMAT", 0xF8},
        {"CIRCLE", 0xF9},
        {"RECT", 0xFA},
        {"SPRDEF", 0xFB},
    };
}

std::vector<BasicToken> BasicTokenizer::tokenize(const std::string& source) {
    std::vector<BasicToken> tokens;
    size_t pos = 0;

    while (pos < source.length()) {
        char c = source[pos];

        if (std::isspace(c)) {
            if (c == '\n') {
                BasicToken eol;
                eol.type = BasicToken::EOL;
                tokens.push_back(eol);
            }
            pos++;
        } else if (c == '"') {
            tokens.push_back(parseString(source, pos));
        } else if (std::isdigit(c)) {
            tokens.push_back(parseNumber(source, pos));
        } else if (std::isalpha(c) || c == '_') {
            BasicToken token = parseIdentifier(source, pos);
            tokens.push_back(token);
        } else if (c == ':') {
            BasicToken sep;
            sep.type = BasicToken::OPERATOR;
            sep.value = ":";
            tokens.push_back(sep);
            pos++;
        } else {
            BasicToken op;
            op.type = BasicToken::OPERATOR;
            op.value = c;
            tokens.push_back(op);
            pos++;
        }
    }

    BasicToken eof;
    eof.type = BasicToken::END_OF_FILE;
    tokens.push_back(eof);

    return tokens;
}

BasicToken BasicTokenizer::parseString(const std::string& source, size_t& pos) {
    BasicToken token;
    token.type = BasicToken::STRING;

    pos++;
    while (pos < source.length() && source[pos] != '"') {
        token.value += source[pos];
        pos++;
    }

    if (pos < source.length() && source[pos] == '"') {
        pos++;
    }

    return token;
}

BasicToken BasicTokenizer::parseNumber(const std::string& source, size_t& pos) {
    BasicToken token;
    token.type = BasicToken::NUMBER;

    while (pos < source.length() && (std::isdigit(source[pos]) || source[pos] == '.')) {
        token.value += source[pos];
        pos++;
    }

    return token;
}

BasicToken BasicTokenizer::parseIdentifier(const std::string& source, size_t& pos) {
    std::string word;

    while (pos < source.length() && (std::isalnum(source[pos]) || source[pos] == '_')) {
        word += source[pos];
        pos++;
    }

    std::string upper = word;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    BasicToken token;
    if (isKeyword(upper)) {
        token.type = BasicToken::KEYWORD;
        token.value = upper;
        token.tokenByte = getKeywordToken(upper);
    } else {
        token.type = BasicToken::IDENTIFIER;
        token.value = word;
    }

    return token;
}

uint8_t BasicTokenizer::getKeywordToken(const std::string& keyword) const {
    auto it = keywords.find(keyword);
    if (it != keywords.end()) {
        return it->second;
    }
    return 0;
}

bool BasicTokenizer::isKeyword(const std::string& word) const {
    return keywords.find(word) != keywords.end();
}
