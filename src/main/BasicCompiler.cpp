// basic45 --compile (#369): a tokenized BASIC 65 program to cc45's IR.
// See include/BasicCompiler.hpp and doc/bin/basic45.md (Compiling).
//
// The program is one function, main. Each line and statement starts a
// block; GOTO is a branch, GOSUB pushes the address of its return block on
// BASIC's stack (__bas_stk, 16-bit entries) and RETURN takes it off
// (br_indirect). FOR pushes its loop's number (entries below 256) and NEXT
// finds it as BASIC 65 does (the run-time stack), so a GOTO out of a loop, a
// NEXT in an IF, a loop restarted by GOTO behave as in the interpreter.
// IF jumps, when false, to the statement after the line's next ELSE (or the
// next line); THEN BEGIN to the matching BEND first (the ROM's IF).
//
// Values: floats (F32), integers known exactly (I16 or I32 as a bound on
// their size allows:
// narrowed float variables (see compileBasicToIR),
// % variables, whole literals, their + - *), the 0/1 results of comparisons,
// strings (a pointer to a length byte and the characters), arrays (a pointer
// to a heap block: __bas_aref, basic_str.c). The arithmetic of
// floats is the library's float runtime (the BASIC 65 ROM's routines, as the
// interpreter computes); BASIC's checks (division by zero, ranges) and
// messages are the runtime's (basic_rt.s45, basic_str.c).

#include "BasicCompiler.hpp"
#include "IR.hpp"
#include "CodegenStage.hpp"
#include "BasicTokenizer.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>

namespace {

// ----------------------------------------------------------------------------
// Tokens
// ----------------------------------------------------------------------------

// Single-byte tokens the compiler names (BASIC 65's table, as BasicTokenizer)
enum : int {
    T_END = 0x80, T_FOR = 0x81, T_NEXT = 0x82, T_INPUTN = 0x84, T_INPUT = 0x85, T_DATA = 0x83, T_DIM = 0x86, T_READ = 0x87, T_RESTORE = 0x8C, T_LET = 0x88, T_GOTO = 0x89,
    T_IF = 0x8B, T_GOSUB = 0x8D, T_RETURN = 0x8E, T_REM = 0x8F, T_STOP = 0x90, T_ON = 0x91,
    T_POKE = 0x97, T_PRINT = 0x99, T_TAB = 0xA3, T_TO = 0xA4, T_GET = 0xA1, T_SPC = 0xA6, T_THEN = 0xA7,
    T_NOT = 0xA8, T_STEP = 0xA9, T_PLUS = 0xAA, T_MINUS = 0xAB, T_MUL = 0xAC, T_DIV = 0xAD,
    T_POW = 0xAE, T_AND = 0xAF, T_OR = 0xB0, T_GT = 0xB1, T_EQ = 0xB2, T_LT = 0xB3,
    T_RND = 0xBB, T_SGN = 0xB4, T_INT = 0xB5, T_ABS = 0xB6, T_SQR = 0xBA, T_LOG = 0xBC, T_EXP = 0xBD,
    T_COS = 0xBE, T_SIN = 0xBF, T_TAN = 0xC0, T_ATN = 0xC1, T_PEEK = 0xC2, T_LEN = 0xC3,
    T_STR = 0xC4, T_ASC = 0xC6, T_CHR = 0xC7, T_LEFT = 0xC8, T_RIGHT = 0xC9,
    T_MID = 0xCA, T_GO = 0xCB, T_KEY = 0xF9, T_ELSE = 0xD5, T_DEF = 0x96, T_FN = 0xA5, T_POS = 0xB9, T_VAL = 0xC5, T_DEC = 0xD1, T_HEX = 0xD2,
    T_INSTR = 0xD4, T_SCNCLR = 0xE8, T_XOR = 0xE9, T_WAIT = 0x92, T_SLEEP = 0xFE0B, T_WPOKE = 0xFE1D,
    T_MOD = 0xCE0B, T_WPEEK = 0xCE10, T_STRBIN = 0xCE12, T_RPT = 0xCE14, T_BANK = 0xFE02, T_ERRS = 0xD3, T_RESUME = 0xD6, T_TRAP = 0xD7, T_ASL = 0xFE52, T_ASR = 0xFE53, T_DO = 0xEB, T_LOOP = 0xEC, T_EXIT = 0xED, T_UNTIL = 0xFC, T_WHILE = 0xFD, T_PI = 0xFF,
    T_BEGIN = 0xFE18, T_BEND = 0xFE19, T_PRINTN = 0x98, T_CMD = 0x9D, T_OPEN = 0x9F, T_CLOSE = 0xA0,
    T_DOPEN = 0xFE0D, T_APPEND = 0xFE0E, T_DCLOSE = 0xFE0F,
};

enum class TK { Kw, Num, Str, Name, Ch };

struct Tok {
    TK k;
    int kw = 0;              // Kw: the byte, or $FExx / $CExx
    double num = 0;          // Num
    bool whole = false;      // Num: written without a point or E (a whole number)
    std::string text;        // Str: the characters; Name: the name (with $ / %)
    char ch = 0;             // Ch
};

struct Seg {                 // a statement: the tokens between colons
    std::vector<Tok> toks;
};

struct Line {
    int num = 0;
    std::vector<Seg> segs;
};

// The name of a token (for messages)
std::string tokenName(int kw) {
    static std::map<int, std::string> names;
    if (names.empty()) {
        BasicTokenizer t;
        for (const auto& [n, b] : t.getKeywords()) names[b] = n;
        for (const auto& [n, b] : t.getEscapeKeywords()) names[0xFE00 | b] = n;
        for (const auto& [n, b] : t.getCeKeywords()) names[0xCE00 | b] = n;
        const char* ops[] = {"+", "-", "*", "/", "^", "AND", "OR", ">", "=", "<"};
        for (int i = 0; i < 10; i++) names[0xAA + i] = ops[i];
        names[T_PI] = "pi";
    }
    auto it = names.find(kw);
    if (it != names.end()) return it->second;
    char b[16];
    snprintf(b, sizeof b, "token $%X", kw);
    return b;
}

bool isLetter(uint8_t c) { return c >= 'A' && c <= 'Z'; }
bool isDigit(uint8_t c) { return c >= '0' && c <= '9'; }

// A line's bytes as statements (REM and DATA: the rest left out)
std::vector<Seg> lexLine(const std::vector<uint8_t>& b, std::string& err) {
    std::vector<Seg> segs(1);
    size_t i = 0;
    auto add = [&](Tok t) { segs.back().toks.push_back(std::move(t)); };
    while (i < b.size()) {
        uint8_t c = b[i];
        if (c == ' ') { i++; continue; }
        if (c == ':') { segs.emplace_back(); i++; continue; }
        if (c == '"') {
            Tok t{TK::Str};
            i++;
            while (i < b.size() && b[i] != '"') t.text += (char)b[i++];
            if (i < b.size()) i++;
            add(t);
            continue;
        }
        if (isDigit(c) || c == '.') {
            std::string s;
            bool whole = true;
            while (i < b.size() && (isDigit(b[i]) || b[i] == '.' || b[i] == ' ')) {
                if (b[i] == '.') whole = false;
                if (b[i] != ' ') s += (char)b[i];
                i++;
            }
            if (i < b.size() && b[i] == 'E') {
                size_t j = i + 1;
                std::string e = "e";
                if (j < b.size() && (b[j] == T_PLUS || b[j] == '+')) j++;
                else if (j < b.size() && (b[j] == T_MINUS || b[j] == '-')) { e += '-'; j++; }
                if (j < b.size() && isDigit(b[j])) {
                    while (j < b.size() && isDigit(b[j])) e += (char)b[j++];
                    s += e;
                    whole = false;
                    i = j;
                }
            }
            Tok t{TK::Num};
            t.num = s == "." ? 0 : std::strtod(s.c_str(), nullptr);
            t.whole = whole;
            add(t);
            continue;
        }
        if (c == '$' && i + 1 < b.size() && (isDigit(b[i + 1]) || (b[i + 1] >= 'A' && b[i + 1] <= 'F'))) {
            // a hex number ($D020)
            i++;
            long v = 0;
            while (i < b.size() && (isDigit(b[i]) || (b[i] >= 'A' && b[i] <= 'F')))
                v = v * 16 + (isDigit(b[i]) ? b[i] - '0' : b[i] - 'A' + 10), i++;
            Tok t{TK::Num};
            t.num = (double)v;
            t.whole = true;
            add(t);
            continue;
        }
        if (isLetter(c)) {
            Tok t{TK::Name};
            while (i < b.size() && (isLetter(b[i]) || isDigit(b[i]))) t.text += (char)b[i++];
            if (i < b.size() && (b[i] == '$' || b[i] == '%')) t.text += (char)b[i++];
            add(t);
            continue;
        }
        if (c == 0xFE || c == 0xCE) {
            if (i + 1 >= b.size()) { err = "a token cut off"; return segs; }
            Tok t{TK::Kw};
            t.kw = (c << 8) | b[i + 1];
            i += 2;
            add(t);
            continue;
        }
        if (c == T_PI) {
            Tok t{TK::Num};
            t.num = M_PI;
            add(t);
            i++;
            continue;
        }
        if (c >= 0x80) {
            Tok t{TK::Kw};
            t.kw = c;
            add(t);
            i++;
            if (c == T_REM) return segs;
            if (c == T_DATA) {
                // its items are not statements: up to the colon (the text kept for READ)
                bool q = false;
                std::string raw;
                while (i < b.size() && (q || b[i] != ':')) { if (b[i] == '"') q = !q; raw += (char)b[i]; i++; }
                segs.back().toks.back().text = raw;
            }
            continue;
        }
        Tok t{TK::Ch};
        t.ch = (char)c;
        add(t);
        i++;
    }
    return segs;
}

// ----------------------------------------------------------------------------
// The compiler
// ----------------------------------------------------------------------------

struct CompileError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

enum class VK { F, I, B, S };

struct Val {
    VK k = VK::F;
    ir::Operand op;          // F: F32, I: I16 (|value| <= 32767, or a 16-bit source) or I32, B: I8 (0/1), S: PTR
    bool isConst = false;    // F / I: a constant (cval)
    double cval = 0;
    int64_t bound = 0;       // I: |value| <= bound
    int slot = -1;           // S: the temporary buffer it is in (-1: a variable or literal)
};

constexpr int STACK_ENTRIES = 128;   // __bas_stk (basic_rt.c)
constexpr int MAX_SLOTS = 8;         // string temporaries in a statement

// Integer narrowing: what a compile saw of a float variable's values
struct VarObs {
    bool bad = false;        // a value not known to be a whole number in range
    int64_t assign = 0;      // the largest bound of a value assigned (FOR's start and limit too)
    int64_t step = 0;        // the largest |STEP| of its FOR loops
};
// A narrowed variable's bound (|value| <= bound, below 2^31) and its storage
using NarrowMap = std::map<std::string, int64_t>;
inline ir::Type narrowType(int64_t bound) { return bound <= 32767 ? ir::Type::I16 : ir::Type::I32; }

struct ForInfo {
    int id;
    std::string var;         // the variable's global
    int varIndex;            // for __bas_forvar
    bool stepConst = true;
    double step = 1;
    size_t li = 0, si = 0;   // where it is
    bool lead = false;       // the first token of its statement
    bool counted = false;    // a counted loop: no entry on BASIC's stack (countedLoops)
};

class Compiler {
public:
    Compiler(const std::vector<Line>& lines, const std::string& source, const NarrowMap& narrow, bool final,
             bool speed)
        : lines_(lines), source_(source), narrow_(narrow), final_(final), speed_(speed) {}

    std::shared_ptr<ir::Module> run();
    std::vector<std::pair<int, std::string>> errors;
    // the float variables' values seen (an analysis compile)
    std::map<std::string, VarObs> obs;

private:
    const std::vector<Line>& lines_;
    std::string source_;
    const NarrowMap& narrow_;
    bool final_;
    bool speed_;        // --speed: the RAM float routines
    std::shared_ptr<ir::Module> mod_;
    ir::Function* fn_ = nullptr;
    ir::Block* cur_ = nullptr;
    int labelN_ = 0;

    // the statement being compiled
    size_t li_ = 0, si_ = 0;
    const std::vector<Tok>* toks_ = nullptr;
    size_t p_ = 0;
    int slots_ = 0, maxSlots_ = 0;

    std::map<int, size_t> lineIndex_;            // line number -> index
    std::map<std::string, ir::Type> vars_;       // global -> type
    std::set<std::string> arrays_;               // the arrays' globals (a pointer: 0 or the block)
    std::map<std::string, std::string> strLits_; // text -> global
    std::vector<ForInfo> fors_;
    std::map<std::pair<size_t, size_t>, int> forAt_;   // (line, statement) -> its FOR
    // a counted loop's NEXT: (line, statement, the variable's place in its
    // list, -1 for none) -> the loop
    std::map<std::tuple<size_t, size_t, int>, int> countedNext_;
    void countedLoops();
    ir::Operand stepDone(const ForInfo& f);
    std::map<std::string, int> forVarIndex_;
    std::set<std::string> externs_;
    std::set<int> errBlocks_;
    std::set<int> stepBlocks_;                   // the FOR loops whose step block a NEXT uses
    // __bas_line (the line an error names): stored where a line is entered
    // (its start; a FOR's body, a GOSUB's return, a DO: reached from other
    // lines) only for a line that can raise an error
    std::vector<std::vector<size_t>> lineEntries_;   // line -> its entry blocks
    std::vector<bool> lineRaises_;
    void entry() { if (li_ < lineEntries_.size()) lineEntries_[li_].push_back(fn_->blocks.size() - 1); }
    void raises() { if (li_ < lineRaises_.size()) lineRaises_[li_] = true; }
    void stepBlock(const ForInfo& f);

    [[noreturn]] void fail(const std::string& m) { throw CompileError(m); }

    // --- IR helpers ---
    ir::Operand vreg(ir::Type t) {
        uint32_t id = fn_->allocVreg();
        fn_->vregTypes[id] = t;
        return ir::Operand::vreg(id, t);
    }
    void emit(ir::Inst in) {
        in.loc.file = source_;
        in.loc.line = li_ < lines_.size() ? lines_[li_].num : 0;
        cur_->insts.push_back(std::move(in));
    }
    void block(const std::string& label) {
        fn_->blocks.push_back({label, {}});
        cur_ = &fn_->blocks.back();
    }
    std::string newLabel(const std::string& p) { return p + std::to_string(labelN_++); }
    static std::string lineLabel(size_t li) { return "L" + std::to_string(li); }
    static std::string stmtLabel(size_t li, size_t si) { return "S" + std::to_string(li) + "_" + std::to_string(si); }
    static std::string elseLabel(size_t li, size_t si) { return "E" + std::to_string(li) + "_" + std::to_string(si); }
    std::string afterLine(size_t li) { return li + 1 < lines_.size() ? lineLabel(li + 1) : "Lend"; }

