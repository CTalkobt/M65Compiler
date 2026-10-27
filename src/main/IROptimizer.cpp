#include <functional>
#include <optional>
#include "IROptimizer.hpp"
#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <vector>

namespace ir {

std::ostream* optTrace = nullptr;
std::function<bool(const Function&, const char*)> passEnabled;

static bool runOn(const Function& fn, const char* pass) {
    if (fn.noOptimize) return false;    // #pragma cc45 optimize(off)
    return !passEnabled || passEnabled(fn, pass);
}

// A direct call of a __pure / __attribute__((pure)) function: 1, or of an
// __attribute__((const)) one: 2; else 0 (Issue #76)
static int purityOf(const Module& mod, const Inst& in) {
    if ((in.op != Op::CALL && in.op != Op::CALL_VOID) || in.src1.kind != OperandKind::GLOBAL) return 0;
    auto it = mod.pureFunctions.find(in.src1.name);
    return it == mod.pureFunctions.end() ? 0 : it->second;
}

static bool sameOperand(const Operand& a, const Operand& b) {
    return a.kind == b.kind && a.vregId == b.vregId && a.immVal == b.immVal && a.name == b.name;
}

static void trace(const char* pass, const std::string& func, const std::string& action,
                  const SourceLoc& loc = {}) {
    if (optTrace) {
        *optTrace << "[" << pass << "] " << func;
        if (loc.valid()) {
            std::string f = loc.file;
            auto slash = f.rfind('/');
            if (slash != std::string::npos) f = f.substr(slash + 1);
            *optTrace << " (" << f << ":" << loc.line << ")";
        }
        *optTrace << ": " << action << "\n";
    }
}

namespace {

struct ExprKey {
    Op op;
    Operand src1;
    Operand src2;
    Type type;
    uint8_t byteWidth;
    std::string name;
    int elemOffset = 0;   // ADDR_ELEM / LOAD: bytes added to a global base
    std::string base;     // LOAD: the global its address is in (#320)

