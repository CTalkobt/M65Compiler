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
// Values: floats (F32), integers known exactly (I32, a bound on their size:
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

#include <cmath>
#include <cstring>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace {

// ----------------------------------------------------------------------------
// Tokens
// ----------------------------------------------------------------------------

// Single-byte tokens the compiler names (BASIC 65's table, as BasicTokenizer)
enum : int {
    T_END = 0x80, T_FOR = 0x81, T_NEXT = 0x82, T_DATA = 0x83, T_DIM = 0x86, T_LET = 0x88, T_GOTO = 0x89,
    T_IF = 0x8B, T_GOSUB = 0x8D, T_RETURN = 0x8E, T_REM = 0x8F, T_STOP = 0x90, T_ON = 0x91,
    T_POKE = 0x97, T_PRINT = 0x99, T_TAB = 0xA3, T_TO = 0xA4, T_SPC = 0xA6, T_THEN = 0xA7,
    T_NOT = 0xA8, T_STEP = 0xA9, T_PLUS = 0xAA, T_MINUS = 0xAB, T_MUL = 0xAC, T_DIV = 0xAD,
    T_POW = 0xAE, T_AND = 0xAF, T_OR = 0xB0, T_GT = 0xB1, T_EQ = 0xB2, T_LT = 0xB3,
    T_SGN = 0xB4, T_INT = 0xB5, T_ABS = 0xB6, T_SQR = 0xBA, T_LOG = 0xBC, T_EXP = 0xBD,
    T_COS = 0xBE, T_SIN = 0xBF, T_TAN = 0xC0, T_ATN = 0xC1, T_PEEK = 0xC2, T_LEN = 0xC3,
    T_STR = 0xC4, T_ASC = 0xC6, T_CHR = 0xC7, T_LEFT = 0xC8, T_RIGHT = 0xC9,
    T_MID = 0xCA, T_GO = 0xCB, T_ELSE = 0xD5, T_PI = 0xFF,
    T_BEGIN = 0xFE18, T_BEND = 0xFE19,
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
                // its items are not statements: up to the colon
                bool q = false;
                while (i < b.size() && (q || b[i] != ':')) { if (b[i] == '"') q = !q; i++; }
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
    ir::Operand op;          // F: F32, I: I32, B: I8 (0/1), S: PTR
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
    std::map<std::string, int> forVarIndex_;
    std::set<std::string> externs_;
    std::set<int> errBlocks_;
    std::set<int> stepBlocks_;                   // the FOR loops whose step block a NEXT uses
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
        externs_.insert(f);
        fn_->originalCallees.insert(f);
        ir::Inst in; in.op = ir::Op::CALL_VOID; in.resultType = ir::Type::VOID;
        in.src1 = ir::Operand::global(f); in.callConv = ir::CallConv::ZP; emit(in);
    }
    // the error n (its block, shared: the line is in __bas_line)
    std::string errLabel(int n) { errBlocks_.insert(n); return "Err" + std::to_string(n); }
    // the error n where the statement runs (a constant out of range: the
    // interpreter's error only when the line is run)
    void raiseNow(int n) { br(errLabel(n)); block(newLabel("dead")); }
    void raiseIf(ir::Operand cond, int n) {
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
        VK k;
        std::string g = varGlobal(name, k);
        Val v;
        v.k = k;
        if (k == VK::F && narrow_.count(g)) {
            // a narrowed variable: a whole number within its bound
            int64_t b = narrow_.at(g);
            ir::Type t = narrowType(b);
            auto x = load(t, ir::Operand::global(g));
            v.k = VK::I;
            v.op = t == ir::Type::I32 ? x : sext(x, t);
            v.bound = b;
        } else if (k == VK::F) v.op = load(ir::Type::F32, ir::Operand::global(g));
        else if (k == VK::S) v.op = load(ir::Type::PTR, ir::Operand::global(g));
        else {
            auto x = load(ir::Type::I16, ir::Operand::global(g));
            v.op = sext(x, ir::Type::I16);
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
            checkRange(v.op, 0, 65535);
            return trunc(v.op, ir::Type::I16);
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
    // subscript: the element reached here when the array exists with one
    // dimension and the index is in it; else (and for more) __bas_aref, which
    // makes the array (11 elements a dimension) or raises the error
    ir::Operand elemAddr(const std::string& name, VK& kind) {
        std::string g = arrayGlobal(name, kind);
        auto subs = subscripts();
        int es = elemSize(kind);
        int m = es | (kind == VK::S ? 0x40 : 0);
        auto r = vreg(ir::Type::PTR);
        if (subs.size() == 1) {
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
        else { v.op = sext(load(ir::Type::I16, a), ir::Type::I16); v.bound = 32768; }
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

    ir::Operand sext(ir::Operand x, ir::Type from) {
        auto d = vreg(ir::Type::I32);
        ir::Inst in; in.op = ir::Op::SEXT; in.dest = d; in.src1 = x; in.resultType = ir::Type::I32;
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
    // v (I / B) as an I32
    ir::Operand asI32(const Val& v) { return v.k == VK::B ? toI16(v).op : v.op; }
    void storeNarrow(const std::string& g, const Val& v) {
        int64_t b = narrow_.at(g);
        if (!fitsNarrow(v, b)) {
            if (final_) fail("internal error: the narrowed variable " + g + " gets a value out of its range");
            return;
        }
        ir::Type t = narrowType(b);
        auto x = asI32(v);
        store(t, t == ir::Type::I32 ? x : trunc(x, t), ir::Operand::global(g));
    }

    // --- conversions ---
    Val constI(int64_t v) {
        Val r; r.k = VK::I; r.isConst = true; r.cval = (double)v; r.bound = v < 0 ? -v : v;
        r.op = iconst(ir::Type::I32, v);
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
    // (rounded down, ?ILLEGAL QUANTITY outside it). I32
    Val toI16(const Val& v) {
        if (v.k == VK::S) fail("a number expected, not a string (?TYPE MISMATCH)");
        if (v.k == VK::B) {
            Val r; r.k = VK::I; r.bound = 1;
            r.op = sext(un(ir::Op::NEG, ir::Type::I8, v.op), ir::Type::I8);
            return r;
        }
        if (v.k == VK::I) {
            if (v.isConst) {
                if (v.cval < -32768 || v.cval > 32767) { raiseNow(14); return constI(0); }
                return v;
            }
            if (v.bound > 32767) checkRange(v.op, -32768, 32767);
            Val r = v; r.bound = std::min<int64_t>(v.bound, 32768);
            return r;
        }
        if (v.isConst) {
            double f = std::floor(v.cval);
            if (f < -32768 || f > 32767) { raiseNow(14); return constI(0); }
            return constI((int64_t)f);
        }
        Val r; r.k = VK::I; r.bound = 32768;
        r.op = sext(viaN(v, "__bas_ayint"), ir::Type::I16);
        return r;
    }
    void checkRange(ir::Operand x, int64_t lo, int64_t hi) {
        raiseIf(bin(ir::Op::CMP_LT, ir::Type::I8, x, ir::Operand::imm(lo, ir::Type::I32)), 14);
        raiseIf(bin(ir::Op::CMP_GT, ir::Type::I8, x, ir::Operand::imm(hi, ir::Type::I32)), 14);
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
            checkRange(v.op, 0, 255);
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
            checkRange(v.op, 0, 65535);
            return trunc(v.op, ir::Type::PTR);
        }
        store(ir::Type::F32, toF(v), global("__float_a"));
        call("__bas_getadr");
        return load(ir::Type::PTR, global("__bas_n"));
    }
    // A condition's truth: I8, nonzero when true
    ir::Operand truth(const Val& v) {
        if (v.k == VK::S) fail("a number expected, not a string (?TYPE MISMATCH)");
        if (v.k == VK::B) return v.op;
        if (v.k == VK::I) return bin(ir::Op::CMP_NE, ir::Type::I8, v.op, ir::Operand::imm(0, ir::Type::I32));
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
        while (acceptKw(T_OR)) {
            Val b = andExpr();
            a = logic(ir::Op::OR, a, b);
        }
        return a;
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
        Val x = toI16(a), y = toI16(b);
        Val r; r.k = VK::I; r.bound = 32768;
        r.op = bin(op, ir::Type::I32, x.op, y.op);
        return r;
    }
    Val notExpr() {
        if (acceptKw(T_NOT)) {
            Val a = notExpr();
            if (a.k == VK::B) {
                Val r; r.k = VK::B; r.op = bin(ir::Op::XOR, ir::Type::I8, a.op, ir::Operand::imm(1, ir::Type::I8));
                return r;
            }
            Val x = toI16(a);
            Val r; r.k = VK::I; r.bound = 32768;
            r.op = bin(ir::Op::XOR, ir::Type::I32, x.op, ir::Operand::imm(-1, ir::Type::I32));
            return r;
        }
        return relExpr();
    }
    Val relExpr() {
        Val a = addExpr();
        for (;;) {
            int rel = 0;    // bits: 1 <, 2 =, 4 >
            while (true) {
                if (acceptKw(T_LT)) rel |= 1;
                else if (acceptKw(T_EQ)) rel |= 2;
                else if (acceptKw(T_GT)) rel |= 4;
                else break;
            }
            if (!rel) return a;
            Val b = addExpr();
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
            ir::Operand x = a.k == VK::B ? toI16(a).op : a.op;
            ir::Operand y = b.k == VK::B ? toI16(b).op : b.op;
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
                Val r = a; r.op = un(ir::Op::NEG, ir::Type::I32, a.op);
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
                r.op = bin(op, ir::Type::I32, x.op, y.op);
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
        case T_ABS: {
            Val a = parenExpr();
            if (a.k == VK::I && !a.isConst) {
                // |x| without the float runtime
                auto neg = bin(ir::Op::CMP_LT, ir::Type::I8, a.op, ir::Operand::imm(0, ir::Type::I32));
                auto r = vreg(ir::Type::I32);
                std::string n = newLabel("absn"), d = newLabel("absd"), p = newLabel("absp");
                brCond(neg, n, p);
                block(n);
                { ir::Inst in; in.op = ir::Op::NEG; in.dest = r; in.src1 = a.op; in.resultType = ir::Type::I32; emit(in); }
                br(d);
                block(p);
                { ir::Inst in; in.op = ir::Op::COPY; in.dest = r; in.src1 = a.op; in.resultType = ir::Type::I32; emit(in); }
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
                // -1, 0, 1: (x > 0) - (x < 0)
                Val x = toI16(a);
                auto gt = bin(ir::Op::CMP_GT, ir::Type::I8, x.op, ir::Operand::imm(0, ir::Type::I32));
                auto lt = bin(ir::Op::CMP_LT, ir::Type::I8, x.op, ir::Operand::imm(0, ir::Type::I32));
                c = bin(ir::Op::SUB, ir::Type::I8, gt, lt);
            }
            Val r; r.k = VK::I; r.bound = 1; r.op = sext(c, ir::Type::I8);
            return r;
        }
        case T_PEEK: {
            Val a = parenExpr();
            auto addr = toAddr(a);
            auto b = load(ir::Type::I8, addr);
            Val r; r.k = VK::I; r.bound = 255; r.op = zext(b, ir::Type::I8, ir::Type::I32);
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
            Val r; r.k = VK::I; r.bound = 255; r.op = zext(b, ir::Type::I8, ir::Type::I32);
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
    void printStmt();
    void ifStmt();
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
                forAt_[{li, si * 1000 + k}] = f.id;
                fors_.push_back(f);
            }
        }
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

    block("entry");
    li_ = 0;
    // the string variables start empty (__bas_empty): set when the program
    // has been compiled (finish), here
    std::string init = "init";
    br(init);
    block("start");
    for (li_ = 0; li_ < lines_.size(); li_++) {
        block(lineLabel(li_));
        store(ir::Type::I16, iconst(ir::Type::I16, lines_[li_].num), global("__bas_line"));
        for (si_ = 0; si_ < lines_[li_].segs.size(); si_++) {
            block(stmtLabel(li_, si_));
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

void Compiler::stepBlock(const ForInfo& f) {
    // N<id>: the loop's variable + step; not past the limit: the body again
    // (F<id>), else the loop's entry popped and on after the NEXT (__bas_nret)
    block("N" + std::to_string(f.id));
    std::string id = std::to_string(f.id);
    ir::Operand c;
    if (narrow_.count(f.var)) {
        // a narrowed variable: integer addition and comparison (the
        // bound allows for the step: no overflow)
        ir::Type t = narrowType(narrow_.at(f.var));
        auto ld = [&](ir::Operand a) { auto x = load(t, a); return t == ir::Type::I32 ? x : sext(x, t); };
        auto v = ld(ir::Operand::global(f.var));
        ir::Operand step = f.stepConst ? iconst(ir::Type::I32, (int64_t)f.step) : ld(global("__bas_fs" + id));
        auto v2 = bin(ir::Op::ADD, ir::Type::I32, v, step);
        store(t, t == ir::Type::I32 ? v2 : trunc(v2, t), ir::Operand::global(f.var));
        auto lim = ld(global("__bas_fl" + id));
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
    ir::Operand fin;
    if (f.stepConst) {
        int sg = f.step > 0 ? 1 : f.step < 0 ? 0xFF : 0;
        fin = bin(ir::Op::CMP_EQ, ir::Type::I8, c, ir::Operand::imm(sg, ir::Type::I8));
    } else {
        fin = bin(ir::Op::CMP_EQ, ir::Type::I8, c, load(ir::Type::I8, global("__bas_fg" + id)));
    }
    std::string pop = "P" + id;
    brCond(fin, pop, "F" + id);
    block(pop);
    auto sp = load(ir::Type::I8, global("__bas_sp"));
    store(ir::Type::I8, bin(ir::Op::SUB, ir::Type::I8, sp, ir::Operand::imm(1, ir::Type::I8)), global("__bas_sp"));
    auto r = load(ir::Type::PTR, global("__bas_nret"));
    ir::Inst in; in.op = ir::Op::BR_INDIRECT; in.src1 = r; emit(in);
}

void Compiler::finish() {
    for (int id : stepBlocks_) stepBlock(fors_[id - 1]);
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
    case T_DATA: return;
    case T_DIM: dimStmt(); return;
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
        Val a = expr();
        expectCh(',');
        auto addr = toAddr(a);
        Val v = expr();
        auto b = toByte(v);
        store(ir::Type::I8, b, addr);
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

void Compiler::assignment(const std::string& name) {
    if (isCh('(')) {
        // an element: its address first (the subscripts, the array made), as BASIC's LET
        VK k;
        auto a = elemAddr(name, k);
        expectKw(T_EQ);
        Val v = expr();
        if (k == VK::S) {
            needS(v);
            store(ir::Type::PTR, v.op, global("__bas_sa"));
            store(ir::Type::PTR, a, global("__bas_sr"));
            call("__bas_sset");
            freeVal(v);
        } else if (k == VK::F) {
            store(ir::Type::F32, toF(v), a);
        } else {
            Val x = toI16(v);
            store(ir::Type::I16, trunc(x.op, ir::Type::I16), a);
        }
        return;
    }
    expectKw(T_EQ);
    VK k;
    std::string g = varGlobal(name, k);
    Val v = expr();
    if (k == VK::S) {
        needS(v);
        store(ir::Type::PTR, v.op, global("__bas_sa"));
        auto a = vreg(ir::Type::PTR);
        ir::Inst in; in.op = ir::Op::ADDR_GLOBAL; in.dest = a; in.src1 = ir::Operand::global(g);
        in.resultType = ir::Type::PTR; emit(in);
        store(ir::Type::PTR, a, global("__bas_sr"));
        call("__bas_sset");
        freeVal(v);
    } else if (k == VK::F) {
        observe(g, v, &VarObs::assign);
        if (narrow_.count(g)) storeNarrow(g, v);
        else store(ir::Type::F32, toF(v), ir::Operand::global(g));
    } else {
        Val x = toI16(v);
        store(ir::Type::I16, trunc(x.op, ir::Type::I16), ir::Operand::global(g));
    }
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

void Compiler::ifStmt() {
    Val c = expr();
    auto cond = truth(c);
    std::string then = newLabel("then");
    if (acceptKw(T_GOTO)) {
        size_t target = lineTarget();
        brCond(cond, lineLabel(target), falseTarget(false));
        block(then);
        return;
    }
    expectKw(T_THEN);
    auto t = peek();
    if (t && t->k == TK::Num) {
        size_t target = lineTarget();
        brCond(cond, lineLabel(target), falseTarget(false));
        block(then);
        return;
    }
    bool begin = isKw(T_BEGIN);
    brCond(cond, then, falseTarget(begin));
    block(then);
    if (begin) p_++;
    statement();
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
}

void Compiler::returnStmt() {
    // FOR entries above the GOSUB's are dropped
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
    brCond(bin(ir::Op::CMP_LTU, ir::Type::I8, e, ir::Operand::imm(256, ir::Type::I16)), loop, go);
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
            auto x = asI32(lim);
            store(t, t == ir::Type::I32 ? x : trunc(x, t), global("__bas_fl" + id));
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
                    auto x = asI32(s);
                    store(t, t == ir::Type::I32 ? x : trunc(x, t), global("__bas_fs" + id));
                    // its sign: -1 ($FF), 0, 1 as FCMP's
                    auto gt = bin(ir::Op::CMP_GT, ir::Type::I8, x, ir::Operand::imm(0, ir::Type::I32));
                    auto lt = bin(ir::Op::CMP_LT, ir::Type::I8, x, ir::Operand::imm(0, ir::Type::I32));
                    store(ir::Type::I8, bin(ir::Op::SUB, ir::Type::I8, gt, lt), global("__bas_fg" + id));
                } else if (final_) fail("internal error: a narrowed FOR step out of range");
            } else {
                auto sf = toF(s);
                store(ir::Type::F32, sf, global("__bas_fs" + id));
                store(ir::Type::I8, bin(ir::Op::FCMP, ir::Type::I8, sf, fconst(0)), global("__bas_fg" + id));
            }
        }
    }
    store(ir::Type::I16, iconst(ir::Type::I16, f.id), global("__bas_n"));
    call("__bas_forpush");
    block("F" + std::to_string(f.id));
}

void Compiler::nextStmt() {
    // the FOR loops the NEXT can be of (its variable's, or any)
    auto one = [&](const std::string* var) {
        std::vector<const ForInfo*> cands;
        for (const auto& f : fors_)
            if (!var || f.var == *var) cands.push_back(&f);
        if (cands.empty()) fail("NEXT " + (var ? *var : std::string()) + " has no FOR (?NEXT WITHOUT FOR)");
        std::string loop = newLabel("nloop"), done = newLabel("ndone");
        {
            auto r = vreg(ir::Type::I16);
            ir::Inst in; in.op = ir::Op::ADDR_LABEL; in.dest = r; in.src1 = ir::Operand::label(done); emit(in);
            store(ir::Type::I16, r, global("__bas_nret"));
        }
        br(loop);
        block(loop);
        auto sp = load(ir::Type::I8, global("__bas_sp"));
        raiseIf(bin(ir::Op::CMP_EQ, ir::Type::I8, sp, ir::Operand::imm(0, ir::Type::I8)), 10);
        auto sp1 = bin(ir::Op::SUB, ir::Type::I8, sp, ir::Operand::imm(1, ir::Type::I8));
        auto a = vreg(ir::Type::PTR);
        { ir::Inst in; in.op = ir::Op::ADDR_ELEM; in.dest = a; in.src1 = global("__bas_stk"); in.src2 = sp1;
          in.args.push_back(ir::Operand::imm(2, ir::Type::I16)); in.resultType = ir::Type::PTR; emit(in); }
        auto e = load(ir::Type::I16, a);
        std::vector<std::string> cases;
        for (const auto* f : cands) {
            std::string c = newLabel("ncase"), nx = newLabel("nnext");
            cases.push_back(c);
            brCond(bin(ir::Op::CMP_EQ, ir::Type::I8, e, ir::Operand::imm(f->id, ir::Type::I16)), c, nx);
            block(nx);
        }
        // another entry: a GOSUB's (?NEXT WITHOUT FOR), else an inner loop: dropped
        raiseIf(bin(ir::Op::CMP_GEU, ir::Type::I8, e, ir::Operand::imm(256, ir::Type::I16)), 10);
        store(ir::Type::I8, sp1, global("__bas_sp"));
        br(loop);
        // a match: the loop's step block (shared by its NEXTs), which goes
        // on at F<id> or, done, pops it and goes on here (__bas_nret)
        for (size_t i = 0; i < cands.size(); i++) {
            block(cases[i]);
            br("N" + std::to_string(cands[i]->id));
            stepBlocks_.insert(cands[i]->id);
        }
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