    ir::Operand global(const std::string& name) {
        if (!vars_.count(name) && !arrays_.count(name)) externs_.insert(name);
        return ir::Operand::global(name);
    }
    void br(const std::string& l) {
        ir::Inst in; in.op = ir::Op::BR; in.src1 = ir::Operand::label(l); emit(in);
    }
    void brCond(ir::Operand c, const std::string& t, const std::string& f) {
        ir::Inst in; in.op = ir::Op::BR_COND; in.src1 = c;
        in.dest = ir::Operand::label(t); in.src2 = ir::Operand::label(f); emit(in);
    }
    ir::Operand load(ir::Type t, ir::Operand addr) {
        auto d = vreg(t);
        ir::Inst in; in.op = ir::Op::LOAD; in.dest = d; in.src1 = addr; in.resultType = t; emit(in);
        return d;
    }
    void store(ir::Type t, ir::Operand v, ir::Operand addr) {
        ir::Inst in; in.op = ir::Op::STORE; in.src1 = v; in.src2 = addr; in.resultType = t; emit(in);
    }
    ir::Operand iconst(ir::Type t, int64_t v) {
        auto d = vreg(t);
        ir::Inst in; in.op = ir::Op::CONST; in.dest = d; in.src1 = ir::Operand::imm(v, t); in.resultType = t; emit(in);
        return d;
    }
    ir::Operand fconst(double v) {
        auto d = vreg(ir::Type::F32);
        int64_t bits;
        std::memcpy(&bits, &v, sizeof bits);
        ir::Inst in; in.op = ir::Op::FCONST; in.dest = d; in.src1 = ir::Operand::imm(bits, ir::Type::F32);
        in.resultType = ir::Type::F32; emit(in);
        return d;
    }
    ir::Operand bin(ir::Op op, ir::Type t, ir::Operand a, ir::Operand b) {
        auto d = vreg(t);
        ir::Inst in; in.op = op; in.dest = d; in.src1 = a; in.src2 = b; in.resultType = t; emit(in);
        return d;
    }
    ir::Operand un(ir::Op op, ir::Type t, ir::Operand a) {
        auto d = vreg(t);
        ir::Inst in; in.op = op; in.dest = d; in.src1 = a; in.resultType = t; emit(in);
        return d;
    }
    void call(const std::string& f) {
        // the run time's routines that never raise an error
        static const std::set<std::string> safe = {
            "__bas_prs", "__bas_prn", "__bas_prcr", "__bas_prcomma", "__bas_prspc", "__bas_prtab", "__bas_scmp",
            "__bas_left", "__bas_right", "__bas_chr", "__bas_str", "__bas_iline", "__bas_iitem", "__bas_get", "__bas_st", "__bas_clrch",
            "__bas_ti", "__bas_tis", "__bas_restore", "__bas_rnd", "__bas_int", "__float_sin", "__float_cos",
            "__float_tan", "__float_atn", "__float_abs"};
        // (with CMD, PRINT's output can go to a file: an error)
        if (!safe.count(f) || (cmdUsed_ && f.compare(0, 8, "__bas_pr") == 0)) raises();
        externs_.insert(f);
        fn_->originalCallees.insert(f);
        ir::Inst in; in.op = ir::Op::CALL_VOID; in.resultType = ir::Type::VOID;
        in.src1 = ir::Operand::global(f); in.callConv = ir::CallConv::ZP; emit(in);
    }
    // the error n (its block, shared: the line is in __bas_line)
    std::string errLabel(int n) { raises(); errBlocks_.insert(n); return "Err" + std::to_string(n); }
    // the error n where the statement runs (a constant out of range: the
    // interpreter's error only when the line is run)
    void raiseNow(int n) { raises(); br(errLabel(n)); block(newLabel("dead")); }
    void raiseIf(ir::Operand cond, int n) {
        raises();
        std::string ok = newLabel("ok");
        brCond(cond, errLabel(n), ok);
        block(ok);
    }

    // --- tokens ---
    bool atEnd() const { return p_ >= toks_->size(); }
    const Tok* peek() const { return atEnd() ? nullptr : &(*toks_)[p_]; }
    bool isKw(int kw) const { auto t = peek(); return t && t->k == TK::Kw && t->kw == kw; }
    bool isCh(char c) const { auto t = peek(); return t && t->k == TK::Ch && t->ch == c; }
    bool acceptKw(int kw) { if (isKw(kw)) { p_++; return true; } return false; }
    bool acceptCh(char c) { if (isCh(c)) { p_++; return true; } return false; }
    void expectCh(char c) { if (!acceptCh(c)) fail(std::string("'") + c + "' expected" + near()); }
    void expectKw(int kw) { if (!acceptKw(kw)) fail(tokenName(kw) + " expected" + near()); }
    std::string near() const {
        auto t = peek();
        if (!t) return " at the end of the statement";
        switch (t->k) {
        case TK::Kw: return " before " + tokenName(t->kw);
        case TK::Name: return " before " + t->text;
        case TK::Str: return " before \"" + t->text + "\"";
        case TK::Num: return " before a number";
        case TK::Ch: return std::string(" before '") + t->ch + "'";
        }
        return "";
    }
    // a line number (GOTO, THEN, ON)
    size_t lineTarget() {
        auto t = peek();
        if (!t || t->k != TK::Num || !t->whole) fail("a line number expected" + near());
        int n = (int)t->num;
        p_++;
        auto it = lineIndex_.find(n);
        if (it == lineIndex_.end()) fail("line " + std::to_string(n) + " does not exist (?UNDEF'D STATEMENT)");
        return it->second;
    }

    // --- variables ---
    // A scalar's global: BASIC 65 tells names apart by their first two
    // characters (and $ / %)
    std::string varGlobal(const std::string& name, VK& kind) {
        char suffix = name.back();
        std::string base = name;
        if (suffix == '$' || suffix == '%') base.pop_back();
        std::string key = base.substr(0, 2);
        static const std::set<std::string> reserved = {"TI", "ST", "DS", "ER", "EL"};
        if (reserved.count(key) && (base.size() == 2 || suffix == '$'))
            fail(name + " (a reserved variable) is not supported by --compile yet");
        std::string g;
        ir::Type t;
        if (suffix == '$') { kind = VK::S; g = "__bv_" + key + "_s"; t = ir::Type::PTR; }
        else if (suffix == '%') { kind = VK::I; g = "__bv_" + key + "_i"; t = ir::Type::I16; }
        else {
            kind = VK::F; g = "__bv_" + key;
            auto n = narrow_.find(g);
            t = n != narrow_.end() ? narrowType(n->second) : ir::Type::F32;
            obs[g];
        }
        vars_[g] = t;
        return g;
    }
    Val loadVar(const std::string& name) {
        // (a DEF FN's parameter while its expression is compiled)
        if (!fnBind_.empty() && name.back() != '$' && name.back() != '%') {
            auto b = fnBind_.find("__bv_" + name.substr(0, 2));
            if (b != fnBind_.end()) return b->second;
        }
        // TI: the seconds counter; TI$: the clock, "hh:mm:ss"
        if (name == "TI") {
            call("__bas_ti");
            Val v; v.k = VK::F; v.op = load(ir::Type::F32, global("__float_a"));
            return v;
        }
        if (name == "TI$") return strCall("__bas_tis", nullptr, nullptr);
        // DS: the drive's status number, DS$ its message (read when not kept)
        if (name == "DS") {
            call("__bas_ds");
            Val v; v.k = VK::I; v.bound = 255;
            v.op = load(ir::Type::I16, global("__bas_n"));
            return v;
        }
        if (name == "DS$") return strCall("__bas_dss", nullptr, nullptr);
        // ST: the KERNAL's I/O status, a signed byte (BASIC 65's Get_ST: float)
        if (name == "ST") {
            call("__bas_st");
            Val v; v.k = VK::I; v.bound = 128;
            v.op = sext(trunc(load(ir::Type::I16, global("__bas_n")), ir::Type::I8), ir::Type::I8, ir::Type::I16);
            return v;
        }
        // ER: the last error's number (-1: none), EL: its line (65535: none)
        if (name == "ER" || name == "EL") {
            erVars_ = true;
            Val v; v.k = VK::I;
            auto x = load(ir::Type::I16, global(name == "ER" ? "__bas_er" : "__bas_el"));
            if (name == "ER") { v.op = x; v.bound = 32768; }
            else { v.op = zext(x, ir::Type::I16, ir::Type::I32); v.bound = 65535; }
            return v;
        }
        VK k;
        std::string g = varGlobal(name, k);
        Val v;
        v.k = k;
        if (k == VK::F && narrow_.count(g)) {
            // a narrowed variable: a whole number within its bound
            int64_t b = narrow_.at(g);
            ir::Type t = narrowType(b);
            v.k = VK::I;
            v.op = load(t, ir::Operand::global(g));
            v.bound = b;
        } else if (k == VK::F) v.op = load(ir::Type::F32, ir::Operand::global(g));
        else if (k == VK::S) v.op = load(ir::Type::PTR, ir::Operand::global(g));
        else {
            v.op = load(ir::Type::I16, ir::Operand::global(g));
            v.bound = 32768;
        }
        return v;
    }
    // --- arrays ---
    // An array's global (BASIC 65 tells arrays apart from scalars, and by
    // their first two characters and $ / %): a pointer to its block
    std::string arrayGlobal(const std::string& name, VK& kind) {
        char suffix = name.back();
        std::string base = name;
        if (suffix == '$' || suffix == '%') base.pop_back();
        std::string g = "__ba_" + base.substr(0, 2);
        if (suffix == '$') { kind = VK::S; g += "_s"; }
        else if (suffix == '%') { kind = VK::I; g += "_i"; }
        else kind = VK::F;
        arrays_.insert(g);
        return g;
    }
    static int elemSize(VK k) { return k == VK::F ? 5 : 2; }
    // A subscript 0..65535 (BASIC's: ?ILLEGAL QUANTITY outside it): I16
    ir::Operand toIndex(const Val& v) {
        if (v.k == VK::S) fail("a number expected, not a string (?TYPE MISMATCH)");
        if ((v.k == VK::I || v.k == VK::F) && v.isConst) {
            double f = std::floor(v.cval);
            if (f < 0 || f > 65535) { raiseNow(14); return iconst(ir::Type::I16, 0); }
            return iconst(ir::Type::I16, (int64_t)f);
        }
        if (v.k == VK::I) {
            checkRange(v, 0, 65535);
            return v.op.type == ir::Type::I16 ? v.op : trunc(v.op, ir::Type::I16);
        }
        if (v.k == VK::B) {
            raiseIf(v.op, 14);              // (-1)
            return iconst(ir::Type::I16, 0);
        }
        store(ir::Type::F32, toF(v), global("__float_a"));
        call("__bas_getadr");
        return load(ir::Type::I16, global("__bas_n"));
    }
    ir::Operand addrGlobal(const std::string& g) {
        auto a = vreg(ir::Type::PTR);
        ir::Inst in; in.op = ir::Op::ADDR_GLOBAL; in.dest = a; in.src1 = global(g);
        in.resultType = ir::Type::PTR; emit(in);
        return a;
    }
    ir::Operand addrElem(ir::Operand base, ir::Operand idx, int size) {
        auto a = vreg(ir::Type::PTR);
        ir::Inst in; in.op = ir::Op::ADDR_ELEM; in.dest = a; in.src1 = base; in.src2 = idx;
        in.args.push_back(ir::Operand::imm(size, ir::Type::I16)); in.resultType = ir::Type::PTR; emit(in);
        return a;
    }
    void copyTo(ir::Operand d, ir::Operand x) {
        ir::Inst in; in.op = ir::Op::COPY; in.dest = d; in.src1 = x; in.resultType = d.type; emit(in);
    }
    // __bas_aref: the subscripts in __bas_sub, their number, the element's size and flags
    void arefCall(const std::string& g, const std::vector<ir::Operand>& subs, int m) {
        for (size_t k = 0; k < subs.size(); k++)
            store(ir::Type::I16, subs[k], addrElem(addrGlobal("__bas_sub"), ir::Operand::imm((int)k, ir::Type::I16), 2));
        store(ir::Type::I16, iconst(ir::Type::I16, (int64_t)subs.size()), global("__bas_n"));
        store(ir::Type::I16, iconst(ir::Type::I16, m), global("__bas_m"));
        store(ir::Type::PTR, addrGlobal(g), global("__bas_sa"));
        call("__bas_aref");
    }
    std::vector<ir::Operand> subscripts() {
        expectCh('(');
        std::vector<ir::Operand> subs;
        do {
            if (subs.size() == 8) fail("more than 8 subscripts are not supported by --compile");
            subs.push_back(toIndex(expr()));
        } while (acceptCh(','));
        expectCh(')');
        return subs;
    }
    // The address of the element name(...) (the '(' next): PTR. One
    // subscript: __bas_ael1 (with --speed: here) reaches the element when the
    // array exists with one dimension and the index is in it; else (and for
    // more) __bas_aref, which makes the array (11 elements a dimension) or
    // raises the error
    ir::Operand elemAddr(const std::string& name, VK& kind) {
        std::string g = arrayGlobal(name, kind);
        auto subs = subscripts();
        int es = elemSize(kind);
        int m = es | (kind == VK::S ? 0x40 : 0);
        auto r = vreg(ir::Type::PTR);
        if (subs.size() == 1 && !speed_) {
            // (the reach of the fast path below in __bas_ael1: the code smaller)
            store(ir::Type::I16, subs[0], global("__bas_n"));
            store(ir::Type::I16, iconst(ir::Type::I16, m), global("__bas_m"));
            store(ir::Type::PTR, addrGlobal(g), global("__bas_sa"));
            externs_.insert("__bas_ael1");
            fn_->originalCallees.insert("__bas_ael1");
            raises();
            ir::Inst in; in.op = ir::Op::CALL; in.dest = r; in.resultType = ir::Type::PTR;
            in.src1 = ir::Operand::global("__bas_ael1"); in.callConv = ir::CallConv::ZP; emit(in);
        } else if (subs.size() == 1) {
            std::string slow = newLabel("aslow"), c1 = newLabel("achk"), c2 = newLabel("achk"),
                        fast = newLabel("afast"), done = newLabel("adone");
            auto p = load(ir::Type::PTR, global(g));
            brCond(bin(ir::Op::CMP_EQ, ir::Type::I8, p, ir::Operand::imm(0, ir::Type::PTR)), slow, c1);
            block(c1);
            auto nd = load(ir::Type::I8, p);
            brCond(bin(ir::Op::CMP_NE, ir::Type::I8, nd, ir::Operand::imm(1, ir::Type::I8)), slow, c2);
            block(c2);
            auto sz = load(ir::Type::I16, addrElem(p, ir::Operand::imm(1, ir::Type::I16), 1));
            brCond(bin(ir::Op::CMP_GEU, ir::Type::I8, subs[0], sz), slow, fast);
            block(fast);
            copyTo(r, addrElem(addrElem(p, subs[0], es), ir::Operand::imm(3, ir::Type::I16), 1));
            br(done);
            block(slow);
            arefCall(g, subs, m);
            copyTo(r, load(ir::Type::PTR, global("__bas_sr")));
            br(done);
            block(done);
        } else {
            arefCall(g, subs, m);
            copyTo(r, load(ir::Type::PTR, global("__bas_sr")));
        }
        return r;
    }
    Val loadElem(const std::string& name) {
        VK k;
        auto a = elemAddr(name, k);
        Val v; v.k = k;
        if (k == VK::F) v.op = load(ir::Type::F32, a);
        else if (k == VK::S) v.op = load(ir::Type::PTR, a);
        else { v.op = load(ir::Type::I16, a); v.bound = 32768; }
        return v;
    }
    void dimStmt() {
        do {
            auto t = peek();
            if (!t || t->k != TK::Name) fail("an array expected after DIM");
            p_++;
            if (!isCh('(')) fail("DIM " + t->text + " without subscripts is not supported by --compile");
            VK k;
            std::string g = arrayGlobal(t->text, k);
            auto subs = subscripts();
            arefCall(g, subs, elemSize(k) | (k == VK::S ? 0x40 : 0) | 0x80);
        } while (acceptCh(','));
    }

    ir::Operand sext(ir::Operand x, ir::Type from, ir::Type to = ir::Type::I32) {
        auto d = vreg(to);
        ir::Inst in; in.op = ir::Op::SEXT; in.dest = d; in.src1 = x; in.resultType = to;
        in.src1.type = from;
        emit(in);
        return d;
    }
    ir::Operand zext(ir::Operand x, ir::Type from, ir::Type to) {
        auto d = vreg(to);
        ir::Inst in; in.op = ir::Op::ZEXT; in.dest = d; in.src1 = x; in.resultType = to;
        in.src1.type = from;
        emit(in);
        return d;
    }
    ir::Operand trunc(ir::Operand x, ir::Type to) {
        auto d = vreg(to);
        ir::Inst in; in.op = ir::Op::TRUNC; in.dest = d; in.src1 = x; in.resultType = to; emit(in);
        return d;
    }

    // --- narrowing ---
    // A value for the float variable g: recorded; its integer (I32) when g is
    // narrowed (an operand of nothing when the value does not fit: only in
    // an analysis compile, which is not used)
    void observe(const std::string& g, const Val& v, int64_t VarObs::*field) {
        auto& o = obs[g];
        if (v.k == VK::I) o.*field = std::max(o.*field, v.isConst ? (int64_t)std::fabs(v.cval) : v.bound);
        else if (v.k == VK::B) o.*field = std::max<int64_t>(o.*field, 1);
        else o.bad = true;
    }
    bool fitsNarrow(const Val& v, int64_t limit) {
        if (v.k == VK::B) return limit >= 1;
        return v.k == VK::I && (v.isConst ? std::fabs(v.cval) : (double)v.bound) <= (double)limit;
    }
    // An integer's type: 16 bits when its bound allows (a 16-bit result of
    // + - * NEG ABS needs |value| <= 32767), else 32
    static ir::Type intType(int64_t bound) { return bound <= 32767 ? ir::Type::I16 : ir::Type::I32; }
    // v (I / B) as an integer of type t (I16 / I32): widened, or cut when it fits
    ir::Operand asType(const Val& v, ir::Type t) {
        Val x = v.k == VK::B ? toI16(v) : v;
        if (x.isConst) return iconst(t, (int64_t)x.cval);
        if (x.op.type == t) return x.op;
        if (t == ir::Type::I32) return sext(x.op, x.op.type, t);
        return trunc(x.op, t);
    }
    void storeNarrow(const std::string& g, const Val& v) {
        int64_t b = narrow_.at(g);
        if (!fitsNarrow(v, b)) {
            if (final_) fail("internal error: the narrowed variable " + g + " gets a value out of its range");
            return;
        }
        ir::Type t = narrowType(b);
        store(t, asType(v, t), ir::Operand::global(g));
    }

