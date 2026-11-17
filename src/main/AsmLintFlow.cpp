// al45: the rules on data flow (#64 phase 3; doc/architecture/lint.md).
//
// What each 45GS02 instruction reads and writes (registers A X Y Z, flags
// N Z C V, the stack, its memory operand), a control-flow graph over the
// file's instructions, backward liveness of registers and flags, and the
// rules on them. Everything errs towards saying nothing: an instruction
// al45 does not know, a simulated operation, a patched operand, a jump it
// cannot follow, a call and a return all count as reading everything, so
// what is reported is unused on every path al45 can see.
#include "AsmLint.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

#include "AssemblerParser.hpp"

namespace asmlint {

namespace {

using ST = AssemblerParser::Statement;
using AM = AddressingMode;

enum : unsigned { A = 1, X = 2, Y = 4, Z = 8, FN = 16, FZ = 32, FC = 64, FV = 128,
                  REGS = A | X | Y | Z, FLAGS = FN | FZ | FC | FV, ALL = REGS | FLAGS, NZ = FN | FZ };

struct Fx {
    unsigned rd = 0;      // may be read
    unsigned kill = 0;    // written whatever happens (what was there is gone)
    unsigned may = 0;     // may be written
    int stack = 0;        // bytes pushed (pulled: negative)
    bool stackUnknown = false;
    enum Kind { Plain, Cond, Jump, Call, Ret, Barrier } kind = Plain;
    bool memRead = false, memWrite = false;   // of its operand
    int width = 1;        // bytes of the operand
};

std::string lower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

std::string squeeze(std::string s) {
    s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); }), s.end());
    return lower(s);
}

unsigned regOf(char c) { return c == 'a' ? (unsigned)A : c == 'x' ? (unsigned)X : c == 'y' ? (unsigned)Y : c == 'z' ? (unsigned)Z : 0u; }
const char* regName(unsigned r) { return r == A ? "A" : r == X ? "X" : r == Y ? "Y" : r == Z ? "Z" : "?"; }

std::string maskNames(unsigned m, bool flags) {
    std::string s;
    auto add = [&](unsigned b, const char* n) { if (m & b) s += (s.empty() ? "" : ", ") + std::string(n); };
    if (!flags) { add(A, "A"); add(X, "X"); add(Y, "Y"); add(Z, "Z"); }
    else { add(FC, "C"); add(FN, "N"); add(FZ, "Z"); add(FV, "V"); }
    return s;
}

// the index register an addressing mode reads
unsigned modeReads(AM m) {
    switch (m) {
        case AM::BASE_PAGE_X: case AM::ABSOLUTE_X: case AM::BASE_PAGE_X_INDIRECT: case AM::ABSOLUTE_X_INDIRECT:
        case AM::LINEAR_ABSOLUTE_X:
            return X;
        case AM::BASE_PAGE_Y: case AM::ABSOLUTE_Y: case AM::BASE_PAGE_INDIRECT_Y: case AM::BASE_PAGE_INDIRECT_SP_Y:
        case AM::STACK_RELATIVE_INDIRECT_Y: case AM::LINEAR_ABSOLUTE_Y:
            return Y;
        case AM::BASE_PAGE_INDIRECT_Z: case AM::FLAT_INDIRECT_Z:
            return Z;
        default:
            return 0;
    }
}

bool accMode(AM m) { return m == AM::ACCUMULATOR || m == AM::IMPLIED || m == AM::QUAD_Q; }

