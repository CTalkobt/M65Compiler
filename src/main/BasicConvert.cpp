// BasicConvert — another dialect's program rewritten as BASIC 65 (#368)
//
// Works on the decompiler's lines (keyword and text pieces, before line
// numbers become labels). BASIC 7.0 (C128):
//
// - keywords BASIC 65 has under another name are renamed (SLOW -> SPEED;
//   PEN(0/1) -> LPEN(0/1); RCLR(n) -> RCOLOR(m), its colour sources
//   renumbered); RGR( (BASIC 65's RGRAPHIC(screen, parameter) is another
//   function) is warned about;
// - keywords BASIC 65 does not have are left as written, with a warning;
//   statements whose arguments differ (the graphics model, BANK) are warned
//   about;
// - TEMPO: BASIC 7.0's whole note lasts 1152 / (n * frame rate) s (19.2/n on
//   NTSC, 23.04/n on PAL), BASIC 65's 24/n on both (1200 PAL / 1440 NTSC
//   minus TEMPO per frame: b65.src): n is scaled;
// - PLAY: BASIC 7.0 plays one string, switching voices with Vn, and waits
//   (in the statement) whenever a note is for a voice that is still busy.
//   BASIC 65 takes a string per voice (PLAY v1$, v2$, ...; V is ignored),
//   returns at once and restarts the voices at the next PLAY; U is one digit
//   (0-9 through a table), M is modulation, octave and length are per voice
//   (default length a quarter, not a whole). A literal PLAY string is split
//   by voice, with rests where BASIC 7.0 would have held a voice back, its
//   octave and length written out, U mapped, M dropped; a wait for the
//   voices to be free goes in front of each PLAY.

#include "BasicDecompiler.hpp"
#include "BasicLister.hpp"
#include "BasicTokenizer.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <functional>
#include <map>
#include <set>
#include <sstream>