    // --- conversions ---
    Val constI(int64_t v) {
        Val r; r.k = VK::I; r.isConst = true; r.cval = (double)v; r.bound = v < 0 ? -v : v;
        r.op = iconst(v >= -32768 && v <= 32767 ? ir::Type::I16 : ir::Type::I32, v);
        return r;
    }
    ir::Operand toF(const Val& v) {
        if (v.k == VK::S) fail("a number expected, not a string (?TYPE MISMATCH)");
        if (v.k == VK::F) return v.op;
        if (v.isConst) return fconst(v.cval);
        ir::Operand src = v.op;
        if (v.k == VK::B) src = un(ir::Op::NEG, ir::Type::I8, v.op);     // 1 -> -1 ($FF)
        auto d = vreg(ir::Type::F32);
        ir::Inst in; in.op = ir::Op::ITOF; in.dest = d; in.src1 = src; in.resultType = ir::Type::F32;
        in.src2 = ir::Operand::imm(1, ir::Type::I8);
        emit(in);
        return d;
    }
    // A float at __float_a, the runtime routine f, its 16-bit result
    ir::Operand viaN(const Val& v, const std::string& f) {
        store(ir::Type::F32, toF(v), global("__float_a"));
        call(f);
        return load(ir::Type::I16, global("__bas_n"));
    }
    // An integer -32768..32767 (AND, OR, NOT, a % variable): BASIC's AYINT
    // (rounded down, ?ILLEGAL QUANTITY outside it). I16
    Val toI16(const Val& v) {
        if (v.k == VK::S) fail("a number expected, not a string (?TYPE MISMATCH)");
        if (v.k == VK::B) {
            Val r; r.k = VK::I; r.bound = 1;
            r.op = sext(un(ir::Op::NEG, ir::Type::I8, v.op), ir::Type::I8, ir::Type::I16);
            return r;
        }
        if (v.k == VK::I) {
            if (v.isConst) {
                if (v.cval < -32768 || v.cval > 32767) { raiseNow(14); return constI(0); }
                return v;
            }
            checkRange(v, -32768, 32767);
            Val r = v; r.bound = std::min<int64_t>(v.bound, 32768);
            if (r.op.type != ir::Type::I16) r.op = trunc(v.op, ir::Type::I16);
            return r;
        }
        if (v.isConst) {
            double f = std::floor(v.cval);
            if (f < -32768 || f > 32767) { raiseNow(14); return constI(0); }
            return constI((int64_t)f);
        }
        Val r; r.k = VK::I; r.bound = 32768;
        r.op = viaN(v, "__bas_ayint");
        return r;
    }
    // AND, OR, NOT, XOR's integer: BASIC 65's ayintw (-32768..65535, rounded
    // down, ?ILLEGAL QUANTITY outside), its 16 bits: I16
    Val toI16w(const Val& v) {
        if (v.k == VK::S) fail("a number expected, not a string (?TYPE MISMATCH)");
        if (v.k == VK::B) return toI16(v);
        if (v.k == VK::I) {
            if (v.isConst) {
                if (v.cval < -32768 || v.cval > 65535) { raiseNow(14); return constI(0); }
                int64_t c = (int64_t)v.cval;
                return constI(c > 32767 ? c - 65536 : c);
            }
            checkRange(v, -32768, 65535);
            Val r = v; r.bound = 32768; r.isConst = false;
            if (r.op.type != ir::Type::I16) r.op = trunc(v.op, ir::Type::I16);
            return r;
        }
        if (v.isConst) {
            double f = std::floor(v.cval);
            if (f < -32768 || f > 65535) { raiseNow(14); return constI(0); }
            int64_t c = (int64_t)f;
            return constI(c > 32767 ? c - 65536 : c);
        }
        Val r; r.k = VK::I; r.bound = 32768;
        r.op = viaN(v, "__bas_ayintw");
        return r;
    }
    // ?ILLEGAL QUANTITY outside lo..hi: only the limits the value's bound
    // (and its type) can pass are checked
    void checkRange(const Val& v, int64_t lo, int64_t hi) {
        ir::Type t = v.op.type;
        int64_t tmin = t == ir::Type::I16 ? -32768 : INT32_MIN, tmax = t == ir::Type::I16 ? 32767 : INT32_MAX;
        if (-v.bound < lo && lo > tmin)
            raiseIf(bin(ir::Op::CMP_LT, ir::Type::I8, v.op, ir::Operand::imm(lo, t)), 14);
        if (v.bound > hi && hi < tmax)
            raiseIf(bin(ir::Op::CMP_GT, ir::Type::I8, v.op, ir::Operand::imm(hi, t)), 14);
    }
    // A byte 0..255 (POKE's value, CHR$, TAB, SPC, ON): I8
    ir::Operand toByte(const Val& v) {
        if (v.k == VK::S) fail("a number expected, not a string (?TYPE MISMATCH)");
        if ((v.k == VK::I || v.k == VK::F) && v.isConst) {
            double f = std::floor(v.cval);
            if (f < 0 || f > 255) { raiseNow(14); return iconst(ir::Type::I8, 0); }
            return iconst(ir::Type::I8, (int64_t)f);
        }
        if (v.k == VK::I) {
            checkRange(v, 0, 255);
            return trunc(v.op, ir::Type::I8);
        }
        if (v.k == VK::B) {
            // -1 (true) is out of range; 0 is 0
            raiseIf(v.op, 14);
            return iconst(ir::Type::I8, 0);
        }
        return trunc(viaN(v, "__bas_getbyt"), ir::Type::I8);
    }
    // An address 0..65535 (POKE, PEEK): PTR
    ir::Operand toAddr(const Val& v) {
        if (v.k == VK::S) fail("a number expected, not a string (?TYPE MISMATCH)");
        if ((v.k == VK::I || v.k == VK::F) && v.isConst) {
            double f = std::floor(v.cval);
            if (f < 0 || f > 65535) { raiseNow(14); return iconst(ir::Type::PTR, 0); }
            return iconst(ir::Type::PTR, (int64_t)f);
        }
        if (v.k == VK::I) {
            checkRange(v, 0, 65535);
            return trunc(v.op, ir::Type::PTR);
        }
        store(ir::Type::F32, toF(v), global("__float_a"));
        call("__bas_getadr");
        return load(ir::Type::PTR, global("__bas_n"));
    }
    // PEEK / POKE / WPEEK / WPOKE's address: 0..2^32-1 (BASIC 65's getadr:
    // above 65535 a 28-bit address, below it the bank BANK chose, 128 the
    // CPU's map). Near (the CPU's map): the address known to be below 65536
    // and no BANK in the program; else the 32-bit address in __bas_fa for
    // __bas_peek / __bas_poke
    struct MemAddr { bool nearMem = true; ir::Operand p; };
    MemAddr memAddr(const Val& v) {
        MemAddr m;
        if (v.k == VK::S) fail("a number expected, not a string (?TYPE MISMATCH)");
        bool fits = (v.isConst && v.cval >= 0 && v.cval < 65536) || (v.k == VK::I && !v.isConst && v.bound <= 65535) ||
                    v.k == VK::B;
        if (fits && !farMem_) { m.p = toAddr(v); return m; }
        m.nearMem = false;
        if (v.isConst) {
            double f = std::floor(v.cval);
            if (f < 0 || f >= 4294967296.0) { raiseNow(14); f = 0; }
            store(ir::Type::I32, iconst(ir::Type::I32, (int64_t)(uint32_t)(uint64_t)f), global("__bas_fa"));
        } else if (v.k == VK::I || v.k == VK::B) {
            Val x = v.k == VK::B ? toI16(v) : v;
            checkRange(x, 0, INT32_MAX);
            store(ir::Type::I32, asType(x, ir::Type::I32), global("__bas_fa"));
        } else {
            store(ir::Type::F32, toF(v), global("__float_a"));
            call("__bas_getadr32");
        }
        return m;
    }
    // the next byte / word of a far access: __bas_fa + k
    void nextFar(int k) {
        store(ir::Type::I32, bin(ir::Op::ADD, ir::Type::I32, load(ir::Type::I32, global("__bas_fa")),
                                 ir::Operand::imm(k, ir::Type::I32)), global("__bas_fa"));
    }
    ir::Operand peekAt(const MemAddr& m, int width) {
        if (m.nearMem) return load(width == 1 ? ir::Type::I8 : ir::Type::I16, m.p);
        store(ir::Type::I16, iconst(ir::Type::I16, width), global("__bas_m"));
        call("__bas_peek");
        auto n = load(ir::Type::I16, global("__bas_n"));
        return width == 1 ? trunc(n, ir::Type::I8) : n;
    }
    void pokeAt(const MemAddr& m, int k, int width, ir::Operand value16) {
        if (m.nearMem) {
            ir::Operand a = k ? addrElem(m.p, ir::Operand::imm(k, ir::Type::I16), 1) : m.p;
            if (width == 1) store(ir::Type::I8, trunc(value16, ir::Type::I8), a);
            else store(ir::Type::I16, value16, a);
            return;
        }
        if (k) nextFar(width);
        store(ir::Type::I16, value16, global("__bas_n"));
        store(ir::Type::I16, iconst(ir::Type::I16, width), global("__bas_m"));
        call("__bas_poke");
    }
    bool farMem_ = false;    // a BANK in the program
    bool usesTrap_ = false, resumeStmt_ = false, erVars_ = false;
    bool cmdUsed_ = false;   // a CMD in the program
    std::set<size_t> trapTargets_;     // the lines TRAPs go to

    // A condition's truth: I8, nonzero when true
    ir::Operand truth(const Val& v) {
        if (v.k == VK::S) fail("a number expected, not a string (?TYPE MISMATCH)");
        if (v.k == VK::B) return v.op;
        if (v.k == VK::I) return bin(ir::Op::CMP_NE, ir::Type::I8, v.op, ir::Operand::imm(0, v.op.type));
        auto c = bin(ir::Op::FCMP, ir::Type::I8, v.op, fconst(0));
        return bin(ir::Op::CMP_NE, ir::Type::I8, c, ir::Operand::imm(0, ir::Type::I8));
    }

    // --- strings ---
    int newSlot() {
        if (slots_ >= MAX_SLOTS) fail("the string expression is too complex (more than 8 strings at once)");
        maxSlots_ = std::max(maxSlots_, slots_ + 1);
        return slots_++;
    }
    void freeVal(const Val& v) { if (v.k == VK::S && v.slot >= 0) slots_--; }
    static std::string slotName(int s) { return "__bas_t" + std::to_string(s); }
    Val strLit(const std::string& text) {
        if (text.size() > 255) fail("a string longer than 255 characters");
        auto it = strLits_.find(text);
        std::string g;
        if (it != strLits_.end()) g = it->second;
        else {
            g = "__bas_c" + std::to_string(strLits_.size());
            strLits_[text] = g;
            ir::Module::GlobalVar gv;
            gv.name = g; gv.type = ir::Type::I8; gv.size = (int)text.size() + 1; gv.isConst = true;
            gv.hasInitValue = true; gv.hasImage = true;
            gv.initBytes.push_back((uint8_t)text.size());
            for (char c : text) gv.initBytes.push_back((uint8_t)c);
            mod_->globals.push_back(gv);
        }
        Val v; v.k = VK::S;
        v.op = vreg(ir::Type::PTR);
        ir::Inst in; in.op = ir::Op::ADDR_GLOBAL; in.dest = v.op; in.src1 = ir::Operand::global(g);
        in.resultType = ir::Type::PTR; emit(in);
        return v;
    }
    ir::Operand slotAddr(int s) {
        auto d = vreg(ir::Type::PTR);
        ir::Inst in; in.op = ir::Op::ADDR_GLOBAL; in.dest = d; in.src1 = ir::Operand::global(slotName(s));
        in.resultType = ir::Type::PTR; emit(in);
        return d;
    }
    // A string routine: arguments a (and b) freed, the result in a new slot
    Val strCall(const std::string& f, const Val* a, const Val* b) {
        if (a) store(ir::Type::PTR, a->op, global("__bas_sa"));
        if (b) store(ir::Type::PTR, b->op, global("__bas_sb"));
        if (b) freeVal(*b);
        if (a) freeVal(*a);
        Val r; r.k = VK::S; r.slot = newSlot();
        r.op = slotAddr(r.slot);
        store(ir::Type::PTR, r.op, global("__bas_sr"));
        call(f);
        return r;
    }
    Val needS(const Val& v) {
        if (v.k != VK::S) fail("a string expected, not a number (?TYPE MISMATCH)");
        return v;
    }