    bool operator==(const ExprKey& o) const {
        return op == o.op && elemOffset == o.elemOffset &&
               src1.kind == o.src1.kind && src1.vregId == o.src1.vregId && src1.immVal == o.src1.immVal && src1.name == o.src1.name &&
               src2.kind == o.src2.kind && src2.vregId == o.src2.vregId && src2.immVal == o.src2.immVal && src2.name == o.src2.name &&
               type == o.type &&
               byteWidth == o.byteWidth &&
               name == o.name;
    }
};

bool isCSECandidate(Op op);

// Instructions that never write memory (#320: loads of globals stay valid
// across them); anything else may
bool memoryNeutral(Op op) {
    if (isCSECandidate(op)) return true;
    switch (op) {
        case Op::CONST: case Op::FCONST: case Op::LOAD: case Op::LOAD_ZP: case Op::ADDR_LOCAL:
        case Op::ADDR_UPLEVEL: case Op::BFEXT: case Op::BFINS: case Op::COPY: case Op::DEREF: case Op::BR:
        case Op::BR_COND: case Op::SWITCH: case Op::RET: case Op::RET_VOID: case Op::PHI: case Op::GET_FP:
        case Op::NOP: case Op::CPU_REG_READ: case Op::CPU_FLAG_READ: case Op::FADD: case Op::FSUB:
        case Op::FMUL: case Op::FDIV: case Op::FCMP: case Op::FNEG: case Op::FTOI: case Op::ITOF:
            return true;
        default:
            return false;
    }
}

bool isCSECandidate(Op op) {
    switch (op) {
        // CONST/FCONST excluded: cheap to recompute (lda #imm), CSE wastes ZP slots
        // ADDR_LOCAL excluded: frame-offset computation, CSE extends live range unnecessarily
        case Op::ADD: case Op::SUB: case Op::MUL: case Op::DIV: case Op::MOD:
        case Op::MUL_U: case Op::DIV_U: case Op::MOD_U:
        case Op::AND: case Op::OR: case Op::XOR:
        case Op::LSL: case Op::LSR: case Op::ASR:
        case Op::SHL: case Op::SHR:
        case Op::NEG: case Op::NOT:
        case Op::CMP_EQ: case Op::CMP_NE:
        case Op::CMP_LT: case Op::CMP_LE: case Op::CMP_GT: case Op::CMP_GE:
        case Op::CMP_LTU: case Op::CMP_LEU: case Op::CMP_GTU: case Op::CMP_GEU:
        case Op::SEXT: case Op::ZEXT: case Op::TRUNC:
        case Op::ADDR_GLOBAL: case Op::ADDR_LABEL: case Op::ADDR_ELEM:
            return true;
        default:
            return false;
    }
}

void propagateOperand(Operand& op, const std::map<uint32_t, Operand>& copies) {
    if (op.isVreg()) {
        auto it = copies.find(op.vregId);
        if (it != copies.end()) {
            // The use keeps its own type: a copy may change the width
            // (copy i8 of an i16 value is its low byte)
            Type useType = op.type;
            op = it->second;
            if (op.isVreg()) op.type = useType;
            // Recursively resolve
            propagateOperand(op, copies);
        }
    }
}


// Vregs that are constants: defined once, by a CONST, never stored to and not
// a variable's slot (vregs are not SSA: a && b's result is CONST 0 on one path)
std::map<uint32_t, int64_t> singleDefConsts(const Function& fn) {
    std::map<uint32_t, int64_t> consts;
    std::map<uint32_t, int> defs;
    for (const auto& b : fn.blocks)
        for (const auto& i : b.insts) {
            if (i.dest.isVreg()) defs[i.dest.vregId]++;
            if ((i.op == Op::STORE || i.op == Op::STORE_ZP) && i.src2.isVreg()) defs[i.src2.vregId] += 2;
            if (i.op == Op::CONST && i.dest.isVreg() && i.src1.isImm()) consts[i.dest.vregId] = i.src1.immVal;
        }
    for (auto it = consts.begin(); it != consts.end();) {
        if (defs[it->first] != 1 || fn.localSlotVregs.count(it->first)) it = consts.erase(it);
        else ++it;
    }
    // A variable (a local's or parameter's slot) assigned a constant exactly
    // once, before anything reads it: that constant everywhere (one = 1;
    // one * x). The store must be in the entry block ahead of every read
    // there, and nothing branches back to the entry block, so it comes
    // first on every path (a parameter's incoming value is never seen). Not
    // for a variable whose address is taken, that is volatile or that a
    // nested function reaches (memoryVregs), register variables, nor in a
    // function with inline assembly (it may name a variable's slot).
    if (!fn.blocks.empty()) {
        bool unsafe = false;
        const std::string& entryLabel = fn.blocks[0].label;
        if (fn.addressTakenLabels.count(entryLabel)) unsafe = true;
        auto isEntry = [&](const Operand& o) { return o.kind == OperandKind::LABEL && o.name == entryLabel; };
        std::map<uint32_t, int> slotDefs;
        for (const auto& b : fn.blocks)
            for (const auto& i : b.insts) {
                if (i.op == Op::ASM_INLINE) unsafe = true;
                if (isEntry(i.src1) || isEntry(i.src2)) unsafe = true;
                for (const auto& a2 : i.args) if (isEntry(a2)) unsafe = true;
                if (i.dest.isVreg() && fn.localSlotVregs.count(i.dest.vregId)) slotDefs[i.dest.vregId] += 2;
                if ((i.op == Op::STORE || i.op == Op::STORE_ZP) && i.src2.isVreg() &&
                    fn.localSlotVregs.count(i.src2.vregId))
                    slotDefs[i.src2.vregId]++;
            }
        if (!unsafe) {
            std::set<uint32_t> readSoFar;
            const auto& entry = fn.blocks[0];
            for (const auto& i : entry.insts) {
                bool store = (i.op == Op::STORE || i.op == Op::STORE_ZP) && i.src2.isVreg();
                if (store) {
                    uint32_t slot = i.src2.vregId;
                    int64_t v = 0;
                    bool known = false;
                    if (i.src1.isImm()) { v = i.src1.immVal; known = true; }
                    else if (i.src1.isVreg() && consts.count(i.src1.vregId)) { v = consts[i.src1.vregId]; known = true; }
                    auto st = fn.vregTypes.find(slot);
                    if (known && fn.localSlotVregs.count(slot) && slotDefs[slot] == 1 && !readSoFar.count(slot) &&
                        !fn.memoryVregs.count(slot) && !fn.registerVregs.count(slot) &&
                        !fn.registerXVregs.count(slot) && !fn.registerYVregs.count(slot) &&
                        !fn.registerZVregs.count(slot) && st != fn.vregTypes.end() && st->second == i.resultType &&
                        i.resultType != Type::F32)
                        consts[slot] = v;
                }
                auto reads = [&](const Operand& o) { if (o.isVreg()) readSoFar.insert(o.vregId); };
                reads(i.src1);
                if (!store) reads(i.src2);
                for (const auto& a2 : i.args) reads(a2);
            }
        }
    }

    // A zext/sext/trunc of such a constant (a 0 of another width: x ^ 0 on a
    // char or long) is the converted constant, followed through chains
    auto bits = [](Type t) -> int {
        switch (t) {
            case Type::I8: return 8;
            case Type::I16: case Type::PTR: return 16;
            case Type::I32: return 32;
            default: return 0;
        }
    };
    for (bool changed = true; changed;) {
        changed = false;
        for (const auto& b : fn.blocks)
            for (const auto& i : b.insts) {
                if ((i.op != Op::ZEXT && i.op != Op::SEXT && i.op != Op::TRUNC) || !i.dest.isVreg() ||
                    !i.src1.isVreg() || consts.count(i.dest.vregId) || defs[i.dest.vregId] != 1 ||
                    fn.localSlotVregs.count(i.dest.vregId))
                    continue;
                auto c = consts.find(i.src1.vregId);
                if (c == consts.end()) continue;
                auto st = fn.vregTypes.find(i.src1.vregId);
                int from = st != fn.vregTypes.end() ? bits(st->second) : 0;
                int to = bits(i.resultType);
                if (!from || !to) continue;
                int64_t v = c->second;
                int w = i.op == Op::TRUNC ? to : from;
                int64_t mask = (w >= 64) ? -1 : ((int64_t)1 << w) - 1;
                v &= mask;
                if (i.op == Op::SEXT && (v >> (from - 1)) & 1) v |= ~mask;
                /* as a value of the destination width, sign-extended: a top
                   bit set reads as negative, so no pass takes 0x8000 for the
                   power of two 32768 in a signed division */
                v &= ((int64_t)1 << to) - 1;
                if ((v >> (to - 1)) & 1) v |= ~(((int64_t)1 << to) - 1);
                consts[i.dest.vregId] = v;
                changed = true;
            }
    }
    return consts;
}
} // namespace

// restrict pointer parameters (Issue #77) whose address the function never
// takes and that it assigns only by stepping them (p++, p += k, p = p - 1:
// still into the same object): what each points to is an object of its own
static std::set<uint32_t> restrictParams(const Function& fn) {
    std::set<uint32_t> r(fn.restrictVregs.begin(), fn.restrictVregs.end());
    if (r.empty()) return r;
    std::map<uint32_t, int> nDefs;
    std::map<uint32_t, const Inst*> defOf;
    for (const auto& b : fn.blocks)
        for (const auto& in : b.insts)
            if (in.dest.isVreg()) { nDefs[in.dest.vregId]++; defOf[in.dest.vregId] = &in; }
    // v is p + / - an integer (through temporaries defined once)
    std::function<bool(const Operand&, uint32_t, int)> stepOf = [&](const Operand& v, uint32_t p, int depth) {
        if (!v.isVreg() || depth > 4) return false;
        auto n = nDefs.find(v.vregId);
        if (n == nDefs.end() || n->second != 1) return false;
        const Inst& d = *defOf[v.vregId];
        if (d.op == Op::DEREF) return d.src1.isVreg() && d.src1.vregId == p;   // (p's value read before it changes)
        if (d.op == Op::COPY) return stepOf(d.src1, p, depth + 1);
        if ((d.op != Op::ADD && d.op != Op::SUB) || d.resultType != Type::PTR) return false;
        if (d.src2.isVreg() && d.src2.type == Type::PTR) return false;
        return d.src1.isVreg() && (d.src1.vregId == p || stepOf(d.src1, p, depth + 1));
    };
    for (const auto& b : fn.blocks)
        for (const auto& in : b.insts)
            if ((in.op == Op::STORE || in.op == Op::STORE_ZP) && in.src2.isVreg() && r.count(in.src2.vregId) &&
                !stepOf(in.src1, in.src2.vregId, 0)) {
                r.erase(in.src2.vregId);
            }
    for (uint32_t v : fn.memoryVregs) r.erase(v);
    return r;
}

// The restrict parameter an address is made from (its value, + / - an
// offset, an element of it, through temporaries defined once): its vreg; -2
// a global's address; -1 unknown
static long addressRoot(const Operand& o, const std::set<uint32_t>& restrictSlots,
                        const std::map<uint32_t, int>& nDefs, const std::map<uint32_t, const Inst*>& defOf, int depth = 0) {
    if (o.kind == OperandKind::GLOBAL) return -2;
    if (!o.isVreg() || depth > 8) return -1;
    if (restrictSlots.count(o.vregId)) return o.vregId;
    auto n = nDefs.find(o.vregId);
    if (n == nDefs.end() || n->second != 1) return -1;
    const Inst& d = *defOf.at(o.vregId);
    switch (d.op) {
        case Op::ADDR_GLOBAL: return -2;
        case Op::DEREF:   // (a variable's value, read where it changes next: *p++)
        case Op::ADDR_ELEM: case Op::COPY: return addressRoot(d.src1, restrictSlots, nDefs, defOf, depth + 1);
        case Op::ADD: case Op::SUB:
            return d.resultType == Type::PTR ? addressRoot(d.src1, restrictSlots, nDefs, defOf, depth + 1) : -1;
        default: return -1;
    }
}

void optimizeCSE(Module& mod) {
    for (auto& fn : mod.functions) {
        if (!runOn(fn, "CommonSubexprElim")) continue;
        const auto& slots = fn.localSlotVregs;
        // The global an address is in (#320): @g, addr_global @g, addr_elem
        // of @g, a pointer added to one of those, through temporaries defined
        // once; "#" for a local's (no global changes through it), "" unknown.
        // A load from a global not declared volatile can be reused until a
        // store into the same global (or through an unknown pointer, a call,
        // asm) -- distinct objects do not overlap in C
        std::map<uint32_t, int> nDefs;
        std::map<uint32_t, const Inst*> defOf;
        for (const auto& b : fn.blocks)
            for (const auto& in : b.insts)
                if (in.dest.isVreg()) { nDefs[in.dest.vregId]++; defOf[in.dest.vregId] = &in; }
        auto isVolatileGlobal = [&](const std::string& g) {
            if (mod.volatileGlobals.count(g)) return true;
            size_t at = g.rfind("__local_");    // a function's static local (_<f>__local_<name>)
            return at != std::string::npos && mod.volatileGlobals.count("_" + g.substr(at + 8));
        };
        // A restrict pointer parameter the function never assigns (Issue
        // #77): what it points to is an object of its own ("&r<vreg>"),
        // which no store into a global or through another restrict pointer
        // changes (only a store through it, or a pointer made from it, or
        // through an unknown pointer, a call, asm)
        const std::set<uint32_t> restrictSlots = restrictParams(fn);
        // The variables a store through a pointer, a call or asm may change:
        // those whose address is taken (or that live in memory: volatile).
        // Every variable in a function with inline asm (it can name one) or
        // that calls setjmp, and in a file with nested functions (they reach
        // their parent's variables through the static chain)
        std::set<uint32_t> exposedSlots(fn.memoryVregs.begin(), fn.memoryVregs.end());
        bool allExposed = std::any_of(mod.functions.begin(), mod.functions.end(),
                                      [](const Function& f) { return f.isNested || f.staticLinkVreg != -1; });
        for (const auto& b : fn.blocks)
            for (const auto& in : b.insts) {
                if (in.op == Op::ADDR_LOCAL && in.src1.isVreg()) exposedSlots.insert(in.src1.vregId);
                if (in.op == Op::ASM_INLINE || in.op == Op::ADDR_UPLEVEL || in.op == Op::BR_INDIRECT) allExposed = true;
                if ((in.op == Op::CALL || in.op == Op::CALL_VOID) && in.src1.name.find("setjmp") != std::string::npos)
                    allExposed = true;
            }
        if (allExposed) exposedSlots.insert(slots.begin(), slots.end());
        auto exposed = [&](uint32_t v) { return exposedSlots.count(v) && !restrictSlots.count(v); };
        std::function<std::string(const Operand&, int)> baseOf = [&](const Operand& o, int depth) -> std::string {
            if (o.kind == OperandKind::GLOBAL) return o.name;
            if (o.isVreg() && restrictSlots.count(o.vregId)) return "&r" + std::to_string(o.vregId);
            if (!o.isVreg() || depth > 8 || slots.count(o.vregId) || fn.memoryVregs.count(o.vregId)) return "";
            auto n = nDefs.find(o.vregId);
            if (n == nDefs.end() || n->second != 1) return "";
            const Inst& d = *defOf[o.vregId];
            switch (d.op) {
                case Op::ADDR_GLOBAL: return d.src1.name;
                case Op::ADDR_LOCAL: return "#";
                case Op::DEREF:   // (a restrict parameter's value: its object)
                    return d.src1.isVreg() && restrictSlots.count(d.src1.vregId) ? baseOf(d.src1, depth + 1) : "";
                case Op::ADDR_ELEM: case Op::COPY: return baseOf(d.src1, depth + 1);
                case Op::ADD: case Op::SUB:
                    return d.resultType == Type::PTR ? baseOf(d.src1, depth + 1) : "";
                default: return "";
            }
        };
        // a store's effect on loads of globals: the global it writes ("#": none), "" any
        auto storeBase = [&](const Inst& in) -> std::string {
            if ((in.op == Op::STORE || in.op == Op::STORE_ZP) && in.src2.isVreg() && slots.count(in.src2.vregId) &&
                !fn.memoryVregs.count(in.src2.vregId))
                return "#";
            if (in.op == Op::STORE) return baseOf(in.src2, 0);
            return "";
        };
        // Extended blocks: a block whose one predecessor is the block before
        // it in the list starts with that block's expressions and copies
        // (an address computed before an if is reused in it: map[y][x]). Its
        // predecessors: the branches to its label (switch cases, phi edges),
        // the fall-through from the block before; a label whose address is
        // taken has unknown ones.
        std::map<std::string, size_t> labelIdx;
        for (size_t bi = 0; bi < fn.blocks.size(); bi++) labelIdx[fn.blocks[bi].label] = bi;
        std::vector<int> predCount(fn.blocks.size(), 0);
        std::vector<long> onlyPred(fn.blocks.size(), -1);
        std::vector<std::set<size_t>> predSet(fn.blocks.size());
        auto addPred = [&](size_t to, size_t from) {
            if (predCount[to]++ == 0) onlyPred[to] = (long)from;
            else if (onlyPred[to] != (long)from) onlyPred[to] = -2;
            if (from < fn.blocks.size()) predSet[to].insert(from);
        };
        for (size_t bi = 0; bi < fn.blocks.size(); bi++) {
            const auto& b = fn.blocks[bi];
            bool falls = true;
            for (const auto& in : b.insts) {
                auto target = [&](const Operand& o) {
                    if (o.kind == OperandKind::LABEL) {
                        auto it = labelIdx.find(o.name);
                        if (it != labelIdx.end()) addPred(it->second, bi);
                    }
                };
                if (in.op == Op::BR || in.op == Op::BR_COND || in.op == Op::SWITCH) {
                    target(in.dest); target(in.src1); target(in.src2);
                    for (const auto& a : in.args) target(a);
                }
                for (const auto& c : in.switchCases) {
                    auto it = labelIdx.find(c.second);
                    if (it != labelIdx.end()) { addPred(it->second, bi); addPred(it->second, bi + 100000); }
                }
            }
            if (!b.insts.empty()) {
                Op last = b.insts.back().op;
                if (last == Op::BR || last == Op::RET || last == Op::RET_VOID || last == Op::BR_INDIRECT ||
                    last == Op::SWITCH)
                    falls = false;
            }
            if (falls && bi + 1 < fn.blocks.size()) addPred(bi + 1, bi);
        }
        for (const auto& l : fn.addressTakenLabels) {
            auto it = labelIdx.find(l);
            if (it != labelIdx.end()) predCount[it->second] += 2;
        }
        // any other way into a block (asm naming labels): no carrying
        bool anyAsm = false;
        for (const auto& b : fn.blocks)
            for (const auto& in : b.insts)
                if (in.op == Op::ASM_INLINE || in.op == Op::BR_INDIRECT) anyAsm = true;
        // Dominators (Issue #294): a block starts with what its immediate
        // dominator computed, when nothing on the paths between them (nor
        // the block itself, in a loop) changes the operands. Blocks with
        // other ways in (an address-taken label, asm) start empty.
        const size_t nb = fn.blocks.size();
        std::vector<bool> unknownIn(nb, false);
        for (const auto& l : fn.addressTakenLabels) {
            auto it = labelIdx.find(l);
            if (it != labelIdx.end()) unknownIn[it->second] = true;
        }
        std::vector<std::vector<bool>> dom(nb, std::vector<bool>(nb, true));
        if (nb) { dom[0].assign(nb, false); dom[0][0] = true; }
        for (bool ch = true; ch; ) {
            ch = false;
            for (size_t b = 1; b < nb; b++) {
                std::vector<bool> d(nb, !predSet[b].empty() && !unknownIn[b]);
                for (size_t p : predSet[b]) for (size_t k = 0; k < nb; k++) d[k] = d[k] && dom[p][k];
                d[b] = true;
                if (d != dom[b]) { dom[b] = d; ch = true; }
            }
        }
        auto idomOf = [&](size_t b) -> long {
            if (unknownIn[b] || predSet[b].empty()) return -1;
            for (size_t d = 0; d < nb; d++) {
                if (d == b || !dom[b][d]) continue;
                bool imm = true;   // every other strict dominator dominates d
                for (size_t o = 0; o < nb && imm; o++)
                    if (o != b && o != d && dom[b][o] && !dom[d][o]) imm = false;
                if (imm) return (long)d;
            }
            return -1;
        };
        std::vector<std::vector<size_t>> succOf(nb);
        for (size_t b = 0; b < nb; b++) for (size_t p : predSet[b]) succOf[p].push_back(b);
        // what each block changes: the vregs it defines or stores, and
        // whether it writes memory (a store elsewhere, a call, asm)
        std::vector<std::set<uint32_t>> blockDefs(nb);
        std::vector<bool> blockMem(nb, false), blockMemPure(nb, false);
        // the globals each block stores into, or memory it may write anywhere
        std::vector<std::set<std::string>> blockStores(nb);
        std::vector<bool> blockAnyStore(nb, false);
        for (size_t b = 0; b < nb; b++)
            for (const auto& in : fn.blocks[b].insts) {
                if (!memoryNeutral(in.op) && !purityOf(mod, in)) {
                    std::string sb = storeBase(in);
                    if (sb.empty()) blockAnyStore[b] = true;
                    else if (sb != "#") blockStores[b].insert(sb);
                }
                if (in.dest.isVreg()) blockDefs[b].insert(in.dest.vregId);
                bool st = in.op == Op::STORE || in.op == Op::STORE_ZP;
                if (st && in.src2.isVreg() && slots.count(in.src2.vregId)) {
                    blockDefs[b].insert(in.src2.vregId);
                    if (fn.memoryVregs.count(in.src2.vregId)) blockMemPure[b] = true;
                } else if ((st || in.op == Op::CALL || in.op == Op::CALL_VOID || in.op == Op::CALL_INDIRECT ||
                            in.op == Op::ASM_INLINE || in.op == Op::CPU_REG_WRITE || in.op == Op::CPU_FLAG_WRITE) &&
                           !purityOf(mod, in)) {
                    blockMem[b] = true;
                    blockMemPure[b] = true;
                }
            }
        std::vector<std::map<uint32_t, Operand>> endCopies(fn.blocks.size());
        std::vector<std::vector<std::pair<ExprKey, Operand>>> endExprs(fn.blocks.size());
        // calls of pure / const functions and their results (Issue #76)
        struct PureCall { std::string name; std::vector<Operand> args; Type type; int purity; Operand result; };
        std::vector<std::vector<PureCall>> endCalls(fn.blocks.size());
        // (an argument that is a constant compares by its value: each call
        // gets its own const vreg)
        const std::map<uint32_t, int64_t> callConsts = singleDefConsts(fn);
        auto argKey = [&](const Operand& o) {
            if (o.isVreg()) {
                auto c = callConsts.find(o.vregId);
                if (c != callConsts.end()) return Operand::imm(c->second, o.type);
            }
            return o;
        };
        for (size_t blockIdx = 0; blockIdx < fn.blocks.size(); blockIdx++) {
            auto& block = fn.blocks[blockIdx];
            std::map<uint32_t, Operand> copies;
            std::vector<std::pair<ExprKey, Operand>> activeExprs;
            std::vector<PureCall> activeCalls;
            long dIdx = anyAsm || blockIdx == 0 ? -1 : idomOf(blockIdx);
            if (dIdx >= 0 && (size_t)dIdx < blockIdx) {
                const size_t D = (size_t)dIdx;
                // the blocks on a path D -> ... -> blockIdx (not through D
                // again), and blockIdx itself when it is in a loop without D
                std::vector<bool> fwd(nb, false), bwd(nb, false);
                std::vector<size_t> todo;
                for (size_t s2 : succOf[D]) if (s2 != D && !fwd[s2]) { fwd[s2] = true; todo.push_back(s2); }
                while (!todo.empty()) {
                    size_t x = todo.back(); todo.pop_back();
                    for (size_t s2 : succOf[x]) if (s2 != D && !fwd[s2]) { fwd[s2] = true; todo.push_back(s2); }
                }
                bwd[blockIdx] = true;
                todo.push_back(blockIdx);
                while (!todo.empty()) {
                    size_t x = todo.back(); todo.pop_back();
                    for (size_t p2 : predSet[x]) if (p2 != D && !bwd[p2]) { bwd[p2] = true; todo.push_back(p2); }
                }
                std::set<uint32_t> killed;
                bool mem = false, memPure = false, anyStore = false;
                std::set<std::string> stored;
                for (size_t x = 0; x < nb; x++) {
                    if (x == D || !fwd[x] || !bwd[x]) continue;
                    // (blockIdx itself only when a path from it comes back)
                    if (x == blockIdx) {
                        bool loops = false;
                        for (size_t p2 : predSet[blockIdx]) if (p2 != D && fwd[p2] && bwd[p2]) loops = true;
                        if (!loops) continue;
                    }
                    killed.insert(blockDefs[x].begin(), blockDefs[x].end());
                    mem = mem || blockMem[x];
                    memPure = memPure || blockMemPure[x];
                    anyStore = anyStore || blockAnyStore[x];
                    stored.insert(blockStores[x].begin(), blockStores[x].end());
                }
                auto alive = [&](const Operand& o) {
                    if (!o.isVreg()) return true;
                    if (killed.count(o.vregId)) return false;
                    // (a variable may change through a write; not a restrict parameter)
                    return !(mem && slots.count(o.vregId) && exposed(o.vregId));
                };
                for (const auto& c : endCalls[D]) {
                    bool ok = alive(c.result) && !(c.purity < 2 && memPure);
                    for (const auto& a : c.args) ok = ok && alive(a);
                    if (ok) activeCalls.push_back(c);   // (a call is expensive to repeat)
                }
                // only what is expensive to compute again (a multiply, an
                // element address, a division): a reused cheap value lives
                // longer and spills more than it saves
                for (const auto& e : endExprs[D]) {
                    Op o = e.first.op;
                    // (measured on vim2 and the library: carrying cheap ones
                    // too, or only from a direct predecessor, is larger)
                    if (!(o == Op::MUL || o == Op::MUL_U || o == Op::ADDR_ELEM || o == Op::DIV ||
                          o == Op::DIV_U || o == Op::MOD || o == Op::MOD_U || o == Op::LOAD))
                        continue;
                    // a load of a global: nothing on the way wrote it (#320)
                    if (o == Op::LOAD && (anyStore || stored.count(e.first.base))) continue;
                    if (alive(e.first.src1) && alive(e.first.src2) && alive(e.second)) activeExprs.push_back(e);
                }
            }

            // Vregs are not SSA (a variable's slot vreg is assigned by STORE, a
            // temp can be defined on several paths): forget every recorded
            // expression and copy that reads or names vreg v once v changes
            auto uses = [](const Operand& o, uint32_t v) { return o.isVreg() && o.vregId == v; };
            auto invalidate = [&](uint32_t v) {
                activeExprs.erase(std::remove_if(activeExprs.begin(), activeExprs.end(),
                    [&](const auto& e) { return uses(e.first.src1, v) || uses(e.first.src2, v) || uses(e.second, v); }),
                    activeExprs.end());
                for (auto it = copies.begin(); it != copies.end();) {
                    if (it->first == v || uses(it->second, v)) it = copies.erase(it);
                    else ++it;
                }
                activeCalls.erase(std::remove_if(activeCalls.begin(), activeCalls.end(), [&](const PureCall& c) {
                    if (uses(c.result, v)) return true;
                    for (const auto& a : c.args) if (uses(a, v)) return true;
                    return false;
                }), activeCalls.end());
            };
            // A store through a pointer, a call or inline asm may change a
            // variable whose address is taken (exposedSlots)
            auto invalidateVariables = [&]() {
                for (uint32_t v : slots) if (exposed(v)) invalidate(v);
            };

            for (size_t instIdx = 0; instIdx < block.insts.size(); instIdx++) {
                auto& inst = block.insts[instIdx];
                // 1. Apply copy propagation to operands. A variable's slot vreg
                // as the address of a LOAD/STORE (or ADDR_LOCAL's operand) names
                // the variable itself, so a copy of it is not propagated there
                bool src1IsAddr = inst.op == Op::LOAD || inst.op == Op::LOAD_ZP || inst.op == Op::ADDR_LOCAL;
                bool src2IsAddr = inst.op == Op::STORE || inst.op == Op::STORE_ZP;
                auto prop = [&](Operand& op, bool addrRole) {
                    Operand before = op;
                    propagateOperand(op, copies);
                    if (addrRole && op.isVreg() && slots.count(op.vregId)) op = before;
                };
                prop(inst.src1, src1IsAddr);
                prop(inst.src2, src2IsAddr);
                for (auto& arg : inst.args) prop(arg, false);
                for (auto& pair : inst.phiIncoming) prop(pair.first, false);

                // 2. Handle memory stores invalidating load expressions (a
                // call of a pure / const function changes nothing)
                const int purity = purityOf(mod, inst);
                if ((inst.op == Op::STORE || inst.op == Op::STORE_ZP ||
                     inst.op == Op::CALL || inst.op == Op::CALL_INDIRECT || inst.op == Op::CALL_VOID ||
                     inst.op == Op::ASM_INLINE || inst.op == Op::CPU_REG_WRITE || inst.op == Op::CPU_FLAG_WRITE) &&
                    !purity) {
                    // (a pure function's result depends on memory too; not on
                    // a local variable whose address is not taken)
                    bool plainLocal = (inst.op == Op::STORE || inst.op == Op::STORE_ZP) && inst.src2.isVreg() &&
                                      slots.count(inst.src2.vregId) && !fn.memoryVregs.count(inst.src2.vregId);
                    if (!plainLocal)
                        activeCalls.erase(std::remove_if(activeCalls.begin(), activeCalls.end(),
                            [](const PureCall& c) { return c.purity < 2; }), activeCalls.end());
                    
                }
                // loads of globals: a store into one forgets those of that
                // global only (#320), a store into a local none; anything
                // else that may write memory forgets them all
                if (!memoryNeutral(inst.op) && !purity) {
                    std::string sb = storeBase(inst);
                    if (sb != "#")
                        activeExprs.erase(std::remove_if(activeExprs.begin(), activeExprs.end(), [&](const auto& pair) {
                            return pair.first.op == Op::LOAD && (sb.empty() || pair.first.base == sb);
                        }), activeExprs.end());
                }
                if ((inst.op == Op::STORE || inst.op == Op::STORE_ZP ||
                     inst.op == Op::CALL || inst.op == Op::CALL_INDIRECT || inst.op == Op::CALL_VOID ||
                     inst.op == Op::ASM_INLINE || inst.op == Op::CPU_REG_WRITE || inst.op == Op::CPU_FLAG_WRITE) &&
                    !purity) {
                    // A STORE to a slot vreg assigns that variable
                    if ((inst.op == Op::STORE || inst.op == Op::STORE_ZP) && inst.src2.isVreg() &&
                        slots.count(inst.src2.vregId))
                        invalidate(inst.src2.vregId);
                    else
                        invalidateVariables();
                }

                // The instruction (re)defines its destination
                if (inst.dest.isVreg()) invalidate(inst.dest.vregId);

                // A call of a pure / const function with the arguments of an
                // earlier one (nothing changed in between): its result
                if (purity && inst.op == Op::CALL && inst.dest.isVreg()) {
                    bool found = false;
                    for (const auto& c : activeCalls) {
                        if (c.name != inst.src1.name || c.type != inst.resultType || c.args.size() != inst.args.size()) continue;
                        bool same = true;
                        for (size_t a = 0; a < c.args.size() && same; a++) same = sameOperand(c.args[a], argKey(inst.args[a]));
                        if (!same) continue;
                        trace("cse", fn.name, "v" + std::to_string(inst.dest.vregId) + " reuses the result of a call of " +
                              inst.src1.name + " (v" + std::to_string(c.result.vregId) + ")", inst.loc);
                        Operand result = c.result;
                        uint32_t d = inst.dest.vregId;
                        Type t = inst.dest.type;
                        SourceLoc where = inst.loc;
                        inst = Inst();
                        inst.op = Op::COPY;
                        inst.dest = Operand::vreg(d, t);
                        inst.src1 = result;
                        inst.resultType = t;
                        inst.loc = where;
                        if (!slots.count(d)) copies[d] = result;
                        found = true;
                        break;
                    }
                    if (found) continue;
                    if (!slots.count(inst.dest.vregId)) {
                        bool self = false;
                        for (const auto& a : inst.args) self = self || uses(a, inst.dest.vregId);
                        std::vector<Operand> keys;
                        for (const auto& a : inst.args) keys.push_back(argKey(a));
                        if (!self) activeCalls.push_back({inst.src1.name, keys, inst.resultType, purity, inst.dest});
                    }
                    continue;
                }

                // 3. Record new copies directly
                if (inst.op == Op::COPY) {
                    if (inst.dest.isVreg() && !slots.count(inst.dest.vregId) &&
                        !(inst.src1.isVreg() && inst.src1.vregId == inst.dest.vregId))
                        copies[inst.dest.vregId] = inst.src1;
                    continue;
                }

                // 4. Try to match expression for CSE (a load: from a global
                // not declared volatile, #320)
                std::string loadBase;
                if (inst.op == Op::LOAD && inst.dest.isVreg() && !slots.count(inst.dest.vregId) &&
                    !fn.memoryVregs.count(inst.dest.vregId) && inst.args.empty()) {
                    loadBase = baseOf(inst.src1, 0);
                    if (loadBase == "#" || isVolatileGlobal(loadBase)) loadBase.clear();
                    // a read-modify-write (x--, x += k: the same address stored
                    // later in the block) loads afresh, so it can update in place
                    // (a variable's slot as a store's target is the variable,
                    // not the memory it points to: p++ after *p)
                    bool slotAddr = inst.src1.isVreg() && slots.count(inst.src1.vregId);
                    for (size_t k = instIdx + 1; k < block.insts.size() && !loadBase.empty() && !slotAddr; k++) {
                        const Inst& later = block.insts[k];
                        if ((later.op == Op::STORE || later.op == Op::STORE_ZP) && sameOperand(later.src2, inst.src1) &&
                            later.elemOffset == inst.elemOffset)
                            loadBase.clear();
                    }
                }
                if ((isCSECandidate(inst.op) || !loadBase.empty()) && inst.dest.isVreg()) {
                    ExprKey key;
                    key.op = inst.op;
                    // (a constant operand by its value: each use has its own
                    // const vreg)
                    key.src1 = argKey(inst.src1);
                    key.src2 = argKey(inst.src2);
                    key.type = inst.resultType;
                    key.byteWidth = inst.resultByteWidth;
                    key.elemOffset = inst.elemOffset;
                    key.base = loadBase;
                    // (through a copy of a restrict parameter: the parameter's
                    // value, as every other copy of it; "*" tells it from a
                    // load of the variable itself)
                    if (inst.op == Op::LOAD && loadBase.rfind("&r", 0) == 0 && inst.src1.isVreg()) {
                        auto n = nDefs.find(inst.src1.vregId);
                        const Inst* d = n != nDefs.end() && n->second == 1 ? defOf[inst.src1.vregId] : nullptr;
                        if (d && d->op == Op::COPY && d->src1.isVreg() && restrictSlots.count(d->src1.vregId)) {
                            key.src1 = d->src1;
                            key.name = "*";
                        }
                    }
                    
                    // If call or memory-indexed, carry label or symbol info
                    if (inst.op == Op::ADDR_GLOBAL || inst.op == Op::ADDR_LABEL) {
                        key.name = inst.src1.name;
                    }

                    // Check if already computed in this block
                    bool found = false;
                    for (const auto& pair : activeExprs) {
                        if (pair.first == key) {
                            // Match found! Replace instruction with COPY
                            trace("cse", fn.name, "v" + std::to_string(inst.dest.vregId) + " reuses v" + std::to_string(pair.second.vregId), inst.loc);
                            inst.op = Op::COPY;
                            inst.src1 = pair.second;
                            inst.src2 = Operand::none();
                            if (!slots.count(inst.dest.vregId)) copies[inst.dest.vregId] = pair.second;
                            // std::printf("    Matched CSE! Replace with copy of %d\n", pair.second.vregId);
                            found = true;
                            break;
                        }
                    }

                    // A slot vreg's value changes with each assignment: only a
                    // temp can stand for the expression
                    if (!found && !slots.count(inst.dest.vregId) &&
                        !uses(inst.src1, inst.dest.vregId) && !uses(inst.src2, inst.dest.vregId)) {
                        activeExprs.push_back({key, inst.dest});
                    }
                }
            }
            endCopies[blockIdx] = std::move(copies);
            endExprs[blockIdx] = std::move(activeExprs);
            endCalls[blockIdx] = std::move(activeCalls);
        }
    }
}

// ============================================================================
// Parameter Narrowing
// An int or long parameter of a static function every call of which passes
// a value that fits in fewer bytes (0-255, 0-65535): the parameter gets that
// type, each call truncates its argument to it, and each use in the function
// zero-extends it again. Only for parameters the function never assigns or
// takes the address of, and functions called only directly in this file;
// only an int parameter whose every use costs nothing zero-extended (byte
// uses, 16-bit add/sub/and/xor and compares, which IRCodeGen computes from
// the byte where it lives), so the function does not grow.
// ============================================================================

// The bytes (1, 2, 4) an unsigned value is known to fit in; 0 if unknown
static int fitBytes(const Operand& o, const std::map<uint32_t, const Inst*>& defs, int depth = 0) {
    auto ofValue = [](int64_t v) { return v < 0 ? 0 : v <= 0xFF ? 1 : v <= 0xFFFF ? 2 : v <= 0xFFFFFFFFLL ? 4 : 0; };
    if (o.isImm()) return ofValue(o.immVal);
    if (!o.isVreg() || depth > 8) return 0;
    auto it = defs.find(o.vregId);
    if (it == defs.end() || !it->second) return 0;
    const Inst& in = *it->second;
    switch (in.op) {
        case Op::CONST: return ofValue(in.src1.immVal);
        case Op::ZEXT: {
            int s = typeSize(in.src1.type);
            return s == 1 || s == 2 ? s : 0;
        }
        case Op::COPY: return typeSize(in.resultType) < typeSize(in.src1.type) ? 0 : fitBytes(in.src1, defs, depth + 1);
        case Op::AND: {
            int a = fitBytes(in.src1, defs, depth + 1), b = fitBytes(in.src2, defs, depth + 1);
            return a && b ? std::min(a, b) : std::max(a, b);
        }
        case Op::MOD_U: {
            int b = fitBytes(in.src2, defs, depth + 1);
            int64_t d = in.src2.isImm() ? in.src2.immVal : -1;
            if (in.src2.isVreg()) {
                auto dt = defs.find(in.src2.vregId);
                if (dt != defs.end() && dt->second && dt->second->op == Op::CONST) d = dt->second->src1.immVal;
            }
            return d == 256 ? 1 : d == 65536 ? 2 : b;
        }
        default: return 0;
    }
}

static bool namesSymbol(const std::string& text, const std::string& sym) {
    auto ident = [](char c) { return isalnum((unsigned char)c) || c == '_'; };
    for (size_t p = text.find(sym); p != std::string::npos; p = text.find(sym, p + 1))
        if ((p == 0 || !ident(text[p - 1])) && (p + sym.size() >= text.size() || !ident(text[p + sym.size()])))
            return true;
    return false;
}

void optimizeParamNarrowing(Module& mod) {
    // (a nested function reaches its parent's parameters from its own frame)
    for (const auto& fn : mod.functions) if (fn.isNested || fn.staticLinkVreg != -1) return;

    // Functions whose address is taken (code, data, asm) may be called with anything
    std::set<std::string> addressTaken;
    auto mentions = [&](const std::string& text) {
        for (const auto& fn : mod.functions) if (namesSymbol(text, fn.name)) addressTaken.insert(fn.name);
    };
    for (const auto& s : mod.fileAsm) mentions(s);
    for (const auto& s : mod.asmRefs) mentions(s);
    for (const auto& g : mod.globals) {
        for (const auto& l : g.initLabels) mentions(l);
        for (const auto& l : g.vtableMethodNames) mentions(l);
        for (const auto& w : g.initWords) mentions(w.second);
    }
    for (const auto& fn : mod.functions)
        for (const auto& blk : fn.blocks)
            for (const auto& in : blk.insts) {
                bool direct = in.op == Op::CALL || in.op == Op::CALL_VOID;
                if (!direct && in.src1.kind == OperandKind::GLOBAL) addressTaken.insert(in.src1.name);
                for (const Operand* o : {&in.src2, &in.dest})
                    if (o->kind == OperandKind::GLOBAL) addressTaken.insert(o->name);
                for (const auto& a : in.args) if (a.kind == OperandKind::GLOBAL) addressTaken.insert(a.name);
                if (in.op == Op::ASM_INLINE) mentions(in.asmText);
            }

    // Narrowing a function makes the arguments of its own calls zero-extended
    // values: again until nothing changes
    for (int round = 0; round < 8; round++) {
        bool changed = false;
        for (auto& fn : mod.functions) {
            if (!fn.isStatic || fn.blocks.empty() || fn.isVariadic || fn.isInterrupt || fn.isNaked ||
                fn.isRegparm || fn.isFar || addressTaken.count(fn.name) || !runOn(fn, "ParamNarrowing"))
                continue;
            const size_t np = fn.paramTypes.size();
            std::vector<int> want(np, 0);   // bytes every call's argument fits in (0: not narrowable)
            for (size_t p = 0; p < np; p++)
                if (fn.paramTypes[p] == Type::I16 || fn.paramTypes[p] == Type::I32) want[p] = 1;

            // the parameter is only read, in plain instructions
            for (const auto& blk : fn.blocks)
                for (const auto& in : blk.insts) {
                    if (in.op == Op::ASM_INLINE || in.op == Op::VA_START) std::fill(want.begin(), want.end(), 0);
                    auto kill = [&](const Operand& o) { if (o.isVreg() && o.vregId < np) want[o.vregId] = 0; };
                    kill(in.dest);
                    if (in.op == Op::STORE) kill(in.src2);
                    if (in.op == Op::ADDR_LOCAL || in.op == Op::ADDR_UPLEVEL || in.op == Op::DEREF ||
                        in.op == Op::BFINS || in.op == Op::PHI) { kill(in.src1); kill(in.src2); }
                    for (const auto& ph : in.phiIncoming) kill(ph.first);
                    // each use must cost nothing zero-extended: a byte use, or
                    // a 16-bit add/sub, and/xor or compare (which read the byte
                    // where it lives and 0 for the high byte)
                    for (size_t p = 0; p < np; p++) {
                        int n = 0;
                        for (const Operand* o : {&in.src1, &in.src2}) if (o->isVreg() && o->vregId == p) n++;
                        for (const auto& a : in.args) if (a.isVreg() && a.vregId == p) n += 2;
                        if (n == 0 || !want[p]) continue;
                        bool cheap = false;
                        if (n == 1 && fn.paramTypes[p] == Type::I16 && in.args.empty()) switch (in.op) {
                            case Op::TRUNC:
                                cheap = in.resultType == Type::I8; break;
                            case Op::ADD: case Op::SUB: case Op::AND: case Op::XOR:
                                cheap = in.resultType == Type::I16 || in.resultType == Type::PTR; break;
                            case Op::CMP_EQ: case Op::CMP_NE: case Op::CMP_LT: case Op::CMP_GE:
                            case Op::CMP_LTU: case Op::CMP_LEU: case Op::CMP_GTU: case Op::CMP_GEU:
                                cheap = true; break;
                            default: break;
                        }
                        if (!cheap) want[p] = 0;
                    }
                }
            for (size_t p = 0; p < np; p++)
                if (fn.memoryVregs.count(p) || fn.registerVregs.count(p) || fn.registerXVregs.count(p) ||
                    fn.registerYVregs.count(p) || fn.registerZVregs.count(p) || fn.retAddrSlot == (int)p)
                    want[p] = 0;
            if (std::none_of(want.begin(), want.end(), [](int w) { return w != 0; })) continue;

            // every call passes a value that fits
            int calls = 0;
            for (const auto& caller : mod.functions) {
                std::map<uint32_t, const Inst*> defs;   // single definitions (nullptr: several)
                for (const auto& blk : caller.blocks)
                    for (const auto& in : blk.insts)
                        if (in.dest.isVreg()) {
                            auto [it, fresh] = defs.emplace(in.dest.vregId, &in);
                            if (!fresh) it->second = nullptr;
                        }
                for (uint32_t s : caller.localSlotVregs) defs[s] = nullptr;
                for (uint32_t p = 0; p < caller.paramTypes.size(); p++) defs[p] = nullptr;
                for (const auto& blk : caller.blocks)
                    for (const auto& in : blk.insts) {
                        if (!(in.op == Op::CALL || in.op == Op::CALL_VOID) || in.src1.kind != OperandKind::GLOBAL ||
                            in.src1.name != fn.name)
                            continue;
                        calls++;
                        if (in.args.size() != np) { std::fill(want.begin(), want.end(), 0); continue; }
                        for (size_t p = 0; p < np; p++) {
                            if (!want[p]) continue;
                            int b = fitBytes(in.args[p], defs);
                            if (b == 0 || b >= typeSize(fn.paramTypes[p])) want[p] = 0;
                            else want[p] = std::max(want[p], b);
                        }
                    }
            }
            if (calls == 0) continue;

            for (uint32_t p = 0; p < np; p++) {
                if (!want[p]) continue;
                Type wide = fn.paramTypes[p];
                Type narrow = want[p] == 1 ? Type::I8 : Type::I16;
                std::string pName = p < fn.paramNames.size() && !fn.paramNames[p].empty() ? fn.paramNames[p] : std::to_string(p);
                trace("param", fn.name, "parameter " + pName + " narrowed to " + (narrow == Type::I8 ? "i8" : "i16") +
                      " (every call passes a value that fits)");

                // the function: each use reads the zero-extended parameter
                fn.paramTypes[p] = narrow;
                fn.vregTypes[p] = narrow;
                if (fn.vregSizes.count(p)) fn.vregSizes[p] = typeSize(narrow);
                for (auto& blk : fn.blocks)
                    for (size_t i = 0; i < blk.insts.size(); i++) {
                        std::vector<Operand*> ops;
                        Inst& in = blk.insts[i];
                        for (Operand* o : {&in.src1, &in.src2}) if (o->isVreg() && o->vregId == p) ops.push_back(o);
                        for (auto& a : in.args) if (a.isVreg() && a.vregId == p) ops.push_back(&a);
                        if (ops.empty()) continue;
                        uint32_t t = fn.allocVreg();
                        fn.vregTypes[t] = wide;
                        for (Operand* o : ops) *o = Operand::vreg(t, wide);
                        Inst ext;
                        ext.op = Op::ZEXT;
                        ext.resultType = wide;
                        ext.dest = Operand::vreg(t, wide);
                        ext.src1 = Operand::vreg(p, narrow);
                        ext.loc = in.loc;
                        blk.insts.insert(blk.insts.begin() + i, ext);
                        i++;
                    }

                // the calls: the argument truncated to the parameter's type (a
                // constant: the constant of that type)
                for (auto& caller : mod.functions) {
                    std::map<uint32_t, int64_t> consts;   // single CONST definitions
                    std::map<uint32_t, int> ndefs;
                    for (const auto& blk : caller.blocks)
                        for (const auto& in : blk.insts)
                            if (in.dest.isVreg()) {
                                ndefs[in.dest.vregId]++;
                                if (in.op == Op::CONST) consts[in.dest.vregId] = in.src1.immVal;
                            }
                    for (auto& blk : caller.blocks)
                        for (size_t i = 0; i < blk.insts.size(); i++) {
                            Inst& in = blk.insts[i];
                            if (!(in.op == Op::CALL || in.op == Op::CALL_VOID) || in.src1.kind != OperandKind::GLOBAL ||
                                in.src1.name != fn.name)
                                continue;
                            Operand arg = in.args[p];
                            if (arg.isVreg() && consts.count(arg.vregId) && ndefs[arg.vregId] == 1 &&
                                !caller.localSlotVregs.count(arg.vregId) && arg.vregId >= caller.paramTypes.size())
                                arg = Operand::imm(consts[arg.vregId], wide);
                            uint32_t t = caller.allocVreg();
                            caller.vregTypes[t] = narrow;
                            Inst cv;
                            cv.dest = Operand::vreg(t, narrow);
                            cv.resultType = narrow;
                            cv.loc = in.loc;
                            if (arg.isImm()) {
                                cv.op = Op::CONST;
                                cv.src1 = Operand::imm(arg.immVal, narrow);
                            } else {
                                cv.op = Op::TRUNC;
                                cv.src1 = arg;
                                cv.src1.type = wide;
                            }
                            in.args[p] = Operand::vreg(t, narrow);
                            blk.insts.insert(blk.insts.begin() + i, cv);
                            i++;
                        }
                }
                changed = true;
            }
        }
        if (!changed) break;
    }
}

// ============================================================================
// Strength Reduction
// A commutative operation (+ * & | ^) with a constant first operand and a
// non-constant second one gets its operands swapped, so the simplifications
// that look at the second operand see 0 ^ x, 1 * x, 4 * x as x ^ 0 ...
static void constantSecond(Inst& inst, const std::map<uint32_t, int64_t>& constVals) {
    switch (inst.op) {
        case Op::ADD: case Op::MUL: case Op::MUL_U: case Op::AND: case Op::OR: case Op::XOR: break;
        default: return;
    }
    auto isConst = [&](const Operand& o) {
        return o.isImm() || (o.isVreg() && constVals.count(o.vregId));
    };
    if (isConst(inst.src1) && !isConst(inst.src2) && inst.src2.kind != OperandKind::NONE)
        std::swap(inst.src1, inst.src2);
}

// ============================================================================

void optimizeStrengthReduction(Module& mod) {
    for (auto& fn : mod.functions) {
        if (!runOn(fn, "StrengthReduction")) continue;
        // Build constant value map: vreg → immediate value
        // (for looking up CONST operands of MUL/DIV/MOD)
        std::map<uint32_t, int64_t> constVals = singleDefConsts(fn);

        // Signed x / 2^k and x % 2^k (C rounds toward zero): a negative x
        // gets the bias 2^k - 1 first, taken from its sign by an arithmetic
        // shift:  b = (x >>a (w-1)) & (2^k - 1);  x / 2^k = (x + b) >>a k;
        // x % 2^k = x - ((x + b) & -2^k). No call of the division routine.
        for (auto& block : fn.blocks) {
            std::vector<Inst> out;
            out.reserve(block.insts.size());
            for (auto& inst : block.insts) {
                int64_t val = 0;
                bool cand = (inst.op == Op::DIV || inst.op == Op::MOD) && inst.src2.isVreg() &&
                            (inst.resultType == Type::I16 || inst.resultType == Type::I32) && inst.dest.isVreg();
                if (cand) {
                    auto it = constVals.find(inst.src2.vregId);
                    cand = it != constVals.end();
                    if (cand) val = it->second;
                }
                int w = inst.resultType == Type::I32 ? 32 : 16;
                int k = 0;
                if (cand && val >= 2 && (val & (val - 1)) == 0) {
                    while ((int64_t(1) << k) < val) k++;
                    cand = k < w - 1;
                } else cand = false;
                if (!cand) { out.push_back(inst); continue; }
                Type t = inst.resultType;
                int64_t mask = val - 1;
                auto mk = [&](Op op, const Operand& a, const Operand& b) {
                    Inst n;
                    n.op = op;
                    n.resultType = t;
                    n.dest = Operand::vreg(fn.allocVreg(), t);
                    fn.vregTypes[n.dest.vregId] = t;
                    n.src1 = a;
                    n.src2 = b;
                    n.loc = inst.loc;
                    out.push_back(n);
                    return n.dest;
                };
                Operand x = inst.src1;
                Operand sgn = mk(Op::ASR, x, Operand::imm(w - 1, Type::I8));
                Operand bias = mk(Op::AND, sgn, Operand::imm(mask, t));
                Operand xb = mk(Op::ADD, x, bias);
                Inst fin = inst;
                if (inst.op == Op::DIV) {
                    trace("strength", fn.name, "v" + std::to_string(inst.dest.vregId) + " = DIV /" +
                          std::to_string(val) + " → bias, ASR " + std::to_string(k), inst.loc);
                    fin.op = Op::ASR;
                    fin.src1 = xb;
                    fin.src2 = Operand::imm(k, Type::I8);
                } else {
                    trace("strength", fn.name, "v" + std::to_string(inst.dest.vregId) + " = MOD %" +
                          std::to_string(val) + " → x - ((x + bias) & -" + std::to_string(val) + ")", inst.loc);
                    int64_t hiMask = (w == 32 ? 0xFFFFFFFFLL : 0xFFFFLL) & ~mask;
                    Operand down = mk(Op::AND, xb, Operand::imm(hiMask, t));
                    fin.op = Op::SUB;
                    fin.src1 = x;
                    fin.src2 = down;
                }
                out.push_back(fin);
            }
            block.insts = std::move(out);
        }

        // Rewrite instructions
        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                // x / x → 1, x % x → 0 (when both operands are the same vreg)
                if (inst.src1.isVreg() && inst.src2.isVreg() &&
                    inst.src1.vregId == inst.src2.vregId) {
                    if (inst.op == Op::DIV || inst.op == Op::DIV_U) {
                        trace("strength", fn.name, "v" + std::to_string(inst.dest.vregId) + " = v/v → 1", inst.loc);
                        inst.op = Op::CONST;
                        inst.src1 = Operand::imm(1, inst.resultType);
                        inst.src2 = Operand::none();
                        continue;
                    }
                    if (inst.op == Op::MOD || inst.op == Op::MOD_U) {
                        trace("strength", fn.name, "v" + std::to_string(inst.dest.vregId) + " = v%v → 0", inst.loc);
                        inst.op = Op::CONST;
                        inst.src1 = Operand::imm(0, inst.resultType);
                        inst.src2 = Operand::none();
                        continue;
                    }
                }

                // A constant first operand of a commutative op goes second
                constantSecond(inst, constVals);

                // Only handle ops whose src2 is a constant (immediate, or a vreg
                // with one constant definition)
                int64_t val;
                if (inst.src2.isImm()) {
                    val = inst.src2.immVal;
                } else {
                    if (!inst.src2.isVreg()) continue;
                    auto it = constVals.find(inst.src2.vregId);
                    if (it == constVals.end()) continue;
                    val = it->second;
                }

                auto vd = [&]() { return "v" + std::to_string(inst.dest.vregId); };
                switch (inst.op) {
                    case Op::MUL:
                    case Op::MUL_U: {
                        if (val == 0) {
                            trace("strength", fn.name, vd() + " = MUL *0 → CONST 0", inst.loc);
                            inst.op = Op::CONST;
                            inst.src1 = Operand::imm(0, inst.resultType);
                            inst.src2 = Operand::none();
                        } else if (val == 1) {
                            trace("strength", fn.name, vd() + " = MUL *1 → COPY", inst.loc);
                            inst.op = Op::COPY;
                            inst.src2 = Operand::none();
                        } else if (val > 0 && (val & (val - 1)) == 0) {
                            int shift = 0;
                            int64_t v = val;
                            while (v > 1) { v >>= 1; shift++; }
                            trace("strength", fn.name, vd() + " = MUL *" + std::to_string(val) + " → SHL " + std::to_string(shift), inst.loc);
                            inst.op = Op::SHL;
                            inst.src2 = Operand::imm(shift, inst.resultType);
                        }
                        break;
                    }
                    case Op::DIV:
                    case Op::DIV_U: {
                        if (val == 1) {
                            trace("strength", fn.name, vd() + " = DIV /1 → COPY", inst.loc);
                            inst.op = Op::COPY;
                            inst.src2 = Operand::none();
                        } else if (inst.op == Op::DIV_U && val > 0 && (val & (val - 1)) == 0) {
                            int shift = 0;
                            int64_t v = val;
                            while (v > 1) { v >>= 1; shift++; }
                            trace("strength", fn.name, vd() + " = DIV_U /" + std::to_string(val) + " → SHR " + std::to_string(shift), inst.loc);
                            inst.op = Op::SHR;
                            inst.src2 = Operand::imm(shift, inst.resultType);
                        }
                        break;
                    }
                    case Op::MOD:
                    case Op::MOD_U: {
                        if (val == 1) {
                            trace("strength", fn.name, vd() + " = MOD %1 → CONST 0", inst.loc);
                            inst.op = Op::CONST;
                            inst.src1 = Operand::imm(0, inst.resultType);
                            inst.src2 = Operand::none();
                        } else if (inst.op == Op::MOD_U && val > 0 && (val & (val - 1)) == 0) {
                            trace("strength", fn.name, vd() + " = MOD_U %" + std::to_string(val) + " → AND " + std::to_string(val - 1), inst.loc);
                            inst.op = Op::AND;
                            inst.src2 = Operand::imm(val - 1, inst.resultType);
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }
}

// ============================================================================
// Algebraic Simplification
// ============================================================================


void optimizeAlgebraic(Module& mod) {
    for (auto& fn : mod.functions) {
        if (!runOn(fn, "AlgebraicSimplify")) continue;
        // Build constant value map
        std::map<uint32_t, int64_t> constVals = singleDefConsts(fn);
        // (vregs are not SSA: only a single definition becomes a constant)
        std::map<uint32_t, int> defCount;
        for (const auto& b : fn.blocks)
            for (const auto& i : b.insts) {
                if (i.dest.isVreg()) defCount[i.dest.vregId]++;
                if ((i.op == Op::STORE || i.op == Op::STORE_ZP) && i.src2.isVreg()) defCount[i.src2.vregId] += 2;
            }

        // An element address with a constant index (after loop unrolling,
        // for example) on a global or pointer base: base + index * stride,
        // which the codegen folds into an absolute address. (A local array's
        // slot as the base means the variable's address: left alone.)
        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                if (inst.op != Op::ADDR_ELEM || inst.args.empty() || !inst.args[0].isImm()) continue;
                int64_t idx;
                if (inst.src2.isImm()) idx = inst.src2.immVal;
                else if (inst.src2.isVreg() && constVals.count(inst.src2.vregId)) idx = constVals[inst.src2.vregId];
                else continue;
                bool baseOk = inst.src1.kind == OperandKind::GLOBAL ||
                    (inst.src1.isVreg() && !fn.localSlotVregs.count(inst.src1.vregId) &&
                     !fn.memoryVregs.count(inst.src1.vregId));
                if (!baseOk) continue;
                int64_t off = idx * inst.args[0].immVal;
                trace("algebraic", fn.name, "v" + std::to_string(inst.dest.vregId) +
                      " = ADDR_ELEM [const " + std::to_string(idx) + "] → ADD +" + std::to_string(off), inst.loc);
                inst.op = Op::ADD;
                inst.resultType = Type::PTR;
                inst.src2 = Operand::imm(off & 0xFFFF, Type::I16);
                inst.args.clear();
            }
        }

        for (auto& block : fn.blocks)
            for (auto& inst : block.insts) constantSecond(inst, constVals);

        // (x + c1) + c2 as x + (c1 + c2), likewise with subtracts (#351:
        // pl.items[7] is @_pl + 11 + 7, one address @_pl+18; #353: x - 25 +
        // 23 is x - 2). Modular arithmetic: the same at any width, the
        // constant wrapped to the type. Only when the inner operation is
        // defined once and its own operand (x) is a global, a constant or a
        // value defined once: a local's slot could change between the two.
        {
            std::map<uint32_t, Inst*> def1;
            for (auto& b : fn.blocks)
                for (auto& in : b.insts)
                    if (in.dest.isVreg() && defCount[in.dest.vregId] == 1 && !fn.localSlotVregs.count(in.dest.vregId) &&
                        !fn.memoryVregs.count(in.dest.vregId))
                        def1[in.dest.vregId] = &in;
            auto stable = [&](const Operand& o) {
                return o.kind == OperandKind::GLOBAL || o.isImm() ||
                       (o.isVreg() && defCount[o.vregId] == 1 && !fn.localSlotVregs.count(o.vregId) &&
                        !fn.memoryVregs.count(o.vregId) && o.vregId >= fn.paramTypes.size());
            };
            // an add / sub of a constant: its operand and the signed constant
            auto split = [&](const Inst& in, Operand& x, int64_t& c) {
                if (in.op == Op::ADD && in.src2.isImm()) { x = in.src1; c = in.src2.immVal; return true; }
                if (in.op == Op::ADD && in.src1.isImm()) { x = in.src2; c = in.src1.immVal; return true; }
                if (in.op == Op::SUB && in.src2.isImm()) { x = in.src1; c = -in.src2.immVal; return true; }
                return false;
            };
            auto width = [](Type t) { return t == Type::I8 ? 8 : t == Type::I32 ? 32 : t == Type::I16 || t == Type::PTR ? 16 : 0; };
            for (auto& b : fn.blocks)
                for (auto& in : b.insts) {
                    Operand x, y;
                    int64_t c2, c1;
                    if (!split(in, x, c2) || !x.isVreg() || !in.args.empty()) continue;
                    auto d = def1.find(x.vregId);
                    if (d == def1.end() || !d->second->args.empty() || !split(*d->second, y, c1) || !stable(y)) continue;
                    int w = width(in.resultType);
                    if (!w || width(d->second->resultType) != w) continue;
                    if (y.isVreg() && width(y.type) && width(y.type) != w) continue;   // (no width change between)
                    uint64_t m = w == 32 ? 0xFFFFFFFFull : ((1ull << w) - 1);
                    int64_t c = c1 + c2;
                    trace("algebraic", fn.name, "v" + std::to_string(in.dest.vregId) + " = (x " +
                          (c1 < 0 ? "- " + std::to_string(-c1) : "+ " + std::to_string(c1)) + ") " +
                          (c2 < 0 ? "- " + std::to_string(-c2) : "+ " + std::to_string(c2)) + " → x " +
                          (c < 0 ? "- " + std::to_string(-c) : "+ " + std::to_string(c)), in.loc);
                    Type ct = in.resultType == Type::PTR ? Type::I16 : in.resultType;
                    in.src1 = y;
                    if (c < 0) { in.op = Op::SUB; in.src2 = Operand::imm((int64_t)((uint64_t)(-c) & m), ct); }
                    else { in.op = Op::ADD; in.src2 = Operand::imm((int64_t)((uint64_t)c & m), ct); }
                }
        }

        // Helper: get constant value of an operand, if known
        auto getConst = [&](const Operand& op, int64_t& out) -> bool {
            if (op.isImm()) { out = op.immVal; return true; }
            if (op.isVreg()) {
                auto it = constVals.find(op.vregId);
                if (it != constVals.end()) { out = it->second; return true; }
            }
            return false;
        };

        // Full-width mask for a given type
        auto fullMask = [](Type t) -> int64_t {
            switch (t) {
                case Type::I8:  return 0xFF;
                case Type::I16: case Type::PTR: return 0xFFFF;
                case Type::I32: return 0xFFFFFFFF;
                default: return 0xFFFF;
            }
        };

        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                // Same-operand patterns (x op x)
                if (inst.src1.isVreg() && inst.src2.isVreg() &&
                    inst.src1.vregId == inst.src2.vregId) {
                    switch (inst.op) {
                        case Op::SUB:   // x - x → 0
                        case Op::XOR:   // x ^ x → 0
                            trace("algebraic", fn.name, "v" + std::to_string(inst.dest.vregId) + " = x^x/x-x → 0", inst.loc);
                            inst.op = Op::CONST;
                            inst.src1 = Operand::imm(0, inst.resultType);
                            inst.src2 = Operand::none();
                            continue;
                        case Op::AND:   // x & x → x
                        case Op::OR:    // x | x → x
                            trace("algebraic", fn.name, "v" + std::to_string(inst.dest.vregId) + " = x&x/x|x → COPY", inst.loc);
                            inst.op = Op::COPY;
                            inst.src2 = Operand::none();
                            continue;
                        default: break;
                    }
                }

                // Both operands constant (c + 1 on a pointer assigned a
                // constant address): the result, as a constant
                int64_t c1, c2;
                bool fold = false;
                int64_t r = 0;
                if (inst.dest.isVreg() && inst.args.empty() && inst.resultType != Type::F32 &&
                    !fn.localSlotVregs.count(inst.dest.vregId) && !fn.memoryVregs.count(inst.dest.vregId) &&
                    defCount[inst.dest.vregId] == 1 && getConst(inst.src1, c1)) {
                    if (inst.op == Op::COPY) { r = c1; fold = true; }
                    else if (getConst(inst.src2, c2)) {
                        int64_t m = fullMask(inst.resultType);
                        fold = true;
                        switch (inst.op) {
                            case Op::ADD: r = c1 + c2; break;
                            case Op::SUB: r = c1 - c2; break;
                            case Op::MUL: case Op::MUL_U: r = c1 * c2; break;
                            case Op::AND: r = c1 & c2; break;
                            case Op::OR:  r = c1 | c2; break;
                            case Op::XOR: r = c1 ^ c2; break;
                            case Op::SHL: case Op::LSL: r = (c2 >= 0 && c2 < 32) ? (c1 << c2) : 0; break;
                            case Op::SHR: case Op::LSR: r = (c2 >= 0 && c2 < 32) ? ((c1 & m) >> c2) : 0; break;
                            default: fold = false;
                        }
                    }
                    if (fold && (inst.resultType == Type::I8 || inst.resultType == Type::I16 ||
                                 inst.resultType == Type::PTR || inst.resultType == Type::I32)) {
                        trace("algebraic", fn.name, "v" + std::to_string(inst.dest.vregId) + " = constants → CONST " +
                              std::to_string(r & fullMask(inst.resultType)), inst.loc);
                        inst.op = Op::CONST;
                        inst.src1 = Operand::imm(r & fullMask(inst.resultType), inst.resultType);
                        inst.src2 = Operand::none();
                        continue;
                    }
                }

                // Constant-operand patterns: check src2
                int64_t val2;
                if (getConst(inst.src2, val2)) {
                    auto vd = [&]() { return "v" + std::to_string(inst.dest.vregId); };
                    switch (inst.op) {
                        case Op::ADD:
                        case Op::SUB:
                            if (val2 == 0) {
                                // x + 0 → x, x - 0 → x
                                trace("algebraic", fn.name, vd() + " = ADD/SUB +0 → COPY", inst.loc);
                                inst.op = Op::COPY;
                                inst.src2 = Operand::none();
                            }
                            break;
                        case Op::AND:
                            if (val2 == 0) {
                                // x & 0 → 0
                                trace("algebraic", fn.name, vd() + " = AND &0 → CONST 0", inst.loc);
                                inst.op = Op::CONST;
                                inst.src1 = Operand::imm(0, inst.resultType);
                                inst.src2 = Operand::none();
                            } else if ((val2 & fullMask(inst.resultType)) == fullMask(inst.resultType)) {
                                // x & 0xFFFF (I16; also written -1) → x
                                trace("algebraic", fn.name, vd() + " = AND &mask → COPY", inst.loc);
                                inst.op = Op::COPY;
                                inst.src2 = Operand::none();
                            }
                            break;
                        case Op::OR:
                            if (val2 == 0) {
                                // x | 0 → x
                                trace("algebraic", fn.name, vd() + " = OR |0 → COPY", inst.loc);
                                inst.op = Op::COPY;
                                inst.src2 = Operand::none();
                            } else if ((val2 & fullMask(inst.resultType)) == fullMask(inst.resultType)) {
                                // x | 0xFFFF (also written -1) → 0xFFFF
                                trace("algebraic", fn.name, vd() + " = OR |mask → CONST mask", inst.loc);
                                inst.op = Op::CONST;
                                inst.src1 = Operand::imm(fullMask(inst.resultType), inst.resultType);
                                inst.src2 = Operand::none();
                            }
                            break;
                        case Op::MUL:
                        case Op::MUL_U:
                            if (val2 == 1) {
                                // x * 1 → x
                                trace("algebraic", fn.name, vd() + " = MUL *1 → COPY", inst.loc);
                                inst.op = Op::COPY;
                                inst.src2 = Operand::none();
                            } else if (val2 == 0) {
                                // x * 0 → 0
                                trace("algebraic", fn.name, vd() + " = MUL *0 → CONST 0", inst.loc);
                                inst.op = Op::CONST;
                                inst.src1 = Operand::imm(0, inst.resultType);
                                inst.src2 = Operand::none();
                            }
                            break;
                        case Op::XOR:
                            if (val2 == 0) {
                                // x ^ 0 → x
                                trace("algebraic", fn.name, vd() + " = XOR ^0 → COPY", inst.loc);
                                inst.op = Op::COPY;
                                inst.src2 = Operand::none();
                            }
                            break;
                        case Op::SHL:
                        case Op::SHR:
                        case Op::LSL:
                        case Op::LSR:
                        case Op::ASR:
                            if (val2 == 0) {
                                // x << 0 → x, x >> 0 → x
                                trace("algebraic", fn.name, vd() + " = shift <<0 → COPY", inst.loc);
                                inst.op = Op::COPY;
                                inst.src2 = Operand::none();
                            }
                            break;
                        default: break;
                    }
                }

                // Constant-operand patterns: check src1 (for commutative ops)
                int64_t val1;
                if (getConst(inst.src1, val1)) {
                    auto vd = [&]() { return "v" + std::to_string(inst.dest.vregId); };
                    switch (inst.op) {
                        case Op::ADD:
                            if (val1 == 0) {
                                // 0 + x → x
                                trace("algebraic", fn.name, vd() + " = 0+x → COPY", inst.loc);
                                inst.op = Op::COPY;
                                inst.src1 = inst.src2;
                                inst.src2 = Operand::none();
                            }
                            break;
                        case Op::AND:
                            if (val1 == 0) {
                                // 0 & x → 0
                                trace("algebraic", fn.name, vd() + " = 0&x → CONST 0", inst.loc);
                                inst.op = Op::CONST;
                                inst.src1 = Operand::imm(0, inst.resultType);
                                inst.src2 = Operand::none();
                            }
                            break;
                        case Op::OR:
                            if (val1 == 0) {
                                // 0 | x → x
                                trace("algebraic", fn.name, vd() + " = 0|x → COPY", inst.loc);
                                inst.op = Op::COPY;
                                inst.src1 = inst.src2;
                                inst.src2 = Operand::none();
                            }
                            break;
                        default: break;
                    }
                }
            }
        }
    }
}

