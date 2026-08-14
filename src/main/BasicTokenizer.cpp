#include "BasicTokenizer.hpp"
#include <cctype>
#include <algorithm>

BasicTokenizer::BasicTokenizer() {
    initializeKeywords();
}

void BasicTokenizer::initializeKeywords() {
    // MEGA65 BASIC 65 single-byte tokens (0x80-0xFD)
    keywords = {
        {"END", 0x80},
        {"FOR", 0x81},
        {"NEXT", 0x82},
        {"DATA", 0x83},
        {"INPUT#", 0x84},
        {"INPUT", 0x85},
        {"DIM", 0x86},
        {"READ", 0x87},
        {"LET", 0x88},
        {"GOTO", 0x89},
        {"RUN", 0x8A},
        {"IF", 0x8B},
        {"RESTORE", 0x8C},
        {"GOSUB", 0x8D},
        {"RETURN", 0x8E},
        {"REM", 0x8F},
        {"STOP", 0x90},
        {"ON", 0x91},
        {"WAIT", 0x92},
        {"LOAD", 0x93},
        {"SAVE", 0x94},
        {"VERIFY", 0x95},
        {"DEF", 0x96},
        {"POKE", 0x97},
        {"PRINT#", 0x98},
        {"PRINT", 0x99},
        {"CONT", 0x9A},
        {"LIST", 0x9B},
        {"CLR", 0x9C},
        {"CMD", 0x9D},
        {"SYS", 0x9E},
        {"OPEN", 0x9F},
        {"CLOSE", 0xA0},
        {"GET", 0xA1},
        {"NEW", 0xA2},
        {"TO", 0xA4},
        {"FN", 0xA5},
        {"THEN", 0xA7},
        {"NOT", 0xA8},
        {"STEP", 0xA9},
        {"AND", 0xAF},
        {"OR", 0xB0},
        {"SGN", 0xB4},
        {"INT", 0xB5},
        {"ABS", 0xB6},
        {"USR", 0xB7},
        {"FRE", 0xB8},
        {"POS", 0xB9},
        {"SQR", 0xBA},
        {"RND", 0xBB},
        {"LOG", 0xBC},
        {"EXP", 0xBD},
        {"COS", 0xBE},
        {"SIN", 0xBF},
        {"TAN", 0xC0},
        {"ATN", 0xC1},
        {"PEEK", 0xC2},
        {"LEN", 0xC3},
        {"STR$", 0xC4},
        {"VAL", 0xC5},
        {"ASC", 0xC6},
        {"CHR$", 0xC7},
        {"LEFT$", 0xC8},
        {"RIGHT$", 0xC9},
        {"MID$", 0xCA},
        {"GO", 0xCB},
        {"RGRAPHIC", 0xCC},
        {"RCOLOR", 0xCD},
        {"JOY", 0xCF},
        {"RPEN", 0xD0},
        {"DEC", 0xD1},
        {"HEX$", 0xD2},
        {"ERR$", 0xD3},
        {"INSTR", 0xD4},
        {"ELSE", 0xD5},
        {"RESUME", 0xD6},
        {"TRAP", 0xD7},
        {"TRON", 0xD8},
        {"TROFF", 0xD9},
        {"SOUND", 0xDA},
        {"VOL", 0xDB},
        {"AUTO", 0xDC},
        {"IMPORT", 0xDD},
        {"GRAPHIC", 0xDE},
        {"PAINT", 0xDF},
        {"CHAR", 0xE0},
        {"BOX", 0xE1},
        {"CIRCLE", 0xE2},
        {"PASTE", 0xE3},
        {"CUT", 0xE4},
        {"LINE", 0xE5},
        {"MERGE", 0xE6},
        {"COLOR", 0xE7},
        {"SCNCLR", 0xE8},
        {"XOR", 0xE9},
        {"HELP", 0xEA},
        {"DO", 0xEB},
        {"LOOP", 0xEC},
        {"EXIT", 0xED},
        {"DIR", 0xEE},
        {"DSAVE", 0xEF},
        {"DLOAD", 0xF0},
        {"HEADER", 0xF1},
        {"SCRATCH", 0xF2},
        {"COLLECT", 0xF3},
        {"COPY", 0xF4},
        {"RENAME", 0xF5},
        {"BACKUP", 0xF6},
        {"DELETE", 0xF7},
        {"RENUMBER", 0xF8},
        {"KEY", 0xF9},
        {"MONITOR", 0xFA},
        {"USING", 0xFB},
        {"UNTIL", 0xFC},
        {"WHILE", 0xFD},
    };

    // MEGA65 BASIC 65 two-byte tokens (0xFE + escape byte)
    escapeKeywords = {
        {"BANK", 0x02},
        {"FILTER", 0x03},
        {"PLAY", 0x04},
        {"TEMPO", 0x05},
        {"MOVSPR", 0x06},
        {"SPRITE", 0x07},
        {"SPRCOLOR", 0x08},
        {"RREG", 0x09},
        {"ENVELOPE", 0x0A},
        {"SLEEP", 0x0B},
        {"CATALOG", 0x0C},
        {"DOPEN", 0x0D},
        {"APPEND", 0x0E},
        {"DCLOSE", 0x0F},
        {"BSAVE", 0x10},
        {"BLOAD", 0x11},
        {"RECORD", 0x12},
        {"CONCAT", 0x13},
        {"DVERIFY", 0x14},
        {"DCLEAR", 0x15},
        {"SPRSAV", 0x16},
        {"COLLISION", 0x17},
        {"BEGIN", 0x18},
        {"BEND", 0x19},
        {"WINDOW", 0x1A},
        {"BOOT", 0x1B},
        {"FREAD#", 0x1C},
        {"WPOKE", 0x1D},
        {"FWRITE#", 0x1E},
        {"DMA", 0x1F},
        {"EDMA", 0x21},
        {"MEM", 0x23},
        {"OFF", 0x24},
        {"FAST", 0x25},
        {"SPEED", 0x26},
        {"TYPE", 0x27},
        {"BVERIFY", 0x28},
        {"ECTORY", 0x29},
        {"ERASE", 0x2A},
        {"FIND", 0x2B},
        {"CHANGE", 0x2C},
        {"SET", 0x2D},
        {"SCREEN", 0x2E},
        {"POLYGON", 0x2F},
        {"ELLIPSE", 0x30},
        {"VIEWPORT", 0x31},
        {"GCOPY", 0x32},
        {"PEN", 0x33},
        {"PALETTE", 0x34},
        {"DMODE", 0x35},
        {"DPAT", 0x36},
        {"FORMAT", 0x37},
        {"TURBO", 0x38},
        {"FOREGROUND", 0x39},
        {"BACKGROUND", 0x3B},
        {"BORDER", 0x3C},
        {"HIGHLIGHT", 0x3D},
        {"MOUSE", 0x3E},
        {"RMOUSE", 0x3F},
        {"DISK", 0x40},
        {"CURSOR", 0x41},
        {"RCURSOR", 0x42},
        {"LOADIFF", 0x43},
        {"SAVEIFF", 0x44},
        {"EDIT", 0x45},
        {"FONT", 0x46},
        {"FGOTO", 0x47},
        {"FGOSUB", 0x48},
        {"MOUNT", 0x49},
        {"FREEZER", 0x4A},
        {"CHDIR", 0x4B},
        {"DOT", 0x4C},
        {"INFO", 0x4D},
    };
}