namespace {

using Pieces = std::vector<DecompiledPiece>;

std::string lower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

// The keywords basic45 tokenizes as BASIC 65's
const std::set<std::string>& b65Keywords() {
    static std::set<std::string> k = [] {
        std::set<std::string> s;
        BasicTokenizer t;
        for (const auto& [n, v] : t.getKeywords()) s.insert(n);
        for (const auto& [n, v] : t.getEscapeKeywords()) s.insert(n);
        for (const auto& [n, v] : t.getCeKeywords()) s.insert(n);
        s.insert("DIRECTORY");   // (DIR + ECTORY)
        s.insert("{pi}");
        return s;
    }();
    return k;
}

bool isNumber(const std::string& s) {
    if (s.empty()) return false;
    bool dot = false;
    for (char c : s) {
        if (c == '.' && !dot) dot = true;
        else if (!std::isdigit((unsigned char)c)) return false;
    }
    return true;
}

// The end of the statement that starts after i: the next ':' or ELSE
size_t statementEnd(const Pieces& p, size_t i) {
    size_t j = i;
    while (j < p.size() && !(!p[j].isKw && p[j].text == ":") && !(p[j].isKw && p[j].text == "ELSE")) j++;
    return j;
}

// Pieces as source text (for an argument kept inside a rewrite)
std::string joinPieces(const Pieces& p, size_t from, size_t to) {
    std::string s;
    for (size_t i = from; i < to; i++) {
        std::string t = p[i].isKw ? lower(p[i].text) : p[i].text;
        bool punct = t == "," || t == ")" || t == ";";
        if (!s.empty() && !punct && s.back() != '(' && s.back() != ' ') s += ' ';
        s += t;
    }
    return s;
}

// ---- PLAY ------------------------------------------------------------------

// BASIC 7.0's counts: a whole note 1152 (ROM $6F07)
const int WHOLE = 1152;
int lengthOf(char c) {
    switch (c) {
        case 'w': return WHOLE;
        case 'h': return WHOLE / 2;
        case 'q': return WHOLE / 4;
        case 'i': return WHOLE / 8;
        case 's': return WHOLE / 16;
    }
    return 0;
}
char letterOf(int len) {
    for (char c : std::string("whqis")) if (lengthOf(c) == len) return c;
    return 'q';
}

// BASIC 65's U: one digit through voltab (b65.src); BASIC 7.0's U is 0-15
int volumeDigit(int v) {
    static const int voltab[10] = {0, 1, 3, 5, 7, 8, 10, 12, 14, 15};
    int best = 0;
    for (int k = 1; k < 10; k++)
        if (std::abs(voltab[k] - v) <= std::abs(voltab[best] - v)) best = k;
    return best;
}

// What BASIC 7.0's PLAY carries from one statement to the next
struct PlayState {
    int voice = 1;
    int octave = 4;
    int length = WHOLE;     // BASIC 7.0's default: a whole note
};

struct PlayResult {
    std::string args;               // "s1", "s2", "s3" (empty ones left out)
    std::vector<int> voices;        // the voices the strings use
    std::vector<std::string> warnings;
};

// A BASIC 7.0 PLAY string (its text, the quotes removed) as BASIC 65 strings
PlayResult convertPlay(const std::string& src, PlayState& st) {
    PlayResult r;
    std::string out[4];
    long busy[4] = {0, 0, 0, 0};    // BASIC 7.0: when the voice is free (from the statement's start)
    long cursor[4] = {0, 0, 0, 0};  // BASIC 65: the end of the voice's string so far
    int octave65[4] = {-1, -1, -1, -1};
    int length65[4] = {-1, -1, -1, -1};
    long t = 0;                     // BASIC 7.0's place in the string, in time
    bool dotted = false;
    int accidental = 0;
    std::set<char> unknown;
    // (a space before a command, a rest or a note after a digit: readable, and no
    // digit runs into the next command)
    auto emit = [&](int v, const std::string& s) {
        bool command = std::string("otux").find(s[0]) != std::string::npos || s.back() == 'r';
        if (!out[v].empty() && out[v].back() != ' ' &&
            (command || std::isdigit((unsigned char)out[v].back())))
            out[v] += ' ';
        out[v] += s;
    };
    // a rest of `gap` counts on voice v, as BASIC 65 rests (dotted lengths too)
    auto rest = [&](int v, long gap) {
        static const int parts[][2] = {
            {WHOLE * 3 / 2, 1}, {WHOLE, 0}, {WHOLE * 3 / 4, 1}, {WHOLE / 2, 0}, {WHOLE * 3 / 8, 1},
            {WHOLE / 4, 0}, {WHOLE * 3 / 16, 1}, {WHOLE / 8, 0}, {WHOLE * 3 / 32, 1}, {WHOLE / 16, 0}};
        for (const auto& pt : parts) {
            while (gap >= pt[0]) {
                int base = pt[1] ? pt[0] * 2 / 3 : pt[0];
                std::string tok = length65[v] != base ? std::string(1, letterOf(base)) : "";
                length65[v] = base;
                emit(v, tok + (pt[1] ? ".r" : "r"));
                gap -= pt[0];
                cursor[v] += pt[0];
            }
        }
        if (gap > 0) r.warnings.push_back("a voice's wait shorter than a sixteenth note is left out");
    };
    for (size_t i = 0; i < src.size(); i++) {
        char c = (char)std::tolower((unsigned char)src[i]);
        auto digits = [&](int maxDigits) {
            int n = -1, k = 0;
            while (i + 1 < src.size() && std::isdigit((unsigned char)src[i + 1]) && k < maxDigits) {
                n = (n < 0 ? 0 : n * 10) + (src[++i] - '0');
                k++;
            }
            return n;
        };
        if (c == ' ') continue;
        if (c == 'v') {
            int n = digits(1);
            if (n >= 1 && n <= 3) st.voice = n;
            else r.warnings.push_back("PLAY: V" + std::to_string(n) + " is not a voice (1-3)");
        } else if (c == 'o') {
            int n = digits(1);
            if (n >= 0 && n <= 6) st.octave = n;
        } else if (c == 't' || c == 'x') {
            int n = digits(1);
            if (n >= 0) emit(st.voice, std::string(1, c) + std::to_string(n));
        } else if (c == 'u') {
            int n = digits(2);
            if (n >= 0) emit(st.voice, "u" + std::to_string(volumeDigit(std::min(n, 15))));
        } else if (c == 'm') {
            for (int v = 1; v <= 3; v++) t = std::max(t, busy[v]);   // wait for the measure: the rests do it
        } else if (lengthOf(c)) {
            st.length = lengthOf(c);
        } else if (c == '.') {
            dotted = true;
        } else if (c == '#') {
            accidental = 1;
        } else if (c == '$') {
            accidental = -1;
        } else if (c == 'r' || (c >= 'a' && c <= 'g')) {
            int v = st.voice;
            long dur = dotted ? st.length * 3 / 2 : st.length;
            long start = std::max(t, busy[v]);
            if (start > cursor[v]) rest(v, start - cursor[v]);
            if (c != 'r' && octave65[v] != st.octave) { emit(v, "o" + std::to_string(st.octave)); octave65[v] = st.octave; }
            if (length65[v] != st.length) { emit(v, std::string(1, letterOf(st.length))); length65[v] = st.length; }
            std::string note;
            if (dotted) note += '.';
            if (accidental > 0) note += '#';
            if (accidental < 0) note += '$';
            note += c;
            emit(v, note);
            cursor[v] = start + dur;
            busy[v] = start + dur;
            t = start;
            dotted = false;
            accidental = 0;
        } else {
            unknown.insert(src[i]);
        }
    }
    for (char u : unknown) r.warnings.push_back(std::string("PLAY: '") + u + "' left out (not BASIC 7.0's)");
    int last = 0;
    for (int v = 1; v <= 3; v++) if (!out[v].empty()) { last = v; r.voices.push_back(v); }
    for (int v = 1; v <= last; v++) {
        if (v > 1) r.args += ", ";
        if (!out[v].empty()) r.args += "\"" + out[v] + "\"";
        if (out[v].size() > 255) r.warnings.push_back("PLAY: voice " + std::to_string(v) + "'s string is over 255 characters");
    }
    return r;
}

// Wait until the voices are free: RPLAY(v) while BASIC 65 still has string
// left for v, and its voice counter (b65.src `voices`, $125D + 2 * (v - 1):
// bit 7 of the high byte set when idle) while the last note sounds
std::string playWait(const std::vector<int>& voices) {
    std::string cond, idle;
    for (int v : voices) {
        cond += (cond.empty() ? "" : " or ") + std::string("rplay(") + std::to_string(v) + ")";
        idle += (idle.empty() ? "" : " and ") + std::string("peek(") + std::to_string(0x125E + 2 * (v - 1)) + ")";
    }
    return "do while " + cond + " or (" + idle + " and 128) = 0 : loop :";
}

// ---- BASIC 7.0 ---------------------------------------------------------------

const std::map<std::string, std::string>& v7Renames() {
    static const std::map<std::string, std::string> m = {
    };   // (SLOW: pass 2 makes it SPEED 1, as SPEED alone is 40 MHz)
    return m;
}

// BASIC 7.0 statements BASIC 65 has, with other arguments
const std::map<std::string, std::string>& v7Differ() {
    static const std::map<std::string, std::string> m = {
        {"BANK", "the C128's bank numbers are not the MEGA65's"},
        {"WINDOW", "check WINDOW's arguments (BASIC 65: WINDOW left, top, right, bottom, clear)"},
    };
    return m;
}

// BASIC 7.0 keywords with no BASIC 65 counterpart, and why
const std::map<std::string, std::string>& v7Missing() {
    static const std::map<std::string, std::string> m = {
        {"RGR", "BASIC 65's RGRAPHIC(screen, parameter) reads a screen's settings, not the GRAPHIC mode"},
        {"SCALE", "BASIC 65's coordinates are the screen's pixels"},
        {"SSHAPE", "BASIC 65 copies screen areas with GCOPY / CUT / PASTE"},
        {"GSHAPE", "BASIC 65 copies screen areas with GCOPY / CUT / PASTE"},
        {"WIDTH", "BASIC 65's lines are one pixel wide"},
        {"RLUM", "the MEGA65's colours have no luminance"},
    };
    return m;
}

// A statement's arguments: the pieces between from and to split at the
// top-level commas (and TO, for DRAW); an omitted one is ""
struct Args {
    std::vector<std::string> list;
    std::vector<bool> afterTo;      // the argument follows a TO
};
Args splitArgs(const Pieces& p, size_t from, size_t to) {
    Args a;
    int depth = 0;
    size_t start = from;
    bool to_ = false;
    for (size_t j = from; j <= to; j++) {
        bool end = j == to;
        if (!end) {
            const auto& t = p[j];
            if (!t.isKw && t.text == "(") depth++;
            if (!t.isKw && t.text == ")") depth--;
            if (!t.isKw && t.text.size() > 1 && t.text.back() == '(') depth++;   // (a name with its parenthesis)
            if (t.isKw && t.text.back() == '(') depth++;
        }
        if (end || (depth == 0 && ((!p[j].isKw && p[j].text == ",") || (p[j].isKw && p[j].text == "TO")))) {
            a.list.push_back(joinPieces(p, start, j));
            a.afterTo.push_back(to_);
            if (!end) to_ = p[j].isKw && p[j].text == "TO";
            start = j + 1;
        }
    }
    return a;
}

// "x" times f, folded for a number
std::string times(const std::string& x, int f) {
    if (f == 1) return x;
    if (isNumber(x)) {
        double v = std::stod(x) * f;
        std::ostringstream s;
        s << v;
        return s.str();
    }
    return "(" + x + ") * " + std::to_string(f);
}
std::string minus1(const std::string& x) {
    if (isNumber(x)) return std::to_string(std::stoi(x) - 1);
    return "(" + x + ") - 1";
}

// The program's variables: only a name's first two letters count in both
// BASICs (BASEX and BASEY are one variable), but basic45 keeps long names
// apart: names that are one variable on the source machine are written as
// their two letters. DT$ (and any DT...$) is BASIC 65's date, a reserved name.
void mergeVariables(std::vector<DecompiledLine>& lines, std::vector<std::string>& warnings, std::vector<std::string>& notes,
                    const std::string& machine) {
    auto isIdent = [](const std::string& t) {
        if (t.empty() || !std::isalpha((unsigned char)t[0])) return false;
        for (size_t q = 1; q < t.size(); q++) {
            char c = t[q];
            if (std::isalnum((unsigned char)c)) continue;
            if ((c == '$' || c == '%') && q + 1 == t.size()) continue;
            return false;
        }
        return true;
    };
    auto suffixOf = [](const std::string& t) { char c = t.back(); return (c == '$' || c == '%') ? std::string(1, c) : std::string(); };
    auto base2 = [&](const std::string& t) {
        std::string sfx = suffixOf(t);
        std::string stem = t.substr(0, t.size() - sfx.size());
        return stem.substr(0, std::min<size_t>(2, stem.size())) + sfx;
    };
    // the variables: key (two letters, type, array) -> spellings
    std::map<std::string, std::set<std::string>> spellings;
    std::set<std::string> taken;
    auto forEachIdent = [&](auto&& f) {
        for (auto& l : lines) {
            bool data = false;
            for (size_t j = 0; j < l.pieces.size(); j++) {
                auto& pc = l.pieces[j];
                if (pc.isKw && pc.text == "REM") break;
                if (pc.isKw && pc.text == "DATA") { data = true; continue; }
                if (!pc.isKw && pc.text == ":") { data = false; continue; }
                if (data || pc.isKw || !isIdent(pc.text)) continue;
                bool array = j + 1 < l.pieces.size() && l.pieces[j + 1].text == "(";
                bool fn = j > 0 && l.pieces[j - 1].isKw && l.pieces[j - 1].text == "FN";
                f(pc, (fn ? "fn " : "") + base2(pc.text) + (array ? "(" : ""), l.num);
            }
        }
    };
    forEachIdent([&](DecompiledPiece& pc, const std::string& key, uint16_t) {
        spellings[key].insert(pc.text);
        taken.insert(pc.text.substr(0, std::min<size_t>(2, pc.text.size())));
    });
    std::map<std::string, std::string> rename;     // spelling -> new name
    for (const auto& [key, names] : spellings) {
        std::string b = key;
        if (b.rfind("fn ", 0) == 0) b = b.substr(3);
        if (!b.empty() && b.back() == '(') b.pop_back();
        std::string sfx = suffixOf(b);
        if (b.rfind("dt", 0) == 0 && sfx == "$" && key.back() != '(' && key.rfind("fn ", 0) != 0) {
            std::string fresh;
            for (char x = 'a'; x <= 'z' && fresh.empty(); x++)
                for (char y : std::string("abcdefghijklmnopqrstuvwxyz0123456789"))
                    if (!taken.count(std::string{x, y}) && std::string{x, y} != "dt") { fresh = std::string{x, y}; break; }
            taken.insert(fresh);
            for (const auto& n : names) rename[n] = fresh + sfx;
            warnings.push_back(*names.begin() + " is renamed " + fresh + sfx + ": BASIC 65's DT$ is the date (a reserved name)");
        } else if (names.size() > 1) {
            std::string all;
            for (const auto& n : names) { rename[n] = b; all += (all.empty() ? "" : ", ") + n; }
            notes.push_back(all + ": one variable on the " + machine + " (two letters count): written " + b);
        }
    }
    if (!rename.empty())
        forEachIdent([&](DecompiledPiece& pc, const std::string&, uint16_t) {
            if (auto it = rename.find(pc.text); it != rename.end()) pc.text = it->second;
        });
}

// BASIC 3.5's colours (the TED's 1-16, each with a luminance 0-7) as the
// MEGA65's 0-15: the same hue where both have it (1-10), else the nearest
// (yellow-green and light green: light green, pink: light red, blue-green:
// cyan, dark blue: blue); white's luminances are the greys, red's, green's and
// blue's light ones (5-7) their light colours. lum < 0: not known (the hue's)
const char* const TED_COLOURS = "00010203040506070809131003140613";   // 1-16, two digits each
int tedColour(int c, int lum) {
    if (c < 1 || c > 16) return -1;
    int m = (TED_COLOURS[(c - 1) * 2] - '0') * 10 + (TED_COLOURS[(c - 1) * 2 + 1] - '0');
    if (lum < 0) return m;
    if (c == 2) return lum <= 1 ? 11 : lum <= 4 ? 12 : lum <= 6 ? 15 : 1;
    if (lum >= 5 && c == 3) return 10;
    if (lum >= 5 && c == 6) return 13;
    if (lum >= 5 && c == 7) return 14;
    return m;
}

void convertV7(std::vector<DecompiledLine>& lines, const BasicDecompileOptions& o, std::vector<std::string>& warnings,
               std::vector<std::string>& notes) {
    const double factor = o.ntsc ? 24.0 / 19.2 : 24.0 / 23.04;
    // BASIC 3.5 (C16 / Plus/4): BASIC 7.0's ancestor, converted by the same rules (its graphics
    // statements are BASIC 7.0's), plus the TED's colours (and luminances) and sound
    const bool v35 = o.source == BasicSource::V35;
    // Simons' BASIC: convertSimons has rewritten its graphics as BASIC 7.0's statements (its plot
    // types as colour sources, "@@cs" for their array, __MODE n for MULTI), converted from here on
    const bool simons = o.source == BasicSource::SIMONS;
    const size_t warnings0 = warnings.size(), notes0 = notes.size();
    auto warn = [&](uint16_t n, const std::string& m) { warnings.push_back("line " + std::to_string(n) + ": " + m); };

    if (!simons) mergeVariables(lines, warnings, notes, v35 ? "Plus/4" : "C128");

    // The graphics state the converted program keeps: the colour sources and
    // the pixel cursor, in names the program does not use (two letters count)
    std::set<std::string> used;
    bool graphics = false;
    static const std::set<std::string> drawing = {"COLOR", "DRAW", "BOX", "CIRCLE", "PAINT", "CHAR", "LOCATE", "RDOT", "GRAPHIC"};
    for (const auto& l : lines)
        for (const auto& pc : l.pieces) {
            if (pc.isKw) { if (drawing.count(pc.text)) graphics = true; continue; }
            if (pc.text.empty() || pc.text[0] == '"' || !std::isalpha((unsigned char)pc.text[0])) continue;
            used.insert(lower(pc.text.substr(0, std::min<size_t>(2, pc.text.size()))));
        }
    // names the program does not use, handed out in order (two letters count)
    std::set<std::string> allocated;
    auto fresh = [&]() {
        for (char a : std::string("zqjkwvu"))
            for (char b : std::string("cxyabdefghijklmnopqrstuvw0123456789")) {
                std::string n{a, b};
                if (!used.count(n) && !allocated.count(n)) { allocated.insert(n); return n; }
            }
        return std::string("zz");
    };
    bool csNamed = false;   // (Simons': the program sets the colour sources itself)
    const std::string CS = fresh(), PX = fresh(), PY = fresh();
    for (auto& l : lines)
        for (auto& pc : l.pieces)
            if (!pc.isKw)
                for (size_t q; (q = pc.text.find("@@cs")) != std::string::npos;) { pc.text.replace(q, 4, CS); graphics = csNamed = true; }
    // the subroutines' arguments and temporaries (shared: they do not nest)
    std::vector<std::string> subArg, subTmp;
    auto argName = [&](size_t k) { while (subArg.size() <= k) subArg.push_back(fresh()); return subArg[k]; };
    auto tmpName = [&](size_t k) { while (subTmp.size() <= k) subTmp.push_back(fresh()); return subTmp[k]; };
    auto src = [&](const std::string& s) { return CS + "(" + (s.empty() ? "1" : s) + ")"; };
    std::string penNow;             // the colour source PEN was last set to in this line
    auto pen = [&](const std::string& s) {
        std::string c = src(s);
        if (c == penNow) return std::string();
        penNow = c;
        return "pen 0, " + c + " : ";
    };
    int mode = 0;                   // the last GRAPHIC mode, in program order (3/4: multicolour, x 0-159)
    bool usedState = false;

    // pass 1: functions and renamed keywords, everywhere (also inside statements rewritten below)
    for (auto& line : lines) {
        auto& p = line.pieces;
        bool data = false;
        for (size_t i = 0; i < p.size(); i++) {
            if (p[i].isKw && p[i].text == "DATA") data = true;
            if (!p[i].isKw && p[i].text == ":") data = false;
            if (data) continue;
            if (v35 && !p[i].isKw && !p[i].text.empty() && p[i].text[0] == '"') {
                // the Plus/4's colour codes 9-16 and flash on / off are other codes on the MEGA65
                static const std::map<std::string, std::string> codes = {
                    {"light-red", "light-green"}, {"dark-gray", "light-red"}, {"gray", "cyan"},
                    {"light-green", "light-blue"}, {"light-blue", "blue"}, {"light-gray", "light-green"},
                    {"under-off", "flash"}, {"$84", "flash-off"}};
                std::string t = p[i].text, r;
                for (size_t q = 0; q < t.size(); q++) {
                    size_t e2 = t[q] == '{' ? t.find('}', q) : std::string::npos;
                    if (e2 == std::string::npos) { r += t[q]; continue; }
                    std::string n = t.substr(q + 1, e2 - q - 1);
                    auto it = codes.find(n);
                    r += "{" + (it != codes.end() ? it->second : n) + "}";
                    q = e2;
                }
                p[i].text = r;
                continue;
            }
            if (v35 && p[i].isKw && p[i].text == "CHR$" && i + 3 < p.size() && p[i + 1].text == "(" && isNumber(p[i + 2].text) &&
                p[i + 3].text == ")") {
                static const std::map<int, int> codes = {{150, 153}, {151, 150}, {152, 159}, {153, 154}, {154, 31},
                                                         {155, 153}, {130, 15}, {132, 143}};
                if (auto it = codes.find(std::stoi(p[i + 2].text)); it != codes.end()) p[i + 2].text = std::to_string(it->second);
                continue;
            }
            if (!p[i].isKw && (p[i].text == "ti" || p[i].text == "ti$")) {
                // TI: BASIC 7.0 counts 1/60 s, BASIC 65 seconds (its 1 MHz timer). TI$: BASIC 7.0's
                // "HHMMSS" (settable), BASIC 65's "HH:MM:SS" from the clock (reserved: not settable)
                bool target = i + 1 < p.size() && p[i + 1].text == "=" &&
                              (i == 0 || p[i - 1].text == ":" || (p[i - 1].isKw && (p[i - 1].text == "THEN" || p[i - 1].text == "ELSE" || p[i - 1].text == "LET")));
                if (p[i].text == "ti") { if (!target) p[i].text = "int(ti * 60)"; continue; }
                if (target) {
                    size_t e = statementEnd(p, i);
                    std::string v = joinPieces(p, i + 2, e);
                    if (v == "\"000000\"") {
                        p.erase(p.begin() + (long)i + 1, p.begin() + (long)e);
                        p[i] = {false, "clr ti"};
                        warn(line.num, "TI$ = \"000000\" -> CLR TI (TI starts again; BASIC 65's TI$ is the real-time clock, not reset)");
                    } else {
                        warn(line.num, "TI$ = ...: BASIC 65's TI$ is the real-time clock, a reserved name (not settable)");
                    }
                    continue;
                }
                p[i].text = "(left$(ti$, 2) + mid$(ti$, 4, 2) + right$(ti$, 2))";
                continue;
            }
            if (!p[i].isKw) continue;
            std::string k = p[i].text;
            if (k == "REM") break;
            // a function's arguments: k ( a, b ) -> the pieces' end (the ')') and the arguments
            auto fnArgs = [&](size_t& close) -> Args {
                close = i;
                if (i + 1 >= p.size() || p[i + 1].text != "(") return {};
                int depth = 0;
                for (size_t j = i + 1; j < p.size(); j++) {
                    if (p[j].text == "(" || (p[j].text.size() > 1 && p[j].text.back() == '(')) depth++;
                    if (p[j].text == ")" && --depth == 0) { close = j; return splitArgs(p, i + 2, j); }
                }
                return {};
            };
            if (k == "RSPRITE" || k == "RSPPOS") {
                // sprites 1-8 -> 0-7; RSPRITE(n, 1), the colour, 0-15 -> 1-16
                size_t close;
                Args a = fnArgs(close);
                if (a.list.size() == 2) {
                    std::string t = lower(k) + "(" + minus1(a.list[0]) + ", " + a.list[1] + ")";
                    if (k == "RSPRITE" && a.list[1] == "1") t = "(" + t + " + 1)";
                    else if (k == "RSPRITE" && !isNumber(a.list[1]))
                        warn(line.num, "RSPRITE(n, " + a.list[1] + "): its colour (1) is 0-15 in BASIC 65, 1-16 in BASIC 7.0");
                    p.erase(p.begin() + (long)i + 1, p.begin() + (long)close + 1);
                    p[i] = {false, t};
                }
                continue;
            }
            if (k == "XOR") {
                // BASIC 7.0's XOR(a, b) is BASIC 65's operator a XOR b
                size_t close;
                Args a = fnArgs(close);
                if (a.list.size() == 2) {
                    p.erase(p.begin() + (long)i + 1, p.begin() + (long)close + 1);
                    p[i] = {false, "((" + a.list[0] + ") xor (" + a.list[1] + "))"};
                }
                continue;
            }
            if (k == "RSPCOLOR") {
                size_t close;
                Args a = fnArgs(close);
                if (a.list.size() == 1) {
                    p.erase(p.begin() + (long)i + 1, p.begin() + (long)close + 1);
                    p[i] = {false, "(rspcolor(" + a.list[0] + ") + 1)"};
                }
                continue;
            }
            if (k == "RWINDOW") {
                int n = -1;
                if (i + 3 < p.size() && p[i + 1].text == "(" && isNumber(p[i + 2].text) && p[i + 3].text == ")") n = std::stoi(p[i + 2].text);
                if (n != 2) warn(line.num, "RWINDOW(" + (n >= 0 ? std::to_string(n) : std::string("n")) +
                                 "): BASIC 65 gives the window's width (0) and height (1); check against BASIC 7.0's");
                continue;
            }
            auto literalArg = [&](int& n) {
                if (i + 3 < p.size() && p[i + 1].text == "(" && isNumber(p[i + 2].text) && p[i + 3].text == ")") {
                    n = std::stoi(p[i + 2].text);
                    return true;
                }
                return false;
            };
            if (auto it = v7Renames().find(k); it != v7Renames().end()) {
                p[i].text = it->second;
            } else if (k == "PEN") {
                int n;
                p[i].text = "LPEN";
                if (!(literalArg(n) && (n == 0 || n == 1)))
                    warn(line.num, "PEN( -> LPEN(: BASIC 65's LPEN(0/1) are the light pen's x and y; "
                                   "PEN(2/3) (the 80-column screen) and PEN(4) have no counterpart");
            } else if (k == "RCLR") {
                // BASIC 7.0: the colour (1-16) of source 0 background, 1 foreground, 2/3 multicolour,
                // 4 border, 5 character, 6 the 80-column background; BASIC 65's RCOLOR: the index
                // (0-15) of 0 background, 1 text, 2 highlight, 3 border
                static const std::map<int, int> m = {{0, 0}, {4, 3}, {5, 1}};
                int n;
                if (literalArg(n) && m.count(n)) {
                    p.erase(p.begin() + (long)i + 1, p.begin() + (long)i + 4);
                    p[i] = {false, "(rcolor(" + std::to_string(m.at(n)) + ") + 1)"};
                } else if (literalArg(n) && n >= 1 && n <= 3) {
                    p.erase(p.begin() + (long)i + 1, p.begin() + (long)i + 4);
                    p[i] = {false, "(" + CS + "(" + std::to_string(n) + ") + 1)"};
                    usedState = true;
                } else {
                    p[i].text = "RCOLOR";
                    warn(line.num, "RCLR( -> RCOLOR(: BASIC 65's sources are 0 background, 1 text, 2 highlight, 3 border "
                                   "(BASIC 7.0's 0, 5, -, 4), its colours 0-15 (BASIC 7.0's 1-16): check");
                }
            } else if (k == "RDOT") {
                int n;
                if (literalArg(n) && n >= 0 && n <= 2) {
                    p.erase(p.begin() + (long)i + 1, p.begin() + (long)i + 4);
                    p[i] = {false, n == 0 ? PX : n == 1 ? PY : "pixel(" + PX + ", " + PY + ")"};
                    usedState = true;
                    if (n == 2) warn(line.num, "RDOT(2): PIXEL() gives the colour there, not BASIC 7.0's colour source");
                } else {
                    warn(line.num, "RDOT( of an expression: BASIC 65 has PIXEL(x, y); the pixel cursor is " + PX + ", " + PY);
                }
            } else if (auto it = v7Missing().find(k); it != v7Missing().end()) {
                warn(line.num, k + " has no BASIC 65 counterpart (" + it->second + "): left as written");
            } else if (auto it = v7Differ().find(k); it != v7Differ().end()) {
                warn(line.num, k + ": " + it->second);
            } else if (!b65Keywords().count(k) && k != "DRAW" && k != "LOCATE" && k != "SLOW" && k != "__PEN" && k != "__MODE" &&
                       k != "FETCH" && k != "STASH" && k != "SWAP") {
                warn(line.num, k + " has no BASIC 65 counterpart: left as written");
            }
        }
    }

    // Subroutines the converted program calls (after an END at its end): the
    // waits before PLAY, 40 / 80 columns, closing a graphics screen
    const uint16_t lastNum = lines.empty() ? 0 : lines.back().num;
    std::vector<DecompiledLine> helperLines;
    std::map<std::string, uint16_t> helperAt;
    uint32_t nextNum = (uint32_t)lastNum + 2;   // (only names: basic45 numbers the lines)
    // a subroutine of lines; body(at) builds them knowing their first line's number (for
    // their own GOTOs and GOSUBs, which become labels as the program's do)
    auto helperP = [&](const std::string& key, const std::function<std::vector<Pieces>(uint16_t)>& body) -> uint16_t {
        if (auto it = helperAt.find(key); it != helperAt.end()) return it->second;
        uint16_t at = (uint16_t)nextNum;
        for (auto& b : body(at)) {
            if (nextNum > 65535) { warnings.push_back("no line numbers left after line " + std::to_string(lastNum) + " for its subroutines"); break; }
            helperLines.push_back({(uint16_t)nextNum++, b});
        }
        helperAt[key] = at;
        return at;
    };
    auto helper = [&](const std::string& key, const std::vector<std::string>& body) -> uint16_t {
        return helperP(key, [&](uint16_t) {
            std::vector<Pieces> v;
            for (const auto& b : body) v.push_back({{false, b}});
            return v;
        });
    };
    auto gosub = [](uint16_t n) { return Pieces{{true, "GOSUB"}, {false, std::to_string(n)}}; };
    auto jump = [](const char* kw, uint16_t n) { return Pieces{{true, kw}, {false, std::to_string(n)}}; };
    auto cat = [](std::initializer_list<Pieces> parts) {
        Pieces r;
        for (const auto& q : parts) r.insert(r.end(), q.begin(), q.end());
        return r;
    };
    auto raw = [](const std::string& t) { return Pieces{{false, t}}; };
    const std::string RAD = ".0174533";   // pi / 180

    // BOX rotated: BASIC 7.0's corners (box.src: about the centre, the half sides dx, dy turned
    // by the angle: corner (sx, sy) at cx + sx*dx*cos - sy*dy*sin, cy + sx*dx*sin + sy*dy*cos),
    // in its own coordinates, x times the multicolour factor; the pixel cursor at x2, y2
    auto boxSub = [&]() {
        return helperP("box", [&](uint16_t) {
            std::string x1 = argName(0), y1 = argName(1), x2 = argName(2), y2 = argName(3), an = argName(4),
                        fl = argName(5), f = argName(6);
            std::string cx = tmpName(0), cy = tmpName(1), dx = tmpName(2), dy = tmpName(3), c = tmpName(4), sn = tmpName(5),
                        k1 = tmpName(6), k2 = tmpName(7), k3 = tmpName(8), k4 = tmpName(9);
            return std::vector<Pieces>{
                raw(cx + " = (" + x1 + " + " + x2 + ") / 2 : " + cy + " = (" + y1 + " + " + y2 + ") / 2 : " + dx + " = abs(" +
                    x2 + " - " + x1 + ") / 2 : " + dy + " = abs(" + y2 + " - " + y1 + ") / 2"),
                raw(c + " = cos(" + an + " * " + RAD + ") : " + sn + " = sin(" + an + " * " + RAD + ") : " + k1 + " = " + dx +
                    " * " + c + " : " + k2 + " = " + dy + " * " + sn + " : " + k3 + " = " + dx + " * " + sn + " : " + k4 + " = " +
                    dy + " * " + c),
                // the corners (-,-), (+,-), (+,+), (-,+), rounded as boxpnt does (cos(90) is not quite 0)
                raw(tmpName(10) + " = int(" + cx + " - " + k1 + " + " + k2 + " + .5) * " + f + " : " + tmpName(11) + " = int(" + cy +
                    " - " + k3 + " - " + k4 + " + .5) : " + tmpName(12) + " = int(" + cx + " + " + k1 + " + " + k2 + " + .5) * " + f +
                    " : " + tmpName(13) + " = int(" + cy + " + " + k3 + " - " + k4 + " + .5)"),
                raw(tmpName(14) + " = int(" + cx + " + " + k1 + " - " + k2 + " + .5) * " + f + " : " + tmpName(15) + " = int(" + cy +
                    " + " + k3 + " + " + k4 + " + .5) : " + tmpName(16) + " = int(" + cx + " - " + k1 + " - " + k2 + " + .5) * " + f +
                    " : " + tmpName(17) + " = int(" + cy + " - " + k3 + " + " + k4 + " + .5)"),
                raw("box " + tmpName(10) + ", " + tmpName(11) + ", " + tmpName(12) + ", " + tmpName(13) + ", " + tmpName(14) + ", " +
                    tmpName(15) + ", " + tmpName(16) + ", " + tmpName(17) + ", " + fl),
                raw(PX + " = " + x2 + " * " + f + " : " + PY + " = " + y2 + " : return"),
            };
        });
    };
    // CIRCLE as BASIC 7.0 draws it (circle.src): points every inc degrees from the arc's
    // start to its end (an end not after the start: + 360), the last exactly at the end,
    // point(b) = (xc + xr*cos(a)*sin(b) + yr*sin(a)*cos(b), yc + xr*sin(a)*sin(b) - yr*cos(a)*cos(b)),
    // a the rotation; x times the multicolour factor; the pixel cursor at the last point
    auto circleSub = [&]() {
        return helperP("circle", [&](uint16_t at) {
            std::string xc = argName(0), yc = argName(1), xr = argName(2), yr = argName(3), sa = argName(4), ea = argName(5),
                        ro = argName(6), in = argName(7), f = argName(8);
            std::string c = tmpName(4), sn = tmpName(5), b = tmpName(0), d = tmpName(1), qx = tmpName(2), qy = tmpName(3);
            uint16_t loop = (uint16_t)(at + 2), point = (uint16_t)(at + 4);
            Pieces step = cat({gosub(point), raw(":"), raw("line " + PX + ", " + PY + ", " + qx + ", " + qy + " : " + PX + " = " +
                                                      qx + " : " + PY + " = " + qy)});
            return std::vector<Pieces>{
                raw("if " + ea + " <= " + sa + " then " + ea + " = " + ea + " + 360"),
                cat({raw(c + " = cos(" + ro + " * " + RAD + ") : " + sn + " = sin(" + ro + " * " + RAD + ") : " + b + " = " + sa),
                     raw(":"), gosub(point), raw(":"), raw(PX + " = " + qx + " : " + PY + " = " + qy)}),
                cat({raw(b + " = " + b + " + " + in + " : if " + b + " >= " + ea + " then " + b + " = " + ea), raw(":"), step,
                     raw(":"), raw("return")}),
                cat({step, raw(":"), jump("GOTO", loop)}),
                raw(d + " = " + b + " * " + RAD + " : " + qx + " = int((" + xc + " + " + xr + " * " + c + " * sin(" + d + ") + " + yr +
                    " * " + sn + " * cos(" + d + ")) * " + f + " + .5) : " + qy + " = int(" + yc + " + " + xr + " * " + sn +
                    " * sin(" + d + ") - " + yr + " * " + c + " * cos(" + d + ") + .5) : return"),
            };
        });
    };
    auto columns = [&](int n) {
        return helper(n == 40 ? "40" : "80", {"if rwindow(2) = " + std::string(n == 40 ? "80" : "40") +
                                             " then print chr$(27) + \"x\";", "return"});
    };
    auto closeScreen = [&]() { return helper("close", {"if rgraphic(0, 0) = 1 then screen close 0", "return"}); };

    std::string soundFactor;        // BASIC 3.5's SOUND: the SID's counts per Hz (its clock)

    // pass 2: statements
    PlayState play;
    bool anyPlay = false, anyTempo = false;
    for (auto& line : lines) {
        auto& p = line.pieces;
        penNow.clear();
        for (size_t i = 0; i < p.size(); i++) {
            if (!p[i].isKw) continue;
            std::string k = p[i].text;
            if (k == "REM") break;
            size_t e = statementEnd(p, i + 1);
            // the statement [i, e) replaced by text, or by pieces (GOSUBs, which become labels)
            auto replace = [&](const std::string& text) {
                p.erase(p.begin() + (long)i + 1, p.begin() + (long)e);
                p[i] = {false, text};
            };
            auto replacePieces = [&](Pieces np) {
                p.erase(p.begin() + (long)i, p.begin() + (long)e);
                p.insert(p.begin() + (long)i, np.begin(), np.end());
                i += np.size() - 1;
            };
            // the statement dropped, with its ':'
            auto drop = [&]() {
                size_t from = i, to = e;
                if (from > 0 && p[from - 1].text == ":") from--;
                else if (to < p.size() && p[to].text == ":") to++;
                p.erase(p.begin() + (long)from, p.begin() + (long)to);
                i = from - 1;   // (from > 0 here, or the loop's i++ makes it 0)
            };
            auto then = [](Pieces a, const Pieces& b) { a.push_back({false, ":"}); a.insert(a.end(), b.begin(), b.end()); return a; };
            if (k == "__PEN") {
                // (Simons' TEXT: the pen of a colour source)
                std::string t = pen(p[i + 1].text);
                usedState = true;
                if (t.empty()) drop();
                else replace(t.substr(0, t.size() - 3));
            } else if (k == "__MODE") {
                // (Simons' MULTI / HIRES: the mode changes, the screen stays)
                mode = std::stoi(p[i + 1].text);
                drop();
            } else if (k == "TEMPO") {
                anyTempo = true;
                if (e == i + 2 && isNumber(p[i + 1].text)) {
                    double n = std::stod(p[i + 1].text);
                    long m = std::lround(n * factor);
                    if (n == 0) warn(line.num, "TEMPO 0 (BASIC 7.0: stop) is an error in BASIC 65: TEMPO 1");
                    m = std::max(1L, std::min(255L, m));
                    p[i + 1].text = std::to_string(m);
                } else if (e > i + 1) {
                    std::string arg = joinPieces(p, i + 1, e);
                    char f[32];
                    std::snprintf(f, sizeof f, "%.4g", factor);
                    p.erase(p.begin() + (long)i + 1, p.begin() + (long)e);
                    p.insert(p.begin() + (long)i + 1, {false, "int((" + arg + ") * " + f + " + .5)"});
                }
            } else if (k == "PLAY") {
                anyPlay = true;
                const std::string& a = e == i + 2 ? p[i + 1].text : "";
                if (a.size() >= 2 && a.front() == '"' && a.back() == '"' && a.find('{') == std::string::npos) {
                    PlayResult pr = convertPlay(a.substr(1, a.size() - 2), play);
                    for (const auto& w : pr.warnings) warn(line.num, w);
                    p[i + 1].text = pr.args;
                    if (!pr.voices.empty()) {
                        std::string key = "play";
                        for (int v : pr.voices) key += std::to_string(v);
                        Pieces w = gosub(helper(key, {playWait(pr.voices) + " return"}));
                        w.push_back({false, ":"});
                        p.insert(p.begin() + (long)i, w.begin(), w.end());
                        i += w.size();
                    }
                } else {
                    warn(line.num, "PLAY of a string that is not a literal: its V (voice), U (volume 0-15) and M (measure) "
                                   "are BASIC 7.0's, not BASIC 65's (one string per voice: PLAY v1$, v2$, v3$)");
                    Pieces w = gosub(helper("play123", {playWait({1, 2, 3}) + " return"}));
                    w.push_back({false, ":"});
                    p.insert(p.begin() + (long)i, w.begin(), w.end());
                    i += w.size();
                }
            } else if (k == "GRAPHIC") {
                if (i + 1 < e && p[i + 1].isKw && p[i + 1].text == "CLR") {
                    warn(line.num, "GRAPHIC CLR (BASIC 7.0: the bit map freed): BASIC 65's GRAPHIC CLR resets its graphics");
                    continue;
                }
                Args a = splitArgs(p, i + 1, e);
                std::string m = a.list.empty() ? "" : a.list[0];
                if (!isNumber(m)) {
                    warn(line.num, "GRAPHIC of an expression: left as written (BASIC 65 opens a screen with SCREEN 320, 200, 4)");
                    continue;
                }
                mode = std::stoi(m);
                if (mode == 0 || mode == 5) {
                    // the text screen: 40 columns (the VIC's) or 80 (the VDC's)
                    replacePieces(then(gosub(closeScreen()), gosub(columns(mode == 5 ? 80 : 40))));
                } else {
                    replacePieces(then(gosub(closeScreen()), {{false, "screen 320, 200, 4"}}));   // 16 colours, cleared
                    if (mode == 2 || mode == 4) warn(line.num, "GRAPHIC " + m + ": BASIC 65 has no split screen");
                    if (mode >= 3) warn(line.num, "GRAPHIC " + m + " (multicolour, x 0-159): the x coordinates are doubled");
                    if (a.list.size() > 1 && a.list[1] == "0") warn(line.num, "GRAPHIC ..., 0 (keep the bit map): SCREEN clears it");
                }
            } else if (k == "COLOR") {
                Args a = splitArgs(p, i + 1, e);
                if (a.list.size() != 2 && !(v35 && a.list.size() == 3)) {
                    warn(line.num, "COLOR with " + std::to_string(a.list.size()) + " arguments: left as written");
                    continue;
                }
                const std::string& s = a.list[0];
                std::string c = minus1(a.list[1]);
                if (v35) {
                    // the TED's colour and luminance: the nearest of the MEGA65's 16
                    std::string lum = a.list.size() == 3 ? a.list[2] : "";
                    if (isNumber(a.list[1]) && (lum.empty() || isNumber(lum))) {
                        int m = tedColour(std::stoi(a.list[1]), lum.empty() ? -1 : std::stoi(lum));
                        if (m >= 0) c = std::to_string(m);
                    } else {
                        c = "val(mid$(\"" + std::string(TED_COLOURS) + "\", (" + a.list[1] + ") * 2 - 1, 2))";
                        if (!lum.empty()) warn(line.num, "COLOR's luminance of an expression: the colour's own hue is used");
                    }
                }
                usedState = true;
                penNow.clear();
                if (s == "0") replace(CS + "(0) = " + c + " : background " + c);
                else if (s == "4") replace("border " + c);
                else if (s == "5") replace("foreground " + c);
                else if (s == "1" || s == "2" || s == "3") replace(CS + "(" + s + ") = " + c);
                else if (s == "6") { drop(); warn(line.num, "COLOR 6 (the 80-column background): dropped"); }
                else { replace(CS + "(" + s + ") = " + c); warn(line.num, "COLOR of a source expression: kept as " + CS + "(source)"); }
            } else if (k == "LOCATE") {
                Args a = splitArgs(p, i + 1, e);
                if (a.list.size() == 2) { replace(PX + " = " + times(a.list[0], mode >= 3 ? 2 : 1) + " : " + PY + " = " + a.list[1]); usedState = true; }
            } else if (k == "DRAW") {
                Args a = splitArgs(p, i + 1, e);
                // [source], x1, y1 [TO x2, y2] ...  or  [source] TO x, y
                std::string s = a.list.empty() ? "" : a.list[0];
                std::vector<std::string> pts;
                size_t j = 1;
                if (a.list.size() > 1 && a.afterTo[1]) { pts.push_back(PX); pts.push_back(PY); }   // from the pixel cursor
                for (; j < a.list.size(); j++)   // (x, y pairs: an x doubled in multicolour)
                    pts.push_back(pts.size() % 2 == 0 ? times(a.list[j], mode >= 3 ? 2 : 1) : a.list[j]);
                if (pts.size() < 2 || pts.size() % 2) { warn(line.num, "DRAW: arguments not understood, left as written"); continue; }
                std::string coords;
                for (size_t q = 0; q < pts.size(); q++) coords += (q ? ", " : "") + pts[q];
                replace(pen(s) + "line " + coords + " : " + PX + " = " + pts[pts.size() - 2] + " : " + PY + " = " + pts.back());
                usedState = true;
            } else if (k == "BOX") {
                // BOX [source], x1, y1 [, x2, y2] [, angle] [, paint]
                Args a = splitArgs(p, i + 1, e);
                auto at = [&](size_t q) { return q < a.list.size() ? a.list[q] : std::string(); };
                int f = mode >= 3 ? 2 : 1;
                std::string solid = at(6).empty() ? "0" : at(6);
                usedState = true;
                if (at(5).empty() || at(5) == "0") {
                    // not rotated: its four corners
                    std::string x1 = times(at(1), f), y1 = at(2);
                    std::string x2 = at(3).empty() ? PX : times(at(3), f), y2 = at(4).empty() ? PY : at(4);
                    replace(pen(at(0)) + "box " + x1 + ", " + y1 + ", " + x2 + ", " + y1 + ", " + x2 + ", " + y2 + ", " +
                            x1 + ", " + y2 + ", " + solid + " : " + PX + " = " + x2 + " : " + PY + " = " + y2);
                } else {
                    // rotated: BASIC 7.0's corners, by a subroutine
                    std::string un = f == 2 ? " / 2" : "";
                    std::string x2 = at(3).empty() ? "(" + PX + ")" + un : at(3), y2 = at(4).empty() ? PY : at(4);
                    uint16_t sub = boxSub();
                    std::string args = argName(0) + " = " + at(1) + " : " + argName(1) + " = " + at(2) + " : " + argName(2) + " = " +
                                       x2 + " : " + argName(3) + " = " + y2 + " : " + argName(4) + " = " + at(5) + " : " +
                                       argName(5) + " = " + solid + " : " + argName(6) + " = " + std::to_string(f);
                    std::string pn = pen(at(0));
                    if (pn.size() > 3) pn = pn.substr(0, pn.size() - 3);
                    Pieces np = pn.empty() ? Pieces{} : raw(pn);
                    if (!np.empty()) np.push_back({false, ":"});
                    Pieces tail = cat({raw(args), raw(":"), gosub(sub)});
                    np.insert(np.end(), tail.begin(), tail.end());
                    replacePieces(np);
                    penNow.clear();   // (a GOSUB: leave the pen to the next statement)
                }
            } else if (k == "CIRCLE") {
                // CIRCLE [source], x, y, xr [, yr] [, sa] [, ea] [, angle] [, inc]
                Args a = splitArgs(p, i + 1, e);
                auto at = [&](size_t q) { return q < a.list.size() ? a.list[q] : std::string(); };
                int f = mode >= 3 ? 2 : 1;
                usedState = true;
                // (an omitted y radius: the x radius, doubled in multicolour as BASIC 7.0 does)
                std::string yr = at(4).empty() ? times(at(3), f) : at(4);
                bool circle = at(4).empty() || at(4) == at(3);
                auto num = [&](size_t q, double dflt, double& v) {
                    if (at(q).empty()) { v = dflt; return true; }
                    if (!isNumber(at(q))) return false;
                    v = std::stod(at(q));
                    return true;
                };
                double sa, ea, ro, inc;
                bool literal = num(5, 0, sa) && num(6, 0, ea) && num(7, 0, ro) && num(8, 2, inc);
                if (literal && inc == 0) warn(line.num, "CIRCLE with an increment of 0: an error in BASIC 7.0");
                // the arc in BASIC 7.0's terms, a circle's rotation folded into it
                double S = sa, E = ea;
                if (literal && circle) { S += ro; E += ro; }
                if (literal && E <= S) E += 360;
                bool turned = literal && ro != 0 && !circle;
                std::string x = at(1).empty() ? PX : times(at(1), f), y = at(2).empty() ? PY : at(2);
                std::string xr = times(at(3), f);
                auto endCursor = [&](double end) {
                    // the pixel cursor at the arc's last point
                    double sb = std::sin(end * M_PI / 180), cb = std::cos(end * M_PI / 180);
                    auto term = [](const std::string& r, double k) {
                        if (std::fabs(k) < 1e-9) return std::string();
                        std::ostringstream o;
                        o << (k < 0 ? " - " : " + ") << r << " * " << std::fabs(k);
                        return o.str();
                    };
                    return " : " + PX + " = int(" + x + term(xr, sb) + " + .5) : " + PY + " = int(" + y + term(yr, -cb) + " + .5)";
                };
                if (literal && !turned && inc > 0 && inc <= 2 && E - S >= 360) {
                    // the whole ellipse, its default fine steps: ELLIPSE
                    replace(pen(at(0)) + "ellipse " + x + ", " + y + ", " + xr + ", " + yr + endCursor(E));
                } else if (literal && !turned && inc > 0 && std::fmod(360.0, inc) == 0 && std::fmod(E - S, inc) == 0 &&
                           E - S <= 360) {
                    // steps that divide the circle, an arc of whole steps: POLYGON, its vertices BASIC 7.0's
                    // (POLYGON counts anticlockwise from 3 o'clock, BASIC 7.0 clockwise from 12: t = 90 - b;
                    // its arguments, as b65.src reads them: x, y, xr, yr, sides, sides drawn, lines to
                    // the centre, start angle, solid — the MEGA65 Book's order is not the ROM's)
                    long n = std::lround(360.0 / inc), sides = std::lround((E - S) / inc);
                    long t0 = ((90 - std::lround(E)) % 360 + 360) % 360;
                    replace(pen(at(0)) + "polygon " + x + ", " + y + ", " + xr + ", " + yr + ", " + std::to_string(n) + ", " +
                            std::to_string(sides) + ", 0, " + std::to_string(t0) + ", 0" + endCursor(E));
                } else {
                    // anything else (a turned ellipse, an arc between steps, expressions): BASIC 7.0's own
                    // algorithm, by a subroutine
                    std::string un = f == 2 ? " / 2" : "";
                    std::string cx = at(1).empty() ? "(" + PX + ")" + un : at(1), cy = at(2).empty() ? PY : at(2);
                    std::string yr7 = at(4).empty() ? times(at(3), f) : at(4);
                    uint16_t sub = circleSub();
                    std::string args = argName(0) + " = " + cx + " : " + argName(1) + " = " + cy + " : " + argName(2) + " = " + at(3) +
                                       " : " + argName(3) + " = " + yr7 + " : " + argName(4) + " = " + (at(5).empty() ? "0" : at(5)) +
                                       " : " + argName(5) + " = " + (at(6).empty() ? "0" : at(6)) + " : " + argName(6) + " = " +
                                       (at(7).empty() ? "0" : at(7)) + " : " + argName(7) + " = " + (at(8).empty() ? "2" : at(8)) +
                                       " : " + argName(8) + " = " + std::to_string(f);
                    std::string pn = pen(at(0));
                    if (pn.size() > 3) pn = pn.substr(0, pn.size() - 3);
                    Pieces np = pn.empty() ? Pieces{} : raw(pn);
                    if (!np.empty()) np.push_back({false, ":"});
                    Pieces tail = cat({raw(args), raw(":"), gosub(sub)});
                    np.insert(np.end(), tail.begin(), tail.end());
                    replacePieces(np);
                    penNow.clear();
                }
            } else if (k == "PAINT") {
                Args a = splitArgs(p, i + 1, e);
                auto at = [&](size_t q) { return q < a.list.size() ? a.list[q] : std::string(); };
                std::string x = at(1).empty() ? PX : times(at(1), mode >= 3 ? 2 : 1), y = at(2).empty() ? PY : at(2);
                warn(line.num, "PAINT: BASIC 65 fills the area of (x, y)'s colour; BASIC 7.0 fills up to the source's colour (mode 0) "
                               "or any non-background (mode 1): check");
                replace("paint " + x + ", " + y + ", 0, " + src(at(0)));
                usedState = true;
            } else if (k == "CHAR") {
                Args a = splitArgs(p, i + 1, e);
                auto at = [&](size_t q) { return q < a.list.size() ? a.list[q] : std::string(); };
                std::string str = at(3).empty() ? "\"\"" : at(3);
                if (!at(4).empty() && at(4) != "0") str = "chr$(18) + " + str;
                if (mode == 0 || mode == 5)
                    replace("cursor " + at(1) + ", " + at(2) + " : print " + str + ";");
                else {
                    replace(pen(at(0)) + "char " + at(1) + ", " + times(at(2), 8) + ", 1, 1, 2, " + str);
                    usedState = true;
                }
            } else if (v35 && k == "SOUND") {
                // BASIC 3.5: SOUND voice (1, 2 tones; 3 noise, on voice 2), TED frequency register (Hz =
                // K / (1024 - f): K 110840.4375 PAL, 111860.781 NTSC), duration (frames); it waits while the
                // voice still sounds. BASIC 65: the SID's frequency (its clock: b65.src's note tables,
                // 16.519 counts per Hz on PAL, 16.404 on NTSC), duration in frames, no waiting
                Args a = splitArgs(p, i + 1, e);
                if (a.list.size() != 3) { warn(line.num, "SOUND: arguments not understood, left as written"); continue; }
                const double K = o.ntsc ? 111860.781 : 110840.4375;
                std::string v = a.list[0], f = a.list[1], d = a.list[2];
                bool noise = v == "3";
                std::string voice = noise ? "2" : isNumber(v) ? v : "(" + v + " + (" + v + " = 3))";   // (true is -1)
                if (!isNumber(v)) warn(line.num, "SOUND of a voice expression: voice 3 (noise) is played as a tone on voice 2");
                std::string hz;
                if (isNumber(f)) {
                    double x = std::stod(f);
                    double h = x < 1024 ? K / (1024 - x) : 0;
                    if (h * 16.52 > 65535) warn(line.num, "SOUND frequency " + f + " (" + std::to_string((long)h) + " Hz) is above the SID's range");
                    char b[32];
                    std::snprintf(b, sizeof b, "%.6g", h);
                    hz = b;
                } else {
                    char b[32];
                    std::snprintf(b, sizeof b, "%.10g", K);
                    hz = std::string(b) + " / (1024 - (" + f + "))";
                }
                if (o.ntsc) d = isNumber(d) ? std::to_string(std::lround(std::stod(d) * 5 / 6)) : "int((" + d + ") * 5 / 6)";
                if (soundFactor.empty()) soundFactor = fresh();
                const std::string& sk = soundFactor;
                replace("do while peek(" + std::string(isNumber(voice) ? std::to_string(4803 + std::stoi(voice)) : "4803 + " + voice) +
                        ") < 128 : loop : sound " + voice + ", int(" + hz + " * " + sk + "), " + d + (noise ? ", 0, 0, 0, 3" : ""));
            } else if (v35 && k == "VOL") {
                // BASIC 3.5's volume 0-8, BASIC 65's 0-15
                Args a = splitArgs(p, i + 1, e);
                if (a.list.size() == 1)
                    replace("vol " + (isNumber(a.list[0]) ? std::to_string(std::lround(std::stod(a.list[0]) * 15 / 8))
                                                           : "int((" + a.list[0] + ") * 15 / 8 + .5)"));
            } else if (k == "SLOW") {
                replace("speed 1");   // (BASIC 65's SPEED alone is 40 MHz)
            } else if (k == "SPRITE" || k == "MOVSPR" || k == "SPRSAV") {
                // sprites 1-8 -> 0-7; SPRITE's colour 1-16 -> 0-15
                Args a = splitArgs(p, i + 1, e);
                if (a.list.empty() || (k == "SPRITE" && a.list[0] == "clr")) continue;
                std::string t = lower(k) + " ";
                for (size_t q = 0; q < a.list.size(); q++) {
                    std::string v = a.list[q];
                    bool str = v.find('$') != std::string::npos || (!v.empty() && v[0] == '"');
                    if (q == 0 || (k == "SPRSAV" && q == 1)) { if (!str) v = minus1(v); }
                    else if (k == "SPRITE" && q == 2 && !v.empty()) v = minus1(v);
                    t += (q ? ", " : "") + v;
                }
                if (k == "MOVSPR" && t.find(';') != std::string::npos)
                    warn(line.num, "MOVSPR n, distance;angle: BASIC 65 moves by +x, +y or angle#speed");
                // (MOVSPR's TO form keeps its pieces: the TO keyword is in the arguments)
                if (k == "MOVSPR" && joinPieces(p, i + 1, e).find(" to ") != std::string::npos) {
                    if (!isNumber(a.list[0])) { warn(line.num, "MOVSPR with TO: its sprite number is BASIC 7.0's (1-8)"); continue; }
                    p[i + 1].text = minus1(p[i + 1].text);
                    continue;
                }
                replace(t);
            } else if (k == "SPRCOLOR") {
                Args a = splitArgs(p, i + 1, e);
                std::string t = "sprcolor ";
                for (size_t q = 0; q < a.list.size(); q++) t += (q ? ", " : "") + (a.list[q].empty() ? "" : minus1(a.list[q]));
                replace(t);
            } else if (k == "SCNCLR") {
                // BASIC 7.0: SCNCLR mode (0/5 text, 1-4 the bit map); BASIC 65: SCNCLR [colour] (the graphics screen)
                Args a = splitArgs(p, i + 1, e);
                if (a.list.empty() || a.list[0].empty()) {
                    // BASIC 3.5's SCNCLR clears the screen in use: the bit map when one is open
                    if (v35 && mode >= 1 && mode <= 4) { replace("scnclr " + CS + "(0)"); usedState = true; }
                    continue;
                }
                const std::string& m = a.list[0];
                if (m == "0" || m == "5") replace("scnclr");
                else if (isNumber(m) && mode >= 1 && mode <= 4) { replace("scnclr " + CS + "(0)"); usedState = true; }
                else if (isNumber(m)) { drop(); warn(line.num, "SCNCLR " + m + " (the bit map) with no graphics screen open: dropped"); }
                else warn(line.num, "SCNCLR of an expression: BASIC 7.0's argument is a screen mode, BASIC 65's a colour");
            } else if (k == "FETCH" || k == "STASH" || k == "SWAP") {
                // FETCH / STASH / SWAP bytes, address, REU bank, REU address: the REU's 64 KB banks as attic RAM
                Args a = splitArgs(p, i + 1, e);
                if (a.list.size() != 4) { warn(line.num, k + ": arguments not understood, left as written"); continue; }
                std::string reu = isNumber(a.list[2]) && isNumber(a.list[3])
                    ? std::to_string(0x8000000L + std::stol(a.list[2]) * 65536L + std::stol(a.list[3]))
                    : "134217728 + (" + a.list[2] + ") * 65536 + (" + a.list[3] + ")";
                std::string n = a.list[0], c = a.list[1];
                if (k == "STASH") replace("edma 0, " + n + ", " + c + ", " + reu);
                else if (k == "FETCH") replace("edma 0, " + n + ", " + reu + ", " + c);
                else replace("edma 2, " + n + ", " + c + ", " + reu);
                warn(line.num, k + " -> EDMA: the RAM expansion's banks are attic RAM ($8000000 + bank * 64 KB); "
                               "the C128 address is taken as bank 0's");
            }
        }
    }
    if (!simons) notes.push_back(std::string(v35 ? "converted from BASIC 3.5 (C16 / Plus/4) by basic45 --convert=basic35 ("
                                    : "converted from BASIC 7.0 (C128) by basic45 --convert=basic7 (") +
                    std::string(o.ntsc ? "NTSC" : "PAL") + " timing): check the warnings");

    // a first line: BASIC 7.0's defaults the program relies on (and its 40-column screen)
    Pieces start = gosub(columns(40));
    std::string prologue;
    if (anyPlay && !anyTempo) {
        long m = std::lround(8 * factor);   // BASIC 7.0's default tempo is 8, BASIC 65's 12
        prologue = "tempo " + std::to_string(m);
        notes.push_back("TEMPO " + std::to_string(m) + ": BASIC 7.0's default tempo (8) in BASIC 65's terms");
    }
    if (csNamed) usedState = true;
    if (graphics && usedState) {
        // the colour sources (BASIC 7.0's defaults less one: background 12 dark grey, foreground 2 white,
        // multicolour 3 red and 6 green) and the pixel cursor
        // (the Plus/4's: background white, foreground black)
        // (the C64's: background blue, foreground light blue)
        std::string g = "dim " + CS + "(6) : " + CS + "(0) = " + (v35 ? "1" : simons ? "6" : "11") + " : " + CS + "(1) = " +
                        (v35 ? "0" : simons ? "14" : "1") +
                        " : " + CS + "(2) = 2 : " + CS + "(3) = 5 : " + PX + " = 0 : " + PY + " = 0";
        prologue += (prologue.empty() ? "" : " : ") + g;
        notes.push_back(CS + "(): BASIC 7.0's colour sources (0-6, colours 0-15), " + PX + ", " + PY + ": its pixel cursor");
    }
    if (!soundFactor.empty()) {
        // the SID's counts per Hz, by the MEGA65's video mode ($D06F bit 7: NTSC), as BASIC 65's note tables
        prologue += (prologue.empty() ? "" : " : ") + soundFactor + " = 16.519 : if peek(53359) and 128 then " + soundFactor + " = 16.404";
        notes.push_back(soundFactor + ": the SID's frequency counts per Hz (for SOUND)");
    }
    if (!prologue.empty()) { start.push_back({false, ":"}); start.push_back({false, prologue}); }
    if (!lines.empty()) {
        if (lines.front().num > 0) {
            lines.insert(lines.begin(), DecompiledLine{(uint16_t)(lines.front().num - 1), start});
        } else {
            auto& p = lines.front().pieces;
            start.push_back({false, ":"});
            p.insert(p.begin(), start.begin(), start.end());
        }
        notes.push_back("the first line: BASIC 7.0's 40-column screen" + std::string(prologue.empty() ? "" : " and its defaults"));
    }
    if (!helperLines.empty()) {
        lines.push_back({(uint16_t)(lastNum + 1), {{true, "END"}}});
        notes.push_back("after the END: subroutines the conversion added (the waits before PLAY, 40/80 columns, closing a screen)");
        lines.insert(lines.end(), helperLines.begin(), helperLines.end());
    }
    if (v35 || simons) {
        // the messages in the dialect's terms
        const char* dialect = v35 ? "BASIC 3.5" : "Simons' BASIC";
        const char* machine = v35 ? "Plus/4" : "C64";
        auto rename = [&](std::string& m) {
            for (auto [from, to] : {std::pair<const char*, const char*>{"BASIC 7.0", dialect}, {"C128", machine}})
                for (size_t q; (q = m.find(from)) != std::string::npos;) m.replace(q, std::strlen(from), to);
        };
        for (size_t q = warnings0; q < warnings.size(); q++) rename(warnings[q]);
        for (size_t q = notes0; q < notes.size(); q++) rename(notes[q]);
    }
}

// ---- Simons' BASIC ------------------------------------------------------------
//
// Simons' BASIC (C64; tokens $64 xx): its keywords converted to BASIC 65's where
// one exists (checked against the TSB sources and the C64-Wiki's pages), the
// rest left as written with a warning. Its graphics are rewritten as BASIC 7.0's
// statements (a plot type as a colour source: 0 paper / background, 1 ink or
// multicolour 1, 2 / 3 multicolour 2 / 3; invert as XOR drawing, source 5;
// MULTI's x doubled), which convertV7 then converts as for a C128 program.

// Simons' keywords with no BASIC 65 counterpart, and why
const std::map<std::string, std::string>& simonsMissing() {
    static const std::map<std::string, std::string> m = {
        {"LOCAL", "BASIC 65 has no local variables"}, {"GLOBAL", "BASIC 65 has no local variables"},
        {"ROT", "a shape string (DRAW / ROT)"}, {"DRAW", "a shape string (DRAW / ROT)"},
        {"UPB", "a window scroll"}, {"UPW", "a window scroll"}, {"DOWNB", "a window scroll"}, {"DOWNW", "a window scroll"},
        {"LEFTB", "a window scroll"}, {"LEFTW", "a window scroll"}, {"RIGHTB", "a window scroll"}, {"RIGHTW", "a window scroll"},
        {"FLASH", "the C64's colour flashing"}, {"BFLASH", "the C64's border flashing"}, {"OFF", "the C64's colour flashing"},
        {"DUP", "a string repeated (no BASIC 65 function)"}, {"FCHR", "a text-screen rectangle"}, {"FCOL", "a text-screen rectangle"},
        {"FILL", "a text-screen rectangle"}, {"MOVE", "a text-screen rectangle"}, {"INV", "a text-screen rectangle"},
        {"BCKGNDS", "the VIC-II's extended background colours"}, {"DESIGN", "@ rows of a sprite or character"},
        {"DETECT", "the collision registers (BASIC 65: BUMP / COLLISION)"}, {"CHECK", "the collision registers (BASIC 65: BUMP / COLLISION)"},
        {"CHAR", "a glyph by its screen code"}, {"SCRSV", "the text screen saved in its own format"},
        {"SCRLD", "the text screen loaded in its own format"}, {"ON KEY", "keys checked between statements"},
        {"DISABLE", "keys checked between statements (ON KEY)"}, {"RESUME", "RESUME after ON KEY"},
        {"MUSIC", "a music string in Simons' notation (BASIC 65: PLAY's)"}, {"PLAY", "a music string in Simons' notation (BASIC 65: PLAY's)"},
        {"WAVE", "the SID's control register (BASIC 65: ENVELOPE / PLAY)"},
        {"ENVELOPE", "a voice's ADSR for MUSIC (BASIC 65's ENVELOPE sets PLAY's tunes)"},
        {"FETCH", "a filtered input"}, {"INKEY", "the function key held"}, {"OUT", "the error message"},
        {"OLD", "a direct-mode command"}, {"COLD", "a direct-mode command"}, {"SECURE", "a direct-mode command"},
        {"DISAPA", "lines hidden from LIST"}, {"DUMP", "a direct-mode command"}, {"MEM", "the C64's memory map"},
        {"HRDCPY", "the MPS 801 printer"}, {"COPY", "the MPS 801 printer"}, {"OPTION", "LIST's display"},
        {"PAGE", "LIST's display"}, {"DELAY", "LIST's speed"},
    };
    return m;
}

Pieces cat3(Pieces a, const Pieces& b) { a.insert(a.end(), b.begin(), b.end()); return a; }

void convertSimons(std::vector<DecompiledLine>& lines, const BasicDecompileOptions& o, std::vector<std::string>& warnings,
                   std::vector<std::string>& notes) {
    (void)o;
    auto warn = [&](uint16_t n, const std::string& m) { warnings.push_back("line " + std::to_string(n) + ": " + m); };
    mergeVariables(lines, warnings, notes, "C64");
    const std::string RAD = ".0174533";

    // %bbbbbbbb and $hhhh literals may have spaces in them: one piece (BASIC 65 reads both)
    for (auto& l : lines) {
        auto& p = l.pieces;
        for (size_t i = 0; i + 1 < p.size(); i++) {
            if (p[i].isKw || p[i].text.empty() || (p[i].text[0] != '%' && p[i].text[0] != '$')) continue;
            bool bin = p[i].text[0] == '%';
            auto digits = [&](const std::string& t) {
                if (t.empty()) return false;
                for (char c : t)
                    if (bin ? (c != '0' && c != '1') : !std::isxdigit((unsigned char)c)) return false;
                return true;
            };
            while (i + 1 < p.size() && !p[i + 1].isKw && p[i].text.size() < (bin ? 9u : 5u) && digits(p[i + 1].text)) {
                p[i].text += p[i + 1].text;
                p.erase(p.begin() + (long)i + 1);
            }
        }
    }

    // names for the conversion's own variables (two letters count; not the program's)
    std::set<std::string> used;
    for (const auto& l : lines)
        for (const auto& pc : l.pieces)
            if (!pc.isKw && !pc.text.empty() && std::isalpha((unsigned char)pc.text[0]))
                used.insert(lower(pc.text.substr(0, std::min<size_t>(2, pc.text.size()))));
    auto fresh = [&]() {
        for (char a : std::string("yxwv"))
            for (char b : std::string("abcdefghijklmnopqrstuvwxyz0123456789")) {
                std::string n{a, b};
                if (!used.count(n)) { used.insert(n); return n; }
            }
        return std::string("yy");
    };
    std::map<std::string, std::string> names;
    auto var = [&](const std::string& key) {
        auto it = names.find(key);
        if (it != names.end()) return it->second;
        return names[key] = fresh();
    };

    // lines the conversion adds after an END at the program's end (handlers, the CGOTO table)
    const uint16_t last = lines.empty() ? 0 : lines.back().num;
    std::vector<DecompiledLine> extra;
    bool anyExtra = false;
    // (numbers only name the lines here: basic45 numbers the result; above 63999 is fine)
    uint32_t nextNum = (uint32_t)last + 2;
    // (after an END, appended when the line being converted is done: the pass converts them too)
    auto addLine = [&](const Pieces& pc) {
        if (extra.empty() && !anyExtra) extra.push_back({(uint16_t)(last + 1), {{true, "END"}}});
        anyExtra = true;
        if (nextNum > 65535) { warnings.push_back("too many lines added after line " + std::to_string(last)); nextNum = 65535; }
        uint16_t n = (uint16_t)nextNum++;
        extra.push_back({n, pc});
        return n;
    };
    auto flushExtra = [&]() {
        lines.insert(lines.end(), extra.begin(), extra.end());
        extra.clear();
    };

    // a procedure's name (PROC / CALL / EXEC name: the rest of the line, keywords crunched in
    // it too, compared as Simons' does) as a label: proc_<letters, digits and _>
    auto procName = [](const Pieces& p, size_t from, size_t to) {
        std::string n;
        for (size_t j = from; j < to; j++)
            for (char c : p[j].text) n += std::isalnum((unsigned char)c) ? (char)std::tolower((unsigned char)c) : '_';
        return "proc_" + n;
    };
    std::map<std::string, uint16_t> procs;
    bool rcomp = false;
    for (auto& line : lines) {
        auto& p = line.pieces;
        // @ rows (DESIGN's) are not statements: kept as REM text
        if (!p.empty() && !p[0].isKw && !p[0].text.empty() && p[0].text[0] == '@') {
            std::string t = joinPieces(p, 0, p.size());
            p = {{true, "REM"}, {false, t}};
            continue;
        }
        for (size_t i = 0; i < p.size(); i++) {
            if (p[i].isKw && p[i].text == "REM") break;
            if (p[i].isKw && p[i].text == "RCOMP") rcomp = true;
            if (!(p[i].isKw && p[i].text == "PROC")) continue;
            // PROC name: the rest of the line is the name
            std::string n = procName(p, i + 1, p.size());
            if (procs.count(n)) warn(line.num, "PROC " + n.substr(5) + " is defined twice: the first is used");
            else procs[n] = line.num;
            if (line.label.empty()) line.label = n;
            size_t from = i;
            if (from > 0 && p[from - 1].text == ":") from--;
            p.erase(p.begin() + (long)from, p.end());
            break;
        }
    }

    // LIN (the cursor's row) is read by RCURSOR first, in front of its statement
    for (auto& line : lines) {
        auto& p = line.pieces;
        size_t stmt = 0;
        bool need = false;
        for (size_t i = 0; i < p.size(); i++) {
            if (p[i].isKw && p[i].text == "REM") break;
            if ((!p[i].isKw && p[i].text == ":") || (p[i].isKw && (p[i].text == "THEN" || p[i].text == "ELSE"))) {
                stmt = i + 1;
                need = false;
                continue;
            }
            if (p[i].isKw && p[i].text == "LIN") {
                p[i] = {false, var("lin")};
                if (!need) {
                    p.insert(p.begin() + (long)stmt, {{false, "rcursor , " + var("lin")}, {false, ":"}});
                    i += 2;
                    need = true;
                }
            }
        }
    }

    bool multi = false;     // MULTI's mode (in program order): x 0-159
    bool anyXorWarn = false;
    const std::string G0 = "@@g0", G1 = "@@g1", G2 = "@@g2", G3 = "@@g3";   // (named below)
    for (size_t li = 0; li < lines.size(); li++) {
        auto& line = lines[li];
        auto& p = line.pieces;
        bool data = false;
        size_t split = 0;   // the statements from here on go to a line of their own (after an IF made here)
        for (size_t i = 0; i < p.size() && !split; i++) {
            if (p[i].isKw && p[i].text == "DATA") data = true;
            if (!p[i].isKw && p[i].text == ":") data = false;
            if (!data && !p[i].isKw && (i == 0 || p[i - 1].text == ":" || (p[i - 1].isKw && (p[i - 1].text == "THEN" || p[i - 1].text == "ELSE"))) &&
                statementEnd(p, i) == i + 1 && procs.count(procName(p, i, i + 1))) {
                // TSB: a procedure's name alone is EXEC name
                warn(line.num, p[i].text + ": a PROC's name alone (TSB's EXEC) -> GOSUB");
                std::string at = std::to_string(procs[procName(p, i, i + 1)]);
                p[i] = {true, "GOSUB"};
                p.insert(p.begin() + (long)i + 1, {false, at});
                i++;
                continue;
            }
            if (data || !p[i].isKw) continue;
            std::string k = p[i].text;
            if (k == "REM") break;
            size_t e = statementEnd(p, i + 1);
            auto replace = [&](const std::string& text) {
                p.erase(p.begin() + (long)i + 1, p.begin() + (long)e);
                p[i] = {false, text};
            };
            auto replacePieces = [&](Pieces np) {
                p.erase(p.begin() + (long)i, p.begin() + (long)e);
                p.insert(p.begin() + (long)i, np.begin(), np.end());
                i += np.size() - 1;
            };
            auto drop = [&]() {
                size_t from = i, to = e;
                if (from > 0 && p[from - 1].text == ":") from--;
                else if (to < p.size() && p[to].text == ":") to++;
                p.erase(p.begin() + (long)from, p.begin() + (long)to);
                i = from - 1;
            };
            // a keyword left as written: plain text from here on (not BASIC 7.0's of that name)
            auto leave = [&](const std::string& why) {
                warn(line.num, k + " has no BASIC 65 counterpart (" + why + "): left as written");
                p[i] = {false, lower(k)};
            };
            auto fnArgs = [&](size_t& close) -> Args {
                close = i;
                if (i + 1 >= p.size() || p[i + 1].text != "(") return {};
                int depth = 0;
                for (size_t j = i + 1; j < p.size(); j++) {
                    if (p[j].text == "(" || (p[j].text.size() > 1 && p[j].text.back() == '(')) depth++;
                    if (p[j].text == ")" && --depth == 0) { close = j; return splitArgs(p, i + 2, j); }
                }
                return {};
            };
            auto replaceFn = [&](size_t close, const std::string& text) {
                p.erase(p.begin() + (long)i + 1, p.begin() + (long)close + 1);
                p[i] = {false, text};
            };
            Args a = splitArgs(p, i + 1, e);
            auto at = [&](size_t q) { return q < a.list.size() ? a.list[q] : std::string(); };
            auto argc = [&](size_t n) {
                if (a.list.size() == n && !(n == 1 && a.list[0].empty())) return true;
                warn(line.num, k + ": " + std::to_string(n) + " arguments expected, left as written");
                p[i] = {false, lower(k)};
                return false;
            };
            // BASIC 7.0's statement kw with these arguments (x, y pairs after TO)
            auto v7 = [](const std::string& kw, const std::vector<std::string>& args, int toAt = -1) {
                Pieces r{{true, kw}};
                for (size_t q = 0; q < args.size(); q++) {
                    if ((int)q == toAt) r.push_back({true, "TO"});
                    else if (q) r.push_back({false, ","});
                    r.push_back({false, args[q]});
                }
                return r;
            };
            auto then = [](Pieces x, const Pieces& y) { x.push_back({false, ":"}); x.insert(x.end(), y.begin(), y.end()); return x; };
            // a plot type as the colour source, its invert (2; MULTI's 4) by XOR drawing (source 5)
            auto draw = [&](const std::string& f, Pieces stmt) {
                std::string inv = multi ? "4" : "2";
                if (f == inv) {
                    stmt[1].text = "5";
                    return then(then({{false, "dmode 0, 1, 0, 0, 1"}}, stmt), {{false, "dmode 0, 0, 0, 0, 1"}});
                }
                if (!isNumber(f) && !anyXorWarn) {
                    warn(line.num, "a plot type of an expression: its invert (" + inv + ") is drawn as a colour, not inverted");
                    anyXorWarn = true;
                }
                return stmt;
            };
            auto xs = [&](const std::string& x) { return x; };   // (BASIC 7.0's conversion doubles MULTI's x)
            // ---- program structure
            if (k == "PRINT") {
                // PRINT AT(column, row) items ... -> CURSOR column, row : PRINT items (each AT)
                std::vector<size_t> ats;
                int depth = 0;
                for (size_t j = i + 1; j < e; j++) {
                    if (p[j].isKw && p[j].text == "AT(" && depth == 0) ats.push_back(j);
                    if (p[j].text == "(" || (p[j].text.size() > 1 && p[j].text.back() == '(')) depth++;
                    if (p[j].text == ")") depth--;
                }
                if (ats.empty()) continue;
                Pieces out;
                auto items = [&](size_t from, size_t to, bool lastSeg) {
                    if (from >= to) return;
                    if (!out.empty()) out.push_back({false, ":"});
                    out.push_back({true, "PRINT"});
                    out.insert(out.end(), p.begin() + (long)from, p.begin() + (long)to);
                    if (!lastSeg && p[to - 1].text != ";") out.push_back({false, ";"});
                };
                items(i + 1, ats[0], false);
                bool ok = true;
                for (size_t q = 0; q < ats.size() && ok; q++) {
                    size_t j = ats[q], close = j;
                    int d = 1;
                    for (size_t m = j + 1; m < e; m++) {
                        if (p[m].text == "(" || (p[m].text.size() > 1 && p[m].text.back() == '(')) d++;
                        if (p[m].text == ")" && --d == 0) { close = m; break; }
                    }
                    Args c = splitArgs(p, j + 1, close);
                    if (close == j || c.list.size() != 2) { ok = false; break; }
                    if (!out.empty()) out.push_back({false, ":"});
                    out.push_back({false, "cursor " + c.list[0] + ", " + c.list[1]});
                    size_t next = q + 1 < ats.size() ? ats[q + 1] : e;
                    items(close + 1, next, q + 1 == ats.size());
                }
                if (!ok) { warn(line.num, "PRINT AT(: arguments not understood, left as written"); continue; }
                replacePieces(out);
            } else if (k == "END PROC") replace("return");
            else if (k == "CALL" || k == "EXEC") {
                // the name: the rest of the line (Simons'), or up to a ':' (TSB)
                std::string n = procName(p, i + 1, p.size());
                auto it = procs.find(n);
                if (it != procs.end()) {
                    p.erase(p.begin() + (long)i, p.end());
                    p.push_back({true, k == "CALL" ? "GOTO" : "GOSUB"});
                    p.push_back({false, std::to_string(it->second)});
                    break;
                }
                it = procs.find(procName(p, i + 1, e));
                if (it == procs.end()) { warn(line.num, k + " " + n.substr(5) + ": no PROC of that name, left as written"); p[i] = {false, lower(k)}; continue; }
                replacePieces({{true, k == "CALL" ? "GOTO" : "GOSUB"}, {false, std::to_string(it->second)}});
            } else if (k == "REPEAT") replace("do");
            else if (k == "UNTIL") replace("loop until " + joinPieces(p, i + 1, e));
            else if (k == "LOOP") replace("do");
            else if (k == "END LOOP") replace("loop");
            else if (k == "EXIT") {
                // EXIT IF c: out of the LOOP; statements after it on the line get a line of their own
                if (i + 1 < e && p[i + 1].isKw && p[i + 1].text == "IF") {
                    replace("if " + joinPieces(p, i + 2, e) + " then exit");
                    if (i + 2 < p.size() && p[i + 1].text == ":") split = i + 2;
                } else {
                    replace("exit");
                }
            } else if (k == "ELSE") {
                // Simons' IF c THEN a : ELSE : b -> BASIC 65's IF c THEN a : ELSE b
                if (i + 1 < p.size() && p[i + 1].text == ":") p.erase(p.begin() + (long)i + 1);
            } else if (k == "RCOMP") {
                replacePieces(cat3({{true, "IF"}, {false, var("rcomp")}, {true, "THEN"}}, Pieces(p.begin() + (long)i + 1, p.begin() + (long)e)));
            } else if (k == "CGOTO") {
                std::string x = joinPieces(p, i + 1, e);
                if (isNumber(x)) { replacePieces({{true, "GOTO"}, {false, x}}); continue; }
                // BASIC 65's GOTO takes a number: a table of the program's lines, once
                if (!names.count("cgoto")) {
                    std::string cg = var("cgoto");
                    names["cgotoAt"] = std::to_string(nextNum);
                    std::vector<uint16_t> nums;
                    for (const auto& l : lines)
                        if (!l.cont && l.num <= last) nums.push_back(l.num);
                    // (the program's lines, not those added)
                    for (uint16_t n : nums) addLine({{true, "IF"}, {false, cg + " = " + std::to_string(n)}, {true, "THEN"}, {false, std::to_string(n)}});
                    addLine({{false, "print \"?undef'd statement in cgoto\" : stop"}});
                    notes.push_back("CGOTO: a table of the program's lines after the END (BASIC 65's GOTO takes a number)");
                }
                replacePieces({{false, var("cgoto") + " = " + x}, {false, ":"}, {true, "GOTO"}, {false, names["cgotoAt"]}});
            } else if (k == "ON ERROR") {
                // the handler is the rest of the line: TRAP to it
                size_t r = i + 1;
                if (r < p.size() && p[r].text == ":") r++;
                Pieces rest(p.begin() + (long)r, p.end());
                p.erase(p.begin() + (long)i, p.end());
                if (rest.size() == 2 && rest[0].isKw && rest[0].text == "GOTO") {
                    p.push_back({true, "TRAP"});
                    p.push_back(rest[1]);
                } else {
                    if (rest.empty()) rest = {{true, "END"}};
                    p.push_back({true, "TRAP"});
                    p.push_back({false, std::to_string(addLine(rest))});
                }
                break;
            } else if (k == "NO ERROR") replace("trap");
            else if (k == "ERR") {
                // ERRN / ERRLN: ERR and the letters
                if (i + 1 < p.size() && !p[i + 1].isKw && p[i + 1].text.rfind("ln", 0) == 0) { p[i + 1].text = "el" + p[i + 1].text.substr(2); p.erase(p.begin() + (long)i); }
                else if (i + 1 < p.size() && !p[i + 1].isKw && p[i + 1].text.rfind("n", 0) == 0) { p[i + 1].text = "er" + p[i + 1].text.substr(1); p.erase(p.begin() + (long)i); }
                else leave("ERR");
            } else if (k == "TRACE") {
                if (at(0) == "10") replace("tron");
                else if (isNumber(at(0))) replace("troff");
                else { warn(line.num, "TRACE of an expression (10: on): TRON"); replace("tron"); }
            } else if (k == "RETRACE") {
                warn(line.num, "RETRACE (shows the traced lines again): dropped");
                drop();
            } else if (k == "RESET") replacePieces({{true, "RESTORE"}, {false, joinPieces(p, i + 1, e)}});
            // ---- functions

            else if (k == "PENX") p[i] = {false, "lpen(0)"};
            else if (k == "PENY") p[i] = {false, "lpen(1)"};
            else if (k == "JOY") p[i] = {false, "joy(2)"};
            else if (k == "SOUND") p[i] = {false, "54272"};
            else if (k == "GRAPHICS") p[i] = {false, "53248"};
            else if (k == "EXOR" || k == "MOD" || k == "DIV" || k == "FRAC" || k == "INST" || k == "INSERT" || k == "PLACE" || k == "POT" || k == "TEST") {
                size_t close;
                Args f = fnArgs(close);
                auto fa = [&](size_t q) { return q < f.list.size() ? f.list[q] : std::string(); };
                std::string r;
                // (BASIC 65's XOR is an operator: its XOR function is LOG10 now)
                if (k == "EXOR" && f.list.size() == 2) r = "((" + fa(0) + ") xor (" + fa(1) + "))";
                else if (k == "MOD" && f.list.size() == 2) r = "mod(int(" + fa(0) + "), int(" + fa(1) + "))";
                else if (k == "DIV" && f.list.size() == 2) r = "int(int(" + fa(0) + ") / int(" + fa(1) + "))";
                else if (k == "FRAC" && f.list.size() == 1) r = "(sgn(" + fa(0) + ") * (abs(" + fa(0) + ") - int(abs(" + fa(0) + "))))";
                // INST(new$, old$, pos) overwrites, INSERT inserts, at a 0-based position; PLACE(find$, in$) finds
                else if (k == "INST" && f.list.size() == 3) r = "(left$(" + fa(1) + ", " + fa(2) + ") + " + fa(0) + " + mid$(" + fa(1) + ", (" + fa(2) + ") + len(" + fa(0) + ") + 1))";
                else if (k == "INSERT" && f.list.size() == 3) r = "(left$(" + fa(1) + ", " + fa(2) + ") + " + fa(0) + " + mid$(" + fa(1) + ", (" + fa(2) + ") + 1))";
                else if (k == "PLACE" && f.list.size() == 2) r = "instr(" + fa(1) + ", " + fa(0) + ")";
                else if (k == "POT" && f.list.size() == 1) r = fa(0) == "0" ? "pot(1)" : isNumber(fa(0)) ? "pot(2)" : "pot(1 - ((" + fa(0) + ") <> 0))";
                else if (k == "TEST" && f.list.size() == 2) {
                    // the pixel's plot type: hires 1 set / 0 clear, MULTI its colour 0-3
                    std::string px = "pixel(" + (multi ? times(fa(0), 2) : fa(0)) + ", " + fa(1) + ")";
                    r = multi ? "(-(" + px + " = @@cs(1)) - 2 * (" + px + " = @@cs(2)) - 3 * (" + px + " = @@cs(3)))"
                              : "(-(" + px + " <> @@cs(0)))";
                }
                if (r.empty()) { warn(line.num, k + ": arguments not understood, left as written"); p[i] = {false, lower(k)}; }
                else replaceFn(close, r);
            }
            // ---- graphics
            else if (k == "HIRES") {
                if (!argc(2)) continue;
                multi = false;
                replacePieces(then(v7("GRAPHIC", {"1"}), {{false, G0 + " = " + at(1) + " : " + G1 + " = " + at(0) + " : @@cs(0) = " + G0 +
                                                                   " : @@cs(1) = " + G1 + " : @@cs(5) = (" + G0 + " xor " + G1 + ") : scnclr " + G0}}));
            } else if (k == "MULTI") {
                if (!argc(3)) continue;
                multi = true;
                replacePieces(then(v7("__MODE", {"3"}),
                                   {{false, G0 + " = rcolor(0) : " + G1 + " = " + at(0) + " : " + G2 + " = " + at(1) + " : " + G3 + " = (" + at(2) +
                                                ") and 15 : @@cs(0) = " + G0 + " : @@cs(1) = " + G1 + " : @@cs(2) = " + G2 + " : @@cs(3) = " + G3 +
                                                " : @@cs(5) = (" + G1 + " xor " + G2 + ")"}}));
                warn(line.num, "MULTI: from here on x is 0-159 (doubled); its invert (4) is drawn as XOR of colours 1 and 2");
            } else if (k == "LOW COL") {
                if (!argc(3)) continue;
                replace(multi ? "@@cs(1) = " + at(0) + " : @@cs(2) = " + at(1) + " : @@cs(3) = (" + at(2) + ") and 15 : @@cs(5) = (@@cs(1) xor @@cs(2))"
                              : "@@cs(1) = " + at(0) + " : @@cs(0) = " + at(1) + " : @@cs(5) = (@@cs(1) xor @@cs(0))");
            } else if (k == "HI COL") {
                replace(multi ? "@@cs(0) = " + G0 + " : @@cs(1) = " + G1 + " : @@cs(2) = " + G2 + " : @@cs(3) = " + G3 + " : @@cs(5) = (" + G1 + " xor " + G2 + ")"
                              : "@@cs(0) = " + G0 + " : @@cs(1) = " + G1 + " : @@cs(5) = (" + G0 + " xor " + G1 + ")");
            } else if (k == "COLOUR") {
                // COLOUR border, background (TSB: [, the text's colour])
                std::string fg = a.list.size() == 3 ? " : foreground " + at(2) : "";
                if (a.list.size() == 3) a.list.resize(2);
                if (!argc(2)) continue;
                replace("border " + at(0) + " : background " + at(1) + (multi ? " : " + G0 + " = " + at(1) + " : @@cs(0) = " + G0 : std::string()) + fg);
            } else if (k == "PLOT") {
                if (!argc(3)) continue;
                replacePieces(draw(at(2), v7("DRAW", {at(2), xs(at(0)), at(1)})));
            } else if (k == "LINE") {
                if (!argc(5)) continue;
                replacePieces(draw(at(4), v7("DRAW", {at(4), at(0), at(1), at(2), at(3)}, 3)));
            } else if (k == "REC") {
                if (!argc(5)) continue;
                replacePieces(draw(at(4), v7("BOX", {at(4), at(0), at(1), "(" + at(0) + ") + (" + at(2) + ")", "(" + at(1) + ") + (" + at(3) + ")"})));
            } else if (k == "BLOCK") {
                if (!argc(5)) continue;
                replacePieces(draw(at(4), v7("BOX", {at(4), at(0), at(1), at(2), at(3), "0", "1"})));
            } else if (k == "CIRCLE") {
                // an ellipse: a 30-sided polygon (ARC 0 to 360 by 12 degrees)
                if (!argc(5)) continue;
                replacePieces(draw(at(4), v7("CIRCLE", {at(4), at(0), at(1), at(2), at(3), "0", "360", "0", "12"})));
            } else if (k == "ARC") {
                // ARC cx, cy, start, end, step, rx, ry, f: 0 degrees up, clockwise, as BASIC 7.0's CIRCLE
                if (!argc(8)) continue;
                replacePieces(draw(at(7), v7("CIRCLE", {at(7), at(0), at(1), at(5), at(6), at(2), at(3), "0", at(4)})));
            } else if (k == "ANGL") {
                // ANGL cx, cy, angle, rx, ry, f: a line from the centre to the ellipse
                if (!argc(6)) continue;
                std::string t = "(" + at(2) + ") * " + RAD;
                replacePieces(draw(at(5), v7("DRAW", {at(5), at(0), at(1), "(" + at(0) + ") + (" + at(3) + ") * sin(" + t + ")",
                                                     "(" + at(1) + ") - (" + at(4) + ") * cos(" + t + ")"}, 3)));
            } else if (k == "PAINT") {
                // PAINT x, y, f: the area of (x, y)'s colour
                if (!argc(3)) continue;
                replace("paint " + (multi ? times(at(0), 2) : at(0)) + ", " + at(1) + ", 1, @@cs(" + (at(2) == (multi ? "4" : "2") ? "5" : at(2)) + ")");
            } else if (k == "TEXT") {
                // TEXT x, y, string, f, height, spacing -> CHAR by 8-pixel columns (MULTI's characters twice as wide)
                if (a.list.size() < 3) { argc(6); continue; }
                std::string f = at(3).empty() ? "1" : at(3), zoom = at(4).empty() ? "1" : at(4);
                if (!at(5).empty() && at(5) != "8") warn(line.num, "TEXT's spacing " + at(5) + ": BASIC 65's CHAR places characters 8 pixels apart");
                if (isNumber(at(0)) && std::stoi(at(0)) % (multi ? 4 : 8)) warn(line.num, "TEXT at x " + at(0) + ": CHAR places text by 8-pixel columns");
                std::string col = multi ? "int((" + at(0) + ") / 4)" : "int((" + at(0) + ") / 8)";
                if (isNumber(at(0))) col = std::to_string(std::stoi(at(0)) / (multi ? 4 : 8));
                replacePieces(then(v7("__PEN", {f == (multi ? "4" : "2") ? "5" : f}),
                                   {{false, "char " + col + ", " + at(1) + ", " + zoom + ", " + (multi ? "2" : "1") + ", 2, " + at(2)}}));
            } else if (k == "NRM" || k == "CSET") {
                std::string n = k == "NRM" ? "0" : at(0);
                if (n == "2") { warn(line.num, "CSET 2 (the graphics screen shown again): BASIC 65 closed it; left as written"); p[i] = {false, "cset"}; continue; }
                if (!isNumber(n)) { warn(line.num, "CSET of an expression: left as written"); p[i] = {false, "cset"}; continue; }
                multi = false;
                replacePieces(then(v7("GRAPHIC", {"0"}), {{false, n == "1" ? "print chr$(14);" : "print chr$(142);"}}));
            }
            // ---- text screen
            else if (k == "CENTRE") {
                std::string x = joinPieces(p, i + 1, e);
                replace("print spc(int((40 - len(" + x + ")) / 2)); " + x + ";");
            } else if (k == "PAUSE") {
                // PAUSE ["text",] seconds: RETURN ends it early
                std::string t = a.list.size() == 2 ? "print " + at(0) + " : " : "";
                std::string n = a.list.size() == 2 ? at(1) : at(0);
                replace(t + var("pause") + " = ti : do : get " + var("key") + "$ : loop until " + var("key") + "$ = chr$(13) or ti - " +
                        var("pause") + " >= " + n);
            } else if (k == "USE") {
                if (!argc(2)) continue;
                replace("print using " + at(0) + "; val(" + at(1) + ");");
                warn(line.num, "USE -> PRINT USING: BASIC 65 rounds, Simons' BASIC truncates");
            } else if (k == "DIR") {
                std::string x = at(0);
                if (x == "\"$\"" || x.empty()) replace("dir");
                else if (x.size() > 2 && x.rfind("\"$", 0) == 0) replace("dir \"" + x.substr(2));
                else replace("dir " + x);
            } else if (k == "DISK") {
                replace("open 15, 8, 15, " + joinPieces(p, i + 1, e) + " : close 15");
            } else if (k == "DISPLAY") replace("key");
            else if (k == "KEY") p[i] = {false, "key"};
            // ---- sprites
            else if (k == "MOB SET") {
                if (a.list.size() > 5) {
                    warn(line.num, "MOB SET with " + std::to_string(a.list.size()) + " arguments (TSB's): the first five converted");
                    a.list.resize(5);
                }
                if (!argc(5)) continue;
                replace("sprite " + at(0) + ", 1, " + at(2) + ", " + at(3) + ", , , " + at(4));
                warn(line.num, "MOB SET: the sprite's data block (" + at(1) + ") is the C64's: give BASIC 65 the data (SPRSAV / SPRDEF)");
            } else if (k == "MMOB" || k == "RLOCMOB") {
                // MMOB n, x1, y1, x2, y2, size, speed (RLOCMOB n, x2, y2, size, speed: from where it is): the program waits
                bool from = k == "MMOB";
                if (from && a.list.size() == 3) { replace("movspr " + at(0) + ", " + at(1) + ", " + at(2)); continue; }   // (TSB: placed)
                if (!argc(from ? 7 : 5)) continue;
                std::string n = at(0), sz = at(from ? 5 : 3), sp = at(from ? 6 : 4);
                std::string x1 = from ? at(1) : "rsppos(" + n + ", 0)", y1 = from ? at(2) : "rsppos(" + n + ", 1)";
                std::string x2 = at(from ? 3 : 1), y2 = at(from ? 4 : 2);
                std::string xe = isNumber(sz) ? std::to_string(std::stoi(sz) & 1) : "(" + sz + ") and 1";
                std::string ye = isNumber(sz) ? std::to_string((std::stoi(sz) >> 1) & 1) : "-(((" + sz + ") and 2) > 0)";
                // (Simons' speed is a delay per pixel, BASIC 65's a speed 1-15)
                std::string sp65 = isNumber(sp) ? std::to_string(std::max(1L, std::min(15L, 15 - std::lround(std::stod(sp) / 18)))) : "8";
                replace("sprite " + n + ", , , , " + xe + ", " + ye + " : movspr " + n + ", " + x1 + ", " + y1 + " to " + x2 + ", " + y2 + ", " + sp65 +
                        " : do while rsppos(" + n + ", 2) : loop");
                warn(line.num, k + ": the speed is approximate (Simons' BASIC waits per pixel, BASIC 65 moves at a speed 1-15)");
            } else if (k == "CMOB") {
                if (!argc(2)) continue;
                replace("sprcolor " + at(0) + ", " + at(1));
            } else if (k == "MOB OFF") replace("sprite " + joinPieces(p, i + 1, e) + ", 0");
            // ---- sound
            else if (k == "VOL") {
                std::string x = joinPieces(p, i + 1, e);
                if (isNumber(x) && std::stoi(x) > 15) warn(line.num, "VOL " + x + ": the SID's filter bits are not BASIC 65's VOL");
                replace("vol " + (isNumber(x) ? std::to_string(std::stoi(x) & 15) : "(" + x + ") and 15"));
            }
            // ---- the rest
            else if (auto it = simonsMissing().find(k); it != simonsMissing().end()) leave(it->second);
            else if (isSimonsKeyword(k)) {
                // direct-mode commands BASIC 65 has too (AUTO, RENUMBER, FIND, MERGE), and the rest
                static const std::set<std::string> same = {"AUTO", "RENUMBER", "FIND", "MERGE"};
                if (!same.count(k)) leave("Simons' BASIC");
                else p[i] = {false, lower(k)};
            }
        }
        if (split) {
            DecompiledLine rest{line.num, Pieces(p.begin() + (long)split, p.end())};
            rest.cont = true;
            p.erase(p.begin() + (long)split - 1, p.end());
            lines.insert(lines.begin() + (long)li + 1, rest);
        }
        flushExtra();
    }
    // RCOMP repeats the last IF's result: each IF keeps it in a variable
    if (rcomp)
        for (auto& line : lines) {
            auto& p = line.pieces;
            for (size_t i = 0; i < p.size(); i++) {
                if (p[i].isKw && p[i].text == "REM") break;
                if (!(p[i].isKw && p[i].text == "IF")) continue;
                size_t t = i + 1;
                while (t < p.size() && !(p[t].isKw && (p[t].text == "THEN" || p[t].text == "GOTO"))) t++;
                if (t >= p.size()) continue;
                std::string c = joinPieces(p, i + 1, t);
                if (c == var("rcomp")) continue;   // (RCOMP's own)
                if (names.count("cgoto") && c.rfind(names["cgoto"] + " = ", 0) == 0) continue;   // (the CGOTO table's: not the program's IFs)
                p.erase(p.begin() + (long)i + 1, p.begin() + (long)t);
                p.insert(p.begin() + (long)i + 1, {false, var("rcomp")});
                p.insert(p.begin() + (long)i, {{false, var("rcomp") + " = ((" + c + ") <> 0)"}, {false, ":"}});
                i += 3;
            }
        }

    // the colours HIRES / MULTI set (HI COL brings them back)
    for (auto [ph, key] : {std::pair<std::string, const char*>{G0, "g0"}, {G1, "g1"}, {G2, "g2"}, {G3, "g3"}})
        for (auto& l : lines)
            for (auto& pc : l.pieces)
                if (!pc.isKw)
                    for (size_t q; (q = pc.text.find(ph)) != std::string::npos;) pc.text.replace(q, ph.size(), var(key));
    notes.push_back("converted from Simons' BASIC (C64) by basic45 --convert=simons: check the warnings");
}

}  // namespace

void convertBasicLines(std::vector<DecompiledLine>& lines, const BasicDecompileOptions& options,
                       std::vector<std::string>& warnings, std::vector<std::string>& notes) {
    if (options.source == BasicSource::V7 || options.source == BasicSource::V35) convertV7(lines, options, warnings, notes);
    if (options.source == BasicSource::SIMONS) {
        convertSimons(lines, options, warnings, notes);
        convertV7(lines, options, warnings, notes);
    }
}