// ============================================================================
// ADDR_ELEM Fusion with LOAD/STORE
// ============================================================================

void optimizeAddrElemFusion(Module& mod) {
    // When ADDR_ELEM result is used exactly once by the immediately following
    // LOAD or STORE, embed the ADDR_ELEM info into the LOAD/STORE instruction's
    // args and remove the ADDR_ELEM. This allows the codegen to compute the
    // address inline without allocating a ZP slot for the intermediate pointer.
    //
    // Pattern:
    //   %5 = addr_elem %base, %idx, stride
    //   %6 = load i16 %5             (or: store i8 %val, %5)
    //
    // After fusion:
    //   %5 = nop                      (removed)
    //   %6 = load i16 %5             (with args[0]=base, args[1]=idx, args[2]=stride)
    //   The codegen detects args.size() >= 3 on LOAD/STORE as "fused addr_elem".

    for (auto& fn : mod.functions) {
        if (!runOn(fn, "AddrElemFusion")) continue;
        // Build use counts for each vreg
        std::map<uint32_t, int> useCount;
        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                if (inst.src1.isVreg()) useCount[inst.src1.vregId]++;
                if (inst.src2.isVreg()) useCount[inst.src2.vregId]++;
                for (auto& arg : inst.args)
                    if (arg.isVreg()) useCount[arg.vregId]++;
            }
        }

        for (auto& block : fn.blocks) {
            for (size_t i = 0; i + 1 < block.insts.size(); i++) {
                auto& addrInst = block.insts[i];
                auto& nextInst = block.insts[i + 1];

                if (addrInst.op != Op::ADDR_ELEM) continue;
                if (!addrInst.dest.isVreg()) continue;
                uint32_t addrVreg = addrInst.dest.vregId;

                // Must be used exactly once
                if (useCount[addrVreg] != 1) continue;
                // Don't fuse local slot vregs
                if (fn.localSlotVregs.count(addrVreg)) continue;
                // A base that is a local's slot vreg is the variable's address in
                // ADDR_ELEM (an array or vector local); the fused access would
                // read the variable's contents as a pointer instead
                if (addrInst.src1.isVreg() && (fn.localSlotVregs.count(addrInst.src1.vregId) ||
                                               fn.memoryVregs.count(addrInst.src1.vregId))) continue;

                // Next instruction must be LOAD or STORE using this vreg as address
                bool isLoad = (nextInst.op == Op::LOAD && nextInst.src1.isVreg() &&
                               nextInst.src1.vregId == addrVreg);
                bool isStore = (nextInst.op == Op::STORE && nextInst.src2.isVreg() &&
                                nextInst.src2.vregId == addrVreg);
                if (!isLoad && !isStore) continue;
                // The codegen computes a fused address only for 1/2/4-byte
                // integer and pointer accesses (a float or I_N access keeps
                // its address vreg)
                // (and for loads only: the fused 4-byte store is not implemented)
                Type t = nextInst.resultType;
                if (t != Type::I8 && t != Type::I16 && t != Type::PTR && !(isLoad && t == Type::I32)) continue;
                if (!nextInst.args.empty()) continue;   // already carries other operands

                // Fuse: copy ADDR_ELEM params into the LOAD/STORE args
                trace("fusion", fn.name, "v" + std::to_string(addrVreg) + " ADDR_ELEM fused into " + (isLoad ? "LOAD" : "STORE"), addrInst.loc);
                int elemSize = (addrInst.args.size() > 0) ? (int)addrInst.args[0].immVal : 2;
                nextInst.args.clear();
                nextInst.args.push_back(addrInst.src1); // base
                nextInst.args.push_back(addrInst.src2); // index
                nextInst.args.push_back(Operand::imm(elemSize, Type::I16)); // stride

                // Remove the ADDR_ELEM
                addrInst.op = Op::NOP;
                addrInst.dest = Operand::none();
                addrInst.src1 = Operand::none();
                addrInst.src2 = Operand::none();
                addrInst.args.clear();
            }
        }
    }
}

// ============================================================================
// COPY Chain Elimination
// ============================================================================

void optimizeCopyChains(Module& mod) {
    for (auto& fn : mod.functions) {
        if (!runOn(fn, "CopyPropagation")) continue;
        // Build forwarding map: follow COPY chains to the root operand.
        // %a = COPY %b; %c = COPY %a → %c should reference %b directly.
        // The map is used across the whole function, so (vregs not being SSA)
        // only a copy defined once whose root is a temp defined at most once
        // — never a variable's slot vreg, which assignments change — is
        // forwarded.
        std::map<uint32_t, int> defCount;
        for (auto& block : fn.blocks)
            for (auto& inst : block.insts) {
                if (inst.dest.isVreg()) defCount[inst.dest.vregId]++;
                if ((inst.op == Op::STORE || inst.op == Op::STORE_ZP) && inst.src2.isVreg())
                    defCount[inst.src2.vregId] += 2;   // assigned: never a fixed value
            }
        auto fixedTemp = [&](uint32_t v) {
            return !fn.localSlotVregs.count(v) && defCount[v] <= 1;
        };
        // Backward: %t = op ...; %d = copy %t (the copy right after, %t used
        // only there) is %d = op ...: the arms of a ternary (x ? a - b : c)
        // compute into its result instead of a temp copied to it (Issue #293)
        {
            std::map<uint32_t, int> useCount;
            for (auto& block : fn.blocks)
                for (auto& inst : block.insts) {
                    for (const Operand* o : {&inst.src1, &inst.src2})
                        if (o->isVreg()) useCount[o->vregId]++;
                    for (auto& a : inst.args) if (a.isVreg()) useCount[a.vregId]++;
                    for (auto& ph : inst.phiIncoming) if (ph.first.isVreg()) useCount[ph.first.vregId] += 2;
                }
            for (auto& block : fn.blocks) {
                auto& insts = block.insts;
                for (size_t k = 0; k < insts.size(); k++) {
                    Inst& def = insts[k];
                    // Arithmetic and logic only: the codegen takes a CONST's vreg
                    // for its value (allocated nowhere, an immediate operand) and a
                    // ZEXT/TRUNC's result for its source byte, which needs their
                    // one definition (x ? 1 : 0 keeps its constants copied)
                    switch (def.op) {
                        case Op::ADD: case Op::SUB: case Op::MUL: case Op::MUL_U:
                        case Op::AND: case Op::OR: case Op::XOR: case Op::SHL: case Op::SHR:
                        case Op::ASR: case Op::NEG: case Op::NOT:
                            break;
                        default:
                            continue;
                    }
                    if (!def.dest.isVreg()) continue;
                    uint32_t t = def.dest.vregId;
                    if (!fixedTemp(t) || useCount[t] != 1 || fn.memoryVregs.count(t)) continue;
                    size_t j = k + 1;
                    while (j < insts.size() && insts[j].op == Op::NOP) j++;
                    if (j >= insts.size()) continue;
                    Inst& cp = insts[j];
                    if (cp.op != Op::COPY || !cp.src1.isVreg() || cp.src1.vregId != t || !cp.dest.isVreg()) continue;
                    uint32_t d = cp.dest.vregId;
                    if (d == t || fn.localSlotVregs.count(d) || fn.memoryVregs.count(d) ||
                        cp.resultType != def.resultType)
                        continue;
                    // (an operation that reads %d itself keeps its temp)
                    bool reads = (def.src1.isVreg() && def.src1.vregId == d) || (def.src2.isVreg() && def.src2.vregId == d);
                    for (auto& a : def.args) if (a.isVreg() && a.vregId == d) reads = true;
                    if (reads) continue;
                    trace("copy", fn.name, "v" + std::to_string(t) + " computed into v" + std::to_string(d), def.loc);
                    def.dest = cp.dest;
                    cp.op = Op::NOP;
                    cp.src1 = Operand::none();
                    cp.src2 = Operand::none();
                    cp.dest = Operand::none();
                }
            }
        }

        std::map<uint32_t, Operand> copyRoot;
        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                if (inst.op == Op::COPY && inst.dest.isVreg() && inst.src1.isVreg() &&
                    fixedTemp(inst.dest.vregId) && fixedTemp(inst.src1.vregId)) {
                    // Follow chain to root
                    Operand root = inst.src1;
                    int depth = 16;
                    while (root.isVreg() && --depth > 0) {
                        auto it = copyRoot.find(root.vregId);
                        if (it != copyRoot.end())
                            root = it->second;
                        else
                            break;
                    }
                    copyRoot[inst.dest.vregId] = root;
                }
            }
        }

        if (copyRoot.empty()) continue;

        for (auto& [dest, root] : copyRoot) {
            trace("copy", fn.name, "v" + std::to_string(dest) + " chain → v" + std::to_string(root.vregId));
        }

        // Rewrite all operands using the forwarding map
        auto rewrite = [&](Operand& op) {
            if (op.isVreg()) {
                auto it = copyRoot.find(op.vregId);
                if (it != copyRoot.end()) {
                    op.vregId = it->second.vregId;
                }
            }
        };

        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                rewrite(inst.src1);
                rewrite(inst.src2);
                for (auto& arg : inst.args) rewrite(arg);
                for (auto& phi : inst.phiIncoming) rewrite(phi.first);
            }
        }

        // Mark COPYs whose dest is no longer referenced as NOP
        std::set<uint32_t> usedVregs;
        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                if (inst.src1.isVreg()) usedVregs.insert(inst.src1.vregId);
                if (inst.src2.isVreg()) usedVregs.insert(inst.src2.vregId);
                for (auto& arg : inst.args)
                    if (arg.isVreg()) usedVregs.insert(arg.vregId);
                for (auto& phi : inst.phiIncoming)
                    if (phi.first.isVreg()) usedVregs.insert(phi.first.vregId);
            }
        }
        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                if (inst.op == Op::COPY && inst.dest.isVreg() &&
                    !usedVregs.count(inst.dest.vregId) &&
                    !fn.localSlotVregs.count(inst.dest.vregId)) {
                    trace("copy", fn.name, "v" + std::to_string(inst.dest.vregId) + " dead COPY removed", inst.loc);
                    inst.op = Op::NOP;
                    inst.src1 = Operand::none();
                    inst.src2 = Operand::none();
                    inst.dest = Operand::none();
                }
            }
        }
    }
}

// ============================================================================
// Type Narrowing (I16/I32 → I8)
// ============================================================================

static void forwardNarrowing(Function& fn);