std::vector<BasicToken> BasicTokenizer::tokenize(const std::string& source) {
    std::vector<BasicToken> tokens;
    size_t pos = 0;
    bool lineStart = true;
    bool hasNonWhitespace = false;

    while (pos < source.length()) {
        char c = source[pos];

        // Skip # comments (entire line)
        if (lineStart && c == '#') {
            while (pos < source.length() && source[pos] != '\n') {
                pos++;
            }
            if (pos < source.length() && source[pos] == '\n') {
                pos++;
            }
            lineStart = true;
            hasNonWhitespace = false;
            continue;
        }

        if (std::isspace(c)) {
            if (c == '\n') {
                // Only emit EOL if line had non-whitespace content
                if (hasNonWhitespace) {
                    BasicToken eol;
                    eol.type = BasicToken::EOL;
                    tokens.push_back(eol);
                }
                lineStart = true;
                hasNonWhitespace = false;
            }
            pos++;
        } else if (c == '"') {
            hasNonWhitespace = true;
            lineStart = false;
            tokens.push_back(parseString(source, pos));
        } else if (std::isdigit(c)) {
            hasNonWhitespace = true;
            lineStart = false;
            tokens.push_back(parseNumber(source, pos));
        } else if (std::isalpha(c) || c == '_') {
            hasNonWhitespace = true;
            lineStart = false;
            BasicToken token = parseIdentifier(source, pos);

            // Check for label (identifier followed by :)
            if (pos < source.length() && source[pos] == ':') {
                token.type = BasicToken::LABEL;
                pos++;  // skip the :
            }

            tokens.push_back(token);
        } else if (c == ':') {
            hasNonWhitespace = true;
            lineStart = false;
            BasicToken sep;
            sep.type = BasicToken::OPERATOR;
            sep.value = ":";
            tokens.push_back(sep);
            pos++;
        } else {
            hasNonWhitespace = true;
            lineStart = false;
            BasicToken op;
            op.type = BasicToken::OPERATOR;
            op.value = c;
            tokens.push_back(op);
            pos++;
        }
    }

    // Emit final EOL if needed
    if (hasNonWhitespace) {
        BasicToken eol;
        eol.type = BasicToken::EOL;
        tokens.push_back(eol);
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

    while (pos < source.length() && (std::isalnum(source[pos]) || source[pos] == '_' || source[pos] == '$' || source[pos] == '#')) {
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
    } else if (isEscapeKeyword(upper)) {
        token.type = BasicToken::KEYWORD;
        token.value = upper;
        token.tokenByte = 0xFE;
        token.escapeByte = getEscapeKeywordByte(upper);
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

bool BasicTokenizer::isEscapeKeyword(const std::string& word) const {
    return escapeKeywords.find(word) != escapeKeywords.end();
}

uint8_t BasicTokenizer::getEscapeKeywordByte(const std::string& keyword) const {
    auto it = escapeKeywords.find(keyword);
    if (it != escapeKeywords.end()) {
        return it->second;
    }
    return 0;
}

const std::unordered_map<std::string, uint8_t>& BasicTokenizer::getKeywords() const {
    return keywords;
}

const std::unordered_map<std::string, uint8_t>& BasicTokenizer::getEscapeKeywords() const {
    return escapeKeywords;
}
