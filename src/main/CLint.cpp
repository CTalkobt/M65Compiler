// cl45: the C linter's rules (#199; doc/architecture/lint.md). See CLint.hpp.
#include "CLint.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <functional>
#include <sstream>

#include "AST.hpp"
#include "IR.hpp"
#include "UnpackFormats.hpp"

namespace clint {

namespace {

std::string hex4(int64_t v) {
    char b[16];
    std::snprintf(b, sizeof b, "$%04X", (unsigned)(v & 0xFFFF));
    return b;
}

bool isIo(int64_t a) { return a >= 0xD000 && a <= 0xDFFF; }

}  // namespace

void Context::addPragmas(const std::vector<Pragma>& ps) {
    for (const auto& p : ps) engine->suppressFrom(p.file, p.line, p.id, p.restore);
}

void Context::report(const std::string& id, const std::string& file, int line, int col, const std::string& message) {
    if (file.empty() || file[0] == '<') return;     // (<complex>: cc45's own source)
    for (const auto& d : systemDirs)
        if (!d.empty() && file.compare(0, d.size(), d) == 0) return;
    if (scanned_.insert(file).second) {
        std::ifstream in(file);
        if (in) {
            std::stringstream ss;
            ss << in.rdbuf();
            engine->scanSuppressions(file, ss.str());
        }
    }
    std::string key = id + "\x01" + file + "\x01" + std::to_string(line) + "\x01" + std::to_string(col) + "\x01" + message;
    if (!seen_.insert(key).second) return;
    engine->report(id, file, line, col, message);
}

void registerRules(lint::Engine& e) {
    using L = lint::Level;
    e.add({"mega65-dma-trigger-order", L::Warning, "all",
           "A DMA list address byte written after the trigger register",
           "Writing $D700 (or $D705, the enhanced list's low byte) starts the DMA job with the list address\n"
           "in $D701/$D702 (and $D704) as they are then; one of those written after the trigger, with no\n"
           "other trigger after it in the same block, belongs to no job. Write the address bytes first."});
    e.add({"mega65-io-not-volatile", L::Warning, "all",
           "An I/O register ($D000-$DFFF) reached through a pointer or variable that is not volatile",
           "A read of an I/O register has an effect or a value that changes by itself, and a write one\n"
           "that the next write does not replace. Only 'volatile' tells the compiler so: cast the address\n"
           "to (volatile T *), declare the pointer 'volatile T *p', the placed variable 'volatile'.\n"
           "cc45 today leaves accesses through pointers alone; it does reuse the value read from a\n"
           "variable placed in I/O with __section, and other compilers merge both."});
    e.add({"mega65-io-personality", L::Info, "standalone",
           "VIC-III/IV registers written with no $47 $53 knock to $D02F",
           "The MEGA65's own I/O ($D030-$D07F and above) is visible only after the knock: $47 then $53\n"
           "written to $D02F. Programs linked with cc45's crt0 have it done at startup; code built to run\n"
           "without it should do it itself, with interrupts masked (php; sei; ...; plp): between the two\n"
           "writes the KERNAL's keyboard scan reads $D611 as $FF and turns its Ctrl-S pause on."});
    e.add({"mega65-isr-rom-call", L::Warning, "cc45",
           "An __interrupt handler, or a function it calls, calls the ROM",
           "The KERNAL and the BASIC ROM are not reentrant: a handler that calls them while the program\n"
           "is inside one of their routines overwrites that routine's work variables (the cursor, the\n"
           "current device, the float accumulators). The library keeps its own state across a handler\n"
           "(__jsr's register bytes, __rom_call's saved stack pointer); it cannot keep the ROM's. Set a\n"
           "flag in the handler and make the call from the main program, or make sure the program is\n"
           "never inside the ROM when the interrupt can arrive."});
    e.add({"mega65-stack-frame-limit", L::Warning, "all",
           "A stack frame larger than the limit (option mega65-stack-frame-limit, default 256 bytes)",
           "The CPU stack is 4 KB by default (-fstack-size) and a function with a stack frame (recursive,\n"
           "__interrupt, #pragma cc45 recurse, -fno-staticalloc) takes its whole frame on every call.\n"
           "Make large locals static, or raise the limit:\n"
           "  c_checks:\n    options:\n      mega65-stack-frame-limit: 512"});
    e.add({"mega65-recursion-frame", L::Info, "all",
           "A function on a call cycle: it gets a stack frame instead of static allocation",
           "cc45 keeps the parameters and locals of a function that cannot be active twice in static\n"
           "storage, which is smaller and faster than a stack frame. A function that calls itself, directly\n"
           "or through others of the file, cannot. #pragma cc45 recurse before it says the recursion is\n"
           "meant (and silences this)."});
    e.add({"mega65-sac-interrupt", L::Warning, "cc45",
           "A static-allocation function reachable from an __interrupt handler of the same file",
           "A function with static allocation has one copy of its parameters and locals. Called from an\n"
           "interrupt handler while the main program is inside it, the handler's call overwrites them;\n"
           "ln45 refuses to link such a program. Put #pragma cc45 recurse before the function (it then\n"
           "uses a stack frame), or keep what handlers call to themselves."});
    e.add({"mega65-narrow-param", L::Info, "all",
           "An int parameter that every call in the file passes a small constant",
           "A static function's int parameter that all its calls give a constant of 0-255 could be an\n"
           "unsigned char: a byte passed and compared instead of two."});
    e.add({"mega65-wide-arith", L::Info, "all",
           "long arithmetic whose operands and result fit 16 bits",
           "Both operands were widened from int or char (or are small constants) and the result is used\n"
           "as an int or char: the 32-bit operation gives the same bits as a 16-bit one, at twice the\n"
           "code. Drop the long cast or the L suffix. (cc45's type narrowing does this itself from -O1.)"});
    e.add({"mega65-signedness", L::Warning, "all",
           "A plain (unsigned in cc45) integer used where standard C would have a signed one",
           "cc45's plain char, short, int and long are unsigned unless declared signed. A negative constant\n"
           "stored in one or compared with one becomes a large positive value; x < 0 is always false; a\n"
           "difference is never negative, so (a - b) / 2 and (a - b) >> 1 differ from standard C when\n"
           "a < b. Declare the variable signed, or build with -fsigned-default."});
    e.add({"mega65-implicit-narrowing", L::Info, "all",
           "A wider integer assigned to a narrower one without a cast",
           "The upper bytes are dropped silently. When that is meant, a cast says so: c = (char)n."});
    e.add({"mega65-unpack-format", L::Warning, "cc45",
           "A pack45 stream unpacked whose format's decoder the program does not link",
           "Each pack45 format's decoder is linked only when the program names it at file scope:\n"
           "UNPACK_FORMAT1; or UNPACK_FORMAT2; (<unpack.h>; UNPACK_IMAGES; is UNPACK_FORMAT2). <unpack.h>\n"
           "names format 1 itself unless UNPACK_EXPLICIT_FORMATS is defined. Without the decoder, unpack(),\n"
           "unpack_to() and load_unpack() return UNPACK_ERROR for such a stream and write nothing. The rule\n"
           "reads the format of streams it can see (an array initialized in the file, #embed too; load_unpack of a\n"
           "name whose file is beside the source or in an include directory) and checks the file with\n"
           "main(): another file may name the decoder, so ln45 checks the whole program as well."});
}

// ---------------------------------------------------------------------------
// The AST as parsed: I/O reached through a constant address
// ---------------------------------------------------------------------------
namespace {

// an integer constant expression of literals (what register addresses are written as)
bool constInt(Expression* e, int64_t& v) {
    if (auto* l = dynamic_cast<IntegerLiteral*>(e)) { v = l->value; return true; }
    if (auto* c = dynamic_cast<CastExpression*>(e)) return c->pointerLevel == 0 && constInt(c->expression.get(), v);
    if (auto* b = dynamic_cast<BinaryOperation*>(e)) {
        int64_t l, r;
        if (!constInt(b->left.get(), l) || !constInt(b->right.get(), r)) return false;
        if (b->op == "+") v = l + r;
        else if (b->op == "-") v = l - r;
        else if (b->op == "*") v = l * r;
        else if (b->op == "|") v = l | r;
        else if (b->op == "<<" && r >= 0 && r < 32) v = l << r;
        else return false;
        return true;
    }
    return false;
}

// (T *)constant in $D000-$DFFF, or that plus an offset: the address and
// whether the pointer is to volatile
bool ioPointer(Expression* e, int64_t& addr, bool& vol) {
    if (auto* c = dynamic_cast<CastExpression*>(e)) {
        if (c->pointerLevel < 1 || c->targetType.rfind("__farp", 0) == 0 || c->targetType.rfind("__nearp", 0) == 0)
            return false;
        int64_t v;
        bool innerVol;
        if (constInt(c->expression.get(), v)) {
            if (!isIo(v)) return false;
        } else if (!ioPointer(c->expression.get(), v, innerVol)) {
            return false;
        }
        addr = v;
        vol = c->isVolatile;
        return true;
    }
    if (auto* b = dynamic_cast<BinaryOperation*>(e)) {
        if (b->op != "+" && b->op != "-") return false;
        if (!ioPointer(b->left.get(), addr, vol)) return false;
        int64_t off;
        if (constInt(b->right.get(), off)) addr += b->op == "+" ? off : -off;
        return true;
    }
    return false;
}

class IoWalker : public ASTWalker {
public:
    explicit IoWalker(Context& c) : c_(c) {}
    using ASTWalker::visit;