void optimizeTypeNarrowing(Module& mod) {
    for (auto& fn : mod.functions) {
        if (!runOn(fn, "TypeNarrowing")) continue;
        // Build use map: for each vreg, collect all instructions that use it
        // and what role (src1, src2, arg, store-value)
        struct UseInfo {
            Inst* inst;
            enum Role { SRC1, SRC2, ARG, STORE_VAL, PHI, OTHER };
            Role role;
        };
        std::map<uint32_t, std::vector<UseInfo>> uses;
        // (pointers into the blocks' instruction vectors: built again after
        // anything inserts instructions)
        auto buildUses = [&]() {
            uses.clear();
            for (auto& block : fn.blocks) {
                for (auto& inst : block.insts) {
                    if (inst.src1.isVreg())
                        uses[inst.src1.vregId].push_back({&inst, UseInfo::SRC1});
                    if (inst.src2.isVreg())
                        uses[inst.src2.vregId].push_back({&inst, UseInfo::SRC2});
                    for (auto& arg : inst.args) {
                        if (arg.isVreg())
                            uses[arg.vregId].push_back({&inst, UseInfo::ARG});
                    }
                }
            }
        };
        buildUses();

        // a vreg's one definition, if it has exactly one
        std::map<uint32_t, int> defCount;
        std::map<uint32_t, const Inst*> defOf;
        for (const auto& block : fn.blocks)
            for (const auto& in : block.insts)
                if (in.dest.isVreg()) { defCount[in.dest.vregId]++; defOf[in.dest.vregId] = &in; }
        auto zextByte = [&](const Operand& o) {
            if (!o.isVreg() || defCount[o.vregId] != 1 || fn.localSlotVregs.count(o.vregId)) return false;
            const Inst* d = defOf[o.vregId];
            return d->op == Op::ZEXT && d->src1.type == Type::I8;
        };
        // Narrowable arithmetic ops (safe to compute in I8 when result is truncated)
        auto isNarrowable = [](Op op) -> bool {
            // (only when every use is a truncation to 8 bits: the low byte of
            // a sum, difference, product, ... does not depend on the high
            // bytes; a result used wider, e.g. a char difference compared as
            // a signed int, is not narrowed)
            switch (op) {
                case Op::ADD: case Op::SUB: case Op::NEG:
                case Op::MUL: case Op::MUL_U:
                case Op::AND: case Op::OR: case Op::XOR:
                case Op::SHL:   // not SHR: the low byte of x >> n comes from x's high bits
                case Op::NOT:
                    return true;
                default:
                    return false;
            }
        };

        // Check if ALL uses of a vreg read only its low byte: TRUNCs to I8,
        // and byte operations whose low byte depends only on their operands'
        // low bytes (#342: (char)(v << 4 | v >> 4))
        auto lowByteUse = [](const UseInfo& u) {
            if ((u.inst->op == Op::TRUNC || u.inst->op == Op::COPY) && u.inst->resultType == Type::I8 &&
                u.role == UseInfo::SRC1)
                return true;
            if (u.inst->resultType != Type::I8 || (u.role != UseInfo::SRC1 && u.role != UseInfo::SRC2)) return false;
            switch (u.inst->op) {
                case Op::ADD: case Op::SUB: case Op::MUL: case Op::MUL_U:
                case Op::AND: case Op::OR: case Op::XOR: case Op::NOT: case Op::NEG:
                    return true;
                case Op::SHL: case Op::LSL:   // (the value, not the count)
                    return u.role == UseInfo::SRC1;
                default:
                    return false;
            }
        };
        auto allUsesTruncI8 = [&](uint32_t vregId) -> bool {
            auto it = uses.find(vregId);
            if (it == uses.end()) return false;
            for (auto& u : it->second)
                if (!lowByteUse(u)) return false;   // a wider use — can't narrow
            return true;
        };
        // the byte operations reading it: their operand is a byte now
        auto retypeUses = [&](uint32_t vregId) {
            for (auto& u : uses[vregId]) {
                if (u.inst->op == Op::TRUNC) continue;
                for (Operand* o : {&u.inst->src1, &u.inst->src2})
                    if (o->isVreg() && o->vregId == vregId) o->type = Type::I8;
            }
        };

        // Pass: narrow I16/I32 ops whose result is only used via TRUNC I8
        // (again until none: narrowing a use can make its operands narrowable)
        for (bool again = true; again;) {
        again = false;
        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                if (!inst.dest.isVreg()) continue;
                if (inst.resultType != Type::I16 && inst.resultType != Type::I32) continue;
                // (x >> n of a zero-extended byte: its low byte is the byte's)
                bool shrByte = (inst.op == Op::SHR || inst.op == Op::LSR) && zextByte(inst.src1) &&
                               (inst.src2.isImm() || inst.src2.isVreg());
                if (!isNarrowable(inst.op) && !shrByte) continue;

                // Don't narrow local slot vregs
                if (fn.localSlotVregs.count(inst.dest.vregId)) continue;

                if (!allUsesTruncI8(inst.dest.vregId)) continue;

                // Narrow: change result type to I8
                trace("narrow", fn.name, "v" + std::to_string(inst.dest.vregId) + " narrowed to I8", inst.loc);
                inst.resultType = Type::I8;
                retypeUses(inst.dest.vregId);
                again = true;

                // Its operands extended from a byte: the byte itself (only the
                // low byte is read), when the byte keeps its value up to here
                size_t at = &inst - &block.insts[0];
                for (Operand* o : {&inst.src1, &inst.src2}) {
                    if (!o->isVreg()) continue;
                    size_t d = at;
                    while (d-- > 0) {
                        const Inst& x = block.insts[d];
                        if (x.dest.isVreg() && x.dest.vregId == o->vregId) break;
                    }
                    if (d == (size_t)-1) continue;
                    const Inst& ext = block.insts[d];
                    if ((ext.op != Op::ZEXT && ext.op != Op::SEXT) || !ext.src1.isVreg() ||
                        ext.src1.type != Type::I8 || fn.localSlotVregs.count(o->vregId))
                        continue;
                    uint32_t src = ext.src1.vregId;
                    bool keeps = true;
                    for (size_t k = d + 1; k < at && keeps; k++) {
                        const Inst& x = block.insts[k];
                        if ((x.dest.isVreg() && (x.dest.vregId == src || x.dest.vregId == o->vregId)) ||
                            (x.op == Op::STORE && x.src2.isVreg() && x.src2.vregId == src) ||
                            x.op == Op::CALL || x.op == Op::CALL_VOID || x.op == Op::CALL_INDIRECT ||
                            x.op == Op::ASM_INLINE)
                            keeps = false;
                        // (memory it shares with pointers: only instructions that write no memory)
                        if (fn.memoryVregs.count(src) &&
                            !(x.op == Op::CONST || x.op == Op::COPY || x.op == Op::ADD || x.op == Op::SUB ||
                              x.op == Op::TRUNC || x.op == Op::ZEXT || x.op == Op::SEXT || x.op == Op::DEREF ||
                              x.op == Op::NOP || (x.op == Op::LOAD && x.args.empty())))
                            keeps = false;
                    }
                    if (keeps) *o = Operand::vreg(src, Type::I8);
                }

                // Convert each TRUNC consumer to COPY (the result is already I8)
                auto& useList = uses[inst.dest.vregId];
                for (auto& u : useList) {
                    if (u.inst->op == Op::TRUNC && u.inst->resultType == Type::I8) {
                        u.inst->op = Op::COPY;
                        u.inst->resultType = Type::I8;
                        // the operand is an I8 value now (copy propagation
                        // passes this operand on to the copy's users)
                        u.inst->src1.type = Type::I8;
                    }
                }
            }
        }
        // a vreg of several definitions (a ?: result) whose uses read its low
        // byte only, each definition narrowable, a copy or extension of a
        // byte, or a constant: all of them in 8 bits (#342)
        buildUses();
        defCount.clear();
        std::map<uint32_t, std::vector<Inst*>> defsOf;
        for (auto& block : fn.blocks)
            for (auto& in : block.insts)
                if (in.dest.isVreg()) { defCount[in.dest.vregId]++; defsOf[in.dest.vregId].push_back(&in); }
        for (auto& [v, defs] : defsOf) {
            if (defs.size() < 2 || fn.localSlotVregs.count(v) || !allUsesTruncI8(v)) continue;
            bool ok = true;
            bool wide = false;
            for (const Inst* d : defs) {
                if (d->resultType == Type::I8) continue;   // (narrowed already)
                if (d->resultType != Type::I16 && d->resultType != Type::I32) { ok = false; break; }
                wide = true;
                bool byteCopy = (d->op == Op::COPY && d->src1.isVreg() && zextByte(d->src1)) ||
                                (d->op == Op::ZEXT && d->src1.type == Type::I8) || d->op == Op::CONST;
                if (!isNarrowable(d->op) && !byteCopy) { ok = false; break; }
            }
            if (!ok || !wide) continue;
            for (Inst* d : defs) {
                if (d->resultType == Type::I8) continue;
                if (d->op == Op::ZEXT) d->op = Op::COPY;
                if (d->op == Op::CONST) d->src1 = Operand::imm(d->src1.immVal & 0xFF, Type::I8);
                if (d->op == Op::COPY) d->src1.type = Type::I8;
                d->resultType = Type::I8;
                d->dest.type = Type::I8;
            }
            if (fn.vregTypes.count(v)) fn.vregTypes[v] = Type::I8;
            trace("narrow", fn.name, "v" + std::to_string(v) + " (" + std::to_string(defs.size()) + " definitions) narrowed to I8");
            retypeUses(v);
            for (auto& u : uses[v])
                if (u.inst->op == Op::TRUNC && u.inst->resultType == Type::I8) {
                    u.inst->op = Op::COPY;
                    u.inst->src1.type = Type::I8;
                }
            again = true;
        }
        if (again) buildUses();
        }

        // Pass 1b: operations on zero-extended bytes on the bytes; constant
        // operands as immediates
        forwardNarrowing(fn);
        buildUses();   // (forwardNarrowing inserts instructions)

        // Pass 2: Eliminate redundant ZEXT/SEXT where the extended value is
        // only used in I8 context (TRUNC back to I8)
        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                if (inst.op != Op::ZEXT && inst.op != Op::SEXT) continue;
                if (!inst.dest.isVreg()) continue;
                // Check if src1 is I8 and all uses are TRUNC back to I8
                if (inst.src1.type != Type::I8) continue;

                if (!allUsesTruncI8(inst.dest.vregId)) continue;

                // The ZEXT/SEXT is pointless — replace with COPY
                trace("narrow", fn.name, "v" + std::to_string(inst.dest.vregId) + " redundant ZEXT/SEXT → COPY", inst.loc);
                inst.op = Op::COPY;
                inst.resultType = Type::I8;
                retypeUses(inst.dest.vregId);

                // Convert TRUNC consumers to COPY
                auto& useList = uses[inst.dest.vregId];
                for (auto& u : useList) {
                    if (u.inst->op == Op::TRUNC && u.inst->resultType == Type::I8) {
                        u.inst->op = Op::COPY;
                        u.inst->resultType = Type::I8;
                        // the operand is an I8 value now (copy propagation
                        // passes this operand on to the copy's users)
                        u.inst->src1.type = Type::I8;
                    }
                }
            }
        }

        // Immediate operands of the byte operations as bytes: p & ~0x80 arrives
        // as a 16-bit AND with -129, which is $7F once the AND is narrowed
        // (Issue #288; the low byte of the result depends only on it)
        for (auto& block : fn.blocks)
            for (auto& inst : block.insts) {
                if (inst.resultType != Type::I8) continue;
                switch (inst.op) {
                    case Op::ADD: case Op::SUB: case Op::MUL: case Op::MUL_U:
                    case Op::AND: case Op::OR: case Op::XOR:
                        for (Operand* o : {&inst.src1, &inst.src2})
                            if (o->isImm() && (o->immVal < 0 || o->immVal > 0xFF))
                                *o = Operand::imm(o->immVal & 0xFF, Type::I8);
                        break;
                    default:
                        break;
                }
            }
    }
}

// Forward narrowing and constant operands (part of TypeNarrowing).
//
// C promotes a byte to int before & | ^ and comparisons, so `flags & 1` or
// `c == 'a'` arrive as 16-bit operations on zero-extended bytes, with the
// constant in a vreg of its own. Here:
// - a ZEXT of a constant becomes a wider constant;
// - a vreg defined by one CONST becomes an immediate second operand of the
//   ops IRCodeGen takes immediates for (commutative ops with a constant first
//   operand are swapped);
// - AND/OR/XOR of zero-extended bytes (or a byte and a constant) are computed
//   on the bytes and zero-extended once: %t = op i8 a, b ; %d = zext %t;
// - comparisons of zero-extended bytes (and constants 0..255) compare the
//   bytes (unsigned: zero-extended values are never negative);
// - a branch on a zero-extended byte tests the byte.
// A vreg counts as fixed only with exactly one definition and no STORE to it
// (the IR is not SSA).
static void forwardNarrowing(Function& fn) {
    std::map<uint32_t, int> defCount;
    std::set<uint32_t> storedTo;
    for (auto& block : fn.blocks)
        for (auto& inst : block.insts) {
            if (inst.dest.isVreg()) defCount[inst.dest.vregId]++;
            if ((inst.op == Op::STORE || inst.op == Op::STORE_ZP) && inst.src2.isVreg())
                storedTo.insert(inst.src2.vregId);
        }
    auto single = [&](uint32_t v) {
        auto it = defCount.find(v);
        return it != defCount.end() && it->second == 1 && !storedTo.count(v) && !fn.localSlotVregs.count(v);
    };
    // the one definition of each single-def vreg
    std::map<uint32_t, Inst*> def;
    for (auto& block : fn.blocks)
        for (auto& inst : block.insts)
            if (inst.dest.isVreg() && single(inst.dest.vregId)) def[inst.dest.vregId] = &inst;
    auto constOf = [&](const Operand& o, int64_t& v) -> bool {
        if (o.isImm()) { v = o.immVal; return true; }
        if (!o.isVreg()) return false;
        auto it = def.find(o.vregId);
        if (it == def.end() || it->second->op != Op::CONST || !it->second->src1.isImm()) return false;
        v = it->second->src1.immVal;
        return true;
    };
    // a zero-extended byte: the I8 source vreg
    auto zextByte = [&](const Operand& o, Operand& src) -> bool {
        if (!o.isVreg()) return false;
        auto it = def.find(o.vregId);
        if (it == def.end() || it->second->op != Op::ZEXT) return false;
        const Operand& s1 = it->second->src1;
        if (!s1.isVreg() || s1.type != Type::I8) return false;
        src = s1;
        return true;
    };
    auto isInt = [](Type t) { return t == Type::I8 || t == Type::I16 || t == Type::I32; };

    // 1. ZEXT/SEXT/TRUNC of a constant: a constant of the new width (a
    // constant argument, operate('d', 'l', n), is then an immediate stored
    // into the parameter block, not a constant in a register-pool temp
    // truncated and copied there: Issue #290)
    for (auto& block : fn.blocks)
        for (auto& inst : block.insts) {
            int64_t v;
            if (!inst.dest.isVreg() || !isInt(inst.resultType) || !isInt(inst.src1.type) || !constOf(inst.src1, v))
                continue;
            int64_t from = inst.src1.type == Type::I8 ? 0xFF : inst.src1.type == Type::I16 ? 0xFFFF : 0xFFFFFFFF;
            int64_t to = inst.resultType == Type::I8 ? 0xFF : inst.resultType == Type::I16 ? 0xFFFF : 0xFFFFFFFF;
            if (inst.op == Op::ZEXT && inst.src1.type != Type::I32) {
                inst.op = Op::CONST;
                inst.src1 = Operand::imm(v & from, inst.resultType);
            } else if (inst.op == Op::SEXT && inst.src1.type != Type::I32) {
                int64_t sign = (from + 1) >> 1;
                int64_t x = v & from;
                if (x & sign) x -= from + 1;
                inst.op = Op::CONST;
                inst.src1 = Operand::imm(x & to, inst.resultType);
            } else if (inst.op == Op::TRUNC) {
                inst.op = Op::CONST;
                inst.src1 = Operand::imm(v & to, inst.resultType);
            }
        }

    // 2. narrowing of AND/OR/XOR, comparisons, branches. The byte
    // operations are inserted after the scan of every block: def holds
    // pointers into the blocks' instruction vectors (of later blocks' operands
    // too), which an insert would move
    std::map<size_t, std::vector<std::pair<size_t, Inst>>> inserts;   // block -> (before index, instruction)
    for (size_t bi = 0; bi < fn.blocks.size(); bi++) {
        auto& block = fn.blocks[bi];
        for (size_t i = 0; i < block.insts.size(); i++) {
            Inst& inst = block.insts[i];
            bool bitwise = inst.op == Op::AND || inst.op == Op::OR || inst.op == Op::XOR;
            bool cmp = inst.op >= Op::CMP_EQ && inst.op <= Op::CMP_GEU;
            if (inst.op == Op::BR_COND) {
                Operand b;
                if (zextByte(inst.src1, b)) inst.src1 = b;
                continue;
            }
            if (!bitwise && !cmp) continue;
            if (inst.src1.type != Type::I16 && inst.src1.type != Type::I32) continue;
            Operand a, b;
            int64_t k;
            bool aByte = zextByte(inst.src1, a);
            bool bByte = zextByte(inst.src2, b);
            bool bConst = constOf(inst.src2, k);
            if (!aByte) continue;
            if (!bByte) {
                if (!bConst) continue;
                if (inst.op == Op::AND) k &= 0xFF;          // the high bits of a are 0
                else if (k < 0 || k > 0xFF) continue;
                b = Operand::imm(k, Type::I8);
            }
            if (cmp) {
                // zero-extended values compare as unsigned bytes
                switch (inst.op) {
                    case Op::CMP_LT: inst.op = Op::CMP_LTU; break;
                    case Op::CMP_LE: inst.op = Op::CMP_LEU; break;
                    case Op::CMP_GT: inst.op = Op::CMP_GTU; break;
                    case Op::CMP_GE: inst.op = Op::CMP_GEU; break;
                    default: break;
                }
                inst.src1 = a;
                inst.src2 = b;
                trace("narrow", fn.name, "compare of zero-extended bytes on the bytes", inst.loc);
                continue;
            }
            if (!inst.dest.isVreg() || fn.localSlotVregs.count(inst.dest.vregId)) continue;
            if (inst.resultType == Type::I8) {
                // the result is a byte already: only the operands change
                inst.src1 = a;
                inst.src2 = b;
                continue;
            }
            // %t = op i8 a, b ; %d = zext %t
            uint32_t t = fn.allocVreg();
            Inst narrow = inst;
            narrow.dest = Operand::vreg(t, Type::I8);
            narrow.resultType = Type::I8;
            narrow.src1 = a;
            narrow.src2 = b;
            Inst ext;
            ext.op = Op::ZEXT;
            ext.dest = inst.dest;
            ext.resultType = inst.resultType;
            ext.src1 = Operand::vreg(t, Type::I8);
            ext.loc = inst.loc;
            trace("narrow", fn.name, "v" + std::to_string(inst.dest.vregId) + " computed on bytes", inst.loc);
            block.insts[i] = ext;
            inserts[bi].push_back({i, narrow});
        }
    }
    for (auto& [bi, list] : inserts)
        for (auto it = list.rbegin(); it != list.rend(); ++it)
            fn.blocks[bi].insts.insert(fn.blocks[bi].insts.begin() + it->first, it->second);
    // (the blocks changed: the definitions again)
    defCount.clear();
    storedTo.clear();
    for (auto& block : fn.blocks)
        for (auto& inst : block.insts) {
            if (inst.dest.isVreg()) defCount[inst.dest.vregId]++;
            if ((inst.op == Op::STORE || inst.op == Op::STORE_ZP) && inst.src2.isVreg())
                storedTo.insert(inst.src2.vregId);
        }
    def.clear();
    for (auto& block : fn.blocks)
        for (auto& inst : block.insts)
            if (inst.dest.isVreg() && single(inst.dest.vregId)) def[inst.dest.vregId] = &inst;

    // 3. constant second operands as immediates
    for (auto& block : fn.blocks) {
        for (auto& inst : block.insts) {
            bool commutative = inst.op == Op::ADD || inst.op == Op::AND || inst.op == Op::OR ||
                               inst.op == Op::XOR || inst.op == Op::CMP_EQ || inst.op == Op::CMP_NE;
            bool takesImm = commutative || inst.op == Op::SUB || inst.op == Op::SHL || inst.op == Op::SHR ||
                            inst.op == Op::ASR || (inst.op >= Op::CMP_EQ && inst.op <= Op::CMP_GEU);
            if (!takesImm || (!isInt(inst.src1.type) && inst.src1.type != Type::PTR)) continue;
            int64_t v;
            if (commutative && !inst.src2.isImm() && constOf(inst.src1, v) && inst.src2.isVreg())
                std::swap(inst.src1, inst.src2);
            if (inst.src2.isVreg() && constOf(inst.src2, v)) {
                Type t = inst.src2.type;
                inst.src2 = Operand::imm(v, t);
            }
        }
    }

    // 4. unsigned comparisons with 0 or 1: an unsigned value is never below 0,
    // so x > 0 is x != 0 (the Z flag alone, no carry), x >= 0 is true, ...
    for (auto& block : fn.blocks) {
        for (auto& inst : block.insts) {
            if (inst.op != Op::CMP_LTU && inst.op != Op::CMP_LEU &&
                inst.op != Op::CMP_GTU && inst.op != Op::CMP_GEU) continue;
            if (!inst.dest.isVreg() || fn.localSlotVregs.count(inst.dest.vregId)) continue;
            int64_t v;
            Op op = inst.op;
            Operand x;
            if (constOf(inst.src2, v) && !constOf(inst.src1, v)) {
                constOf(inst.src2, v);
                x = inst.src1;
            } else if (constOf(inst.src1, v) && !constOf(inst.src2, v)) {
                constOf(inst.src1, v);
                x = inst.src2;
                // k OP x  ->  x OP' k
                op = op == Op::CMP_LTU ? Op::CMP_GTU : op == Op::CMP_LEU ? Op::CMP_GEU :
                     op == Op::CMP_GTU ? Op::CMP_LTU : Op::CMP_LEU;
            } else continue;
            int result = -1;          // 0 / 1: always false / true
            Op eq = Op::NOP;          // or x == 0 / x != 0
            if (v == 0) {
                if (op == Op::CMP_GTU) eq = Op::CMP_NE;
                else if (op == Op::CMP_LEU) eq = Op::CMP_EQ;
                else if (op == Op::CMP_GEU) result = 1;
                else result = 0;
            } else if (v == 1) {
                if (op == Op::CMP_LTU) eq = Op::CMP_EQ;
                else if (op == Op::CMP_GEU) eq = Op::CMP_NE;
                else continue;
            } else continue;
            if (result >= 0) {
                inst.op = Op::CONST;
                inst.src1 = Operand::imm(result, inst.resultType);
                inst.src2 = Operand::none();
                trace("narrow", fn.name, "unsigned compare with 0 is constant", inst.loc);
            } else {
                inst.op = eq;
                inst.src1 = x;
                inst.src2 = Operand::imm(0, x.type);
                trace("narrow", fn.name, "unsigned compare with 0/1 is a test for 0", inst.loc);
            }
        }
    }
}

// ============================================================================
// Branch Folding and Dead Block Elimination
// ============================================================================

