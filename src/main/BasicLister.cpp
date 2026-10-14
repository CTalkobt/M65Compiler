#include "BasicLister.hpp"
#include "BasicTokenizer.hpp"
#include <cstdio>
#include <map>
#include <set>

namespace {

// BASIC V2 tokens $80-$CB (and $FF = pi)
const char* const V2[] = {
    "END", "FOR", "NEXT", "DATA", "INPUT#", "INPUT", "DIM", "READ", "LET", "GOTO", "RUN", "IF",
    "RESTORE", "GOSUB", "RETURN", "REM", "STOP", "ON", "WAIT", "LOAD", "SAVE", "VERIFY", "DEF",
    "POKE", "PRINT#", "PRINT", "CONT", "LIST", "CLR", "CMD", "SYS", "OPEN", "CLOSE", "GET", "NEW",
    "TAB(", "TO", "FN", "SPC(", "THEN", "NOT", "STEP", "+", "-", "*", "/", "^", "AND", "OR", ">",
    "=", "<", "SGN", "INT", "ABS", "USR", "FRE", "POS", "SQR", "RND", "LOG", "EXP", "COS", "SIN",
    "TAN", "ATN", "PEEK", "LEN", "STR$", "VAL", "ASC", "CHR$", "LEFT$", "RIGHT$", "MID$", "GO"};

// BASIC V7 tokens $CC-$FD ($CE and $FE prefix two-byte tokens)
const char* const V7[] = {
    "RGR", "RCLR", nullptr, "JOY", "RDOT", "DEC", "HEX$", "ERR$", "INSTR", "ELSE", "RESUME", "TRAP",
    "TRON", "TROFF", "SOUND", "VOL", "AUTO", "PUDEF", "GRAPHIC", "PAINT", "CHAR", "BOX", "CIRCLE",
    "GSHAPE", "SSHAPE", "DRAW", "LOCATE", "COLOR", "SCNCLR", "SCALE", "HELP", "DO", "LOOP", "EXIT",
    "DIRECTORY", "DSAVE", "DLOAD", "HEADER", "SCRATCH", "COLLECT", "COPY", "RENAME", "BACKUP",
    "DELETE", "RENUMBER", "KEY", "MONITOR", "USING", "UNTIL", "WHILE"};
const std::map<uint8_t, const char*> V7CE = {
    {0x02, "POT"}, {0x03, "BUMP"}, {0x04, "PEN"}, {0x05, "RSPPOS"}, {0x06, "RSPRITE"},
    {0x07, "RSPCOLOR"}, {0x08, "XOR"}, {0x09, "RWINDOW"}, {0x0A, "POINTER"}};
const std::map<uint8_t, const char*> V7FE = {
    {0x02, "BANK"}, {0x03, "FILTER"}, {0x04, "PLAY"}, {0x05, "TEMPO"}, {0x06, "MOVSPR"},
    {0x07, "SPRITE"}, {0x08, "SPRCOLOR"}, {0x09, "RREG"}, {0x0A, "ENVELOPE"}, {0x0B, "SLEEP"},
    {0x0C, "CATALOG"}, {0x0D, "DOPEN"}, {0x0E, "APPEND"}, {0x0F, "DCLOSE"}, {0x10, "BSAVE"},
    {0x11, "BLOAD"}, {0x12, "RECORD"}, {0x13, "CONCAT"}, {0x14, "DVERIFY"}, {0x15, "DCLEAR"},
    {0x16, "SPRSAV"}, {0x17, "COLLISION"}, {0x18, "BEGIN"}, {0x19, "BEND"}, {0x1A, "WINDOW"},
    {0x1B, "BOOT"}, {0x1C, "WIDTH"}, {0x1D, "SPRDEF"}, {0x1E, "QUIT"}, {0x1F, "STASH"},
    {0x21, "FETCH"}, {0x23, "SWAP"}, {0x24, "OFF"}, {0x25, "FAST"}, {0x26, "SLOW"}};

// Control codes in strings: their {name} escapes
const std::map<uint8_t, const char*> CONTROL = {
    {0x90, "black"}, {0x05, "white"}, {0x1C, "red"}, {0x9F, "cyan"}, {0x9C, "purple"},
    {0x1E, "green"}, {0x1F, "blue"}, {0x9E, "yellow"}, {0x81, "orange"}, {0x95, "brown"},
    {0x96, "light-red"}, {0x97, "dark-gray"}, {0x98, "gray"}, {0x99, "light-green"},
    {0x9A, "light-blue"}, {0x9B, "light-gray"}, {0x13, "home"}, {0x93, "clear"},
    {0x12, "reverse"}, {0x92, "off"}, {0x14, "del"}, {0x94, "insert"}, {0x91, "up"},
    {0x11, "down"}, {0x9D, "left"}, {0x1D, "right"}, {0x85, "f1"}, {0x86, "f3"}, {0x87, "f5"},
    {0x88, "f7"}, {0x89, "f2"}, {0x8A, "f4"}, {0x8B, "f6"}, {0x8C, "f8"}, {0x0D, "return"},
    {0x0E, "lower"}, {0x8E, "upper"}, {0x07, "bell"}, {0x1B, "esc"}};

std::string hexEscape(uint8_t b) {
    char buf[8];
    std::snprintf(buf, sizeof buf, "{$%02X}", b);
    return buf;
}

// A byte of a string or REM text
std::string textChar(uint8_t b) {
    if (b >= 0x20 && b <= 0x5D && b != 0x5C) return std::string(1, (char)b);
    if (b == 0x5C) return "{pound}";
    if (b == 0x5E) return "^";
    if (b == 0x5F) return "{left-arrow}";
    if (b >= 0xC1 && b <= 0xDA) return std::string(1, (char)(b - 0xC1 + 'a'));   // shifted letters
    if (b == 0xFF || b == 0xDE) return "{pi}";
    auto it = CONTROL.find(b);
    if (it != CONTROL.end()) return std::string("{") + it->second + "}";
    return hexEscape(b);
}

}  // namespace

