#include "BasicDecompiler.hpp"
#include "BasicTokenizer.hpp"
#include "PETSCIIEncoder.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <map>
#include <set>
#include <sstream>

namespace {

// Control codes in strings: the escape basic45 reads back
const std::map<uint8_t, std::string>& escapeNames() {
    static const std::map<uint8_t, std::string> m = {
        {0x90, "black"}, {0x05, "white"}, {0x1C, "red"}, {0x9F, "cyan"}, {0x9C, "purple"},
        {0x1E, "green"}, {0x1F, "blue"}, {0x9E, "yellow"}, {0x81, "orange"}, {0x95, "brown"},
        {0x96, "light-red"}, {0x97, "dark-gray"}, {0x98, "gray"}, {0x99, "light-green"},
        {0x9A, "light-blue"}, {0x9B, "light-gray"}, {0x13, "home"}, {0x93, "clear"},
        {0x12, "reverse"}, {0x92, "off"}, {0x02, "under"}, {0x82, "under-off"}, {0x94, "insert"},
        {0x14, "del"}, {0x07, "bell"}, {0x09, "tab"}, {0x1B, "esc"}, {0x91, "up"}, {0x11, "down"},
        {0x9D, "left"}, {0x1D, "right"}, {0x85, "f1"}, {0x86, "f3"}, {0x87, "f5"}, {0x88, "f7"},
        {0x89, "f2"}, {0x8A, "f4"}, {0x8B, "f6"}, {0x8C, "f8"},
        {0x0D, "return"}, {0x0A, "linefeed"}, {0x0E, "lower"}, {0x8E, "upper"},
        {0x0B, "case-lock"}, {0x0C, "case-unlock"}, {0x0F, "flash"}, {0x8F, "flash-off"},
        {0x1A, "tab-left"}, {0x15, "word-left"}, {0x17, "word-right"}, {0x18, "tab-set"},
    };
    return m;
}

std::string hexEscape(uint8_t b) {
    char buf[8];
    snprintf(buf, sizeof buf, "{$%02X}", b);
    return buf;
}

// A PETSCII byte in a string or REM/DATA text as basic45 source text
std::string textChar(uint8_t b, bool inString) {
    if (b >= 0x41 && b <= 0x5A) return std::string(1, (char)(b + 32));   // letters: lower case
    if (b == 0x20 || (b >= 0x21 && b <= 0x3F && b != 0x22) || b == 0x40 || b == 0x5B ||
        b == 0x5D || b == 0x5E)
        return std::string(1, (char)b);
    if (b == 0x22 && !inString) return "\"";
    if (inString) {
        auto it = escapeNames().find(b);
        if (it != escapeNames().end()) return "{" + it->second + "}";
        if (b >= 0xC1 && b <= 0xDA) return std::string("{shift-") + (char)('a' + b - 0xC1) + "}";
        static const uint8_t cbmCode[26] = {
            0xB0, 0xBF, 0xBC, 0xAC, 0xB1, 0xBB, 0xA5, 0xB4, 0xA2, 0xB5, 0xA1, 0xB6, 0xA7,
            0xAA, 0xB9, 0xAF, 0xAB, 0xB2, 0xAE, 0xA3, 0xB8, 0xBE, 0xB3, 0xBD, 0xB7, 0xAD,
        };
        for (int k = 0; k < 26; k++)
            if (cbmCode[k] == b) return std::string("{cbm-") + (char)('a' + k) + "}";
        if (b == 0x5C) return "{pound}";
        if (b == 0xDE) return "{pi}";
        if (b == 0xA0) return "{shift-space}";
    }
    return hexEscape(b);
}

}  // namespace