void optimizeBranchFold(Module& mod) {
    for (auto& fn : mod.functions) {
        if (!runOn(fn, "BranchFolding")) continue;
        bool changed = true;
        int maxIter = 16;
        while (changed && --maxIter > 0) {
            changed = false;

            // Build constant value map. Vregs are not SSA: a vreg is a constant
            // only when its one definition is a CONST and nothing stores to it
            // (a && b's result gets CONST 0 on one path and a value on the other;
            // a STORE to a local's slot vreg assigns the variable)
            std::map<uint32_t, int64_t> constVals;
            std::map<uint32_t, int> defCount;
            std::set<uint32_t> storedTo;
            for (auto& block : fn.blocks) {
                for (auto& inst : block.insts) {
                    if (inst.dest.isVreg()) defCount[inst.dest.vregId]++;
                    if ((inst.op == Op::STORE || inst.op == Op::STORE_ZP) && inst.src2.isVreg())
                        storedTo.insert(inst.src2.vregId);
                    if (inst.op == Op::CONST && inst.dest.isVreg() && inst.src1.isImm())
                        constVals[inst.dest.vregId] = inst.src1.immVal;
                }
            }
            for (auto it = constVals.begin(); it != constVals.end();) {
                if (defCount[it->first] != 1 || storedTo.count(it->first) || fn.localSlotVregs.count(it->first))
                    it = constVals.erase(it);
                else
                    ++it;
            }

            // Pass 0: instructions after a block's first terminator never run
            // (break inside if: "br @while_end; br @if_end"); later passes and
            // the codegen treat the last instruction as the terminator
            for (auto& blk : fn.blocks) {
                for (size_t k = 0; k + 1 < blk.insts.size(); k++) {
                    Op op = blk.insts[k].op;
                    if (op == Op::BR || op == Op::BR_COND || op == Op::BR_INDIRECT || op == Op::SWITCH ||
                        op == Op::RET || op == Op::RET_VOID) {
                        trace("branch", fn.name, "unreachable instructions after the terminator of " + blk.label + " removed");
                        blk.insts.erase(blk.insts.begin() + k + 1, blk.insts.end());
                        changed = true;
                        break;
                    }
                }
            }

            // Pass 1: Fold BR_COND on constant condition
            // If target is the next block, remove the branch entirely (fallthrough).
            // Otherwise, replace with unconditional BR.
            for (size_t bi = 0; bi < fn.blocks.size(); bi++) {
                for (auto it = fn.blocks[bi].insts.begin(); it != fn.blocks[bi].insts.end(); ++it) {
                    if (it->op != Op::BR_COND) continue;
                    if (!it->src1.isVreg()) continue;
                    auto cv = constVals.find(it->src1.vregId);
                    if (cv == constVals.end()) continue;

                    std::string target = (cv->second != 0) ? it->dest.name : it->src2.name;

                    // If target is the next block, just remove the branch (fallthrough)
                    if (bi + 1 < fn.blocks.size() && fn.blocks[bi + 1].label == target) {
                        trace("branch", fn.name, "BR_COND const → fallthrough");
                        fn.blocks[bi].insts.erase(it);
                        break;   // (the terminator: the iterator is gone)
                    } else {
                        trace("branch", fn.name, "BR_COND const → BR " + target);
                        it->op = Op::BR;
                        it->src1 = Operand::label(target);
                        it->src2 = Operand::none();
                        it->dest = Operand::none();
                    }
                    changed = true;
                    break; // block's terminator changed, move on
                }
            }

            // Pass 2: Thread jump chains — BR to a block that only contains BR
            {
                // Build label → block index map
                std::map<std::string, size_t> blockIndex;
                for (size_t i = 0; i < fn.blocks.size(); i++)
                    blockIndex[fn.blocks[i].label] = i;

                for (auto& block : fn.blocks) {
                    for (auto& inst : block.insts) {
                        auto threadLabel = [&](std::string& label) {
                            int depth = 8; // limit chain depth
                            while (--depth > 0) {
                                auto bi = blockIndex.find(label);
                                if (bi == blockIndex.end()) break;
                                auto& target = fn.blocks[bi->second];
                                // Block contains only a single BR instruction?
                                if (target.insts.size() == 1 && target.insts[0].op == Op::BR &&
                                    !target.insts[0].src1.name.empty() &&
                                    target.insts[0].src1.name != label) {
                                    trace("branch", fn.name, "thread " + label + " → " + target.insts[0].src1.name);
                                    label = target.insts[0].src1.name;
                                    changed = true;
                                } else {
                                    break;
                                }
                            }
                        };

                        // An edge of br_cond %v to a block that only tests %v
                        // again: the test is decided on that edge (if (!block
                        // && !len) ... else if (!block || ...): Issue #295)
                        auto threadCond = [&](std::string& label, uint32_t v, bool truth) {
                            int depth = 8;
                            while (--depth > 0) {
                                auto bi = blockIndex.find(label);
                                if (bi == blockIndex.end()) break;
                                const Inst* only = nullptr;
                                bool other = false;
                                for (const auto& x : fn.blocks[bi->second].insts) {
                                    if (x.op == Op::NOP) continue;
                                    if (only) { other = true; break; }
                                    only = &x;
                                }
                                if (other || !only || only->op != Op::BR_COND || !only->src1.isVreg() ||
                                    only->src1.vregId != v)
                                    break;
                                const std::string& next = truth ? only->dest.name : only->src2.name;
                                if (next.empty() || next == label) break;
                                trace("branch", fn.name, "thread " + label + " → " + next + " (condition known)");
                                label = next;
                                changed = true;
                            }
                        };

                        if (inst.op == Op::BR && !inst.src1.name.empty()) {
                            threadLabel(inst.src1.name);
                        } else if (inst.op == Op::BR_COND) {
                            if (!inst.dest.name.empty()) threadLabel(inst.dest.name);
                            if (!inst.src2.name.empty()) threadLabel(inst.src2.name);
                            if (inst.src1.isVreg()) {
                                if (!inst.dest.name.empty()) threadCond(inst.dest.name, inst.src1.vregId, true);
                                if (!inst.src2.name.empty()) threadCond(inst.src2.name, inst.src1.vregId, false);
                            }
                        }
                    }
                }
            }

            // Pass 2b: tests whose outcome is known where they run (Issue #295).
            // Forward dataflow over the blocks: a fact "%v != 0" / "%v == 0"
            // holds on the edges of a br_cond on %v and lasts until %v is
            // assigned; a block's facts are those of all its predecessors. A
            // test of a known %v becomes a plain branch (if (block) ...
            // if (!block && !len) ... else if (!block || cap < len) tested
            // block three times on paths where it was known).
            {
                size_t nb = fn.blocks.size();
                std::map<std::string, size_t> at;
                for (size_t i = 0; i < nb; i++) at[fn.blocks[i].label] = i;
                // vregs whose value memory can change (address taken): no facts
                std::set<uint32_t> mem(fn.memoryVregs.begin(), fn.memoryVregs.end());
                // labels whose address is taken (here, or by a nested function
                // for its goto to a label of this one): entered from anywhere
                std::set<std::string> taken;
                for (const auto& f : mod.functions)
                    for (const auto& b : f.blocks)
                        for (const auto& x : b.insts)
                            if (x.op == Op::ADDR_LABEL) taken.insert(x.src1.name);
                for (const auto& b : fn.blocks)
                    for (const auto& x : b.insts)
                        if (x.op == Op::ADDR_LOCAL && x.src1.isVreg()) mem.insert(x.src1.vregId);
                using Facts = std::map<uint32_t, bool>;
                auto isTerm = [](Op op) {
                    return op == Op::BR || op == Op::BR_COND || op == Op::BR_INDIRECT || op == Op::SWITCH ||
                           op == Op::RET || op == Op::RET_VOID;
                };
                auto kill = [&](Facts& f, const Inst& x) {
                    if (x.op == Op::ASM_INLINE) { f.clear(); return; }   // (it may name any local)
                    if (x.dest.isVreg()) f.erase(x.dest.vregId);
                    if ((x.op == Op::STORE || x.op == Op::STORE_ZP) && x.src2.isVreg()) f.erase(x.src2.vregId);
                };
                // successors with the fact each edge adds
                struct Edge { size_t to; bool hasFact; uint32_t v; bool truth; };
                std::vector<std::vector<Edge>> succ(nb);
                for (size_t i = 0; i < nb; i++) {
                    const auto& insts = fn.blocks[i].insts;
                    const Inst* t = insts.empty() || !isTerm(insts.back().op) ? nullptr : &insts.back();
                    auto add = [&](const std::string& l, bool hf, uint32_t v, bool tr) {
                        auto it = at.find(l);
                        if (it != at.end()) succ[i].push_back({it->second, hf, v, tr});
                    };
                    if (!t) { if (i + 1 < nb) succ[i].push_back({i + 1, false, 0, false}); continue; }
                    if (t->op == Op::BR) add(t->src1.name, false, 0, false);
                    else if (t->op == Op::BR_COND) {
                        bool hf = t->src1.isVreg() && !mem.count(t->src1.vregId) && t->dest.name != t->src2.name;
                        uint32_t v = t->src1.isVreg() ? t->src1.vregId : 0;
                        add(t->dest.name, hf, v, true);
                        add(t->src2.name, hf, v, false);
                    } else if (t->op == Op::SWITCH) {
                        // the default and the cases: no facts
                        add(t->src2.name, false, 0, false);
                        for (const auto& sc : t->switchCases) add(sc.second, false, 0, false);
                    }
                }
                bool opaque = false;   // a terminator whose targets are not all known
                for (const auto& b : fn.blocks)
                    if (!b.insts.empty() && b.insts.back().op == Op::BR_INDIRECT) opaque = true;
                if (!opaque && nb > 1) {
                    std::vector<std::optional<Facts>> in(nb);
                    in[0] = Facts{};
                    for (size_t i = 0; i < nb; i++) if (taken.count(fn.blocks[i].label)) in[i] = Facts{};
                    bool moved = true;
                    for (int round = 0; moved && round < 32; round++) {
                        moved = false;
                        for (size_t i = 0; i < nb; i++) {
                            if (!in[i]) continue;
                            Facts out = *in[i];
                            for (const auto& x : fn.blocks[i].insts) kill(out, x);
                            for (const auto& e : succ[i]) {
                                Facts f = out;
                                if (e.hasFact) f[e.v] = e.truth;
                                if (taken.count(fn.blocks[e.to].label) || e.to == 0) continue;
                                if (!in[e.to]) { in[e.to] = f; moved = true; continue; }
                                Facts& d = *in[e.to];
                                for (auto it = d.begin(); it != d.end();) {
                                    auto g = f.find(it->first);
                                    if (g == f.end() || g->second != it->second) { it = d.erase(it); moved = true; }
                                    else ++it;
                                }
                            }
                        }
                    }
                    for (size_t i = 0; i < nb; i++) {
                        auto& insts = fn.blocks[i].insts;
                        if (!in[i] || insts.empty() || insts.back().op != Op::BR_COND) continue;
                        Facts f = *in[i];
                        for (size_t k = 0; k + 1 < insts.size(); k++) kill(f, insts[k]);
                        Inst& br = insts.back();
                        if (!br.src1.isVreg()) continue;
                        auto it = f.find(br.src1.vregId);
                        if (it == f.end()) continue;
                        std::string target = it->second ? br.dest.name : br.src2.name;
                        trace("branch", fn.name, "test of v" + std::to_string(br.src1.vregId) + " in " +
                              fn.blocks[i].label + " known: br " + target, br.loc);
                        br.op = Op::BR;
                        br.src1 = Operand::label(target);
                        br.src2 = Operand::none();
                        br.dest = Operand::none();
                        changed = true;
                    }
                }
            }

            // Pass 2a: a conditional branch on a constant (if (0) { ... } kept
            // for a case label inside it) becomes a plain branch
            for (auto& blk : fn.blocks) {
                if (blk.insts.empty() || blk.insts.back().op != Op::BR_COND) continue;
                auto& br = blk.insts.back();
                bool known = false;
                int64_t v = 0;
                if (br.src1.isImm()) { known = true; v = br.src1.immVal; }
                else if (br.src1.isVreg()) {
                    for (size_t k = blk.insts.size() - 1; k-- > 0;) {
                        const auto& d = blk.insts[k];
                        if (d.dest.isVreg() && d.dest.vregId == br.src1.vregId) {
                            if (d.op == Op::CONST && d.src1.isImm()) { known = true; v = d.src1.immVal; }
                            break;
                        }
                    }
                }
                if (!known || br.dest.name.empty() || br.src2.name.empty()) continue;
                std::string target = v ? br.dest.name : br.src2.name;
                trace("branch", fn.name, "constant condition: br " + target, br.loc);
                Inst b;
                b.op = Op::BR;
                b.src1 = Operand::label(target);
                b.loc = br.loc;
                br = b;
                changed = true;
            }

            // Pass 2b: Remove redundant BR to next block (fallthrough)
            for (size_t bi = 0; bi + 1 < fn.blocks.size(); bi++) {
                auto& insts = fn.blocks[bi].insts;
                if (!insts.empty() && insts.back().op == Op::BR &&
                    insts.back().src1.name == fn.blocks[bi + 1].label) {
                    insts.pop_back();
                    changed = true;
                }
            }

            // Pass 3: Remove unreachable blocks (no predecessors except entry)
            if (fn.blocks.size() > 1) {
                // Collect all referenced labels
                std::set<std::string> referenced;
                referenced.insert(fn.blocks[0].label); // entry is always reachable
                // A label whose address is taken can be reached by goto *
                referenced.insert(fn.addressTakenLabels.begin(), fn.addressTakenLabels.end());

                for (size_t i = 0; i < fn.blocks.size(); i++) {
                    auto& block = fn.blocks[i];
                    bool hasTerminator = false;
                    for (auto& inst : block.insts) {
                        if (inst.op == Op::ADDR_LABEL) referenced.insert(inst.src1.name);
                        if (inst.op == Op::BR) {
                            referenced.insert(inst.src1.name);
                            hasTerminator = true;
                        } else if (inst.op == Op::BR_COND) {
                            referenced.insert(inst.dest.name);
                            referenced.insert(inst.src2.name);
                            hasTerminator = true;
                        } else if (inst.op == Op::SWITCH) {
                            referenced.insert(inst.src2.name);
                            for (auto& sc : inst.switchCases)
                                referenced.insert(sc.second);
                            hasTerminator = true;
                        } else if (inst.op == Op::RET || inst.op == Op::RET_VOID) {
                            hasTerminator = true;
                        }
                    }
                    // Fallthrough: if no terminator, the next block is reachable
                    if (!hasTerminator && i + 1 < fn.blocks.size()) {
                        referenced.insert(fn.blocks[i + 1].label);
                    }
                }

                // Remove blocks that aren't referenced
                auto it = fn.blocks.begin();
                while (it != fn.blocks.end()) {
                    if (!referenced.count(it->label)) {
                        trace("branch", fn.name, "dead block " + it->label + " removed");
                        it = fn.blocks.erase(it);
                        changed = true;
                    } else {
                        ++it;
                    }
                }
            }
        }
    }
}

// ============================================================================
// LICM — Loop-Invariant Code Motion
// ============================================================================

namespace {

// Collect branch targets from a block's terminator instruction
void collectSuccessors(const Block& block, std::vector<std::string>& succs) {
    for (const auto& inst : block.insts) {
        switch (inst.op) {
            case Op::BR:
                if (!inst.src1.name.empty())
                    succs.push_back(inst.src1.name);
                break;
            case Op::BR_COND:
                // BR_COND: src1=condition, dest=true label, src2=false label
                if (!inst.dest.name.empty())
                    succs.push_back(inst.dest.name);   // true label
                if (!inst.src2.name.empty())
                    succs.push_back(inst.src2.name);   // false label
                break;
            case Op::SWITCH:
                if (!inst.src2.name.empty())
                    succs.push_back(inst.src2.name);  // default label
                for (const auto& sc : inst.switchCases)
                    succs.push_back(sc.second);
                break;
            case Op::BR_INDIRECT:
                // Computed goto — target unknown at compile time; no successors to track
                break;
            default:
                break;
        }
    }
}

// Check if an instruction is pure (no side effects) and safe to hoist
bool isHoistCandidate(Op op) {
    switch (op) {
        // CONST and FCONST are marked as hoist candidates for invariance
        // analysis (so dependents can be hoisted), but are DUPLICATED not MOVED
        // during the actual hoisting phase to avoid wasting ZP registers.
        case Op::CONST:
        case Op::FCONST:
        case Op::ADD: case Op::SUB: case Op::MUL: case Op::DIV: case Op::MOD:
        case Op::MUL_U: case Op::DIV_U: case Op::MOD_U:
        case Op::AND: case Op::OR: case Op::XOR:
        case Op::LSL: case Op::LSR: case Op::ASR:
        case Op::SHL: case Op::SHR:
        case Op::NEG: case Op::NOT:
        case Op::CMP_EQ: case Op::CMP_NE:
        case Op::CMP_LT: case Op::CMP_LE: case Op::CMP_GT: case Op::CMP_GE:
        case Op::CMP_LTU: case Op::CMP_LEU: case Op::CMP_GTU: case Op::CMP_GEU:
        case Op::SEXT: case Op::ZEXT: case Op::TRUNC:
        case Op::ADDR_GLOBAL: case Op::ADDR_LOCAL: case Op::ADDR_LABEL:
        case Op::ADDR_ELEM:
        case Op::COPY:
        case Op::FNEG:
            return true;
        default:
            return false;
    }
}

} // namespace

std::set<size_t> loopBlocks(const Function& fn) {
    std::set<size_t> in;
    std::map<std::string, size_t> blockIndex;
    for (size_t i = 0; i < fn.blocks.size(); i++) blockIndex[fn.blocks[i].label] = i;
    std::vector<std::vector<size_t>> succs(fn.blocks.size()), preds(fn.blocks.size());
    for (size_t i = 0; i < fn.blocks.size(); i++) {
        std::vector<std::string> targets;
        collectSuccessors(fn.blocks[i], targets);
        bool terminated = false;
        for (const auto& inst : fn.blocks[i].insts)
            if (inst.op == Op::BR || inst.op == Op::BR_INDIRECT || inst.op == Op::SWITCH ||
                inst.op == Op::RET || inst.op == Op::RET_VOID) terminated = true;
        if (!terminated && i + 1 < fn.blocks.size()) { succs[i].push_back(i + 1); preds[i + 1].push_back(i); }
        for (const auto& t : targets) {
            auto it = blockIndex.find(t);
            if (it != blockIndex.end()) { succs[i].push_back(it->second); preds[it->second].push_back(i); }
        }
    }
    for (size_t tail = 0; tail < fn.blocks.size(); tail++)
        for (size_t header : succs[tail]) {
            if (header > tail) continue;
            // the blocks that reach the back edge's source without passing its header
            std::vector<size_t> work{tail};
            std::set<size_t> loop{header, tail};
            while (!work.empty()) {
                size_t b = work.back();
                work.pop_back();
                if (b == header) continue;
                for (size_t p : preds[b])
                    if (loop.insert(p).second) work.push_back(p);
            }
            in.insert(loop.begin(), loop.end());
        }
    return in;
}

// ============================================================================
// Loop strength reduction
// ============================================================================

// An element address g[v] whose element size is not a power of 2, or v * K,
// in a loop where the variable v changes only by v = v ± c: a new variable
// w holds it, set once before the loop (the multiply there) and stepped by
// c * size right after v's update, the uses read w. v is a 16-bit local
// whose address is never taken (its value wraps as w's does); the base a
// global or a value the loop does not change. Loops innermost first.
void optimizeLoopStrengthReduction(Module& mod) {
    for (auto& fn : mod.functions) {
        if (!runOn(fn, "LoopStrengthReduction") || fn.blocks.size() < 2 || fn.staticLinkVreg >= 0) continue;
        bool asmIn = false;
        for (const auto& blk : fn.blocks)
            for (const auto& in : blk.insts) if (in.op == Op::ASM_INLINE) asmIn = true;
        if (asmIn) continue;

        // the CFG (as loopBlocks)
        size_t nb = fn.blocks.size();
        std::map<std::string, size_t> blockIndex;
        for (size_t i = 0; i < nb; i++) blockIndex[fn.blocks[i].label] = i;
        std::vector<std::vector<size_t>> succs(nb), preds(nb);
        for (size_t i = 0; i < nb; i++) {
            std::vector<std::string> targets;
            collectSuccessors(fn.blocks[i], targets);
            bool terminated = false;
            for (const auto& inst : fn.blocks[i].insts)
                if (inst.op == Op::BR || inst.op == Op::BR_INDIRECT || inst.op == Op::SWITCH ||
                    inst.op == Op::RET || inst.op == Op::RET_VOID) terminated = true;
            if (!terminated && i + 1 < nb) { succs[i].push_back(i + 1); preds[i + 1].push_back(i); }
            for (const auto& t : targets) {
                auto it = blockIndex.find(t);
                if (it != blockIndex.end()) { succs[i].push_back(it->second); preds[it->second].push_back(i); }
            }
        }
        // natural loops, one per header (the back edges' sources merged)
        std::map<size_t, std::set<size_t>> loops;
        for (size_t tail = 0; tail < nb; tail++)
            for (size_t header : succs[tail]) {
                if (header > tail) continue;
                std::vector<size_t> work{tail};
                auto& loop = loops[header];
                loop.insert(header);
                loop.insert(tail);
                while (!work.empty()) {
                    size_t b = work.back();
                    work.pop_back();
                    if (b == header) continue;
                    for (size_t p : preds[b])
                        if (loop.insert(p).second) work.push_back(p);
                }
            }
        std::vector<std::pair<size_t, std::set<size_t>>> order(loops.begin(), loops.end());
        std::sort(order.begin(), order.end(), [](const auto& a, const auto& b) { return a.second.size() < b.second.size(); });

        std::map<uint32_t, int> defCount;
        std::set<uint32_t> addrTaken;
        for (const auto& blk : fn.blocks)
            for (const auto& in : blk.insts) {
                if (in.dest.isVreg()) defCount[in.dest.vregId]++;
                if (in.op == Op::ADDR_LOCAL && in.src1.isVreg()) addrTaken.insert(in.src1.vregId);
            }
        auto isPow2 = [](int64_t k) { return k > 0 && !(k & (k - 1)); };

        for (const auto& [header, L] : order) {
            // the variables stored in the loop: how often, and each one's update
            std::map<uint32_t, int> stores;
            std::map<uint32_t, std::pair<size_t, size_t>> storeAt;   // block, index
            for (size_t bi : L)
                for (size_t ii = 0; ii < fn.blocks[bi].insts.size(); ii++) {
                    const auto& in = fn.blocks[bi].insts[ii];
                    if ((in.op == Op::STORE || in.op == Op::STORE_ZP) && in.src2.isVreg() && in.args.empty()) {
                        stores[in.src2.vregId]++;
                        storeAt[in.src2.vregId] = {bi, ii};
                    }
                }
            // the header goes into the loop only when v < N (an unsigned byte
            // compare with a constant), else out of it
            auto guardedBelow = [&](uint32_t v) {
                const auto& hi = fn.blocks[header].insts;
                for (size_t k = 0; k < hi.size(); k++) {
                    const auto& br = hi[k];
                    if (br.op != Op::BR_COND || !br.src1.isVreg()) continue;
                    // (BR_COND: dest the true label, src2 the false one)
                    bool inT = blockIndex.count(br.dest.name) && L.count(blockIndex[br.dest.name]);
                    bool inF = blockIndex.count(br.src2.name) && L.count(blockIndex[br.src2.name]);
                    if (!inT || inF) return false;
                    for (size_t j = 0; j < k; j++) {
                        const auto& c = hi[j];
                        if (c.dest.isVreg() && c.dest.vregId == br.src1.vregId)
                            return defCount[c.dest.vregId] == 1 && c.op == Op::CMP_LTU && c.src1.isVreg() &&
                                   c.src1.vregId == v && c.src2.isImm() && c.src2.immVal >= 0 && c.src2.immVal <= 255;
                    }
                    return false;
                }
                return false;
            };
            // v = v ± c, the only change of v in the loop: c, else 0
            auto stepOf = [&](uint32_t v) -> int64_t {
                if (!fn.localSlotVregs.count(v) || fn.memoryVregs.count(v) || addrTaken.count(v)) return 0;
                auto vt = fn.vregTypes.find(v);
                if (vt == fn.vregTypes.end() || (vt->second != Type::I16 && vt->second != Type::I8)) return 0;
                bool byte = vt->second == Type::I8;
                // (a byte: v++ only, the loop entered at its header only while
                // v < N, so v never wraps where the loop uses it)
                if (byte && !guardedBelow(v)) return 0;
                if (stores[v] != 1) return 0;
                auto [bi, ii] = storeAt[v];
                const auto& st = fn.blocks[bi].insts[ii];
                if (st.resultType != vt->second || !st.src1.isVreg()) return 0;
                uint32_t t = st.src1.vregId;
                if (defCount[t] != 1) return 0;
                // v itself, or its copy (x++ keeps the old value) made before in the block
                auto isV = [&](const Operand& o, size_t before) {
                    if (!o.isVreg()) return false;
                    if (o.vregId == v) return true;
                    if (defCount[o.vregId] != 1) return false;
                    for (size_t k = 0; k < before; k++) {
                        const auto& d = fn.blocks[bi].insts[k];
                        if (d.dest.isVreg() && d.dest.vregId == o.vregId)
                            return (d.op == Op::COPY || d.op == Op::DEREF) && d.src1.isVreg() && d.src1.vregId == v;
                    }
                    return false;
                };
                for (size_t k = 0; k < ii; k++) {
                    const auto& d = fn.blocks[bi].insts[k];
                    if (!d.dest.isVreg() || d.dest.vregId != t) continue;
                    if ((d.op != Op::ADD && d.op != Op::SUB) || !isV(d.src1, k) ||
                        !d.src2.isImm() || d.resultType != vt->second) return 0;
                    int64_t c = d.op == Op::ADD ? d.src2.immVal : -d.src2.immVal;
                    if (byte) return (c & 0xFF) == 1 ? 1 : 0;
                    return (c & 0xFFFF) ? c : 0;
                }
                return 0;
            };
            // a base the loop does not change: a global or a temporary (a
            // local's slot as ADDR_ELEM's base may be the local array itself,
            // not its value: left alone)
            auto invariant = [&](const Operand& o) {
                if (o.kind == OperandKind::GLOBAL) return true;
                if (!o.isVreg() || fn.localSlotVregs.count(o.vregId) || fn.memoryVregs.count(o.vregId)) return false;
                if (defCount[o.vregId] != 1) return false;
                for (size_t bi : L)
                    for (const auto& in : fn.blocks[bi].insts)
                        if (in.dest.isVreg() && in.dest.vregId == o.vregId) return false;
                return true;
            };
            // the variable an index or factor is: v itself, or zext(v) of a byte v
            auto varOf = [&](const Operand& o, uint32_t& v, bool& widened) {
                if (!o.isVreg()) return false;
                widened = false;
                if (fn.localSlotVregs.count(o.vregId)) { v = o.vregId; return true; }
                if (defCount[o.vregId] != 1) return false;
                for (size_t bi : L)
                    for (const auto& d : fn.blocks[bi].insts)
                        if (d.dest.isVreg() && d.dest.vregId == o.vregId) {
                            if (d.op != Op::ZEXT || !d.src1.isVreg() || !fn.localSlotVregs.count(d.src1.vregId) ||
                                d.resultType != Type::I16) return false;
                            auto vt = fn.vregTypes.find(d.src1.vregId);
                            if (vt == fn.vregTypes.end() || vt->second != Type::I8) return false;
                            v = d.src1.vregId;
                            widened = true;
                            return true;
                        }
                return false;
            };
            // a constant factor: an immediate or a CONST
            auto constOf = [&](const Operand& o, int64_t& k) {
                if (o.isImm()) { k = o.immVal; return true; }
                if (!o.isVreg() || defCount[o.vregId] != 1) return false;
                for (const auto& blk : fn.blocks)
                    for (const auto& d : blk.insts)
                        if (d.dest.isVreg() && d.dest.vregId == o.vregId) {
                            if (d.op != Op::CONST || !d.src1.isImm()) return false;
                            k = d.src1.immVal;
                            return true;
                        }
                return false;
            };
            struct Group { uint32_t w; int64_t inc; Inst init; int64_t mul; uint32_t v; bool widened; };
            std::vector<Group> groups;
            std::map<std::string, size_t> groupOf;
            std::map<uint32_t, std::vector<size_t>> groupsOfVar;
            for (size_t bi : L)
                for (auto& in : fn.blocks[bi].insts) {
                    if (!in.dest.isVreg()) continue;
                    uint32_t v = 0;
                    bool widened = false;
                    int64_t k = 0;
                    std::string key;
                    bool elem = in.op == Op::ADDR_ELEM;
                    if (elem && !in.args.empty() && in.args[0].isImm()) {
                        k = in.args[0].immVal;
                        if (k <= 1 || isPow2(k) || !invariant(in.src1) || !varOf(in.src2, v, widened)) continue;
                        key = "e " + (in.src1.kind == OperandKind::GLOBAL ? in.src1.name : "%" + std::to_string(in.src1.vregId)) +
                              " " + std::to_string(in.elemOffset) + " " + std::to_string(k);
                    } else if ((in.op == Op::MUL || in.op == Op::MUL_U) && in.resultType == Type::I16) {
                        if (constOf(in.src2, k) && varOf(in.src1, v, widened)) {}
                        else if (constOf(in.src1, k) && varOf(in.src2, v, widened)) {}
                        else continue;
                        k &= 0xFFFF;
                        if (k <= 1 || isPow2(k)) continue;
                        key = "m " + std::to_string(k);
                    } else continue;
                    // (a 16-bit v as itself; a byte v only through its zero extension)
                    auto vt = fn.vregTypes.find(v);
                    if (vt == fn.vregTypes.end() || (vt->second == Type::I8) != widened) continue;
                    int64_t c = stepOf(v);
                    if (!c) continue;
                    key += " %" + std::to_string(v);
                    auto g = groupOf.find(key);
                    size_t gi;
                    Type t = elem ? Type::PTR : Type::I16;
                    if (g == groupOf.end()) {
                        uint32_t w = fn.allocVreg();
                        fn.localSlotVregs.insert(w);
                        fn.vregTypes[w] = t;
                        std::string name = "__lsr_" + std::to_string(w);
                        fn.localNames[name] = w;
                        fn.localNamesOrder.push_back(name);
                        // the value before the loop: the same computation on v there
                        Inst init = in;
                        init.args.clear();
                        if (elem) {
                            init.args.push_back(Operand::imm(k, Type::I16));
                        } else {
                            init.op = Op::MUL_U;
                            init.resultType = Type::I16;
                            init.src2 = Operand::imm(k, Type::I16);
                        }
                        groups.push_back({w, (c * k) & 0xFFFF, init, k, v, widened});
                        gi = groups.size() - 1;
                        groupOf[key] = gi;
                        groupsOfVar[v].push_back(gi);
                        trace("lsr", fn.name, "v" + std::to_string(in.dest.vregId) + " = " +
                              (elem ? "ADDR_ELEM" : "MUL") + " *" + std::to_string(k) +
                              " → variable v" + std::to_string(w) + " stepped by " + std::to_string(groups[gi].inc), in.loc);
                    } else gi = g->second;
                    in.op = Op::COPY;
                    in.resultType = t;
                    in.src1 = Operand::vreg(groups[gi].w, t);
                    in.src2 = Operand::none();
                    in.args.clear();
                    in.elemOffset = 0;
                }
            if (groups.empty()) continue;
            std::set<uint32_t> groupVar;
            for (const auto& gv : groupsOfVar) groupVar.insert(gv.first);
            // each w stepped right after its variable's update
            for (const auto& [v, gis] : groupsOfVar) {
                auto [bi, ii] = storeAt[v];
                auto& insts = fn.blocks[bi].insts;
                std::vector<Inst> add;
                for (size_t gi : gis) {
                    const auto& g = groups[gi];
                    Type t = fn.vregTypes[g.w];
                    uint32_t tmp = fn.allocVreg();
                    fn.vregTypes[tmp] = t;
                    Inst a;
                    a.op = Op::ADD;
                    a.resultType = t;
                    a.dest = Operand::vreg(tmp, t);
                    a.src1 = Operand::vreg(g.w, t);
                    a.src2 = Operand::imm(g.inc, Type::I16);
                    a.loc = insts[ii].loc;
                    Inst s;
                    s.op = Op::STORE;
                    s.resultType = t;
                    s.src1 = Operand::vreg(tmp, t);
                    s.src2 = Operand::vreg(g.w, t);
                    s.loc = a.loc;
                    add.push_back(a);
                    add.push_back(s);
                }
                insts.insert(insts.begin() + ii + 1, add.begin(), add.end());
            }
            // each w set before the loop: at the end of each block entering it
            for (size_t p : preds[header]) {
                if (L.count(p)) continue;
                auto& insts = fn.blocks[p].insts;
                size_t at = insts.size();
                if (at && (insts[at - 1].op == Op::BR || insts[at - 1].op == Op::BR_COND ||
                           insts[at - 1].op == Op::SWITCH || insts[at - 1].op == Op::BR_INDIRECT)) {
                    at--;
                    // (a branch's compare kept next to it)
                    const auto& br = insts[at];
                    if (br.op == Op::BR_COND && at && br.src1.isVreg() && insts[at - 1].dest.isVreg() &&
                        insts[at - 1].dest.vregId == br.src1.vregId) at--;
                }
                // v's value here when the block sets it to a constant (for (v = 0; ...)):
                // the initial address or product a constant too
                bool known = false;
                int64_t kv = 0;
                for (size_t k = at; k-- > 0;) {
                    const auto& x = insts[k];
                    if ((x.op == Op::STORE || x.op == Op::STORE_ZP) && x.src2.isVreg() && groupVar.count(x.src2.vregId)) {
                        // (the last store to one of the variables: only when it is the one)
                        if (x.src1.isImm()) { known = true; kv = x.src1.immVal; }
                        else if (x.src1.isVreg() && defCount[x.src1.vregId] == 1)
                            for (size_t j = 0; j < k; j++)
                                if (insts[j].dest.isVreg() && insts[j].dest.vregId == x.src1.vregId &&
                                    insts[j].op == Op::CONST) { known = true; kv = insts[j].src1.immVal; }
                        if (known) known = groupVar.size() == 1;
                        break;
                    }
                    if (x.op == Op::CALL || x.op == Op::CALL_VOID || x.op == Op::CALL_INDIRECT) break;
                }
                std::vector<Inst> init;
                for (const auto& g : groups) {
                    Type t = fn.vregTypes[g.w];
                    uint32_t tmp = fn.allocVreg();
                    fn.vregTypes[tmp] = t;
                    Inst i0 = g.init;
                    i0.dest = Operand::vreg(tmp, t);
                    Type vt = fn.vregTypes[g.v];
                    if (g.widened) kv &= 0xFF;
                    Operand idx = Operand::vreg(g.v, vt);
                    if (!known && g.widened) {
                        uint32_t z = fn.allocVreg();
                        fn.vregTypes[z] = Type::I16;
                        Inst ze;
                        ze.op = Op::ZEXT;
                        ze.resultType = Type::I16;
                        ze.dest = Operand::vreg(z, Type::I16);
                        ze.src1 = idx;
                        ze.loc = i0.loc;
                        init.push_back(ze);
                        idx = ze.dest;
                    }
                    if (known && i0.op == Op::ADDR_ELEM) {
                        i0.src2 = Operand::imm(kv & 0xFFFF, Type::I16);
                    } else if (known) {
                        i0.op = Op::CONST;
                        i0.src1 = Operand::imm((kv * g.mul) & 0xFFFF, Type::I16);
                        i0.src2 = Operand::none();
                    } else if (i0.op == Op::ADDR_ELEM) {
                        i0.src2 = idx;
                    } else {
                        i0.src1 = idx;
                    }
                    Inst s;
                    s.op = Op::STORE;
                    s.resultType = t;
                    s.src1 = Operand::vreg(tmp, t);
                    s.src2 = Operand::vreg(g.w, t);
                    s.loc = i0.loc;
                    init.push_back(i0);
                    init.push_back(s);
                }
                insts.insert(insts.begin() + at, init.begin(), init.end());
            }
            // (block contents changed: the counts again for the next loop)
            defCount.clear();
            for (const auto& blk : fn.blocks)
                for (const auto& in : blk.insts) if (in.dest.isVreg()) defCount[in.dest.vregId]++;
        }
    }
}