    // --- expressions ---
    Val expr() { return orExpr(); }
    Val orExpr() {
        Val a = andExpr();
        for (;;) {
            // (XOR: OR's precedence)
            if (acceptKw(T_OR)) a = logic(ir::Op::OR, a, andExpr());
            else if (acceptKw(T_XOR)) a = logic(ir::Op::XOR, a, andExpr());
            else return a;
        }
    }
    Val andExpr() {
        Val a = notExpr();
        while (acceptKw(T_AND)) {
            Val b = notExpr();
            a = logic(ir::Op::AND, a, b);
        }
        return a;
    }
    Val logic(ir::Op op, const Val& a, const Val& b) {
        if (a.k == VK::B && b.k == VK::B) {
            Val r; r.k = VK::B; r.op = bin(op, ir::Type::I8, a.op, b.op);
            return r;
        }
        // (BASIC 65's ayintw: -32768..65535, the result 16 bits, signed)
        Val x = toI16w(a), y = toI16w(b);
        Val r; r.k = VK::I; r.bound = 32768;
        r.op = bin(op, ir::Type::I16, x.op, y.op);
        return r;
    }
    Val notExpr() {
        if (acceptKw(T_NOT)) {
            Val a = notExpr();
            if (a.k == VK::B) {
                Val r; r.k = VK::B; r.op = bin(ir::Op::XOR, ir::Type::I8, a.op, ir::Operand::imm(1, ir::Type::I8));
                return r;
            }
            Val x = toI16w(a);
            Val r; r.k = VK::I; r.bound = 32768;
            r.op = bin(ir::Op::XOR, ir::Type::I16, x.op, ir::Operand::imm(-1, ir::Type::I16));
            return r;
        }
        return relExpr();
    }
    // << and >> (the tokens $FE52, $FE53): between the comparisons and + -;
    // both operands BASIC's AYINT (-32768..32767), the count 0..15; the
    // result 16 bits, signed (>> keeps the sign)
    Val shiftExpr() {
        Val a = addExpr();
        while (isKw(T_ASL) || isKw(T_ASR)) {
            bool left = isKw(T_ASL);
            p_++;
            Val b = addExpr();
            Val n = toI16(b);
            if (n.isConst) {
                if (n.cval < 0 || n.cval > 15) { raiseNow(14); n = constI(0); }
            } else {
                raiseIf(bin(ir::Op::CMP_LTU, ir::Type::I8, ir::Operand::imm(15, ir::Type::I16), n.op), 14);
            }
            Val x = toI16(a);
            Val r; r.k = VK::I; r.bound = 32768;
            r.op = bin(left ? ir::Op::SHL : ir::Op::ASR, ir::Type::I16, asType(x, ir::Type::I16), asType(n, ir::Type::I16));
            a = r;
        }
        return a;
    }
    Val relExpr() {
        Val a = shiftExpr();
        for (;;) {
            int rel = 0;    // bits: 1 <, 2 =, 4 >
            while (true) {
                if (acceptKw(T_LT)) rel |= 1;
                else if (acceptKw(T_EQ)) rel |= 2;
                else if (acceptKw(T_GT)) rel |= 4;
                else break;
            }
            if (!rel) return a;
            Val b = shiftExpr();
            a = compare(rel, a, b);
        }
    }
    Val compare(int rel, const Val& a, const Val& b) {
        if (rel == 7) fail("<=> is not a comparison");
        Val r; r.k = VK::B;
        if (a.k == VK::S || b.k == VK::S) {
            needS(a); needS(b);
            store(ir::Type::PTR, a.op, global("__bas_sa"));
            store(ir::Type::PTR, b.op, global("__bas_sb"));
            freeVal(b); freeVal(a);
            call("__bas_scmp");
            auto c = trunc(load(ir::Type::I16, global("__bas_n")), ir::Type::I8);
            r.op = cmpResult(rel, c);
            return r;
        }
        if (a.k != VK::F && b.k != VK::F) {
            // (in the wider of the two types)
            Val xa = a.k == VK::B ? toI16(a) : a, ya = b.k == VK::B ? toI16(b) : b;
            ir::Type t = xa.op.type == ir::Type::I32 || ya.op.type == ir::Type::I32 ? ir::Type::I32 : ir::Type::I16;
            ir::Operand x = asType(xa, t), y = asType(ya, t);
            static const ir::Op ops[] = {ir::Op::NOP, ir::Op::CMP_LT, ir::Op::CMP_EQ, ir::Op::CMP_LE,
                                         ir::Op::CMP_GT, ir::Op::CMP_NE, ir::Op::CMP_GE};
            r.op = bin(ops[rel], ir::Type::I8, x, y);
            return r;
        }
        auto c = bin(ir::Op::FCMP, ir::Type::I8, toF(a), toF(b));
        r.op = cmpResult(rel, c);
        return r;
    }
    // FCMP's / __bas_scmp's -1 ($FF), 0, 1 to the relation
    ir::Operand cmpResult(int rel, ir::Operand c) {
        auto imm = [](int v) { return ir::Operand::imm(v, ir::Type::I8); };
        switch (rel) {
        case 1: return bin(ir::Op::CMP_EQ, ir::Type::I8, c, imm(0xFF));
        case 2: return bin(ir::Op::CMP_EQ, ir::Type::I8, c, imm(0));
        case 3: return bin(ir::Op::CMP_NE, ir::Type::I8, c, imm(1));
        case 4: return bin(ir::Op::CMP_EQ, ir::Type::I8, c, imm(1));
        case 5: return bin(ir::Op::CMP_NE, ir::Type::I8, c, imm(0));
        default: return bin(ir::Op::CMP_NE, ir::Type::I8, c, imm(0xFF));
        }
    }
    Val addExpr() {
        Val a = mulExpr();
        for (;;) {
            if (acceptKw(T_PLUS)) {
                Val b = mulExpr();
                if (a.k == VK::S || b.k == VK::S) { needS(a); needS(b); a = strCall("__bas_scat", &a, &b); }
                else a = arith(ir::Op::ADD, a, b);
            } else if (acceptKw(T_MINUS)) {
                a = arith(ir::Op::SUB, a, mulExpr());
            } else {
                return a;
            }
        }
    }
    Val mulExpr() {
        Val a = unaryExpr();
        for (;;) {
            if (acceptKw(T_MUL)) a = arith(ir::Op::MUL, a, unaryExpr());
            else if (acceptKw(T_DIV)) a = arith(ir::Op::DIV, a, unaryExpr());
            else return a;
        }
    }
    Val unaryExpr() {
        if (acceptKw(T_MINUS)) {
            Val a = unaryExpr();
            if (a.k == VK::S) fail("a number expected, not a string (?TYPE MISMATCH)");
            if (a.k == VK::B) a = toI16(a);
            if (a.k == VK::I) {
                if (a.isConst) return constI(-(int64_t)a.cval);
                ir::Type t = intType(a.bound);      // (-(-32768) needs 32 bits)
                Val r = a; r.op = un(ir::Op::NEG, t, asType(a, t));
                return r;
            }
            Val r; r.k = VK::F;
            if (a.isConst) { r.isConst = true; r.cval = -a.cval; r.op = fconst(-a.cval); return r; }
            r.op = un(ir::Op::FNEG, ir::Type::F32, a.op);
            return r;
        }
        if (acceptKw(T_PLUS)) return unaryExpr();
        return powExpr();
    }
    Val powExpr() {
        Val a = primary();
        while (acceptKw(T_POW)) {
            Val b;
            if (isKw(T_MINUS) || isKw(T_PLUS)) b = unaryExpr();
            else b = primary();
            // (both converted first: ITOF works in __float_a)
            auto fa = toF(a), fb = toF(b);
            store(ir::Type::F32, fa, global("__float_a"));
            store(ir::Type::F32, fb, global("__float_b"));
            call("__bas_pow");
            a = Val{};
            a.k = VK::F;
            a.op = load(ir::Type::F32, global("__float_a"));
        }
        return a;
    }
    Val arith(ir::Op op, const Val& a, const Val& b) {
        if (a.k == VK::S || b.k == VK::S) fail("a number expected, not a string (?TYPE MISMATCH)");
        Val x = a.k == VK::B ? toI16(a) : a;
        Val y = b.k == VK::B ? toI16(b) : b;
        if (x.k == VK::I && y.k == VK::I && op != ir::Op::DIV) {
            int64_t bound = op == ir::Op::MUL ? x.bound * y.bound : x.bound + y.bound;
            if (x.bound <= (1LL << 31) && y.bound <= (1LL << 31) && bound < (1LL << 31)) {
                if (x.isConst && y.isConst) {
                    int64_t u = (int64_t)x.cval, v = (int64_t)y.cval;
                    return constI(op == ir::Op::ADD ? u + v : op == ir::Op::SUB ? u - v : u * v);
                }
                Val r; r.k = VK::I; r.bound = bound;
                ir::Type t = intType(bound);
                r.op = bin(op, t, asType(x, t), asType(y, t));
                return r;
            }
        }
        Val r; r.k = VK::F;
        if (op == ir::Op::DIV) {
            auto fa = toF(x), fb = toF(y);
            store(ir::Type::F32, fa, global("__float_a"));
            store(ir::Type::F32, fb, global("__float_b"));
            call(speed_ ? "__bas_fdivx" : "__bas_fdiv");
            r.op = load(ir::Type::F32, global("__float_a"));
            return r;
        }
        ir::Op fop = op == ir::Op::ADD ? ir::Op::FADD : op == ir::Op::SUB ? ir::Op::FSUB : ir::Op::FMUL;
        r.op = bin(fop, ir::Type::F32, toF(x), toF(y));
        return r;
    }
    Val number(double n, bool whole) {
        if (whole && n <= 999999999.0) return constI((int64_t)n);
        Val r; r.k = VK::F; r.isConst = true; r.cval = n; r.op = fconst(n);
        return r;
    }
    Val parenExpr() {
        expectCh('(');
        Val v = expr();
        expectCh(')');
        return v;
    }
    Val primary() {
        auto t = peek();
        if (!t) fail("an expression expected" + near());
        if (t->k == TK::Num) { p_++; return number(t->num, t->whole); }
        if (t->k == TK::Str) { p_++; return strLit(t->text); }
        if (t->k == TK::Name) {
            p_++;
            if (isCh('(')) return loadElem(t->text);
            return loadVar(t->text);
        }
        if (t->k == TK::Ch && t->ch == '(') return parenExpr();
        if (t->k == TK::Kw) return function(t->kw);
        fail("an expression expected" + near());
    }
    // A float function: __float_a in and out
    Val floatFn(const std::string& f) {
        Val a = parenExpr();
        store(ir::Type::F32, toF(a), global("__float_a"));
        call(f);
        Val r; r.k = VK::F; r.op = load(ir::Type::F32, global("__float_a"));
        return r;
    }
    Val function(int kw) {
        p_++;
        switch (kw) {
        case T_SQR: return floatFn("__bas_sqr");
        case T_LOG: return floatFn("__bas_log");
        case T_EXP: return floatFn("__bas_exp");
        case T_SIN: return floatFn("__float_sin");
        case T_COS: return floatFn("__float_cos");
        case T_TAN: return floatFn("__float_tan");
        case T_ATN: return floatFn("__float_atn");
        case T_RND: return floatFn("__bas_rnd");
        case T_POS: {
            parenExpr();     // (a dummy argument)
            call("__bas_pos");
            Val r; r.k = VK::I; r.bound = 255; r.op = load(ir::Type::I16, global("__bas_n"));
            return r;
        }
        case T_VAL: case T_DEC: {
            Val a = needS(parenExpr());
            store(ir::Type::PTR, a.op, global("__bas_sa"));
            freeVal(a);
            call(kw == T_VAL ? "__bas_val" : "__bas_dec");
            Val r; r.k = VK::F; r.op = load(ir::Type::F32, global("__float_a"));
            return r;
        }
        case T_HEX: case T_STRBIN: {
            Val a = parenExpr();
            store(ir::Type::F32, toF(a), global("__float_a"));
            store(ir::Type::I16, iconst(ir::Type::I16, kw == T_STRBIN ? 1 : 0), global("__bas_m"));
            return strCall("__bas_hex", nullptr, nullptr);
        }
        case T_INSTR: {
            expectCh('(');
            Val h = needS(expr());
            expectCh(',');
            Val n = needS(expr());
            ir::Operand at;
            if (acceptCh(',')) {
                Val v = expr();
                at = toByte(v);
                // (0 the one position not allowed)
                raiseIf(bin(ir::Op::CMP_EQ, ir::Type::I8, at, ir::Operand::imm(0, ir::Type::I8)), 14);
            } else at = iconst(ir::Type::I8, 1);
            expectCh(')');
            store(ir::Type::I16, zext(at, ir::Type::I8, ir::Type::I16), global("__bas_n"));
            store(ir::Type::PTR, h.op, global("__bas_sa"));
            store(ir::Type::PTR, n.op, global("__bas_sb"));
            freeVal(n); freeVal(h);
            call("__bas_instr");
            Val r; r.k = VK::I; r.bound = 255; r.op = load(ir::Type::I16, global("__bas_n"));
            return r;
        }
        case T_MOD: {
            // number - range * INT(number / range), as the ROM computes it
            expectCh('(');
            Val a = expr();
            expectCh(',');
            Val b = expr();
            expectCh(')');
            Val q = arith(ir::Op::DIV, a, b);
            store(ir::Type::F32, toF(q), global("__float_a"));
            call("__bas_int");
            Val qi; qi.k = VK::F; qi.op = load(ir::Type::F32, global("__float_a"));
            return arith(ir::Op::SUB, a, arith(ir::Op::MUL, b, qi));
        }
        case T_WPEEK: {
            Val a = parenExpr();
            auto w = peekAt(memAddr(a), 2);
            Val r; r.k = VK::I; r.bound = 65535; r.op = zext(w, ir::Type::I16, ir::Type::I32);
            return r;
        }
        case T_RPT: {
            expectCh('(');
            Val a = needS(expr());
            expectCh(',');
            Val n = expr();
            expectCh(')');
            store(ir::Type::I16, zext(toByte(n), ir::Type::I8, ir::Type::I16), global("__bas_n"));
            return strCall("__bas_rpt", &a, nullptr);
        }
        case T_FN: return fnCall();
        case T_ERRS: {
            Val n = toI16(parenExpr());
            store(ir::Type::I16, asType(n, ir::Type::I16), global("__bas_n"));
            return strCall("__bas_errs", nullptr, nullptr);
        }
        case T_ABS: {
            Val a = parenExpr();
            if (a.k == VK::I && !a.isConst) {
                // |x| without the float runtime (|-32768| needs 32 bits)
                ir::Type t = intType(a.bound);
                auto x = asType(a, t);
                auto neg = bin(ir::Op::CMP_LT, ir::Type::I8, x, ir::Operand::imm(0, t));
                auto r = vreg(t);
                std::string n = newLabel("absn"), d = newLabel("absd"), p = newLabel("absp");
                brCond(neg, n, p);
                block(n);
                { ir::Inst in; in.op = ir::Op::NEG; in.dest = r; in.src1 = x; in.resultType = t; emit(in); }
                br(d);
                block(p);
                { ir::Inst in; in.op = ir::Op::COPY; in.dest = r; in.src1 = x; in.resultType = t; emit(in); }
                block(d);
                Val v = a; v.op = r;
                return v;
            }
            if (a.k == VK::I) return constI((int64_t)std::fabs(a.cval));
            store(ir::Type::F32, toF(a), global("__float_a"));
            call("__float_abs");
            Val r; r.k = VK::F; r.op = load(ir::Type::F32, global("__float_a"));
            return r;
        }
        case T_INT: {
            Val a = parenExpr();
            if (a.k == VK::I) return a;
            store(ir::Type::F32, toF(a), global("__float_a"));
            call("__bas_int");
            Val r; r.k = VK::F; r.op = load(ir::Type::F32, global("__float_a"));
            return r;
        }
        case T_SGN: {
            Val a = parenExpr();
            ir::Operand c;
            if (a.k == VK::S) fail("a number expected, not a string (?TYPE MISMATCH)");
            if (a.k == VK::F) c = bin(ir::Op::FCMP, ir::Type::I8, a.op, fconst(0));
            else {
                // -1, 0, 1: (x > 0) - (x < 0), in the value's own type
                Val x = a.k == VK::B ? toI16(a) : a;
                auto gt = bin(ir::Op::CMP_GT, ir::Type::I8, x.op, ir::Operand::imm(0, x.op.type));
                auto lt = bin(ir::Op::CMP_LT, ir::Type::I8, x.op, ir::Operand::imm(0, x.op.type));
                c = bin(ir::Op::SUB, ir::Type::I8, gt, lt);
            }
            Val r; r.k = VK::I; r.bound = 1; r.op = sext(c, ir::Type::I8, ir::Type::I16);
            return r;
        }
        case T_PEEK: {
            Val a = parenExpr();
            auto b = peekAt(memAddr(a), 1);
            Val r; r.k = VK::I; r.bound = 255; r.op = zext(b, ir::Type::I8, ir::Type::I16);
            return r;
        }
        case T_LEN: case T_ASC: {
            Val a = needS(parenExpr());
            ir::Operand p = a.op;
            if (kw == T_ASC) {
                // the first character; ?ILLEGAL QUANTITY for ""
                auto len = load(ir::Type::I8, p);
                raiseIf(bin(ir::Op::CMP_EQ, ir::Type::I8, len, ir::Operand::imm(0, ir::Type::I8)), 14);
                ir::Inst in; in.op = ir::Op::ADDR_ELEM; in.dest = vreg(ir::Type::PTR); in.src1 = p;
                in.src2 = ir::Operand::imm(1, ir::Type::I16); in.args.push_back(ir::Operand::imm(1, ir::Type::I16));
                in.resultType = ir::Type::PTR;
                p = in.dest;
                emit(in);
            }
            auto b = load(ir::Type::I8, p);
            freeVal(a);
            Val r; r.k = VK::I; r.bound = 255; r.op = zext(b, ir::Type::I8, ir::Type::I16);
            return r;
        }
        case T_CHR: {
            Val a = parenExpr();
            auto b = toByte(a);
            store(ir::Type::I16, zext(b, ir::Type::I8, ir::Type::I16), global("__bas_n"));
            return strCall("__bas_chr", nullptr, nullptr);
        }
        case T_STR: {
            Val a = parenExpr();
            store(ir::Type::F32, toF(a), global("__float_a"));
            return strCall("__bas_str", nullptr, nullptr);
        }
        case T_LEFT: case T_RIGHT: case T_MID: {
            expectCh('(');
            Val s = needS(expr());
            expectCh(',');
            Val n = expr();
            Val m;
            bool hasM = false;
            if (kw == T_MID && acceptCh(',')) { m = expr(); hasM = true; }
            expectCh(')');
            auto nb = toByte(n);
            store(ir::Type::I16, zext(nb, ir::Type::I8, ir::Type::I16), global("__bas_n"));
            if (kw == T_MID) {
                ir::Operand mb = hasM ? zext(toByte(m), ir::Type::I8, ir::Type::I16) : iconst(ir::Type::I16, 255);
                store(ir::Type::I16, mb, global("__bas_m"));
            }
            return strCall(kw == T_LEFT ? "__bas_left" : kw == T_RIGHT ? "__bas_right" : "__bas_mid", &s, nullptr);
        }
        default:
            fail(tokenName(kw) + " is not supported by --compile yet");
        }
    }