BasicDecompileResult decompileBasic(const std::vector<uint8_t>& prg) {
    BasicDecompileResult r;
    if (prg.size() < 4) { r.error = "not a BASIC program (too short)"; return r; }
    uint16_t load = prg[0] | (prg[1] << 8);

    BasicTokenizer tokenizer;
    PETSCIIEncoder petscii;
    std::map<uint8_t, std::string> kw, kwEsc, kwCe;
    for (const auto& [name, tok] : tokenizer.getKeywords()) kw[tok] = name;
    for (const auto& [name, tok] : tokenizer.getEscapeKeywords()) kwEsc[tok] = name;
    for (const auto& [name, tok] : tokenizer.getCeKeywords()) kwCe[tok] = name;
    static const std::map<uint8_t, std::string> ops = {
        {0xAA, "+"}, {0xAB, "-"}, {0xAC, "*"}, {0xAD, "/"}, {0xAE, "^"},
        {0xB1, ">"}, {0xB2, "="}, {0xB3, "<"},
    };

    // One piece of a line: a keyword, or text (numbers, names, strings...)
    struct Piece { bool isKw; std::string text; };
    struct Line { uint16_t num; std::vector<Piece> pieces; };
    std::vector<Line> lines;

    size_t pos = 2;
    std::set<size_t> seen;
    while (true) {
        if (pos + 2 > prg.size()) { r.warnings.push_back("program ends without its end marker"); break; }
        uint16_t link = prg[pos] | (prg[pos + 1] << 8);
        if (link == 0) break;
        if (pos + 4 > prg.size()) { r.warnings.push_back("program ends inside a line"); break; }
        if (!seen.insert(pos).second) { r.error = "line links form a loop"; return r; }
        Line line;
        line.num = prg[pos + 2] | (prg[pos + 3] << 8);
        size_t p = pos + 4;
        bool inString = false;
        enum { NORMAL, REM, DATA } mode = NORMAL;
        std::string text;
        auto flush = [&]() { if (!text.empty()) { line.pieces.push_back({false, text}); text.clear(); } };
        while (p < prg.size() && prg[p] != 0) {
            uint8_t b = prg[p++];
            if (mode == REM) { text += textChar(b, false); continue; }
            if (b == 0x22) { inString = !inString; text += '"'; continue; }
            if (inString && b == 0x1B && p < prg.size() && prg[p] != 0 && prg[p] != 0x22) {
                // a MEGA65 ESC sequence: ESC + key
                uint8_t k = prg[p];
                std::string name;
                for (const auto& [n, key] : petscii.getEscSequences())
                    if ((uint8_t)key == k && (name.empty() || n < name)) name = n;
                if (name.empty() && ((k >= 0x41 && k <= 0x5A) || (k >= 0x30 && k <= 0x39) || k == 0x40))
                    name = std::string("esc-") + (char)((k >= 0x41 && k <= 0x5A) ? k + 32 : k);
                if (!name.empty()) { text += "{" + name + "}"; p++; continue; }
            }
            if (inString) { text += textChar(b, true); continue; }
            if (mode == DATA) {
                if (b == ':') { mode = NORMAL; flush(); line.pieces.push_back({false, ":"}); continue; }
                text += textChar(b, false);
                continue;
            }
            if (b == 0xFE && p < prg.size() && (prg[p] == 0x52 || prg[p] == 0x53)) {
                flush();
                line.pieces.push_back({false, prg[p++] == 0x52 ? "<<" : ">>"});
            } else if (b == 0xFE && p < prg.size() && kwEsc.count(prg[p])) {
                flush();
                line.pieces.push_back({true, kwEsc[prg[p++]]});
            } else if (b == 0xCE && p < prg.size() && kwCe.count(prg[p])) {
                flush();
                line.pieces.push_back({true, kwCe[prg[p++]]});
            } else if (ops.count(b)) {
                flush();
                line.pieces.push_back({false, ops.at(b)});
            } else if (b >= 0x80 && kw.count(b)) {
                flush();
                line.pieces.push_back({true, kw[b]});
                if (kw[b] == "REM") mode = REM;
                if (kw[b] == "DATA") mode = DATA;
            } else if (b >= 0x80) {
                text += hexEscape(b);
                r.warnings.push_back("line " + std::to_string(line.num) + ": unknown token $" +
                                     hexEscape(b).substr(2, 2));
            } else if (b == ':' || b == ',' || b == '(' || b == ')' || b == ';') {
                flush();
                line.pieces.push_back({false, std::string(1, (char)b)});
            } else if (b == ' ') {
                flush();
            } else {
                text += (b >= 0x41 && b <= 0x5A) ? (char)(b + 32) : (char)b;
            }
        }
        flush();
        lines.push_back(line);
        size_t next = (size_t)link - load + 2;
        if (link < load || next >= prg.size()) {
            r.warnings.push_back("line " + std::to_string(line.num) + ": link outside the program");
            break;
        }
        pos = next;
    }

    // Line-number references: the numbers after the jump keywords
    std::set<uint16_t> lineNums, targets;
    for (const auto& l : lines) lineNums.insert(l.num);
    auto isNumber = [](const std::string& s) {
        return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit) && s.size() <= 5;
    };
    auto forEachRef = [&](Line& l, auto&& f) {
        for (size_t i = 0; i < l.pieces.size(); i++) {
            if (!l.pieces[i].isKw) continue;
            const std::string& k = l.pieces[i].text;
            size_t j = i + 1;
            bool list = false;
            if (k == "GOTO" || k == "GOSUB") list = true;
            else if (k == "GO" && j < l.pieces.size() && l.pieces[j].isKw && l.pieces[j].text == "TO") { list = true; j++; }
            else if (k != "THEN" && k != "ELSE" && k != "RESTORE" && k != "TRAP" && k != "RESUME" && k != "RUN") continue;
            while (j < l.pieces.size() && !l.pieces[j].isKw && isNumber(l.pieces[j].text)) {
                f(l.pieces[j]);
                if (!list || j + 2 >= l.pieces.size() || l.pieces[j + 1].text != ",") break;
                j += 2;
            }
        }
    };
    for (auto& l : lines)
        forEachRef(l, [&](Piece& p) {
            uint16_t n = (uint16_t)std::stoul(p.text);
            if (lineNums.count(n)) targets.insert(n);
        });
    for (auto& l : lines)
        forEachRef(l, [&](Piece& p) {
            uint16_t n = (uint16_t)std::stoul(p.text);
            if (targets.count(n)) p.text = "l" + std::to_string(n);
        });

    // Text: a space between words, none around punctuation
    std::ostringstream out;
    for (const auto& l : lines) {
        std::string s;
        for (const auto& p : l.pieces) {
            std::string t = p.isKw ? p.text : p.text;
            if (p.isKw) std::transform(t.begin(), t.end(), t.begin(), ::tolower);
            bool punct = t == ":" || t == "," || t == ";" || t == "(" || t == ")";
            if (!s.empty() && !punct && s.back() != '(' && s.back() != ' ')
                s += ' ';
            if (t == ":") { s += " : "; continue; }
            if (t == ",") { s += ", "; continue; }
            if (t == ";") { s += "; "; continue; }
            s += t;
            if (p.isKw && (t == "tab(" || t == "spc(")) continue;
        }
        // tidy the spacing left by punctuation
        std::string tidy;
        for (char c : s) {
            if (c == ' ' && !tidy.empty() && tidy.back() == ' ') continue;
            tidy += c;
        }
        while (!tidy.empty() && tidy.back() == ' ') tidy.pop_back();
        // two-character comparisons are two tokens: write them together
        for (const char* pair : {"< >", "< =", "> ="}) {
            size_t at;
            while ((at = tidy.find(pair)) != std::string::npos) tidy.erase(at + 1, 1);
        }
        // a run of three or more of the same escape: {name*n}
        {
            std::string packed;
            size_t i = 0;
            while (i < tidy.size()) {
                if (tidy[i] == '{') {
                    size_t e = tidy.find('}', i);
                    if (e != std::string::npos) {
                        std::string esc = tidy.substr(i, e - i + 1);
                        size_t n = 1, j = e + 1;
                        while (tidy.compare(j, esc.size(), esc) == 0) { n++; j += esc.size(); }
                        if (n >= 3 && esc.find('*') == std::string::npos) {
                            packed += esc.substr(0, esc.size() - 1) + "*" + std::to_string(n) + "}";
                            i = j;
                            continue;
                        }
                    }
                }
                packed += tidy[i++];
            }
            tidy = packed;
        }
        if (targets.count(l.num)) out << "l" << l.num << ": " << tidy << "\n";
        else out << "  " << tidy << "\n";
    }
    r.source = out.str();
    r.ok = true;
    return r;
}