// What an instruction does. false: al45 does not know it (a barrier).
bool effects(const std::string& m, AM mode, Fx& f) {
    unsigned idx = modeReads(mode);
    auto mem = [&](bool rd, bool wr, int w = 1) { f.memRead = rd; f.memWrite = wr; f.width = w; f.rd |= idx; };
    auto wr = [&](unsigned w) { f.kill |= w; f.may |= w; };

    // loads, stores, transfers
    if (m == "lda" || m == "ldx" || m == "ldy" || m == "ldz") { wr(regOf(m[2]) | NZ); if (mode != AM::IMMEDIATE) mem(true, false); return true; }
    if (m == "ldq") { wr(REGS | NZ); mem(true, false, 4); return true; }
    if (m == "sta" || m == "stx" || m == "sty" || m == "stz") { f.rd |= regOf(m[2]); mem(false, true); return true; }
    if (m == "stq") { f.rd |= REGS; mem(false, true, 4); return true; }
    if (m.size() == 3 && m[0] == 't' && regOf(m[1]) && regOf(m[2])) { f.rd |= regOf(m[1]); wr(regOf(m[2]) | NZ); return true; }
    if (m == "tsx") { wr(X | NZ); return true; }
    if (m == "tsy") { wr(Y | NZ); return true; }
    if (m == "txs") { f.rd |= X; f.stackUnknown = true; return true; }
    if (m == "tys") { f.rd |= Y; f.stackUnknown = true; return true; }
    if (m == "tab") { f.rd |= A; return true; }
    if (m == "tba") { wr(A | NZ); return true; }

    // arithmetic and logic on A
    if (m == "adc" || m == "sbc") { f.rd |= A | FC; wr(A | NZ | FC | FV); if (mode != AM::IMMEDIATE) mem(true, false); return true; }
    if (m == "and" || m == "ora" || m == "eor") { f.rd |= A; wr(A | NZ); if (mode != AM::IMMEDIATE) mem(true, false); return true; }
    if (m == "cmp" || m == "cpx" || m == "cpy" || m == "cpz") {
        f.rd |= m == "cmp" ? A : regOf(m[2]);
        wr(NZ | FC);
        if (mode != AM::IMMEDIATE) mem(true, false);
        return true;
    }
    if (m == "bit") {
        f.rd |= A;
        if (mode == AM::IMMEDIATE) wr(FZ); else { wr(NZ | FV); mem(true, false); }
        return true;
    }
    if (m == "neg") { f.rd |= A; wr(A | NZ); return true; }
    if (m == "inc" || m == "dec") {
        if (accMode(mode)) { f.rd |= A; wr(A | NZ); } else { wr(NZ); mem(true, true); }
        return true;
    }
    if (m == "inx" || m == "dex" || m == "iny" || m == "dey" || m == "inz" || m == "dez") { unsigned r = regOf(m[2]); f.rd |= r; wr(r | NZ); return true; }
    if (m == "inw" || m == "dew") { wr(NZ); mem(true, true, 2); return true; }
    if (m == "asl" || m == "lsr" || m == "asr" || m == "rol" || m == "ror") {
        if (m[0] == 'r') f.rd |= FC;
        if (accMode(mode)) { f.rd |= A; wr(A | NZ | FC); } else { wr(NZ | FC); mem(true, true); }
        return true;
    }
    if (m == "asw") { wr(NZ | FC); mem(true, true, 2); return true; }
    if (m == "row") { f.rd |= FC; wr(NZ | FC); mem(true, true, 2); return true; }
    if (m == "tsb" || m == "trb") { f.rd |= A; wr(FZ); mem(true, true); return true; }
    if (m.size() == 4 && (m.compare(0, 3, "rmb") == 0 || m.compare(0, 3, "smb") == 0)) { mem(true, true); return true; }

    // the Q register (A X Y Z as 32 bits)
    if (m == "adcq" || m == "sbcq") { f.rd |= REGS | FC; wr(REGS | NZ | FC | FV); mem(true, false, 4); return true; }
    if (m == "andq" || m == "oraq" || m == "eorq") { f.rd |= REGS; wr(REGS | NZ); mem(true, false, 4); return true; }
    if (m == "cmpq") { f.rd |= REGS; wr(NZ | FC); mem(true, false, 4); return true; }
    if (m == "bitq") { f.rd |= REGS; wr(NZ | FV); mem(true, false, 4); return true; }
    if (m == "negq") { f.rd |= REGS; wr(REGS | NZ); return true; }
    if (m == "inq" || m == "deq") {
        if (accMode(mode)) { f.rd |= REGS; wr(REGS | NZ); } else { wr(NZ); mem(true, true, 4); }
        return true;
    }
    if (m == "aslq" || m == "lsrq" || m == "asrq" || m == "rolq" || m == "rorq") {
        if (m[0] == 'r') f.rd |= FC;
        if (accMode(mode)) { f.rd |= REGS; wr(REGS | NZ | FC); } else { wr(NZ | FC); mem(true, true, 4); }
        return true;
    }

    // flags
    if (m == "clc" || m == "sec") { wr(FC); return true; }
    if (m == "clv") { wr(FV); return true; }
    if (m == "cld" || m == "sed" || m == "cli" || m == "sei" || m == "cle" || m == "see" || m == "nop" || m == "eom") return true;
    if (m == "map") { f.rd |= REGS; return true; }

    // the stack
    if (m == "pha" || m == "phx" || m == "phy" || m == "phz") { f.rd |= regOf(m[2]); f.stack = 1; return true; }
    if (m == "pla" || m == "plx" || m == "ply" || m == "plz") { wr(regOf(m[2]) | NZ); f.stack = -1; return true; }
    if (m == "php") { f.rd |= FLAGS; f.stack = 1; return true; }
    if (m == "plp") { wr(FLAGS); f.stack = -1; return true; }
    if (m == "phw") { f.stack = 2; if (mode != AM::IMMEDIATE && mode != AM::IMMEDIATE16) mem(true, false, 2); return true; }

    // control
    static const std::map<std::string, unsigned> cond = {
        {"bcc", FC}, {"bcs", FC}, {"beq", FZ}, {"bne", FZ}, {"bmi", FN}, {"bpl", FN}, {"bvc", FV}, {"bvs", FV}};
    auto c = cond.find(m.size() == 4 && m[0] == 'l' ? m.substr(1) : m);
    if (c != cond.end()) { f.rd |= c->second; f.kind = Fx::Cond; return true; }
    if (m.size() == 4 && (m.compare(0, 3, "bbr") == 0 || m.compare(0, 3, "bbs") == 0)) { f.kind = Fx::Cond; mem(true, false); return true; }
    if (m == "bra" || m == "lbra" || m == "jmp") { f.kind = Fx::Jump; f.rd |= idx; return true; }
    if (m == "jsr" || m == "bsr" || m == "lbsr") { f.kind = Fx::Call; f.rd |= ALL; f.may |= ALL; return true; }
    if (m == "rts" || m == "rti") { f.kind = Fx::Ret; return true; }
    if (m == "rtn") { f.kind = Fx::Ret; f.stackUnknown = true; return true; }
    return false;
}

struct Node {
    int stmt = -1;               // index into p.statements
    Fx fx;
    std::string m;               // mnemonic, lower case ("" for a simulated operation)
    std::vector<int> succ;
    bool exitAll = false;        // a successor al45 cannot follow: everything is read there
    bool fallsIn = true;         // the node before runs into this one
    bool labeled = false;        // a label names it (a jump may come here)
    bool globalLabel = false;    // ... one that is not local (@name)
    bool patched = false;
    unsigned liveOut = ALL;
    // its operand: a value known now (not a relocatable symbol's)
    bool hasV = false;
    uint32_t v = 0;
    std::string key;             // what its operand names, "" when al45 cannot say (redundant loads)
    int callee = -1;             // Call: the node it calls in this file (-1: elsewhere)
    std::string target;          // Call / Jump: the operand's text
    const AssemblerParser::ProcContext* proc = nullptr;   // the proc it is in
    AM mode = AM::IMPLIED;       // its addressing mode (base page or absolute as assembled)
};

// The mode as assembled: a symbol operand is ABSOLUTE to the parser until
// the instruction's size says base page
AM assembledMode(const ST& s, const std::string& m) {
    AM md = s.instr.mode;
    if (md != AM::ABSOLUTE && md != AM::ABSOLUTE_X && md != AM::ABSOLUTE_Y) return md;
    static const std::set<std::string> q = {"ldq", "stq", "adcq", "sbcq", "andq", "oraq", "eorq", "cmpq", "bitq",
                                            "inq", "deq", "aslq", "lsrq", "asrq", "rolq", "rorq"};
    if (s.size != 2 + (q.count(m) ? 2 : 0)) return md;
    return md == AM::ABSOLUTE ? AM::BASE_PAGE : md == AM::ABSOLUTE_X ? AM::BASE_PAGE_X : AM::BASE_PAGE_Y;
}

}  // namespace