    void visit(FunctionDeclaration& node) override {
        locals_.clear();
        // (a parameter's qualifiers are the pointer's own: what it points at is not known here)
        shadow_.clear();
        for (const auto& p : node.parameters) shadow_.insert(p.name);
        ASTWalker::visit(node);
        locals_.clear();
        shadow_.clear();
    }
    void visit(VariableDeclaration& node) override {
        auto& scope = node.isGlobal ? globals_ : locals_;
        if (node.pointerLevel > 0 && node.arrayDims.empty()) scope[node.name] = node.pointeeVolatile;
        else scope.erase(node.name), (node.isGlobal ? (void)0 : (void)(shadow_.insert(node.name)));
        int64_t a;
        bool vol;
        if (node.pointerLevel > 0 && !node.pointeeVolatile && node.initializer &&
            ioPointer(node.initializer.get(), a, vol)) {
            c_.report("mega65-io-not-volatile", node.sourceFile, node.line, node.column,
                      "'" + node.name + "' points at I/O (" + hex4(a) + ") but not at a volatile type: declare it 'volatile " +
                      node.type + " *" + node.name + "'");
        }
        ASTWalker::visit(node);
    }
    void visit(Assignment& node) override {
        int64_t a;
        bool vol;
        auto* vr = dynamic_cast<VariableReference*>(node.target.get());
        if (vr && node.op == "=" && ioPointer(node.expression.get(), a, vol)) {
            auto l = locals_.find(vr->name);
            auto g = globals_.find(vr->name);
            bool known = l != locals_.end() || (!shadow_.count(vr->name) && g != globals_.end());
            bool pv = l != locals_.end() ? l->second : (g != globals_.end() && g->second);
            if (known && !pv)
                c_.report("mega65-io-not-volatile", node.sourceFile, node.line, node.column,
                          "'" + vr->name + "' is given an I/O address (" + hex4(a) +
                          ") but does not point at a volatile type");
        }
        ASTWalker::visit(node);
    }
    void visit(UnaryOperation& node) override {
        if (node.op == "*") access(node.operand.get(), node);
        ASTWalker::visit(node);
    }
    void visit(ArrayAccess& node) override {
        access(node.arrayExpr.get(), node);
        ASTWalker::visit(node);
    }
    void visit(MemberAccess& node) override {
        if (node.isArrow) access(node.structExpr.get(), node);
        ASTWalker::visit(node);
    }

private:
    void access(Expression* ptr, ASTNode& at) {
        int64_t a;
        bool vol;
        if (!ioPointer(ptr, a, vol) || vol) return;
        c_.report("mega65-io-not-volatile", at.sourceFile, at.line, at.column,
                  "I/O (" + hex4(a) + ") accessed through a pointer that is not to a volatile type: cast the address to (volatile T *)");
    }
    Context& c_;
    std::map<std::string, bool> globals_, locals_;   // pointer variables: point at volatile
    std::set<std::string> shadow_;
};

}  // namespace

void runAST(Context& c, TranslationUnit& unit) {
    if (c.engine->levelOf("mega65-io-not-volatile") == lint::Level::Off) return;
    IoWalker w(c);
    unit.accept(w);
}

// ---------------------------------------------------------------------------
// The IR as written
// ---------------------------------------------------------------------------
namespace {

using ir::Op;
using ir::OperandKind;

bool isCall(const ir::Inst& i) { return i.op == Op::CALL || i.op == Op::CALL_VOID; }

std::string cName(const std::string& sym) { return !sym.empty() && sym[0] == '_' ? sym.substr(1) : sym; }

// where a function is declared
struct Where { std::string file; int line, col; };
Where declOf(const ir::Module& m, const ir::Function& f) {
    return {f.declFile.empty() ? m.sourceFile : f.declFile, f.declLine, f.declColumn};
}

// The constant addresses stores of a block go to: constants followed through
// temporaries and local slots, a block at a time
struct BlockConsts {
    const ir::Function& fn;
    std::map<uint32_t, int64_t> val;
    explicit BlockConsts(const ir::Function& f) : fn(f) {}
    bool slot(const ir::Operand& o) const { return o.isVreg() && fn.localSlotVregs.count(o.vregId); }
    bool known(const ir::Operand& o, int64_t& v) const {
        if (o.isImm()) { v = o.immVal; return true; }
        if (o.kind == OperandKind::GLOBAL && o.name.size() > 1 && o.name[0] == '$') {
            v = std::strtol(o.name.c_str() + 1, nullptr, 16);
            return true;
        }
        if (o.isVreg()) {
            auto it = val.find(o.vregId);
            if (it != val.end()) { v = it->second; return true; }
        }
        return false;
    }
    // the address a STORE writes (false: a local slot's assignment, or not known)
    bool storeAddr(const ir::Inst& i, int64_t& a) const {
        if (i.op != Op::STORE || slot(i.src2)) return false;
        return known(i.src2, a);
    }
    void step(const ir::Inst& i) {
        int64_t a, b;
        if (i.op == Op::STORE) {
            if (slot(i.src2)) {
                if (known(i.src1, a)) val[i.src2.vregId] = a;
                else val.erase(i.src2.vregId);
            }
            return;
        }
        if (isCall(i) || i.op == Op::CALL_INDIRECT || i.op == Op::ASM_INLINE)
            for (uint32_t v : fn.memoryVregs) val.erase(v);     // (their address may be known elsewhere)
        if (!i.dest.isVreg()) return;
        bool ok = false;
        int64_t r = 0;
        switch (i.op) {
            case Op::CONST: ok = i.src1.isImm(); r = i.src1.immVal; break;
            case Op::COPY: ok = known(i.src1, r); break;
            case Op::ADD: ok = known(i.src1, a) && known(i.src2, b); r = a + b; break;
            case Op::SUB: ok = known(i.src1, a) && known(i.src2, b); r = a - b; break;
            default: break;
        }
        if (ok) val[i.dest.vregId] = r & 0xFFFF;
        else val.erase(i.dest.vregId);
    }
};

void dmaAndKnock(Context& c, const ir::Module& m) {
    bool knock = false;
    ir::SourceLoc firstVic4;
    for (const auto& fn : m.functions) {
        ir::SourceLoc last;
        for (const auto& blk : fn.blocks) {
            BlockConsts k(fn);
            int trigger = 0;                       // the line of a trigger write in this block
            struct Late { ir::SourceLoc loc; int64_t reg; int trigger; };
            std::vector<Late> late;
            auto flush = [&](bool report) {
                if (report)
                    for (const auto& d : late)
                        c.report("mega65-dma-trigger-order", d.loc.file, d.loc.line, d.loc.column,
                                 "DMA list address byte " + hex4(d.reg) + " written after the trigger (line " +
                                 std::to_string(d.trigger) + ") and no trigger follows: the job started with the old value");
                late.clear();
                trigger = 0;
            };
            for (const auto& i : blk.insts) {
                if (i.loc.valid()) last = i.loc;
                int64_t a;
                if (k.storeAddr(i, a)) {
                    a &= 0xFFFF;
                    const ir::SourceLoc& at = i.loc.valid() ? i.loc : last;
                    if (a == 0xD02F) knock = true;
                    if (a >= 0xD030 && a <= 0xD07F && !firstVic4.valid()) firstVic4 = at;
                    if (a == 0xD700 || a == 0xD705) {
                        late.clear();              // (the bytes before it were this job's)
                        trigger = at.line;
                    } else if ((a == 0xD701 || a == 0xD702 || a == 0xD704) && trigger) {
                        late.push_back({at, a, trigger});
                    }
                }
                if (isCall(i) || i.op == Op::CALL_INDIRECT) flush(true);
                else if (i.op == Op::ASM_INLINE) flush(false);
                k.step(i);
            }
            flush(true);
        }
    }
    if (firstVic4.valid() && !knock)
        c.report("mega65-io-personality", firstVic4.file, firstVic4.line, firstVic4.column,
                 "a MEGA65 I/O register is written, but nothing writes $47, $53 to $D02F (the knock that makes it visible)");
}

// the file's functions and their direct calls
struct Graph {
    std::map<std::string, const ir::Function*> fns;
    std::map<std::string, std::set<std::string>> calls;    // all direct callees (in the file or not)
    explicit Graph(const ir::Module& m) {
        for (const auto& f : m.functions) fns[f.name] = &f;
        for (const auto& f : m.functions)
            for (const auto& b : f.blocks)
                for (const auto& i : b.insts)
                    if (isCall(i) && i.src1.kind == OperandKind::GLOBAL) calls[f.name].insert(i.src1.name);
    }
    // what f reaches through the file's functions (f itself only on a cycle)
    std::set<std::string> reach(const std::string& f) const {
        std::set<std::string> seen;
        std::vector<std::string> todo;
        auto push = [&](const std::string& n) {
            auto it = calls.find(n);
            if (it == calls.end()) return;
            for (const auto& x : it->second)
                if (fns.count(x) && seen.insert(x).second) todo.push_back(x);
        };
        push(f);
        while (!todo.empty()) {
            std::string n = todo.back();
            todo.pop_back();
            push(n);
        }
        return seen;
    }
};

bool usesAlloca(const ir::Function& f) {
    for (const auto& b : f.blocks)
        for (const auto& i : b.insts)
            if (isCall(i) && i.src1.kind == OperandKind::GLOBAL &&
                (i.src1.name == "_alloca" || i.src1.name == "___builtin_alloca")) return true;
    return false;
}

// a call into the ROM, or the library's way there: what it is, "" if not
std::string romCall(const ir::Inst& i) {
    if (isCall(i) && i.src1.kind == OperandKind::GLOBAL) {
        const std::string& n = i.src1.name;
        static const char* const names[] = {"_putchar", "_getchar", "_puts", "_printf", "_vprintf", "_gets",
                                            "_fopen", "_fclose", "_fread", "_fwrite", "_fgetc", "_fputc",
                                            "_fputs", "_fgets", "_fprintf", "_fscanf", "_scanf",
                                            "___rom_call", "___rom_calli", "__rom_call", "__rom_calli", "__bp_rom"};
        for (const char* k : names) if (n == k) return cName(n) + "()";
        if (n.rfind("_cbm_", 0) == 0) return cName(n) + "()";
        return "";
    }
    if (i.op == Op::ASM_INLINE) {
        for (const char* k : {"__rom_call", "__bp_rom"})
            if (i.asmText.find(k) != std::string::npos) return std::string(k) + " (inline assembly)";
        // jsr $Exxx / $Fxxx
        std::string t = i.asmText;
        std::transform(t.begin(), t.end(), t.begin(), ::tolower);
        for (size_t p = t.find("jsr"); p != std::string::npos; p = t.find("jsr", p + 3)) {
            size_t q = t.find_first_not_of(" \t", p + 3);
            if (q != std::string::npos && t[q] == '$' && q + 4 < t.size() && (t[q + 1] == 'e' || t[q + 1] == 'f') &&
                std::isxdigit((unsigned char)t[q + 4]) && (q + 5 >= t.size() || !std::isxdigit((unsigned char)t[q + 5])))
                return "jsr " + i.asmText.substr(q, 5) + " (inline assembly)";
        }
    }
    return "";
}

void callGraphRules(Context& c, const ir::Module& m) {
    Graph g(m);
    std::set<std::string> cyclic;
    for (const auto& f : m.functions)
        if (g.reach(f.name).count(f.name)) cyclic.insert(f.name);

    for (const auto& f : m.functions) {
        if (!cyclic.count(f.name) || f.isRecurse || !c.staticAlloc || f.isInterrupt || f.isNaked) continue;
        Where w = declOf(m, f);
        bool self = g.calls[f.name].count(f.name) > 0;
        c.report("mega65-recursion-frame", w.file, w.line, w.col,
                 "'" + cName(f.name) + "' " + (self ? "calls itself" : "is on a call cycle") +
                 ": it gets a stack frame instead of static allocation (#pragma cc45 recurse before it says so)");
    }

    for (const auto& h : m.functions) {
        if (!h.isInterrupt) continue;
        std::set<std::string> reached = g.reach(h.name);
        // static-allocation functions the handler reaches
        for (const auto& n : reached) {
            const ir::Function& f = *g.fns[n];
            bool sac = c.staticAlloc && !f.isRecurse && !f.isInterrupt && !f.isNaked && !cyclic.count(n) && !usesAlloca(f);
            if (!sac) continue;
            Where w = declOf(m, f);
            c.report("mega65-sac-interrupt", w.file, w.line, w.col,
                     "'" + cName(n) + "' uses static allocation and is reachable from the interrupt handler '" +
                     cName(h.name) + "': ln45 will refuse it (put #pragma cc45 recurse before it)");
        }
        reached.insert(h.name);
        for (const auto& n : reached) {
            const ir::Function& f = *g.fns[n];
            ir::SourceLoc last;
            for (const auto& b : f.blocks)
                for (const auto& i : b.insts) {
                    if (i.loc.valid()) last = i.loc;
                    std::string what = romCall(i);
                    if (what.empty()) continue;
                    const ir::SourceLoc& at = i.loc.valid() ? i.loc : last;
                    Where w = declOf(m, f);
                    c.report("mega65-isr-rom-call", at.valid() ? at.file : w.file, at.valid() ? at.line : w.line,
                             at.valid() ? at.column : w.col,
                             what + " calls the ROM" + (n == h.name ? " in the interrupt handler '" : " in '" + cName(n) +
                             "', which the interrupt handler '") + cName(h.name) + (n == h.name ? "'" : "' reaches") +
                             ": the ROM is not reentrant (the program may be inside it)");
                }
        }
    }
}

// 32-bit arithmetic of widened operands whose result is narrowed again
void wideArith(Context& c, const ir::Function& f) {
    std::map<uint32_t, int> defs;
    std::map<uint32_t, const ir::Inst*> def;
    std::map<uint32_t, std::vector<const ir::Inst*>> uses;
    auto use = [&](const ir::Operand& o, const ir::Inst* i) { if (o.isVreg()) uses[o.vregId].push_back(i); };
    for (const auto& b : f.blocks)
        for (const auto& i : b.insts) {
            if (i.dest.isVreg()) { defs[i.dest.vregId]++; def[i.dest.vregId] = &i; }
            use(i.src1, &i);
            use(i.src2, &i);
            for (const auto& a : i.args) use(a, &i);
        }
    auto narrow = [](ir::Type t) { return t == ir::Type::I8 || t == ir::Type::I16; };
    auto fits = [&](const ir::Operand& o) {
        if (o.isImm()) return o.immVal >= 0 && o.immVal <= 0xFFFF;
        if (!o.isVreg() || defs[o.vregId] != 1) return false;
        const ir::Inst* d = def[o.vregId];
        if (d->op == Op::CONST) return d->src1.immVal >= 0 && d->src1.immVal <= 0xFFFF;
        return (d->op == Op::ZEXT || d->op == Op::SEXT) && narrow(d->src1.type);
    };
    auto isConstOp = [&](const ir::Operand& o) {
        return o.isImm() || (o.isVreg() && defs[o.vregId] == 1 && def[o.vregId]->op == Op::CONST);
    };
    for (const auto& b : f.blocks)
        for (const auto& i : b.insts) {
            const char* sym = nullptr;
            switch (i.op) {
                case Op::ADD: sym = "+"; break;
                case Op::SUB: sym = "-"; break;
                case Op::MUL: case Op::MUL_U: sym = "*"; break;
                case Op::AND: sym = "&"; break;
                case Op::OR: sym = "|"; break;
                case Op::XOR: sym = "^"; break;
                default: break;
            }
            if (!sym || i.resultType != ir::Type::I32 || !i.dest.isVreg() || defs[i.dest.vregId] != 1 || !i.loc.valid()) continue;
            if (!fits(i.src1) || !fits(i.src2) || (isConstOp(i.src1) && isConstOp(i.src2))) continue;
            const auto& us = uses[i.dest.vregId];
            if (us.empty()) continue;
            bool all = true;
            for (const ir::Inst* u : us)
                if (u->op != Op::TRUNC || !narrow(u->resultType)) all = false;
            if (!all) continue;
            c.report("mega65-wide-arith", i.loc.file, i.loc.line, i.loc.column,
                     std::string("long '") + sym + "' of operands that fit an int, its result used as one: "
                     "16-bit arithmetic gives the same value");
        }
}

// variables placed (with __section) in a section declared at an I/O address
class SectionWalker : public ASTWalker {
public:
    SectionWalker(Context& c, const ir::Module& m) : c_(c), m_(m) {}
    using ASTWalker::visit;
    void visit(VariableDeclaration& node) override {
        if (!node.section.empty() && !node.isVolatile && node.pointerLevel == 0) {
            auto it = m_.sections.find(node.section);
            if (it != m_.sections.end() && isIo(it->second))
                c_.report("mega65-io-not-volatile", node.sourceFile, node.line, node.column,
                          "'" + node.name + "' is placed in I/O (section " + node.section + " at " + hex4(it->second) +
                          ") but is not volatile: cc45 may reuse a value read from it");
        }
        ASTWalker::visit(node);
    }
private:
    Context& c_;
    const ir::Module& m_;
};

}  // namespace

// pack45 streams unpacked whose format's decoder the file does not link
// (#666): in the file with main() only (UNPACK_FORMATn may be in any one file
// of the program; ln45 checks the whole program)
void unpackFormats(Context& c, const ir::Module& m) {
    if (m.unpackSites.empty()) return;
    bool hasMain = false;
    for (const auto& f : m.functions)
        if (f.name == "_main" && !f.blocks.empty()) hasMain = true;
    if (!hasMain) return;
    std::set<int> linked = unpackfmt::declared(m.fileAsm);
    for (const auto& s : m.unpackSites) {
        if (linked.count(s.format)) continue;
        std::string n = std::to_string(s.format);
        c.report("mega65-unpack-format", s.file, s.line, s.column,
                 "a format " + n + " stream (" + s.what + ") is unpacked here, but this file does not link its decoder: "
                 "put UNPACK_FORMAT" + n + "; at file scope (unless another file of the program does, unpack() returns UNPACK_ERROR for it)");
    }
}

void runIR(Context& c, const ir::Module& m, TranslationUnit& unit) {
    unpackFormats(c, m);
    dmaAndKnock(c, m);
    callGraphRules(c, m);
    if (c.engine->levelOf("mega65-wide-arith") != lint::Level::Off)
        for (const auto& f : m.functions) wideArith(c, f);
    SectionWalker sw(c, m);
    unit.accept(sw);
}

void runFrames(Context& c, const ir::Module& m, const std::map<std::string, int>& frames) {
    int limit = 256;
    try { limit = std::stoi(c.engine->option("mega65-stack-frame-limit", "256")); } catch (...) {}
    for (const auto& f : m.functions) {
        auto it = frames.find(f.name);
        if (it == frames.end() || it->second <= limit) continue;
        Where w = declOf(m, f);
        c.report("mega65-stack-frame-limit", w.file, w.line, w.col,
                 "'" + cName(f.name) + "' has a stack frame of " + std::to_string(it->second) + " bytes (limit " +
                 std::to_string(limit) + ")");
    }
}

}  // namespace clint