    // --- statements ---
    void statement();
    void assignment(const std::string& name);
    // a place a value is stored: a scalar (its global) or an element (its address)
    struct LV { VK k = VK::F; bool elem = false; std::string g; ir::Operand addr; };
    LV parseLV(const std::string& name);
    void assignLV(const LV& lv, const Val& v);
    LV listVar(const char* stmt);
    void inputStmt(bool chan = false);
    void chanArg(const char* routine);
    void openStmt();
    void dosStmt(int kw);
    Val optValue();
    void doStmt();
    // DEF FN name(x) = expr: each FN name(...) is the expression compiled where
    // it is used, x bound to the argument; a flag set when the DEF runs
    // (FN before it: ?UNDEFINED FUNCTION, as BASIC 65's)
    struct DefFn { int id; size_t li, si, body; std::string param; bool twice = false; };
    std::map<std::string, DefFn> defFns_;
    std::map<std::string, Val> fnBind_;     // a parameter's global -> the argument
    int fnDepth_ = 0;
    void defStmt();
    Val fnCall();
    void readStmt();
    void restoreStmt();
    // DATA: the statements' text in program order ([length][text] each,
    // $FF at the end: __bas_data) and the offset of each line's first one
    std::vector<uint8_t> dataImage_;
    std::vector<size_t> dataAt_;         // line index -> offset of the first DATA at or after it
    void loopStmt();
    void exitStmt();
    // after the LOOP that ends the DO loop around token k of statement (li, si)
    // (the ROM's skip_to_loop: a scan of the text, nested DO ... LOOPs passed)
    std::string loopTarget(size_t li, size_t si, size_t k);
    void getStmt();
    void printStmt();
    void ifStmt();
    bool shortCircuits(size_t a, size_t b) const;
    void condJump(const std::string& t, const std::string& f);
    void forStmt();
    void nextStmt();
    void gosubTo(size_t target);
    void returnStmt();
    void onStmt();
    std::string falseTarget(bool begin);
    // The statement after the BEND matching the BEGIN at (li, si) (its
    // token p): (line, statement)
    std::pair<size_t, size_t> findBend(size_t li, size_t si, size_t p);
    // The label of the statement after (li, si) in this line, or of the next line
    std::string nextStmtLabel(size_t li, size_t si) {
        if (si + 1 < lines_[li].segs.size()) return stmtLabel(li, si + 1);
        return afterLine(li);
    }
    // From statement (li, si) on: the ELSE body of the first ELSE in the
    // line, or the next line
    std::string elseFrom(size_t li, size_t si) {
        for (size_t s = si; s < lines_[li].segs.size(); s++) {
            const auto& t = lines_[li].segs[s].toks;
            if (!t.empty() && t[0].k == TK::Kw && t[0].kw == T_ELSE) return elseLabel(li, s);
        }
        return afterLine(li);
    }
    void prescan();
    void finish();
};

void Compiler::prescan() {
    for (size_t li = 0; li < lines_.size(); li++) lineIndex_[lines_[li].num] = li;
    for (const auto& l : lines_)
        for (const auto& sg : l.segs)
            for (const auto& t : sg.toks)
                if (t.k == TK::Kw && t.kw == T_BANK) farMem_ = true;
    // TRAP; RESUME going back to a statement (none or NEXT after it)
    for (const auto& l : lines_)
        for (const auto& sg : l.segs)
            for (size_t k = 0; k < sg.toks.size(); k++) {
                const Tok& t = sg.toks[k];
                if (t.k != TK::Kw) continue;
                if (t.kw == T_TRAP) usesTrap_ = true;
                if (t.kw == T_CMD) cmdUsed_ = true;
                if (t.kw == T_RESUME && !(k + 1 < sg.toks.size() && sg.toks[k + 1].k == TK::Num)) resumeStmt_ = true;
            }
    // the DATA statements (a statement's first token)
    dataAt_.assign(lines_.size() + 1, 0);
    for (size_t li = 0; li < lines_.size(); li++) {
        dataAt_[li] = dataImage_.size();
        for (const auto& sg : lines_[li].segs)
            if (!sg.toks.empty() && sg.toks[0].k == TK::Kw && sg.toks[0].kw == T_DATA) {
                const std::string& t = sg.toks[0].text;
                dataImage_.push_back((uint8_t)std::min<size_t>(t.size(), 254));
                for (size_t k = 0; k < t.size() && k < 254; k++) dataImage_.push_back((uint8_t)t[k]);
            }
    }
    dataAt_[lines_.size()] = dataImage_.size();
    dataImage_.push_back(0xFF);
    // the FOR statements: their numbers, their variables' numbers
    for (size_t li = 0; li < lines_.size(); li++)
        for (size_t si = 0; si < lines_[li].segs.size(); si++) {
            const auto& t = lines_[li].segs[si].toks;
            // (FOR after THEN / ELSE too)
            for (size_t k = 0; k < t.size(); k++) {
                if (t[k].k != TK::Kw || t[k].kw != T_FOR) continue;
                if (k + 1 >= t.size() || t[k + 1].k != TK::Name) continue;
                ForInfo f;
                f.id = (int)fors_.size() + 1;
                std::string name = t[k + 1].text;
                if (name.back() == '%' || name.back() == '$') continue;   // (an error when compiled)
                std::string key = name.substr(0, 2);
                f.var = "__bv_" + key;
                if (!forVarIndex_.count(f.var)) forVarIndex_[f.var] = (int)forVarIndex_.size() + 1;
                f.varIndex = forVarIndex_[f.var];
                if (f.id > 255) { errors.push_back({lines_[li].num, "more than 255 FOR statements"}); return; }
                // STEP: a constant (a number, maybe negative) or not
                size_t j = k + 1;
                while (j < t.size() && !(t[j].k == TK::Kw && t[j].kw == T_STEP)) j++;
                if (j < t.size()) {
                    size_t q = j + 1;
                    double sign = 1;
                    if (q < t.size() && t[q].k == TK::Kw && (t[q].kw == T_MINUS || t[q].kw == T_PLUS)) {
                        if (t[q].kw == T_MINUS) sign = -1;
                        q++;
                    }
                    if (q + 1 == t.size() && t[q].k == TK::Num) f.step = sign * t[q].num;
                    else f.stepConst = false;
                }
                f.li = li;
                f.si = si;
                f.lead = k == 0;
                forAt_[{li, si * 1000 + k}] = f.id;
                fors_.push_back(f);
            }
        }
    countedLoops();
    // DEF FN name(x) = expr
    for (size_t li = 0; li < lines_.size(); li++)
        for (size_t si = 0; si < lines_[li].segs.size(); si++) {
            const auto& t = lines_[li].segs[si].toks;
            if (t.size() < 8 || t[0].k != TK::Kw || t[0].kw != T_DEF || t[1].k != TK::Kw || t[1].kw != T_FN ||
                t[2].k != TK::Name || !(t[3].k == TK::Ch && t[3].ch == '(') || t[4].k != TK::Name ||
                !(t[5].k == TK::Ch && t[5].ch == ')') || t[6].k != TK::Kw || t[6].kw != T_EQ)
                continue;
            std::string key = t[2].text.substr(0, 2);
            auto it = defFns_.find(key);
            if (it != defFns_.end()) { it->second.twice = true; continue; }
            DefFn d;
            d.id = (int)defFns_.size();
            d.li = li; d.si = si; d.body = 7;
            d.param = "__bv_" + t[4].text.substr(0, 2);
            defFns_[key] = d;
        }
}

void Compiler::defStmt() {
    auto t = toks_;
    if (t->size() < 8 || (*t)[1].k != TK::Kw || (*t)[1].kw != T_FN || (*t)[2].k != TK::Name)
        fail("DEF FN name(variable) = expression expected (?SYNTAX ERROR)");
    std::string key = (*t)[2].text.substr(0, 2);
    if ((*t)[2].text.back() == '$' || (*t)[4].text.back() == '$' || (*t)[4].text.back() == '%')
        fail("DEF FN of a string or with an integer parameter is not supported by --compile yet");
    const DefFn& d = defFns_.at(key);
    if (d.twice) fail("FN " + (*t)[2].text + " is defined more than once: not supported by --compile");
    store(ir::Type::I8, iconst(ir::Type::I8, 1), global("__bas_fn" + std::to_string(d.id)));
    p_ = t->size();
}

Val Compiler::fnCall() {
    auto t = peek();
    if (!t || t->k != TK::Name) fail("a function name expected after FN");
    p_++;
    std::string key = t->text.substr(0, 2);
    expectCh('(');
    Val arg = expr();
    expectCh(')');
    auto it = defFns_.find(key);
    if (it == defFns_.end()) { raiseNow(27); return constI(0); }
    const DefFn& d = it->second;
    if (d.twice) fail("FN " + t->text + " is defined more than once: not supported by --compile");
    if (arg.k == VK::S) fail("FN needs a number (?TYPE MISMATCH)");
    if (++fnDepth_ > 16) fail("FN " + t->text + " calls itself");
    // (DEF not run yet: ?UNDEFINED FUNCTION)
    raiseIf(bin(ir::Op::CMP_EQ, ir::Type::I8, load(ir::Type::I8, global("__bas_fn" + std::to_string(d.id))),
                ir::Operand::imm(0, ir::Type::I8)), 27);
    // the expression, its parameter the argument
    auto savedToks = toks_;
    size_t savedP = p_;
    auto savedBind = fnBind_.find(d.param) != fnBind_.end() ? std::optional<Val>(fnBind_[d.param]) : std::nullopt;
    fnBind_[d.param] = arg;
    toks_ = &lines_[d.li].segs[d.si].toks;
    p_ = d.body;
    Val r = expr();
    if (!atEnd()) fail("unexpected" + near() + " in DEF FN's expression (?SYNTAX ERROR)");
    toks_ = savedToks;
    p_ = savedP;
    if (savedBind) fnBind_[d.param] = *savedBind; else fnBind_.erase(d.param);
    fnDepth_--;
    return r;
}

// A FOR loop compiled as a counted loop (its NEXT an inline step, no entry
// on BASIC's stack) does what BASIC's stack would when nothing can reach
// the stack's view of it: the FOR and its NEXT statements of their own
// (not after THEN), the NEXT the first one matching it in the text (nested
// loops' NEXTs before it), and in between no way out of the loop or into it:
// no GOTO, GOSUB, ON, RETURN, DO / LOOP / EXIT, BEGIN / BEND, THEN / ELSE
// with a line number, no FOR of the same variable, no line a jump goes to,
// no IF before the NEXT in its line (its false branch would skip it), and
// only counted loops inside.
void Compiler::countedLoops() {
    // the lines jumps go to
    std::set<size_t> targets;
    for (size_t li = 0; li < lines_.size(); li++)
        for (const auto& sg : lines_[li].segs) {
            const auto& t = sg.toks;
            for (size_t k = 0; k + 1 < t.size(); k++) {
                if (t[k].k != TK::Kw) continue;
                int kw = t[k].kw;
                bool goTo = kw == T_TO && k > 0 && t[k - 1].k == TK::Kw && t[k - 1].kw == T_GO;
                if (kw == T_GOTO || kw == T_GOSUB || kw == T_THEN || kw == T_ELSE || goTo || kw == T_RESTORE) {
                    // (ON n GOTO a, b, c: every number of the list)
                    for (size_t j = k + 1; j < t.size(); j++) {
                        if (t[j].k == TK::Num) {
                            auto it = lineIndex_.find((int)t[j].num);
                            if (it != lineIndex_.end()) targets.insert(it->second);
                        } else if (!(t[j].k == TK::Ch && t[j].ch == ',')) break;
                    }
                }
            }
        }
    auto key = [](const std::string& name) { return "__bv_" + name.substr(0, 2); };
    std::vector<std::tuple<size_t, size_t, int>> nextOf(fors_.size() + 1);
    std::vector<std::vector<int>> inner(fors_.size() + 1);
    for (auto& f : fors_) {
        if (!f.lead) continue;
        std::vector<std::string> nest;     // the loops opened inside, innermost last
        bool ok = true, found = false;
        size_t l = f.li, s = f.si + 1;
        for (; ok && !found && l < lines_.size(); l++, s = 0) {
            if (l > f.li && targets.count(l)) { ok = false; break; }
            bool ifSeen = false;
            for (; ok && !found && s < lines_[l].segs.size(); s++) {
                const auto& t = lines_[l].segs[s].toks;
                if (t.empty()) continue;
                for (size_t k = 0; k < t.size() && ok; k++) {
                    if (t[k].k != TK::Kw) continue;
                    switch (t[k].kw) {
                    case T_GOTO: case T_GO: case T_GOSUB: case T_ON: case T_RETURN: case T_DO: case T_LOOP:
                    case T_EXIT: case T_BEGIN: case T_BEND:
                        ok = false; break;
                    case T_THEN: case T_ELSE:
                        if (k + 1 < t.size() && t[k + 1].k == TK::Num) ok = false;
                        if (t[k].kw == T_THEN) ifSeen = true;
                        break;
                    case T_IF: ifSeen = true; break;
                    case T_FOR:
                        if (k != 0 || k + 1 >= t.size() || t[k + 1].k != TK::Name || key(t[k + 1].text) == f.var) ok = false;
                        else {
                            nest.push_back(key(t[k + 1].text));
                            auto it = forAt_.find({l, s * 1000});
                            if (it != forAt_.end()) inner[f.id].push_back(it->second);
                        }
                        break;
                    case T_NEXT: {
                        if (k != 0) { ok = false; break; }
                        // its variables (none: the innermost loop's)
                        std::vector<std::string> vs;
                        for (size_t j = 1; j < t.size(); j++)
                            if (t[j].k == TK::Name) vs.push_back(key(t[j].text));
                        if (vs.empty()) vs.push_back("");
                        for (size_t j = 0; j < vs.size() && ok && !found; j++) {
                            if (!nest.empty()) {
                                if (!vs[j].empty() && vs[j] != nest.back()) ok = false;
                                else nest.pop_back();
                            } else if (vs[j].empty() || vs[j] == f.var) {
                                if (ifSeen) ok = false;
                                else { found = true; nextOf[f.id] = {l, s, t.size() > 1 ? (int)j : -1}; }
                            } else ok = false;
                        }
                        k = t.size();
                        break;
                    }
                    default: break;
                    }
                }
            }
            if (found) break;
        }
        f.counted = ok && found;
    }
    // only counted loops inside a counted one
    for (bool changed = true; changed;) {
        changed = false;
        for (auto& f : fors_) {
            if (!f.counted) continue;
            for (int in : inner[f.id])
                if (!fors_[in - 1].counted) { f.counted = false; changed = true; break; }
        }
    }
    for (const auto& f : fors_)
        if (f.counted) countedNext_[nextOf[f.id]] = f.id;
}

std::shared_ptr<ir::Module> Compiler::run() {
    mod_ = std::make_shared<ir::Module>();
    mod_->sourceFile = source_;
    mod_->fastFloat = speed_;
    mod_->functions.push_back({});
    fn_ = &mod_->functions.back();
    fn_->name = "_main";
    fn_->returnType = ir::Type::I16;
    fn_->conv = ir::CallConv::ZP;
    fn_->originalIsLeaf = false;
    prescan();
    if (!errors.empty()) return nullptr;
    lineEntries_.assign(lines_.size(), {});
    lineRaises_.assign(lines_.size(), false);

    block("entry");
    li_ = 0;
    // the string variables start empty (__bas_empty): set when the program
    // has been compiled (finish), here
    std::string init = "init";
    br(init);
    block("start");
    for (li_ = 0; li_ < lines_.size(); li_++) {
        block(lineLabel(li_));
        entry();
        for (si_ = 0; si_ < lines_[li_].segs.size(); si_++) {
            block(stmtLabel(li_, si_));
            if (resumeStmt_) {
                // (RESUME: this statement and the next one, should it raise an error)
                for (auto [lbl, g] : {std::pair<std::string, const char*>{stmtLabel(li_, si_), "__bas_spc"},
                                      {nextStmtLabel(li_, si_), "__bas_snext"}}) {
                    auto r = vreg(ir::Type::I16);
                    ir::Inst in; in.op = ir::Op::ADDR_LABEL; in.dest = r; in.src1 = ir::Operand::label(lbl); emit(in);
                    store(ir::Type::I16, r, global(g));
                }
            }
            toks_ = &lines_[li_].segs[si_].toks;
            p_ = 0;
            slots_ = 0;
            try {
                statement();
                if (!atEnd()) fail("unexpected" + near().substr(0) + " (?SYNTAX ERROR)");
            } catch (const CompileError& e) {
                errors.push_back({lines_[li_].num, e.what()});
            }
        }
    }
    li_ = lines_.size();
    block("Lend");
    {
        ir::Inst in; in.op = ir::Op::RET; in.resultType = ir::Type::I16;
        in.src1 = ir::Operand::imm(0, ir::Type::I16); emit(in);
    }
    finish();
    if (!errors.empty()) return nullptr;
    return mod_;
}

ir::Operand Compiler::stepDone(const ForInfo& f) {
    // the loop's variable + step: true when past the limit
    std::string id = std::to_string(f.id);
    ir::Operand c;
    if (narrow_.count(f.var)) {
        // (in the variable's type: its bound allows for the step)
        ir::Type t = narrowType(narrow_.at(f.var));
        auto v = load(t, ir::Operand::global(f.var));
        ir::Operand step = f.stepConst ? ir::Operand::imm((int64_t)f.step, t) : load(t, global("__bas_fs" + id));
        auto v2 = bin(ir::Op::ADD, t, v, step);
        store(t, v2, ir::Operand::global(f.var));
        auto lim = load(t, global("__bas_fl" + id));
        if (f.stepConst && f.step != 0)
            return bin(f.step > 0 ? ir::Op::CMP_GT : ir::Op::CMP_LT, ir::Type::I8, v2, lim);
        auto gt = bin(ir::Op::CMP_GT, ir::Type::I8, v2, lim);
        auto lt = bin(ir::Op::CMP_LT, ir::Type::I8, v2, lim);
        c = bin(ir::Op::SUB, ir::Type::I8, gt, lt);
    } else {
        auto v = load(ir::Type::F32, ir::Operand::global(f.var));
        ir::Operand step = f.stepConst ? fconst(f.step) : load(ir::Type::F32, global("__bas_fs" + id));
        auto v2 = bin(ir::Op::FADD, ir::Type::F32, v, step);
        store(ir::Type::F32, v2, ir::Operand::global(f.var));
        c = bin(ir::Op::FCMP, ir::Type::I8, v2, load(ir::Type::F32, global("__bas_fl" + id)));
    }
    if (f.stepConst) {
        int sg = f.step > 0 ? 1 : f.step < 0 ? 0xFF : 0;
        return bin(ir::Op::CMP_EQ, ir::Type::I8, c, ir::Operand::imm(sg, ir::Type::I8));
    }
    return bin(ir::Op::CMP_EQ, ir::Type::I8, c, load(ir::Type::I8, global("__bas_fg" + id)));
}

void Compiler::stepBlock(const ForInfo& f) {
    // N<id>: the step; not past the limit: the body again (F<id>), else the
    // loop's entry popped and on after the NEXT (__bas_nret)
    block("N" + std::to_string(f.id));
    std::string id = std::to_string(f.id);
    ir::Operand fin = stepDone(f);
    brCond(fin, "P" + id, "F" + id);
    block("P" + id);
    auto sp = load(ir::Type::I8, global("__bas_sp"));
    store(ir::Type::I8, bin(ir::Op::SUB, ir::Type::I8, sp, ir::Operand::imm(2, ir::Type::I8)), global("__bas_sp"));
    auto r = load(ir::Type::PTR, global("__bas_nret"));
    ir::Inst in; in.op = ir::Op::BR_INDIRECT; in.src1 = r; emit(in);
}

void Compiler::finish() {
    // __bas_line at the entries of the lines that can raise an error
    for (size_t li = 0; li < lines_.size(); li++) {
        if (!lineRaises_[li]) continue;
        for (size_t bi : lineEntries_[li]) {
            auto& insts = fn_->blocks[bi].insts;
            ir::Inst c; c.op = ir::Op::CONST; c.dest = vreg(ir::Type::I16);
            c.src1 = ir::Operand::imm(lines_[li].num, ir::Type::I16); c.resultType = ir::Type::I16;
            c.loc.file = source_; c.loc.line = lines_[li].num;
            ir::Inst st; st.op = ir::Op::STORE; st.src1 = c.dest; st.src2 = global("__bas_line");
            st.resultType = ir::Type::I16; st.loc = c.loc;
            insts.insert(insts.begin(), {c, st});
        }
    }
    for (int id : stepBlocks_) stepBlock(fors_[id - 1]);
    // TRAP: an error goes on at the trap's line (__bas_error comes here)
    if (usesTrap_) {
        block("trapgo");
        auto ln = load(ir::Type::I16, global("__bas_trapln"));
        for (size_t t : trapTargets_) {
            std::string nx = newLabel("trapnx");
            brCond(bin(ir::Op::CMP_EQ, ir::Type::I8, ln, ir::Operand::imm(lines_[t].num, ir::Type::I16)), lineLabel(t), nx);
            block(nx);
        }
        br("Lend");
    }
    // the errors' blocks
    for (int n : errBlocks_) {
        block("Err" + std::to_string(n));
        store(ir::Type::I16, iconst(ir::Type::I16, n), global("__bas_n"));
        call("__bas_error");
        ir::Inst in; in.op = ir::Op::RET; in.resultType = ir::Type::I16;
        in.src1 = ir::Operand::imm(n, ir::Type::I16); emit(in);
    }
    // init: the string variables empty; then the program
    block("init");
    store(ir::Type::I8, iconst(ir::Type::I8, 0), global("__bas_sp"));
    if (usesTrap_ || erVars_) {
        store(ir::Type::I16, iconst(ir::Type::I16, -1), global("__bas_er"));
        store(ir::Type::I16, iconst(ir::Type::I16, -1), global("__bas_el"));
    }
    if (usesTrap_) {
        call("__bas_trapinit");
        auto r = vreg(ir::Type::I16);
        ir::Inst in; in.op = ir::Op::ADDR_LABEL; in.dest = r; in.src1 = ir::Operand::label("trapgo"); emit(in);
        store(ir::Type::I16, r, global("__bas_trapv"));
    }
    for (const auto& [g, t] : vars_)
        if (t == ir::Type::PTR) {
            auto e = vreg(ir::Type::PTR);
            ir::Inst in; in.op = ir::Op::ADDR_GLOBAL; in.dest = e; in.src1 = global("__bas_empty");
            in.resultType = ir::Type::PTR; emit(in);
            store(ir::Type::PTR, e, ir::Operand::global(g));
        }
    br("start");

    // the variables
    for (const auto& [g, t] : vars_) {
        ir::Module::GlobalVar gv;
        gv.name = g; gv.type = t; gv.size = ir::typeSize(t);
        mod_->globals.push_back(gv);
    }
    {
        ir::Module::GlobalVar gv;
        gv.name = "__bas_nret"; gv.type = ir::Type::I16; gv.size = 2;
        mod_->globals.push_back(gv);
    }
    {
        ir::Module::GlobalVar gv;
        gv.name = "__bas_data"; gv.type = ir::Type::I8; gv.size = (int)dataImage_.size(); gv.isConst = true;
        gv.hasInitValue = true; gv.hasImage = true; gv.initBytes = dataImage_;
        mod_->globals.push_back(gv);
    }
    for (const auto& [k, d] : defFns_) {
        ir::Module::GlobalVar gv;
        gv.name = "__bas_fn" + std::to_string(d.id); gv.type = ir::Type::I8; gv.size = 1;
        mod_->globals.push_back(gv);
    }
    for (const auto& g : arrays_) {
        ir::Module::GlobalVar gv;
        gv.name = g; gv.type = ir::Type::PTR; gv.size = 2;
        mod_->globals.push_back(gv);
    }
    for (int s = 0; s < maxSlots_; s++) {
        ir::Module::GlobalVar gv;
        gv.name = slotName(s); gv.type = ir::Type::I8; gv.size = 256;
        mod_->globals.push_back(gv);
    }
    // the FOR loops: limits, steps (when not constant), the variables' numbers
    for (const auto& f : fors_) {
        auto n = narrow_.find(f.var);
        ir::Type ft = n != narrow_.end() ? narrowType(n->second) : ir::Type::F32;
        ir::Module::GlobalVar l;
        l.name = "__bas_fl" + std::to_string(f.id); l.type = ft; l.size = ir::typeSize(ft);
        mod_->globals.push_back(l);
        if (!f.stepConst) {
            ir::Module::GlobalVar s;
            s.name = "__bas_fs" + std::to_string(f.id); s.type = ft; s.size = ir::typeSize(ft);
            mod_->globals.push_back(s);
            ir::Module::GlobalVar g;
            g.name = "__bas_fg" + std::to_string(f.id); g.type = ir::Type::I8; g.size = 1;
            mod_->globals.push_back(g);
        }
    }
    {
        ir::Module::GlobalVar fv;
        fv.name = "__bas_forvar"; fv.type = ir::Type::I8; fv.size = (int)fors_.size() + 1; fv.isConst = true;
        fv.hasInitValue = true; fv.hasImage = true;
        fv.initBytes.push_back(0);
        for (const auto& f : fors_) fv.initBytes.push_back((uint8_t)f.varIndex);
        mod_->globals.push_back(fv);
        externs_.insert("__bas_forpush");
    }
    for (const auto& e : externs_) mod_->externs.push_back(e);
}

void Compiler::statement() {
    auto t = peek();
    if (!t) return;     // an empty statement
    if (t->k == TK::Name) { p_++; assignment(t->text); return; }
    if (t->k != TK::Kw) fail("a statement expected" + near() + " (?SYNTAX ERROR)");
    int kw = t->kw;
    p_++;
    switch (kw) {
    case T_LET: {
        auto n = peek();
        if (!n || n->k != TK::Name) fail("a variable expected after LET");
        p_++;
        assignment(n->text);
        return;
    }
    case T_REM: p_ = toks_->size(); return;
    case T_DATA: p_ = toks_->size(); return;
    case T_READ: readStmt(); return;
    case T_RESTORE: restoreStmt(); return;
    case T_DIM: dimStmt(); return;
    case T_INPUT: inputStmt(); return;
    case T_INPUTN: inputStmt(true); return;
    case T_PRINTN: case T_CMD: {
        // PRINT# / CMD lf [, items]: the output to the file; PRINT# then
        // back to the screen (CMD: until a PRINT# or an error)
        bool cmd = kw == T_CMD;
        chanArg("__bas_chkout");
        if (!atEnd() && !isKw(T_ELSE)) expectCh(',');
        printStmt();
        if (!cmd) call("__bas_clrch");
        return;
    }
    case T_OPEN: openStmt(); return;
    case T_DOPEN: case T_APPEND: case T_DCLOSE: dosStmt(kw); return;
    case T_CLOSE: {
        Val v = expr();
        store(ir::Type::I16, zext(toByte(v), ir::Type::I8, ir::Type::I16), global("__bas_n"));
        call("__bas_close");
        return;
    }
    case T_GET: getStmt(); return;
    case T_DO: doStmt(); return;
    case T_DEF: defStmt(); return;
    case T_SCNCLR: {
        if (!atEnd() && !isKw(T_ELSE)) fail("SCNCLR of a graphics screen is not supported by --compile yet");
        Val c = strLit(std::string(1, (char)147));
        store(ir::Type::PTR, c.op, global("__bas_sa"));
        call("__bas_prs");
        return;
    }
    case T_SLEEP: {
        Val v = expr();
        store(ir::Type::F32, toF(v), global("__float_a"));
        call("__bas_sleep");
        return;
    }
    case T_WPOKE: {
        // WPOKE address, word [, word ...]: low byte first
        Val a = expr();
        MemAddr m = memAddr(a);
        int k = 0;
        while (acceptCh(',')) {
            Val v = expr();
            pokeAt(m, 2 * k, 2, toAddr(v));
            k++;
        }
        if (!k) fail("WPOKE needs a value (?SYNTAX ERROR)");
        return;
    }
    case T_WAIT: {
        // until (PEEK(address) XOR x) AND mask is not 0
        Val a = expr();
        MemAddr m = memAddr(a);
        expectCh(',');
        auto mask = toByte(expr());
        ir::Operand x = iconst(ir::Type::I8, 0);
        if (acceptCh(',')) x = toByte(expr());
        std::string loop = newLabel("wait"), done = newLabel("waited");
        br(loop);
        block(loop);
        auto b = peekAt(m, 1);
        auto t = bin(ir::Op::AND, ir::Type::I8, bin(ir::Op::XOR, ir::Type::I8, b, x), mask);
        brCond(bin(ir::Op::CMP_NE, ir::Type::I8, t, ir::Operand::imm(0, ir::Type::I8)), done, loop);
        block(done);
        return;
    }
    case T_LOOP: loopStmt(); return;
    case T_EXIT: exitStmt(); return;
    case T_PRINT: printStmt(); return;
    case T_IF: ifStmt(); return;
    case T_GOTO: br(lineLabel(lineTarget())); block(newLabel("dead")); return;
    case T_GO:
        if (acceptKw(T_TO)) { br(lineLabel(lineTarget())); block(newLabel("dead")); return; }
        fail("GO without TO");
    case T_GOSUB: gosubTo(lineTarget()); return;
    case T_RETURN: returnStmt(); return;
    case T_ON: onStmt(); return;
    case T_FOR: forStmt(); return;
    case T_NEXT: nextStmt(); return;
    case T_END: br("Lend"); block(newLabel("dead")); return;
    case T_STOP: br(errLabel(30)); block(newLabel("dead")); return;
    case T_POKE: {
        // POKE address, value [, value ...]: each value 0..65535, its low
        // byte stored (BASIC 65's comwrd)
        Val a = expr();
        MemAddr m = memAddr(a);
        int k = 0;
        while (acceptCh(',')) {
            Val v = expr();
            pokeAt(m, k, 1, toAddr(v));
            k++;
        }
        if (!k) fail("POKE needs a value (?SYNTAX ERROR)");
        return;
    }
    case T_TRAP: {
        // TRAP line: on; TRAP: off
        if (atEnd() || isKw(T_ELSE)) {
            store(ir::Type::I8, iconst(ir::Type::I8, 0), global("__bas_trapon"));
            return;
        }
        auto t = peek();
        if (!t || t->k != TK::Num) fail("TRAP with a computed line is not supported by --compile");
        size_t target = lineTarget();
        trapTargets_.insert(target);
        store(ir::Type::I16, iconst(ir::Type::I16, lines_[target].num), global("__bas_trapln"));
        store(ir::Type::I8, iconst(ir::Type::I8, 1), global("__bas_trapon"));
        return;
    }
    case T_RESUME: {
        // an error to resume from (else ?CAN'T RESUME): the trap on again, the error cleared
        raiseIf(bin(ir::Op::CMP_EQ, ir::Type::I8, load(ir::Type::I8, global("__bas_pend")),
                    ir::Operand::imm(0, ir::Type::I8)), 31);
        store(ir::Type::I8, iconst(ir::Type::I8, 0), global("__bas_pend"));
        store(ir::Type::I8, load(ir::Type::I8, global("__bas_trapre")), global("__bas_trapon"));
        // (back in the error's line: an error there names it)
        store(ir::Type::I16, load(ir::Type::I16, global("__bas_el")), global("__bas_line"));
        store(ir::Type::I16, iconst(ir::Type::I16, -1), global("__bas_er"));
        store(ir::Type::I16, iconst(ir::Type::I16, -1), global("__bas_el"));
        auto t = peek();
        if (t && t->k == TK::Num) { br(lineLabel(lineTarget())); block(newLabel("dead")); return; }
        bool next = acceptKw(T_NEXT);
        auto a = load(ir::Type::PTR, global(next ? "__bas_rnext" : "__bas_rpc"));
        ir::Inst in; in.op = ir::Op::BR_INDIRECT; in.src1 = a; emit(in);
        block(newLabel("dead"));
        return;
    }
    case T_BANK: {
        // (kept as bank XOR 128: BSS's 0 is BANK 128)
        Val v = expr();
        store(ir::Type::I8, bin(ir::Op::XOR, ir::Type::I8, toByte(v), ir::Operand::imm(0x80, ir::Type::I8)),
              global("__bas_bankx"));
        return;
    }
    case T_ELSE: {
        // reached in order: the rest of the line is skipped (ELSE BEGIN: to
        // after the matching BEND)
        if (isKw(T_BEGIN)) {
            auto [bl, bs] = findBend(li_, si_, p_);
            br(bs < lines_[bl].segs.size() ? stmtLabel(bl, bs) : afterLine(bl));
            p_++;
        } else {
            br(afterLine(li_));
        }
        block(elseLabel(li_, si_));
        auto n = peek();
        if (n && n->k == TK::Num) { br(lineLabel(lineTarget())); block(newLabel("dead")); return; }
        statement();
        return;
    }
    case T_BEGIN: return;     // (THEN BEGIN / ELSE BEGIN: already passed)
    case T_BEND: return;      // the end of a block: nothing to do
    default:
        fail(tokenName(kw) + " is not supported by --compile yet");
    }
}

Compiler::LV Compiler::parseLV(const std::string& name) {
    LV lv;
    if (isCh('(')) {
        lv.elem = true;
        lv.addr = elemAddr(name, lv.k);
    } else {
        lv.g = varGlobal(name, lv.k);
    }
    return lv;
}

void Compiler::assignLV(const LV& lv, const Val& v) {
    if (lv.k == VK::S) {
        needS(v);
        store(ir::Type::PTR, v.op, global("__bas_sa"));
        store(ir::Type::PTR, lv.elem ? lv.addr : addrGlobal(lv.g), global("__bas_sr"));
        call("__bas_sset");
        freeVal(v);
    } else if (lv.k == VK::F) {
        if (lv.elem) { store(ir::Type::F32, toF(v), lv.addr); return; }
        observe(lv.g, v, &VarObs::assign);
        if (narrow_.count(lv.g)) storeNarrow(lv.g, v);
        else store(ir::Type::F32, toF(v), ir::Operand::global(lv.g));
    } else {
        Val x = toI16(v);
        store(ir::Type::I16, asType(x, ir::Type::I16), lv.elem ? lv.addr : ir::Operand::global(lv.g));
    }
}

void Compiler::assignment(const std::string& name) {
    // (an element's address first: the subscripts, the array made, as BASIC's LET)
    LV lv = parseLV(name);
    expectKw(T_EQ);
    assignLV(lv, expr());
}

// The next variable of an INPUT / GET list
Compiler::LV Compiler::listVar(const char* stmt) {
    auto t = peek();
    if (!t || t->k != TK::Name) fail(std::string("a variable expected after ") + stmt);
    p_++;
    return parseLV(t->text);
}

void Compiler::chanArg(const char* routine) {
    Val n = expr();
    store(ir::Type::I16, zext(toByte(n), ir::Type::I8, ir::Type::I16), global("__bas_n"));
    call(routine);
}

// A DOS parameter's value after its letter (U8, D0, L80, UDV, U(X)): the
// rest of the name as the start of the expression, as the ROM reads the
// text after the letter
Val Compiler::optValue() {
    std::string rest = peek()->text.substr(1);
    p_++;
    if (rest.empty()) return expr();
    Tok r;
    if (std::all_of(rest.begin(), rest.end(), [](char c) { return c >= '0' && c <= '9'; })) {
        r.k = TK::Num; r.num = std::stod(rest); r.whole = true;
    } else if (rest[0] >= 'A' && rest[0] <= 'Z') {
        r.k = TK::Name; r.text = rest;
    } else fail("a value expected after the DOS parameter's letter (?SYNTAX ERROR)");
    std::vector<Tok> tmp{r};
    tmp.insert(tmp.end(), toks_->begin() + (long)p_, toks_->end());
    auto saved = toks_;
    size_t at = p_;
    toks_ = &tmp;
    p_ = 0;
    Val v = expr();
    size_t used = p_;
    toks_ = saved;
    p_ = at + used - 1;
    return v;
}

void Compiler::dosStmt(int kw) {
    // DOPEN# lf, name [, D drive] [, U unit] [, W] [, L len] [ON U unit];
    // APPEND# lf, name [, D] [, U]; DCLOSE [# lf] [, U unit]: as the ROM's
    // dosprs (each once, in any order; ON U without a comma too)
    const char* what = kw == T_DOPEN ? "DOPEN" : kw == T_APPEND ? "APPEND" : "DCLOSE";
    int flags = 0;
    bool haveName = false, first = true;
    Val name;
    auto once = [&](int bit) {
        if (flags & bit) fail(std::string(what) + ": a parameter given twice (?SYNTAX ERROR)");
        flags |= bit;
    };
    auto byteTo = [&](const char* g, Val v) { store(ir::Type::I8, toByte(v), global(g)); };
    while (!atEnd() && !isKw(T_ELSE)) {
        if (!first && !isKw(T_ON)) expectCh(',');
        first = false;
        auto t = peek();
        if (!t) fail(std::string(what) + ": a parameter expected (?SYNTAX ERROR)");
        if (acceptCh('#')) {
            once(16);
            byteTo("__bas_dlf", expr());
            continue;
        }
        if (kw != T_DCLOSE && (t->k == TK::Str || isCh('('))) {
            if (haveName) fail(std::string(what) + ": a parameter given twice (?SYNTAX ERROR)");
            haveName = true;
            name = expr();
            if (name.k != VK::S) fail(std::string(what) + "'s file name must be a string (?TYPE MISMATCH)");
            continue;
        }
        bool on = acceptKw(T_ON);
        t = peek();
        if (!t || t->k != TK::Name) fail(std::string(what) + ": this parameter is not supported by --compile" + near() + " (?SYNTAX ERROR)");
        char c = t->text[0];
        if (c == 'U') { once(4); byteTo("__bas_dunit", optValue()); continue; }
        if (on) fail(std::string(what) + ": ON takes U (?SYNTAX ERROR)");
        if (c == 'D' && kw != T_DCLOSE) { once(8); byteTo("__bas_ddrv", optValue()); continue; }
        if (kw == T_DOPEN && t->text == "W") { once(1 | 2); p_++; flags &= ~2; continue; }
        if (kw == T_DOPEN && c == 'L') {
            once(1 | 2);
            flags &= ~1;
            if (t->text == "L" && (p_ + 1 >= toks_->size())) {
                p_++;
                store(ir::Type::I8, ir::Operand::imm(1, ir::Type::I8), global("__bas_drcl"));
            } else byteTo("__bas_drcl", optValue());
            continue;
        }
        fail(std::string(what) + ": this parameter is not supported by --compile" + near() + " (?SYNTAX ERROR)");
    }
    if (kw != T_DCLOSE && (!(flags & 16) || !haveName))
        fail(std::string(what) + " needs # and a file name (?SYNTAX ERROR)");
    if (haveName) store(ir::Type::PTR, name.op, global("__bas_sa"));
    store(ir::Type::I8, ir::Operand::imm(flags, ir::Type::I8), global("__bas_dflg"));
    call(kw == T_DOPEN ? "__bas_dopen" : kw == T_APPEND ? "__bas_append" : "__bas_dclose");
    if (haveName) freeVal(name);
}

void Compiler::openStmt() {
    // OPEN lf [, device [, secondary [, name]]]
    static const char* const g[] = {"__bas_olf", "__bas_odev", "__bas_osa"};
    int n = 0;
    do {
        if (n == 3) break;
        Val v = expr();
        store(ir::Type::I8, toByte(v), global(g[n]));
        n++;
    } while (acceptCh(','));
    ir::Operand nm = ir::Operand::imm(0, ir::Type::PTR);
    Val name;
    if (n == 3 && toks_->at(p_ - 1).k == TK::Ch && toks_->at(p_ - 1).ch == ',') {
        name = expr();
        if (name.k != VK::S) fail("OPEN's file name must be a string (?TYPE MISMATCH)");
        nm = name.op;
        n++;
    }
    store(ir::Type::PTR, nm, global("__bas_sa"));
    store(ir::Type::I8, ir::Operand::imm(n, ir::Type::I8), global("__bas_ocnt"));
    call("__bas_open");
    if (n == 4) freeVal(name);
}

void Compiler::inputStmt(bool chan) {
    if (isCh('#')) fail("INPUT # (with a space) is not supported by --compile yet");
    // INPUT# lf, ...: from the file (no prompt; bad data ?FILE DATA)
    if (chan) {
        chanArg("__bas_chkin");
        expectCh(',');
    }
    // (?REDO FROM START: the INPUT again, its prompt too)
    std::string again = newLabel("input"), end = newLabel("inend");
    br(again);
    block(again);
    bool qmark = !chan;
    auto t = peek();
    if (!chan && t && t->k == TK::Str) {
        p_++;
        Val pr = strLit(t->text);
        store(ir::Type::PTR, pr.op, global("__bas_sa"));
        call("__bas_prs");
        if (acceptCh(',')) qmark = false;   // (BASIC 65: no "? " after a prompt and a comma)
        else expectCh(';');
    }
    store(ir::Type::I16, iconst(ir::Type::I16, qmark ? 1 : 0), global("__bas_n"));
    call("__bas_iline");
    std::string items = newLabel("initem");
    brCond(bin(ir::Op::CMP_NE, ir::Type::I8, trunc(load(ir::Type::I16, global("__bas_n")), ir::Type::I8),
               ir::Operand::imm(0, ir::Type::I8)), end, items);
    block(items);
    do {
        LV lv = listVar("INPUT");
        Val v;
        if (lv.k == VK::S) {
            v.k = VK::S;
            v.slot = newSlot();
            v.op = slotAddr(v.slot);
            store(ir::Type::PTR, v.op, global("__bas_sr"));
        }
        store(ir::Type::I16, iconst(ir::Type::I16, lv.k == VK::S ? 1 : 0), global("__bas_m"));
        call("__bas_iitem");
        std::string ok = newLabel("inok");
        brCond(bin(ir::Op::CMP_NE, ir::Type::I8, trunc(load(ir::Type::I16, global("__bas_n")), ir::Type::I8),
                   ir::Operand::imm(0, ir::Type::I8)), again, ok);
        block(ok);
        if (lv.k != VK::S) { v.k = VK::F; v.op = load(ir::Type::F32, global("__float_a")); }
        assignLV(lv, v);
    } while (acceptCh(','));
    br(end);
    block(end);
    if (chan) call("__bas_clrch");
}

std::string Compiler::loopTarget(size_t li, size_t si, size_t k) {
    int depth = 0;
    size_t l = li, s = si, i = k;
    while (l < lines_.size()) {
        const auto& segs = lines_[l].segs;
        while (s < segs.size()) {
            const auto& t = segs[s].toks;
            for (; i < t.size(); i++) {
                if (t[i].k != TK::Kw) continue;
                if (t[i].kw == T_DO) depth++;
                else if (t[i].kw == T_LOOP && depth-- == 0) return nextStmtLabel(l, s);
            }
            s++;
            i = 0;
        }
        l++;
        s = 0;
    }
    return errLabel(32);     // ?LOOP NOT FOUND
}

void Compiler::doStmt() {
    // the entry: this DO's address (LOOP comes back to it), then 0
    std::string again = newLabel("do");
    size_t at = p_;          // (after the DO token)
    br(again);
    block(again);
    entry();            // (LOOP comes back here from its line)
    {
        auto r = vreg(ir::Type::I16);
        ir::Inst in; in.op = ir::Op::ADDR_LABEL; in.dest = r; in.src1 = ir::Operand::label(again); emit(in);
        store(ir::Type::I16, r, global("__bas_n"));
    }
    call("__bas_dopush");
    bool w = acceptKw(T_WHILE), u = !w && acceptKw(T_UNTIL);
    if (!w && !u) return;
    auto c = truth(expr());
    // WHILE false / UNTIL true: the entry popped, on after the loop's LOOP
    std::string out = newLabel("doout"), body = newLabel("dobody");
    brCond(c, w ? body : out, w ? out : body);
    block(out);
    call("__bas_dopop");
    br(loopTarget(li_, si_, at));
    block(body);
}

void Compiler::loopStmt() {
    // WHILE false / UNTIL true: the DO entry popped and on; else (and without a
    // condition) popped and back to its DO
    std::string back = newLabel("loopbk"), out = newLabel("loopout");
    bool w = acceptKw(T_WHILE), u = !w && acceptKw(T_UNTIL);
    if (w || u) {
        auto c = truth(expr());
        brCond(c, w ? back : out, w ? out : back);
        block(out);
        call("__bas_dopop");
        std::string on = newLabel("loopon");
        br(on);
        block(back);
        call("__bas_dopop");
        auto a = load(ir::Type::PTR, global("__bas_n"));
        ir::Inst in; in.op = ir::Op::BR_INDIRECT; in.src1 = a; emit(in);
        block(on);
        return;
    }
    call("__bas_dopop");
    auto a = load(ir::Type::PTR, global("__bas_n"));
    ir::Inst in; in.op = ir::Op::BR_INDIRECT; in.src1 = a; emit(in);
    block(newLabel("dead"));
}

void Compiler::exitStmt() {
    if (!atEnd() && !isKw(T_ELSE)) fail("EXIT takes nothing after it (?SYNTAX ERROR)");
    call("__bas_dopop");
    br(loopTarget(li_, si_, p_));
    block(newLabel("dead"));
}

void Compiler::readStmt() {
    do {
        LV lv = listVar("READ");
        Val v;
        if (lv.k == VK::S) {
            v.k = VK::S;
            v.slot = newSlot();
            v.op = slotAddr(v.slot);
            store(ir::Type::PTR, v.op, global("__bas_sr"));
        }
        store(ir::Type::I16, iconst(ir::Type::I16, lv.k == VK::S ? 1 : 0), global("__bas_m"));
        call("__bas_read");
        if (lv.k != VK::S) { v.k = VK::F; v.op = load(ir::Type::F32, global("__float_a")); }
        assignLV(lv, v);
    } while (acceptCh(','));
}

void Compiler::restoreStmt() {
    size_t off = 0;
    auto t = peek();
    if (t && t->k == TK::Num) off = dataAt_[lineTarget()];
    store(ir::Type::I16, iconst(ir::Type::I16, (int64_t)off), global("__bas_n"));
    call("__bas_restore");
}

void Compiler::getStmt() {
    // GET# lf, ...: from the file (never waits)
    bool chan = acceptCh('#');
    if (chan) {
        chanArg("__bas_chkin");
        expectCh(',');
    }
    bool wait = !chan && acceptKw(T_KEY);
    do {
        LV lv = listVar(wait ? "GETKEY" : "GET");
        Val v;
        if (lv.k == VK::S) {
            v.k = VK::S;
            v.slot = newSlot();
            v.op = slotAddr(v.slot);
            store(ir::Type::PTR, v.op, global("__bas_sr"));
        }
        store(ir::Type::I16, iconst(ir::Type::I16, (wait ? 1 : 0) | (lv.k == VK::S ? 2 : 0)), global("__bas_m"));
        call("__bas_get");
        if (lv.k != VK::S) {
            // BASIC 65: the key's code (0 for none), a signed byte (its float)
            v.k = VK::I; v.bound = 128;
            v.op = sext(trunc(load(ir::Type::I16, global("__bas_n")), ir::Type::I8), ir::Type::I8, ir::Type::I16);
        }
        assignLV(lv, v);
    } while (acceptCh(','));
    if (chan) call("__bas_clrch");
}

void Compiler::printStmt() {
    bool newline = true;
    while (!atEnd() && !isKw(T_ELSE)) {
        if (acceptCh(';')) { newline = false; continue; }
        if (acceptCh(',')) { call("__bas_prcomma"); newline = false; continue; }
        if (isKw(T_TAB) || isKw(T_SPC)) {
            bool tab = isKw(T_TAB);
            p_++;
            Val n = expr();
            expectCh(')');
            store(ir::Type::I16, zext(toByte(n), ir::Type::I8, ir::Type::I16), global("__bas_n"));
            call(tab ? "__bas_prtab" : "__bas_prspc");
            newline = false;
            continue;
        }
        Val v = expr();
        if (v.k == VK::S) {
            store(ir::Type::PTR, v.op, global("__bas_sa"));
            freeVal(v);
            call("__bas_prs");
        } else {
            store(ir::Type::F32, toF(v), global("__float_a"));
            call("__bas_prn");
        }
        newline = true;
    }
    if (newline) call("__bas_prcr");
}

std::pair<size_t, size_t> Compiler::findBend(size_t li, size_t si, size_t p) {
    // p: the BEGIN token in statement (li, si)
    int depth = 0;
    size_t l = li, s = si, k = p;
    while (l < lines_.size()) {
        const auto& segs = lines_[l].segs;
        while (s < segs.size()) {
            const auto& t = segs[s].toks;
            for (; k < t.size(); k++) {
                if (t[k].k != TK::Kw) continue;
                if (t[k].kw == T_BEGIN) depth++;
                else if (t[k].kw == T_BEND && --depth == 0) {
                    if (k + 1 != t.size()) fail("BEND is not the end of its statement");
                    return {l, s + 1};
                }
            }
            s++;
            k = 0;
        }
        l++;
        s = 0;
    }
    fail("BEGIN without BEND (?BEND NOT FOUND)");
}

std::string Compiler::falseTarget(bool begin) {
    if (begin) {
        // to the matching BEND, then to an ELSE in the rest of its line
        auto [bl, bs] = findBend(li_, si_, p_);
        return elseFrom(bl, bs);
    }
    return elseFrom(li_, si_ + 1);
}

// A condition AND / OR may jump through: its operands (at the top level,
// split by AND / OR) each a comparison (so 0 or -1: AND / OR of them is the
// logical one), and nothing in it can raise an error or call anything with
// an effect (variables, numbers, + - *, comparisons, NOT, parentheses; no
// string + ) - so leaving the rest of it unevaluated changes nothing
bool Compiler::shortCircuits(size_t a, size_t b) const {
    const auto& t = *toks_;
    int depth = 0;
    bool logic = false, rel = false, strs = false, plus = false;
    for (size_t i = a; i < b; i++) {
        const Tok& k = t[i];
        if (k.k == TK::Name) {
            if (i + 1 < b && t[i + 1].k == TK::Ch && t[i + 1].ch == '(') return false;    // (an element)
            if (k.text.back() == '$') strs = true;
        } else if (k.k == TK::Str) strs = true;
        else if (k.k == TK::Ch) {
            if (k.ch == '(') depth++;
            else if (k.ch == ')') depth--;
            else return false;
        } else if (k.k == TK::Kw) {
            switch (k.kw) {
            case T_AND: case T_OR:
                if (depth == 0) {
                    if (!rel) return false;
                    logic = true;
                    rel = false;
                }
                break;
            case T_LT: case T_EQ: case T_GT:
                if (depth == 0) rel = true;
                break;
            case T_PLUS: plus = true; break;
            case T_MINUS: case T_MUL: case T_NOT: break;
            default: return false;
            }
        } else if (k.k != TK::Num) return false;
    }
    return logic && rel && depth == 0 && !(strs && plus);
}

// The condition (shortCircuits): to t when true, to f when false
void Compiler::condJump(const std::string& t, const std::string& f) {
    for (;;) {
        // an AND term: each comparison false -> the next OR term (or f)
        std::string nextOr = newLabel("cor");
        for (;;) {
            Val v = notExpr();
            if (acceptKw(T_AND)) {
                std::string next = newLabel("cand");
                brCond(truth(v), next, nextOr);
                block(next);
                continue;
            }
            brCond(truth(v), t, nextOr);
            break;
        }
        block(nextOr);
        if (!acceptKw(T_OR)) { br(f); return; }
    }
}

void Compiler::ifStmt() {
    // the targets first: after THEN / GOTO
    size_t start = p_, q = p_;
    const auto& tk = *toks_;
    while (q < tk.size() && !(tk[q].k == TK::Kw && (tk[q].kw == T_THEN || tk[q].kw == T_GOTO))) q++;
    if (q >= tk.size()) { expr(); expectKw(T_THEN); }    // (the error)
    std::string then = newLabel("then"), tTarget, fTarget;
    bool stmts = false, begin = false;
    p_ = q + 1;
    auto t = peek();
    if (tk[q].kw == T_GOTO || (t && t->k == TK::Num)) {
        tTarget = lineLabel(lineTarget());
        fTarget = falseTarget(false);
    } else {
        begin = isKw(T_BEGIN);
        tTarget = then;
        fTarget = falseTarget(begin);
        stmts = true;
    }
    size_t after = p_;
    p_ = start;
    if (shortCircuits(start, q)) condJump(tTarget, fTarget);
    else brCond(truth(expr()), tTarget, fTarget);
    if (p_ != q) { p_ = q; fail("THEN or GOTO expected" + near()); }
    p_ = after;
    block(then);
    if (stmts) {
        if (begin) p_++;
        statement();
    }
}

void Compiler::gosubTo(size_t target) {
    // push the return block's address
    auto sp = load(ir::Type::I8, global("__bas_sp"));
    raiseIf(bin(ir::Op::CMP_GEU, ir::Type::I8, sp, ir::Operand::imm(STACK_ENTRIES, ir::Type::I8)), 16);
    std::string ret = newLabel("ret");
    auto a = vreg(ir::Type::PTR);
    { ir::Inst in; in.op = ir::Op::ADDR_ELEM; in.dest = a; in.src1 = global("__bas_stk"); in.src2 = sp;
      in.args.push_back(ir::Operand::imm(2, ir::Type::I16)); in.resultType = ir::Type::PTR; emit(in); }
    auto r = vreg(ir::Type::I16);
    { ir::Inst in; in.op = ir::Op::ADDR_LABEL; in.dest = r; in.src1 = ir::Operand::label(ret); emit(in); }
    store(ir::Type::I16, r, a);
    store(ir::Type::I8, bin(ir::Op::ADD, ir::Type::I8, sp, ir::Operand::imm(1, ir::Type::I8)), global("__bas_sp"));
    br(lineLabel(target));
    block(ret);
    entry();            // (RETURN comes back here from its line)
}

void Compiler::returnStmt() {
    // FOR and DO entries above the GOSUB's are dropped
    std::string loop = newLabel("rloop"), go = newLabel("rgo");
    br(loop);
    block(loop);
    auto sp = load(ir::Type::I8, global("__bas_sp"));
    raiseIf(bin(ir::Op::CMP_EQ, ir::Type::I8, sp, ir::Operand::imm(0, ir::Type::I8)), 12);
    auto sp1 = bin(ir::Op::SUB, ir::Type::I8, sp, ir::Operand::imm(1, ir::Type::I8));
    store(ir::Type::I8, sp1, global("__bas_sp"));
    auto a = vreg(ir::Type::PTR);
    { ir::Inst in; in.op = ir::Op::ADDR_ELEM; in.dest = a; in.src1 = global("__bas_stk"); in.src2 = sp1;
      in.args.push_back(ir::Operand::imm(2, ir::Type::I16)); in.resultType = ir::Type::PTR; emit(in); }
    auto e = load(ir::Type::I16, a);
    // a DO's entry (its address, then 0) or a FOR's (its step code, then 1..255): both words dropped
    std::string rdo = newLabel("rdo");
    brCond(bin(ir::Op::CMP_LTU, ir::Type::I8, e, ir::Operand::imm(256, ir::Type::I16)), rdo, go);
    block(rdo);
    store(ir::Type::I8, bin(ir::Op::SUB, ir::Type::I8, sp1, ir::Operand::imm(1, ir::Type::I8)), global("__bas_sp"));
    br(loop);
    block(go);
    auto pe = load(ir::Type::PTR, a);
    { ir::Inst in; in.op = ir::Op::BR_INDIRECT; in.src1 = pe; emit(in); }
    block(newLabel("dead"));
}

void Compiler::onStmt() {
    Val v = expr();
    bool gosub = false;
    if (acceptKw(T_GOSUB)) gosub = true;
    else if (!acceptKw(T_GOTO)) {
        if (acceptKw(T_GO) && acceptKw(T_TO)) {}
        else fail("GOTO or GOSUB expected after ON's expression");
    }
    std::vector<size_t> targets;
    do targets.push_back(lineTarget()); while (acceptCh(','));
    auto n = toByte(v);
    std::string after = newLabel("onend");
    for (size_t i = 0; i < targets.size(); i++) {
        std::string next = newLabel("on");
        std::string hit = gosub ? newLabel("ong") : lineLabel(targets[i]);
        brCond(bin(ir::Op::CMP_EQ, ir::Type::I8, n, ir::Operand::imm((int)i + 1, ir::Type::I8)), hit, next);
        if (gosub) {
            block(hit);
            gosubTo(targets[i]);
            br(after);
        }
        block(next);
    }
    br(after);
    block(after);
}

void Compiler::forStmt() {
    auto t = peek();
    if (!t || t->k != TK::Name) fail("a variable expected after FOR");
    std::string name = t->text;
    if (name.back() == '%') fail("FOR with an integer variable is not supported by --compile yet");
    if (name.back() == '$') fail("FOR needs a numeric variable (?TYPE MISMATCH)");
    auto it = forAt_.find({li_, si_ * 1000 + (p_ - 1)});
    if (it == forAt_.end()) fail("FOR not found by the prescan");
    ForInfo& f = fors_[it->second - 1];
    p_++;
    if (isCh('(')) fail("FOR needs a simple variable");
    expectKw(T_EQ);
    VK k;
    std::string g = varGlobal(name, k);
    std::string id = std::to_string(f.id);
    bool nar = narrow_.count(g) > 0;
    Val init = expr();
    observe(g, init, &VarObs::assign);
    if (nar) storeNarrow(g, init);
    else store(ir::Type::F32, toF(init), ir::Operand::global(g));
    expectKw(T_TO);
    Val lim = expr();
    observe(g, lim, &VarObs::assign);
    if (nar) {
        // the limit and step in the variable's type (within its bound)
        ir::Type t = narrowType(narrow_.at(g));
        if (fitsNarrow(lim, narrow_.at(g))) {
            store(t, asType(lim, t), global("__bas_fl" + id));
        } else if (final_) fail("internal error: a narrowed FOR limit out of range");
    } else {
        store(ir::Type::F32, toF(lim), global("__bas_fl" + id));
    }
    {
        // the step's size (1 without STEP)
        auto& o = obs[g];
        if (f.stepConst) {
            if (f.step != std::floor(f.step) || std::fabs(f.step) > 1e9) o.bad = true;
            else o.step = std::max(o.step, (int64_t)std::fabs(f.step));
        }
    }
    if (acceptKw(T_STEP)) {
        Val s = expr();
        if (!f.stepConst) {
            observe(g, s, &VarObs::step);
            if (nar) {
                ir::Type t = narrowType(narrow_.at(g));
                if (fitsNarrow(s, narrow_.at(g))) {
                    auto x = asType(s, t);
                    store(t, x, global("__bas_fs" + id));
                    // its sign: -1 ($FF), 0, 1 as FCMP's
                    auto gt = bin(ir::Op::CMP_GT, ir::Type::I8, x, ir::Operand::imm(0, t));
                    auto lt = bin(ir::Op::CMP_LT, ir::Type::I8, x, ir::Operand::imm(0, t));
                    store(ir::Type::I8, bin(ir::Op::SUB, ir::Type::I8, gt, lt), global("__bas_fg" + id));
                } else if (final_) fail("internal error: a narrowed FOR step out of range");
            } else {
                auto sf = toF(s);
                store(ir::Type::F32, sf, global("__bas_fs" + id));
                store(ir::Type::I8, bin(ir::Op::FCMP, ir::Type::I8, sf, fconst(0)), global("__bas_fg" + id));
            }
        }
    }
    if (f.counted) {
        // a counted loop: nothing on BASIC's stack, its NEXT steps it
        block("F" + std::to_string(f.id));
        entry();
        return;
    }
    store(ir::Type::I16, iconst(ir::Type::I16, f.id), global("__bas_n"));
    {
        // (the entry holds the loop's step code: NEXT goes there)
        auto r = vreg(ir::Type::I16);
        ir::Inst in; in.op = ir::Op::ADDR_LABEL; in.dest = r; in.src1 = ir::Operand::label("N" + std::to_string(f.id)); emit(in);
        store(ir::Type::I16, r, global("__bas_m"));
        stepBlocks_.insert(f.id);
    }
    call("__bas_forpush");
    block("F" + std::to_string(f.id));
    entry();            // (NEXT comes back here from its line)
}

void Compiler::nextStmt() {
    // __bas_next finds the loop (of the variable, or any) on BASIC's stack and
    // gives its step code's address; that goes on at its FOR's body or, done,
    // pops it and comes back here (__bas_nret)
    int place = 0;
    auto one = [&](const std::string* var) {
        // a counted loop's NEXT: its step, back to the body or on
        auto cn = countedNext_.find({li_, si_, var ? place : -1});
        if (cn != countedNext_.end()) {
            const ForInfo& f = fors_[cn->second - 1];
            std::string on = newLabel("ncon");
            brCond(stepDone(f), on, "F" + std::to_string(f.id));
            block(on);
            return;
        }
        int vi = 0;
        if (var) {
            auto it = forVarIndex_.find(*var);
            if (it == forVarIndex_.end()) { raiseNow(10); return; }    // (no FOR of it: ?NEXT WITHOUT FOR when run)
            vi = it->second;
        } else if (fors_.empty()) { raiseNow(10); return; }
        std::string done = newLabel("ndone");
        {
            auto r = vreg(ir::Type::I16);
            ir::Inst in; in.op = ir::Op::ADDR_LABEL; in.dest = r; in.src1 = ir::Operand::label(done); emit(in);
            store(ir::Type::I16, r, global("__bas_nret"));
        }
        store(ir::Type::I16, iconst(ir::Type::I16, vi), global("__bas_n"));
        call("__bas_next");
        auto a = load(ir::Type::PTR, global("__bas_n"));
        ir::Inst in; in.op = ir::Op::BR_INDIRECT; in.src1 = a; emit(in);
        block(done);
    };
    if (atEnd() || isKw(T_ELSE)) { one(nullptr); return; }
    do {
        auto t = peek();
        if (!t || t->k != TK::Name) fail("a variable expected after NEXT");
        p_++;
        VK k;
        std::string g = varGlobal(t->text, k);
        if (k != VK::F) fail("NEXT " + t->text + ": FOR needs a numeric variable");
        one(&g);
        place++;
    } while (acceptCh(','));
}

}  // namespace