void optimizeLICM(Module& mod) {
    for (auto& fn : mod.functions) {
        if (!runOn(fn, "LICM")) continue;
        // Restart loop: after inserting a pre-header block, indices shift,
        // so we rebuild the CFG and scan for the next back-edge.
        bool restart = true;
        int maxPasses = 32; // safety limit for deeply nested loops
        while (restart && --maxPasses > 0) {
            restart = false;
            if (fn.blocks.size() < 2) break;

        // Build block index: label → position in fn.blocks
        std::map<std::string, size_t> blockIndex;
        for (size_t i = 0; i < fn.blocks.size(); i++) {
            blockIndex[fn.blocks[i].label] = i;
        }

        // Build successor/predecessor maps
        std::vector<std::vector<size_t>> succs(fn.blocks.size());
        std::vector<std::vector<size_t>> preds(fn.blocks.size());
        for (size_t i = 0; i < fn.blocks.size(); i++) {
            std::vector<std::string> targets;
            collectSuccessors(fn.blocks[i], targets);

            // If the block has no explicit branch/return, it falls through
            // to the next block (implicit edge)
            if (targets.empty() && i + 1 < fn.blocks.size()) {
                bool hasTerminator = false;
                for (const auto& inst : fn.blocks[i].insts) {
                    if (inst.op == Op::BR || inst.op == Op::BR_COND ||
                        inst.op == Op::BR_INDIRECT || inst.op == Op::SWITCH ||
                        inst.op == Op::RET || inst.op == Op::RET_VOID) {
                        hasTerminator = true;
                        break;
                    }
                }
                if (!hasTerminator) {
                    succs[i].push_back(i + 1);
                    preds[i + 1].push_back(i);
                }
            }

            for (const auto& t : targets) {
                auto it = blockIndex.find(t);
                if (it != blockIndex.end()) {
                    succs[i].push_back(it->second);
                    preds[it->second].push_back(i);
                }
            }
        }

        // Find back-edges: edge i→j where j <= i (target is at or before source
        // in block order; works for structured code from cc45)
        struct BackEdge { size_t from; size_t to; };
        std::vector<BackEdge> backEdges;
        for (size_t i = 0; i < fn.blocks.size(); i++) {
            for (size_t s : succs[i]) {
                if (s <= i) {
                    backEdges.push_back({i, s});
                }
            }
        }

        if (backEdges.empty()) break;

        // Process back-edges. If we hoist anything, restart.
        for (const auto& be : backEdges) {
            size_t header = be.to;
            size_t tail = be.from;

            // Skip pre-header blocks we already inserted
            if (fn.blocks[header].label.find("_ph") != std::string::npos) continue;

            std::set<size_t> loopBlocks;
            loopBlocks.insert(header);

            if (header != tail) {
                // Reverse DFS from tail, collecting blocks that reach tail
                // without passing through header
                std::vector<size_t> worklist;
                loopBlocks.insert(tail);
                worklist.push_back(tail);

                while (!worklist.empty()) {
                    size_t b = worklist.back();
                    worklist.pop_back();
                    for (size_t p : preds[b]) {
                        if (loopBlocks.find(p) == loopBlocks.end()) {
                            loopBlocks.insert(p);
                            worklist.push_back(p);
                        }
                    }
                }
            }

            if (loopBlocks.size() < 2) continue; // trivial loop, skip

            // Collect all vregs defined inside the loop, and
            // local slot vregs that are STORED TO inside the loop.
            // Local slots are memory locations (stack variables). Even though the
            // slot vreg (address) is defined outside the loop, reads from mutated
            // slots are NOT loop-invariant.
            std::set<uint32_t> loopDefinedVregs;
            std::set<uint32_t> loopMutatedSlots;
            bool loopWritesMemory = false;   // a call, asm or store through a pointer
            for (size_t bi : loopBlocks) {
                for (const auto& inst : fn.blocks[bi].insts) {
                    if (inst.dest.isVreg()) {
                        loopDefinedVregs.insert(inst.dest.vregId);
                    }
                    // STORE to a local slot vreg mutates that variable
                    if ((inst.op == Op::STORE || inst.op == Op::STORE_ZP) &&
                        inst.src2.isVreg() && fn.localSlotVregs.count(inst.src2.vregId)) {
                        loopMutatedSlots.insert(inst.src2.vregId);
                    } else if ((inst.op == Op::STORE || inst.op == Op::STORE_ZP ||
                                inst.op == Op::CALL || inst.op == Op::CALL_VOID ||
                                inst.op == Op::CALL_INDIRECT || inst.op == Op::ASM_INLINE ||
                                inst.op == Op::CPU_REG_WRITE) && !purityOf(mod, inst)) {
                        loopWritesMemory = true;
                    }
                }
            }
            // A variable whose address is taken changes through such a write
            if (loopWritesMemory) {
                for (const auto& blk : fn.blocks)
                    for (const auto& inst : blk.insts)
                        if (inst.op == Op::ADDR_LOCAL && inst.src1.isVreg())
                            loopMutatedSlots.insert(inst.src1.vregId);
                for (uint32_t v : fn.memoryVregs) loopMutatedSlots.insert(v);
            }
            // Vregs are not SSA: only an instruction that is its destination's
            // one definition can move
            std::map<uint32_t, int> defCount;
            for (const auto& blk : fn.blocks)
                for (const auto& inst : blk.insts)
                    if (inst.dest.isVreg()) defCount[inst.dest.vregId]++;
            // Loads through restrict parameters (Issue #77): what one points to
            // changes in the loop only by a store through it (or a pointer
            // made from it), through an unknown pointer, a call or asm --
            // not by stores into globals or through other restrict pointers
            const std::set<uint32_t> restrictSlots = restrictParams(fn);
            std::map<uint32_t, const Inst*> defOf;
            for (const auto& blk : fn.blocks)
                for (const auto& inst : blk.insts)
                    if (inst.dest.isVreg()) defOf[inst.dest.vregId] = &inst;
            std::set<long> restrictWritten;   // their objects the loop writes (-1: any)
            if (!restrictSlots.empty())
                for (size_t bi : loopBlocks)
                    for (const auto& inst : fn.blocks[bi].insts) {
                        if (purityOf(mod, inst)) continue;
                        bool st = inst.op == Op::STORE || inst.op == Op::STORE_ZP;
                        if (st && inst.src2.isVreg() && fn.localSlotVregs.count(inst.src2.vregId) &&
                            !fn.memoryVregs.count(inst.src2.vregId))
                            continue;   // (a variable)
                        if (st) {
                            long r = addressRoot(inst.src2, restrictSlots, defCount, defOf);
                            if (r != -2) restrictWritten.insert(r);
                        } else if (inst.op == Op::CALL || inst.op == Op::CALL_VOID || inst.op == Op::CALL_INDIRECT ||
                                   inst.op == Op::ASM_INLINE || inst.op == Op::CPU_REG_WRITE || inst.op == Op::BR_INDIRECT) {
                            restrictWritten.insert(-1);
                        }
                    }
            auto restrictLoad = [&](const Inst& inst) {
                if (inst.op != Op::LOAD || !inst.args.empty() || restrictSlots.empty() || restrictWritten.count(-1))
                    return false;
                long r = addressRoot(inst.src1, restrictSlots, defCount, defOf);
                return r >= 0 && !restrictWritten.count(r);
            };

            // Iteratively find loop-invariant instructions.
            // An instruction is invariant if:
            //   1. It is a pure (side-effect-free) operation
            //   2. All its source operands are either:
            //      - Constants/immediates/globals
            //      - Defined outside the loop (and not a mutated local slot)
            //      - Defined by another already-identified invariant instruction
            //   3. No source reads a local slot mutated inside the loop
            //   4. Its dest vreg is not a local slot or memory-pinned vreg
            std::set<uint32_t> invariantVregs;
            // vregs with one definition, a constant
            std::map<uint32_t, int64_t> loopConsts = singleDefConsts(fn);
            bool changed = true;
            while (changed) {
                changed = false;
                for (size_t bi : loopBlocks) {
                    for (const auto& inst : fn.blocks[bi].insts) {
                        if (!inst.dest.isVreg()) continue;
                        if (invariantVregs.count(inst.dest.vregId)) continue;
                        // A call of a const function, or of a pure one in a
                        // loop that writes no memory (Issue #76): only from
                        // the header, which runs whenever the loop is entered
                        // (one in a conditional block, or in a loop that
                        // runs no times, would then run when it did not)
                        const int purity = purityOf(mod, inst);
                        const bool pureCall = purity && inst.op == Op::CALL && bi == header &&
                                              (purity == 2 || !loopWritesMemory);
                        if (!isHoistCandidate(inst.op) && !pureCall && !restrictLoad(inst)) continue;
                        // A global's address (+ a constant) costs nothing where it
                        // is used (name+off): hoisted, it and the address arithmetic
                        // on it would be variables live across the loop (across its
                        // calls in memory, copied to zero page for each use)
                        if (inst.op == Op::ADDR_GLOBAL ||
                            ((inst.op == Op::ADD || inst.op == Op::SUB) && inst.src1.kind == OperandKind::GLOBAL &&
                             inst.src2.isImm()))
                            continue;
                        // Likewise anything computed from constants alone (a
                        // conversion of a constant): only an instruction with a
                        // variable operand (k * 7) is worth a variable
                        if (inst.op != Op::CONST && inst.op != Op::FCONST && !pureCall) {
                            bool hasVar = false;
                            auto isVar = [&](const Operand& o) {
                                return o.isVreg() && !loopConsts.count(o.vregId);
                            };
                            if (isVar(inst.src1) || isVar(inst.src2)) hasVar = true;
                            for (const auto& a : inst.args) if (isVar(a)) hasVar = true;
                            if (!hasVar) continue;
                        }

                        // Don't hoist local slot vregs (they represent stack addresses)
                        if (fn.localSlotVregs.count(inst.dest.vregId)) continue;
                        // Don't hoist memory-pinned vregs (volatile, address-taken)
                        if (fn.memoryVregs.count(inst.dest.vregId)) continue;
                        if (defCount[inst.dest.vregId] != 1) continue;

                        auto isInvariantOrExternal = [&](const Operand& op) -> bool {
                            if (op.isNone() || op.isImm()) return true;
                            if (op.kind == OperandKind::GLOBAL || op.kind == OperandKind::LABEL) return true;
                            if (op.isVreg()) {
                                // Local slot mutated inside the loop → value changes each iteration
                                if (loopMutatedSlots.count(op.vregId))
                                    return false;
                                // Defined outside loop?
                                if (loopDefinedVregs.find(op.vregId) == loopDefinedVregs.end())
                                    return true;
                                // Defined by another invariant instruction?
                                if (invariantVregs.count(op.vregId))
                                    return true;
                                return false;
                            }
                            return true;
                        };

                        if (isInvariantOrExternal(inst.src1) && isInvariantOrExternal(inst.src2)) {
                            // Also check args (for ADDR_ELEM etc.)
                            bool allArgsOk = true;
                            for (const auto& arg : inst.args) {
                                if (!isInvariantOrExternal(arg)) { allArgsOk = false; break; }
                            }
                            if (allArgsOk) {
                                invariantVregs.insert(inst.dest.vregId);
                                changed = true;
                            }
                        }
                    }
                }
            }

            if (invariantVregs.empty()) continue;

            // Collect invariant CONST definitions (for duplication, not movement).
            // CONST instructions stay in the loop body but are duplicated in the
            // pre-header so that hoisted dependents can reference them.
            // So is the zero extension of a variable (a char parameter used
            // as an int): read where it is used, it costs an ldx #0; hoisted,
            // a 16-bit copy live across the loop.
            std::map<uint32_t, Inst> constDefs;
            for (size_t bi : loopBlocks) {
                for (const auto& inst : fn.blocks[bi].insts) {
                    bool cheap = inst.op == Op::CONST || inst.op == Op::FCONST ||
                                 (inst.op == Op::ZEXT && inst.src1.isVreg() &&
                                  (fn.localSlotVregs.count(inst.src1.vregId) || inst.src1.vregId < fn.paramTypes.size()));
                    if (cheap && inst.dest.isVreg() && invariantVregs.count(inst.dest.vregId)) {
                        constDefs[inst.dest.vregId] = inst;
                    }
                }
            }

            // Remove CONST-only invariants from the hoist set — they stay in the loop
            std::set<uint32_t> nonConstInvariants;
            for (uint32_t v : invariantVregs) {
                if (!constDefs.count(v)) nonConstInvariants.insert(v);
            }

            if (nonConstInvariants.empty()) continue;

            for (uint32_t v : nonConstInvariants) {
                trace("licm", fn.name, "v" + std::to_string(v) + " hoisted to pre-header");
            }

            // Collect non-CONST invariant instructions (MOVE from loop body)
            std::vector<Inst> hoisted;
            for (size_t bi : loopBlocks) {
                auto& insts = fn.blocks[bi].insts;
                auto it = insts.begin();
                while (it != insts.end()) {
                    if (it->dest.isVreg() && nonConstInvariants.count(it->dest.vregId)) {
                        hoisted.push_back(std::move(*it));
                        it = insts.erase(it);
                    } else {
                        ++it;
                    }
                }
            }

            // For hoisted instructions that reference CONST vregs still in the
            // loop body, create fresh CONSTs in the pre-header with new vregs
            // to avoid double-definition of the same vreg.
            std::set<uint32_t> neededConsts;
            for (const auto& inst : hoisted) {
                auto checkOp = [&](const Operand& op) {
                    if (op.isVreg() && constDefs.count(op.vregId))
                        neededConsts.insert(op.vregId);
                };
                checkOp(inst.src1);
                checkOp(inst.src2);
                for (const auto& arg : inst.args) checkOp(arg);
            }

            // Allocate new vregs for duplicated CONSTs and build a rewrite map
            std::map<uint32_t, uint32_t> constRemap; // old vreg → new vreg
            std::vector<Inst> withConsts;
            for (uint32_t cv : neededConsts) {
                uint32_t newVreg = fn.allocVreg();
                constRemap[cv] = newVreg;
                Inst dup = constDefs[cv];
                dup.dest = Operand::vreg(newVreg, dup.dest.type);
                withConsts.push_back(dup);
            }

            // Rewrite operands in hoisted instructions to use new CONST vregs
            auto remapOp = [&](Operand& op) {
                if (op.isVreg()) {
                    auto it = constRemap.find(op.vregId);
                    if (it != constRemap.end())
                        op.vregId = it->second;
                }
            };
            for (auto& inst : hoisted) {
                remapOp(inst.src1);
                remapOp(inst.src2);
                for (auto& arg : inst.args) remapOp(arg);
            }

            withConsts.insert(withConsts.end(), hoisted.begin(), hoisted.end());
            hoisted = std::move(withConsts);

            // Check if a pre-header already exists (from a prior pass)
            std::string preheaderLabel = fn.blocks[header].label + "_ph";
            auto phIt = blockIndex.find(preheaderLabel);
            if (phIt != blockIndex.end()) {
                // Pre-header exists — insert hoisted instructions before its terminal BR
                auto& phBlock = fn.blocks[phIt->second];
                auto brIt = phBlock.insts.end();
                for (auto rit = phBlock.insts.begin(); rit != phBlock.insts.end(); ++rit) {
                    if (rit->op == Op::BR || rit->op == Op::BR_COND) {
                        brIt = rit;
                        break;
                    }
                }
                phBlock.insts.insert(brIt, hoisted.begin(), hoisted.end());
            } else {
                // Create a new pre-header block
                Block preheader;
                preheader.label = preheaderLabel;
                preheader.insts = std::move(hoisted);

                // Add branch from pre-header to header
                Inst brToHeader;
                brToHeader.op = Op::BR;
                brToHeader.src1 = Operand::label(fn.blocks[header].label);
                preheader.insts.push_back(brToHeader);

                // Redirect non-loop predecessors of header to the pre-header
                for (size_t pi : preds[header]) {
                    if (loopBlocks.count(pi)) continue;

                    auto& predBlock = fn.blocks[pi];
                    for (auto& inst : predBlock.insts) {
                        auto rewriteLabel = [&](Operand& op) {
                            if (op.kind == OperandKind::LABEL && op.name == fn.blocks[header].label)
                                op.name = preheaderLabel;
                        };
                        auto rewriteName = [&](std::string& name) {
                            if (name == fn.blocks[header].label)
                                name = preheaderLabel;
                        };

                        switch (inst.op) {
                            case Op::BR:
                                rewriteLabel(inst.src1);
                                break;
                            case Op::BR_COND:
                                rewriteName(inst.dest.name);
                                rewriteLabel(inst.src2);
                                break;
                            case Op::SWITCH:
                                rewriteLabel(inst.src2);
                                for (auto& sc : inst.switchCases)
                                    rewriteName(sc.second);
                                break;
                            default:
                                break;
                        }
                    }
                }

                // Insert pre-header just before the header
                fn.blocks.insert(fn.blocks.begin() + header, std::move(preheader));
            }

            // Note: hoisted vregs' liveness across the loop is handled by
            // VRegAllocator::extendLiveRangesAcrossLoops(), which extends
            // live ranges that overlap loop bodies to cover the entire loop.

                restart = true;
                break; // Indices shifted — restart CFG analysis
            }
        } // while (restart)
    }
}


// ============================================================================
// Loop unrolling (-floop-unrolling)
// ============================================================================
//
// Fully unrolls a counted for loop when the unrolled code is estimated to fit
// unrollMaxBytes. IRBuilder lowers `for (init; cond; inc) body` to
//
//   <pred>:        ... store <const>, %v ; br @for_condN
//   @for_condN:    <v compared with a constant> ; br_cond %c, @for_bodyN, @for_endN
//   @for_bodyN ... (the body's blocks)
//   @for_incN:     %x = deref %v ; %y = add/sub %x, <const> ; store %y, %v ; br @for_condN
//   @for_endN:
//
// with %v the loop variable's slot. The trip count is simulated from the
// constants. Each copy of the body gets its own labels and reads the
// variable's value for that iteration from a fresh CONST temp (so the other
// passes can fold indexes and conditions with it); `continue` jumps to the
// next copy, `break` to @for_endN. The variable gets its final value after
// the last copy (and, when the body can break out, at the start of each copy).
//
// Not unrolled: a body that assigns the variable, takes its address, contains
// asm, address-taken labels or labels reached from outside the loop; a
// variable in memory (volatile, address-taken); values that would wrap or
// cross the sign bit; more than 255 iterations.

int unrollMaxBytes = 128;

namespace {

struct ForLoop {
    size_t cond, body, inc, end;   // block indexes
    uint32_t var;                  // the loop variable's slot vreg
    std::vector<int64_t> values;   // the variable's value in each iteration
    int64_t finalValue;
    Type varType;
};

bool isConstOperand(const Function& fn, const Operand& o, int64_t& v,
                    const std::map<uint32_t, int>& defCount) {
    if (o.isImm()) { v = o.immVal; return true; }
    if (!o.isVreg()) return false;
    auto dc = defCount.find(o.vregId);
    if (dc == defCount.end() || dc->second != 1) return false;
    for (const auto& b : fn.blocks)
        for (const auto& i : b.insts)
            if (i.dest.isVreg() && i.dest.vregId == o.vregId) {
                if (i.op == Op::CONST && i.src1.isImm()) { v = i.src1.immVal; return true; }
                // an extension of a constant (the callers only accept small
                // non-negative bounds, for which zext/sext/copy keep the value)
                if (i.op == Op::ZEXT || i.op == Op::SEXT || i.op == Op::COPY)
                    return isConstOperand(fn, i.src1, v, defCount);
                return false;
            }
    return false;
}

bool matchForLoop(const Function& fn, size_t condIdx, const std::map<uint32_t, int>& defCount, ForLoop& L) {
    const std::string& cl = fn.blocks[condIdx].label;
    if (cl.compare(0, 8, "for_cond") != 0) return false;
    // (the labels share one counter: for_cond0, for_body1, for_inc2, for_end3)
    const Block& cb = fn.blocks[condIdx];
    if (cb.insts.empty() || cb.insts.back().op != Op::BR_COND) return false;
    const Inst& br = cb.insts.back();
    L.cond = condIdx;
    L.body = L.inc = L.end = SIZE_MAX;
    for (size_t k = condIdx + 1; k < fn.blocks.size(); k++) {
        if (fn.blocks[k].label == br.dest.name && L.body == SIZE_MAX) L.body = k;
        if (fn.blocks[k].label == br.src2.name) { L.end = k; break; }
    }
    if (L.body != condIdx + 1 || L.end == SIZE_MAX || L.end < L.body + 2) return false;
    L.inc = L.end - 1;
    if (fn.blocks[L.body].label.compare(0, 8, "for_body") != 0 ||
        fn.blocks[L.inc].label.compare(0, 7, "for_inc") != 0) return false;

    // Condition block: v (through deref/copy/zext/sext) compared with a constant
    std::map<uint32_t, uint32_t> fromVar;   // vreg -> the slot it reads
    const Inst* cmp = nullptr;
    for (size_t k = 0; k + 1 < cb.insts.size(); k++) {
        const Inst& i = cb.insts[k];
        if (i.op == Op::CONST) continue;
        if ((i.op == Op::DEREF || i.op == Op::COPY || i.op == Op::ZEXT || i.op == Op::SEXT) &&
            i.src1.isVreg() && i.dest.isVreg()) {
            uint32_t s = i.src1.vregId;
            int64_t cv;
            if (fn.localSlotVregs.count(s)) fromVar[i.dest.vregId] = s;
            else if (fromVar.count(s)) fromVar[i.dest.vregId] = fromVar[s];
            else if (!isConstOperand(fn, i.src1, cv, defCount)) return false;   // (zext of the bound)
            continue;
        }
        if (i.op >= Op::CMP_EQ && i.op <= Op::CMP_GEU && i.dest.isVreg() && br.src1.isVreg() &&
            i.dest.vregId == br.src1.vregId && !cmp) { cmp = &i; continue; }
        return false;
    }
    if (!cmp) return false;
    int64_t bound;
    uint32_t var;
    if (cmp->src1.isVreg() && fn.localSlotVregs.count(cmp->src1.vregId)) var = cmp->src1.vregId;
    else if (cmp->src1.isVreg() && fromVar.count(cmp->src1.vregId)) var = fromVar[cmp->src1.vregId];
    else return false;
    if (!isConstOperand(fn, cmp->src2, bound, defCount)) return false;
    if (fn.memoryVregs.count(var)) return false;
    L.var = var;

    // Increment block: v = v +/- step
    const Block& ib = fn.blocks[L.inc];
    if (ib.insts.empty() || ib.insts.back().op != Op::BR || ib.insts.back().src1.name != cl) return false;
    std::map<uint32_t, int64_t> varPlus;    // vreg -> v + offset
    int64_t step = 0;
    bool stored = false;
    for (size_t k = 0; k + 1 < ib.insts.size(); k++) {
        const Inst& i = ib.insts[k];
        if (i.op == Op::CONST) continue;
        if ((i.op == Op::DEREF || i.op == Op::COPY) && i.src1.isVreg() && i.src1.vregId == var && i.dest.isVreg()) {
            varPlus[i.dest.vregId] = 0; continue;
        }
        // widening/narrowing of v + offset (c++ on a char): the values are
        // checked below to stay in range, so the offset is unchanged
        if ((i.op == Op::ZEXT || i.op == Op::SEXT || i.op == Op::TRUNC || i.op == Op::COPY) &&
            i.src1.isVreg() && varPlus.count(i.src1.vregId) && i.dest.isVreg()) {
            varPlus[i.dest.vregId] = varPlus[i.src1.vregId]; continue;
        }
        if ((i.op == Op::ADD || i.op == Op::SUB) && i.dest.isVreg()) {
            int64_t c;
            int64_t base;
            if (i.src1.isVreg() && i.src1.vregId == var) base = 0;
            else if (i.src1.isVreg() && varPlus.count(i.src1.vregId)) base = varPlus[i.src1.vregId];
            else return false;
            if (!isConstOperand(fn, i.src2, c, defCount)) return false;
            varPlus[i.dest.vregId] = base + (i.op == Op::ADD ? c : -c);
            continue;
        }
        if (i.op == Op::STORE && i.src2.isVreg() && i.src2.vregId == var && i.src1.isVreg() &&
            varPlus.count(i.src1.vregId) && !stored) {
            step = varPlus[i.src1.vregId]; stored = true; continue;
        }
        return false;
    }
    if (!stored || step == 0) return false;

    // Initial value: the block before the condition stores a constant into v
    if (condIdx == 0) return false;
    const Block& pb = fn.blocks[condIdx - 1];
    if (pb.insts.empty() || pb.insts.back().op != Op::BR || pb.insts.back().src1.name != cl) return false;
    int64_t init = 0;
    bool haveInit = false;
    for (size_t k = pb.insts.size() - 1; k-- > 0;) {
        const Inst& i = pb.insts[k];
        if (i.op == Op::STORE && i.src2.isVreg() && i.src2.vregId == var) {
            haveInit = isConstOperand(fn, i.src1, init, defCount);
            break;
        }
        if (i.op == Op::CALL || i.op == Op::CALL_VOID || i.op == Op::CALL_INDIRECT || i.op == Op::ASM_INLINE) break;
    }
    if (!haveInit) return false;
    // The only other edges into the condition: the increment (and the pred)
    for (size_t b = 0; b < fn.blocks.size(); b++) {
        if (b == condIdx - 1 || b == L.inc) continue;
        for (const auto& i : fn.blocks[b].insts) {
            if ((i.op == Op::BR && i.src1.name == cl) ||
                (i.op == Op::BR_COND && (i.dest.name == cl || i.src2.name == cl))) return false;
        }
    }

    // Trip count: every value non-negative and below the sign bit of its type
    auto vt = fn.vregTypes.find(var);
    L.varType = vt != fn.vregTypes.end() ? vt->second : Type::I16;
    int bits = L.varType == Type::I8 ? 8 : L.varType == Type::I32 ? 32 : 16;
    int64_t limit = (int64_t)1 << (bits - 1);
    if (bound < 0 || bound >= limit) return false;
    int64_t v = init;
    L.values.clear();
    for (;;) {
        if (v < -limit || v >= limit) return false;
        bool go;
        switch (cmp->op) {
            case Op::CMP_LT: case Op::CMP_LTU: go = v < bound; break;
            case Op::CMP_LE: case Op::CMP_LEU: go = v <= bound; break;
            case Op::CMP_GT: case Op::CMP_GTU: go = v > bound; break;
            case Op::CMP_GE: case Op::CMP_GEU: go = v >= bound; break;
            case Op::CMP_NE: go = v != bound; break;
            case Op::CMP_EQ: go = v == bound; break;
            default: return false;
        }
        if (!go) break;
        if (v < 0) return false;                 // a negative value of a (maybe) unsigned variable
        L.values.push_back(v);
        if (L.values.size() > 255) return false;
        v += step;
    }
    // An unsigned comparison of a final value below zero would have wrapped
    if (v < 0 && cmp->op >= Op::CMP_LTU) return false;
    L.finalValue = v;
    return true;
}

} // namespace