const char* basicDialectName(BasicDialect d) {
    switch (d) {
        case BasicDialect::V2: return "BASIC V2";
        case BasicDialect::V7: return "BASIC V7";
        case BasicDialect::B65: return "BASIC 65";
        default: return "auto";
    }
}

BasicListing listBasic(const std::vector<uint8_t>& prg, BasicDialect dialect) {
    BasicListing r;
    if (prg.size() < 4) { r.error = "not a BASIC program (too short)"; return r; }
    uint16_t load = prg[0] | (prg[1] << 8);
    if (dialect == BasicDialect::AUTO)
        dialect = load == 0x0801 ? BasicDialect::V2 : load == 0x1C01 ? BasicDialect::V7
                : load == 0x2001 ? BasicDialect::B65 : BasicDialect::V2;
    r.dialect = dialect;

    // BASIC 65: basic45's tokens ($FE xx: its escape keywords)
    std::map<uint8_t, std::string> kw65, kw65fe;
    if (dialect == BasicDialect::B65) {
        BasicTokenizer t;
        for (const auto& [name, tok] : t.getKeywords()) kw65[tok] = name;
        for (const auto& [name, tok] : t.getEscapeKeywords()) kw65fe[tok] = name;
    }
    auto keyword = [&](const std::vector<uint8_t>& p, size_t& i, std::string& out) -> bool {
        uint8_t b = p[i];
        if (b == 0xFF) { out = "{pi}"; i++; return true; }
        if (dialect == BasicDialect::B65) {
            if (b == 0xFE && i + 1 < p.size() && kw65fe.count(p[i + 1])) { out = kw65fe[p[i + 1]]; i += 2; return true; }
            if (kw65.count(b)) { out = kw65[b]; i++; return true; }
            return false;
        }
        if (b >= 0x80 && b <= 0xCB) { out = V2[b - 0x80]; i++; return true; }
        if (dialect != BasicDialect::V7) return false;
        if ((b == 0xCE || b == 0xFE) && i + 1 < p.size()) {
            const auto& m = b == 0xCE ? V7CE : V7FE;
            auto it = m.find(p[i + 1]);
            if (it != m.end()) { out = it->second; i += 2; return true; }
            return false;
        }
        if (b >= 0xCC && b <= 0xFD && V7[b - 0xCC]) { out = V7[b - 0xCC]; i++; return true; }
        return false;
    };

    size_t pos = 2;
    std::set<size_t> seen;
    while (true) {
        if (pos + 2 > prg.size()) { r.warnings.push_back("the program ends without its end marker"); break; }
        uint16_t link = prg[pos] | (prg[pos + 1] << 8);
        if (link == 0) break;
        if (pos + 4 > prg.size()) { r.warnings.push_back("the program ends inside a line"); break; }
        if (!seen.insert(pos).second) { r.error = "the line links form a loop"; return r; }
        uint16_t num = prg[pos + 2] | (prg[pos + 3] << 8);
        std::string line = std::to_string(num) + " ";
        size_t i = pos + 4;
        bool inString = false, rem = false;
        while (i < prg.size() && prg[i] != 0) {
            uint8_t b = prg[i];
            if (b == 0x22) { inString = !inString; line += '"'; i++; continue; }
            if (inString || rem) { line += textChar(b); i++; continue; }
            std::string k;
            if (b >= 0x80 && keyword(prg, i, k)) {
                line += k;
                if (k == "REM") rem = true;
                continue;
            }
            if (b >= 0x80) {
                line += hexEscape(b);
                r.warnings.push_back("line " + std::to_string(num) + ": unknown token " + hexEscape(b));
            } else {
                line += textChar(b);
            }
            i++;
        }
        r.text += line + "\n";
        // the next line: where the link points (load address based), else
        // right after this one
        size_t next = (size_t)(link - load) + 2;
        pos = (next > pos && next < prg.size()) ? next : i + 1;
    }
    r.ok = true;
    return r;
}
