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
#include "BasicTokenizer.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
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

void convertV7(std::vector<DecompiledLine>& lines, const BasicDecompileOptions& o, std::vector<std::string>& warnings,
               std::vector<std::string>& notes) {
    const double factor = o.ntsc ? 24.0 / 19.2 : 24.0 / 23.04;
    auto warn = [&](uint16_t n, const std::string& m) { warnings.push_back("line " + std::to_string(n) + ": " + m); };

    // pass 0: variables. Only a name's first two letters count in both
    // BASICs (BASEX and BASEY are one variable), but basic45 keeps long names
    // apart: names that are one variable on the C128 are written as their two
    // letters. DT$ (and any DT...$) is BASIC 65's date, a reserved name.
    {
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
                notes.push_back(all + ": one variable on the C128 (two letters count): written " + b);
            }
        }
        if (!rename.empty())
            forEachIdent([&](DecompiledPiece& pc, const std::string&, uint16_t) {
                if (auto it = rename.find(pc.text); it != rename.end()) pc.text = it->second;
            });
    }

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
    const std::string CS = fresh(), PX = fresh(), PY = fresh();
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
            } else if (!b65Keywords().count(k) && k != "DRAW" && k != "LOCATE" && k != "SLOW" &&
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
    uint16_t nextNum = (uint16_t)std::min(63999, lastNum + 2);
    // a subroutine of lines; body(at) builds them knowing their first line's number (for
    // their own GOTOs and GOSUBs, which become labels as the program's do)
    auto helperP = [&](const std::string& key, const std::function<std::vector<Pieces>(uint16_t)>& body) -> uint16_t {
        if (auto it = helperAt.find(key); it != helperAt.end()) return it->second;
        uint16_t at = nextNum;
        for (auto& b : body(at)) {
            if (nextNum > 63999) { warnings.push_back("no line numbers left after line " + std::to_string(lastNum) + " for its subroutines"); break; }
            helperLines.push_back({nextNum++, b});
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
            if (k == "TEMPO") {
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
                if (a.list.size() != 2) { warn(line.num, "COLOR with " + std::to_string(a.list.size()) + " arguments: left as written"); continue; }
                const std::string& s = a.list[0];
                std::string c = minus1(a.list[1]);
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
                if (a.list.empty() || a.list[0].empty()) continue;
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
    notes.push_back("converted from BASIC 7.0 (C128) by basic45 --convert=basic7 (" +
                    std::string(o.ntsc ? "NTSC" : "PAL") + " timing): check the warnings");

    // a first line: BASIC 7.0's defaults the program relies on (and its 40-column screen)
    Pieces start = gosub(columns(40));
    std::string prologue;
    if (anyPlay && !anyTempo) {
        long m = std::lround(8 * factor);   // BASIC 7.0's default tempo is 8, BASIC 65's 12
        prologue = "tempo " + std::to_string(m);
        notes.push_back("TEMPO " + std::to_string(m) + ": BASIC 7.0's default tempo (8) in BASIC 65's terms");
    }
    if (graphics && usedState) {
        // the colour sources (BASIC 7.0's defaults less one: background 12 dark grey, foreground 2 white,
        // multicolour 3 red and 6 green) and the pixel cursor
        std::string g = "dim " + CS + "(6) : " + CS + "(0) = 11 : " + CS + "(1) = 1 : " + CS + "(2) = 2 : " + CS +
                        "(3) = 5 : " + PX + " = 0 : " + PY + " = 0";
        prologue += (prologue.empty() ? "" : " : ") + g;
        notes.push_back(CS + "(): BASIC 7.0's colour sources (0-6, colours 0-15), " + PX + ", " + PY + ": its pixel cursor");
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
}

}  // namespace

void convertBasicLines(std::vector<DecompiledLine>& lines, const BasicDecompileOptions& options,
                       std::vector<std::string>& warnings, std::vector<std::string>& notes) {
    if (options.source == BasicSource::V7) convertV7(lines, options, warnings, notes);
}