void registerFlowRules(lint::Engine& e) {
    using L = lint::Level;
    e.add({"mega65-flag-stale", L::Warning, "all",
           "A comparison whose flags are overwritten before anything tests them",
           "cmp / cpx / cpy / cpz set N, Z and C for a branch that follows. An instruction between the two\n"
           "that sets the same flags (a load, a transfer, a pull, inx, a call) leaves the branch testing\n"
           "that instruction's result: 'cmp #5 / ldx #0 / beq' branches on X, always. Reported where the\n"
           "branch reads a flag the comparison set and something else has set since, with nothing having\n"
           "read the comparison; and where every flag of a comparison is set again unread. Save the flags\n"
           "(php / plp), or compare after the load."});
    e.add({"mega65-stack-balance", L::Warning, "all",
           "A return with bytes still pushed",
           "rts and rti take the return address from the top of the stack: a byte pushed and not pulled on\n"
           "the way there is taken for it. Followed from each routine's entry (a proc, a jsr's target, an\n"
           "exported label) along every path. Two bytes pushed before rts are a jump through the stack\n"
           "(lda #>addr / pha / lda #<addr / pha / rts) and are not reported; nor is a routine that pulls\n"
           "its own return address, sets the stack pointer (txs), or reaches a place with two different\n"
           "depths."});
    e.add({"mega65-dead-reg-write", L::Info, "all",
           "A register loaded with a value nothing uses",
           "A load or transfer whose register is written again on every path before anything reads it,\n"
           "and whose N and Z nothing tests. Calls, returns, jumps out of the file and writes to the\n"
           "hypervisor's trap registers ($D640-$D67F) count as reading every register. A load from an I/O\n"
           "register ($D000-$DFFF), where the read itself matters, is not reported."});
    e.add({"mega65-redundant-load", L::Info, "all",
           "A register loaded with the value it already holds",
           "The same constant or the same memory byte loaded again with the register untouched and the\n"
           "byte not written in between, in straight-line code (a label or a call ends what al45 knows),\n"
           "and with nothing needing the N and Z the load sets. A register just stored to a byte holds\n"
           "that byte's value. I/O registers are never the same twice."});
    e.add({"mega65-dead-store", L::Info, "all",
           "A base-page byte stored twice with no read between",
           "Two stores to the same base-page byte in straight-line code with nothing reading it between\n"
           "them: the first value is lost. An indexed or indirect read, a call, a branch or a label ends\n"
           "what al45 knows. $00 and $01 (the CPU port) are not judged."});
    e.add({"mega65-zp-param-block", L::Warning, "cc45",
           "A parameter-block byte ($10-$2F) read after a call that may have changed it",
           "cc45's functions take their arguments in $10-$2F and use what is left of it as they like: after\n"
           "a call of a C function (jsr _name) those bytes are not what they were. A routine that needs\n"
           "its own arguments after such a call keeps them elsewhere (a register, the stack, its own\n"
           "storage). Reported for a read of a byte no store has set since the call, in straight-line code.\n"
           "A callee in the same file that declares what it changes (.zp_clobbers) is believed."});
    e.add({"mega65-lib-temps", L::Warning, "cc45",
           "A library temporary ($F6-$FC) read after a call that may have changed it",
           "$F6-$FC are the library's temporaries: any library routine may use them. A value kept there\n"
           "across a call of a C function (jsr _name) may be gone. As mega65-zp-param-block."});
    e.add({"mega65-clobber-mismatch", L::Warning, "all",
           "A proc changes a register or flag its .reg_clobbers / .flag_clobbers does not list",
           "ca45's optimizer and ln45 believe a proc's attributes: a caller keeps a value in a register the\n"
           "callee is declared not to change. A register the proc writes (and does not save and restore\n"
           "with a push and pull) must be in .reg_clobbers, a flag in .flag_clobbers; a proc declared leaf\n"
           "(.func_flags leaf) makes no calls. Calls of procs in the file count with what they declare;\n"
           "other calls are not judged. al45 --suggest-clobbers prints the lists for every proc."});
}