void optimizeLoopUnroll(Module& mod) {
    for (auto& fn : mod.functions) {
        if (!runOn(fn, "LoopUnrolling")) continue;
        bool changed = true;
        int guard = 64;
        while (changed && --guard > 0) {
            changed = false;
            std::map<uint32_t, int> defCount;
            for (const auto& b : fn.blocks)
                for (const auto& i : b.insts) {
                    if (i.dest.isVreg()) defCount[i.dest.vregId]++;
                    if ((i.op == Op::STORE || i.op == Op::STORE_ZP) && i.src2.isVreg()) defCount[i.src2.vregId] += 2;
                }
            for (size_t c = 0; c < fn.blocks.size() && !changed; c++) {
                ForLoop L;
                if (!matchForLoop(fn, c, defCount, L)) continue;
                // Innermost first: no other loop condition inside the body
                bool inner = false;
                for (size_t b = L.body; b < L.inc; b++)
                    if (fn.blocks[b].label.compare(0, 8, "for_cond") == 0 ||
                        fn.blocks[b].label.compare(0, 10, "while_cond") == 0 ||
                        fn.blocks[b].label.compare(0, 7, "do_body") == 0) inner = true;
                if (inner) continue;

                // The body: no asm, no assignment / address of the variable,
                // labels only reached from inside the loop
                std::set<std::string> bodyLabels;
                for (size_t b = L.body; b < L.inc; b++) bodyLabels.insert(fn.blocks[b].label);
                bool ok = true, breaks = false;
                int size = 0;
                for (size_t b = L.body; b < L.inc && ok; b++) {
                    if (fn.addressTakenLabels.count(fn.blocks[b].label)) ok = false;
                    for (const auto& i : fn.blocks[b].insts) {
                        if (i.op == Op::NOP) continue;
                        size++;
                        if (i.op == Op::ASM_INLINE || i.op == Op::ADDR_LABEL || i.op == Op::BR_INDIRECT) ok = false;
                        if ((i.op == Op::STORE || i.op == Op::STORE_ZP) && i.src2.isVreg() && i.src2.vregId == L.var) ok = false;
                        if (i.op == Op::ADDR_LOCAL && i.src1.isVreg() && i.src1.vregId == L.var) ok = false;
                        if (i.dest.isVreg() && i.dest.vregId == L.var) ok = false;
                        auto target = [&](const std::string& t) {
                            if (t == fn.blocks[L.end].label) breaks = true;
                        };
                        if (i.op == Op::BR) target(i.src1.name);
                        if (i.op == Op::BR_COND) { target(i.dest.name); target(i.src2.name); }
                    }
                }
                for (size_t b = 0; b < fn.blocks.size() && ok; b++) {
                    if (b >= L.body && b < L.inc) continue;
                    for (const auto& i : fn.blocks[b].insts) {
                        auto into = [&](const std::string& t) { if (bodyLabels.count(t) && t != fn.blocks[L.body].label) ok = false; };
                        if (i.op == Op::BR) into(i.src1.name);
                        if (i.op == Op::BR_COND) { into(i.dest.name); into(i.src2.name); }
                        if (i.op == Op::SWITCH) { into(i.src2.name); for (const auto& sc : i.switchCases) into(sc.second); }
                    }
                }
                if (!ok) continue;
                // Estimate: about 4 bytes per IR instruction (and 2 per copy for a CONST/store)
                int64_t bytes = (int64_t)L.values.size() * (size * 4 + (breaks ? 6 : 2));
                if (bytes > unrollMaxBytes) continue;

                // Temporaries the body defines get fresh vregs in each copy, so
                // the unrolled code has one definition per temp, like straight
                // code (the register allocator's live ranges expect that)
                std::set<uint32_t> bodyTemps;
                for (size_t b = L.body; b < L.inc; b++)
                    for (const auto& i : fn.blocks[b].insts)
                        if (i.dest.isVreg() && !fn.localSlotVregs.count(i.dest.vregId) &&
                            !fn.memoryVregs.count(i.dest.vregId))
                            bodyTemps.insert(i.dest.vregId);
                // ... unless one is also used outside the body
                for (size_t b = 0; b < fn.blocks.size(); b++) {
                    if (b >= L.body && b < L.inc) continue;
                    for (const auto& i : fn.blocks[b].insts) {
                        auto out = [&](const Operand& o) { if (o.isVreg()) bodyTemps.erase(o.vregId); };
                        out(i.dest); out(i.src1); out(i.src2);
                        for (const auto& a : i.args) out(a);
                    }
                }

                // Build the copies
                std::string incLabel = fn.blocks[L.inc].label, endLabel = fn.blocks[L.end].label;
                std::vector<Block> out;
                size_t n = L.values.size();
                auto copyLabel = [&](const std::string& l, size_t k) { return l + "_u" + std::to_string(k); };
                for (size_t k = 0; k < n; k++) {
                    uint32_t t = fn.allocVreg();
                    fn.vregTypes[t] = L.varType;
                    std::map<uint32_t, uint32_t> fresh;
                    for (uint32_t v : bodyTemps) {
                        uint32_t nv = fn.allocVreg();
                        fresh[v] = nv;
                        auto ti = fn.vregTypes.find(v);
                        if (ti != fn.vregTypes.end()) fn.vregTypes[nv] = ti->second;
                        auto si = fn.vregSizes.find(v);
                        if (si != fn.vregSizes.end()) fn.vregSizes[nv] = si->second;
                        auto ai = fn.vregAlign.find(v);
                        if (ai != fn.vregAlign.end()) fn.vregAlign[nv] = ai->second;
                        if (fn.registerVregs.count(v)) fn.registerVregs.insert(nv);
                    }
                    auto renameTemp = [&](Operand& o) {
                        if (o.isVreg()) { auto f = fresh.find(o.vregId); if (f != fresh.end()) o.vregId = f->second; }
                    };
                    std::string next = (k + 1 < n) ? copyLabel(fn.blocks[L.body].label, k + 1) : incLabel;
                    for (size_t b = L.body; b < L.inc; b++) {
                        Block nb = fn.blocks[b];
                        nb.label = copyLabel(nb.label, k);
                        if (b == L.body) {
                            Inst ci;
                            ci.op = Op::CONST;
                            ci.dest = Operand::vreg(t, L.varType);
                            ci.src1 = Operand::imm(L.values[k], L.varType);
                            ci.resultType = L.varType;
                            std::vector<Inst> pre{ci};
                            if (breaks) {
                                Inst st;
                                st.op = Op::STORE;
                                st.resultType = L.varType;
                                st.src1 = Operand::vreg(t, L.varType);
                                st.src2 = Operand::vreg(L.var, L.varType);
                                pre.push_back(st);
                            }
                            nb.insts.insert(nb.insts.begin(), pre.begin(), pre.end());
                        }
                        auto relabel = [&](std::string& l) {
                            if (bodyLabels.count(l)) l = copyLabel(l, k);
                            else if (l == incLabel) l = next;
                        };
                        for (auto& i : nb.insts) {
                            if (i.op == Op::BR) relabel(i.src1.name);
                            if (i.op == Op::BR_COND) { relabel(i.dest.name); relabel(i.src2.name); }
                            if (i.op == Op::SWITCH) { relabel(i.src2.name); for (auto& sc : i.switchCases) relabel(sc.second); }
                            // The variable's value in this iteration
                            if (i.op == Op::CONST && i.dest.isVreg() && i.dest.vregId == t) continue;
                            if (i.op == Op::STORE && i.src2.isVreg() && i.src2.vregId == L.var) continue;
                            if ((i.op == Op::DEREF || i.op == Op::LOAD) && i.src1.isVreg() && i.src1.vregId == L.var) {
                                i.op = Op::COPY;
                                i.src1 = Operand::vreg(t, L.varType);
                                continue;
                            }
                            auto sub = [&](Operand& o) { if (o.isVreg() && o.vregId == L.var) o.vregId = t; };
                            sub(i.src1); sub(i.src2);
                            for (auto& a : i.args) sub(a);
                        }
                        for (auto& i : nb.insts) {
                            renameTemp(i.dest); renameTemp(i.src1); renameTemp(i.src2);
                            for (auto& a : i.args) renameTemp(a);
                            for (auto& ph : i.phiIncoming) renameTemp(ph.first);
                        }
                        out.push_back(std::move(nb));
                    }
                }
                // The increment block becomes: v = final value
                Block fin;
                fin.label = incLabel;
                {
                    uint32_t t = fn.allocVreg();
                    fn.vregTypes[t] = L.varType;
                    Inst ci;
                    ci.op = Op::CONST;
                    ci.dest = Operand::vreg(t, L.varType);
                    ci.src1 = Operand::imm(L.finalValue, L.varType);
                    ci.resultType = L.varType;
                    Inst st;
                    st.op = Op::STORE;
                    st.resultType = L.varType;
                    st.src1 = Operand::vreg(t, L.varType);
                    st.src2 = Operand::vreg(L.var, L.varType);
                    fin.insts = {ci, st};
                }
                out.push_back(std::move(fin));

                // The condition block jumps straight into the first copy
                Block& cb = fn.blocks[L.cond];
                cb.insts.clear();
                Inst jb;
                jb.op = Op::BR;
                jb.src1 = Operand::label(n ? copyLabel(fn.blocks[L.body].label, 0) : incLabel);
                cb.insts.push_back(jb);

                trace("unroll", fn.name, "loop at " + fn.blocks[L.cond].label + " unrolled " +
                      std::to_string(n) + " times");
                fn.blocks.erase(fn.blocks.begin() + L.body, fn.blocks.begin() + L.inc + 1);
                fn.blocks.insert(fn.blocks.begin() + L.body, out.begin(), out.end());
                changed = true;
            }
        }
    }
}


// ============================================================================
// Dead temporaries
// ============================================================================

// Removes pure instructions whose temporary result is never used (left by
// the other passes: a loop unroll's per-copy constant that every use folded
// away, a copy whose users were redirected). Loads (maybe volatile),
// variables' slots and memory-pinned vregs are never removed.
// *p++ / *p-- on a pointer variable S builds
//     %t = deref S;  %u = add %t, c;  store %u, S;  ... accesses through %t
// so the access goes through a copy of the old pointer. When nothing between
// the store and the last use of %t reads or writes S (or calls, or is asm),
// the increment moves after that use and %t becomes S + 0, which IRCodeGen
// folds into the accesses ((S),y with S in ZP):
//     %t = add S, 0;  ... accesses through %t;  %u = add S, c;  store %u, S
// ++/-- on an integer (or pointer) variable S, updated in place (codegen:
// inc/dec/inw/dew on S) instead of through a temporary:
//   post (n--):  %t = deref S; %u = op %t, c; store %u, S
//                → %u = op S, c   (S has %t's value there; %t stays for its uses)
//   pre  (--n):  %u = op S, c; store %u, S; ...uses of %u
//                → the uses read S (it holds %u until written again), so %u
//                  is used only by the store
static void incDecInPlace(Function& fn) {
    auto isTerm = [](Op op) { return op == Op::BR || op == Op::BR_COND || op == Op::SWITCH || op == Op::RET; };
    std::map<uint32_t, int> defs;
    for (const auto& blk : fn.blocks)
        for (const auto& in : blk.insts) if (in.dest.isVreg()) defs[in.dest.vregId]++;
    auto isVar = [&](const Operand& o) {
        return o.isVreg() && fn.localSlotVregs.count(o.vregId) && !fn.memoryVregs.count(o.vregId) &&
               !fn.vregSizes.count(o.vregId);
    };
    for (auto& blk : fn.blocks) {
        auto& insts = blk.insts;
        for (size_t i = 0; i + 1 < insts.size(); i++) {
            Inst& a = insts[i];
            const Inst& st = insts[i + 1];
            // a call's result stored into a variable (c = getc()): its later
            // uses read the variable too (the result goes from A/X into it only)
            bool isCall = (a.op == Op::CALL || a.op == Op::CALL_INDIRECT) && a.dest.isVreg();
            if (!isCall && ((a.op != Op::ADD && a.op != Op::SUB) || !a.dest.isVreg() || !a.src2.isImm()))
                continue;
            if (a.resultType == Type::F32 || a.resultType == Type::I32 ||
                st.op != Op::STORE || !st.src1.isVreg() || st.src1.vregId != a.dest.vregId || !st.src2.isVreg())
                continue;
            const Operand S = st.src2;
            if (!isVar(S) || defs[a.dest.vregId] != 1 || fn.localSlotVregs.count(a.dest.vregId)) continue;
            if (isCall) {
                auto vt = fn.vregTypes.find(S.vregId);
                if (vt == fn.vregTypes.end() || vt->second != a.resultType || st.resultType != a.resultType) continue;
            }
            uint32_t u = a.dest.vregId;
            // post: the operand is a copy of S read just before
            if (!isCall && a.src1.isVreg() && a.src1.vregId != S.vregId && i > 0) {
                const Inst& d = insts[i - 1];
                if ((d.op == Op::DEREF || d.op == Op::COPY) && d.dest.isVreg() && d.dest.vregId == a.src1.vregId &&
                    d.src1.isVreg() && d.src1.vregId == S.vregId && d.resultType == a.resultType) {
                    a.src1 = S;
                    a.src1.type = a.resultType;
                    trace("incdec", fn.name, "update of a variable in place (post)", a.loc);
                }
            }
            // pre: the uses of %u after the store read S
            if (!isCall && (!a.src1.isVreg() || a.src1.vregId != S.vregId)) continue;
            std::vector<std::pair<size_t, Operand*>> uses;
            bool ok = true, sWritten = false;
            for (size_t j = i + 2; j < insts.size() && ok; j++) {
                Inst& x = insts[j];
                auto note = [&](Operand& o, bool valuePos) {
                    if (!o.isVreg() || o.vregId != u) return;
                    if (sWritten || !valuePos) ok = false;
                    else uses.push_back({j, &o});
                };
                note(x.src1, true);
                note(x.src2, !(x.op == Op::STORE));                 // a store's target: no
                if (x.op == Op::STORE && x.dest.isVreg() && x.dest.vregId == u) ok = false;
                for (auto& o : x.args) note(o, true);
                for (auto& ph : x.phiIncoming) if (ph.first.isVreg() && ph.first.vregId == u) ok = false;
                if (x.op == Op::CALL || x.op == Op::CALL_VOID || x.op == Op::CALL_INDIRECT || x.op == Op::ASM_INLINE)
                    sWritten = true;   // (not written by a call: S is not address-taken; be careful anyway)
                if ((x.dest.isVreg() && x.dest.vregId == S.vregId) ||
                    (x.op == Op::STORE && x.src2.isVreg() && x.src2.vregId == S.vregId))
                    sWritten = true;
                (void)isTerm;
            }
            if (!ok || uses.empty()) continue;
            // every use of %u: the store and these (none in other blocks)
            int total = 0;
            for (const auto& b2 : fn.blocks)
                for (const auto& x : b2.insts) {
                    auto is = [&](const Operand& o) { return o.isVreg() && o.vregId == u; };
                    if (is(x.src1)) total++;
                    if (is(x.src2)) total++;
                    for (const auto& o : x.args) if (is(o)) total++;
                    for (const auto& ph : x.phiIncoming) if (is(ph.first)) total++;
                }
            if (total != (int)uses.size() + 1) continue;
            for (auto& [j, o] : uses) { Type t = o->type; *o = S; o->type = t; }
            trace("incdec", fn.name, isCall ? "a call's result read from the variable it is stored in"
                                            : "update of a variable in place (pre)", a.loc);
        }
    }
}

void optimizePostIncrementSink(Module& mod) {
    auto isTerm = [](Op op) { return op == Op::BR || op == Op::BR_COND || op == Op::SWITCH || op == Op::RET; };
    for (auto& fn : mod.functions) {
        if (!runOn(fn, "PostIncrementSink")) continue;
        std::map<uint32_t, int> defs, useCount;
        for (const auto& blk : fn.blocks)
            for (const auto& in : blk.insts) {
                if (in.dest.isVreg()) defs[in.dest.vregId]++;
                for (const Operand* o : {&in.src1, &in.src2})
                    if (o->isVreg()) useCount[o->vregId]++;
                for (const auto& a : in.args) if (a.isVreg()) useCount[a.vregId]++;
                for (const auto& ph : in.phiIncoming) if (ph.first.isVreg()) useCount[ph.first.vregId]++;
            }
        auto reads = [](const Inst& in, uint32_t v) {
            auto is = [v](const Operand& o) { return o.isVreg() && o.vregId == v; };
            if (is(in.src1) || is(in.src2)) return true;
            for (const auto& a : in.args) if (is(a)) return true;
            for (const auto& ph : in.phiIncoming) if (is(ph.first)) return true;
            return false;
        };
        for (auto& blk : fn.blocks) {
            auto& insts = blk.insts;
            for (size_t i = 0; i + 3 < insts.size(); i++) {
                Inst& d = insts[i];
                if (d.op != Op::DEREF || d.resultType != Type::PTR || !d.dest.isVreg() || !d.src1.isVreg()) continue;
                uint32_t t = d.dest.vregId, S = d.src1.vregId;
                if (!fn.localSlotVregs.count(S) || fn.memoryVregs.count(S) || fn.localSlotVregs.count(t) ||
                    defs[t] != 1) continue;
                Inst& a = insts[i + 1];
                const Inst& st = insts[i + 2];
                if ((a.op != Op::ADD && a.op != Op::SUB) || !a.dest.isVreg() || !a.src1.isVreg() ||
                    a.src1.vregId != t || !a.src2.isImm()) continue;
                uint32_t u = a.dest.vregId;
                if (defs[u] != 1 || useCount[u] != 1 || fn.localSlotVregs.count(u)) continue;
                if (st.op != Op::STORE || !st.src1.isVreg() || st.src1.vregId != u ||
                    !st.src2.isVreg() || st.src2.vregId != S) continue;
                // every other use of %t is in this block after the store
                int found = 0;
                size_t last = 0;
                for (size_t j = i + 3; j < insts.size(); j++)
                    if (reads(insts[j], t)) {
                        for (const Operand* o : {&insts[j].src1, &insts[j].src2})
                            if (o->isVreg() && o->vregId == t) found++;
                        for (const auto& x : insts[j].args) if (x.isVreg() && x.vregId == t) found++;
                        for (const auto& ph : insts[j].phiIncoming) if (ph.first.isVreg() && ph.first.vregId == t) found++;
                        last = j;
                    }
                if (found == 0 || found != useCount[t] - 1 || isTerm(insts[last].op)) continue;
                bool ok = true;
                for (size_t j = i + 3; j <= last && ok; j++) {
                    const Inst& x = insts[j];
                    if (x.op == Op::CALL || x.op == Op::CALL_VOID || x.op == Op::CALL_INDIRECT ||
                        x.op == Op::ASM_INLINE || x.op == Op::PHI || isTerm(x.op)) ok = false;
                    if ((x.dest.isVreg() && x.dest.vregId == S) || reads(x, S)) ok = false;
                    if (x.dest.isVreg() && x.dest.vregId == u) ok = false;
                }
                if (!ok) continue;
                d.op = Op::ADD;
                d.src2 = Operand::imm(0, Type::I16);
                a.src1 = Operand::vreg(S, Type::PTR);
                Inst ma = a, ms = st;
                trace("postinc", fn.name, "access before the increment of the pointer", d.loc);
                insts.insert(insts.begin() + last + 1, {ma, ms});
                insts.erase(insts.begin() + i + 1, insts.begin() + i + 3);
                useCount[t]--;   // (the increment no longer reads %t)
                useCount[S]++;
            }
        }
    }
    for (auto& fn : mod.functions)
        if (runOn(fn, "PostIncrementSink")) incDecInPlace(fn);
}

// A leaf function (no calls, no inline asm) keeps its pointer locals in the
// ZP register pool (IRCodeGen: like register variables). Its pointer
// parameters arrive in their own storage (static allocation: _f__param_p,
// which same-file callers fill before entering at _f__sac; or a stack frame):
// one used twice or more is stored at the start of the body into a new local,
// a register variable, that every other use refers to.
bool paramsStayInZp(const Function& fn) {
    if (getenv("CC45_NO_ZP_PARAMS")) return false;
    if (fn.conv != CallConv::ZP || fn.isVariadic || fn.isRegparm || fn.isNaked || fn.isInterrupt ||
        fn.isNested || fn.paramTypes.empty() || fn.blocks.empty())
        return false;
    for (auto t : fn.paramTypes) if (t == Type::F32) return false;
    for (const auto& [v, t] : fn.vregTypes) if (t == Type::F32) return false;
    for (const auto& blk : fn.blocks)
        for (const auto& in : blk.insts) {
            // a call or asm can overwrite the block; float operations call the runtime
            if (in.op == Op::CALL || in.op == Op::CALL_VOID || in.op == Op::CALL_INDIRECT ||
                in.op == Op::ASM_INLINE || in.resultType == Type::F32 ||
                in.src1.type == Type::F32 || in.src2.type == Type::F32)
                return false;
        }
    return true;
}

void leafPointerParamsToZp(Module& mod) {
    if (getenv("CC45_NO_ZP_POINTERS")) return;
    for (auto& fn : mod.functions) {
        if (fn.blocks.empty() || fn.isNaked) continue;
        bool leaf = true;
        for (const auto& blk : fn.blocks)
            for (const auto& in : blk.insts)
                if (in.op == Op::CALL || in.op == Op::CALL_VOID || in.op == Op::CALL_INDIRECT || in.op == Op::ASM_INLINE)
                    leaf = false;
        if (!leaf) continue;
        // the copy must run once: no branch (or label address) to the entry block
        const std::string& entryLabel = fn.blocks[0].label;
        bool reentered = false;
        for (const auto& blk : fn.blocks)
            for (const auto& in : blk.insts) {
                for (const Operand* o : {&in.src1, &in.src2, &in.dest})
                    if (o->kind == OperandKind::LABEL && o->name == entryLabel) reentered = true;
                for (const auto& a : in.args)
                    if (a.kind == OperandKind::LABEL && a.name == entryLabel) reentered = true;
                for (const auto& c : in.switchCases) if (c.second == entryLabel) reentered = true;
            }
        if (reentered) continue;
        // (parameters left in the ZP parameter block are in ZP already)
        if (paramsStayInZp(fn)) continue;
        for (uint32_t p = 0; p < fn.paramTypes.size(); p++) {
            if (fn.paramTypes[p] != Type::PTR || fn.memoryVregs.count(p) || fn.registerVregs.count(p)) continue;
            auto uses = [&](const Operand& o) { return o.isVreg() && o.vregId == p; };
            int n = 0;
            for (const auto& blk : fn.blocks)
                for (const auto& in : blk.insts) {
                    if (uses(in.src1)) n++;
                    if (uses(in.src2)) n++;
                    if (uses(in.dest)) n++;
                    for (const auto& a : in.args) if (uses(a)) n++;
                    for (const auto& ph : in.phiIncoming) if (uses(ph.first)) n++;
                }
            if (n < 2) continue;
            uint32_t v = fn.allocVreg();
            for (auto& blk : fn.blocks)
                for (auto& in : blk.insts) {
                    if (uses(in.src1)) in.src1.vregId = v;
                    if (uses(in.src2)) in.src2.vregId = v;
                    if (uses(in.dest)) in.dest.vregId = v;
                    for (auto& a : in.args) if (uses(a)) a.vregId = v;
                    for (auto& ph : in.phiIncoming) if (uses(ph.first)) ph.first.vregId = v;
                }
            Inst st;
            st.op = Op::STORE;
            st.resultType = Type::PTR;
            st.src1 = Operand::vreg(p, Type::PTR);
            st.src2 = Operand::vreg(v, Type::PTR);
            auto& entry = fn.blocks[0].insts;
            if (!entry.empty()) st.loc = entry[0].loc;
            entry.insert(entry.begin(), st);
            fn.localSlotVregs.insert(v);
            fn.registerVregs.insert(v);
            fn.vregTypes[v] = Type::PTR;
            std::string name = "__zp_" + (p < fn.paramNames.size() && !fn.paramNames[p].empty()
                                          ? fn.paramNames[p] : std::to_string(p));
            fn.localNames[name] = v;
            fn.localNamesOrder.push_back(name);
            trace("zp", fn.name, "pointer parameter " + name.substr(5) + " in a ZP register (leaf)", st.loc);
        }
    }
}

void removeDeadTemps(Module& mod) {
    auto pure = [](Op op) {
        switch (op) {
            case Op::CONST: case Op::FCONST: case Op::COPY:
            case Op::ADD: case Op::SUB: case Op::MUL: case Op::MUL_U:
            case Op::AND: case Op::OR: case Op::XOR:
            case Op::LSL: case Op::LSR: case Op::ASR: case Op::SHL: case Op::SHR:
            case Op::NEG: case Op::NOT:
            case Op::CMP_EQ: case Op::CMP_NE: case Op::CMP_LT: case Op::CMP_LE:
            case Op::CMP_GT: case Op::CMP_GE: case Op::CMP_LTU: case Op::CMP_LEU:
            case Op::CMP_GTU: case Op::CMP_GEU:
            case Op::SEXT: case Op::ZEXT: case Op::TRUNC:
            case Op::ADDR_GLOBAL: case Op::ADDR_LOCAL: case Op::ADDR_LABEL: case Op::ADDR_ELEM:
                return true;
            default:
                return false;
        }
    };
    for (auto& fn : mod.functions) {
        if (fn.noOptimize) continue;
        bool changed = true;
        while (changed) {
            changed = false;
            std::set<uint32_t> used;
            for (const auto& b : fn.blocks)
                for (const auto& i : b.insts) {
                    if (i.src1.isVreg()) used.insert(i.src1.vregId);
                    if (i.src2.isVreg()) used.insert(i.src2.vregId);
                    for (const auto& a : i.args) if (a.isVreg()) used.insert(a.vregId);
                    for (const auto& p : i.phiIncoming) if (p.first.isVreg()) used.insert(p.first.vregId);
                }
            for (auto& b : fn.blocks)
                for (auto& i : b.insts) {
                    // a read of a local variable (deref of its slot: the value
                    // of x++ when nothing uses it), unless the variable is
                    // volatile or in memory
                    bool slotRead = i.op == Op::DEREF && i.src1.isVreg() &&
                                    fn.localSlotVregs.count(i.src1.vregId) &&
                                    !fn.memoryVregs.count(i.src1.vregId);
                    // a call of a pure / const function whose result nothing
                    // uses (or that returns nothing): no effect (Issue #76)
                    if (purityOf(mod, i) && (i.op == Op::CALL_VOID || !i.dest.isVreg())) {
                        i = Inst();
                        i.op = Op::NOP;
                        changed = true;
                        continue;
                    }
                    bool pureCall = purityOf(mod, i) > 0;
                    if ((!pure(i.op) && !slotRead && !pureCall) || !i.dest.isVreg()) continue;
                    uint32_t d = i.dest.vregId;
                    if (used.count(d) || fn.localSlotVregs.count(d) || fn.memoryVregs.count(d)) continue;
                    if ((int)d == fn.staticLinkVreg || (int)d == fn.retAddrSlot) continue;
                    i = Inst();
                    i.op = Op::NOP;
                    changed = true;
                }
        }
    }
}