BasicCompileResult compileBasicToIR(const std::vector<uint8_t>& prg, const std::string& sourceName, bool speed) {
    BasicCompileResult r;
    if (prg.size() < 4) { r.errors.push_back({0, "the program is empty"}); return r; }
    uint32_t load = prg[0] | (prg[1] << 8);
    std::vector<Line> lines;
    size_t at = 2;
    std::set<int> seen;
    while (at + 4 <= prg.size()) {
        uint32_t link = prg[at] | (prg[at + 1] << 8);
        if (link == 0) break;
        Line l;
        l.num = prg[at + 2] | (prg[at + 3] << 8);
        size_t e = at + 4;
        while (e < prg.size() && prg[e]) e++;
        std::vector<uint8_t> bytes(prg.begin() + at + 4, prg.begin() + e);
        std::string err;
        l.segs = lexLine(bytes, err);
        if (!err.empty()) r.errors.push_back({l.num, err});
        lines.push_back(l);
        // the next line: after the zero (the links are the interpreter's)
        at = e + 1;
        (void)load;
    }
    if (!r.errors.empty()) return r;
    // Integer narrowing: a float variable that only ever holds whole numbers
    // below 2^31 is an integer (exact as the interpreter's floats are there,
    // so the same results). Compiled with bounds assumed until they hold:
    // every value assigned (FOR's start and limit too) within the bound; a FOR
    // variable's NEXT adds its step, which takes it at most a step past the
    // values it had, once for each loop of it on BASIC's stack (128 entries).
    // A bound that keeps growing (N = N + 1) is not one: the variable stays a float.
    NarrowMap narrow;
    {
        // every float variable assumed a narrow 0 first
        Compiler a(lines, sourceName, narrow, false, speed);
        a.run();
        if (!a.errors.empty()) { r.errors = a.errors; return r; }
        for (const auto& [g, o] : a.obs) narrow[g] = 0;
    }
    std::map<std::string, int> grew;
    bool fixed = false;
    for (int round = 0; round < 200 && !fixed; round++) {
        Compiler a(lines, sourceName, narrow, false, speed);
        a.run();
        if (!a.errors.empty()) { r.errors = a.errors; return r; }
        fixed = true;
        for (auto it = narrow.begin(); it != narrow.end();) {
            const VarObs& o = a.obs[it->first];
            int64_t b = o.assign + 128 * o.step;
            if (o.bad || o.assign >= (1LL << 31) || b >= (1LL << 31) || (b > it->second && ++grew[it->first] > 8)) {
                it = narrow.erase(it);
                fixed = false;
                continue;
            }
            if (b > it->second) { it->second = b; fixed = false; }
            ++it;
        }
    }
    // (no fixed point: all floats)
    if (!fixed) narrow.clear();
    Compiler c(lines, sourceName, narrow, true, speed);
    r.module = c.run();
    r.errors = c.errors;
    r.ok = r.errors.empty() && r.module;
    return r;
}

std::string basicIRToAssembly(std::shared_ptr<ir::Module> module, int optLevel, const std::string& sourceName) {
    CodegenStage cg(nullptr, nullptr, optLevel, 0, false, true, true, false, false, true);
    cg.setSourceFile(sourceName);
    cg.setIRModule(module);
    auto res = cg.execute();
    if (!res.success) throw std::runtime_error(res.error);
    return cg.getIROutput();
}