void runFlow(lint::Engine& e, AssemblerParser& p, const std::string& mainFile,
             const std::set<const void*>& patchedStmts, bool suggestClobbers, std::ostream* suggestOut) {
    auto fileOf = [&](const ST& s) { return s.asmFile.empty() ? mainFile : s.asmFile; };
    const auto& S = p.statements;

    // ---- the nodes: instructions and simulated operations, in order ----
    std::vector<Node> N;
    std::set<int> procEntries;   // each proc's first node
    // cc45's procs (.var cc45.fp): what the compiler leaves for ca45's
    // optimizer to remove is not reported as wasted work
    std::set<const AssemblerParser::ProcContext*> compiled;
    std::map<std::pair<std::string, uint32_t>, int> at;   // (segment, address) -> node
    {
        bool breakBefore = true;     // nothing runs into the next node
        bool labelBefore = false, globalBefore = false, procStart = false;
        const AssemblerParser::ProcContext* curProc = nullptr;
        static const std::set<std::string> dataDirs = {"byte", "word", "dword", "long", "text", "ascii", "asciiz", "res",
            "space", "fill", "incbin", "float", "petscii", "screencode", "align", "lword", "dbyt", "addr"};
        for (size_t i = 0; i < S.size(); i++) {
            const ST& s = *S[i];
            if (s.deleted) continue;
            if (!s.label.empty()) {
                labelBefore = true;
                if (s.label.find('@') == std::string::npos) globalBefore = true;
            }
            if (s.type == ST::NONE) continue;
            if (s.type == ST::DIRECTIVE) {
                if (curProc && s.dir.name == "var" && s.dir.varName.find("cc45.fp") != std::string::npos) compiled.insert(curProc);
                if (s.dir.name == "segment" || s.dir.name == "org" || s.dir.name == "code" || s.dir.name == "data" ||
                    s.dir.name == "bss" || dataDirs.count(s.dir.name)) {
                    // (data after code: the code before it does not run into what follows)
                    if (!N.empty() && !breakBefore) N.back().exitAll = true;
                    breakBefore = true;
                }
                continue;
            }
            if (s.type == ST::BASIC_UPSTART) { breakBefore = true; continue; }
            bool isProc = s.type == ST::INSTRUCTION && (s.instr.mnemonic == "proc" || s.instr.mnemonic == "endproc");
            if (isProc) {
                labelBefore = true;
                curProc = s.instr.mnemonic == "proc" ? s.procCtx.get() : nullptr;
                procStart = curProc != nullptr;
                continue;
            }
            Node n;
            n.stmt = (int)i;
            n.proc = curProc;
            if (procStart) { procEntries.insert((int)N.size()); procStart = false; }
            n.fallsIn = !breakBefore;
            n.labeled = labelBefore;
            n.globalLabel = globalBefore;
            n.patched = patchedStmts.count(&s) || s.selfModified;
            if (s.type == ST::INSTRUCTION) {
                n.m = lower(s.instr.mnemonic);
                n.mode = assembledMode(s, n.m);
                if (!effects(n.m, n.mode, n.fx)) { n.fx = Fx(); n.fx.kind = Fx::Barrier; }
            } else {
                n.fx.kind = Fx::Barrier;
            }
            if (n.fx.kind == Fx::Barrier) {
                n.fx.rd = ALL; n.fx.may = ALL; n.fx.kill = 0; n.fx.stackUnknown = true;
                n.fx.memRead = n.fx.memWrite = true;
            }
            if (n.patched) { n.fx.rd |= ALL; n.fx.kill = 0; }
            // the operand
            if (s.type == ST::INSTRUCTION && !n.patched) {
                AM md = n.mode;
                bool none = md == AM::IMPLIED || md == AM::ACCUMULATOR || md == AM::QUAD_Q || md == AM::NONE;
                bool reloc = !none && p.lintRelocatable(s.instr.operandTokenIndex, s.scopePrefix);
                uint32_t v = 0;
                bool ev = !none && p.lintEvalAt(s, s.instr.operandTokenIndex, v);
                n.target = squeeze(s.instr.operand);
                if (ev && !reloc) { n.hasV = true; n.v = v; }
                // what the operand names, for "the same as before"
                bool pcRel = n.target.find('*') != std::string::npos;
                if (md == AM::IMMEDIATE && !pcRel) {
                    n.key = "#" + n.target;   // (as written: #0 and #<SIZE are not "the same")
                } else if (md == AM::BASE_PAGE && ev && !reloc) {
                    if (v > 1) n.key = "b:" + std::to_string(v);
                } else if (md == AM::ABSOLUTE && !pcRel) {
                    if (ev && !reloc) { if (v >= 0x200 && !(v >= 0xD000 && v <= 0xDFFF)) n.key = "a:" + std::to_string(v); }
                    else if (reloc) n.key = "t:" + n.target;
                }
            }
            // a write to the hypervisor's trap registers: a call of it
            if (n.fx.memWrite && n.hasV && n.v >= 0xD640 && n.v <= 0xD67F) { n.fx.rd |= ALL; n.fx.may |= ALL; n.fx.kill = 0; }
            at[{s.segmentName, s.address}] = (int)N.size();
            N.push_back(n);
            breakBefore = false;
            labelBefore = globalBefore = false;
            if (n.fx.kind == Fx::Jump || n.fx.kind == Fx::Ret) breakBefore = true;
        }
    }
    if (N.empty()) return;

    // ---- the edges ----
    auto targetOf = [&](const Node& n) -> int {
        const ST& s = *S[n.stmt];
        if (n.patched) return -1;
        int tok = s.instr.operandTokenIndex;
        if (n.m.size() == 4 && (n.m.compare(0, 3, "bbr") == 0 || n.m.compare(0, 3, "bbs") == 0)) tok = s.instr.bitBranchTokenIndex;
        AM md = s.instr.mode;
        if (md != AM::RELATIVE && md != AM::RELATIVE16 && md != AM::ABSOLUTE && md != AM::BASE_PAGE_RELATIVE) return -1;
        uint32_t v;
        if (p.lintExternal(tok) || !p.lintEvalAt(s, tok, v)) return -1;
        auto it = at.find({s.segmentName, v});
        return it == at.end() ? -1 : it->second;
    };
    std::set<int> jumpedTo;
    for (size_t i = 0; i < N.size(); i++) {
        Node& n = N[i];
        bool next = i + 1 < N.size() && N[i + 1].fallsIn;
        auto fall = [&]() { if (next) n.succ.push_back((int)i + 1); else n.exitAll = true; };
        switch (n.fx.kind) {
            case Fx::Plain: case Fx::Barrier: fall(); break;
            case Fx::Cond: {
                int t = targetOf(n);
                if (t >= 0) { n.succ.push_back(t); jumpedTo.insert(t); } else n.exitAll = true;
                fall();
                break;
            }
            case Fx::Jump: {
                int t = targetOf(n);
                if (t >= 0) { n.succ.push_back(t); jumpedTo.insert(t); } else n.exitAll = true;
                break;
            }
            case Fx::Call: n.callee = targetOf(n); fall(); break;
            case Fx::Ret: n.exitAll = true; break;
        }
    }
    // a jump or branch into a node: a label names it even when written "*+5"
    for (int t : jumpedTo) N[t].labeled = true;

    // in-file callees that declare what they change
    auto calleeCtx = [&](const Node& n) -> const AssemblerParser::ProcContext* {
        if (n.callee < 0) return nullptr;
        const auto* c = N[n.callee].proc;
        return c && c->hasFuncAttrs && !c->unnamed && procEntries.count(n.callee) ? c : nullptr;
    };
    auto ctxRegs = [](const AssemblerParser::ProcContext* c) {
        unsigned m = 0;
        if (c->regClobbersMask & 1) m |= A;
        if (c->regClobbersMask & 2) m |= X;
        if (c->regClobbersMask & 4) m |= Y;
        if (c->regClobbersMask & 8) m |= Z;
        return m;
    };
    auto ctxFlags = [](const AssemblerParser::ProcContext* c) {
        unsigned m = 0;
        if (c->flagClobbersMask & 1) m |= FC;
        if (c->flagClobbersMask & 2) m |= FN;
        if (c->flagClobbersMask & 4) m |= FZ;
        if (c->flagClobbersMask & 8) m |= FV;
        return m;
    };

    // ---- liveness of registers and flags (backward, to a fixed point) ----
    {
        std::vector<unsigned> liveIn(N.size(), ALL);
        for (auto& n : N) n.liveOut = n.exitAll ? (unsigned)ALL : 0u;
        for (size_t i = 0; i < N.size(); i++) liveIn[i] = N[i].fx.rd | (N[i].liveOut & ~N[i].fx.kill);
        bool changed = true;
        while (changed) {
            changed = false;
            for (size_t k = N.size(); k-- > 0;) {
                Node& n = N[k];
                unsigned out = n.exitAll ? (unsigned)ALL : 0u;
                for (int t : n.succ) out |= liveIn[t];
                unsigned in = n.fx.rd | (out & ~n.fx.kill);
                if (out != n.liveOut || in != liveIn[k]) { n.liveOut = out; liveIn[k] = in; changed = true; }
            }
        }
    }

    // the flags an instruction reads later (a return or a jump al45 cannot
    // follow reads none here; a call reads none and leaves its own)
    std::vector<unsigned> flagsUsedAfter(N.size(), 0);
    {
        std::vector<unsigned> in(N.size(), 0);
        auto rdOf = [&](const Node& n) { return n.fx.kind == Fx::Call ? 0u : n.fx.kind == Fx::Barrier || n.patched ? (unsigned)FLAGS : n.fx.rd & FLAGS; };
        auto killOf = [&](const Node& n) {
            if (n.fx.kind != Fx::Call) return n.fx.kill & FLAGS;
            const auto* cc = calleeCtx(n);
            return cc ? ctxFlags(cc) : (unsigned)FLAGS;
        };
        bool changed = true;
        while (changed) {
            changed = false;
            for (size_t k = N.size(); k-- > 0;) {
                unsigned out = 0;
                for (int t : N[k].succ) out |= in[t];
                unsigned i2 = rdOf(N[k]) | (out & ~killOf(N[k]));
                if (out != flagsUsedAfter[k] || i2 != in[k]) { flagsUsedAfter[k] = out; in[k] = i2; changed = true; }
            }
        }
    }

    auto line = [&](const Node& n) { return S[n.stmt]->line; };
    if (getenv("AL45_DUMP")) {
        for (size_t i = 0; i < N.size(); i++) {
            const Node& n = N[i];
            const ST& s = *S[n.stmt];
            std::cerr << i << " L" << s.line << " " << (n.m.empty() ? "<op>" : n.m) << " " << s.instr.operand << " mode=" << (int)s.instr.mode
                      << " seg=" << s.segmentName << " addr=" << s.address << " v=" << (n.hasV ? (int)n.v : -1) << " key=" << n.key
                      << " callee=" << n.callee << " live=" << n.liveOut << (n.fallsIn ? "" : " !fall") << (n.labeled ? " lab" : "")
                      << (n.exitAll ? " exit" : "") << " succ=";
            for (int t : n.succ) std::cerr << t << ",";
            std::cerr << "\n";
        }
    }
    auto text = [&](const Node& n) {
        const ST& s = *S[n.stmt];
        std::string o = s.instr.operand;
        size_t b = o.find_first_not_of(" \t");
        o = b == std::string::npos ? "" : o.substr(b);
        switch (n.mode) {
            case AM::IMMEDIATE: case AM::IMMEDIATE16: o = "#" + o; break;
            case AM::BASE_PAGE_X: case AM::ABSOLUTE_X: case AM::LINEAR_ABSOLUTE_X: o += ",x"; break;
            case AM::BASE_PAGE_Y: case AM::ABSOLUTE_Y: case AM::LINEAR_ABSOLUTE_Y: o += ",y"; break;
            case AM::BASE_PAGE_INDIRECT_Y: o = "(" + o + "),y"; break;
            case AM::BASE_PAGE_INDIRECT_Z: o = "(" + o + "),z"; break;
            case AM::FLAT_INDIRECT_Z: o = "[" + o + "],z"; break;
            case AM::BASE_PAGE_X_INDIRECT: case AM::ABSOLUTE_X_INDIRECT: o = "(" + o + ",x)"; break;
            case AM::INDIRECT: case AM::ABSOLUTE_INDIRECT: o = "(" + o + ")"; break;
            default: break;
        }
        return n.m + (o.empty() ? "" : " " + o);
    };
    auto isIO = [](const Node& n) { return n.hasV && n.v >= 0xD000 && n.v <= 0xDFFF; };
    // several instructions from one source line: a macro's expansion (or a
    // line of several statements). What a macro repeats is not the writer's
    // to remove: the rules on wasted work leave it alone.
    std::map<std::pair<std::string, int>, int> perLine;
    for (const auto& n : N) perLine[{fileOf(*S[n.stmt]), S[n.stmt]->line}]++;
    auto fromMacro = [&](const Node& n) {
        return perLine[{fileOf(*S[n.stmt]), S[n.stmt]->line}] > 1 || (n.proc && compiled.count(n.proc));
    };

    // ---- mega65-dead-reg-write ----
    for (const auto& n : N) {
        if (n.patched || n.m.empty()) continue;
        const ST& s = *S[n.stmt];
        bool load = n.m == "lda" || n.m == "ldx" || n.m == "ldy" || n.m == "ldz";
        bool xfer = n.m.size() == 3 && n.m[0] == 't' && regOf(n.m[1]) && regOf(n.m[2]);
        if (!load && !xfer) continue;
        if (load) {
            AM md = n.mode;
            // (only a read that cannot matter by itself)
            if (md != AM::IMMEDIATE && md != AM::BASE_PAGE && md != AM::ABSOLUTE) continue;
            if (md != AM::IMMEDIATE && (isIO(n) || (n.hasV && n.v < 2))) continue;
        }
        if ((n.fx.kill & n.liveOut) || fromMacro(n)) continue;
        unsigned r = n.fx.kill & REGS;
        e.report("mega65-dead-reg-write", fileOf(s), s.line, 1,
                 "'" + text(n) + "': " + regName(r) + " is written again before anything reads it, and nothing tests its flags");
    }

    // ---- mega65-flag-stale (straight-line) ----
    for (size_t i = 0; i < N.size(); i++) {
        const Node& c = N[i];
        if (c.patched || !(c.m == "cmp" || c.m == "cpx" || c.m == "cpy" || c.m == "cpz" || c.m == "cmpq")) continue;
        unsigned pending = NZ | FC;        // the comparison's flags nothing has set since
        std::map<unsigned, int> killer;    // flag -> the node that set it again
        for (size_t j = i + 1; j < N.size() && pending; j++) {
            const Node& n = N[j];
            if (!n.fallsIn || n.labeled) break;              // a join: other paths arrive
            if (n.fx.kind == Fx::Barrier || n.patched) break;
            unsigned rd = n.fx.rd, kill = n.fx.kill;
            if (n.fx.kind == Fx::Call) {
                // (a call does not take a comparison's flags; it leaves its own)
                const auto* cc = calleeCtx(n);
                rd = 0;
                kill = cc ? ctxFlags(cc) : (NZ | FC);
            }
            if (n.fx.kind == Fx::Ret || n.fx.kind == Fx::Jump) break;   // (the flags go with it)
            if (rd & pending) break;                         // the comparison is used
            if (n.fx.kind == Fx::Cond) {
                unsigned stale = n.fx.rd & (NZ | FC) & ~pending;
                // (a flag of the comparison still to be read, the carry after
                // "cmp / lda / beq / bcs": the comparison is used)
                if (stale && killer.count(stale) && !(pending & flagsUsedAfter[j])) {
                    const Node& k = N[killer[stale]];
                    e.report("mega65-flag-stale", fileOf(*S[n.stmt]), line(n), 1,
                             "'" + n.m + "' tests the flags of '" + text(k) + "' (line " + std::to_string(line(k)) +
                             "), not of '" + text(c) + "' (line " + std::to_string(line(c)) + "), whose result nothing reads");
                }
                break;   // (the paths part here)
            }
            for (unsigned f : {(unsigned)FN, (unsigned)FZ, (unsigned)FC})
                if ((kill & f) && (pending & f)) { killer[f] = (int)j; pending &= ~f; }
            if (!pending) {
                e.report("mega65-flag-stale", fileOf(*S[c.stmt]), line(c), 1,
                         "the flags of '" + text(c) + "' are all set again (the last by '" + text(n) + "', line " +
                         std::to_string(line(n)) + ") before anything reads them");
            }
        }
    }

    // ---- mega65-redundant-load and mega65-dead-store (straight-line) ----
    {
        // what a register is known to equal: operand keys ("#5", "b:16", ...)
        std::map<unsigned, std::set<std::string>> same;
        std::map<uint32_t, int> stored;      // base-page byte -> the node of a store nothing has read
        unsigned flagsOf = 0;                // the register N and Z describe (0: unknown)
        auto forgetAll = [&]() { same.clear(); stored.clear(); flagsOf = 0; };
        auto forgetMem = [&](const std::string& exceptKey, bool relocWrite) {
            for (auto& [r, ks] : same)
                for (auto it = ks.begin(); it != ks.end();) {
                    const std::string& k = *it;
                    bool mem = k[0] != '#';
                    bool drop = mem && (exceptKey.empty() || relocWrite || k[0] == 't' || k == exceptKey);
                    it = drop ? ks.erase(it) : std::next(it);
                }
        };
        for (size_t i = 0; i < N.size(); i++) {
            const Node& n = N[i];
            const ST& s = *S[n.stmt];
            if (!n.fallsIn || n.labeled) forgetAll();
            if (n.fx.kind == Fx::Barrier || n.patched || n.fx.kind == Fx::Call) { forgetAll(); continue; }
            if (n.fx.kind == Fx::Jump || n.fx.kind == Fx::Ret) { forgetAll(); continue; }
            AM md = n.mode;
            bool bpKnown = md == AM::BASE_PAGE && n.hasV;

            // reads of base-page bytes: the stores before them were needed
            if (n.fx.memRead || n.fx.kind == Fx::Cond) {
                if (bpKnown) for (int k = 0; k < n.fx.width; k++) stored.erase(n.v + k);
                else if (n.fx.memRead) stored.clear();
            }
            if (n.fx.kind == Fx::Cond) { stored.clear(); continue; }   // (the other path may read them)

            bool load = n.m == "lda" || n.m == "ldx" || n.m == "ldy" || n.m == "ldz";
            if (load) {
                unsigned r = regOf(n.m[2]);
                if (!n.key.empty() && same[r].count(n.key) && (!(n.liveOut & NZ) || flagsOf == r) && !fromMacro(n)) {
                    std::string what = n.key[0] == '#' ? "that value" : "that byte's value";
                    e.report("mega65-redundant-load", fileOf(s), s.line, 1,
                             "'" + text(n) + "': " + regName(r) + " already holds " + what);
                } else {
                    same[r].clear();
                    if (!n.key.empty()) same[r].insert(n.key);
                    flagsOf = r;
                }
                continue;
            }
            bool store = n.m == "sta" || n.m == "stx" || n.m == "sty" || n.m == "stz";
            if (store) {
                unsigned r = regOf(n.m[2]);
                if (bpKnown && n.v > 1) {
                    auto it = stored.find(n.v);
                    if (it != stored.end() && !fromMacro(N[it->second]) && !fromMacro(n)) {
                        const Node& first = N[it->second];
                        char b[8];
                        std::snprintf(b, sizeof b, "$%02X", n.v);
                        e.report("mega65-dead-store", fileOf(*S[first.stmt]), line(first), 1,
                                 "'" + text(first) + "': " + b + " is stored again (line " + std::to_string(s.line) +
                                 ") before anything reads it");
                    }
                    stored[n.v] = (int)i;
                } else if (md != AM::ABSOLUTE || !n.hasV || n.v < 0x100) {
                    stored.clear();   // (it may be a base-page byte by another name)
                }
                forgetMem(n.key, n.key.empty() || n.key[0] == 't');
                if (!n.key.empty()) same[r].insert(n.key);
                continue;
            }
            // anything else: what it writes is no longer known
            if (n.fx.memWrite) {
                if (bpKnown) for (int k = 0; k < n.fx.width; k++) stored.erase(n.v + k);
                else stored.clear();
                forgetMem(n.fx.width == 1 ? n.key : std::string(), n.key.empty() || n.key[0] == 't');
            }
            bool xfer = n.m.size() == 3 && n.m[0] == 't' && regOf(n.m[1]) && regOf(n.m[2]);
            if (xfer) {
                same[regOf(n.m[2])] = same[regOf(n.m[1])];
                flagsOf = regOf(n.m[2]);
                continue;
            }
            for (unsigned r : {(unsigned)A, (unsigned)X, (unsigned)Y, (unsigned)Z})
                if (n.fx.may & r) same[r].clear();
            if (n.fx.may & NZ) flagsOf = 0;
        }
    }

    // ---- mega65-zp-param-block, mega65-lib-temps (cc45 profile; straight-line) ----
    if (e.profile == "cc45") {
        std::map<uint32_t, int> gone;   // base-page byte -> the call after which it is not known
        for (size_t i = 0; i < N.size(); i++) {
            const Node& n = N[i];
            const ST& s = *S[n.stmt];
            if (!n.fallsIn || n.labeled) gone.clear();
            if (n.fx.kind == Fx::Barrier || n.patched || n.fx.kind == Fx::Jump || n.fx.kind == Fx::Ret) { gone.clear(); continue; }
            if (n.fx.kind == Fx::Call) {
                // a C function: _name (not __name, the runtime's own routines)
                bool cfn = n.target.size() > 1 && n.target[0] == '_' && n.target[1] != '_';
                const auto* cc = calleeCtx(n);
                if (cc && cc->zpClobbersMask.any()) {
                    for (uint32_t a = 0x10; a <= 0xFC; a++)
                        if ((a <= 0x2F || a >= 0xF6) && cc->zpClobbersMask.test(a)) gone[a] = (int)i;
                } else if (cfn && !cc) {
                    for (uint32_t a = 0x10; a <= 0x2F; a++) gone[a] = (int)i;
                    for (uint32_t a = 0xF6; a <= 0xFC; a++) gone[a] = (int)i;
                } else if (!cc) {
                    gone.clear();   // (a routine al45 knows nothing of: it may put them back)
                }
                continue;
            }
            if (!n.hasV || gone.empty()) continue;
            AM md = n.mode;
            bool direct = md == AM::BASE_PAGE;
            bool pointer = md == AM::BASE_PAGE_INDIRECT_Y || md == AM::BASE_PAGE_INDIRECT_Z || md == AM::FLAT_INDIRECT_Z ||
                           md == AM::BASE_PAGE_X_INDIRECT;
            if (!direct && !pointer) continue;
            int w = pointer ? (md == AM::FLAT_INDIRECT_Z ? 4 : 2) : n.fx.width;
            bool reads = pointer || n.fx.memRead;
            if (md == AM::BASE_PAGE_X_INDIRECT) continue;   // (which bytes depends on X)
            if (reads) {
                for (int k = 0; k < w; k++) {
                    auto it = gone.find(n.v + k);
                    if (it == gone.end()) continue;
                    const Node& call = N[it->second];
                    bool param = n.v + k <= 0x2F;
                    char b[8];
                    std::snprintf(b, sizeof b, "$%02X", n.v + k);
                    e.report(param ? "mega65-zp-param-block" : "mega65-lib-temps", fileOf(s), s.line, 1,
                             "'" + text(n) + "' reads " + b + " after 'jsr " + call.target + "' (line " +
                             std::to_string(line(call)) + "), which may have changed " +
                             (param ? "the parameter block" : "the library's temporaries"));
                    gone.erase(it);   // (once per byte)
                    break;
                }
            }
            if (direct && n.fx.memWrite && !n.fx.memRead)
                for (int k = 0; k < n.fx.width; k++) gone.erase(n.v + k);
        }
    }

    // ---- routine entries: procs, the targets of calls, exported labels ----
    std::set<int> entries;
    {
        std::set<std::string> exported;
        for (const auto& sp : S)
            if (sp->type == ST::DIRECTIVE && (sp->dir.name == "global" || sp->dir.name == "export"))
                for (const auto& a : sp->dir.arguments) exported.insert(a);
        std::vector<std::string> pending;
        size_t ni = 0;
        for (size_t i = 0; i < S.size() && ni < N.size(); i++) {
            const ST& s = *S[i];
            if (!s.label.empty()) pending.push_back(s.label);
            if ((int)i != N[ni].stmt) continue;
            bool entry = false;
            for (const auto& l : pending) if (exported.count(l)) entry = true;
            if (procEntries.count((int)ni)) entry = true;
            if (entry) entries.insert((int)ni);
            pending.clear();
            ni++;
        }
        for (const auto& n : N) if (n.callee >= 0) entries.insert(n.callee);
        // hand-written code with no proc: the file's first instruction, and a
        // global label nothing runs or jumps into (called from elsewhere)
        entries.insert(0);
        for (size_t i = 0; i < N.size(); i++)
            if (N[i].globalLabel && !N[i].fallsIn && !jumpedTo.count((int)i)) entries.insert((int)i);
    }

    // ---- mega65-stack-balance: the depth along every path from each entry ----
    {
        std::set<int> reported;
        for (int entry : entries) {
            std::map<int, int> depth;                 // node -> bytes pushed on arrival
            std::map<int, int> pushedAt;              // node -> the node of the innermost push still there
            std::vector<int> work = {entry};
            depth[entry] = 0;
            pushedAt[entry] = -1;
            bool tainted = false;
            std::vector<std::pair<int, int>> finds;   // (return node, depth)
            std::map<int, int> findPush;
            size_t steps = 0;
            while (!work.empty() && !tainted && steps++ < 200000) {
                int k = work.back();
                work.pop_back();
                const Node& n = N[k];
                int d = depth[k];
                if (n.fx.stackUnknown && n.fx.kind != Fx::Ret) { tainted = true; break; }
                if (n.fx.kind == Fx::Ret) {
                    if (n.fx.stackUnknown) { tainted = true; break; }   // (rtn takes more with it)
                    if (d > 0 && !(d == 2 && n.m == "rts")) { finds.push_back({k, d}); findPush[k] = pushedAt[k]; }
                    continue;
                }
                int nd = d + n.fx.stack;
                if (nd < 0) { tainted = true; break; }   // (it takes its return address: its own business)
                int pa = n.fx.stack > 0 ? k : pushedAt[k];
                for (int t : n.succ) {
                    // (leaving for another routine: not this one's return)
                    if (n.fx.kind == Fx::Jump && entries.count(t) && t != entry) continue;
                    auto it = depth.find(t);
                    if (it == depth.end()) { depth[t] = nd; pushedAt[t] = pa; work.push_back(t); }
                    else if (it->second != nd) { tainted = true; break; }
                }
            }
            if (tainted || steps >= 200000) continue;
            for (const auto& [k, d] : finds) {
                if (!reported.insert(k).second) continue;
                const Node& n = N[k];
                std::string where;
                if (findPush[k] >= 0) where = " ('" + text(N[findPush[k]]) + "', line " + std::to_string(line(N[findPush[k]])) + ")";
                e.report("mega65-stack-balance", fileOf(*S[n.stmt]), line(n), 1,
                         "'" + n.m + "' with " + std::to_string(d) + " byte" + (d == 1 ? "" : "s") + " still pushed" + where +
                         ": the return address is under " + (d == 1 ? "it" : "them"));
            }
        }
    }

    // ---- mega65-clobber-mismatch, --suggest-clobbers ----
    {
        struct Body { std::vector<int> nodes; int firstLine = 0; std::string file; };
        std::map<const AssemblerParser::ProcContext*, Body> procs;
        std::vector<const AssemblerParser::ProcContext*> order;
        for (size_t i = 0; i < N.size(); i++) {
            const ST& s = *S[N[i].stmt];
            const auto* pc = N[i].proc;
            if (!pc || pc->unnamed || pc->name.empty()) continue;
            auto& b = procs[pc];
            if (b.nodes.empty()) { order.push_back(pc); b.firstLine = s.line; b.file = fileOf(s); }
            b.nodes.push_back((int)i);
        }
        for (const auto* ctx : order) {
            const Body& b = procs[ctx];
            // What the proc changes, along its paths from its entry. A
            // register pushed (pha, phx...) is saved until its pull: what is
            // written to it meanwhile is undone, and the pull is the
            // restoring, not a change. php / plp: the flags. Pushed and not
            // pulled here (the routine it jumps to pulls it): saved to the end.
            std::map<unsigned, int> firstWrite;       // register / flag -> node
            unsigned unknownCalls = 0, written = 0;
            bool calls = false, unknown = false;
            std::string unknownName;
            {
                struct St { int k; int sv[5]; };       // saved depth of A, X, Y, Z, the flags
                auto slot = [](unsigned bit) { return bit == A ? 0 : bit == X ? 1 : bit == Y ? 2 : bit == Z ? 3 : 4; };
                std::set<int> seen;
                std::vector<St> work;
                work.push_back({b.nodes[0], {0, 0, 0, 0, 0}});
                seen.insert(b.nodes[0]);
                auto note = [&](unsigned w, int k, const St& st) {
                    for (unsigned bit = 1; bit < 256; bit <<= 1)
                        if ((w & bit) && st.sv[slot(bit)] == 0) {
                            written |= bit;
                            if (!firstWrite.count(bit) || k < firstWrite[bit]) firstWrite[bit] = k;
                        }
                };
                while (!work.empty()) {
                    St st = work.back();
                    work.pop_back();
                    const Node& n = N[st.k];
                    if (n.fx.kind == Fx::Barrier || n.patched) unknown = true;
                    else if (n.m.size() == 3 && n.m[0] == 'p' && n.m[1] == 'h' && regOf(n.m[2])) st.sv[slot(regOf(n.m[2]))]++;
                    else if (n.m == "php") st.sv[4]++;
                    else if (n.m.size() == 3 && n.m[0] == 'p' && n.m[1] == 'l' && regOf(n.m[2])) {
                        int sl = slot(regOf(n.m[2]));
                        if (st.sv[sl] > 0) { st.sv[sl]--; note(NZ, st.k, st); }   // (restored; the pull sets N and Z)
                        else note(n.fx.kill, st.k, st);
                    } else if (n.m == "plp") {
                        // (not pushed here: the flags of whoever pushed them, not this proc's doing)
                        if (st.sv[4] > 0) st.sv[4]--;
                    } else if (n.fx.kind == Fx::Call) {
                        calls = true;
                        const auto* cc = calleeCtx(n);
                        if (cc) note(ctxRegs(cc) | ctxFlags(cc), st.k, st);
                        else { unknownCalls++; if (unknownName.empty()) unknownName = n.target; }
                    } else {
                        note(n.fx.kill, st.k, st);
                    }
                    for (int t : n.succ) {
                        if (N[t].proc != ctx || !seen.insert(t).second) continue;
                        St nx = st;
                        nx.k = t;
                        work.push_back(nx);
                    }
                }
            }
            unsigned declR = ctxRegs(ctx), declF = ctxFlags(ctx);
            if (ctx->hasFuncAttrs && !unknown) {
                unsigned missR = written & REGS & ~declR, missF = written & FLAGS & ~declF;
                // (attributes that list no register and no flag at all say nothing about them)
                bool declares = ctx->regClobbersMask || ctx->flagClobbersMask;
                if (declares && missR) {
                    unsigned first = missR & (~missR + 1);
                    const Node& n = N[firstWrite[first]];
                    e.report("mega65-clobber-mismatch", b.file, line(n), 1,
                             "proc " + ctx->name + " changes " + maskNames(missR, false) + " ('" + text(n) + "'), which its .reg_clobbers does not list");
                }
                if (declares && missF) {
                    unsigned first = missF & (~missF + 1);
                    const Node& n = N[firstWrite[first]];
                    e.report("mega65-clobber-mismatch", b.file, line(n), 1,
                             "proc " + ctx->name + " changes the flag" + (missF & (missF - 1) ? "s " : " ") + maskNames(missF, true) +
                             " ('" + text(n) + "'), which its .flag_clobbers does not list");
                }
                if ((ctx->funcFlags & 1) && calls) {
                    for (int k : b.nodes)
                        if (N[k].fx.kind == Fx::Call) {
                            e.report("mega65-clobber-mismatch", b.file, line(N[k]), 1,
                                     "proc " + ctx->name + " is declared leaf (.func_flags) and calls " + N[k].target);
                            break;
                        }
                }
            }
            if (suggestClobbers && suggestOut) {
                unsigned sr = written & REGS, sf = written & FLAGS;
                if (unknownCalls || unknown) { sr = REGS; sf = FLAGS; }
                *suggestOut << b.file << ":" << b.firstLine << ": proc " << ctx->name << ": .reg_clobbers "
                            << (sr ? maskNames(sr, false) : std::string("(none)")) << "  .flag_clobbers "
                            << (sf ? maskNames(sf, true) : std::string("(none)"));
                if (unknown) *suggestOut << "  ; (an operation al45 does not follow: everything)";
                else if (unknownCalls) *suggestOut << "  ; (calls " << unknownName << (unknownCalls > 1 ? ", ..." : "") << ": everything)";
                if (!calls && !unknown) *suggestOut << "  ; leaf";
                *suggestOut << "\n";
            }
        }
    }
}

}  // namespace asmlint