// A member of a global struct or an element of a global array at a constant
// index: %p = add ptr @g, c used only as the address of loads and stores
// (and defined once) is @g+c in them, an absolute address, not a pointer
// computed into a register (g.y = 5 is sta _g+2). Only for this file's
// globals in the default address space (not __zp: base-page addressing;
// not __far). Runs after the other passes, which see the add; the add is
// then dead (removeDeadTemps). Under AddrElemFusion's switch.
void foldGlobalOffsetAccess(Module& mod) {
    std::set<std::string> plain;
    for (const auto& g : mod.globals)
        if (g.addressSpace == 0) plain.insert(g.name);
    for (auto& fn : mod.functions) {
        if (!runOn(fn, "AddrElemFusion")) continue;
        std::map<uint32_t, int> defCount;
        std::map<uint32_t, const Inst*> def;
        std::set<uint32_t> other;          // used other than as a load's / store's address
        for (const auto& b : fn.blocks)
            for (const auto& in : b.insts) {
                if (in.dest.isVreg()) { defCount[in.dest.vregId]++; def[in.dest.vregId] = &in; }
                bool addr1 = in.op == Op::LOAD && in.args.empty();
                bool addr2 = in.op == Op::STORE && in.args.empty();
                if (in.src1.isVreg() && !addr1) other.insert(in.src1.vregId);
                if (in.src2.isVreg() && !addr2) other.insert(in.src2.vregId);
                for (const auto& a : in.args)
                    if (a.isVreg()) other.insert(a.vregId);
            }
        std::map<uint32_t, std::string> at;   // vreg -> "@g+c"
        for (const auto& [v, n] : defCount) {
            if (n != 1 || other.count(v) || fn.localSlotVregs.count(v) || fn.memoryVregs.count(v)) continue;
            const Inst* d = def[v];
            if (d->op != Op::ADD || d->resultType != Type::PTR || d->src1.kind != OperandKind::GLOBAL ||
                !d->src2.isImm() || !plain.count(d->src1.name)) continue;
            int64_t c = d->src2.immVal;
            if (c <= 0 || c > 0xFFFF) continue;
            at[v] = d->src1.name + "+" + std::to_string(c);
        }
        if (at.empty()) continue;
        for (auto& b : fn.blocks)
            for (auto& in : b.insts) {
                ir::Operand* a = in.op == Op::LOAD ? &in.src1 : in.op == Op::STORE ? &in.src2 : nullptr;
                if (!a || !a->isVreg() || !at.count(a->vregId)) continue;
                trace("globaloff", fn.name, "v" + std::to_string(a->vregId) + " is " + at[a->vregId], in.loc);
                *a = Operand::global(at[a->vregId]);
            }
    }
}

// x op y with op commutative (add, and, or, xor, mul) and y the result of
// the instruction just before (x not): y op x, so the operand loaded into
// A/X first is the one already there; ca45 then drops its store and reload
// (#350: y << 9 | x << 2 stored x << 2 and loaded y << 9 back). Runs last,
// on the order code generation sees. Under AlgebraicSimplify's switch.
void orderCommutativeOperands(Module& mod) {
    for (auto& fn : mod.functions) {
        if (!runOn(fn, "AlgebraicSimplify")) continue;
        for (auto& b : fn.blocks) {
            const Inst* prev = nullptr;
            for (auto& in : b.insts) {
                if (in.op == Op::NOP) continue;
                bool comm = in.op == Op::ADD || in.op == Op::AND || in.op == Op::OR || in.op == Op::XOR ||
                            in.op == Op::MUL || in.op == Op::MUL_U;
                if (comm && prev && prev->dest.isVreg() && in.src1.isVreg() && in.src2.isVreg() &&
                    in.src2.vregId == prev->dest.vregId && in.src1.vregId != prev->dest.vregId &&
                    in.args.empty()) {
                    trace("commute", fn.name, "v" + std::to_string(in.src2.vregId) + " (just computed) first", in.loc);
                    std::swap(in.src1, in.src2);
                }
                prev = &in;
            }
        }
    }
}

// x / c and x % c of the same x and constant c in a block (#352: k % 3, k
// / 3 made two calls of the divide): the divide once, before both, and
// x % c as x - (x / c) * c (C's truncating division: the same for signed
// values), the multiply by a constant shifts and adds. Only when x is not
// written between them and both results are defined once. Under
// StrengthReduction's switch.
void optimizeShareDivMod(Module& mod) {
    for (auto& fn : mod.functions) {
        if (!runOn(fn, "StrengthReduction")) continue;
        std::map<uint32_t, int64_t> constVals = singleDefConsts(fn);
        std::map<uint32_t, int> defCount;
        for (const auto& b : fn.blocks)
            for (const auto& in : b.insts) {
                if (in.dest.isVreg()) defCount[in.dest.vregId]++;
                if ((in.op == Op::STORE || in.op == Op::STORE_ZP) && in.src2.isVreg()) defCount[in.src2.vregId] += 2;
            }
        auto constOf = [&](const Operand& o, int64_t& v) {
            if (o.isImm()) { v = o.immVal; return true; }
            if (o.isVreg() && constVals.count(o.vregId)) { v = constVals[o.vregId]; return true; }
            return false;
        };
        for (auto& b : fn.blocks) {
            for (size_t i = 0; i < b.insts.size(); i++) {
                Inst& a = b.insts[i];
                bool aDiv = a.op == Op::DIV || a.op == Op::DIV_U, aMod = a.op == Op::MOD || a.op == Op::MOD_U;
                if ((!aDiv && !aMod) || !a.src1.isVreg() || !a.dest.isVreg() || defCount[a.dest.vregId] != 1) continue;
                int64_t c;
                if (!constOf(a.src2, c) || c == 0) continue;
                bool sg = a.op == Op::DIV || a.op == Op::MOD;
                uint32_t x = a.src1.vregId;
                for (size_t j = i + 1; j < b.insts.size(); j++) {
                    Inst& w = b.insts[j];
                    if ((w.dest.isVreg() && w.dest.vregId == x) ||
                        ((w.op == Op::STORE || w.op == Op::STORE_ZP) && w.src2.isVreg() && w.src2.vregId == x))
                        break;   // (x written: no longer the same value)
                    bool wDiv = w.op == (sg ? Op::DIV : Op::DIV_U), wMod = w.op == (sg ? Op::MOD : Op::MOD_U);
                    int64_t c2;
                    if (!(wDiv && aMod) && !(wMod && aDiv)) continue;
                    if (!w.src1.isVreg() || w.src1.vregId != x || !constOf(w.src2, c2) || c2 != c ||
                        w.resultType != a.resultType || !w.dest.isVreg() || defCount[w.dest.vregId] != 1)
                        continue;
                    Type t = a.resultType;
                    Inst div = aDiv ? a : w;
                    Inst mod = aMod ? a : w;
                    div.src2 = Operand::imm(c, t);   // (moved: the divisor's own constant may come after)
                    uint32_t prod = fn.allocVreg();
                    Inst mul;
                    mul.op = sg ? Op::MUL : Op::MUL_U;
                    mul.dest = Operand::vreg(prod, t);
                    mul.src1 = div.dest;
                    mul.src2 = Operand::imm(c, t);
                    mul.resultType = t;
                    mul.loc = mod.loc;
                    Inst sub;
                    sub.op = Op::SUB;
                    sub.dest = mod.dest;
                    sub.src1 = mod.src1;
                    sub.src2 = Operand::vreg(prod, t);
                    sub.resultType = t;
                    sub.loc = mod.loc;
                    fn.vregTypes[prod] = t;
                    trace("divmod", fn.name, "v" + std::to_string(mod.dest.vregId) + " = x % " + std::to_string(c) +
                          " as x - (x / " + std::to_string(c) + ") * " + std::to_string(c), mod.loc);
                    Inst nop;
                    nop.op = Op::NOP;
                    if (aMod) {
                        // the divide first, then the remainder from it
                        b.insts[j] = nop;
                        b.insts[i] = div;
                        b.insts.insert(b.insts.begin() + i + 1, {mul, sub});
                    } else {
                        b.insts[j] = mul;
                        b.insts.insert(b.insts.begin() + j + 1, sub);
                    }
                    break;
                }
            }
        }
    }
}

// p[c] through a far pointer (#348): a FAR_LOAD / FAR_STORE whose address
// is p + c (an add of a constant, or a copy of p) is the access at p with
// the byte offset c in its elemOffset, Z's start (the far pointer is not
// recomputed: ldz #c). c >= 256: the address p + (c & $FF00) and the offset
// c & $FF, that address computed once for the accesses of a block (p[128],
// p[129]). Only in a block, the add (or copy) before the access, p not
// written between; the add is then dead (removeDeadTemps). Under
// AddrElemFusion's switch.
void foldFarOffset(Module& mod) {
    for (auto& fn : mod.functions) {
        if (!runOn(fn, "AddrElemFusion")) continue;
        std::map<uint32_t, int> defCount;
        for (const auto& b : fn.blocks)
            for (const auto& in : b.insts)
                if (in.dest.isVreg()) defCount[in.dest.vregId]++;
        for (auto& b : fn.blocks) {
            // (p, c & $FF00) -> the vreg holding p + it in this block
            std::map<std::pair<uint32_t, int64_t>, std::pair<uint32_t, size_t>> made;
            auto writes = [&](const Inst& w, uint32_t v) {
                return (w.dest.isVreg() && w.dest.vregId == v) ||
                       (w.op == Op::STORE && w.src2.isVreg() && w.src2.vregId == v && w.args.empty()) ||
                       w.op == Op::CALL || w.op == Op::CALL_VOID || w.op == Op::CALL_INDIRECT || w.op == Op::ASM_INLINE;
            };
            auto unchanged = [&](uint32_t v, size_t from, size_t to) {
                for (size_t k = from + 1; k < to; k++)
                    if (writes(b.insts[k], v)) return false;
                return true;
            };
            for (size_t ui = 0; ui < b.insts.size(); ui++) {
                Inst& use = b.insts[ui];
                if (use.op != Op::FAR_LOAD && use.op != Op::FAR_STORE) continue;
                Operand& addr = use.op == Op::FAR_LOAD ? use.src1 : use.src2;
                if (!addr.isVreg() || defCount[addr.vregId] != 1) continue;
                int size = use.resultType == Type::I8 ? 1 : use.resultType == Type::I32 ? 4 : 2;
                // the address's definition, earlier in this block
                size_t di = ui;
                for (size_t k = 0; k < ui; k++)
                    if (b.insts[k].dest.isVreg() && b.insts[k].dest.vregId == addr.vregId) di = k;
                if (di == ui) continue;
                const Inst& def = b.insts[di];
                Operand base;
                int64_t c = 0;
                if (def.op == Op::COPY && def.resultType == Type::I32 && def.src1.isVreg()) base = def.src1;
                else if (def.op == Op::ADD && def.resultType == Type::I32 && def.src1.isVreg() && def.src2.isImm())
                    { base = def.src1; c = def.src2.immVal; }
                else if (def.op == Op::ADD && def.resultType == Type::I32 && def.src2.isVreg() && def.src1.isImm())
                    { base = def.src2; c = def.src1.immVal; }
                else continue;
                c += use.elemOffset;
                if (c < 0 || c > 0xFFFFFF || fn.memoryVregs.count(base.vregId)) continue;
                if (!unchanged(base.vregId, di, ui)) continue;
                int64_t lo = c & 0xFF, hi = c - lo;
                if (lo + size > 256) { lo = 0; hi = c; }   // (Z would wrap)
                if (lo && size == 4 && use.op == Op::FAR_LOAD) { lo = 0; hi = c; }   // (ldq [bp],z: no Z offset)
                if (hi == 0) {
                    trace("faroff", fn.name, "far access at v" + std::to_string(base.vregId) + " + " + std::to_string(lo), use.loc);
                    addr = base;
                    use.elemOffset = (int)lo;
                    continue;
                }
                // p + hi: an earlier one of this block, or the add (now p + hi)
                auto key = std::make_pair(base.vregId, hi);
                auto m = made.find(key);
                if (m != made.end() && defCount[m->second.first] == 1 && unchanged(base.vregId, m->second.second, ui)) {
                    addr = Operand::vreg(m->second.first, Type::I32);
                } else {
                    if (def.op != Op::ADD) continue;
                    // (the add's other uses would see p + hi: only when this is its one use)
                    int uses = 0;
                    for (const auto& b2 : fn.blocks)
                        for (const auto& in : b2.insts) {
                            auto u = [&](const Operand& o) { return o.isVreg() && o.vregId == addr.vregId; };
                            if (u(in.src1) || u(in.src2) || (in.op == Op::STORE && u(in.dest))) uses++;
                            for (const auto& a : in.args) if (u(a)) uses++;
                            for (const auto& ph : in.phiIncoming) if (u(ph.first)) uses++;
                        }
                    if (uses != 1) continue;
                    Inst& d = b.insts[di];
                    d.src1 = base;
                    d.src2 = Operand::imm(hi, Type::I32);
                    made[key] = {addr.vregId, di};
                }
                trace("faroff", fn.name, "far access at v" + std::to_string(base.vregId) + " + " + std::to_string(hi) +
                      ", offset " + std::to_string(lo), use.loc);
                use.elemOffset = (int)lo;
            }
        }
    }
}

// a[x + c] (also a[x - c]) with a global base: element x of the array
// shifted by c*stride bytes, so the index is x itself and the base address
// carries the constant (bubble sort's a[j] / a[j + 1] use the same index).
// Only when x + c is defined once, in the same block before the access, x
// is not written between, and the add is 16-bit (no 8-bit wraparound). Runs
// after the other passes (CSE would not see the offset)
void foldElemIndexOffset(Module& mod) {
    for (auto& fn : mod.functions) {
        if (fn.noOptimize) continue;
        std::map<uint32_t, int> defCount;
        for (const auto& b : fn.blocks)
            for (const auto& i : b.insts)
                if (i.dest.isVreg()) defCount[i.dest.vregId]++;
        for (auto& b : fn.blocks) {
            for (size_t ui = 0; ui < b.insts.size(); ui++) {
                Inst& use = b.insts[ui];
                Operand* base = nullptr;
                Operand* index = nullptr;
                int stride = 0;
                if ((use.op == Op::LOAD || use.op == Op::STORE) && use.args.size() >= 3) {
                    base = &use.args[0]; index = &use.args[1]; stride = (int)use.args[2].immVal;
                } else if (use.op == Op::ADDR_ELEM && !use.args.empty()) {
                    base = &use.src1; index = &use.src2; stride = (int)use.args[0].immVal;
                }
                if (!base || base->kind != OperandKind::GLOBAL || !index->isVreg() || stride <= 0) continue;
                uint32_t iv = index->vregId;
                if (defCount[iv] != 1 || fn.localSlotVregs.count(iv)) continue;
                // the definition, earlier in this block
                size_t di = ui;
                for (size_t k = 0; k < ui; k++)
                    if (b.insts[k].dest.isVreg() && b.insts[k].dest.vregId == iv) di = k;
                if (di == ui) continue;
                const Inst& def = b.insts[di];
                if ((def.op != Op::ADD && def.op != Op::SUB) || (def.resultType != Type::I16 && def.resultType != Type::PTR))
                    continue;
                Operand x;
                int64_t c;
                if (def.src1.isVreg() && def.src2.isImm()) { x = def.src1; c = def.src2.immVal; }
                else if (def.op == Op::ADD && def.src2.isVreg() && def.src1.isImm()) { x = def.src2; c = def.src1.immVal; }
                else continue;
                if (def.op == Op::SUB) c = -c;
                if (fn.memoryVregs.count(x.vregId)) continue;
                // x unchanged from the add to the access
                bool written = false;
                for (size_t k = di + 1; k < ui && !written; k++) {
                    const Inst& w = b.insts[k];
                    if (w.dest.isVreg() && w.dest.vregId == x.vregId) written = true;
                    if (w.op == Op::STORE && w.src2.isVreg() && w.src2.vregId == x.vregId) written = true;
                    if (w.op == Op::CALL || w.op == Op::CALL_VOID || w.op == Op::CALL_INDIRECT || w.op == Op::ASM_INLINE)
                        written = true;
                }
                if (written) continue;
                int64_t off = (int64_t)use.elemOffset + c * stride;
                if (off < -32768 || off > 32767) continue;
                trace("elemoff", fn.name, "a[v" + std::to_string(x.vregId) + (c >= 0 ? " + " : " - ") +
                      std::to_string(c >= 0 ? c : -c) + "]: base " + base->name + " + " + std::to_string(off), use.loc);
                *index = x;
                use.elemOffset = (int)off;
            }
        }
    }
}

// Loop rotation (#138): a loop's test at its bottom. A for / while loop is
// built as
//   @cond:  <test> ; br_cond %c, @body, @end     (H)
//   @body:  ...
//   @inc:   ... ; br @cond                       (the latch)
//   @end:
// so every iteration runs a branch back and the test's branch. With H moved
// after the latch, the latch falls into the test, whose branch goes back to
// the body; the block before the loop branches to the test once. The same
// code (the latch's branch becomes the entry's), one branch less an
// iteration. Only a one-block test whose branch goes to the block after it
// and out of the loop, and the last branch back to it (a continue's stay).
// The functions on decides; returns those it changed. (Run by CodegenStage
// on the finished IR, which keeps a function's rotation only when it is no
// larger in the assembled object.)
std::set<std::string> optimizeLoopRotation(Module& mod, const std::function<bool(const Function&)>& on) {
    std::set<std::string> rotated;
    auto ends = [](const Block& b) {
        if (b.insts.empty()) return false;
        switch (b.insts.back().op) {
            case Op::BR: case Op::BR_COND: case Op::BR_INDIRECT: case Op::SWITCH:
            case Op::RET: case Op::RET_VOID:
                return true;
            default:
                return false;
        }
    };
    for (auto& fn : mod.functions) {
        if (!on(fn) || fn.blocks.size() < 3) continue;
        std::set<std::string> done;
        for (size_t h = 1; h + 1 < fn.blocks.size(); h++) {
            Block& H = fn.blocks[h];
            if (done.count(H.label) || H.insts.empty() || H.insts.back().op != Op::BR_COND) continue;
            const Inst& bc = H.insts.back();
            // the latch: the last block branching back to H
            size_t latch = 0;
            for (size_t b = h + 1; b < fn.blocks.size(); b++) {
                const Block& B = fn.blocks[b];
                if (!B.insts.empty() && B.insts.back().op == Op::BR && B.insts.back().src1.name == H.label) latch = b;
            }
            if (!latch) continue;
            // the test goes to the next block (the body) and past the latch (out)
            std::map<std::string, size_t> at;
            for (size_t b = 0; b < fn.blocks.size(); b++) at[fn.blocks[b].label] = b;
            auto t = at.find(bc.dest.name), f = at.find(bc.src2.name);
            if (t == at.end() || f == at.end()) continue;
            size_t in = t->second, out = f->second;
            if (in != h + 1) std::swap(in, out);
            if (in != h + 1 || out <= latch) continue;
            // the block before falls into H: it branches there now
            Block& P = fn.blocks[h - 1];
            if (!ends(P)) {
                Inst br;
                br.op = Op::BR;
                br.src1 = Operand::label(H.label);
                br.loc = P.insts.empty() ? bc.loc : P.insts.back().loc;
                P.insts.push_back(br);
            }
            fn.blocks[latch].insts.pop_back();      // falls into the test
            done.insert(H.label);
            rotated.insert(fn.name);
            trace("rotate", fn.name, "@" + H.label + " after @" + fn.blocks[latch].label, bc.loc);
            Block moved = std::move(fn.blocks[h]);
            fn.blocks.erase(fn.blocks.begin() + h);
            fn.blocks.insert(fn.blocks.begin() + latch, std::move(moved));
            h--;    // (the block now at h)
        }
    }
    return rotated;
}

// Block reordering: the cold blocks (the branch of an if that __unlikely /
// __likely says is not expected to run) after the others, so the expected
// path falls through its branches. A block that falls into the next one
// gets a branch to it first (the last one: to the epilogue).
void optimizeBlockReorder(Module& mod) {
    auto ends = [](const Block& b) {
        if (b.insts.empty()) return false;
        switch (b.insts.back().op) {
            case Op::BR: case Op::BR_COND: case Op::BR_INDIRECT: case Op::SWITCH:
            case Op::RET: case Op::RET_VOID:
                return true;
            default:
                return false;
        }
    };
    for (auto& fn : mod.functions) {
        if (!runOn(fn, "BlockReordering") || fn.blocks.size() < 3) continue;
        // the entry block stays first; is a cold block before a hot one?
        bool cold = false, move = false;
        for (size_t i = 1; i < fn.blocks.size() && !move; i++) {
            if (fn.blocks[i].cold) cold = true;
            else if (cold) move = true;
        }
        if (!move) continue;
        for (size_t i = 0; i < fn.blocks.size(); i++) {
            if (ends(fn.blocks[i])) continue;
            Inst br;
            br.op = Op::BR;
            br.src1 = Operand::label(i + 1 < fn.blocks.size() ? fn.blocks[i + 1].label : "__return");
            if (!fn.blocks[i].insts.empty()) br.loc = fn.blocks[i].insts.back().loc;
            fn.blocks[i].insts.push_back(br);
        }
        std::vector<Block> hot, coldBlocks;
        for (size_t i = 0; i < fn.blocks.size(); i++)
            (i > 0 && fn.blocks[i].cold ? coldBlocks : hot).push_back(std::move(fn.blocks[i]));
        trace("reorder", fn.name, std::to_string(coldBlocks.size()) + " cold block(s) moved to the end");
        fn.blocks = std::move(hot);
        for (auto& b : coldBlocks) fn.blocks.push_back(std::move(b));
        fn.coldAtEnd = true;
    }
}

// Purity inference (-fipa-pure-const): a function of this module is const
// when it reads nothing but its parameters and its own locals, writes
// nothing but its own locals, always returns (no loops, no recursion) and
// calls only const functions; pure when it calls pure ones too. It never
// reads memory itself (the IR does not mark volatile loads: a function
// reading a hardware register must not be merged or hoisted), so it is pure
// only through the functions it calls (declared __pure / const, or
// inferred). The result joins mod.pureFunctions for CSE, LICM and the
// removal of unused calls.
void inferPurity(Module& mod) {
    std::map<std::string, Function*> defs;
    for (auto& fn : mod.functions)
        if (!fn.blocks.empty()) defs[fn.name] = &fn;
    // the functions that may qualify, each with its first estimate (2)
    std::map<std::string, int> level;
    for (auto& [name, fn] : defs) {
        if (!runOn(*fn, "PurityInference")) continue;
        if (fn->isInterrupt || fn->isNaked || fn->isVariadic || fn->isWeak || fn->isNested) continue;
        if (fn->returnType == Type::F32 || !fn->memoryVregs.empty()) continue;
        if (!fn->paramNames.empty() && fn->paramNames[0] == "__sret") continue;
        bool ok = true;
        std::map<std::string, size_t> idx;
        for (size_t b = 0; b < fn->blocks.size(); b++) idx[fn->blocks[b].label] = b;
        auto slot = [&](const Operand& o) { return o.isVreg() && fn->localSlotVregs.count(o.vregId); };
        for (size_t b = 0; b < fn->blocks.size() && ok; b++)
            for (const auto& in : fn->blocks[b].insts) {
                // (a branch back: a loop, which might not end)
                for (const Operand* o : {&in.dest, &in.src1, &in.src2})
                    if ((in.op == Op::BR || in.op == Op::BR_COND) && o->kind == OperandKind::LABEL) {
                        auto t = idx.find(o->name);
                        if (t == idx.end() || t->second <= b) ok = false;
                    }
                for (const auto& c : in.switchCases) {
                    auto t = idx.find(c.second);
                    if (t == idx.end() || t->second <= b) ok = false;
                }
                switch (in.op) {
                    case Op::CONST: case Op::ADD: case Op::SUB: case Op::MUL: case Op::DIV: case Op::MOD:
                    case Op::MUL_U: case Op::DIV_U: case Op::MOD_U: case Op::AND: case Op::OR: case Op::XOR:
                    case Op::LSL: case Op::LSR: case Op::ASR: case Op::SHL: case Op::SHR: case Op::NEG: case Op::NOT:
                    case Op::CMP_EQ: case Op::CMP_NE: case Op::CMP_LT: case Op::CMP_LE: case Op::CMP_GT:
                    case Op::CMP_GE: case Op::CMP_LTU: case Op::CMP_LEU: case Op::CMP_GTU: case Op::CMP_GEU:
                    case Op::SEXT: case Op::ZEXT: case Op::TRUNC: case Op::COPY: case Op::PHI: case Op::NOP:
                    case Op::BR: case Op::BR_COND: case Op::SWITCH: case Op::RET: case Op::RET_VOID:
                    case Op::ADDR_GLOBAL:   // (an address, passed on: reading through it is the callee's)
                        break;
                    case Op::LOAD: case Op::DEREF:    // only of its own variables
                        if (!slot(in.src1) || !in.args.empty()) ok = false;
                        break;
                    case Op::STORE:                   // only to its own variables
                        if (!slot(in.src2) || !in.args.empty()) ok = false;
                        break;
                    case Op::CALL: case Op::CALL_VOID:
                        if (in.src1.kind != OperandKind::GLOBAL) ok = false;
                        break;
                    default:
                        ok = false;
                }
                if (!ok) break;
            }
        if (ok) level[name] = 2;
    }
    // the fixed point: a function is at most what it calls
    for (bool changed = true; changed; ) {
        changed = false;
        for (auto& [name, lv] : level) {
            if (!lv) continue;
            int l = 2;
            for (const auto& blk : defs[name]->blocks)
                for (const auto& in : blk.insts) {
                    if (in.op != Op::CALL && in.op != Op::CALL_VOID) continue;
                    const std::string& callee = in.src1.name;
                    int c = 0;
                    auto d = mod.pureFunctions.find(callee);
                    if (d != mod.pureFunctions.end()) c = d->second;
                    auto inf = level.find(callee);
                    if (inf != level.end()) c = std::max(c, inf->second);
                    if (callee == name) c = 0;   // (recursion)
                    l = std::min(l, c);
                }
            if (l < lv) { lv = l; changed = true; }
        }
    }
    // (mutual recursion: a cycle of qualifying functions keeps its first
    // estimate above; such a function may not end, so it does not qualify)
    std::map<std::string, std::set<std::string>> calls;
    for (const auto& [name, lv] : level)
        for (const auto& blk : defs[name]->blocks)
            for (const auto& in : blk.insts)
                if ((in.op == Op::CALL || in.op == Op::CALL_VOID) && level.count(in.src1.name)) calls[name].insert(in.src1.name);
    for (auto& [name, lv] : level) {
        if (!lv) continue;
        std::set<std::string> seen;
        std::vector<std::string> todo(calls[name].begin(), calls[name].end());
        bool cycle = false;
        while (!todo.empty() && !cycle) {
            std::string n = todo.back(); todo.pop_back();
            if (n == name) cycle = true;
            else if (seen.insert(n).second) for (const auto& c : calls[n]) todo.push_back(c);
        }
        if (cycle) lv = 0;
    }
    for (const auto& [name, lv] : level) {
        if (!lv) continue;
        int& p = mod.pureFunctions[name];
        if (lv > p) {
            trace("purity", name, std::string("inferred ") + (lv == 2 ? "const" : "pure"));
            p = lv;
        }
    }
}

} // namespace ir
