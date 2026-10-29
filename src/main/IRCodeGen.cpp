#include "CompilerLog.hpp"
#include "IRCodeGen.hpp"
#include "OpEffect.hpp"
#include "IROptimizer.hpp"
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <set>

#include "CBMFloat.hpp"


static std::string hex8(uint8_t val) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)val;
    return ss.str();
}

static std::string hex16(uint16_t val) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (int)val;
    return ss.str();
}

// Whether assembler text names a symbol (not as part of a longer name)
static bool asmNamesSymbol(const std::string& text, const std::string& sym) {
    auto ident = [](char c) { return std::isalnum((unsigned char)c) || c == '_' || c == '.' || c == '@'; };
    for (size_t pos = text.find(sym); pos != std::string::npos; pos = text.find(sym, pos + 1)) {
        size_t end = pos + sym.size();
        if ((pos == 0 || !ident(text[pos - 1])) && (end == text.size() || !ident(text[end]))) return true;
    }
    return false;
}

// Whether the element address in __zp_scratch (elemCache_) stays right
// across an instruction: a fused element load or store (they handle it),
// a NOP, a store of a value to a variable in ZP, or a load / store through
// a pointer in ZP: none of them touches __zp_scratch; none may write the
// cached index variable
bool IRCodeGen::elemCacheSurvives(const ir::Inst& inst) {
    if (inst.op == ir::Op::NOP) return true;
    if (elemAddrFromCache(inst) >= 0) return true;   // reads __zp_scratch only
    bool fused = (inst.op == ir::Op::LOAD || inst.op == ir::Op::STORE) && inst.args.size() >= 3;
    if (fused) return true;
    auto inZp = [&](const ir::Operand& o) {
        return o.isVreg() && alloc_.getAlloc(o.vregId).loc == VRegAllocator::IN_ZP;
    };
    auto plainVal = [&](const ir::Operand& o) {   // loaded without scratch
        return o.isImm() || inZp(o) || (o.isVreg() && vregConstVal_.count(o.vregId));
    };
    if (inst.resultType == ir::Type::F32 || inst.resultType == ir::Type::I32 || inst.resultType == ir::Type::I_N)
        return false;
    if (inst.op == ir::Op::STORE && inst.args.empty() && inZp(inst.src2) && plainVal(inst.src1) &&
        inst.src2.vregId != elemCache_.index && !foldedAddrs_.count(inst.src2.vregId))
        return true;   // a variable in ZP, or *p with p in ZP
    if (inst.op == ir::Op::LOAD && inst.args.empty() && inZp(inst.src1) && !foldedAddrs_.count(inst.src1.vregId) &&
        inst.dest.isVreg() && inst.dest.vregId != elemCache_.index && inZp(inst.dest))
        return true;   // *p with p in ZP, into ZP
    return false;
}

// An element address the cache holds already (&a[j] after a load of a[j]):
// the offset from the cached one, else -1
int IRCodeGen::elemAddrFromCache(const ir::Inst& inst) {
    if (inst.op != ir::Op::ADDR_ELEM || !elemCache_.valid || inst.src1.kind != ir::OperandKind::GLOBAL ||
        inst.src1.name != elemCache_.base || !inst.src2.isVreg() || inst.src2.vregId != elemCache_.index ||
        !inst.dest.isVreg() || inst.dest.vregId == elemCache_.index || foldedAddrs_.count(inst.dest.vregId))
        return -1;
    int stride = inst.args.size() > 0 ? (int)inst.args[0].immVal : 2;
    int d = inst.elemOffset - elemCache_.offset;
    if (stride != elemCache_.stride || d < 0 || d > 255) return -1;
    if (alloc_.getAlloc(inst.dest.vregId).loc != VRegAllocator::IN_ZP) return -1;
    return d;
}

std::string IRCodeGen::globalBase(const ir::Operand& base, int offset) {
    if (offset == 0) return "#" + base.name;
    return "#(" + base.name + (offset > 0 ? "+" : "-") + std::to_string(offset > 0 ? offset : -offset) + ")";
}

void IRCodeGen::pushWordConst(int v) {
    emit("phw #$" + hex16((uint16_t)(((v & 0xFF) << 8) | ((v >> 8) & 0xFF))));
}

void IRCodeGen::pushWordAddr(const std::string& sym) {
    emit(".byte $F4, >" + sym + ", <" + sym, "phw #" + sym + " (little-endian on the stack)");
}

// A runtime alloca (variable size) lowers SP inside the function: it needs a
// stack frame and FP so the epilogue can reset SP, and can't use SAC.
static bool callsRuntimeAlloca(const ir::Function& fn) {
    for (const auto& blk : fn.blocks)
        for (const auto& inst : blk.insts)
            if (inst.op == ir::Op::CALL && inst.src1.kind == ir::OperandKind::GLOBAL &&
                (inst.src1.name == "_alloca" || inst.src1.name == "___builtin_alloca"))
                return true;
    return false;
}

static std::string hex32(uint32_t val) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << (int)val;
    return ss.str();
}

IRCodeGen::IRCodeGen(std::ostream& out) : out_(out) {}

void IRCodeGen::emitF32Byte(const std::string& op, uint32_t vregId, int i) {
    auto a = alloc_.getAlloc(vregId);
    std::string idx = "+" + std::to_string(i);
    if (a.loc == VRegAllocator::IN_ZP) {
        emit(op + " $" + hex8((uint8_t)a.offset) + idx);
    } else if (currentFunctionUseSAC_) {
        std::string sym;
        if (currentFn_ && vregId < currentFn_->paramTypes.size()) {
            std::string pName = (vregId < currentFn_->paramNames.size() && !currentFn_->paramNames[vregId].empty())
                ? currentFn_->paramNames[vregId] : std::to_string(vregId);
            sym = currentFunctionName_ + "__param_" + pName;
        } else {
            sym = currentFunctionName_ + "__local_" + std::to_string(vregId);
        }
        emit(op + " " + sym + idx);
    } else {
        int off = vregOffset_.count(vregId) ? vregOffset_[vregId] : a.offset;
        emit("ldy #" + std::to_string(off + i));
        emit(op + " ($fd),y");
    }
}

// The pointer for a folded base + offset address: the base's ZP pair, or the
// base copied to scratch; yOff = the offset
std::string IRCodeGen::foldedPtr(uint32_t addrVreg, const std::string& scratch, int& yOff) {
    const auto& fa = foldedAddrs_.at(addrVreg);
    yOff = fa.offset;
    auto ba = alloc_.getAlloc(fa.base);
    if (ba.loc == VRegAllocator::IN_ZP) return "$" + hex8((uint8_t)ba.offset);
    loadVreg(fa.base);
    emit("sta " + scratch);
    emit("stx " + scratch + "+1");
    return scratch;
}

// Y for an element address kept as base+off,y (elemAbsY_): the byte index,
// or index * stride; the operand
std::string IRCodeGen::elemYOperand(const ElemY& e) {
    if (e.base.isVreg()) {
        // a pointer in the base page: (p),y
        std::string p = "$" + hex8((uint8_t)alloc_.getAlloc(e.base.vregId).offset);
        emit("ldy " + directBase(e.index.vregId), "the index ((p),y: no address built)");
        ms_.invalidateAll();
        elemCache_.valid = false;
        return "(" + p + ")";
    }
    if (e.stride == 1 && alloc_.getAlloc(e.index.vregId).type == ir::Type::I8) {
        emit("ldy " + directBase(e.index.vregId), "the index (" + e.base.name + "+off,y: no address built)");
    } else {
        loadOperand(e.index);
        for (int k = e.stride; k > 1; k >>= 1) emit("asl");
        emit("tay", "index * " + std::to_string(e.stride) + " (" + e.base.name + " is at most 256 bytes)");
    }
    ms_.invalidateAll();
    elemCache_.valid = false;
    return e.elemOffset ? e.base.name + "+" + std::to_string(e.elemOffset) : e.base.name;
}

// ln45's program-wide plan of base-page block variables (-fzp-plan=FILE,
// CC45_ZP_PLAN; #304): "fn <function> <storage symbol>..." lines, every
// function ln45 planned (perhaps with none). Read once; a file that cannot be
// read is a warning (the local rule then)
const std::map<std::string, std::set<std::string>>& IRCodeGen::zpPlan() {
    static std::map<std::string, std::set<std::string>> plan;
    static bool loaded = false;
    if (loaded) return plan;
    loaded = true;
    const char* path = getenv("CC45_ZP_PLAN");
    if (!path || !*path) return plan;
    std::ifstream in(path);
    if (!in) {
        std::cerr << "cc45: warning: cannot read the base-page plan '" << path << "' (-fzp-plan): ignored" << std::endl;
        return plan;
    }
    for (std::string line; std::getline(in, line); ) {
        std::istringstream ls(line);
        std::string kw, fnName, sym;
        if (!(ls >> kw >> fnName) || kw != "fn") continue;
        auto& vars = plan[fnName];
        while (ls >> sym) vars.insert(sym);
    }
    return plan;
}

// SAC storage symbol of a frame vreg: fn__param_<name> for parameters, else fn__local_<id>
std::string IRCodeGen::sacSymbol(uint32_t vregId) const {
    if (currentFn_ && vregId < currentFn_->paramTypes.size()) {
        std::string pName = (vregId < currentFn_->paramNames.size() && !currentFn_->paramNames[vregId].empty())
            ? currentFn_->paramNames[vregId] : std::to_string(vregId);
        return currentFunctionName_ + "__param_" + pName;
    }
    return currentFunctionName_ + "__local_" + std::to_string(vregId);
}

// A call's (ZP convention) arguments computed in their slots of the
// parameter block instead of a temporary copied there before the call:
// an argument vreg defined once, in the call's block, used only by the
// call, with only instructions between that keep the parameter block (no
// call, no division or float code, no inline asm: they can use $10-$2F)
void IRCodeGen::findArgTargets(const ir::Function& fn) {
    argTargets_.clear();
    if (!zpParams_.empty()) return;     // (the function's own parameters stay there)
    std::map<uint32_t, int> defs, uses;
    for (const auto& blk : fn.blocks)
        for (const auto& in : blk.insts) {
            if (in.dest.isVreg()) defs[in.dest.vregId]++;
            for (const ir::Operand* o : {&in.src1, &in.src2}) if (o->isVreg()) uses[o->vregId]++;
            for (const auto& a : in.args) if (a.isVreg()) uses[a.vregId]++;
            for (const auto& ph : in.phiIncoming) if (ph.first.isVreg()) uses[ph.first.vregId] += 2;
        }
    auto intType = [](ir::Type t) {
        return t == ir::Type::I8 || t == ir::Type::I16 || t == ir::Type::PTR || t == ir::Type::I32;
    };
    // code that leaves $10-$2F alone
    auto safe = [&](const ir::Inst& x) {
        switch (x.op) {
            case ir::Op::NOP: return true;
            case ir::Op::CONST: case ir::Op::COPY: case ir::Op::ZEXT: case ir::Op::SEXT: case ir::Op::TRUNC:
            case ir::Op::ADD: case ir::Op::SUB: case ir::Op::AND: case ir::Op::OR: case ir::Op::XOR:
            case ir::Op::LSL: case ir::Op::LSR: case ir::Op::ASR: case ir::Op::SHL: case ir::Op::SHR:
            case ir::Op::NEG: case ir::Op::NOT:
            case ir::Op::CMP_EQ: case ir::Op::CMP_NE: case ir::Op::CMP_LT: case ir::Op::CMP_LE: case ir::Op::CMP_GT:
            case ir::Op::CMP_GE: case ir::Op::CMP_LTU: case ir::Op::CMP_LEU: case ir::Op::CMP_GTU: case ir::Op::CMP_GEU:
            case ir::Op::LOAD: case ir::Op::STORE:
            case ir::Op::ADDR_GLOBAL: case ir::Op::ADDR_LOCAL: case ir::Op::ADDR_ELEM:
                return x.resultType == ir::Type::VOID || intType(x.resultType);
            default: return false;
        }
    };
    for (const auto& blk : fn.blocks) {
        const auto& insts = blk.insts;
        for (size_t c = 0; c < insts.size(); c++) {
            const auto& call = insts[c];
            if ((call.op != ir::Op::CALL && call.op != ir::Op::CALL_VOID) || call.callConv != ir::CallConv::ZP)
                continue;
            int zpOff = (int)zeroPageStart_ + 8;
            for (size_t k = 0; k < call.args.size(); k++) {
                const auto& arg = call.args[k];
                int ps = ir::typeSize(arg.type);
                int slot = ps < 2 ? 2 : ps;
                int at = zpOff;
                zpOff += slot;
                if (call.isRegparm && k == 0) continue;
                if (!arg.isVreg() || !intType(arg.type)) continue;
                uint32_t v = arg.vregId;
                if (v < fn.paramTypes.size() || defs[v] != 1 || uses[v] != 1 || fn.localSlotVregs.count(v) ||
                    fn.memoryVregs.count(v) || argTargets_.count(v)) continue;
                // its definition in this block, before the call, and safe from there on
                size_t d = c;
                for (size_t i = 0; i < c; i++)
                    if (insts[i].dest.isVreg() && insts[i].dest.vregId == v) d = i;
                if (d == c || insts[d].op == ir::Op::CONST || !intType(insts[d].resultType)) continue;
                bool ok = true;
                for (size_t i = d; i < c && ok; i++) ok = safe(insts[i]);
                if (ok) argTargets_[v] = at;
            }
        }
    }
}

// A value whose one use stores it into a local kept in the base page (its
// zarVregs_ home): computed there instead of in a temporary copied over.
// Nothing from its definition to the store may mention the local (a read
// would see the new value early, a write would be lost), and the local's
// address is never taken
void IRCodeGen::findStoreTargets(const ir::Function& fn) {
    storeTargets_.clear();
    if (zarVregs_.empty()) return;
    std::map<uint32_t, int> defs, uses;
    for (const auto& blk : fn.blocks)
        for (const auto& in : blk.insts) {
            if (in.dest.isVreg()) defs[in.dest.vregId]++;
            for (const ir::Operand* o : {&in.src1, &in.src2}) if (o->isVreg()) uses[o->vregId]++;
            for (const auto& a : in.args) if (a.isVreg()) uses[a.vregId]++;
            for (const auto& ph : in.phiIncoming) if (ph.first.isVreg()) uses[ph.first.vregId] += 2;
        }
    auto mentions = [](const ir::Inst& x, uint32_t v) {
        if (x.dest.isVreg() && x.dest.vregId == v) return true;
        for (const ir::Operand* o : {&x.src1, &x.src2}) if (o->isVreg() && o->vregId == v) return true;
        for (const auto& a : x.args) if (a.isVreg() && a.vregId == v) return true;
        for (const auto& ph : x.phiIncoming) if (ph.first.isVreg() && ph.first.vregId == v) return true;
        return false;
    };
    for (const auto& blk : fn.blocks) {
        const auto& insts = blk.insts;
        for (size_t s = 0; s < insts.size(); s++) {
            const auto& st = insts[s];
            if (st.op != ir::Op::STORE || !st.args.empty() || !st.src1.isVreg() || !st.src2.isVreg()) continue;
            uint32_t t = st.src1.vregId, l = st.src2.vregId;
            auto home = zarVregs_.find(l);
            if (home == zarVregs_.end() || !fn.localSlotVregs.count(l) || fn.memoryVregs.count(l)) continue;
            if (t < fn.paramTypes.size() || defs[t] != 1 || uses[t] != 1 || fn.localSlotVregs.count(t) ||
                fn.memoryVregs.count(t) || argTargets_.count(t) || storeTargets_.count(t)) continue;
            auto vt = fn.vregTypes.find(t), lt = fn.vregTypes.find(l);
            if (vt == fn.vregTypes.end() || lt == fn.vregTypes.end() ||
                ir::typeSize(vt->second) != ir::typeSize(lt->second) || ir::typeSize(st.resultType) != ir::typeSize(lt->second) ||
                vt->second == ir::Type::F32)
                continue;
            size_t d = s;
            for (size_t i = 0; i < s; i++)
                if (insts[i].dest.isVreg() && insts[i].dest.vregId == t) d = i;
            if (d == s || insts[d].op == ir::Op::CONST || insts[d].op == ir::Op::CALL ||
                insts[d].op == ir::Op::CALL_VOID || insts[d].op == ir::Op::CALL_INDIRECT) continue;
            bool ok = true;
            for (size_t i = d; i < s && ok; i++) ok = !mentions(insts[i], l) && insts[i].op != ir::Op::ASM_INLINE;
            if (ok) storeTargets_[t] = home->second;
        }
    }
}

std::string IRCodeGen::directBase(uint32_t vregId) const {
    auto ga = globalLoadAlias_.find(vregId);
    if (ga != globalLoadAlias_.end()) return ga->second;
    auto a = alloc_.getAlloc(vregId);
    if (a.loc == VRegAllocator::IN_ZP) return "$" + hex8((uint8_t)a.offset);
    if (a.loc == VRegAllocator::IN_FRAME && currentFunctionUseSAC_ && vregOffset_.count(vregId))
        return sacSymbol(vregId);
    return "";
}

std::string IRCodeGen::directByte(const ir::Operand& op, int k) const {
    auto imm = [k](int64_t v) { return "#" + std::to_string((int)((v >> (8 * k)) & 0xFF)); };
    if (op.isImm()) return imm(op.immVal);
    if (!op.isVreg()) return "";
    auto za = zextAlias_.find(op.vregId);
    if (za != zextAlias_.end()) return k == 0 ? directByte(za->second, 0) : "#0";
    auto cit = vregConstVal_.find(op.vregId);
    if (cit != vregConstVal_.end() && !localSlotVregs_.count(op.vregId)) return imm(cit->second);
    std::string base = directBase(op.vregId);
    if (base.empty()) return "";
    if (k == 0) return base;
    if (base[0] == '$') return "$" + hex8((uint8_t)(std::stoi(base.substr(1), nullptr, 16) + k));
    return base + "+" + std::to_string(k);
}

// A 16/32-bit vreg whose bytes can all be read where it lives (not just
// stored from A/X, not a constant): tested by OR-ing them in place
bool IRCodeGen::directTest(const ir::Operand& op) const {
    if (!op.isVreg() || justStored(op.vregId)) return false;
    auto a = alloc_.getAlloc(op.vregId);
    int nb = a.type == ir::Type::I32 ? 4 : (a.type == ir::Type::I16 || a.type == ir::Type::PTR) ? 2 : 0;
    if (!nb) return false;
    for (int k = 0; k < nb; k++) {
        std::string b = directByte(op, k);
        if (b.empty() || b[0] == '#') return false;
    }
    return true;
}

std::map<uint32_t, IRCodeGen::ByteJoin> IRCodeGen::findByteJoins(const ir::Function& fn) const {
    std::map<uint32_t, ByteJoin> found;
    auto width = [](ir::Type t) {
        return t == ir::Type::I8 ? 1 : (t == ir::Type::I16 || t == ir::Type::PTR) ? 2 : t == ir::Type::I32 ? 4 : 0;
    };
    // definitions and uses of each vreg
    std::map<uint32_t, int> defs, uses;
    for (const auto& blk : fn.blocks)
        for (const auto& in : blk.insts) {
            if (in.dest.isVreg()) defs[in.dest.vregId]++;
            for (const ir::Operand* o : {&in.src1, &in.src2}) if (o->isVreg()) uses[o->vregId]++;
            for (const auto& a : in.args) if (a.isVreg()) uses[a.vregId]++;
            for (const auto& ph : in.phiIncoming) if (ph.first.isVreg()) uses[ph.first.vregId] += 2;
        }
    std::set<uint32_t> taken;   // nodes of a tree found already
    for (const auto& blk : fn.blocks) {
        const auto& insts = blk.insts;
        std::map<uint32_t, size_t> at;   // vreg defined in this block (single definition) → index
        for (size_t i = 0; i < insts.size(); i++)
            if (insts[i].dest.isVreg() && defs[insts[i].dest.vregId] == 1) at[insts[i].dest.vregId] = i;
        for (size_t r = insts.size(); r-- > 0;) {
            const auto& root = insts[r];
            if (root.op != ir::Op::OR || !root.dest.isVreg() || taken.count(root.dest.vregId) ||
                defs[root.dest.vregId] != 1 || fn.localSlotVregs.count(root.dest.vregId) ||
                fn.memoryVregs.count(root.dest.vregId) || width(root.resultType) < 2 || !root.args.empty())
                continue;
            ByteJoin bj;
            std::set<uint32_t> leaves;
            std::map<uint32_t, size_t> leafUser;   // the first node reading a leaf
            size_t first = r;
            // the bytes of op (w bytes wide) as consumed by instruction user
            std::function<bool(const ir::Operand&, int, size_t, std::vector<ByteSrc>&)> bytesOf;
            bytesOf = [&](const ir::Operand& op, int w, size_t user, std::vector<ByteSrc>& out) -> bool {
                out.assign(w, ByteSrc{});
                if (op.isImm()) {
                    for (int k = 0; k < w; k++) {
                        int v = (int)((op.immVal >> (8 * k)) & 0xFF);
                        out[k] = v ? ByteSrc{2, 0, v} : ByteSrc{};
                    }
                    return true;
                }
                if (!op.isVreg()) return false;
                uint32_t v = op.vregId;
                auto it = at.find(v);
                bool node = it != at.end() && it->second < user && uses[v] == 1 && !taken.count(v) &&
                            !fn.localSlotVregs.count(v) && !fn.memoryVregs.count(v) && insts[it->second].args.empty();
                if (node) {
                    auto savedNodes = bj.nodes; auto savedLeaves = leaves; auto savedUser = leafUser;
                    size_t savedFirst = first;
                    const auto& in = insts[it->second];
                    int iw = width(in.resultType);
                    std::vector<ByteSrc> a, b;
                    bool ok = iw == w;
                    if (ok) switch (in.op) {
                        case ir::Op::OR:
                            ok = bytesOf(in.src1, w, it->second, a) && bytesOf(in.src2, w, it->second, b);
                            for (int k = 0; ok && k < w; k++) {
                                if (a[k].kind && b[k].kind) ok = false;
                                else out[k] = a[k].kind ? a[k] : b[k];
                            }
                            break;
                        case ir::Op::ZEXT: {
                            int sw = width(in.src1.type);
                            ok = sw > 0 && sw < w && bytesOf(in.src1, sw, it->second, a);
                            for (int k = 0; ok && k < sw; k++) out[k] = a[k];
                            break;
                        }
                        case ir::Op::TRUNC: {
                            int sw = width(in.src1.type);
                            ok = sw > w && bytesOf(in.src1, sw, it->second, a);
                            for (int k = 0; ok && k < w; k++) out[k] = a[k];
                            break;
                        }
                        case ir::Op::SHL: case ir::Op::LSL:
                        case ir::Op::SHR: case ir::Op::LSR: {
                            ok = in.src2.isImm() && in.src2.immVal > 0 && in.src2.immVal % 8 == 0 &&
                                 bytesOf(in.src1, w, it->second, a);
                            int n = ok ? (int)(in.src2.immVal / 8) : 0;
                            bool left = in.op == ir::Op::SHL || in.op == ir::Op::LSL;
                            for (int k = 0; ok && k < w; k++) {
                                int s = left ? k - n : k + n;
                                if (s >= 0 && s < w) out[k] = a[s];
                            }
                            break;
                        }
                        case ir::Op::AND:
                            ok = in.src2.isImm() && bytesOf(in.src1, w, it->second, a);
                            for (int k = 0; ok && k < w; k++) {
                                int m = (int)((in.src2.immVal >> (8 * k)) & 0xFF);
                                if (m == 0xFF) out[k] = a[k];
                                else if (m != 0) ok = false;
                            }
                            break;
                        default:
                            ok = false;
                    }
                    if (ok) {
                        bj.nodes.insert(v);
                        first = std::min(first, it->second);
                        return true;
                    }
                    bj.nodes = savedNodes; leaves = savedLeaves; leafUser = savedUser; first = savedFirst;
                    out.assign(w, ByteSrc{});
                }
                // a leaf: its bytes as they are
                if (width(op.type) != w) return false;
                leaves.insert(v);
                if (!leafUser.count(v) || leafUser[v] > user) leafUser[v] = user;
                for (int k = 0; k < w; k++) out[k] = ByteSrc{1, v, k};
                return true;
            };
            std::vector<ByteSrc> a, b;
            int w = width(root.resultType);
            if (!bytesOf(root.src1, w, r, a) || !bytesOf(root.src2, w, r, b) || bj.nodes.empty()) continue;
            bool ok = true;
            bj.bytes.assign(w, ByteSrc{});
            for (int k = 0; k < w && ok; k++) {
                if (a[k].kind && b[k].kind) ok = false;
                else bj.bytes[k] = a[k].kind ? a[k] : b[k];
            }
            // between the first node and the root: nothing that writes memory
            // or a leaf
            for (size_t i = first; i < r && ok; i++) {
                const auto& in = insts[i];
                if (in.dest.isVreg() && bj.nodes.count(in.dest.vregId)) continue;
                // (a leaf keeps its value from the node reading it to the root)
                if (in.dest.isVreg() && leaves.count(in.dest.vregId) && i >= leafUser[in.dest.vregId]) ok = false;
                switch (in.op) {
                    case ir::Op::CONST: case ir::Op::ADD: case ir::Op::SUB: case ir::Op::AND: case ir::Op::OR:
                    case ir::Op::XOR: case ir::Op::SHL: case ir::Op::SHR: case ir::Op::LSL: case ir::Op::LSR:
                    case ir::Op::ASR: case ir::Op::NEG: case ir::Op::NOT: case ir::Op::ZEXT: case ir::Op::SEXT:
                    case ir::Op::TRUNC: case ir::Op::COPY: case ir::Op::NOP: case ir::Op::ADDR_GLOBAL:
                        break;
                    case ir::Op::LOAD:
                        if (!in.args.empty()) ok = false;
                        break;
                    default:
                        ok = false;
                }
            }
            if (!ok) continue;
            taken.insert(bj.nodes.begin(), bj.nodes.end());
            found[root.dest.vregId] = bj;
        }
    }
    return found;
}

bool IRCodeGen::f32AbsBase(uint32_t vregId, std::string& base) {
    auto a = alloc_.getAlloc(vregId);
    if (a.loc == VRegAllocator::IN_ZP) { base = "$" + hex8((uint8_t)a.offset); return true; }
    if (!currentFunctionUseSAC_) return false;
    if (currentFn_ && vregId < currentFn_->paramTypes.size()) {
        std::string pName = (vregId < currentFn_->paramNames.size() && !currentFn_->paramNames[vregId].empty())
            ? currentFn_->paramNames[vregId] : std::to_string(vregId);
        base = currentFunctionName_ + "__param_" + pName;
    } else {
        base = currentFunctionName_ + "__local_" + std::to_string(vregId);
    }
    return true;
}

void IRCodeGen::emitF32CopyAbs(const std::string& src, const std::string& dst) {
    ms_.invalidateAll();
    std::string lbl = "@__fcpy_" + std::to_string(labelCounter_++);
    // X-indexed: LDA/STA have no base-page,Y mode, so "$20,y" would assemble
    // as absolute $0020,y and miss the relocated base page.
    emit("ldx #4");
    emit(lbl + ":");
    emit(".bpindex 5", "(the base-page bytes the copy reaches: ln45's cc45.bptop)");
    emit("lda " + src + ",x");
    emit("sta " + dst + ",x");
    emit(".bpindex 0");
    emit("dex");
    emit("bpl " + lbl);
}

void IRCodeGen::emitF32VregToAbs(uint32_t vregId, const std::string& dst) {
    std::string base;
    if (f32AbsBase(vregId, base)) { emitF32CopyAbs(base, dst); return; }
    ms_.invalidateAll();
    for (int b = 0; b < 5; b++) { emitF32Byte("lda", vregId, b); emit("sta " + dst + "+" + std::to_string(b)); }
}

void IRCodeGen::emitF32AbsToVreg(const std::string& src, uint32_t vregId) {
    std::string base;
    if (f32AbsBase(vregId, base)) { emitF32CopyAbs(src, base); return; }
    ms_.invalidateAll();
    for (int b = 0; b < 5; b++) { emit("lda " + src + "+" + std::to_string(b)); emitF32Byte("sta", vregId, b); }
}

const ir::Inst* IRCodeGen::peekNextInst() const {
    if (!currentFn_) return nullptr;
    const auto& block = currentFn_->blocks[currentBlockIdx_];
    if (currentInstInBlock_ + 1 < block.insts.size())
        return &block.insts[currentInstInBlock_ + 1];
    return nullptr;
}

void IRCodeGen::emit(const std::string& line, const std::string& reason) {
    if (!sectLink_.empty()) {
        std::string rw = rewriteSectNames(line);
        if (rw != line) { emit(rw, reason); return; }
    }
    if (!zpPlace_.empty()) {
        std::string rw = rewriteZpOperands(line);
        if (rw != line) { emit(rw, reason); return; }
    }
    // the @labels this line names (a block label nothing names is not emitted)
    for (size_t at = line.find('@'); at != std::string::npos; at = line.find('@', at + 1)) {
        size_t e = at + 1;
        while (e < line.size() && (std::isalnum((unsigned char)line[e]) || line[e] == '_')) e++;
        if (e > at + 1) emittedLabelRefs_.insert(line.substr(at + 1, e - at - 1));
    }
    if (staleFlags_ && line.size() >= 3) {
        std::string m = line.substr(0, 3);
        std::transform(m.begin(), m.end(), m.begin(), ::tolower);
        if ((m == "beq" || m == "bne" || m == "bmi" || m == "bpl") &&
            (line.size() == 3 || line[3] == ' ' || line[3] == '\t'))
            restoreFlags();
    }
    if (line.rfind(".segment", 0) == 0)
        inBss_ = line.find("\"bss\"") != std::string::npos;
    else if (inBss_ && !line.empty() && line[0] != ';' &&
             line.rfind(".global", 0) != 0 && line.rfind(".local", 0) != 0)
        bssBoundaryClean_ = false;
    if (line.find("__zp_scratch") != std::string::npos || line.rfind("jsr", 0) == 0 ||
        line.rfind("jmp", 0) == 0)
        scratchTouched_ = true;
    else if (line[0] != '.' && line[0] != ';') {   // a simulated instruction may use scratch
        std::string m = line.substr(0, line.find_first_of(" \t"));
        if (m.find('.') != std::string::npos && m != "cmp.16") scratchTouched_ = true;
    }
    out_ << "    " << line;
    if (emitReasons_ && !reason.empty()) {
        out_ << "  ; [" << reason << "]";
    }
    out_ << "\n";
    noteEmitted(line);

    // Phase 78: Track instruction offset for SMC parameter access recording
    // Estimate instruction size based on addressing mode patterns
    if (trackSMCOffsetsEnabled_ && !line.empty() && line[0] != '.' && line[0] != ';') {
        // Heuristic instruction size estimation:
        // - If contains '#': likely immediate (2 bytes)
        // - If contains '$' and ':': likely absolute address (3 bytes)
        // - If contains '$': likely ZP (2 bytes)
        // - Otherwise: likely implied (1 byte)
        // This is conservative; actual size will be refined by assembler
        uint32_t estimatedSize = 1;  // Default to 1 byte (implied)

        if (line.find('#') != std::string::npos) {
            estimatedSize = 2;  // Immediate
        } else if (line.find("$") != std::string::npos) {
            // Check for absolute addressing (16-bit address)
            size_t dollarPos = line.find('$');
            if (dollarPos != std::string::npos && dollarPos + 5 < line.length()) {
                // If we see 4 hex digits or more, it's likely absolute
                std::string after = line.substr(dollarPos + 1, 4);
                if (after.find_first_not_of("0123456789ABCDEFabcdef") == std::string::npos &&
                    after.length() >= 4) {
                    estimatedSize = 3;  // Absolute address
                } else {
                    estimatedSize = 2;  // ZP address
                }
            } else {
                estimatedSize = 2;  // ZP address
            }
        }

        currentInstructionOffset_ += estimatedSize;
    }

    // Auto-update MachineState for simulated ops (contain '.' in mnemonic).
    // Native instructions are handled with precise ms_ updates by the caller.
    // Simulated ops (like stax.fp, add.16, mul.s16) need table-driven invalidation
    // because the caller can't know what the expansion will clobber.
    if (!line.empty() && line[0] != '.' && line[0] != ';') {
        // Check if this is a simulated op (contains '.' in the mnemonic portion)
        auto space = line.find(' ');
        std::string mnem = (space != std::string::npos) ? line.substr(0, space) : line;
        if (mnem.find('.') != std::string::npos) {
            const OpEffect& effect = getOpEffect(mnem);
            effect.apply(ms_);
        }
    }
}

// __zp globals of relocatable output: a placeholder base-page address each
// (in name order), from the end of the largest .zar block a function can
// have up to $F1 (above: the 32-bit temp and library temps, whose
// operands must not read as a variable's)
void IRCodeGen::placeZpGlobals(const ir::Module& mod) {
    zpPlace_.clear();
    zpLink_.clear();
    if (!relocMode_) return;
    std::map<std::string, std::pair<int, bool>> vars;   // name -> size, static
    for (const auto& g : mod.globals)
        if (g.addressSpace == 1) vars[g.name] = {g.size > 0 ? g.size : ir::typeSize(g.type), g.isStatic};
    for (const auto& [name, size] : mod.zpExterns)
        if (!vars.count(name)) vars[name] = {size, false};
    zarPlanLimit_ = ZAR_END - ZAR_START;
    if (vars.empty()) return;
    int zarMax = ZAR_MAX;
    if (const char* m = getenv("CC45_ZP_LOCALS_MAX")) zarMax = std::max(0, std::min(atoi(m), ZAR_END - ZAR_START));
    if (getenv("CC45_NO_ZP_LOCALS")) zarMax = 0;
    // a static one's symbol: name__<file> (as a static function's)
    std::string tag;
    {
        std::string f = mod.sourceFile;
        size_t sl = f.find_last_of('/');
        if (sl != std::string::npos) f = f.substr(sl + 1);
        size_t dot = f.find_last_of('.');
        if (dot != std::string::npos) f = f.substr(0, dot);
        for (char c : f) tag += std::isalnum((unsigned char)c) ? c : '_';
    }
    int at = ZAR_START + zarMax;
    // with ln45's plan (-fzp-plan) a block can be larger than the cap: the
    // variables go at the top of the range and planned blocks stop below them
    if (!zpPlan().empty()) {
        int total = 0;
        for (const auto& [name, v] : vars) total += std::max(v.first, 0);
        at = std::max(ZAR_START + zarMax, ZAR_END - total);
        zarPlanLimit_ = at - ZAR_START;
    }
    for (const auto& [name, v] : vars) {
        const std::string clean = name.size() > 1 && name[0] == '_' ? name.substr(1) : name;
        if (v.first <= 0)
            throw std::runtime_error("__zp variable '" + clean + "' needs a size (an extern __zp array must give its length)");
        if (at + v.first > ZAR_END)
            throw std::runtime_error("__zp variables: '" + clean + "' does not fit; one file can use " +
                                     std::to_string(ZAR_END - ZAR_START - zarMax) + " bytes of them (less with a larger -fzp-locals-max)");
        zpPlace_[name] = {at, v.first};
        if (v.second) zpLink_[name] = name + "__" + tag;
        at += v.first;
    }
}

// __section variables (Issue #70): each global's part of its section in
// this object, in declaration order; a static one's symbol is name__<file>
// (ln45 defines them all)
void IRCodeGen::placeSections(const ir::Module& mod) {
    sectPlace_.clear();
    sectLink_.clear();
    sectSize_.clear();
    std::map<std::string, size_t> last;
    for (size_t i = 0; i < mod.globals.size(); i++) last[mod.globals[i].name] = i;
    std::string tag;
    {
        std::string f = mod.sourceFile;
        size_t sl = f.find_last_of('/');
        if (sl != std::string::npos) f = f.substr(sl + 1);
        size_t dot = f.find_last_of('.');
        if (dot != std::string::npos) f = f.substr(0, dot);
        for (char c : f) tag += std::isalnum((unsigned char)c) ? c : '_';
    }
    for (size_t i = 0; i < mod.globals.size(); i++) {
        const auto& g = mod.globals[i];
        if (g.section.empty() || last[g.name] != i) continue;
        int size = g.size > 0 ? g.size : ir::typeSize(g.type);
        int& at = sectSize_[g.section];
        sectPlace_[g.name] = {g.section, at, size};
        at += size;
        if (g.isStatic && relocMode_) sectLink_[g.name] = g.name + "__" + tag;
        if (!relocMode_) {
            // -S: the address from the file's #pragma section
            auto a = mod.sections.find(g.section);
            if (a == mod.sections.end() || a->second < 0)
                throw std::runtime_error("section '" + g.section + "' has no address in the file (-S output is not linked: "
                                         "#pragma section(\"" + g.section + "\", address))");
            static std::set<std::string> warned;   // (the code is generated twice: loop rotation)
            if (g.hasInitValue && warned.insert(g.name).second)
                std::cerr << "cc45: warning: '" << g.name.substr(1) << "': -S output does not copy a section variable's initial value\n";
        }
    }
}

// a static __section variable's name -> its file-unique symbol (also in
// a .dbg_var record's sym:)
std::string IRCodeGen::rewriteSectNames(const std::string& line) const {
    if (line.find(".dbg_var") != std::string::npos) {
        std::string out = line;
        for (const auto& [name, link] : sectLink_) {
            size_t p = out.find("sym:" + name + "\"");
            if (p != std::string::npos) out.replace(p + 4, name.size(), link);
        }
        return out;
    }
    auto isId = [](char c) { return std::isalnum((unsigned char)c) || c == '_' || c == '.'; };
    std::string out;
    size_t i = 0;
    while (i < line.size()) {
        char c = line[i];
        if (c == '"') {
            size_t e = line.find('"', i + 1);
            e = e == std::string::npos ? line.size() : e + 1;
            out += line.substr(i, e - i);
            i = e;
            continue;
        }
        if (c == ';') { out += line.substr(i); break; }
        if (!isId(c) || (i > 0 && isId(line[i - 1]))) { out += c; i++; continue; }
        size_t e = i;
        while (e < line.size() && isId(line[e])) e++;
        std::string id = line.substr(i, e - i);
        auto it = sectLink_.find(id);
        out += it == sectLink_.end() ? id : it->second;
        i = e;
    }
    return out;
}

std::string IRCodeGen::rewriteZpOperands(const std::string& line) const {
    size_t b = line.find_first_not_of(" \t");
    if (b == std::string::npos || line[b] == ';') return line;
    // a directive (.word _x) or a label: only a static variable's name changes
    bool operandsToo = line[b] != '.' && line.substr(0, line.find(';')).find(':') == std::string::npos;
    std::string out;
    size_t i = 0;
    // the mnemonic stays (and, after it, each identifier is a name)
    if (operandsToo) {
        size_t e = line.find_first_of(" \t", b);
        if (e == std::string::npos) return line;
        out = line.substr(0, e);
        i = e;
    }
    auto isId = [](char c) { return std::isalnum((unsigned char)c) || c == '_' || c == '.'; };
    while (i < line.size()) {
        char c = line[i];
        if (c == '"') {   // a string: as it is
            size_t e = line.find('"', i + 1);
            e = e == std::string::npos ? line.size() : e + 1;
            out += line.substr(i, e - i);
            i = e;
            continue;
        }
        if (!isId(c) || (i > 0 && isId(line[i - 1]))) { out += c; i++; continue; }
        size_t e = i;
        while (e < line.size() && isId(line[e])) e++;
        std::string id = line.substr(i, e - i);
        auto it = zpPlace_.find(id);
        if (it == zpPlace_.end()) { out += id; i = e; continue; }
        // an immediate (#<_x, #>_x) is the variable's address: its symbol
        size_t p = out.find_last_not_of(" \t<>");
        bool address = !operandsToo || (p != std::string::npos && out[p] == '#');
        if (address) out += zpLinkName(id);
        else { char h[8]; snprintf(h, sizeof h, "$%02X", it->second.addr); out += h; }
        i = e;
    }
    return out;
}

// (__zp globals' operands are rewritten by emit: rewriteZpOperands)
void IRCodeGen::emitOptimized(const std::string& line, const std::string& reason) {
    emit(line, reason);
}

// Is vreg id read by any instruction of the current function other than
// a and b? (the inc/dec peephole leaves the incremented value unloaded)
bool IRCodeGen::tryFusedRMW(const ir::Inst& load, const std::string& zpPair, int yOff) {
    if (!currentFn_ || !load.dest.isVreg() || !load.src1.isVreg()) return false;
    ir::Type T = load.resultType;
    int nb = T == ir::Type::I8 ? 1 : (T == ir::Type::I16 || T == ir::Type::PTR) ? 2 : T == ir::Type::I32 ? 4 : 0;
    if (!nb || multiDefVregs_.count(load.dest.vregId) || localSlotVregs_.count(load.dest.vregId)) return false;
    const auto& insts = currentFn_->blocks[currentBlockIdx_].insts;
    size_t i = currentInstInBlock_ + 1;
    auto skipNops = [&]() { while (i < insts.size() && insts[i].op == ir::Op::NOP) i++; };
    auto onlyUsedBy = [&](uint32_t v, const ir::Inst* user) { return !vregUsedOutside(v, &load, user); };
    // the address of the STORE: the same vreg, or a copy of the same pointer
    auto copySource = [&](uint32_t v) -> int64_t {
        for (const auto& in : insts)
            if (in.op == ir::Op::COPY && in.dest.isVreg() && in.dest.vregId == v && in.src1.isVreg() &&
                !multiDefVregs_.count(v))
                return in.src1.vregId;
        return -1;
    };
    uint32_t val = load.dest.vregId;
    skipNops();
    // optional zero-extension / truncation of the loaded value
    if (i < insts.size() && (insts[i].op == ir::Op::ZEXT || insts[i].op == ir::Op::TRUNC) &&
        insts[i].src1.isVreg() && insts[i].src1.vregId == val && insts[i].dest.isVreg() &&
        onlyUsedBy(val, &insts[i])) {
        val = insts[i].dest.vregId;
        i++;
        skipNops();
    }
    if (i >= insts.size()) return false;
    const ir::Inst& op = insts[i];
    if ((op.op != ir::Op::ADD && op.op != ir::Op::SUB) || op.resultType != T || !op.dest.isVreg() ||
        foldedAddrs_.count(op.dest.vregId) || multiDefVregs_.count(op.dest.vregId)) return false;
    const ir::Operand* other = nullptr;
    if (op.src1.isVreg() && op.src1.vregId == val) other = &op.src2;
    else if (op.op == ir::Op::ADD && op.src2.isVreg() && op.src2.vregId == val) other = &op.src1;
    if (!other || (other->isVreg() && other->vregId == val)) return false;
    for (int k = 0; k < nb; k++) if (directByte(*other, k).empty()) return false;
    if (!onlyUsedBy(val, &op)) return false;
    i++;
    skipNops();
    // optional copies of the pointer for the STORE (used by it only)
    std::set<uint32_t> storeAddrs = {load.src1.vregId};
    int64_t base = copySource(load.src1.vregId);
    size_t firstCopy = i;
    while (i < insts.size() && insts[i].op == ir::Op::COPY && insts[i].dest.isVreg() && insts[i].src1.isVreg() &&
           base >= 0 && insts[i].src1.vregId == (uint32_t)base) {
        storeAddrs.insert(insts[i].dest.vregId);
        i++;
        skipNops();
    }
    if (i >= insts.size()) return false;
    const ir::Inst& st = insts[i];
    if (st.op != ir::Op::STORE || st.resultType != T || !st.args.empty() || !st.src1.isVreg() ||
        st.src1.vregId != op.dest.vregId || !st.src2.isVreg() || !storeAddrs.count(st.src2.vregId) ||
        localSlotVregs_.count(st.src2.vregId) || !onlyUsedBy(op.dest.vregId, &st)) return false;
    for (size_t c = firstCopy; c < i; c++)
        if (insts[c].op == ir::Op::COPY && insts[c].dest.vregId != st.src2.vregId) return false;
    if (st.src2.vregId != load.src1.vregId && !onlyUsedBy(st.src2.vregId, &st)) return false;

    bool add = op.op == ir::Op::ADD;
    std::string r = add ? "add in place" : "sub in place";
    ms_.invalidateAll();
    storeCache_.vreg = UINT32_MAX;
    emit("ldy #" + std::to_string(yOff), r);
    std::string b0 = directByte(*other, 0);
    if (nb == 1 && (b0 == "#1" || (b0 == "#255" && add))) {
        emit("lda (" + zpPair + "),y");
        emit(((b0 == "#1") == add) ? "inc a" : "dec a");
        emit("sta (" + zpPair + "),y");
    } else {
        emit(add ? "clc" : "sec");
        for (int k = 0; k < nb; k++) {
            if (k) emit("iny");
            emit("lda (" + zpPair + "),y");
            emit((add ? "adc " : "sbc ") + directByte(*other, k));
            emit("sta (" + zpPair + "),y");
        }
    }
    resultInAX_ = -1;
    skipThrough_ = &st;
    return true;
}

bool IRCodeGen::vregUsedOutside(uint32_t id, const ir::Inst* a, const ir::Inst* b) const {
    if (!currentFn_) return true;
    auto uses = [&](const ir::Operand& o) { return o.isVreg() && o.vregId == id; };
    for (const auto& blk : currentFn_->blocks)
        for (const auto& in : blk.insts) {
            if (&in == a || &in == b) continue;
            if (uses(in.src1) || uses(in.src2)) return true;
            if (in.op == ir::Op::STORE && uses(in.dest)) return true;
            for (const auto& x : in.args) if (uses(x)) return true;
            for (const auto& ph : in.phiIncoming) if (uses(ph.first)) return true;
        }
    return false;
}

void IRCodeGen::restoreFlags() {
    char r = staleFlags_;
    staleFlags_ = 0;
    // (marked: pruneDeadFlagRestores drops it when the code at the label
    // sets N/Z before reading them, Issue #297)
    if (r == 'A') { emitComment(kFlagRestore); emit("ora #0", "N/Z of a dropped load"); }
    else if (r == 'X') { emitComment(kFlagRestore); emit("inx", "N/Z of a dropped load"); emit("dex"); }
}

void IRCodeGen::emitLabel(const std::string& label) {
    if (staleFlags_) restoreFlags();  // the fall-through path
    if (inBss_) bssBoundaryClean_ = false;
    out_ << label << ":\n";
    emitSeq_++;       // a branch target: nothing is known here
    elemCache_.valid = false;
    flagReg_ = 0;
}

// The store-then-load cache's view of an emitted line: every instruction
// counts; the N/Z flags follow the native instructions that set them from a
// register, stores and pushes leave them, anything else (compare, call,
// simulated instruction, ...) makes them unknown. Directives and comments
// change nothing.
void IRCodeGen::noteEmitted(const std::string& line) {
    if (line.empty() || line[0] == '.' || line[0] == ';') return;
    size_t sp = line.find_first_of(" \t");
    std::string m = line.substr(0, sp);
    std::transform(m.begin(), m.end(), m.begin(), ::tolower);
    std::string arg = sp == std::string::npos ? "" : line.substr(line.find_first_not_of(" \t", sp));
    if (!m.empty() && m.back() == ':') { emitSeq_++; flagReg_ = 0; return; }  // a label line
    emitSeq_++;
    static const std::set<std::string> setsA = {"lda", "pla", "txa", "tya", "tza", "and", "ora", "eor",
                                                "adc", "sbc", "neg"};
    static const std::set<std::string> keeps = {"sta", "stx", "sty", "stz", "pha", "phx", "phy", "phz",
                                                "clc", "sec", "cli", "sei", "cld", "sed", "clv", "nop"};
    if (!keeps.count(m)) staleFlags_ = 0;  // the flags are set anew
    if (setsA.count(m)) flagReg_ = 'A';
    else if ((m == "asl" || m == "lsr" || m == "rol" || m == "ror" || m == "inc" || m == "dec") &&
             (arg.empty() || arg == "a"))
        flagReg_ = 'A';
    else if (m == "ldx" || m == "tax" || m == "inx" || m == "dex" || m == "plx" || m == "tsx") flagReg_ = 'X';
    else if (m == "ldy" || m == "tay" || m == "iny" || m == "dey" || m == "ply" || m == "tsy") flagReg_ = 'Y';
    else if (m == "ldz" || m == "taz" || m == "inz" || m == "dez" || m == "plz") flagReg_ = 'Z';
    else if (!keeps.count(m)) flagReg_ = 0;
}

void IRCodeGen::emitComment(const std::string& text) {
    out_ << "; " << text << "\n";
}

void IRCodeGen::emitBlank() {
    out_ << "\n";
}

// Debug information (#202): a variable for ca45's OPT_DEBUG_VARS record:
// kind (global / static / local / param), scope (its function's label, "-"),
// C name, type descriptor, location
// (written after the function's endproc, flushDebugVars: a directive inside
// a proc would stand between instructions for ca45's optimiser passes)
void IRCodeGen::emitDebugVar(const std::string& kind, const std::string& scope, const std::string& name,
                             const std::string& type, const std::string& loc) {
    pendingDebugVars_.push_back(".dbg_var \"" + kind + " " + scope + " " + name + " " + type + " " + loc + "\"");
}

void IRCodeGen::flushDebugVars() {
    for (const auto& l : pendingDebugVars_) emit(l);
    pendingDebugVars_.clear();
}

// Where a local's value lives for the debugger: its base-page block byte
// (sym:<fn>__zar+n), a byte of the ZP register pool (zp:$xx, in the
// program's base page), its static storage (sym:<fn>__local_N / __param_x,
// which ca45 finds or not: an optimised-out local has none), a frame offset
// (frame:n, from the frame pointer at $FD/$FE) or a register (reg:AX)
std::string IRCodeGen::debugLocation(const ir::Function& fn, uint32_t vregId) {
    auto z = zarVregs_.find(vregId);
    if (z != zarVregs_.end())
        return "sym:" + fn.name + "__zar+" + std::to_string(z->second - ZAR_START);
    auto a = alloc_.getAlloc(vregId);
    switch (a.loc) {
        case VRegAllocator::IN_ZP: return "zp:$" + hex8((uint8_t)a.offset);
        case VRegAllocator::IN_AX: return "reg:AX";
        case VRegAllocator::IN_X:  return "reg:X";
        case VRegAllocator::IN_Y:  return "reg:Y";
        case VRegAllocator::IN_Z:  return "reg:Z";
        case VRegAllocator::IN_FRAME:
            // (fn__param_<name> / fn__local_<id>, as sacSymbol names them)
            if (currentFunctionUseSAC_)
                return "sym:" + fn.name + (vregId < fn.paramTypes.size()
                    ? "__param_" + (vregId < fn.paramNames.size() && !fn.paramNames[vregId].empty()
                                    ? fn.paramNames[vregId] : std::to_string(vregId))
                    : "__local_" + std::to_string(vregId));
            if (vregOffset_.count(vregId)) return "frame:" + std::to_string(vregOffset_.at(vregId));
            return "none";
    }
    return "none";
}

// ============================================================================
// SAC Debug Output
// ============================================================================

void IRCodeGen::emitSACDebugEnter(const std::string& funcName, int arSize) {
    if (!sacDebugMode_) return;

    // Emit entry marker with function name and AR size
    emitComment("SAC_DEBUG_ENTER: " + funcName + " (AR size: " + std::to_string(arSize) + " bytes)");

    // Create string literal for function name
    std::string nameLabel = funcName + "__debug_name_str";

    // Emit call to debug helper (will output entry marker)
    // __sac_debug_enter(const char* funcName, uint16_t arAddr, uint16_t arSize)
    emit("; ENTER " + funcName + " (AR buffer allocated at __" + funcName + "__ar)");

    // Load AR address and call helper
    // Note: Actual AR address resolution happens at link time
    // For now, we just emit the symbolic reference
    emit("lda #<__" + funcName + "__ar", "AR buffer low byte");
    emit("ldx #>__" + funcName + "__ar", "AR buffer high byte");
    emit("ldy #" + std::to_string(arSize), "AR size");
    emit("jsr __sac_debug_enter", "call debug helper");
}

void IRCodeGen::emitSACDebugExit(const std::string& funcName) {
    if (!sacDebugMode_) return;

    // Emit exit marker
    emitComment("SAC_DEBUG_EXIT: " + funcName);
    emit("; EXIT " + funcName);
    emit("jsr __sac_debug_exit", "call debug helper");
}

// ============================================================================
// Frame management
// ============================================================================

void IRCodeGen::resetFrame() {
    vregOffset_.clear();
    vregType_.clear();
    frameSize_ = 0;
}

// ============================================================================
// Load/store vRegs via frame pointer
// ============================================================================

std::string IRCodeGen::vregDesc(uint32_t vregId) {
    if (!emitReasons_) return "";
    auto alloc = alloc_.getAlloc(vregId);
    std::string loc;
    switch (alloc.loc) {
        case VRegAllocator::IN_AX: loc = "AX"; break;
        case VRegAllocator::IN_ZP: loc = "ZP:$" + hex8((uint8_t)alloc.offset); break;
        case VRegAllocator::IN_FRAME: loc = "frame"; break;
    }
    std::string ty;
    switch (alloc.type) {
        case ir::Type::I8: ty = "I8"; break;
        case ir::Type::I16: ty = "I16/PTR"; break;  // PTR == I16 on 45GS02
        case ir::Type::I32: ty = "I32"; break;
        default: ty = "?"; break;
    }
    return "v" + std::to_string(vregId) + "(" + ty + "," + loc + ")";
}

// An element of a global array of at most 256 bytes: every in-bounds byte
// offset fits in Y, so the element is name+off,y (no address built in
// __zp_scratch). Y = index * stride for strides 1, 2, 4; the index is
// loaded first (its load may use Y).
// Also a byte element at a byte index of any global array (#318): the index
// fits in Y whatever the array's size.
bool IRCodeGen::arrayYOk(const ir::Operand& base, const ir::Operand& index, int stride, int elemOffset,
                         int elemSize) const {
    if (base.kind != ir::OperandKind::GLOBAL || !index.isVreg() || elemOffset < 0) return false;
    if (stride == 1 && elemSize == 1 && alloc_.getAlloc(index.vregId).type == ir::Type::I8) return true;
    return smallGlobals_.count(base.name) && (stride == 1 || stride == 2 || stride == 4) &&
           elemOffset + elemSize <= 256;
}

bool IRCodeGen::smallArrayY(const ir::Operand& base, const ir::Operand& index, int stride, int elemOffset,
                            int elemSize, std::string& operand) {
    if (!arrayYOk(base, index, stride, elemOffset, elemSize)) return false;
    std::string direct = stride == 1 && alloc_.getAlloc(index.vregId).type == ir::Type::I8 ? directBase(index.vregId) : "";
    if (!direct.empty()) {
        emit("ldy " + direct, "the index (" + base.name + "+off,y)");
    } else {
        loadOperand(index);
        for (int k = stride; k > 1; k >>= 1) emit("asl");
        emit("tay", "index * " + std::to_string(stride) + " (" + base.name + "+off,y)");
    }
    operand = elemOffset ? base.name + "+" + std::to_string(elemOffset) : base.name;
    elemCache_.valid = false;
    return true;
}

// p[i] with the pointer p in ZP and a byte index i (stride 1, no offset):
// the address is (p),y with Y = i (the caller loads Y). The ZP pair in zp.
bool IRCodeGen::zpPtrByteIndex(const ir::Operand& base, const ir::Operand& index, int stride, int elemOffset,
                               std::string& zp) const {
    if (base.kind == ir::OperandKind::GLOBAL || !base.isVreg() || !index.isVreg() || stride != 1 || elemOffset != 0)
        return false;
    if (alloc_.getAlloc(index.vregId).type != ir::Type::I8) return false;
    zp = directBase(base.vregId);
    return zp.size() == 3 && zp[0] == '$';
}

// A:X *= c (the low 16 bits) without the hardware multiplier, when that is
// no bigger than limit bytes (its 22; a call of __mul16, 11, or of __mul16y, 5): the high byte is shifted in a scratch byte
// (asl; rol s: no pha/pla), the multiplicand's bytes kept for the adds of a
// constant with more than one bit set. A byte value (X known 0) times 2 is
// asl; bcc; inx. Y is kept. False: not worth it (the caller multiplies).
bool IRCodeGen::emitMulAXConst(long c, bool xZero, const std::string& why, int limit) {
    c &= 0xFFFF;
    if (c == 1) return true;
    if (c == 0) { emit("lda #0", why); emit("ldx #0"); return true; }
    int top = 15;
    while (!((c >> top) & 1)) top--;
    int bits = 0;
    for (int b = 0; b <= top; b++) bits += (c >> b) & 1;
    const std::string S = "__zp_scratch4+1", X0 = "__zp_scratch4", X1 = "__zp_scratch3+1";
    if (bits == 1) {
        if (top == 1 && xZero) {
            std::string l = "@__mulc_" + std::to_string(labelCounter_++);
            emit("asl", why); emit("bcc " + l); emit("inx"); emitLabel(l);
        } else if (top == 1) {
            emit("asl", why); emit("pha"); emit("txa"); emit("rol"); emit("tax"); emit("pla");
        } else if (top >= 8) {
            // * 256: the low byte up (shifted first: x << 9 is asl; tax; lda #0)
            for (int k = 8; k < top; k++) emit("asl", k == 8 ? why : "");
            emit("tax", top == 8 ? why : ""); emit("lda #0");
        } else if (xZero && top == 2) {
            // a byte << 2: the high byte its top two bits (three rol bring
            // them down: and #3), 10 bytes (#350)
            emit("pha", why);
            emit("rol"); emit("rol"); emit("rol");
            emit("and #3");
            emit("tax"); emit("pla");
            emit("asl"); emit("asl");
        } else if (xZero && top >= 3) {
            // a byte: the high byte its top bits (lsr), the low byte shifted
            // (#348: 11 bytes, not 4 + 3 per bit through a scratch byte)
            emit("pha", why);
            for (int k = top; k < 8; k++) emit("lsr");
            emit("tax"); emit("pla");
            for (int k = 0; k < top; k++) emit("asl");
        } else {
            emit("stx " + S, why);
            for (int k = 0; k < top; k++) { emit("asl"); emit("rol " + S); }
            emit("ldx " + S);
        }
        return true;
    }
    int bytes = 2 + (xZero ? 0 : 2) + 2 + top * 3 + (bits - 1) * (xZero ? 8 : 11) + 2;
    if (bytes > limit) return false;
    // Horner over the bits below the top one: r = r * 2 (+ x)
    emit("sta " + X0, why + " (shifts and adds)");
    if (!xZero) emit("stx " + X1);
    emit("stx " + S);
    for (int b = top - 1; b >= 0; b--) {
        emit("asl"); emit("rol " + S);
        if ((c >> b) & 1) {
            emit("clc"); emit("adc " + X0);
            if (xZero) {
                std::string l = "@__mulc_" + std::to_string(labelCounter_++);
                emit("bcc " + l); emit("inc " + S); emitLabel(l);
            } else {
                emit("pha"); emit("lda " + S); emit("adc " + X1); emit("sta " + S); emit("pla");
            }
        }
    }
    emit("ldx " + S);
    return true;
}

void IRCodeGen::loadVreg(uint32_t vregId) {
    auto ga = globalLoadAlias_.find(vregId);
    if (ga != globalLoadAlias_.end()) {
        if (alloc_.getAlloc(vregId).type == ir::Type::I32) emit("ldq " + ga->second, "long read where it is");
        // (a byte: not the byte after it, #343)
        else if (alloc_.getAlloc(vregId).type == ir::Type::I8) { emit("lda " + ga->second, "read where it is"); emit("ldx #0"); }
        else { emit("lda " + ga->second, "read where it is"); emit("ldx " + ga->second + "+1"); }
        ms_.invalidateAll();
        return;
    }
    auto za = zextAlias_.find(vregId);
    if (za != zextAlias_.end()) {
        emit("lda " + directByte(za->second, 0), "zero-extended byte");
        emit("ldx #0");
        return;
    }
    auto alloc = alloc_.getAlloc(vregId);
    std::string r = "loadVreg " + vregDesc(vregId);
    if (suppressedVregs_.count(vregId) && vregConstVal_.count(vregId) && !localSlotVregs_.count(vregId)) {
        // a constant that was not materialized
        int64_t v = vregConstVal_[vregId];
        emit("lda #" + std::to_string((int)(v & 0xFF)), r);
        if (alloc.type == ir::Type::I32) {
            emit("ldx #" + std::to_string((int)((v >> 8) & 0xFF)));
            emit("ldy #" + std::to_string((int)((v >> 16) & 0xFF)));
            emit("ldz #" + std::to_string((int)((v >> 24) & 0xFF)));
        } else if (alloc.type != ir::Type::I8) {
            emit("ldx #" + std::to_string((int)((v >> 8) & 0xFF)));
        } else {
            emit("ldx #0");
        }
        return;
    }
    switch (alloc.loc) {
        case VRegAllocator::IN_AX:
            // Already in A:X — check if it's still there
            if (alloc_.isInAX(vregId, currentInstIdx_)) return; // no-op!
            // Fell through from A:X to somewhere — treat as frame
            if (vregOffset_.count(vregId)) {
                emit("ldax.local " + std::to_string(vregOffset_[vregId]), r);
            }
            break;
        case VRegAllocator::IN_ZP: {
            std::stringstream ss;
            ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << alloc.offset;
            std::string zpAddr = ss.str();
            bool cached = justStored(vregId);  // A (and X) hold it already
            if (alloc.type == ir::Type::I8) {
                if (!cached) emit("lda " + zpAddr, r);
                emit("ldx #0", "zext I8→I16");
            } else if (alloc.type == ir::Type::I32) {
                // LDQ base-page (LDZ has no base-page mode)
                emit("ldq " + zpAddr, r);
            } else {
                if (cached && storeCache_.xValid) {
                    if (flagReg_ != 'X') { staleFlags_ = 'X'; flagReg_ = 'X'; }
                    break;
                }
                if (!cached) emit("lda " + zpAddr, r);
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)(alloc.offset + 1);
                emit("ldx " + ss.str(), r);
            }
            break;
        }
        case VRegAllocator::IN_FRAME: {
            if (vregOffset_.count(vregId)) {
                if (currentFunctionUseSAC_) {
                    // SAC: Direct absolute addressing to inline storage symbols
                    std::string storageSymbol;
                    if (vregId < currentFn_->paramTypes.size()) {
                        // This is a parameter vreg
                        std::string pName = (vregId < currentFn_->paramNames.size() && !currentFn_->paramNames[vregId].empty())
                            ? currentFn_->paramNames[vregId] : std::to_string(vregId);
                        storageSymbol = currentFunctionName_ + "__param_" + pName;
                    } else {
                        // This is a local vreg
                        storageSymbol = currentFunctionName_ + "__local_" + std::to_string(vregId);
                    }

                    // Phase 78: Record parameter access for SMC analysis
                    if (trackSMCOffsetsEnabled_ && vregId < currentFn_->paramTypes.size()) {
                        recordParameterAccess((int)vregId, ir::typeSize(alloc.type));
                    }

                    bool cached = justStored(vregId);  // A (and X) hold it already
                    if (alloc.type == ir::Type::I32) {
                        emit("lda " + storageSymbol, r);
                        emit("ldx " + storageSymbol + "+1", r);
                        emit("ldy " + storageSymbol + "+2", r);
                        emit("ldz " + storageSymbol + "+3", r);
                    } else if (alloc.type == ir::Type::I8) {
                        if (cached) {
                            if (flagReg_ != 'A') { staleFlags_ = 'A'; flagReg_ = 'A'; }
                        } else {
                            emit("lda " + storageSymbol, r);
                        }
                    } else {
                        if (cached) {
                            if (flagReg_ != 'X') { staleFlags_ = 'X'; flagReg_ = 'X'; }
                            break;
                        }
                        emit("lda " + storageSymbol, r);
                        emit("ldx " + storageSymbol + "+1", r);
                    }
                } else {
                    // Non-SAC: FP-relative addressing through pseudo-ops
                    std::string sym = "__vr" + std::to_string(vregId);
                    if (alloc.type == ir::Type::I32) {
                        emit("ldaxyz.local " + sym, r);
                    } else if (alloc.type == ir::Type::I8) {
                        emit("lda.local " + sym, r);
                    } else {
                        emit("ldax.local " + sym, r);
                    }
                }
            }
            break;
        }
    }
}

void IRCodeGen::loadVregA(uint32_t vregId) {
    auto ga = globalLoadAlias_.find(vregId);
    if (ga != globalLoadAlias_.end()) { emit("lda " + ga->second, "long read where it is"); ms_.invalidateAll(); return; }
    auto ta = truncAlias_.find(vregId);
    if (ta != truncAlias_.end()) { emit("lda " + directByte(ta->second, 0), "low byte"); return; }
    auto za = zextAlias_.find(vregId);
    if (za != zextAlias_.end()) { emit("lda " + directByte(za->second, 0), "zero-extended byte"); return; }
    if (suppressedVregs_.count(vregId) && vregConstVal_.count(vregId) && !localSlotVregs_.count(vregId)) {
        emit("lda #" + std::to_string((int)(vregConstVal_[vregId] & 0xFF)), "constant");
        return;
    }
    auto alloc = alloc_.getAlloc(vregId);
    if (alloc.type != ir::Type::I8) { loadVreg(vregId); return; }
    std::string r = "loadVregA " + vregDesc(vregId);
    switch (alloc.loc) {
        case VRegAllocator::IN_AX:
            if (alloc_.isInAX(vregId, currentInstIdx_)) return;
            if (vregOffset_.count(vregId)) {
                // For SAC functions, use direct absolute addressing to inline storage
                if (currentFunctionUseSAC_) {
                    std::string storageSymbol;
                    if (vregId < currentFn_->paramTypes.size()) {
                        std::string pName = (vregId < currentFn_->paramNames.size() && !currentFn_->paramNames[vregId].empty())
                            ? currentFn_->paramNames[vregId] : std::to_string(vregId);
                        storageSymbol = currentFunctionName_ + "__param_" + pName;
                        // Phase 78: Record parameter access for SMC analysis (IN_AX case)
                        if (trackSMCOffsetsEnabled_) {
                            recordParameterAccess((int)vregId, 1);  // I8 = 1 byte
                        }
                    } else {
                        storageSymbol = currentFunctionName_ + "__local_" + std::to_string(vregId);
                    }
                    emit("lda " + storageSymbol, r);
                } else {
                    emit("lda.local " + std::to_string(vregOffset_[vregId]), r);
                }
            }
            break;
        case VRegAllocator::IN_ZP: {
            if (justStored(vregId)) {  // A holds it
                if (flagReg_ != 'A') { staleFlags_ = 'A'; flagReg_ = 'A'; }
                break;
            }
            std::stringstream ss;
            ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << alloc.offset;
            emit("lda " + ss.str(), r);
            break;
        }
        case VRegAllocator::IN_FRAME:
            if (vregOffset_.count(vregId)) {
                if (currentFunctionUseSAC_ && justStored(vregId)) {
                    if (flagReg_ != 'A') { staleFlags_ = 'A'; flagReg_ = 'A'; }
                    break;
                }
                if (currentFunctionUseSAC_) {
                    std::string storageSymbol;
                    if (vregId < currentFn_->paramTypes.size()) {
                        std::string pName = (vregId < currentFn_->paramNames.size() && !currentFn_->paramNames[vregId].empty())
                            ? currentFn_->paramNames[vregId] : std::to_string(vregId);
                        storageSymbol = currentFunctionName_ + "__param_" + pName;
                        // Phase 78: Record parameter access for SMC analysis (IN_FRAME case)
                        if (trackSMCOffsetsEnabled_) {
                            recordParameterAccess((int)vregId, 1);  // I8 = 1 byte
                        }
                    } else {
                        storageSymbol = currentFunctionName_ + "__local_" + std::to_string(vregId);
                    }
                    emit("lda " + storageSymbol, r);
                } else {
                    std::string sym = "__vr" + std::to_string(vregId);
                    emit("lda.local " + sym, r);
                }
            }
            break;
    }
}

void IRCodeGen::storeVreg(uint32_t vregId) {
    // a 16-bit ZP value whose bytes are equal is stored from A twice: X is
    // then not known to hold the high byte
    bool xValid = !ms_.regsEqual(REG_A, REG_X);
    storeVregEmit(vregId);
    // A (and X) still hold the value for an I8/I16 stored from A/X
    auto alloc = alloc_.getAlloc(vregId);
    bool fromAX = alloc.type == ir::Type::I8 ||
                  (alloc.type != ir::Type::I32 && (alloc.loc == VRegAllocator::IN_ZP ||
                                                   (alloc.loc == VRegAllocator::IN_FRAME && valueByte_[1] != REG_Z)));
    if (fromAX && (alloc.loc == VRegAllocator::IN_ZP ||
                   (alloc.loc == VRegAllocator::IN_FRAME && currentFunctionUseSAC_)))
        storeCache_ = {vregId, emitSeq_, xValid || alloc.loc != VRegAllocator::IN_ZP};
}

void IRCodeGen::storeVregEmit(uint32_t vregId) {
    auto alloc = alloc_.getAlloc(vregId);
    std::string r = "storeVreg " + vregDesc(vregId);
    switch (alloc.loc) {
        case VRegAllocator::IN_AX:
            break;
        case VRegAllocator::IN_ZP: {
            std::stringstream ss;
            ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << alloc.offset;
            std::string zpAddr = ss.str();
            if (alloc.type == ir::Type::I8) {
                emit("sta " + zpAddr, r);
            } else if (alloc.type == ir::Type::I32) {
                emit("sta " + zpAddr);
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)(alloc.offset + 1);
                emit(ms_.regsEqual(REG_A, REG_X) ? "sta " + ss.str() : "stx " + ss.str());
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)(alloc.offset + 2);
                emit(ms_.regsEqual(REG_A, REG_Y) ? "sta " + ss.str() : "sty " + ss.str());
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)(alloc.offset + 3);
                emit(ms_.regsEqual(REG_A, REG_Z) ? "sta " + ss.str() : "stz " + ss.str());
            } else {
                emit("sta " + zpAddr);
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)(alloc.offset + 1);
                emit(ms_.regsEqual(REG_A, REG_X) ? "sta " + ss.str() : "stx " + ss.str());
            }
            break;
        }
        case VRegAllocator::IN_FRAME: {
            // SAC: Direct absolute addressing to inline storage symbols
            // Non-SAC: FP-relative addressing through pseudo-ops
            if (currentFunctionUseSAC_) {
                // Direct absolute: inline storage symbols
                std::string storageSymbol;
                if (vregId < currentFn_->paramTypes.size()) {
                    // This is a parameter vreg
                    std::string pName = (vregId < currentFn_->paramNames.size() && !currentFn_->paramNames[vregId].empty())
                        ? currentFn_->paramNames[vregId] : std::to_string(vregId);
                    storageSymbol = currentFunctionName_ + "__param_" + pName;
                } else {
                    // This is a local vreg
                    storageSymbol = currentFunctionName_ + "__local_" + std::to_string(vregId);
                }

                if (alloc.type == ir::Type::I32) {
                    emit("sta " + storageSymbol);
                    emit("stx " + storageSymbol + "+1");
                    emit("sty " + storageSymbol + "+2");
                    emit("stz " + storageSymbol + "+3");
                } else if (alloc.type == ir::Type::I8) {
                    emit("sta " + storageSymbol);
                } else if (valueByte_[1] == REG_Z) {
                    emit("sta " + storageSymbol);
                    emit("stz " + storageSymbol + "+1");
                } else {
                    emit("sta " + storageSymbol);
                    emit("stx " + storageSymbol + "+1");
                }
            } else {
                // FP-relative: use pseudo-ops that expand to indirect addressing
                std::string sym = "__vr" + std::to_string(vregId);
                if (alloc.type == ir::Type::I32) {
                    emit("staxyz.local " + sym);
                } else if (alloc.type == ir::Type::I8) {
                    emit("sta.local " + sym);
                } else if (valueByte_[1] == REG_Z) {
                    emit("staz.local " + sym);
                } else {
                    emit("stax.local " + sym);
                }
            }
            break;
        }
    }
}

// Load only A from an operand (no X/Y/Z). For I8 use only.
void IRCodeGen::loadOperandA(const ir::Operand& op) {
    switch (op.kind) {
        case ir::OperandKind::VREG:
            loadVregA(op.vregId);
            break;
        case ir::OperandKind::IMM:
            emit("lda #" + std::to_string((int)(op.immVal & 0xFF)));
            break;
        default:
            loadOperand(op); // fallback
            break;
    }
}

void IRCodeGen::loadOperand(const ir::Operand& op) {
    switch (op.kind) {
        case ir::OperandKind::VREG:
            loadVreg(op.vregId);
            break;
        case ir::OperandKind::IMM:
            emit("lda #" + std::to_string((int)(op.immVal & 0xFF)));
            if (op.type == ir::Type::I32) {
                emit("ldx #" + std::to_string((int)((op.immVal >> 8) & 0xFF)));
                emit("ldy #" + std::to_string((int)((op.immVal >> 16) & 0xFF)));
                emit("ldz #" + std::to_string((int)((op.immVal >> 24) & 0xFF)));
            } else if (op.type != ir::Type::I8) {
                emit("ldx #" + std::to_string((int)((op.immVal >> 8) & 0xFF)));
            }
            break;
        case ir::OperandKind::GLOBAL:
            emit("ldax #" + op.name);
            break;
        default:
            emit("lda #0");
            emit("ldx #0");
            break;
    }
}

std::string IRCodeGen::src2MemOperand(const ir::Operand& op) {
    if (op.isImm()) {
        return "#" + std::to_string((int)op.immVal);
    }
    if (op.isVreg() && globalLoadAlias_.count(op.vregId)) return globalLoadAlias_.at(op.vregId);
    if (op.isVreg()) {
        // Check for suppressed CONST vreg — use immediate instead of ZP
        auto alloc = alloc_.getAlloc(op.vregId);
        bool is32 = alloc.type == ir::Type::I32 || op.type == ir::Type::I32;
        auto cit = vregConstVal_.find(op.vregId);
        if (cit != vregConstVal_.end() && suppressedVregs_.count(op.vregId)) {
            return "#" + std::to_string(is32 ? (long long)(uint32_t)cit->second : (long long)(cit->second & 0xFFFF));
        }
        switch (alloc.loc) {
            case VRegAllocator::IN_ZP: {
                std::stringstream ss;
                ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << alloc.offset;
                return ss.str();
            }
            case VRegAllocator::IN_FRAME:
                // SAC storage is addressed directly
                if (currentFunctionUseSAC_ && vregOffset_.count(op.vregId)) return sacSymbol(op.vregId);
                // Frame vRegs use ldax.fp which internally uses __zp_scratch.
                // Spill to __zp_scratch2 to avoid conflict when loadOperand(src1)
                // also needs __zp_scratch for its own ldax.fp.
                loadVreg(op.vregId);
                if (is32) { emit("stq __zp_scratch2"); return "__zp_scratch2"; }  // all 4 bytes (AXYZ)
                emit("sta __zp_scratch2");
                emit("stx __zp_scratch2+1");
                return "__zp_scratch2";
            case VRegAllocator::IN_AX:
                // Value is in A:X — spill to __zp_scratch2 (not __zp_scratch,
                // because loadOperand for the other operand may use ldax.fp
                // which clobbers __zp_scratch internally).
                if (is32) { emit("stq __zp_scratch2"); return "__zp_scratch2"; }
                emit("sta __zp_scratch2");
                emit("stx __zp_scratch2+1");
                return "__zp_scratch2";
        }
    }
    if (op.kind == ir::OperandKind::GLOBAL) {
        // Load global address into scratch for use as memory operand
        emit("lda #<" + op.name);
        emit("sta __zp_scratch2");
        emit("lda #>" + op.name);
        emit("sta __zp_scratch2+1");
        return "__zp_scratch2";
    }
    return "#0";
}

void IRCodeGen::emitArrayElemAddr(const std::string& baseStr, const ir::Operand& indexOp,
                                   int stride, const std::string& destZP) {
    // Emit inline address calculation: result = baseAddr + (index * stride)
    // Result is returned in A:X and optionally stored to destZP (if non-empty)
    // baseStr can be: "#symbol" (immediate), "$XX" (ZP address), or other memory operand
    // indexOp is the array index (can be immediate, operand, or vReg)
    // stride is the element size in bytes

    if (indexOp.isImm()) {
        // Immediate index: compute constant offset at compile time
        uint32_t offset = indexOp.immVal * stride;
        if (!destZP.empty()) {
            emit("struct_elem.16 " + destZP + ", " + baseStr + ", #" + std::to_string(offset));
        } else {
            // Load base and add constant offset
            if (baseStr[0] == '#') {
                std::string symName = baseStr.substr(1);
                emit("ldax #" + symName);  // Load immediate address of symbol into A:X
            } else if (baseStr[0] == '$') {
                // Load from ZP address
                emit("lda " + baseStr);
                emit("ldx " + baseStr + "+1");
            } else {
                // Memory operand
                emit("lda " + baseStr);
                emit("ldx " + baseStr + "+1");
            }
            if (offset > 0) {
                emit("add.16 .AX, #" + std::to_string(offset));
            }
        }
        return;
    }

    // Runtime index: need to compute address = base + (index * stride)
    // Step 1: Load index into A:X (a byte index loads only A: clear X)
    loadOperand(indexOp);  // Load into A:X
    if (indexOp.type == ir::Type::I8 ||
        (indexOp.isVreg() && alloc_.getAlloc(indexOp.vregId).type == ir::Type::I8))
        emit("ldx #0");

    // Step 2: Multiply index by stride if stride > 1: shifts and adds (Y
    // kept) or the hardware multiplier; outside loops (as a multiply, #317)
    // a call of __mul16y / __mul16 when the shifts are bigger (Y and
    // __zp_scratch2 changed: not with a base there)
    if (stride > 1) {
        bool byteIndex = indexOp.type == ir::Type::I8 ||
                         (indexOp.isVreg() && alloc_.getAlloc(indexOp.vregId).type == ir::Type::I8);
        const char* mulCallEnv = getenv("CC45_MUL_CALL");
        bool call = relocMode_ && baseStr.find("__zp_scratch") == std::string::npos &&
                    (mulCallEnv ? mulCallEnv[0] != '0' : !loopBlocks_.count(currentBlockIdx_));
        bool byteFactor = !(stride & 0xFF00);
        std::string why = "index * " + std::to_string(stride);
        if (emitMulAXConst(stride, byteIndex, why, call ? (byteFactor ? 5 : 11) : 22)) {
        } else if (call && byteFactor) {
            emit("ldy #" + std::to_string(stride), why);
            emit(".extern __mul16y");
            emit("jsr __mul16y");
            ms_.invalidateAll();
        } else if (call) {
            emit("ldy #" + std::to_string(stride & 0xFF), why);
            emit("sty __zp_scratch2");
            emit("ldy #" + std::to_string((stride >> 8) & 0xFF));
            emit("sty __zp_scratch2+1");
            emit(".extern __mul16");
            emit("jsr __mul16");
            ms_.invalidateAll();
        } else {
            emit("mul.16 .AX, #" + std::to_string(stride));  // A:X = index * stride
        }
    }

    // Step 3 with a destination: the scaled index (in A:X) plus the base,
    // byte by byte into it (no scratch, no pha/pla)
    if (!destZP.empty()) {
        std::string lo = baseStr, hi = baseStr + "+1";
        if (baseStr[0] == '#') { lo = "#<" + baseStr.substr(1); hi = "#>" + baseStr.substr(1); }
        emit("clc", "index + base into " + destZP);
        emit("adc " + lo);
        emit("sta " + destZP);
        emit("txa");
        emit("adc " + hi);
        emit("sta " + destZP + "+1");
        ms_.invalidateAll();   // (A holds the high byte)
        return;
    }

    // Step 3: Load base address and add the scaled index
    // Save scaled index first
    emit("sta __zp_scratch3");           // Save low byte of scaled index
    emit("stx __zp_scratch3+1");         // Save high byte of scaled index

    // Load base address based on its type
    if (baseStr[0] == '#') {
        // Immediate symbol - use ldax with # prefix to load address as immediate
        std::string symName = baseStr.substr(1);
        emit("ldax #" + symName);        // Load immediate address of symbol into A:X
    } else if (baseStr[0] == '$') {
        // ZP address
        emit("lda " + baseStr);          // Load from ZP low byte
        emit("ldx " + baseStr + "+1");   // Load from ZP high byte
    } else {
        // Other memory operand
        emit("lda " + baseStr);
        emit("ldx " + baseStr + "+1");
    }

    // Add base + scaled index
    emit("clc", "Add low bytes");
    emit("adc __zp_scratch3", "Add scratch3 low");
    emit("pha", "Push A");
    emit("txa", "Transfer X to A");
    emit("adc __zp_scratch3+1", "Add scratch3 high with carry");
    emit("tax", "Transfer A to X");
    emit("pla", "Pop into A");

    // Step 4: Store to destZP if provided
    if (!destZP.empty()) {
        emit("sta " + destZP);
        emit("stx " + destZP + "+1");
    }
}

void IRCodeGen::emitStackCleanup(int frameSize) {
    for (int i = 0; i < frameSize; i++) {
        emit("plz");
    }
}

// ============================================================================
// Module-level emission
// ============================================================================

void IRCodeGen::generate(const ir::Module& mod, uint32_t zpStart, bool relocMode, bool zpCallMode, bool emitReasons, bool staticAllocMode, bool sacDebugMode, uint32_t prgBase) {
    currentModule_ = &mod;
    dataAlign_ = bssAlign_ = 1;
    textAlign_ = 1;
    emitReasons_ = emitReasons;
    relocMode_ = relocMode;
    zpCallMode_ = zpCallMode;
    staticAllocMode_ = staticAllocMode;
    sacDebugMode_ = sacDebugMode;
    zeroPageStart_ = zpStart;
    sourceFile_ = mod.sourceFile;

    // Initialize ZP allocator
    zpRegs_.clear();
    for (uint32_t i = 0; i < zeroPageAvail_; ++i)
        zpRegs_.push_back({false});

    // Reserve ZP scratch area for __zp_scratch symbols:
    // zpAddr(index) returns zeroPageStart_ + index, so to reserve $08-$0F (8 bytes),
    // we need to mark zpRegs_[0-7] as inUse since zpAddr(0) = $08, zpAddr(7) = $0F
    for (uint32_t i = 0; i < 8 && i < zpRegs_.size(); ++i) {
        zpRegs_[i].inUse = true;
    }

    // Phase 97.5: Build symbol → addressSpace mapping for instruction optimization
    // (__zp only in relocatable output: -S keeps them plain globals)
    symbolAddressSpace_.clear();
    smallGlobals_.clear();
    for (const auto& global : mod.globals) {
        int space = global.addressSpace == 1 && !relocMode ? 0 : global.addressSpace;
        symbolAddressSpace_[global.name] = space;
        if (global.size > 0 && global.size <= 256 && space == 0) smallGlobals_.insert(global.name);
    }
    plainExterns_.clear();
    for (const auto& e : mod.externs)
        if (!mod.zpExterns.count(e)) plainExterns_.insert(e);
    placeZpGlobals(mod);
    placeSections(mod);

    // Pre-scan all functions to compute frame offsets (needed for capture)
    // Bug #179 fix: Skip prescan for all functions.
    // Prescan allocates without knowing allocator decisions, causing frame conflicts.
    // emitFunction will re-allocate based on allocator's decisions.
    std::map<std::string, const ir::Function*> funcMap;
    for (const auto& fn : mod.functions) {
        // Do NOT call prescanFunction here — it causes frame allocation conflicts
        funcMap[fn.name] = &fn;
    }
    setFunctionMap(&funcMap);

    // Phase 2: Pre-compute clobber masks for all functions (fine-grained register and flag invalidation)
    // First pass: compute base clobbers for all functions
    std::map<std::string, FuncClobbers> baseClobbers;
    functionClobberMasks_.clear();
    functionFlagClobberMasks_.clear();
    functionClobberInfo_.clear();
    for (const auto& fn : mod.functions) {
        FuncClobbers clobbers = computeFuncClobbers(fn);
        baseClobbers[fn.name] = clobbers;

        // Register clobber mask (bit 0=A, 1=X, 2=Y, 3=Z)
        int regMask = 0;
        if (clobbers.regs & (1 << 0)) regMask |= (1 << REG_A);  // bit 0 = A
        if (clobbers.regs & (1 << 1)) regMask |= (1 << REG_X);  // bit 1 = X
        if (clobbers.regs & (1 << 2)) regMask |= (1 << REG_Y);  // bit 2 = Y
        if (clobbers.regs & (1 << 3)) regMask |= (1 << REG_Z);  // bit 3 = Z
        functionClobberMasks_[fn.name] = regMask;

        // Flag clobber mask (bit 0=C, 1=N, 2=Z, 3=V) — pass through unchanged
        functionFlagClobberMasks_[fn.name] = clobbers.flags;
        functionClobberInfo_[fn.name] = clobbers;
    }

    // Second pass: for non-leaf functions, union in the clobbers of all called functions
    for (const auto& fn : mod.functions) {
        if (!fn.originalIsLeaf && !fn.originalCallees.empty()) {
            FuncClobbers& myFC = baseClobbers[fn.name];
            // Union in clobbers from all called functions
            for (const auto& calleeName : fn.originalCallees) {
                // originalCallees stores C names; baseClobbers uses IR names (_ prefix)
                std::string irName = "_" + calleeName;
                if (baseClobbers.count(irName) > 0) {
                    myFC.regs |= baseClobbers[irName].regs;
                    myFC.flags |= baseClobbers[irName].flags;
                } else if (baseClobbers.count(calleeName) > 0) {
                    myFC.regs |= baseClobbers[calleeName].regs;
                    myFC.flags |= baseClobbers[calleeName].flags;
                } else {
                    // External/unknown callee: conservatively clobber everything
                    myFC.regs |= 0x0F;  // A|X|Y|Z
                    myFC.flags |= 0x0F; // C|N|Z|V
                }
            }
            // Update the masks and clobber info with the unioned clobber info
            int regMask = 0;
            if (myFC.regs & (1 << 0)) regMask |= (1 << REG_A);
            if (myFC.regs & (1 << 1)) regMask |= (1 << REG_X);
            if (myFC.regs & (1 << 2)) regMask |= (1 << REG_Y);
            if (myFC.regs & (1 << 3)) regMask |= (1 << REG_Z);
            functionClobberMasks_[fn.name] = regMask;
            functionFlagClobberMasks_[fn.name] = myFC.flags;
            functionClobberInfo_[fn.name] = myFC;
        }
    }

    // Functions on a call cycle within this module (direct or mutual
    // recursion) can't use SAC: each activation needs its own frame
    recursiveFunctions_.clear();
    {
        std::map<std::string, std::set<std::string>> calls;
        for (const auto& fn : mod.functions) {
            auto& out = calls[fn.name];
            for (const auto& blk : fn.blocks)
                for (const auto& inst : blk.insts)
                    if ((inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID) &&
                        inst.src1.kind == ir::OperandKind::GLOBAL)
                        out.insert(inst.src1.name);
        }
        // A function is recursive when it can reach itself
        for (const auto& [name, _] : calls) {
            std::set<std::string> seen;
            std::vector<std::string> work(calls[name].begin(), calls[name].end());
            while (!work.empty()) {
                std::string f = work.back(); work.pop_back();
                if (f == name) { recursiveFunctions_.insert(name); break; }
                if (!seen.insert(f).second) continue;
                auto it = calls.find(f);
                if (it != calls.end()) work.insert(work.end(), it->second.begin(), it->second.end());
            }
        }
    }

    // Phase 3: Pre-scan functions to identify which ones use SAC
    // (Static Allocation Convention — uses __ar symbols instead of stack frame)
    sacFunctions_.clear();
    functionParameterNames_.clear();
    weakDefaults_.clear();
    for (const auto& fn : mod.functions)
        if (fn.isWeak && !fn.isMethod) weakDefaults_.insert(fn.name);
    if (staticAllocMode) {
        for (const auto& fn : mod.functions) {
            // Pre-populate parameter names for all functions so SAC call sites
            // can generate correct __param_<name> symbols regardless of emission order
            std::vector<std::string> paramNames;
            for (size_t i = 0; i < fn.paramTypes.size(); i++) {
                if (i < fn.paramNames.size() && !fn.paramNames[i].empty()) {
                    paramNames.push_back(fn.paramNames[i]);
                } else {
                    paramNames.push_back(std::to_string(i));
                }
            }
            functionParameterNames_[fn.name] = paramNames;
            functionParameterTypes_[fn.name] = fn.paramTypes;

            // SAC eligibility: enabled globally, not recursive, not interrupt/naked/variadic
            bool useSAC = !fn.isRecurse && !fn.isInterrupt && !fn.isNaked && !fn.isVariadic &&
                          !callsRuntimeAlloca(fn) && !recursiveFunctions_.count(fn.name);
            if (useSAC) {
                // Quick self-recursion check
                bool hasSelfCall = false;
                for (const auto& blk : fn.blocks) {
                    for (const auto& inst : blk.insts) {
                        if ((inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID) && inst.src1.kind == ir::OperandKind::GLOBAL) {
                            if (inst.src1.name == fn.name) {
                                hasSelfCall = true;
                                break;
                            }
                        }
                    }
                    if (hasSelfCall) break;
                }
                if (!hasSelfCall) {
                    sacFunctions_.insert(fn.name);  // Mark as SAC-eligible
                }
            }
        }
    }

    // Phase 3.5: Analyze SAC function calls to detect constant parameters
    // (optimization: skip initialization for parameters that always receive the same constant)
    if (staticAllocMode && !sacFunctions_.empty()) {
        analyzeConstantParameters(mod);
    }

    // Phase 3.6: Detect leaf functions (don't call any other functions)
    // Leaf functions can use simpler, faster SAC code without recursive AR setup
    if (staticAllocMode && !sacFunctions_.empty()) {
        detectLeafFunctions(mod);
    }

    // Phase 3.7: Analyze parameter access patterns for SMC optimization
    // (Self-Modifying Code: embed parameters in instruction immediates)
    if (staticAllocMode && !sacFunctions_.empty()) {
        functionSMCMetadata_.clear();

        // For each SAC function, initialize parameter metadata
        for (const auto& fn : mod.functions) {
            if (sacFunctions_.count(fn.name) == 0) continue;  // Only SAC functions

            std::map<int, O45SACParam> paramMetadata;

            // Initialize metadata for each parameter
            for (size_t i = 0; i < fn.paramNames.size(); i++) {
                O45SACParam param;
                param.symbolName = fn.name + "__param_" + fn.paramNames[i];  // e.g., "add_many__param_a"
                param.accessCount = 0;
                param.useSMC = false;
                paramMetadata[(int)i] = param;
            }

            // Store for this function
            functionSMCMetadata_[fn.name] = paramMetadata;
        }
    }

    if (relocMode) {
        // Emit .org for standalone assembly generation
        // The linker ignores this for .o45 files, but ca45 uses it for PRG generation
        std::stringstream ss;
        ss << ".org $" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << prgBase;
        emit(ss.str());
        // __sp_base is weak — linker may provide it, or we use default
        emit(".weak __sp_base");
        emit("__sp_base = $0101");  // Default stack page
        // crt0 helpers that inline asm (cbm.h) calls around ROM routines;
        // __rom_call only where some asm names it (rom_call.s45, a library
        // member of its own: an import would link it into every program, #322)
        emit(".extern __bp_rom");
        emit(".extern __bp_prog");
        bool namesRomCall = false;
        for (const auto& a : mod.fileAsm) namesRomCall = namesRomCall || a.find("__rom_call") != std::string::npos;
        for (const auto& fn : mod.functions)
            for (const auto& blk : fn.blocks)
                for (const auto& in : blk.insts)
                    if (in.op == ir::Op::ASM_INLINE && in.asmText.find("__rom_call") != std::string::npos) namesRomCall = true;
        if (namesRomCall) emit(".extern __rom_call");
        // (__jsr of a ROM address: jsr __rom_calli / .word addr)
        bool namesRomCalli = false;
        for (const auto& fn : mod.functions)
            for (const auto& blk : fn.blocks)
                for (const auto& in : blk.insts)
                    if (in.op == ir::Op::ASM_INLINE && in.asmText.find("__rom_calli") != std::string::npos) namesRomCalli = true;
        if (namesRomCalli) emit(".extern __rom_calli");

        // The module defining main chooses the CPU stack: crt0's weak
        // __stack_top ($BFFF, 16-bit stack) is overridden here; 0 keeps the
        // ROM's 8-bit page-1 stack (-fno-stack16)
        if (mod.stackTop >= 0) {
            for (const auto& fn : mod.functions) {
                if (fn.name != "_main") continue;
                emit(".global __stack_top");
                emit("__stack_top = $" + hex16((uint16_t)mod.stackTop));
                break;
            }
        }
        // -foverlay=name: the module's code is in that code overlay
        if (!mod.overlay.empty()) emit(".overlay " + mod.overlay);
        // -fbss-fill=V: the module defining main sets the byte crt0 fills
        // BSS with (otherwise ln45's --bss-fill, default 0)
        // -fstack-size=N: the CPU stack's share below __stack_top (the heap
        // ends there; otherwise ln45's --stack-size, default 4096)
        if (mod.stackSize >= 0) {
            for (const auto& fn : mod.functions) {
                if (fn.name != "_main") continue;
                emit(".global __stack_size");
                emit("__stack_size = $" + hex16((uint16_t)mod.stackSize));
                break;
            }
        }
        if (mod.bssFill >= 0) {
            for (const auto& fn : mod.functions) {
                if (fn.name != "_main") continue;
                emit(".global __bss_fill");
                emit("__bss_fill = $" + hex8((uint8_t)mod.bssFill));
                break;
            }
        }

        // SAC __ar symbols are linker-computed overlay addresses
        // Don't export them — they will be defined by the linker's overlay allocation pass
        // (Exporting undefined symbols corrupts the export table)
        // for (const auto& name : sacFunctions_) {
        //     emit(".global " + name + "__ar");
        // }
    } else {
        // Standalone PRG mode: emit startup stub
        bool hasMain = false;
        for (const auto& fn : mod.functions) {
            if (fn.name == "_main") { hasMain = true; break; }
        }
        // Emit PRG load address
        std::stringstream ss;
        ss << "* = $" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << prgBase;
        out_ << ss.str() << "\n";
        emit("__sp_base = $0101");

        // --- Set Base Page register ---
        if (mod.setBP >= 0) {
            emitComment("Set B (base page) register to " + std::to_string(mod.setBP));
            emit("lda #" + std::to_string(mod.setBP));
            emit("tab");
        }

        // --- Base page relocation ---
        // Move the zero page to a private page after the program so the ROM's
        // zero page is never disturbed (no save/restore needed).
        if (mod.saveZP && mod.setBP < 0) {
            emitComment("Relocate base page to __zp_page");
            emit("lda #>__zp_page");
            emit("tab");
        }

        // --- 16-bit CPU stack (E clear) topped at stackTop ---
        if (hasMain && mod.stackTop != 0) {
            uint16_t top = mod.stackTop < 0 ? 0xBFFF : (uint16_t)mod.stackTop;
            emitComment("16-bit CPU stack at $" + hex16(top));
            emit("sei");
            emit("cle");
            emit("ldy #$" + hex8((uint8_t)(top >> 8)));
            emit("tys");
            emit("ldx #$" + hex8((uint8_t)top));
            emit("txs");
            emit("cli");
        }

        if (hasMain) {
            emit("jsr _main");
        }

        if (mod.saveZP && mod.setBP < 0) {
            // Back to the ROM's zero page; preserve the return value in A
            emit("tay");
            emit("lda #0");
            emit("tab");
            emit("tya");
        }
        emitLabel("__halt");
        emit("jmp __halt");
    }

    auto hex8 = [](uint32_t val) {
        std::stringstream ss;
        ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (val & 0xFF);
        return ss.str();
    };

    std::string symDir = relocMode ? ".weak" : ".global";
    emit(symDir + " __static_chain");
    emit(symDir + " __zp_scratch");
    emit(symDir + " __zp_scratch2");
    emit(symDir + " __zp_scratch3");
    emit(symDir + " __zp_scratch4");
    emit(symDir + " cc45.zeroPageStart");  // Weak in relocatable mode to avoid duplicate symbol errors
    emit("__static_chain = " + hex8(zeroPageStart_ - 2));
    emit("__zp_scratch = " + hex8(zeroPageStart_));
    emit("__zp_scratch2 = " + hex8(zeroPageStart_ + 2));
    emit("__zp_scratch3 = " + hex8(zeroPageStart_ + 4));
    emit("__zp_scratch4 = " + hex8(zeroPageStart_ + 6));
    emit("cc45.zeroPageStart = " + hex8(zeroPageStart_));
    if (relocMode) {
        // Compilation mode marker: the linker knows calls between objects of the
        // same mode already use the callee's convention (no thunk needed)
        std::string modeSym = zpCallMode_ ? "cc45.mode_zp" : "cc45.mode_stack";
        emit(".weak " + modeSym);
        emit(modeSym + " = 1");
        // this object marks its functions that call through pointers
        // (cc45.icall.<f>, cc45.isig.<f>.<sig>): ln45 need not assume all
        // of them do
        emit(".weak cc45.icallinfo");
        emit("cc45.icallinfo = 1");
        // A stack-mode module's ZP-convention calls (to __fastcall__
        // functions): cc45.zpcall.<callee> tells the linker those calls
        // already follow the callee's convention
        if (!zpCallMode_) {
            std::set<std::string> zpCallees;
            for (const auto& fn : mod.functions)
                for (const auto& blk : fn.blocks)
                    for (const auto& inst : blk.insts)
                        if ((inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID) &&
                            inst.callConv == ir::CallConv::ZP && inst.src1.kind == ir::OperandKind::GLOBAL)
                            zpCallees.insert(inst.src1.name);
            for (const auto& c : zpCallees) {
                emit(".weak cc45.zpcall." + c);
                emit("cc45.zpcall." + c + " = 1");
            }
        }
    }
    emitBlank();

    // Emit extern declarations (only for symbols actually referenced in code)
    // Symbols this module defines (a global variable or function)
    auto definedHere = [&](const std::string& sym) {
        for (const auto& g : mod.globals) if (g.name == sym) return true;
        for (const auto& f : mod.functions) if (f.name == sym && !f.blocks.empty()) return true;
        // a label or equate in file-scope asm ("sym:" / "sym =")
        for (const auto& text : mod.fileAsm) {
            size_t p = 0;
            while ((p = text.find(sym, p)) != std::string::npos) {
                bool startOk = p == 0 || text[p - 1] == '\n' || text[p - 1] == ' ' || text[p - 1] == '\t';
                size_t e = text.find_first_not_of(" \t", p + sym.size());
                if (startOk && e != std::string::npos && (text[e] == ':' || text[e] == '=')) return true;
                p += sym.size();
            }
        }
        return false;
    };
    if (relocMode) {
        // Collect all symbol references from instructions and globals
        // (an operand may be sym+offset: foldGlobalOffsetAccess, #356; its
        // symbol part, as for initializer words)
        auto symOf = [](const std::string& l) { return l.substr(0, l.find_first_of("+-", 1)); };
        std::set<std::string> referencedSymbols;
        for (const auto& fn : mod.functions) {
            for (const auto& blk : fn.blocks) {
                for (const auto& inst : blk.insts) {
                    // Check operand names (GLOBAL/LABEL references)
                    if (!inst.dest.name.empty()) referencedSymbols.insert(symOf(inst.dest.name));
                    if (!inst.src1.name.empty()) referencedSymbols.insert(symOf(inst.src1.name));
                    if (!inst.src2.name.empty()) referencedSymbols.insert(symOf(inst.src2.name));
                    for (const auto& arg : inst.args) {
                        if (!arg.name.empty()) referencedSymbols.insert(symOf(arg.name));
                    }
                }
            }
        }
        for (const auto& g : mod.globals) {
            for (const auto& lbl : g.initLabels) {
                if (!lbl.empty()) referencedSymbols.insert(symOf(lbl));
            }
            for (const auto& [off, lbl] : g.initWords) referencedSymbols.insert(symOf(lbl));
        }
        // Only emit .extern for symbols actually used, and not for one this
        // module defines (extern int x; ... int x = 1;)
        for (const auto& ext : mod.externs) {
            if (referencedSymbols.count(ext) && !definedHere(ext) && !zpPlace_.count(ext) && !sectPlace_.count(ext)) {
                emit(".extern " + ext);
            }
        }
        // __zp globals (also those defined here): ln45 defines their symbols;
        // the placeholders' operands are relocated against them
        for (const auto& [name, zp] : zpPlace_) {
            char h[8];
            snprintf(h, sizeof h, "$%02X", zp.addr);
            emit(".extern " + zpLinkName(name));
            emit(".zpsym " + zpLinkName(name) + ", " + h + ", " + std::to_string(zp.size));
        }
    } else {
        for (const auto& ext : mod.externs) {
            if (!definedHere(ext)) emit(".extern " + ext);
        }
    }

    // main(argc, argv): link the library's __mainargs (mainargs.c), which
    // replaces crt0's weak argc 0 / argv NULL with the BASIC line's arguments
    if (relocMode) {
        for (const auto& fn : mod.functions)
            if (fn.name == "_main" && !fn.paramTypes.empty()) { emit(".extern ___mainargs_link"); break; }
    }

    // Auto-emit float runtime externs if any float ops are used
    if (relocMode) {
        bool hasFloat = false;
        // Only a float in the "..." of a variadic call (or an unprototyped
        // call) needs the %f-aware printf: a float passed to a prototyped
        // parameter (__vformat's own ffmt call) does not
        bool hasFloatArg = mod.floatVarArg;
        bool hasFloatSprintf = mod.floatSprintfArg;
        for (const auto& fn : mod.functions) {
            for (const auto& blk : fn.blocks) {
                for (const auto& inst : blk.insts) {
                    if (inst.op >= ir::Op::FADD && inst.op <= ir::Op::FCONST) {
                        hasFloat = true;
                    }
                }
            }
        }
        if (hasFloat) {
            emit(".extern __float_a");
            emit(".extern __float_b");
            emit(".extern __float_add");
            emit(".extern __float_sub");
            emit(".extern __float_mul");
            emit(".extern __float_div");
            emit(".extern __float_neg");
            emit(".extern __float_cmp");
            emit(".extern __float_itof");
            emit(".extern __float_utof");
            emit(".extern __float_ltof");
            emit(".extern __float_ultof");
            emit(".extern __float_ftoi");
            emit(".extern __float_ftol");
        }
        if (hasFloatArg) {
            emit(".extern _printf_float");
        }
        for (const auto& r : mod.asmRefs) emit(".ref " + r);
        // sprintf with a float argument: link the %f-aware sprintf
        if (hasFloatSprintf) {
            emit(".extern _sprintf_float");
        }
        // scanf reading a float: link the %f-aware scanf family
        if (mod.floatScanfArg || (mod.callsVscanf && hasFloat)) {
            emit(".extern _scanf_float");
        }
    }

    // File-scope asm("..."): module-level assembler text, then back to the
    // code segment the functions are emitted in
    if (!mod.fileAsm.empty()) {
        emitComment("file-scope asm");
        // in the code segment (in the default one, a second text part, the
        // linker would place it after all objects' code)
        if (relocMode) emit(".segment \"code\"");
        for (const auto& text : mod.fileAsm) {
            std::stringstream lines(text);
            std::string line;
            while (std::getline(lines, line)) {
                size_t b = line.find_first_not_of(" \t");
                if (b == std::string::npos) continue;
                line = line.substr(b);
                // a label at the start of a line stays in column 0
                size_t colon = line.find(':');
                bool isLabel = colon != std::string::npos && line[0] != '.' && line[0] != ';' &&
                               line.find_first_of(" \t=") > colon;
                if (isLabel) {
                    emitLabel(line.substr(0, colon));
                    std::string rest = line.substr(colon + 1);
                    size_t rb = rest.find_first_not_of(" \t");
                    if (rb != std::string::npos) emit(rest.substr(rb));
                } else {
                    emit(line);
                }
            }
        }
        emit(".segment \"code\"");
    }

    // __interrupt handlers reload the base page from __zp_page (linker-provided;
    // in a standalone build it is the page after the program, or 0 if the base
    // page isn't relocated)
    bool hasInterrupt = false;
    for (const auto& fn : mod.functions) if (fn.isInterrupt) hasInterrupt = true;
    if (hasInterrupt) {
        if (relocMode) {
            emit(".extern __zp_page_isr");
            emit(".extern __isr_depth");
            emit(".extern __jsr_isr_save, __jsr_isr_restore, __rc_isr_save, __rc_isr_restore");
        } else if (!(mod.saveZP && mod.setBP < 0)) {
            // Base page not relocated: handlers share the program's page
            emit("__zp_page_isr = $" + hex16(mod.setBP >= 0 ? (uint16_t)(mod.setBP << 8) : 0));
        }
    }

    if (!mod.externs.empty()) emitBlank();

    // Emit global declarations (segment switching only in reloc mode)
    placeNoescape(mod);
    emitGlobals(mod, relocMode);

    // Emit functions (skip dead code and inline-only functions from Phase 91.3 IPOAnalyzer)
    // (a nested function reads its parent's variables)
    bool nestedInModule = std::any_of(mod.functions.begin(), mod.functions.end(),
        [](const ir::Function& f) { return f.isNested || f.staticLinkVreg != -1; });
    for (const auto& fn : mod.functions) {
        // Phase 91.3: Skip functions marked as dead code by IPOAnalyzer
        if (deadCodeFunctions_.count(fn.name)) {
            continue;  // Skip emitting dead code function
        }
        // Phase 91.3.3: Skip standalone definitions for inline-only functions
        if (inlineOnlyFunctions_.count(fn.name)) {
            continue;  // Skip emitting inline-only function (only available inlined)
        }
        // a __far function: in the "far" text segment, which ln45 keeps in
        // far memory and maps into the window when called
        if (fn.isFar && relocMode_) {
            // (__bank(n): "far.n", ln45 keeps its overlay in bank n)
            textSeg_ = fn.bank >= 0 ? "far." + std::to_string(fn.bank) : "far";
            emit(".segment \"" + textSeg_ + "\"");
            emitFunctionPruned(fn, relocMode, nestedInModule);
            textSeg_ = "code";
            emit(".segment \"code\"");
            continue;
        }
        // a __section function: in the text segment "sect.NAME", which ln45
        // links to run at the section's address and copies there at startup
        if (!fn.section.empty() && relocMode_) {
            textSeg_ = "sect." + fn.section;
            emit(".segment \"" + textSeg_ + "\"");
            emitFunctionPruned(fn, relocMode, nestedInModule);
            textSeg_ = "code";
            emit(".segment \"code\"");
            continue;
        }
        if (!fn.section.empty()) {
            static std::set<std::string> warned;   // (the code is generated twice: loop rotation)
            if (warned.insert(fn.name).second)
                std::cerr << "cc45: warning: '" << fn.name.substr(1) << "': -S output keeps a section function with the other code\n";
        }
        emitFunctionPruned(fn, relocMode, nestedInModule);
    }

    // Emit string data
    emitStrings(mod);

    // Aligned data/BSS: tell ln45 to start this object's part on that boundary
    if (relocMode) {
        if (dataAlign_ > 1) { emit(".weak cc45.align.data"); emit("cc45.align.data = " + std::to_string(dataAlign_)); }
        if (textAlign_ > 1) { emit(".weak cc45.align.text"); emit("cc45.align.text = " + std::to_string(textAlign_)); }
        if (bssAlign_ > 1) { emit(".weak cc45.align.bss"); emit("cc45.align.bss = " + std::to_string(bssAlign_)); }
    }

    // Relocated base page: the first page boundary after the program
    // (unused RAM, no .res needed).
    if (!relocMode && hasInterrupt) {
        emitLabel("__isr_depth");
        emit(".byte 0");
    }
    if (!relocMode && mod.saveZP && mod.setBP < 0) {
        emitBlank();
        emit(".align 256");
        emitLabel("__zp_page");
        // Interrupt handlers' base page: the page after (unused RAM too)
        if (hasInterrupt) emit("__zp_page_isr = __zp_page + $100");
    }
}

// __noescape objects: kept with the code of the functions that use them when
// those all go to one place: the overlay (an overlay object's code) or the
// far chunk (__far functions). One used by a resident initializer, by code
// in two places or by nothing stays resident.
void IRCodeGen::placeNoescape(const ir::Module& mod) {
    noescapeSeg_.clear();
    noescapeCalls_.clear();
    if (!relocMode_) return;
    std::set<std::string> want;
    for (const auto& g : mod.globals) if (g.noescape && g.hasInitValue) want.insert(g.name);
    // (literals kept with the code: emitStrings' rule)
    std::set<std::string> kept;
    for (const auto& sl : mod.strings)
        if (sl.noescape && (sl.far || !mod.overlay.empty())) kept.insert(sl.label);
    if (want.empty() && kept.empty()) return;
    std::map<std::string, std::set<std::string>> places;   // object -> "code"/"far"
    for (const auto& fn : mod.functions) {
        if (deadCodeFunctions_.count(fn.name) || inlineOnlyFunctions_.count(fn.name)) continue;
        std::string where = fn.isFar ? "far" : !mod.overlay.empty() ? "code" : "data";
        // (a global at an offset, g+2 from foldGlobalOffsetAccess: g's)
        auto see = [&](const ir::Operand& o) {
            if (o.kind != ir::OperandKind::GLOBAL) return;
            std::string g = o.name.substr(0, o.name.find('+'));
            if (want.count(g)) places[g].insert(where);
        };
        for (const auto& b : fn.blocks)
            for (const auto& in : b.insts) {
                see(in.dest); see(in.src1); see(in.src2);
                for (const auto& a : in.args) see(a);
            }
    }
    // (a resident initializer's pointer to it: its address is stored)
    for (const auto& g : mod.globals) {
        if (g.noescape) continue;
        auto mention = [&](const std::string& l) {
            for (const auto& w : want)
                if (l == w || l.rfind(w + "+", 0) == 0) places[w].insert("data");
        };
        for (const auto& l : g.initLabels) mention(l);
        for (const auto& [o, l] : g.initWords) mention(l);
    }
    for (const auto& w : want) {
        auto it = places.find(w);
        if (it != places.end() && it->second.size() == 1 && *it->second.begin() != "data")
            noescapeSeg_[w] = *it->second.begin();
    }
    for (const auto& [w, seg] : noescapeSeg_) kept.insert(w);

    // Calls passing such an address (Issue #301): a pointer value is from
    // kept data when computed from a kept label or such a value (its
    // address, a copy, an offset, an element, a pointer loaded from a kept
    // table, a local it was stored in). ln45 warns when the callee can reach
    // another overlay, which would replace the data while it runs.
    for (const auto& fn : mod.functions) {
        if (deadCodeFunctions_.count(fn.name) || inlineOnlyFunctions_.count(fn.name)) continue;
        std::set<uint32_t> from;
        auto isKept = [&](const ir::Operand& o) {
            return (o.kind == ir::OperandKind::GLOBAL && kept.count(o.name.substr(0, o.name.find('+')))) ||
                   (o.isVreg() && from.count(o.vregId));
        };
        for (bool changed = true; changed; ) {
            changed = false;
            for (const auto& b : fn.blocks)
                for (const auto& in : b.insts) {
                    if (in.op == ir::Op::CALL || in.op == ir::Op::CALL_VOID || in.op == ir::Op::CALL_INDIRECT) continue;
                    bool k = isKept(in.src1) || isKept(in.src2);
                    for (const auto& a : in.args) k = k || isKept(a);
                    if (!k) continue;
                    if (in.op == ir::Op::STORE) {
                        // (stored in a local: the local holds it)
                        if (in.src2.isVreg() && fn.localSlotVregs.count(in.src2.vregId) && isKept(in.src1) &&
                            from.insert(in.src2.vregId).second) changed = true;
                        continue;
                    }
                    if (!in.dest.isVreg()) continue;
                    switch (in.op) {
                        case ir::Op::ADDR_GLOBAL: case ir::Op::COPY: case ir::Op::ADD: case ir::Op::SUB:
                        case ir::Op::ADDR_ELEM: case ir::Op::LOAD: case ir::Op::PHI:
                            break;
                        default:
                            continue;
                    }
                    if (in.dest.type != ir::Type::PTR && in.dest.type != ir::Type::I16) continue;
                    if (from.insert(in.dest.vregId).second) changed = true;
                }
        }
        for (const auto& b : fn.blocks)
            for (const auto& in : b.insts) {
                if (in.op != ir::Op::CALL && in.op != ir::Op::CALL_VOID && in.op != ir::Op::CALL_INDIRECT) continue;
                bool k = false;
                for (const auto& a : in.args) k = k || isKept(a);
                if (!k) continue;
                std::string callee = in.op == ir::Op::CALL_INDIRECT || in.src1.kind != ir::OperandKind::GLOBAL
                                   ? std::string("icall") : in.src1.name;
                auto key = std::make_pair(fn.name, callee);
                if (!noescapeCalls_.count(key)) noescapeCalls_[key] = in.loc.line;
            }
    }
}

void IRCodeGen::emitGlobals(const ir::Module& mod, bool relocMode) {
    // Deduplicate globals: extern decl + definition → keep definition only
    std::set<std::string> emittedGlobals;
    // __bank(n) variables (#69): in the text sub-segment "bank.n", which ln45
    // keeps in bank n (with -S: resident)
    if (relocMode)
        for (const auto& g : mod.globals)
            if (g.bank >= 0) noescapeSeg_[g.name] = "bank." + std::to_string(g.bank);

    // Emit .global directives for non-static globals and functions
    if (relocMode) {
        // __section variables: ln45 defines their symbols (an object's part
        // of a section: cc45.sectsize.NAME bytes; its address from
        // cc45.section.NAME or ln45 --section; each variable's offset in the
        // part cc45.sectvar.NAME.<sym>; an initial value in data as
        // cc45.sectinit.<sym>, copied there at startup)
        for (const auto& [n, a] : mod.sections) {
            if (a < 0) continue;
            emit(".weak cc45.section." + n);
            emit("cc45.section." + n + " = " + std::to_string(a));
        }
        for (const auto& [n, size] : sectSize_) {
            emit(".weak cc45.sectsize." + n);
            emit("cc45.sectsize." + n + " = " + std::to_string(size));
        }
        for (const auto& [name, sp] : sectPlace_) {
            emit(".extern " + sectLinkName(name));
            emit(".global cc45.sectvar." + sp.section + "." + sectLinkName(name));
            for (const auto& o : mod.globals)
                if (o.name == name && o.hasInitValue) { emit(".global cc45.sectinit." + sectLinkName(name)); break; }
        }
        for (const auto& g : mod.globals) {
            if (sectPlace_.count(g.name)) continue;
            if (zpPlace_.count(g.name)) {
                // a __zp global: its size for ln45, its initial value in data
                if (!emittedGlobals.insert(g.name).second) continue;
                emit(".global cc45.zpvar." + zpLinkName(g.name));
                bool init = false;
                for (const auto& o : mod.globals) if (o.name == g.name && o.hasInitValue) init = true;
                if (init) emit(".global cc45.zpinit." + zpLinkName(g.name));
                continue;
            }
            if (!g.isStatic && !emittedGlobals.count(g.name)) {
                emit(".global " + g.name);
                emittedGlobals.insert(g.name);
                // __noescape data kept in code: ln45 gives it no call stub
                // and refuses a reference from outside its overlay
                if (noescapeSeg_.count(g.name) && g.bank < 0) {
                    emit(".global cc45.noescape." + g.name);
                    emit("cc45.noescape." + g.name + " = 1");
                }
            }
        }
        // calls passing kept data's address: cc45.nesc.<caller>.<callee> =
        // the call's line (ln45 warns when the callee reaches another overlay)
        for (const auto& [k, line] : noescapeCalls_) {
            std::string m = "cc45.nesc." + k.first + "." + k.second;
            emit(".global " + m);
            emit(m + " = " + std::to_string(std::max(line, 1)));
        }
        for (const auto& fn : mod.functions) {
            // Phase 91.3: Skip global declarations for dead code functions
            if (deadCodeFunctions_.count(fn.name)) {
                continue;  // Don't emit .global/.weak for dead code
            }
            // Phase 91.3.3: Skip global declarations for inline-only functions
            if (inlineOnlyFunctions_.count(fn.name)) {
                continue;  // Don't emit .global/.weak for inline-only functions
            }
            if (!fn.isStatic) {
                if (fn.isWeak) emit(".weak " + fn.name);
                else emit(".global " + fn.name);
            } else if (!mod.sourceFile.empty()) {
                // A static function has a module-unique name (_name__<file>):
                // exporting it lets the linker attribute its call sites to it
                // rather than to the preceding exported function
                emit(".global " + fn.name);
            }
        }
        emitBlank();
    }

    // Deduplicate globals for data/bss emission: if a name appears multiple times
    // (extern decl + definition), keep the last occurrence (the definition).
    std::map<std::string, size_t> globalLastIdx;
    for (size_t i = 0; i < mod.globals.size(); i++) {
        globalLastIdx[mod.globals[i].name] = i;
    }

    // Emit data section for initialized globals
    bool hasData = false;
    bool hasBss = false;
    std::string curSeg;               // the segment an initialized global went to
    std::set<std::string> padded;     // "code"/"far": __noescape data begun there
    for (size_t gi = 0; gi < mod.globals.size(); gi++) {
        const auto& g = mod.globals[gi];
        if (globalLastIdx[g.name] != gi) continue; // skip earlier duplicate

        // A __zp global (relocatable output): no storage here, ln45 places it
        // in the base page; an initial value goes in data as cc45.zpinit.<sym>
        // (copied there at startup). With -S it is a plain global.
        auto sp = sectPlace_.find(g.name);
        if (sp != sectPlace_.end()) {
            if (!relocMode) {
                // -S: an equate at the file's address of the section
                emit(g.name + " = " + std::to_string(mod.sections.at(sp->second.section) + sp->second.offset));
                continue;
            }
            emit("cc45.sectvar." + sp->second.section + "." + sectLinkName(g.name) + " = " +
                 std::to_string(sp->second.offset));
            if (!g.hasInitValue) continue;
        }
        bool zpVar = zpPlace_.count(g.name) > 0;
        std::string label = zpVar ? "cc45.zpinit." + zpLinkName(g.name)
                          : sp != sectPlace_.end() ? "cc45.sectinit." + sectLinkName(g.name) : g.name;
        if (zpVar) {
            emit("cc45.zpvar." + zpLinkName(g.name) + " = " + std::to_string(zpPlace_[g.name].size));
            if (!g.hasInitValue) continue;
        }
        if (g.hasInitValue) {
            // Initialized global: goes to data segment (a __noescape one to
            // the code it is kept with: placeNoescape).
            // Must switch back from BSS if we were emitting uninitialized globals.
            auto ns = noescapeSeg_.find(g.name);
            if (ns != noescapeSeg_.end()) {
                if (curSeg != ns->second) {
                    emit(".segment \"" + ns->second + "\"");
                    // (a relocation at offset 0 cannot be encoded: see data's pad)
                    if (padded.insert(ns->second).second) emit(".byte 0");
                    curSeg = ns->second;
                    hasBss = false;
                }
            } else if (!hasData || hasBss || curSeg != "data") {
                curSeg = "data";
                if (relocMode) {
                    emit(".segment \"data\"");
                    if (!hasData) {
                        // Pad byte: .o65 relocation encoding uses delta $00 as
                        // end-of-table, so relocations at segment offset 0 can't
                        // be encoded. A 1-byte pad ensures offset >= 1.
                        emit(".byte 0");
                    }
                }
                hasData = true;
                hasBss = false; // reset so next uninit global re-emits .segment "bss"
            }
            if (g.align > 1) {
                emit(".align " + std::to_string(g.align));
                dataAlign_ = std::max(dataAlign_, g.align);
            }
            emitSplit(label);   // (after the .align: it becomes the section's alignment)
            emitLabel(label);
            if (g.hasImage) {
                // Byte image: plain bytes, address words and string text
                int n = (int)g.initBytes.size();
                for (int off = 0; off < n; ) {
                    auto w = g.initWords.find(off);
                    if (w != g.initWords.end()) { emit(".word " + w->second); off += 2; continue; }
                    auto st = g.initStrings.find(off);
                    if (st != g.initStrings.end()) {
                        std::string line = stringDirective(st->second.value, st->second.escMask, st->second.encoding);
                        if (!line.empty()) emit(line);
                        off += (int)st->second.value.size();
                        continue;
                    }
                    std::string run;
                    int cnt = 0;
                    while (off < n && cnt < 16 && !g.initWords.count(off) && !g.initStrings.count(off)) {
                        run += (cnt ? ", " : "") + std::to_string((int)g.initBytes[off]);
                        ++off; ++cnt;
                    }
                    emit(".byte " + run);
                }
                if (n < g.size) emit(".res " + std::to_string(g.size - n));
                continue;
            }
            // Phase 3: Vtable — emit function address entries
            if (!g.vtableMethodNames.empty()) {
                for (size_t slot = 0; slot < g.vtableMethodNames.size(); slot++) {
                    const auto& methodName = g.vtableMethodNames[slot];
                    if (!methodName.empty()) {
                        emit(".word " + methodName);
                        // Phase 4: Emit .vtable_entry for linker devirtualization
                        if (relocMode) {
                            emit(".vtable_entry " + g.name + ", " + std::to_string(slot) + ", " + methodName);
                        }
                    } else {
                        emit(".word 0"); // empty vtable slot
                    }
                }
            } else if (!g.initList.empty()) {
                // Array initializer
                for (size_t i = 0; i < g.initList.size(); i++) {
                    // Check for symbolic label reference (e.g. string literal pointer)
                    if (i < g.initLabels.size() && !g.initLabels[i].empty()) {
                        // (a far pointer: bank 0; a near one: the low byte, #68)
                        if (g.type == ir::Type::I8) { emit(".byte <(" + g.initLabels[i] + ")"); continue; }
                        emit(".word " + g.initLabels[i]);
                        if (g.type == ir::Type::I32) emit(".word 0");
                    } else if (g.type == ir::Type::I8) emit(".byte " + std::to_string((int)(g.initList[i] & 0xFF)));
                    else if (g.type == ir::Type::I32) emit(".dword " + std::to_string((int)g.initList[i]));
                    else emit(".word " + std::to_string((int)(g.initList[i] & 0xFFFF)));
                }
                // Handle partial initialization: padding with zeros
                int bytesEmitted = (int)g.initList.size() * ir::typeSize(g.type);
                if (bytesEmitted < g.size) {
                    emit(".res " + std::to_string(g.size - bytesEmitted));
                }
            } else if (!g.initLabels.empty() && !g.initLabels[0].empty()) {
                // Scalar symbolic reference (e.g. char *p = "hello"; a far
                // pointer's high bytes 0, a near one's low byte, #68; a
                // banked object's far address, #69)
                if (g.farLabel) emit(".dword " + g.initLabels[0]);
                else if (g.type == ir::Type::I8) emit(".byte <(" + g.initLabels[0] + ")");
                else emit(".word " + g.initLabels[0]);
                if (g.type == ir::Type::I32 && !g.farLabel) emit(".word 0");
            } else {
                // Scalar initializer
                if (g.type == ir::Type::F32) {
                    // Convert stored double bits to CBM 40-bit float
                    double val; std::memcpy(&val, &g.initValue, sizeof(double));
                    uint8_t cbm[5]; doubleToCBM40(val, cbm);
                    emit(".byte $" + hex8(cbm[0]) + ", $" + hex8(cbm[1]) + ", $" +
                         hex8(cbm[2]) + ", $" + hex8(cbm[3]) + ", $" + hex8(cbm[4]));
                } else if (g.type == ir::Type::I8) {
                    emit(".byte " + std::to_string((int)(g.initValue & 0xFF)));
                } else if (g.type == ir::Type::I32) {
                    emit(".dword " + std::to_string((int)g.initValue));
                } else {
                    emit(".word " + std::to_string((int)(g.initValue & 0xFFFF)));
                }
                // Scalar might be larger than its type if it's an array init to zero?
                // e.g. char a[10] = 0;
                int typeSz = ir::typeSize(g.type);
                if (typeSz < g.size) {
                    emit(".res " + std::to_string(g.size - typeSz));
                }
            }
        } else {
            // Uninitialized global: goes to bss segment
            curSeg = "bss";
            if (!hasBss) {
                if (hasData) emitBlank();
                if (relocMode) emit(".segment \"bss\"");
                hasBss = true;
            }
            if (g.align > 1) {
                emit(".align " + std::to_string(g.align));
                bssAlign_ = std::max(bssAlign_, g.align);
            }
            emitSplit(g.name);
            emitLabel(g.name);
            emit(".res " + std::to_string(g.size > 0 ? g.size : ir::typeSize(g.type)));
        }
    }
    if (hasData || hasBss) emitBlank();

    // Debug information (#202): the module's globals and statics, and the
    // layouts of the structs and unions their types name
    for (const auto& t : mod.debugTypes) emit(".dbg_type \"" + t + "\"");
    for (const auto& d : mod.debugGlobals)
        emitDebugVar(d.function.empty() ? (d.fileStatic ? "static" : "global") : "static",
                     d.function.empty() ? "-" : d.function, d.name, d.type, "sym:" + d.symbol);
    flushDebugVars();

    // Switch to code segment (only in reloc mode; PRG mode is linear)
    if (relocMode) {
        emit(".segment \"code\"");
    }
    emitBlank();
}

void IRCodeGen::emitStrings(const ir::Module& mod) {
    if (mod.strings.empty()) return;
    emitBlank();
    // a __noescape literal (#pragma cc45 noescape_strings) is kept with the
    // code using it: in the far chunk for a __far function, in the overlay
    // for an overlay object's code (Issue #301); the others are resident
    auto segOf = [&](const ir::Module::StringLiteral& sl) -> std::string {
        if (!relocMode_ || !sl.noescape) return "data";
        if (sl.far) return "far";
        return mod.overlay.empty() ? "data" : "code";
    };
    std::string cur;
    for (const char* seg : {"data", "code", "far"})
    for (const auto& sl : mod.strings) {
        if (segOf(sl) != seg) continue;
        if (relocMode_ && cur != seg) { emit(std::string(".segment \"") + seg + "\""); cur = seg; }
        emitSplit(sl.label);
        emitLabel(sl.label);
        if (sl.wide) {
            // L"...": each character in a 16-bit unit (low byte first)
            for (size_t i = 0; i < sl.value.size(); i++) {
                emit(stringDirective(sl.value.substr(i, 1), i < sl.escMask.size() ? sl.escMask.substr(i, 1) : ".", sl.encoding));
                emit(".byte 0");
            }
            emit(".word 0");
            continue;
        }
        std::string line = stringDirective(sl.value, sl.escMask, sl.encoding);
        if (!line.empty()) emit(line);
        emit(".byte 0");
    }
}

// The directive for a string literal's text (no terminator): .text (PETSCII),
// .ascii or .screencode. \n/\r are left for the assembler to map for the
// directive (PETSCII newline is $0D); other escapes and non-printable bytes
// become exact 3-digit octal escapes. Empty for an empty string.
std::string IRCodeGen::stringDirective(const std::string& value, const std::string& escMask, int encoding) {
    const std::string dir = encoding == 1 ? ".ascii" : encoding == 2 ? ".screencode" : ".text";
    std::string out;
    for (size_t i = 0; i < value.size(); ++i) {
        unsigned char c = (unsigned char)value[i];
        char kind = i < escMask.size() ? escMask[i] : '.';
        if (kind == 'n') { out += "\\n"; continue; }
        if (kind == 'r') { out += "\\r"; continue; }
        if (kind == '.' && c == '"') { out += "\\\""; continue; }
        if (kind == '.' && c == '\\') { out += "\\\\"; continue; }
        if (kind == '.' && c >= 0x20 && c < 0x7F) { out += (char)c; continue; }
        char oct[5];
        snprintf(oct, sizeof(oct), "\\%03o", c);
        out += oct;
    }
    return out.empty() ? std::string() : dir + " \"" + out + "\"";
}

// ============================================================================
// Function emission
// ============================================================================

// ============================================================================
// Per-function clobber analysis
// ============================================================================

IRCodeGen::FuncClobbers IRCodeGen::computeFuncClobbers(const ir::Function& fn) {
    // Register bit constants
    constexpr uint8_t A = 0x01, X = 0x02, Y = 0x04, Z = 0x08;
    constexpr uint8_t C = 0x01, N = 0x02, ZF = 0x04, V = 0x08;

    FuncClobbers fc;

    for (const auto& block : fn.blocks) {
        for (const auto& inst : block.insts) {
            switch (inst.op) {
                case ir::Op::NOP:
                    break;

                case ir::Op::CONST:
                    fc.regs |= A | X;    // lda #imm; ldx #imm
                    if (inst.resultType == ir::Type::I32) fc.regs |= Y | Z;  // ldy/ldz for I32
                    fc.flags |= N | ZF;
                    break;

                case ir::Op::ADD:
                case ir::Op::SUB:
                    fc.regs |= A | X;    // simulated add.16/sub.16 uses AX
                    fc.flags |= C | N | ZF | V;
                    if (inst.resultType == ir::Type::I32) fc.regs |= Y | Z;
                    break;

                case ir::Op::MUL:
                case ir::Op::MUL_U:
                case ir::Op::DIV:
                case ir::Op::DIV_U:
                case ir::Op::MOD:
                case ir::Op::MOD_U:
                    fc.regs |= A | X;    // hardware math, result in AX
                    fc.flags |= C | N | ZF | V;
                    if (inst.resultType == ir::Type::I32) fc.regs |= Y | Z;
                    break;

                case ir::Op::NEG:
                case ir::Op::NOT:
                    fc.regs |= A | X;
                    fc.flags |= C | N | ZF;
                    break;

                case ir::Op::AND:
                case ir::Op::OR:
                case ir::Op::XOR:
                    fc.regs |= A | X;
                    fc.flags |= N | ZF;
                    if (inst.resultType == ir::Type::I32) fc.regs |= Y | Z;
                    break;

                case ir::Op::SHL:
                case ir::Op::SHR:
                case ir::Op::ASR:
                    fc.regs |= A | X;
                    fc.flags |= C | N | ZF;
                    break;

                case ir::Op::CMP_EQ: case ir::Op::CMP_NE:
                case ir::Op::CMP_LT: case ir::Op::CMP_LE:
                case ir::Op::CMP_GT: case ir::Op::CMP_GE:
                case ir::Op::CMP_LTU: case ir::Op::CMP_LEU:
                case ir::Op::CMP_GTU: case ir::Op::CMP_GEU:
                    fc.regs |= A | X;    // cmp + set result in AX
                    fc.flags |= C | N | ZF | V;
                    break;

                case ir::Op::LOAD:
                    fc.regs |= A | X;
                    fc.flags |= N | ZF;
                    if (inst.resultType == ir::Type::I32) fc.regs |= Y | Z;
                    if (inst.src1.kind != ir::OperandKind::GLOBAL) fc.regs |= Y; // ldy #0 for indirect
                    break;

                case ir::Op::STORE:
                    fc.regs |= A | X;
                    fc.flags |= N | ZF;
                    if (inst.resultType == ir::Type::I32) fc.regs |= Y | Z;
                    if (inst.src2.kind != ir::OperandKind::GLOBAL) fc.regs |= Y; // ldy for indirect
                    break;

                case ir::Op::LOAD_ZP:
                case ir::Op::STORE_ZP:
                    fc.regs |= A;
                    fc.flags |= N | ZF;
                    break;

                case ir::Op::FAR_LOAD:
                case ir::Op::FAR_STORE:
                case ir::Op::NEAR_LOAD:
                case ir::Op::NEAR_STORE:
                case ir::Op::NEAR_ADDR:
                    fc.regs |= A | X | Y | Z;
                    fc.flags |= N | ZF;
                    break;

                case ir::Op::ADDR_GLOBAL:
                    fc.regs |= A | X;
                    fc.flags |= N | ZF;
                    break;

                case ir::Op::ADDR_LOCAL:
                    fc.regs |= A | X;    // leax.fp uses AX
                    fc.flags |= C | N | ZF;
                    break;

                case ir::Op::ADDR_ELEM:
                    fc.regs |= A | X;    // mul.16 + add.16
                    fc.flags |= C | N | ZF | V;
                    break;

                case ir::Op::SEXT:
                    fc.regs |= A | X;    // sxt.8/sxt.16 modify AX
                    fc.flags |= C | N | ZF;
                    break;

                case ir::Op::ZEXT:
                    fc.regs |= A | X;    // ldx #0
                    fc.flags |= N | ZF;
                    break;

                case ir::Op::TRUNC:
                    fc.regs |= A | X;    // load into AX, keep low
                    fc.flags |= N | ZF;
                    break;

                case ir::Op::BFEXT:
                case ir::Op::BFINS:
                    fc.regs |= A | X;
                    fc.flags |= C | N | ZF;
                    break;

                case ir::Op::COPY:
                case ir::Op::DEREF:
                    fc.regs |= A | X;
                    // Y only needed for struct copy (multi-byte) or I32
                    if (inst.resultType == ir::Type::I32) fc.regs |= Y | Z;
                    else if (inst.resultType != ir::Type::I8 && inst.resultType != ir::Type::I16) fc.regs |= Y;
                    fc.flags |= N | ZF;
                    break;

                case ir::Op::BR:
                case ir::Op::PHI:
                    break; // no register effects

                case ir::Op::BR_COND:
                    fc.regs |= A;        // chknonzero.8 tests A
                    fc.flags |= N | ZF;
                    break;

                case ir::Op::VEC_BIN:
                    fc.regs |= A | X | Y;
                    fc.flags |= C | N | ZF | V;
                    break;

                case ir::Op::RET:
                    fc.regs |= A | X;    // return value in AX
                    fc.flags |= N | ZF;
                    if (inst.src1.type == ir::Type::I32) fc.regs |= Y | Z;
                    break;

                case ir::Op::RET_VOID:
                    break;

                case ir::Op::CALL:
                case ir::Op::CALL_VOID:
                    // Direct calls: defer clobber info to second pass (callee union)
                    // Only mark as non-leaf here; second pass adds callee clobbers
                    fc.isLeaf = false;
                    break;

                case ir::Op::CALL_INDIRECT:
                    // Indirect calls: target unknown, conservatively clobber everything
                    fc.regs |= A | X | Y | Z;
                    fc.flags |= C | N | ZF | V;
                    fc.isLeaf = false;
                    break;

                case ir::Op::ASM_INLINE:
                    // Inline asm — conservatively assume everything clobbered
                    fc.regs |= A | X | Y | Z;
                    fc.flags |= C | N | ZF | V;
                    break;

                case ir::Op::VA_START:
                    fc.regs |= A | X;
                    fc.flags |= C | N | ZF;
                    break;

                case ir::Op::CPU_REG_READ:
                    fc.regs |= A | X; // result goes to AX
                    if (inst.resultType == ir::Type::I32) fc.regs |= Y | Z;
                    fc.flags |= N | ZF;
                    break;

                case ir::Op::CPU_REG_WRITE:
                    fc.regs |= A | X | Y | Z; // conservative write
                    fc.flags |= N | ZF;
                    break;

                case ir::Op::CPU_FLAG_READ:
                    fc.regs |= A; // lda #0; adc #0
                    fc.flags |= C | N | ZF | V;
                    break;

                case ir::Op::CPU_FLAG_WRITE:
                    fc.regs |= A; // lda val
                    fc.flags |= C | N | ZF | V;
                    break;
            }
        }
    }

    return fc;
}

// Stores to the vreg pool ($30-$7F) that nothing in the function reads: a
// temporary written to its register although every use took it from A/X
// (lda x; sta $30; stx $31; asl; ...). The pool holds only the function's own
// temporaries and variables: a callee, a caller or the next call of the
// function never reads them. Not with inline asm, which may read anything.
void IRCodeGen::emitFunctionPruned(const ir::Function& fn, bool relocMode, bool nestedInModule) {
    bool hasAsm = false;
    for (const auto& blk : fn.blocks)
        for (const auto& in : blk.insts) if (in.op == ir::Op::ASM_INLINE) hasAsm = true;
    if (hasAsm || fn.isNaked || nestedInModule || getenv("CC45_NO_POOL_PRUNE")) {
        emitFunction(fn, relocMode);
        return;
    }
    std::ostringstream buf;
    std::streambuf* old = out_.rdbuf(buf.rdbuf());
    try {
        emitFunction(fn, relocMode);
    } catch (...) {
        out_.rdbuf(old);
        throw;
    }
    out_.rdbuf(old);
    out_ << pruneDeadFlagRestores(pruneDeadLdx(pruneDeadPoolStores(buf.str())));
}

// The N/Z flags of a dropped load restored on the fall-through into a label
// (restoreFlags: ora #0, or inx / dex; marked kFlagRestore): not needed when
// the code that follows sets N/Z before anything reads them. (Not dropped
// before rts: a caller tests the result by the flags the call leaves.) Stores and the
// carry instructions are skipped; a branch, call, jump, return, php or an
// unknown (simulated) instruction keeps them (Issue #297).
std::string IRCodeGen::pruneDeadFlagRestores(const std::string& text) {
    std::vector<std::string> lines;
    {
        std::istringstream in(text);
        std::string l;
        while (std::getline(in, l)) lines.push_back(l);
    }
    const std::string mark = std::string("; ") + kFlagRestore;
    static const std::set<std::string> sets = {
        "lda", "ldx", "ldy", "ldz", "cmp", "cpx", "cpy", "cpz", "adc", "sbc", "and", "ora", "eor", "inc", "dec",
        "inx", "dex", "iny", "dey", "inz", "dez", "asl", "lsr", "rol", "ror", "tax", "tay", "txa", "tya", "taz",
        "tza", "tsx", "pla", "plx", "ply", "plz", "bit", "neg", "asr", "ldq"};
    static const std::set<std::string> leaves = {"sta", "stx", "sty", "stz", "stq", "clc", "sec", "cld", "nop", "pha",
                                                  "phx", "phy", "phz"};
    auto parse = [](const std::string& l, std::string& mn) {
        size_t q = l.find_first_not_of(" \t");
        if (q == std::string::npos || l[q] == ';') return 0;                         // blank, comment
        if (l.compare(q, 4, ".loc") == 0 || l.compare(q, 4, ".var") == 0 || l.compare(q, 5, ".dbg_") == 0) return 0;
        if (l[q] == '.') return 2;                                                   // another directive
        if (l.substr(0, l.find(';')).find(':') != std::string::npos) return 3;       // a label (not a comment's ':')
        std::string body = l.substr(q);
        size_t sp = body.find_first_of(" \t;");
        mn = body.substr(0, sp);
        std::transform(mn.begin(), mn.end(), mn.begin(), ::tolower);
        return 1;
    };
    std::vector<bool> drop(lines.size(), false);
    for (size_t i = 0; i < lines.size(); i++) {
        size_t p = lines[i].find_first_not_of(" \t");
        if (p == std::string::npos || lines[i].compare(p, mark.size(), mark) != 0) continue;
        // the restore: ora #0, or inx / dex
        std::string mn;
        size_t j = i + 1, n = 0;
        if (j < lines.size() && parse(lines[j], mn) == 1 && mn == "ora") n = 1;
        else if (j + 1 < lines.size() && parse(lines[j], mn) == 1 && mn == "inx") n = 2;
        drop[i] = true;   // (the mark itself)
        if (!n) continue;
        bool dead = false;
        for (size_t k = j + n; k < lines.size(); k++) {
            int t = parse(lines[k], mn);
            if (t == 0 || t == 3) continue;
            if (t != 1) break;
            if (leaves.count(mn)) continue;
            dead = sets.count(mn) > 0;
            break;
        }
        if (dead) for (size_t k = j; k < j + n; k++) drop[k] = true;
    }
    std::string out;
    for (size_t i = 0; i < lines.size(); i++)
        if (!drop[i]) { out += lines[i]; out += '\n'; }
    return out;
}

// A load of X just before a ZP-convention call (marked kXFreeCall), with
// nothing in between that reads or writes X, is dead: the arguments are in
// $10-$2F, and an 8-bit value's ldx #0 (its high byte) is never used
// there (Issue #296). Removes it and the marks.
std::string IRCodeGen::pruneDeadLdx(const std::string& text) {
    std::vector<std::string> lines;
    {
        std::istringstream in(text);
        std::string l;
        while (std::getline(in, l)) lines.push_back(l);
    }
    const std::string mark = std::string("; ") + kXFreeCall;
    // instructions that neither read nor write X (with no ,x operand)
    static const std::set<std::string> noX = {
        "lda", "sta", "ldy", "sty", "ldz", "stz", "adc", "sbc", "and", "ora", "eor", "cmp", "cpy", "cpz",
        "bit", "inc", "dec", "asl", "lsr", "rol", "ror", "clc", "sec", "tay", "tya", "taz", "tza", "iny",
        "dey", "inz", "dez", "neg", "nop"};
    std::vector<bool> drop(lines.size(), false);
    for (size_t i = 0; i < lines.size(); i++) {
        size_t p = lines[i].find_first_not_of(" \t");
        if (p == std::string::npos || lines[i].compare(p, mark.size(), mark) != 0) continue;
        drop[i] = true;
        for (size_t j = i; j-- > 0; ) {
            if (drop[j]) continue;
            const std::string& l = lines[j];
            size_t q = l.find_first_not_of(" \t");
            if (q == std::string::npos || l[q] == ';' || l.compare(q, 4, ".loc") == 0 ||
                l.compare(q, 5, ".dbg_") == 0) continue;
            if (l[q] == '.' || l.substr(0, l.find(';')).find(':') != std::string::npos || l[q] == '@')
                break;   // a directive, a label (not a comment's ':')
            std::string body = l.substr(q);
            size_t c = body.find(';');
            if (c != std::string::npos) body = body.substr(0, c);
            size_t sp = body.find_first_of(" \t");
            std::string mn = body.substr(0, sp), op = sp == std::string::npos ? "" : body.substr(sp);
            std::transform(mn.begin(), mn.end(), mn.begin(), ::tolower);
            std::transform(op.begin(), op.end(), op.begin(), ::tolower);
            if (mn == "ldx" && !op.empty() && op.find(",") == std::string::npos) { drop[j] = true; break; }
            if (!noX.count(mn) || op.find(",x") != std::string::npos || op.find(", x") != std::string::npos) break;
        }
    }
    std::string out;
    for (size_t i = 0; i < lines.size(); i++)
        if (!drop[i]) { out += lines[i]; out += '\n'; }
    return out;
}

std::string IRCodeGen::pruneDeadPoolStores(const std::string& text) {
    struct Line { std::string text, mn, operand; };
    std::vector<Line> lines;
    {
        std::istringstream in(text);
        std::string l;
        while (std::getline(in, l)) {
            Line ln{l, "", ""};
            size_t p = l.find_first_not_of(" \t");
            // (the code before a comment: -Rcodegen's "  ; [... ZP:$42]" is no label)
            std::string code = l.substr(0, l.find(';'));
            if (p != std::string::npos && l[p] != ';' && l[p] != '.' && code.find(':') == std::string::npos) {
                std::string body = code.substr(p);
                while (!body.empty() && isspace((unsigned char)body.back())) body.pop_back();
                size_t sp = body.find_first_of(" \t");
                ln.mn = body.substr(0, sp);
                if (sp != std::string::npos) {
                    ln.operand = body.substr(body.find_first_not_of(" \t", sp));
                    while (!ln.operand.empty() && isspace((unsigned char)ln.operand.back())) ln.operand.pop_back();
                }
                std::transform(ln.mn.begin(), ln.mn.end(), ln.mn.begin(), ::tolower);
            } else if (p != std::string::npos && l[p] != ';' && l.compare(p, 4, ".var") != 0 &&
                       l.compare(p, 4, ".loc") != 0 && l.compare(p, 5, ".dbg_") != 0) {
                ln.mn = ".";   // a directive or a label line: any address in it counts as read
                ln.operand = code.substr(p);
            }
            lines.push_back(ln);
        }
    }
    // a pool address $HH or $HH+k at s[i]: its value, or -1
    auto poolAddr = [](const std::string& s, size_t i, size_t* end) -> int {
        if (s[i] != '$' || i + 2 >= s.size() + 0 || !isxdigit((unsigned char)s[i + 1]) ||
            !isxdigit((unsigned char)s[i + 2]) || (i + 3 < s.size() && isxdigit((unsigned char)s[i + 3])))
            return -1;
        int v = std::stoi(s.substr(i + 1, 2), nullptr, 16);
        size_t j = i + 3;
        if (j + 1 < s.size() && s[j] == '+' && isdigit((unsigned char)s[j + 1])) {
            size_t k = j + 1;
            while (k < s.size() && isdigit((unsigned char)s[k])) k++;
            v += std::stoi(s.substr(j + 1, k - j - 1));
            j = k;
        }
        if (end) *end = j;
        return v >= ZP_POOL_START && v < ZAR_START ? v : -1;
    };
    static const std::set<std::string> byteOps = {
        "lda", "ldx", "ldy", "ldz", "adc", "sbc", "cmp", "cpx", "cpy", "cpz", "and", "ora", "eor",
        "bit", "inc", "dec", "asl", "lsr", "rol", "ror", "asr", "tsb", "trb"};
    static const std::set<std::string> wordOps = {"inw", "dew", "asw", "row"};
    // a plain store's address and width (0: not one)
    auto plainStore = [&](const Line& ln, int& addr) -> int {
        int w = (ln.mn == "sta" || ln.mn == "stx" || ln.mn == "sty" || ln.mn == "stz") ? 1 : ln.mn == "stq" ? 4 : 0;
        if (!w || ln.operand.empty()) return 0;
        size_t end = 0;
        addr = poolAddr(ln.operand, 0, &end);
        return addr >= 0 && end == ln.operand.size() ? w : 0;
    };
    // the pool bytes a line reads (false: it reaches bytes it does not name)
    auto readsOf = [&](const Line& ln, std::set<int>& rd) -> bool {
        // ([bp],z: a 32-bit pointer, #68)
        bool indirect32 = ln.operand.find('[') != std::string::npos;
        bool indirect = indirect32 || ln.operand.find('(') != std::string::npos;
        // (anything else, a simulated op on a float or a long: up to 8 bytes)
        int w = indirect32 ? 4 : indirect ? 2 : byteOps.count(ln.mn) ? 1 : wordOps.count(ln.mn) ? 2 : 8;
        for (size_t i = 0; i < ln.operand.size(); i++) {
            size_t end;
            int v = poolAddr(ln.operand, i, &end);
            if (v < 0) continue;
            // indexed ($30,x: a copy loop over a float) or its address taken
            // (#$30, #<$30): bytes reached that the operand does not name
            bool indexed = !indirect && end < ln.operand.size() && ln.operand[end] == ',';
            size_t h = ln.operand.find_last_not_of("<> ", i ? i - 1 : 0);
            bool immediate = i > 0 && h != std::string::npos && ln.operand[h] == '#';
            if (indexed || immediate || ln.mn == ".") return false;
            for (int k = 0; k < w; k++) rd.insert(v + k);
            i = end - 1;
        }
        return true;
    };
    std::set<int> read;
    for (const auto& ln : lines) {
        if (ln.mn.empty()) continue;
        int a;
        if (plainStore(ln, a)) continue;
        if (!readsOf(ln, read)) return text;
    }
    std::vector<bool> drop(lines.size(), false);
    for (size_t i = 0; i < lines.size(); i++) {
        int a, w = lines[i].mn.empty() ? 0 : plainStore(lines[i], a);
        bool dead = w > 0;
        for (int k = 0; dead && k < w; k++) if (read.count(a + k)) dead = false;
        if (dead) drop[i] = true;
    }
    // A store whose bytes are all stored again further on in the same
    // straight-line code, none read before: dead (a temp computed, stored
    // to a global and overwritten: sta $30 / sta g / lda #5 / sta $30; #293).
    // Labels, branches, calls and simulated instructions end the run.
    for (size_t i = 0; i < lines.size(); i++) {
        int a, w = drop[i] || lines[i].mn.empty() ? 0 : plainStore(lines[i], a);
        if (!w) continue;
        std::set<int> pending;
        for (int k = 0; k < w; k++) pending.insert(a + k);
        for (size_t j = i + 1; j < lines.size() && !pending.empty(); j++) {
            const Line& ln = lines[j];
            if (ln.mn.empty()) continue;
            const std::string& m = ln.mn;
            if (m == "." || m[0] == 'b' || m == "jmp" || m == "jsr" || m == "rts" || m == "rti" ||
                m == "brk" || m.find('.') != std::string::npos)
                break;
            int b, wb = plainStore(ln, b);
            if (wb) {
                for (int k = 0; k < wb; k++) pending.erase(b + k);
                continue;
            }
            std::set<int> rd;
            if (!readsOf(ln, rd)) break;
            bool hit = false;
            for (int x : pending) if (rd.count(x)) { hit = true; break; }
            if (hit) break;
        }
        if (pending.empty()) drop[i] = true;
    }
    std::string out;
    for (size_t i = 0; i < lines.size(); i++) {
        if (drop[i]) continue;
        out += lines[i].text;
        out += '\n';
    }
    return out;
}

void IRCodeGen::emitFunction(const ir::Function& fn, bool relocMode, bool isMainWithZPSave) {
    emitComment("function " + fn.name);
    zpBitAlias_.clear();

    // Set current function context (used by loadVreg/storeVreg for SAC addressing)
    currentFunctionName_ = fn.name;

    // Phase 78: Initialize SMC parameter tracking for this function
    if (staticAllocMode_ && sacFunctions_.count(fn.name) > 0) {
        currentFunctionParams_ = fn.paramNames;
        trackSMCOffsetsEnabled_ = true;
        currentInstructionOffset_ = 0;
    } else {
        trackSMCOffsetsEnabled_ = false;
    }

    // Phase 47: Initialize IR tracking for this function
    initIRForFunction(fn.name, fn);

    // Bug #179 fix: Skip prescan here. Prescan runs before the allocator and doesn't know
    // which vRegs will be allocated to ZP vs frame. This causes overlapping frame offsets.
    // Instead, we allocate based on allocator decisions below.
    // prescanFunction(fn);  // REMOVED

    // Run register allocator. In a leaf function (no calls, no inline asm)
    // nothing can clobber the ZP register pool while it runs: its pointer,
    // char and int locals live there like register variables (a dereference
    // uses the pointer in place, a loop counter is a ZP access).
    regVregs_ = fn.registerVregs;
    kb_ = getenv("CC45_NO_KNOWN_BITS") ? ir::KnownBits{} : ir::computeKnownBits(fn);
    {
        bool leaf = true;
        for (const auto& blk : fn.blocks)
            for (const auto& in : blk.insts)
                if (in.op == ir::Op::CALL || in.op == ir::Op::CALL_VOID || in.op == ir::Op::CALL_INDIRECT ||
                    in.op == ir::Op::ASM_INLINE)
                    leaf = false;
        if (leaf && !getenv("CC45_NO_ZP_POINTERS")) {
            uint32_t paramCount = (uint32_t)fn.paramTypes.size();
            // (a variable whose address is used, such as a small struct read
            // member by member, stays in memory)
            std::set<uint32_t> addressedLocals;
            std::set<uint32_t> farAddrs;   // (a far pointer: its base-page bytes are its home, #348)
            for (const auto& blk : fn.blocks)
                for (const auto& in : blk.insts) {
                    if (in.op == ir::Op::ADDR_LOCAL && in.src1.isVreg()) addressedLocals.insert(in.src1.vregId);
                    const ir::Operand& fa = in.op == ir::Op::FAR_LOAD ? in.src1 : in.src2;
                    if ((in.op == ir::Op::FAR_LOAD || in.op == ir::Op::FAR_STORE) && fa.isVreg()) farAddrs.insert(fa.vregId);
                }
            for (const auto& [name, vid] : fn.localNames) {
                if (vid < paramCount || fn.memoryVregs.count(vid) || fn.vregSizes.count(vid) ||
                    addressedLocals.count(vid))
                    continue;
                if (fn.registerXVregs.count(vid) || fn.registerYVregs.count(vid) || fn.registerZVregs.count(vid)) continue;
                auto t = fn.vregTypes.find(vid);
                // pointers, and the char and int variables (loop counters):
                // zero-page accesses instead of absolute ones
                if (t != fn.vregTypes.end() &&
                    (t->second == ir::Type::PTR || t->second == ir::Type::I16 || t->second == ir::Type::I8 ||
                     (t->second == ir::Type::I32 && farAddrs.count(vid))))
                    regVregs_.insert(vid);
            }
        }
    }
    alloc_.setRegisterVregs(regVregs_);
    // A ZP-convention leaf function uses its parameters where the caller put
    // them, in the ZP parameter block: no copy into storage at the entry
    zpParams_.clear();
    // vregs whose address is used (&x, a struct's members, a vector's
    // elements): they need an absolute address, not a ZP one
    std::set<uint32_t> addressed;
    for (const auto& blk : fn.blocks)
        for (const auto& in : blk.insts) {
            // (an element address of a vector parameter is its address; of a
            // pointer, its value. Fused into a load/store, args[0] is the base)
            auto elemBase = [&](const ir::Operand& b) {
                if (!b.isVreg()) return;
                auto t = b.vregId < fn.paramTypes.size() ? fn.paramTypes[b.vregId]
                         : (fn.vregTypes.count(b.vregId) ? fn.vregTypes.at(b.vregId) : ir::Type::PTR);
                if (t != ir::Type::PTR) addressed.insert(b.vregId);
            };
            if (in.op == ir::Op::ADDR_LOCAL && in.src1.isVreg()) addressed.insert(in.src1.vregId);
            if (in.op == ir::Op::ADDR_ELEM) elemBase(in.src1);
            if ((in.op == ir::Op::LOAD || in.op == ir::Op::STORE) && in.args.size() >= 3) elemBase(in.args[0]);
            // a parameter inline asm names by its static copy (_f__param_x)
            // must have that copy
            if (in.op == ir::Op::ASM_INLINE)
                for (uint32_t i = 0; i < fn.paramTypes.size() && i < fn.paramNames.size(); i++)
                    if (asmNamesSymbol(in.asmText, fn.name + "__param_" + fn.paramNames[i])) addressed.insert(i);
        }
    if (ir::paramsStayInZp(fn) && !isZeroAllocLeaf(fn.name, fn)) {
        int zpOff = (int)zeroPageStart_ + 8;
        for (uint32_t i = 0; i < fn.paramTypes.size(); i++) {
            int ps = ir::typeSize(fn.paramTypes[i]);
            if (!fn.memoryVregs.count(i) && !fn.vregSizes.count(i) && !addressed.count(i)) zpParams_[i] = zpOff;
            zpOff += ps < 2 ? 2 : ps;
        }
    } else if (fn.conv == ir::CallConv::ZP && !fn.isVariadic && !fn.isRegparm && !fn.isNaked &&
               !fn.isInterrupt && !fn.isNested && !fn.blocks.empty() && !getenv("CC45_NO_ZP_PARAMS")) {
        // A function that makes calls: a parameter used only in the entry
        // block before its first call (or asm) is used where the caller put
        // it too; the block is untouched until that call stores its own
        // arguments. (Not with float code: the float runtime is called
        // without an IR call.)
        bool floats = false;
        for (auto t : fn.paramTypes) if (t == ir::Type::F32) floats = true;
        for (const auto& blk : fn.blocks)
            for (const auto& in : blk.insts)
                if (in.resultType == ir::Type::F32 || in.src1.type == ir::Type::F32 || in.src2.type == ir::Type::F32)
                    floats = true;
        // Each block's first call, and the blocks a call can reach (after a
        // call the parameter block holds that call's arguments): a parameter
        // used only where no call has run yet is used in place, not copied
        // into storage by the entry thunk (Issue #292). (A computed goto:
        // the entry block only, as its targets are not known.)
        size_t nb = fn.blocks.size();
        auto isCall = [](const ir::Inst& in) {
            return in.op == ir::Op::CALL || in.op == ir::Op::CALL_VOID ||
                   in.op == ir::Op::CALL_INDIRECT || in.op == ir::Op::ASM_INLINE;
        };
        std::vector<size_t> firstCall(nb);
        std::map<std::string, size_t> blockAt;
        bool indirect = false;
        for (size_t b = 0; b < nb; b++) {
            const auto& insts = fn.blocks[b].insts;
            firstCall[b] = insts.size();
            for (size_t k = 0; k < insts.size(); k++) if (isCall(insts[k])) { firstCall[b] = k; break; }
            blockAt[fn.blocks[b].label] = b;
            for (const auto& in : insts) if (in.op == ir::Op::BR_INDIRECT || in.op == ir::Op::ADDR_LABEL) indirect = true;
        }
        // a label whose address a nested function takes (its goto to a label
        // of this one) is entered from there, after calls
        std::set<std::string> takenLabels;
        if (currentModule_)
            for (const auto& f : currentModule_->functions)
                for (const auto& b : f.blocks)
                    for (const auto& in : b.insts)
                        if (in.op == ir::Op::ADDR_LABEL) takenLabels.insert(in.src1.name);
        std::vector<bool> afterCall(nb, false);   // entered after a call may have run
        if (indirect) {
            for (size_t b = 1; b < nb; b++) afterCall[b] = true;
        } else {
            auto succ = [&](size_t b) {
                std::vector<size_t> out;
                const auto& insts = fn.blocks[b].insts;
                auto add = [&](const ir::Operand& o) {
                    if (o.kind != ir::OperandKind::LABEL) return;
                    auto it = blockAt.find(o.name);
                    if (it != blockAt.end()) out.push_back(it->second);
                };
                if (insts.empty()) { if (b + 1 < nb) out.push_back(b + 1); return out; }
                const auto& t = insts.back();
                if (t.op == ir::Op::BR) add(t.src1);
                else if (t.op == ir::Op::BR_COND) { add(t.dest); add(t.src2); }
                else if (t.op == ir::Op::SWITCH) {
                    add(t.src2);
                    for (const auto& sc : t.switchCases) {
                        auto it = blockAt.find(sc.second);
                        if (it != blockAt.end()) out.push_back(it->second);
                    }
                } else if (t.op != ir::Op::RET && t.op != ir::Op::RET_VOID && b + 1 < nb) out.push_back(b + 1);
                return out;
            };
            std::vector<size_t> work;
            for (size_t b = 0; b < nb; b++)
                if (firstCall[b] < fn.blocks[b].insts.size()) work.push_back(b);
            std::vector<bool> seen(nb, false);
            while (!work.empty()) {
                size_t b = work.back(); work.pop_back();
                if (seen[b]) continue;
                seen[b] = true;
                for (size_t n : succ(b)) { afterCall[n] = true; if (!seen[n]) work.push_back(n); }
            }
            // A block no branch of the function reaches is entered some other
            // way (a nested function's goto to a label of this one, after the
            // calls that led there: nestfunc-5): after a call
            std::vector<bool> reach(nb, false);
            std::vector<size_t> todo{0};
            while (!todo.empty()) {
                size_t b = todo.back(); todo.pop_back();
                if (reach[b]) continue;
                reach[b] = true;
                for (size_t n : succ(b)) if (!reach[n]) todo.push_back(n);
            }
            for (size_t b = 0; b < nb; b++)
                if (!reach[b] || takenLabels.count(fn.blocks[b].label)) afterCall[b] = true;
            // (and so are the blocks those reach)
            std::vector<size_t> more;
            for (size_t b = 0; b < nb; b++) if (afterCall[b] && takenLabels.count(fn.blocks[b].label)) more.push_back(b);
            while (!more.empty()) {
                size_t b = more.back(); more.pop_back();
                for (size_t n : succ(b)) if (!afterCall[n]) { afterCall[n] = true; more.push_back(n); }
            }
        }
        int zpOff = (int)zeroPageStart_ + 8;
        for (uint32_t i = 0; i < fn.paramTypes.size() && !floats; i++) {
            int ps = ir::typeSize(fn.paramTypes[i]);
            bool ok = !fn.memoryVregs.count(i) && !fn.vregSizes.count(i) && !addressed.count(i) &&
                      !fn.registerVregs.count(i);
            bool used = false;
            auto uses = [&](const ir::Inst& in) {
                auto is = [i](const ir::Operand& o) { return o.isVreg() && o.vregId == i; };
                if (is(in.src1) || is(in.src2) || is(in.dest)) return true;
                for (const auto& a : in.args) if (is(a)) return true;
                for (const auto& ph : in.phiIncoming) if (is(ph.first)) return true;
                return false;
            };
            for (size_t b = 0; b < fn.blocks.size() && ok; b++)
                for (size_t k = 0; k < fn.blocks[b].insts.size() && ok; k++)
                    if (uses(fn.blocks[b].insts[k])) {
                        used = true;
                        if (afterCall[b] || k >= firstCall[b]) ok = false;
                    }
            if (ok && used) zpParams_[i] = zpOff;
            zpOff += ps < 2 ? 2 : ps;
        }
    }
    // A static-allocation function that makes calls keeps its pointer
    // variables in a base-page block of its own ($80..., .zar), which ln45
    // places where no function active at the same time has its block: they
    // survive the calls, and a dereference uses the pointer in place. In the
    // ZP convention its pointer parameters go there too (the entry thunk
    // copies them from the parameter block)
    zarVregs_.clear();
    zarSize_ = 0;
    {
        bool leaf = true;
        for (const auto& blk : fn.blocks)
            for (const auto& in : blk.insts)
                if (in.op == ir::Op::CALL || in.op == ir::Op::CALL_VOID || in.op == ir::Op::CALL_INDIRECT ||
                    in.op == ir::Op::ASM_INLINE)
                    leaf = false;
        bool willSac = staticAllocMode_ && !fn.isRecurse && !fn.isInterrupt && !fn.isNaked &&
                       !callsRuntimeAlloca(fn) && !recursiveFunctions_.count(fn.name);
        int zarMax = ZAR_MAX;
        if (const char* m = getenv("CC45_ZP_LOCALS_MAX")) zarMax = std::max(0, std::min(atoi(m), ZAR_END - ZAR_START));
        // (the candidates are recorded for ln45 --zp-plan-out whatever the
        // options; -fzp-plan's choice wins over -fno-zp-locals and the cap)
        const bool zarAllowed = zarMax > 0 && !getenv("CC45_NO_ZP_LOCALS");
        if (!leaf && willSac && relocMode && !getenv("CC45_NO_ZP_POINTERS")) {
            const uint32_t paramCount = (uint32_t)fn.paramTypes.size();
            bool fnZpConv = fn.conv == ir::CallConv::ZP && !fn.isVariadic;
            auto typeOf = [&](uint32_t v) {
                if (v < paramCount) return fn.paramTypes[v];
                auto t = fn.vregTypes.find(v);
                return t != fn.vregTypes.end() ? t->second : ir::Type::VOID;
            };
            // references, those in loop blocks counted 4 times; and (for the
            // program-wide plan, #304: code size) each counted once, and the
            // loads and stores through each
            std::map<uint32_t, int> refs, uses, derefs;
            for (const auto& blk : fn.blocks) {
                const std::string& l = blk.label;
                bool inLoop = (l.rfind("while_", 0) == 0 || l.rfind("for_", 0) == 0 || l.rfind("do_", 0) == 0) &&
                              l.find("_end") == std::string::npos;
                int w = inLoop ? 4 : 1;
                for (const auto& in : blk.insts) {
                    for (const ir::Operand* o : {&in.src1, &in.src2, &in.dest})
                        if (o->isVreg()) { refs[o->vregId] += w; uses[o->vregId]++; }
                    for (const auto& a : in.args) if (a.isVreg()) { refs[a.vregId] += w; uses[a.vregId]++; }
                }
            }
            // loads and stores through a variable: their address is the
            // variable plus a constant (add ptr v, c; addr_elem v, ...; a copy),
            // or a pointer temporary itself (a store to a variable's slot is an
            // assignment, not one)
            {
                std::map<uint32_t, int> nDef;
                std::map<uint32_t, const ir::Inst*> defOf;
                for (const auto& blk : fn.blocks)
                    for (const auto& in : blk.insts)
                        if (in.dest.isVreg()) { nDef[in.dest.vregId]++; defOf[in.dest.vregId] = &in; }
                auto rootOf = [&](uint32_t a) {
                    for (int depth = 0; depth < 8; depth++) {
                        auto d = defOf.find(a);
                        if (fn.localSlotVregs.count(a) || a < fn.paramTypes.size() || d == defOf.end() || nDef[a] != 1) break;
                        const ir::Inst& di = *d->second;
                        if ((di.op == ir::Op::ADD || di.op == ir::Op::SUB || di.op == ir::Op::COPY || di.op == ir::Op::ADDR_ELEM) &&
                            di.src1.isVreg())
                            a = di.src1.vregId;
                        else break;
                    }
                    return a;
                };
                for (const auto& blk : fn.blocks)
                    for (const auto& in : blk.insts) {
                        const ir::Operand* addr = in.op == ir::Op::LOAD ? &in.src1 : in.op == ir::Op::STORE ? &in.src2 : nullptr;
                        if (!addr || !addr->isVreg()) continue;
                        uint32_t a = addr->vregId, r = rootOf(a);
                        if (r != a || !(fn.localSlotVregs.count(a) || a < fn.paramTypes.size())) derefs[r]++;
                    }
            }
            std::vector<uint32_t> cands;
            auto consider = [&](uint32_t v) {
                ir::Type t = typeOf(v);
                // (a long local is a "memory" vreg of 4 bytes, like a struct:
                // fine when its address is not used)
                bool plainLong = t == ir::Type::I32 && fn.vregSizes.count(v) && fn.vregSizes.at(v) == 4;
                if ((t != ir::Type::PTR && t != ir::Type::I8 && t != ir::Type::I16 && t != ir::Type::I32) ||
                    zpParams_.count(v) ||
                    (fn.memoryVregs.count(v) && !plainLong) ||
                    addressed.count(v) ||
                    (fn.vregSizes.count(v) && !plainLong) || fn.registerVregs.count(v) || fn.registerXVregs.count(v) ||
                    fn.registerYVregs.count(v) || fn.registerZVregs.count(v) || !refs.count(v))
                    return;
                if (std::find(cands.begin(), cands.end(), v) == cands.end()) cands.push_back(v);
            };
            if (fnZpConv && !fn.isRegparm)
                for (uint32_t i = 0; i < paramCount; i++) consider(i);
            for (const auto& name : fn.localNamesOrder) {
                auto it = fn.localNames.find(name);
                if (it != fn.localNames.end() && it->second >= paramCount) consider(it->second);
            }
            // pointer temporaries dereferenced and live across a call (kept in
            // static storage otherwise: copied to __zp_scratch at each use)
            {
                std::map<uint32_t, std::pair<int, int>> span;   // first, last mention
                std::vector<int> calls;
                int idx = 0;
                std::set<uint32_t> derefd;
                for (const auto& blk : fn.blocks)
                    for (const auto& in : blk.insts) {
                        if (in.op == ir::Op::CALL || in.op == ir::Op::CALL_VOID || in.op == ir::Op::CALL_INDIRECT)
                            calls.push_back(idx);
                        if (in.op == ir::Op::LOAD && in.src1.isVreg() && in.args.empty()) derefd.insert(in.src1.vregId);
                        if (in.op == ir::Op::STORE && in.src2.isVreg() && in.args.empty()) derefd.insert(in.src2.vregId);
                        auto mention = [&](const ir::Operand& o) {
                            if (!o.isVreg()) return;
                            auto sp = span.find(o.vregId);
                            if (sp == span.end()) span[o.vregId] = {idx, idx};
                            else sp->second.second = idx;
                        };
                        mention(in.dest); mention(in.src1); mention(in.src2);
                        for (const auto& a : in.args) mention(a);
                        idx++;
                    }
                for (uint32_t v : derefd) {
                    if (v < paramCount || fn.localSlotVregs.count(v) || typeOf(v) != ir::Type::PTR) continue;
                    auto sp = span.find(v);
                    if (sp == span.end()) continue;
                    bool crosses = false;
                    for (int c : calls) if (c > sp->second.first && c < sp->second.second) crosses = true;
                    if (crosses) consider(v);
                }
            }
            // pointers first (a dereference needs the pointer in ZP), then by use
            std::stable_sort(cands.begin(), cands.end(), [&](uint32_t a, uint32_t b) {
                bool pa = typeOf(a) == ir::Type::PTR, pb = typeOf(b) == ir::Type::PTR;
                if (pa != pb) return pa;
                return refs[a] > refs[b];
            });
            // ln45's program-wide plan (-fzp-plan, #304): a function it lists
            // takes exactly the variables it chose (no cap: the plan fits the
            // area); one it does not know keeps the rule above
            const std::set<std::string>* planned = nullptr;
            if (const auto& plan = zpPlan(); !plan.empty()) {
                auto it = plan.find(fn.name);
                if (it != plan.end()) planned = &it->second;
            }
            // (its storage symbol: sacSymbol's, but currentFn_ is not set yet)
            auto symOf = [&](uint32_t v) {
                if (v < paramCount)
                    return fn.name + "__param_" + (v < fn.paramNames.size() && !fn.paramNames[v].empty()
                                                       ? fn.paramNames[v] : std::to_string(v));
                return fn.name + "__local_" + std::to_string(v);
            };
            for (uint32_t v : cands) {
                int sz = typeOf(v) == ir::Type::I8 ? 1 : typeOf(v) == ir::Type::I32 ? 4 : 2;
                // the candidate's record (cc45.zc.<storage symbol>): its size,
                // whether a pointer, its uses and loads/stores through it -- for
                // ln45 --zp-plan-out
                uint32_t rec = (uint32_t)sz | (typeOf(v) == ir::Type::PTR ? 0x10u : 0u) |
                               ((uint32_t)std::min(uses[v], 4095) << 8) | ((uint32_t)std::min(derefs[v], 4095) << 20);
                emit((fn.isWeak ? ".weak cc45.zc." : ".global cc45.zc.") + symOf(v));
                emit("cc45.zc." + symOf(v) + " = " + std::to_string(rec));
                if (planned ? !planned->count(symOf(v)) || zarSize_ + sz > zarPlanLimit_
                            : (!zarAllowed || zarSize_ + sz > zarMax)) continue;
                zarVregs_[v] = ZAR_START + zarSize_;
                zarSize_ += sz;
            }
        }
    }
    findArgTargets(fn);
    findStoreTargets(fn);
    {
        std::map<uint32_t, int> fixed = zpParams_;
        fixed.insert(zarVregs_.begin(), zarVregs_.end());
        fixed.insert(argTargets_.begin(), argTargets_.end());
        fixed.insert(storeTargets_.begin(), storeTargets_.end());
        alloc_.setFixedZpVregs(fixed);
    }
    alloc_.setRegisterXVregs(fn.registerXVregs);
    alloc_.setRegisterYVregs(fn.registerYVregs);
    alloc_.setRegisterZVregs(fn.registerZVregs);
    byteJoins_ = findByteJoins(fn);
    byteJoinSkip_.clear();
    {
        std::vector<std::pair<uint32_t, uint32_t>> live;
        for (const auto& [root, bj] : byteJoins_)
            for (const auto& b : bj.bytes) if (b.kind == 1) live.push_back({b.vreg, root});
        alloc_.setLiveUntil(live);
    }
    alloc_.analyze(fn, ZP_POOL_START, ZP_POOL_SLOTS);

    // Populate vregOffset_ from allocator decisions (frame-allocated vregs only)
    // This prevents IRCodeGen from re-allocating frame slots that VRegAllocator already assigned
    vregOffset_.clear();
    vregType_.clear();
    vregSizes_ = fn.vregSizes;

    for (uint32_t vid = 0; vid < fn.nextVreg; vid++) {
        auto alloc = alloc_.getAlloc(vid);
        if (alloc.loc == VRegAllocator::IN_FRAME) {
            vregOffset_[vid] = alloc.offset;
            vregType_[vid] = alloc.type;
        }
    }

    // Set frameSize from allocator (which computed optimal frame layout)
    frameSize_ = alloc_.frameSize();

    // Copy local slot info from IR function
    localSlotVregs_ = fn.localSlotVregs;
    vregConstVal_.clear();

    // Pointer + constant offsets used only as load/store addresses: the ADD
    // is not emitted; the access indexes the base pointer with Y = offset
    // (p->m, a[3], v[i] with a constant index)
    foldedAddrs_.clear();
    elemAbsY_.clear();
    zextSkip_.clear();
    {
        // Every use of each vreg: (block, index, how)
        struct Use { size_t blk, idx; bool addrOnly; };
        std::map<uint32_t, std::vector<Use>> uses;
        std::vector<const ir::Block*> blocks;
        for (const auto& blk : fn.blocks) blocks.push_back(&blk);
        for (size_t b = 0; b < blocks.size(); b++) {
            const auto& insts = blocks[b]->insts;
            for (size_t i = 0; i < insts.size(); i++) {
                const auto& in = insts[i];
                auto note = [&](const ir::Operand& o, bool addr) {
                    if (o.isVreg()) uses[o.vregId].push_back({b, i, addr});
                };
                bool plain = in.args.empty();
                note(in.src1, plain && in.op == ir::Op::LOAD && in.resultType != ir::Type::F32);
                note(in.src2, plain && in.op == ir::Op::STORE && in.resultType != ir::Type::F32 &&
                              !(in.src1.isVreg() && in.src2.isVreg() && in.src1.vregId == in.src2.vregId));
                for (const auto& a : in.args) note(a, false);
            }
        }
        for (size_t b = 0; b < blocks.size(); b++) {
            const auto& insts = blocks[b]->insts;
            for (size_t i = 0; i < insts.size(); i++) {
                const auto& in = insts[i];
                // (a COPY of a pointer: offset 0)
                bool isCopy = (in.op == ir::Op::COPY || in.op == ir::Op::DEREF) && in.resultType == ir::Type::PTR;
                if (!isCopy && (in.op != ir::Op::ADD || in.resultType != ir::Type::PTR ||
                    !in.src2.isImm() || in.src2.immVal < 0 || in.src2.immVal > 250))
                    continue;
                if (!in.dest.isVreg() || !in.src1.isVreg()) continue;
                uint32_t d = in.dest.vregId, base = in.src1.vregId;
                if (fn.localSlotVregs.count(d) || fn.memoryVregs.count(d)) continue;
                // A base kept only in registers may be gone by the access
                auto bl = alloc_.getAlloc(base).loc;
                if (bl != VRegAllocator::IN_ZP && bl != VRegAllocator::IN_FRAME) continue;
                if (fn.memoryVregs.count(base)) continue;  // could change through a pointer
                auto uit = uses.find(d);
                if (uit == uses.end() || uit->second.empty()) continue;
                bool ok = true;
                size_t last = i;
                for (const auto& u : uit->second) {
                    if (!u.addrOnly || u.blk != b || u.idx <= i) { ok = false; break; }
                    last = std::max(last, u.idx);
                }
                if (!ok) continue;
                // A temporary base lives (for the allocator) only up to the
                // ADD: then only one access right after it can use it
                if ((!fn.localSlotVregs.count(base) || regVregs_.count(base)) &&
                    (uit->second.size() != 1 || uit->second[0].idx != i + 1)) continue;
                // The base keeps its value up to the last use
                for (size_t j = i + 1; j <= last && ok; j++) {
                    const auto& x = insts[j];
                    if (x.dest.isVreg() && x.dest.vregId == base) ok = false;
                    if (x.op == ir::Op::STORE && x.src2.isVreg() && x.src2.vregId == base) ok = false;
                }
                if (!ok) continue;
                foldedAddrs_[d] = {base, isCopy ? 0 : (int)in.src2.immVal};
            }
        }
        // Element addresses of a global (ADDR_ELEM) used only as load/store
        // addresses in their block, the index in the base page and unchanged
        // up to the last use: base+off,y at each access, no address built
        // (#318). Y = a byte index (any array), or index * stride (an array
        // of at most 256 bytes: every element offset fits in Y). Bytes through
        // a pointer in the base page with a byte index likewise: (p),y
        for (size_t b = 0; b < blocks.size(); b++) {
            const auto& insts = blocks[b]->insts;
            for (size_t i = 0; i < insts.size(); i++) {
                const auto& in = insts[i];
                if (in.op != ir::Op::ADDR_ELEM || !in.dest.isVreg() ||
                    !in.src2.isVreg() || in.args.empty() || in.elemOffset < 0) continue;
                uint32_t d = in.dest.vregId, ix = in.src2.vregId;
                int stride = (int)in.args[0].immVal;
                // (or a pointer in the base page, a byte index: (p),y, bytes
                // only; Y does not carry into a second byte)
                bool zpPtr = in.src1.isVreg() && in.elemOffset == 0 && stride == 1 &&
                             alloc_.getAlloc(in.src1.vregId).loc == VRegAllocator::IN_ZP &&
                             !fn.memoryVregs.count(in.src1.vregId);
                if (in.src1.kind != ir::OperandKind::GLOBAL && !zpPtr) continue;
                if (fn.localSlotVregs.count(d) || fn.memoryVregs.count(d) || foldedAddrs_.count(d)) continue;
                // a byte widened only for the index (%8 = zext %0; a[%8]): the
                // byte is the index, the ZEXT left out (zextSkip_)
                ir::Operand indexOp = in.src2;
                uint32_t zextOf = 0;
                size_t zextAt = i;
                if (stride == 1 && alloc_.getAlloc(ix).type != ir::Type::I8 && !fn.localSlotVregs.count(ix)) {
                    int ndef = 0;
                    const ir::Inst* zd = nullptr;
                    for (size_t j = 0; j < insts.size(); j++)
                        if (insts[j].dest.isVreg() && insts[j].dest.vregId == ix) { ndef++; zd = &insts[j]; zextAt = j; }
                    for (size_t bb = 0; bb < blocks.size(); bb++)
                        if (bb != b) for (const auto& x : blocks[bb]->insts) if (x.dest.isVreg() && x.dest.vregId == ix) ndef++;
                    if (ndef == 1 && zd && zd->op == ir::Op::ZEXT && zd->src1.isVreg() && zextAt < i &&
                        alloc_.getAlloc(zd->src1.vregId).type == ir::Type::I8 &&
                        (fn.localSlotVregs.count(zd->src1.vregId) && !regVregs_.count(zd->src1.vregId))) {
                        auto u8 = uses.find(ix);
                        if (u8 != uses.end() && u8->second.size() == 1) {   // (the ADDR_ELEM only)
                            zextOf = ix;
                            indexOp = zd->src1;
                            ix = zd->src1.vregId;
                        }
                    }
                }
                if (!zextOf) zextAt = i;
                auto ia = alloc_.getAlloc(ix);
                if (ia.loc != VRegAllocator::IN_ZP || fn.memoryVregs.count(ix)) continue;
                bool byteIndex = ia.type == ir::Type::I8 && stride == 1;
                bool small = !zpPtr && smallGlobals_.count(in.src1.name) && (stride == 1 || stride == 2 || stride == 4) &&
                             in.elemOffset < 256;
                if (!byteIndex && !small) continue;
                auto uit = uses.find(d);
                if (uit == uses.end() || uit->second.empty()) continue;
                bool ok = true;
                size_t last = i;
                for (const auto& u : uit->second) {
                    if (!u.addrOnly || u.blk != b || u.idx <= i) { ok = false; break; }
                    const auto& x = insts[u.idx];
                    if (zpPtr && x.resultType != ir::Type::I8) ok = false;
                    if (x.op == ir::Op::LOAD) {
                        if (x.resultType != ir::Type::I8 && x.resultType != ir::Type::I16 && x.resultType != ir::Type::PTR) ok = false;
                    } else {
                        // a byte; (Y through A for a stride: the value must not need Y)
                        if (x.resultType != ir::Type::I8) ok = false;
                        if (!byteIndex && !(x.src1.isImm() || (x.src1.isVreg() &&
                              alloc_.getAlloc(x.src1.vregId).loc == VRegAllocator::IN_ZP))) ok = false;
                    }
                    if (!ok) break;
                    last = std::max(last, u.idx);
                }
                if (!ok) continue;
                // A temporary index lives (for the allocator) only up to the
                // ADDR_ELEM: then only one access right after it can use it
                // (the pointer likewise)
                uint32_t pb = zpPtr ? in.src1.vregId : ix;
                if (((!fn.localSlotVregs.count(ix) || regVregs_.count(ix)) ||
                     (zpPtr && pb >= fn.paramTypes.size() && (!fn.localSlotVregs.count(pb) || regVregs_.count(pb)))) &&
                    (uit->second.size() != 1 || uit->second[0].idx != i + 1)) continue;
                // The index (and pointer) keep their values up to the last use (from the ZEXT)
                for (size_t j = zextAt + 1; j <= last && ok; j++) {
                    const auto& x = insts[j];
                    for (uint32_t v : {ix, pb}) {
                        if (x.dest.isVreg() && x.dest.vregId == v) ok = false;
                        if (x.op == ir::Op::STORE && x.src2.isVreg() && x.src2.vregId == v) ok = false;
                    }
                }
                if (!ok) continue;
                elemAbsY_[d] = {in.src1, in.elemOffset, indexOp, byteIndex ? 1 : stride};
                if (zextOf) zextSkip_.insert(zextOf);
            }
        }
        // A byte TRUNC whose one use is a byte STORE (through a pointer or to
        // a global) later in the block, with only folded addresses between:
        // not emitted; the STORE loads the source's low byte where it lives
        truncAlias_.clear();
        std::map<uint32_t, int> defs;
        for (const auto& blk : fn.blocks)
            for (const auto& in : blk.insts) if (in.dest.isVreg()) defs[in.dest.vregId]++;
        for (size_t b = 0; b < blocks.size(); b++) {
            const auto& insts = blocks[b]->insts;
            for (size_t i = 0; i < insts.size(); i++) {
                const auto& in = insts[i];
                if (in.op != ir::Op::TRUNC || in.resultType != ir::Type::I8 || !in.dest.isVreg() ||
                    !in.src1.isVreg() || in.src1.vregId == in.dest.vregId) continue;
                uint32_t d = in.dest.vregId;
                // (a variable: a temporary's register can be reused before the STORE)
                if (defs[d] != 1 || fn.localSlotVregs.count(d) || !fn.localSlotVregs.count(in.src1.vregId)) continue;
                // (a direct address: checked when the TRUNC is emitted, once SAC is known)
                auto sl = alloc_.getAlloc(in.src1.vregId).loc;
                if (sl != VRegAllocator::IN_ZP && sl != VRegAllocator::IN_FRAME) continue;
                auto uit = uses.find(d);
                if (uit == uses.end() || uit->second.size() != 1 || uit->second[0].blk != b) continue;
                size_t j = uit->second[0].idx;
                if (j <= i) continue;
                const auto& st = insts[j];
                if (st.op != ir::Op::STORE || st.resultType != ir::Type::I8 || !st.args.empty() ||
                    !st.src1.isVreg() || st.src1.vregId != d) continue;
                if (!(st.src2.kind == ir::OperandKind::GLOBAL ||
                      (st.src2.isVreg() && st.src2.vregId != d && !fn.localSlotVregs.count(st.src2.vregId))))
                    continue;
                // (uses lists src1/src2/args: also no PHI or STORE-dest use)
                bool ok = true;
                for (const auto& blk2 : fn.blocks)
                    for (const auto& x : blk2.insts) {
                        for (const auto& ph : x.phiIncoming)
                            if (ph.first.isVreg() && ph.first.vregId == d) ok = false;
                        if (&x != &in && x.dest.isVreg() && x.dest.vregId == d) ok = false;
                    }
                // the source keeps its value up to the STORE (which computes
                // its address before it loads the value)
                uint32_t sv = in.src1.vregId;
                for (size_t k = i + 1; k < j && ok; k++) {
                    const auto& x = insts[k];
                    if ((x.dest.isVreg() && x.dest.vregId == sv) ||
                        (x.op == ir::Op::STORE && x.src2.isVreg() && x.src2.vregId == sv) ||
                        x.op == ir::Op::CALL || x.op == ir::Op::CALL_VOID || x.op == ir::Op::CALL_INDIRECT ||
                        x.op == ir::Op::ASM_INLINE)
                        ok = false;
                    // (memory it shares with pointers: only instructions that write no memory)
                    if (fn.memoryVregs.count(sv) &&
                        !(x.op == ir::Op::CONST || x.op == ir::Op::COPY || x.op == ir::Op::ADD ||
                          x.op == ir::Op::SUB || x.op == ir::Op::TRUNC || x.op == ir::Op::ZEXT ||
                          x.op == ir::Op::SEXT || x.op == ir::Op::DEREF || x.op == ir::Op::NOP ||
                          (x.op == ir::Op::LOAD && x.args.empty())))
                        ok = false;
                }
                if (ok && !storeTargets_.count(d)) truncAlias_[d] = in.src1;
            }
        }
        // A byte variable's ZEXT whose one use is a 16-bit ADD/SUB, AND/XOR or
        // compare (as its second operand) later in the block: not emitted;
        // the use reads the byte where it lives and 0 for the high byte
        zextAlias_.clear();
        std::set<uint32_t> joinNodes;
        for (const auto& bj : byteJoins_) joinNodes.insert(bj.second.nodes.begin(), bj.second.nodes.end());
        auto is16 = [](ir::Type t) { return t == ir::Type::I16 || t == ir::Type::PTR; };
        for (size_t b = 0; b < blocks.size(); b++) {
            const auto& insts = blocks[b]->insts;
            for (size_t i = 0; i < insts.size(); i++) {
                const auto& in = insts[i];
                if (in.op != ir::Op::ZEXT || in.src1.type != ir::Type::I8 || !is16(in.resultType) ||
                    !in.dest.isVreg() || !in.src1.isVreg() || in.src1.vregId == in.dest.vregId) continue;
                uint32_t d = in.dest.vregId, sv = in.src1.vregId;
                if (defs[d] != 1 || fn.localSlotVregs.count(d) || fn.memoryVregs.count(d) || joinNodes.count(d))
                    continue;
                auto sl = alloc_.getAlloc(sv).loc;
                if (sl != VRegAllocator::IN_ZP && sl != VRegAllocator::IN_FRAME) continue;
                auto uit = uses.find(d);
                if (uit == uses.end() || uit->second.size() != 1 || uit->second[0].blk != b) continue;
                size_t j = uit->second[0].idx;
                if (j <= i) continue;
                // (a variable or a parameter: a temporary's register can be
                // reused before the use, unless only NOPs come between)
                if (!fn.localSlotVregs.count(sv) && sv >= fn.paramTypes.size()) {
                    bool adjacent = sl == VRegAllocator::IN_ZP;
                    for (size_t k = i + 1; k < j && adjacent; k++) adjacent = insts[k].op == ir::Op::NOP;
                    if (!adjacent) continue;
                }
                const auto& u = insts[j];
                if (!u.args.empty()) continue;
                bool at1 = u.src1.isVreg() && u.src1.vregId == d, at2 = u.src2.isVreg() && u.src2.vregId == d;
                if (at1 == at2) continue;
                bool fits = false;
                switch (u.op) {
                    case ir::Op::ADD: case ir::Op::SUB: case ir::Op::AND: case ir::Op::XOR:
                        fits = is16(u.resultType); break;
                    case ir::Op::CMP_EQ: case ir::Op::CMP_NE: case ir::Op::CMP_LT: case ir::Op::CMP_GE:
                    case ir::Op::CMP_LTU: case ir::Op::CMP_LEU: case ir::Op::CMP_GTU: case ir::Op::CMP_GEU:
                        fits = at2 ? is16(u.src1.type) : (u.src2.isImm() || is16(u.src2.type)); break;
                    default: break;
                }
                if (!fits) continue;
                bool ok = true;
                for (const auto& blk2 : fn.blocks)
                    for (const auto& x : blk2.insts)
                        for (const auto& ph : x.phiIncoming)
                            if (ph.first.isVreg() && ph.first.vregId == d) ok = false;
                // the byte keeps its value up to the use
                for (size_t k = i + 1; k < j && ok; k++) {
                    const auto& x = insts[k];
                    if ((x.dest.isVreg() && x.dest.vregId == sv) ||
                        (x.op == ir::Op::STORE && x.src2.isVreg() && x.src2.vregId == sv) ||
                        x.op == ir::Op::CALL || x.op == ir::Op::CALL_VOID || x.op == ir::Op::CALL_INDIRECT ||
                        x.op == ir::Op::ASM_INLINE)
                        ok = false;
                    if (fn.memoryVregs.count(sv) &&
                        !(x.op == ir::Op::CONST || x.op == ir::Op::COPY || x.op == ir::Op::ADD ||
                          x.op == ir::Op::SUB || x.op == ir::Op::TRUNC || x.op == ir::Op::ZEXT ||
                          x.op == ir::Op::SEXT || x.op == ir::Op::DEREF || x.op == ir::Op::NOP ||
                          (x.op == ir::Op::LOAD && x.args.empty())))
                        ok = false;
                }
                if (ok && !storeTargets_.count(d)) zextAlias_[d] = in.src1;
            }
        }
        // A long, 16-bit value or byte loaded from a global (of this program,
        // not on the base page, not a parameter's storage) whose one use is
        // the next instruction: read by that use where it is
        globalLoadAlias_.clear();
        for (size_t b = 0; b < blocks.size(); b++) {
            const auto& insts = blocks[b]->insts;
            for (size_t i = 0; i + 1 < insts.size(); i++) {
                const auto& in = insts[i];
                bool w32 = in.resultType == ir::Type::I32;
                bool w16 = in.resultType == ir::Type::I16 || in.resultType == ir::Type::PTR;
                bool w8 = in.resultType == ir::Type::I8;
                if (in.op != ir::Op::LOAD || !(w32 || w16 || w8) || !in.args.empty() ||
                    in.src1.kind != ir::OperandKind::GLOBAL || !in.dest.isVreg()) continue;
                const std::string& g = in.src1.name;
                if (g.find("__param_") != std::string::npos) continue;
                if (symbolAddressSpace_.count(g) ? symbolAddressSpace_[g] != 0 : !plainExterns_.count(g)) continue;
                uint32_t d = in.dest.vregId;
                if (defs[d] != 1 || fn.localSlotVregs.count(d) || fn.memoryVregs.count(d) || joinNodes.count(d))
                    continue;
                auto uit = uses.find(d);
                if (uit == uses.end() || uit->second.size() != 1 || uit->second[0].blk != b ||
                    uit->second[0].idx <= i) continue;
                // the use comes next but for instructions that write no
                // memory and read none but parameters' storage (the value
                // read later is the same; volatile reads keep their order)
                size_t j = uit->second[0].idx;
                bool next = true;
                for (size_t x = i + 1; x < j && next; x++) {
                    const auto& y = insts[x];
                    switch (y.op) {
                        case ir::Op::NOP: case ir::Op::CONST: case ir::Op::COPY: case ir::Op::ZEXT:
                        case ir::Op::SEXT: case ir::Op::TRUNC: case ir::Op::ADDR_GLOBAL: case ir::Op::ADDR_LOCAL:
                            break;
                        case ir::Op::LOAD:
                            next = y.args.empty() && y.src1.kind == ir::OperandKind::GLOBAL &&
                                   y.src1.name.find("__param_") != std::string::npos;
                            break;
                        default: next = false;
                    }
                }
                if (!next) continue;
                const auto& u = insts[j];
                bool fits = false;
                // (the use's width: a long's, a byte's, or 16 bits / a pointer's)
                auto sameWidth = [&](ir::Type t) {
                    return w32 ? t == ir::Type::I32 : w8 ? t == ir::Type::I8 : (t == ir::Type::I16 || t == ir::Type::PTR);
                };
                switch (u.op) {
                    case ir::Op::ADD: case ir::Op::SUB:
                        // (not a pointer + offset folded into its loads and
                        // stores: they index the pointer in its ZP home)
                        fits = sameWidth(u.resultType) && u.args.empty() &&
                               !(u.dest.isVreg() && foldedAddrs_.count(u.dest.vregId));
                        break;
                    case ir::Op::CMP_EQ: case ir::Op::CMP_NE: case ir::Op::CMP_LT: case ir::Op::CMP_GE:
                    case ir::Op::CMP_LE: case ir::Op::CMP_GT:
                    case ir::Op::CMP_LTU: case ir::Op::CMP_LEU: case ir::Op::CMP_GTU: case ir::Op::CMP_GEU:
                        fits = sameWidth(u.src1.type) && u.args.empty(); break;
                    case ir::Op::STORE:      // (the value, not the address)
                        fits = sameWidth(u.resultType) && u.src1.isVreg() && u.src1.vregId == d &&
                               !(u.src2.isVreg() && u.src2.vregId == d);
                        break;
                    case ir::Op::ZEXT: case ir::Op::SEXT:   // (a byte widened: loadOperand)
                        fits = w8 && u.args.empty();
                        break;
                    default: break;
                }
                if (!fits) continue;
                bool inPhi = false;
                for (const auto& blk2 : fn.blocks)
                    for (const auto& x : blk2.insts)
                        for (const auto& ph : x.phiIncoming)
                            if (ph.first.isVreg() && ph.first.vregId == d) inPhi = true;
                if (!inPhi && !storeTargets_.count(d)) globalLoadAlias_[d] = g;
            }
        }
    }

    // Pre-scan: identify CONST vregs and detect which are only used as
    // STORE addresses. These can be suppressed (no emit, no frame slot)
    // because the codegen uses the constant value directly for sta $ADDR.
    suppressedVregs_.clear();
    {
        // Pass 1: collect all CONST vreg values. Vregs are not SSA: one defined
        // more than once (a && b's result: CONST 0 on one path, a value on the
        // other) is not a constant
        std::map<uint32_t, int64_t> constDefs;
        std::map<uint32_t, int> defCount;
        for (const auto& blk : fn.blocks) {
            for (const auto& inst : blk.insts) {
                if (inst.dest.isVreg()) defCount[inst.dest.vregId]++;
                if (inst.op == ir::Op::CONST && inst.dest.isVreg())
                    constDefs[inst.dest.vregId] = inst.src1.immVal;
            }
        }
        multiDefVregs_.clear();
        for (const auto& [v, n] : defCount)
            if (n > 1) { multiDefVregs_.insert(v); constDefs.erase(v); }
        // Pass 2: identify CALL instructions where all args are simple
        // (const vregs or immediates). Mark those args' local slot vregs as dead.
        auto isSimpleArg = [&](const ir::Operand& arg) -> bool {
            if (arg.isImm()) return true;
            if (arg.kind == ir::OperandKind::GLOBAL) return true;
            if (arg.isVreg() && constDefs.count(arg.vregId)) return true;
            return false;
        };
        std::set<uint32_t> deadCallArgSlots; // local slots only used by phw-eligible calls
        for (const auto& blk : fn.blocks) {
            for (const auto& inst : blk.insts) {
                if (inst.op != ir::Op::CALL && inst.op != ir::Op::CALL_VOID) continue;
                if (inst.callConv == ir::CallConv::ZP) continue;
                bool allSimple = true;
                for (const auto& a : inst.args)
                    if (!isSimpleArg(a)) { allSimple = false; break; }
                if (allSimple) {
                    for (const auto& a : inst.args)
                        if (a.isVreg()) deadCallArgSlots.insert(a.vregId);
                }
            }
        }

        // Pass 3: for each CONST vreg, check if it can be suppressed.
        // Suppressible if ONLY used as:
        //   (a) src2 of STORE (address operand, not local slot), OR
        //   (b) src1 of STORE where src2 is a known-constant address, OR
        //   (c) arg of an all-simple CALL (phw path), OR
        //   (d) src1 of STORE to a local slot that is a dead call arg slot
        for (auto& [vid, val] : constDefs) {
            bool canSuppress = true;
            bool hasUse = false;
            for (const auto& blk : fn.blocks) {
                for (const auto& inst : blk.insts) {
                    if (inst.op == ir::Op::NOP) continue;
                    // Check args: OK if CALL with all-simple args
                    for (const auto& a : inst.args) {
                        if (a.isVreg() && a.vregId == vid) {
                            if ((inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID) &&
                                deadCallArgSlots.count(vid)) {
                                hasUse = true;
                            } else if ((inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID) &&
                                       inst.callConv == ir::CallConv::ZP && !fn.localSlotVregs.count(vid) &&
                                       a.type != ir::Type::F32) {
                                // an immediate stored straight into the parameter
                                // block (loadOperand), not a register-pool temp
                                // copied there (Issue #290)
                                hasUse = true;
                            } else {
                                canSuppress = false;
                            }
                            break;
                        }
                    }
                    if (!canSuppress) break;
                    // Check src1
                    if (inst.src1.isVreg() && inst.src1.vregId == vid) {
                        if (inst.op == ir::Op::CPU_REG_WRITE && !fn.localSlotVregs.count(vid)) {
                            hasUse = true; // loaded straight into the register (directByte)
                        } else if ((inst.op == ir::Op::MUL || inst.op == ir::Op::MUL_U) && inst.resultType != ir::Type::I32 &&
                            inst.resultType != ir::Type::F32 && !(inst.src1.isVreg() && inst.src2.isVreg() &&
                            inst.src1.vregId == inst.src2.vregId)) {
                            hasUse = true; // a factor of a 16-bit multiply (an immediate there)
                        } else if (inst.op == ir::Op::LOAD && inst.args.empty() &&
                            inst.resultType != ir::Type::F32 && !fn.localSlotVregs.count(vid)) {
                            hasUse = true; // address of a load (absolute)
                        } else if (inst.op == ir::Op::STORE && inst.resultType != ir::Type::F32 &&
                            inst.src2.kind == ir::OperandKind::GLOBAL) {
                            hasUse = true; // value of a global store (loaded as an immediate)
                        } else if (inst.op == ir::Op::STORE && inst.resultType != ir::Type::F32 &&
                                   inst.src2.isVreg() && !fn.localSlotVregs.count(inst.src2.vregId) &&
                                   inst.src2.vregId != vid) {
                            hasUse = true; // value of a store through a pointer
                        } else if (inst.op == ir::Op::STORE && inst.src2.isVreg()) {
                            if (constDefs.count(inst.src2.vregId) &&
                                !fn.localSlotVregs.count(inst.src2.vregId)) {
                                hasUse = true; // value of const-addr store
                            } else if (fn.localSlotVregs.count(inst.src2.vregId) &&
                                       deadCallArgSlots.count(inst.src2.vregId)) {
                                hasUse = true; // store to dead call arg slot
                            } else {
                                canSuppress = false; break;
                            }
                        } else {
                            canSuppress = false; break;
                        }
                    }
                    // Check src2: OK if STORE address (not local slot), CMP, or ADD/SUB operand
                    if (inst.src2.isVreg() && inst.src2.vregId == vid) {
                        if ((inst.op == ir::Op::MUL || inst.op == ir::Op::MUL_U) && inst.resultType != ir::Type::I32 &&
                            inst.resultType != ir::Type::F32 && !(inst.src1.isVreg() && inst.src2.isVreg() &&
                            inst.src1.vregId == inst.src2.vregId)) {
                            hasUse = true; // a factor of a 16-bit multiply (an immediate there)
                        } else if (inst.op == ir::Op::STORE && !fn.localSlotVregs.count(vid)) {
                            hasUse = true;
                        } else if (inst.op == ir::Op::CMP_EQ || inst.op == ir::Op::CMP_NE ||
                                   inst.op == ir::Op::CMP_LT || inst.op == ir::Op::CMP_LE ||
                                   inst.op == ir::Op::CMP_GT || inst.op == ir::Op::CMP_GE ||
                                   inst.op == ir::Op::CMP_LTU || inst.op == ir::Op::CMP_LEU ||
                                   inst.op == ir::Op::CMP_GTU || inst.op == ir::Op::CMP_GEU) {
                            hasUse = true; // CMP already uses vregConstVal_ for immediate
                        } else if (inst.op == ir::Op::ADD || inst.op == ir::Op::SUB) {
                            hasUse = true; // store-fused ADD/SUB uses vregConstVal_ for immediate
                        } else {
                            canSuppress = false; break;
                        }
                    }
                }
                if (!canSuppress) break;
            }
            if (canSuppress && hasUse) {
                suppressedVregs_.insert(vid);
                vregConstVal_[vid] = val;
            }
        }
        // Also suppress the dead call arg LOCAL SLOT vregs themselves
        for (uint32_t slotId : deadCallArgSlots) {
            if (fn.localSlotVregs.count(slotId)) {
                suppressedVregs_.insert(slotId);
            }
        }
    }

    // Bug #179 fix: Skip prescan entirely. Only use allocator's decisions for frame layout.
    // Prescan runs before allocator and doesn't know which vRegs go to ZP vs FRAME.
    // This causes incorrect frame offsets that overlap between locals and temporaries.
    vregSizes_ = fn.vregSizes;  // Still need vregSizes for size overrides

    // Determine SAC mode early (needed for frame address allocation decision)
    // SAC enabled if: (1) -fstaticalloc flag enabled, (2) not recursive, (3) not interrupt/naked/variadic
    bool earlyUseSAC = staticAllocMode_ && !fn.isRecurse && !fn.isInterrupt &&
                       !fn.isNaked;

    // Don't resetFrame() here — vregOffset_ was already populated by allocator at lines 958-968
    frameAddrZPIndex_ = -1;  // reset for new function

    // Allocate frame slots based on ALLOCATOR's decisions only
    // First: allocate named locals and parameters in localNames order (deterministic)
    // All frame allocations are now handled by VRegAllocator
    // vregOffset_ was already populated above, no need for allocSlot calls

    // Allocate a dedicated ZP slot for frame address (2 bytes) if function has frame
    // (NOT needed for SAC functions, which use direct absolute addressing)
    // This prevents leax.fp results from being clobbered by intermediate operations
    if (frameSize_ > 0 && !earlyUseSAC) {
        // The frame's address is the frame pointer itself ($FD/$FE), which is
        // saved and restored around calls (a shared cache slot would be
        // overwritten by callees)
        frameAddrZPIndex_ = 0xFD - (int)zeroPageStart_;
    }

    frameAddrCacheValid_ = true;  // Initialize cache validity for new function
    currentInstIdx_ = 0;

    // For struct-returning functions, emit static buffer in data section
    // This buffer will hold the struct value before it's returned
    std::string structBufferName;
    if (fn.returnType == ir::Type::I32) {
        // Could be a 4-byte struct - emit a static buffer
        // Check if this looks like a struct (all structs we support are I32)
        structBufferName = fn.name + "__struct_buf";
        // Emit buffer declaration (will be resolved to actual address by assembler/linker)
        emitBlank();
        emit("; Static buffer for struct return from " + fn.name);
        emit(structBufferName + ":");
        emit(".byte 0, 0, 0, 0");
        emitBlank();
    }

    // __naked: emit only proc label + body + endproc, no prologue/epilogue
    if (fn.isNaked) {
        emit("proc " + fn.name);
        for (const auto& block : fn.blocks) {
            emitLabel("@" + block.label);
            if (fn.addressTakenLabels.count(block.label)) {
                emit(".global " + fn.labelAlias(block.label));
                emitLabel(fn.labelAlias(block.label));
            }
            for (const auto& inst : block.insts) {
                // (the body's asm returns: no epilogue to branch to, even
                // for a function that returns a value)
                if (inst.op != ir::Op::RET && inst.op != ir::Op::RET_VOID) emitInst(inst);
                currentInstIdx_++;
            }
        }
        emit("endproc");
        flushDebugVars();
        emitBlank();
        return;
    }

    // Phase 1 SAC eligibility: static allocation convention (Phase 1: non-overlapping)
    // Function uses SAC only if ALL criteria hold:
    // 1. -fstaticalloc flag enabled (TU-wide opt-in)
    // 2. Not marked #pragma cc55 recurse (function-level opt-out)
    // 3. Not __interrupt, __naked, or variadic
    // Linker will verify runtime-checkable constraints (4-5): not reachable from ISR, no cycles
    bool usesAlloca = callsRuntimeAlloca(fn);
    // (a variadic function too: its arguments stay on the stack, where the
    // entry thunk finds them; va_start reads the address it keeps, <fn>__va)
    bool useSAC = staticAllocMode_ && !fn.isRecurse && !fn.isInterrupt &&
                  !fn.isNaked && !usesAlloca;
    // Mutual recursion within the module (a direct self-call warns below)
    if (useSAC && recursiveFunctions_.count(fn.name)) {
        bool selfCall = false;
        for (const auto& blk : fn.blocks)
            for (const auto& inst : blk.insts)
                if ((inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID) &&
                    inst.src1.kind == ir::OperandKind::GLOBAL && inst.src1.name == fn.name)
                    selfCall = true;
        if (!selfCall) useSAC = false;
    }
    // This function's own convention (the IRBuilder's per-function decision)
    bool fnZp = fn.conv == ir::CallConv::ZP && !fn.isVariadic;

    // Compile-time self-recursion check (not authoritative, but good UX)
    // Scan for direct calls to this function name
    if (useSAC) {
        bool hasSelfCall = false;
        for (const auto& blk : fn.blocks) {
            for (const auto& inst : blk.insts) {
                if ((inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID) && inst.src1.kind == ir::OperandKind::GLOBAL) {
                    if (inst.src1.name == fn.name) {
                        hasSelfCall = true;
                        break;
                    }
                }
            }
            if (hasSelfCall) break;
        }
        if (hasSelfCall) {
            std::cerr << fn.name << ": warning: recursive function uses a stack frame instead of static allocation"
                      << " (add #pragma cc45 recurse before the function to silence)\n";
            useSAC = false;  // Fall back to a stack frame for this function
        }
    }

    // Store SAC mode for this function (used by loadVreg/storeVreg for addressing)
    currentFunctionUseSAC_ = useSAC;

    // the byte joins whose leaves all have an address (in ZP or SAC storage)
    for (auto it = byteJoins_.begin(); it != byteJoins_.end();) {
        bool ok = true;
        for (const auto& b : it->second.bytes)
            if (b.kind == 1 && directBase(b.vreg).empty()) ok = false;
        if (!ok) { it = byteJoins_.erase(it); continue; }
        byteJoinSkip_.insert(it->second.nodes.begin(), it->second.nodes.end());
        ++it;
    }

    // Track parameter names for all functions (needed for SAC call site naming)
    std::vector<std::string> paramNames;
    for (size_t i = 0; i < fn.paramNames.size(); i++) {
        if (i < fn.paramNames.size() && !fn.paramNames[i].empty()) {
            paramNames.push_back(fn.paramNames[i]);
        } else {
            paramNames.push_back(std::to_string(i));
        }
    }
    functionParameterNames_[fn.name] = paramNames;
    functionParameterTypes_[fn.name] = fn.paramTypes;

    if (useSAC) {
        // (same-file callers of a function in this set store its arguments
        // and skip its thunk: not for a variadic one, whose ... is pushed)
        if (!fn.isVariadic) sacFunctions_.insert(fn.name);  // Track for .global __ar symbol emission

        // Detect zero-alloc leaves (leaf + no locals + all constant params)
        detectZeroAllocLeaves(fn);

        // Emit SAC storage declarations in BSS segment (not in code stream)
        // These are zero-initialized at startup by the CRT, so they don't need
        // to occupy space in the binary.
        // Skip for zero-alloc leaves (no storage needed at all)
        bool isZeroAlloc = zeroAllocLeaves_.count(fn.name) > 0;
        if (!isZeroAlloc) {
            emitComment("SAC storage: " + std::to_string(frameSize_) + " bytes (in BSS)");
        } else {
            emitComment("SAC zero-alloc leaf: no storage overhead");
        }

        // Skip storage emission entirely for zero-alloc leaves
        if (!isZeroAlloc) {

        // Switch to BSS segment for SAC storage
        emit(".segment \"bss\"");

        // <fn>__ar marks the start of the block and cc45.sacsize.<fn> its
        // size: ln45 moves the block into the SAC overlay area, where
        // functions that are never active at the same time share storage.
        // A block holding aligned variables keeps its place (no size marker).
        // ln45 sees only offsets: a pointer just past the variable before
        // the block, or just past the block's last variable, must not be an
        // offset of another piece. A pad byte before the block (unless a
        // block's trailing pad is already there) and one ending the block
        // keep them apart.
        int sacBlockSize = 0;
        bool sacBlockAligned = false;
        bool sacMovable = false;
        {
            bool hasStorage = false;
            for (size_t i = 0; i < fn.paramTypes.size(); i++)
                if (!(sacConstParams_.count(fn.name) && sacConstParams_[fn.name].count((int)i) &&
                      sacConstParams_[fn.name].at((int)i).isConstant))
                    hasStorage = true;
            bool aligned = false;
            for (const auto& [vregId, offset] : vregOffset_) {
                (void)offset;
                if (alloc_.getAlloc(vregId).loc != VRegAllocator::IN_FRAME) continue;
                hasStorage = true;
                auto ait = fn.vregAlign.find(vregId);
                if (ait != fn.vregAlign.end() && ait->second > 1) aligned = true;
            }
            if (fn.isVariadic) hasStorage = true;   // (<fn>__va)
            sacMovable = hasStorage && !aligned;
        }
        // a section of its own, unless it holds aligned locals (their
        // alignment inside the block is not recorded): then residual
        if (functionSections_ && relocMode_) {
            if (sacMovable) emitSplit(fn.name + "__ar");
            else emit(".nosplit");
        }
        if (sacMovable && !bssBoundaryClean_) emit(".res 1", "SAC block lead pad");
        emit((fn.isWeak ? ".weak " : ".global ") + fn.name + "__ar");
        emitLabel(fn.name + "__ar");

        // Emit storage for parameters
        // Use parameter names for documentation, fall back to indices if unavailable
        // Make symbols globally visible so caller and function can both reference them
        for (size_t i = 0; i < fn.paramTypes.size(); i++) {
            int ps = ir::typeSize(fn.paramTypes[i]);
            if (ps < 2) ps = 2;  // Minimum 2 bytes
            std::string pName = (i < fn.paramNames.size() && !fn.paramNames[i].empty())
                ? fn.paramNames[i] : std::to_string(i);
            std::string paramSymbol = fn.name + "__param_" + pName;
            emit((fn.isWeak ? ".weak " : ".global ") + paramSymbol);

            // Check if this is a constant parameter
            int64_t constValue = 0;
            bool isConstParam = false;
            if (sacConstParams_.count(fn.name)) {
                const auto& params = sacConstParams_[fn.name];
                if (params.count((int)i) && params.at((int)i).isConstant) {
                    isConstParam = true;
                    constValue = params.at((int)i).value;
                }
            }

            // In BSS, use .res to reserve space (no initial value in binary)
            // Constant parameters will be initialized by CRT or first call
            if (isConstParam) {
                // Constant params go in data segment (initialized)
                emit(".segment \"data\"");
                emitSplit(paramSymbol);
                if (ps == 2) {
                    emit(paramSymbol + ": .word " + std::to_string((int)(constValue & 0xFFFF)));
                } else if (ps == 4) {
                    emit(paramSymbol + ": .long " + std::to_string((int)(constValue & 0xFFFFFFFF)));
                } else {
                    emit(paramSymbol + ": .word " + std::to_string((int)(constValue & 0xFFFF)));
                    for (int j = 2; j < ps; j += 2) {
                        emit(".word 0");
                    }
                }
                emit(".segment \"bss\"");
            } else {
                emitLabel(paramSymbol);
                emit(".res " + std::to_string(ps));
                sacBlockSize += ps;
            }
        }

        // Emit storage for locals
        for (const auto& [vregId, offset] : vregOffset_) {
            auto alloc = alloc_.getAlloc(vregId);
            if (alloc.loc == VRegAllocator::IN_FRAME) {
                std::string localSymbol = fn.name + "__local_" + std::to_string(vregId);
                // Use vregSizes for arrays (which store the full buffer size),
                // fall back to type size for scalars
                int varSize = 0;
                auto vsit = fn.vregSizes.find(vregId);
                if (vsit != fn.vregSizes.end()) {
                    varSize = vsit->second;  // array: full buffer size
                } else {
                    ir::Type varType = vregType_.count(vregId) ? vregType_[vregId] : ir::Type::I16;
                    varSize = ir::typeSize(varType);
                }
                if (varSize < 2) varSize = 2;
                auto ait = fn.vregAlign.find(vregId);
                if (ait != fn.vregAlign.end() && ait->second > 1) {
                    emit(".align " + std::to_string(ait->second));
                    bssAlign_ = std::max(bssAlign_, ait->second);
                    sacBlockAligned = true;
                }
                emitLabel(localSymbol);
                emit(".res " + std::to_string(varSize));
                sacBlockSize += varSize;
            }
        }
        if (fn.isVariadic) {
            emitLabel(fn.name + "__va");
            emit(".res 2", "address of the first ... argument (the entry thunk)");
            sacBlockSize += 2;
        }
        if (sacMovable && !sacBlockAligned && sacBlockSize > 0) {
            emit(".res 1", "SAC block trailing pad");
            sacBlockSize += 1;
            emit(std::string(fn.isWeak ? ".weak " : ".global ") + "cc45.sacsize." + fn.name);
            emit("cc45.sacsize." + fn.name + " = " + std::to_string(sacBlockSize));
            bssBoundaryClean_ = true;
        }

        // Switch back to the function's text segment
        emit(".segment \"" + textSeg_ + "\"");

        }  // Close if (!isZeroAlloc)
    }

    // An aligned function starts on its boundary (ln45 keeps the object's
    // code at the same offset modulo cc45.align.text)
    if (fn.align > 1) {
        emit(".align " + std::to_string(fn.align));
        textAlign_ = std::max(textAlign_, fn.align);
    }

    // proc directive with parameter specs
    // For regparm, first param is in A/AX — not on stack, not in proc line
    std::string procLine = "proc " + fn.name;
    for (size_t i = 0; i < fn.paramTypes.size(); i++) {
        if (fn.isRegparm && i == 0) continue; // first param in register
        procLine += ", ";
        int pSize = ir::typeSize(fn.paramTypes[i]);
        if (pSize == 1) procLine += "B";
        else if (pSize == 4) procLine += "D";
        else if (pSize == 5) procLine += "F";
        else procLine += "W";
        if (i < fn.paramNames.size() && !fn.paramNames[i].empty()) {
            procLine += "#@_p_" + fn.paramNames[i];
        } else {
            procLine += "#@_p_" + std::to_string(i);
        }
    }
    // A function that calls through a pointer of unknown type (or has
    // inline asm, which may): ln45 assumes it can call any function whose
    // address is used (cc45.icall.<f>). Through a prototyped pointer only
    // those with its parameter slots (cc45.isig.<f>.<sig>), and every
    // function says its own (cc45.fsig.<f>.<sig>)
    if (relocMode) {
        bool icall = false;
        std::set<std::string> sigs;
        for (const auto& blk : fn.blocks)
            for (const auto& in : blk.insts) {
                if (in.op == ir::Op::ASM_INLINE || (in.op == ir::Op::CALL_INDIRECT && in.callSig.empty())) icall = true;
                else if (in.op == ir::Op::CALL_INDIRECT) sigs.insert(in.callSig);
            }
        if (icall) {
            emit(".global cc45.icall." + fn.name);
            emit("cc45.icall." + fn.name + " = 1");
        }
        for (const auto& sig : sigs) {
            emit(".weak cc45.isig." + fn.name + "." + sig);
            emit("cc45.isig." + fn.name + "." + sig + " = 1");
        }
        if (!fn.ptrSig.empty()) {
            emit(".weak cc45.fsig." + fn.name + "." + fn.ptrSig);
            emit("cc45.fsig." + fn.name + "." + fn.ptrSig + " = 1");
        }
    }
    if (zarSize_ > 0) {
        // the base-page block: ln45 defines <fn>__zar and places it
        emit(".extern " + fn.name + "__zar");
        emit(std::string(fn.isWeak ? ".weak " : ".global ") + "cc45.zarsize." + fn.name);
        emit("cc45.zarsize." + fn.name + " = " + std::to_string(zarSize_));
    }
    emit(procLine);
    // #pragma cc45 optimize(off): ca45 leaves the body as it is
    if (fn.noOptimize) emit(".noopt_start");
    if (zarSize_ > 0) emit(".zar $" + hex8((uint8_t)ZAR_START) + ", " + std::to_string(zarSize_));
    for (const auto& r : fn.asmRefs) emit(".ref " + r);

    // __regparm: the first argument arrives in A (char) or A:X. Before any
    // other code (a prologue or the return-address capture use A/X), put it
    // where the normal convention would have it: the first ZP parameter slot
    // ($10; the caller leaves it free), its SAC storage (stack convention:
    // _f__sac, which same-file callers use after storing it there, follows),
    // or $10 for a stack frame to pick up after its prologue.
    const bool regparmArg = fn.isRegparm && !fn.paramTypes.empty();
    const bool regparmWide = regparmArg && ir::typeSize(fn.paramTypes[0]) >= 2;
    const std::string regparmZp = "$" + hex8((uint8_t)(zeroPageStart_ + 8));
    const std::string regparmZp1 = "$" + hex8((uint8_t)(zeroPageStart_ + 9));
    if (regparmArg) {
        if (!fnZp && currentFunctionUseSAC_) {
            if (!isZeroAllocLeaf(fn.name, fn)) {
                std::string p0 = fn.name + "__param_" +
                    ((!fn.paramNames.empty() && !fn.paramNames[0].empty()) ? fn.paramNames[0] : std::string("0"));
                emit("sta " + p0);
                if (regparmWide) emit("stx " + p0 + "+1");
            }
        } else {
            emit("sta " + regparmZp);
            if (regparmWide) emit("stx " + regparmZp1);
        }
    }

    // __builtin_return_address(0): copy the return address (the stacked JSR
    // address + 1) to __zp_scratch3 before anything is pushed
    auto emitRetAddrCapture = [&]() {
        if (fn.retAddrSlot < 0) return;
        emit("tsx"); emit("tsy");
        emit("stx __zp_scratch4"); emit("sty __zp_scratch4+1");
        emit("ldy #1"); emit("lda (__zp_scratch4),y");
        emit("clc"); emit("adc #1"); emit("sta __zp_scratch3");
        emit("iny"); emit("lda (__zp_scratch4),y");
        emit("adc #0"); emit("sta __zp_scratch3+1");
    };
    if (!currentFunctionUseSAC_) emitRetAddrCapture();

    // Mark SAC functions for assembler
    // Phase 51: Skip for zero-alloc leaves (all parameters constant)
    if (currentFunctionUseSAC_) {
        bool isZeroAlloc = isZeroAllocLeaf(fn.name, fn);
        if (!isZeroAlloc) {
            emit(".sac");
        } else {
            emitComment("Phase 51: zero-alloc leaf (all parameters constant)");
        }
    }

    bool sacLabelEmitted = false;
    // SAC thunk prologue: load params from stack into SAC param symbols.
    // This allows cross-unit callers (which push to stack) to call SAC functions.
    // Same-unit SAC callers call _func__sac directly (skipping the thunk).
    if (currentFunctionUseSAC_ && (!fn.paramTypes.empty() || fn.isVariadic) &&
        !(fn.isRegparm && fn.paramTypes.size() == 1 && !fnZp)) {
        bool isZeroAlloc = isZeroAllocLeaf(fn.name, fn);
        if (!isZeroAlloc) {
            LOG_INFO("cc45", "SAC thunk generated for " << fn.name << " (" << fn.paramTypes.size() << " params)");
            if (fnZp) {
                // ZP convention: copy the parameter block into SAC storage
                emitComment("SAC thunk: load ZP params into SAC storage");
                // __zp_args (stdlib_asm/zp_args.s45) copies them from a list
                // of the parameters' sizes and homes, 3 bytes each (1 for one
                // not copied) after a jsr: when that is shorter than the
                // copies inline, lda/sta per byte (-S output, assembled
                // without the library, copies them inline)
                int inlineBytes = 0, listBytes = 4, pending = 0;   // (jsr, the 0)
                std::vector<std::string> list;
                for (size_t i = 0; i < fn.paramTypes.size(); i++) {
                    std::string pName = (i < fn.paramNames.size() && !fn.paramNames[i].empty())
                        ? fn.paramNames[i] : std::to_string(i);
                    int pSize = ir::typeSize(fn.paramTypes[i]);
                    if (zpParams_.count((uint32_t)i)) { pending++; continue; }
                    auto zv = zarVregs_.find((uint32_t)i);
                    inlineBytes += pSize * (zv != zarVregs_.end() ? 4 : 5);
                    // (the skipped parameters before this one)
                    for (size_t k = i - pending; k < i; k++)
                        list.push_back(".byte " + std::to_string(0x80 | ir::typeSize(fn.paramTypes[k])));
                    listBytes += pending + 3;
                    pending = 0;
                    list.push_back(".byte " + std::to_string(pSize));
                    list.push_back(".word " + (zv != zarVregs_.end()
                        ? fn.name + "__zar+" + std::to_string(zv->second - ZAR_START)
                        : fn.name + "__param_" + pName));
                }
                // (-fno-zp-args, -O3 / -Ospeed: the copies inline, #359)
                bool lists = relocMode_ && listBytes < inlineBytes && fn.argLists;
                if (lists) {
                    emit(".extern __zp_args");
                    emit("jsr __zp_args");
                    for (const auto& l : list) emit(l);
                    emit(".byte 0");
                }
                int zpOff = (int)zeroPageStart_ + 8;
                for (size_t i = 0; !lists && i < fn.paramTypes.size(); i++) {
                    std::string pName = (i < fn.paramNames.size() && !fn.paramNames[i].empty())
                        ? fn.paramNames[i] : std::to_string(i);
                    std::string sym = fn.name + "__param_" + pName;
                    int pSize = ir::typeSize(fn.paramTypes[i]);
                    // (one used in the block is not copied; one in the
                    // function's base-page block is copied there)
                    auto zv = zarVregs_.find((uint32_t)i);
                    for (int b = 0; b < pSize && !zpParams_.count((uint32_t)i); b++) {
                        emit("lda $" + hex8((uint8_t)(zpOff + b)));
                        if (zv != zarVregs_.end()) { emit("sta $" + hex8((uint8_t)(zv->second + b))); continue; }
                        emit("sta " + sym + (b ? "+" + std::to_string(b) : std::string()));
                    }
                    zpOff += (pSize < 2) ? 2 : pSize;
                }
            } else {
            emitComment("SAC thunk: load stack params into SAC storage");
            // Read through a pointer to SP so the stack may be on any page
            bool anyParamBytes = fn.isVariadic;
            for (auto t : fn.paramTypes) if (ir::typeSize(t) > 0) anyParamBytes = true;
            bool lists = anyParamBytes && !fn.isVariadic && relocMode_ && fn.argLists;   // (#359)
            if (lists) {
                // __sac_args (stdlib_asm/sac_args.s45) copies them, from a
                // list of the parameters' sizes and storage: 3 bytes each
                // instead of 7 per byte (-S output, assembled without the
                // library, copies them inline)
                emit(".extern __sac_args");
                emit("jsr __sac_args");
                for (size_t i = 0; i < fn.paramTypes.size(); i++) {
                    if (fn.isRegparm && i == 0) continue;  // in A/A:X, stored above
                    std::string pName = (i < fn.paramNames.size() && !fn.paramNames[i].empty())
                        ? fn.paramNames[i] : std::to_string(i);
                    int pSize = ir::typeSize(fn.paramTypes[i]);
                    if (pSize <= 0) continue;
                    emit(".byte " + std::to_string(pSize));
                    emit(".word " + fn.name + "__param_" + pName);
                }
                emit(".byte 0");
            } else if (anyParamBytes) {
                emit("tsx");
                emit("tsy");
                emit("stx $08");
                emit("sty $09");
            }
            int stackOff = 2; // past return address
            for (size_t i = 0; !lists && i < fn.paramTypes.size(); i++) {
                if (fn.isRegparm && i == 0) continue;  // in A/A:X, stored above
                std::string pName = (i < fn.paramNames.size() && !fn.paramNames[i].empty())
                    ? fn.paramNames[i] : std::to_string(i);
                std::string sym = fn.name + "__param_" + pName;
                int pSize = ir::typeSize(fn.paramTypes[i]);
                for (int b = 0; b < pSize; b++) {
                    emit("ldy #" + std::to_string(1 + stackOff + b));
                    emit("lda ($08),y");
                    emit("sta " + sym + (b ? "+" + std::to_string(b) : std::string()));
                }
                stackOff += (pSize < 2) ? 2 : pSize; // C promotes char to 16-bit on stack
            }
            if (fn.isVariadic) {
                // the first ... argument follows the named ones: va_start's start
                emit("lda $08", "va_start: SP + 1 + " + std::to_string(stackOff));
                emit("clc");
                emit("adc #" + std::to_string(1 + stackOff));
                emit("sta " + fn.name + "__va");
                emit("lda $09");
                emit("adc #0");
                emit("sta " + fn.name + "__va+1");
            }
            }
            // Emit __sac label for direct SAC callers to skip thunk
            emit((fn.isWeak ? ".weak " : ".global ") + fn.name + "__sac");
            emitLabel(fn.name + "__sac");
            sacLabelEmitted = true;
        }
    }
    // Same-unit SAC callers always call _func__sac: without a parameter thunk
    // (no params, regparm, zero-alloc leaf) it is the function entry itself
    if (currentFunctionUseSAC_ && !sacLabelEmitted) {
        emit((fn.isWeak ? ".weak " : ".global ") + fn.name + "__sac");
        emitLabel(fn.name + "__sac");
    }
    if (currentFunctionUseSAC_) emitRetAddrCapture();

    // Reset source location tracking for this function
    lastLocLine_ = -1;
    lastLocFile_.clear();

    // __interrupt: save all registers at entry
    if (fn.isInterrupt) {
        emit("pha"); emit("phx"); emit("phy"); emit("phz");
        // The ROM IRQ entry sets the base page to 0; switch to the interrupt
        // handlers' own page (saving the interrupted B) so the zero page of
        // the interrupted code (parameters, temporaries, FP) is untouched
        emit("tba"); emit("pha");
        emit("inc __isr_depth");  // __bp_prog returns to this page after ROM calls
        emit("lda #>__zp_page_isr"); emit("tab");
        // (the library's bytes the interrupted code may be using: __jsr's,
        // __rom_call's; crt0's empty defaults when not linked)
        if (relocMode) { emit("jsr __jsr_isr_save"); emit("jsr __rc_isr_save"); }
    }

    // Frame pointer variable ("cc45.fp" can't clash with a C global named fp)
    emit(".var cc45.fp = 0");
    // A caller may test the result by the flags its rts leaves: a char's
    // (#348), or what inline asm sets; ca45 takes any other cc45 function's
    // rts as reading no N / Z (#351)
    {
        bool asmBody = false;   // (inline asm, or __cpu / __flags written: left for the caller perhaps)
        for (const auto& blk : fn.blocks)
            for (const auto& in : blk.insts)
                if (in.op == ir::Op::ASM_INLINE || in.op == ir::Op::CPU_REG_WRITE || in.op == ir::Op::CPU_FLAG_WRITE)
                    asmBody = true;
        if (fn.returnType == ir::Type::I8 || asmBody || fn.isInterrupt) emit(".var cc45.retnz = 1");
        // (and X: any result, a char's too: a caller without its prototype,
        // -fimplicit-function-declaration, takes it for an int; inline asm,
        // an interrupt's)
        if (fn.returnType != ir::Type::VOID || asmBody || fn.isInterrupt)
            emit(".var cc45.retx = 1");
    }

    // Emit .loc for function declaration line (before prologue and param loading,
    // so objdump attributes setup code to the function signature, not the first statement)
    if (fn.declLine > 0 && !sourceFile_.empty()) {
        // Use line mapping if available to find correct source file for this line
        std::string locFile = sourceFile_;
        int displayLine = fn.declLine;

        if (!lineToFileMap_.empty()) {
            // Find the context that applies to this line
            for (auto it = lineToFileMap_.rbegin(); it != lineToFileMap_.rend(); ++it) {
                if (it->first <= fn.declLine) {
                    const auto& context = it->second;
                    locFile = context.first;
                    // Convert absolute line to file-relative line
                    displayLine = context.second + (fn.declLine - it->first);
                    break;
                }
            }
        }

        emit(".loc \"" + locFile + "\", " + std::to_string(displayLine));
        lastLocFile_ = locFile;
        lastLocLine_ = displayLine;
    }

    // Allocate frame only for frame-allocated vRegs
    int localFrameSize = frameSize_;
    // A runtime alloca needs FP (set up only with a frame) to reset SP on return
    if (usesAlloca && localFrameSize == 0) localFrameSize = 1;
    // Its frame pointer is the static chain of its nested functions (GET_FP;
    // a non-local goto back into it restores SP from it): it needs one even
    // when no variable lives in the frame
    if (localFrameSize == 0)
        for (const auto& blk : fn.blocks)
            for (const auto& in : blk.insts)
                if (in.op == ir::Op::GET_FP) localFrameSize = 1;
    localFrameSize_ = localFrameSize;  // save for RET cleanup

    // SAC storage is now emitted BEFORE proc directive for correct memory placement

    // Frame-pointer functions save the caller's FP ($FD/$FE) below their frame;
    // without this a caller that uses FP-relative locals reads the callee's
    // frame after any call (and recursion fails).
    savedCallerFP_ = !useSAC && localFrameSize > 0;
    // Push $FD then $FE through A: "phw $00FD" is absolute and would read the
    // ROM's page 0 rather than the program's relocated base page.
    if (savedCallerFP_) {
        emit("lda $FD");
        emit("pha");
        emit("lda $FE");
        emit("pha");
    }

    // Push stack frame only for non-SAC functions
    // SAC functions use static AR buffers instead of stack frames
    if (!useSAC) {
        // (frames over 255 bytes are addressed through $FB/$FC = FP + 256*n:
        // M65Emitter::framePtrFor; test_frame_large_xemu.c)
        if (localFrameSize > 0) {
            emitComment("frame: " + std::to_string(localFrameSize) + " bytes (frame-allocated vRegs only)");
            // Push frame in 2-byte chunks; if odd size, push final byte separately
            // (a large frame in a loop: Z is free at the entry)
            int words = localFrameSize / 2;
            if (words >= 4 && localFrameSize <= 255) {
                // SP -= size when that does not cross a page (txs: the low
                // byte only, right in either stack mode); else the pushes.
                // C locals start undefined: the frame is not cleared
                std::string slow = "@__fslow_" + std::to_string(labelCounter_++);
                std::string done = "@__fdone_" + std::to_string(labelCounter_++);
                std::string lbl = "@__fpush_" + std::to_string(labelCounter_++);
                emit("tsx", "frame: SP - " + std::to_string(localFrameSize));
                emit("txa");
                emit("sec");
                emit("sbc #" + std::to_string(localFrameSize));
                emit("bcc " + slow);
                emit("tax");
                emit("txs");
                emit("bra " + done);
                emitLabel(slow);
                emit("ldz #" + std::to_string(words));
                emitLabel(lbl);
                emit("phw #0");
                emit("dez");
                emit("bne " + lbl);
                if (localFrameSize % 2 == 1) emit("pha");
                emitLabel(done);
                words = 0;
            } else if (words >= 4) {
                for (int done = 0; done < words; ) {
                    int n = std::min(words - done, 255);
                    std::string lbl = "@__fpush_" + std::to_string(labelCounter_++);
                    emit("ldz #" + std::to_string(n));
                    emitLabel(lbl);
                    emit("phw #0");
                    emit("dez");
                    emit("bne " + lbl);
                    done += n;
                }
            } else {
                for (int i = 0; i < words; i++) emit("phw #0");
            }
            if (localFrameSize % 2 == 1 && !(localFrameSize / 2 >= 4 && localFrameSize <= 255)) {
                emit("lda #0");
                emit("pha");  // Push final odd byte
            }
        }
    }

    // Set up ZP frame pointer for local variable access (stack convention only)
    // SAC functions use direct absolute addressing to inline storage, no FP needed
    useStackParams_ = !fnZp;  // Stack params for all non-ZP functions

    // PHASE 80: Optimize frame pointer initialization
    // Detect if function actually uses frame-pointer-relative addressing
    // to avoid unnecessary FP setup (12 bytes per function)
    bool needsFP = false;
    if (!useSAC && localFrameSize > 0) {
        // Only functions with local variables that actually use them need FP
        // For now, conservatively assume all stack-convention functions with locals need FP
        // Future: analyze IR to detect actual .fp references
        needsFP = true;
    }

    if (!useSAC && needsFP) {
        // Stack convention only: Calculate frame pointer from SP: FP = __sp_base + SPL + 1
        emit("tsy");           // SPH → Y
        emit("tsx");           // SPL → X
        emit("inx");           // X = SPL + 1 (sets carry if wrapped)
        std::string noCarryLabel = "@__fp_no_carry_" + std::to_string(labelCounter_++);
        emit("bne " + noCarryLabel);  // Skip if X != 0 (no carry)
        emit("iny");           // If carry, increment Y (SPH)
        emitLabel(noCarryLabel);
        emit("stx $FD");       // Store low byte
        emit("sty $FE");       // Store high byte
        emit(".frameptr_zp $FD");
    }

    // Compute and cache frame address in dedicated ZP slot if function uses frame-relative access
    // (NOT needed for SAC functions, which use direct absolute addressing)
    // (frameAddrZPIndex_ names the frame pointer $FD/$FE: nothing to set up)

    // Emit .local for frame-allocated vRegs only (skipped in SAC mode, which uses AR-relative)
    if (!currentFunctionUseSAC_) {
        for (const auto& [vregId, offset] : vregOffset_) {
            auto alloc = alloc_.getAlloc(vregId);
            if (alloc.loc == VRegAllocator::IN_FRAME) {
                // Nested functions reach the variable at this offset from the
                // frame pointer (their static link)
                frameOffsetsByFn_[fn.name][vregId] = offset;
                emit(".local __vr" + std::to_string(vregId) + " = " + std::to_string(offset));
            }
        }
    }

    // Identify which localNames are parameters
    std::set<std::string> paramNamesSet(fn.paramNames.begin(), fn.paramNames.end());

    // Populate parameterVregs_ for SAC addressing logic
    // IMPORTANT: This should be EMPTY for SAC functions!
    // Vreg allocation: vregs 0..N-1 are PARAMETER STORAGE (should use AR-relative in SAC)
    // Named parameters in localNames are FORMAL PARAMETERS (use FP-relative for stack access)
    parameterVregs_.clear();
    for (const auto& [name, vregId] : fn.localNames) {
        if (paramNamesSet.count(name) > 0) {
            // These are named parameters - use FP-relative (they're passed on stack)
            // But only for non-SAC or when parameters are in localNames separately
            parameterVregs_.insert(vregId);
        }
    }

    // Emit .local/.var aliases for named variables (to support inline asm)
    for (const auto& [name, vregId] : fn.localNames) {
        bool isParam = paramNamesSet.count(name) > 0;
        if (isParam) continue; // Parameters are handled by .var overrides later

        std::string prefix = "@_l_";

        if (vregOffset_.count(vregId)) {
            // It's a local/param on the stack
            emit(".local " + prefix + name + " = " + std::to_string(vregOffset_.at(vregId)));
        } else {
            // It might be in ZP?
            auto alloc = alloc_.getAlloc(vregId);
            if (alloc.loc == VRegAllocator::IN_ZP) {
                std::stringstream ss;
                ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)alloc.offset;
                emit(".var " + prefix + name + " = " + ss.str());
            }
        }
    }

    // Debug information (#202): each local and parameter, where its value is
    // (not the compiler's own: __static_link, __zp_<param>)
    for (const auto& name : fn.localNamesOrder) {
        auto it = fn.localNames.find(name);
        if (it == fn.localNames.end() || paramNamesSet.count(name) || name.rfind("__", 0) == 0) continue;
        auto ty = fn.debugTypes.find(name);
        emitDebugVar("local", fn.name, name, ty != fn.debugTypes.end() ? ty->second : "u16",
                     debugLocation(fn, it->second));
    }
    for (size_t i = 0; i < fn.paramTypes.size(); i++) {
        if (i >= fn.paramNames.size() || fn.paramNames[i].empty()) continue;
        // (a pointer parameter the optimiser moved to a ZP register: there)
        auto it = fn.localNames.find("__zp_" + fn.paramNames[i]);
        if (it == fn.localNames.end()) it = fn.localNames.find(fn.paramNames[i]);
        if (it == fn.localNames.end()) continue;
        auto ty = fn.debugTypes.find(fn.paramNames[i]);
        emitDebugVar("param", fn.name, fn.paramNames[i], ty != fn.debugTypes.end() ? ty->second : "u16",
                     debugLocation(fn, it->second));
    }

    // Param .var overrides
    // For SAC functions, parameters are still on the stack (accessed via FP)
    // Only local vregs are allocated to the AR buffer
    // In SAC mode, frame is NOT pushed, so parameter offsets are 2 bytes less
    bool useStackParams = !fnZp;
    if (useStackParams) {
        // Stack convention (or variadic in zpCall mode): args pushed right-to-left,
        // first param is closest to SP (smallest offset past frame + return address).
        // In SAC mode, no frame is pushed, so offset is 2 (just return address)
        // In non-SAC mode, offset is 2 + frame size
        int pOff = useSAC ? 2 : (localFrameSize + 2 + (savedCallerFP_ ? 2 : 0));
        for (size_t i = 0; i < fn.paramTypes.size(); i++) {
            if (fn.isRegparm && i == 0) continue;  // in A/A:X, not on the stack
            int ps = ir::typeSize(fn.paramTypes[i]);
            if (ps < 2) ps = 2;
            std::string pName = (i < fn.paramNames.size() && !fn.paramNames[i].empty())
                ? fn.paramNames[i] : std::to_string(i);
            emit(".var @_p_" + pName + " = " + std::to_string(pOff));
            pOff += ps;
        }
    } else {
        // ZP call convention: params in ZP block past scratch area
        auto hex8 = [](uint32_t val) {
            std::stringstream ss;
            ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (val & 0xFF);
            return ss.str();
        };
        int zpOff = (int)zeroPageStart_ + 8;
        for (size_t i = 0; i < fn.paramTypes.size(); i++) {
            int ps = ir::typeSize(fn.paramTypes[i]);
            if (ps < 2) ps = 2;
            std::string pName = (i < fn.paramNames.size() && !fn.paramNames[i].empty())
                ? fn.paramNames[i] : std::to_string(i);
            emit(".var @_p_" + pName + " = " + hex8(zpOff));
            zpOff += ps;
        }
    }

    emitBlank();

    // Load parameters from stack into storage
    // For SAC: load into inline storage using direct addressing
    // For stack convention: load into vregs via FP-relative addressing
    if (useStackParams) {
        if (useSAC) {
            // SAC: Parameters already in inline storage (placed there by caller)
            // No parameter loading needed - caller bypassed stack entirely
            // Do NOT emit "load from stack" code - it would overwrite caller's storage!
        } else {
            // Stack convention: params are on the stack, copy to vregs
            // (a __regparm first parameter last: loading the others goes
            // through A/X, where its value may be kept)
            const size_t np = fn.paramTypes.size();
            for (size_t k = 0; k < np; k++) {
                size_t i = fn.isRegparm ? (k + 1) % np : k;
                uint32_t vid = (uint32_t)i;
                if (fn.isNested && i == fn.paramTypes.size() - 1 && fn.staticLinkVreg != -1) {
                    // vid = (uint32_t)fn.staticLinkVreg; // Wait! No more hidden param!
                }

                if (fn.isRegparm && i == 0) {
                    // The register argument, saved in $10 at the entry
                    emit("lda " + regparmZp);
                    if (regparmWide) emit("ldx " + regparmZp1);
                    storeVreg(vid);
                    continue;
                }
                std::string pName = (i < fn.paramNames.size() && !fn.paramNames[i].empty())
                    ? fn.paramNames[i] : std::to_string(i);
                if (fn.paramTypes[i] == ir::Type::F32) {
                    // 5-byte float: copy from the caller's arguments into the vreg
                    for (int bi = 0; bi < 5; bi++) {
                        emit("lda.param @_p_" + pName + "+" + std::to_string(bi));
                        emitF32Byte("sta", vid, bi);
                    }
                } else if (fn.paramTypes[i] == ir::Type::I32) {
                    emit("ldaxyz.param @_p_" + pName);
                    storeVreg(vid);
                } else {
                    emit("ldax.param @_p_" + pName);
                    storeVreg(vid);
                }
            }
        }
    } else if (fnZp && !useSAC) {
        // ZP call convention (non-SAC): params arrive in the ZP block; copy them
        // to their vregs (calls made by this function reuse the block)
        for (size_t i = 0; i < fn.paramTypes.size(); i++) {
            uint32_t vid = (uint32_t)i;
            if (zpParams_.count(vid)) continue;   // used in the block
            // (a __regparm first argument was stored into its slot at the entry)
            std::string pName = (i < fn.paramNames.size() && !fn.paramNames[i].empty())
                ? fn.paramNames[i] : std::to_string(i);
            std::string p = "@_p_" + pName;
            if (fn.paramTypes[i] == ir::Type::F32) {
                for (int bi = 0; bi < 5; bi++) {
                    emit("lda " + p + "+" + std::to_string(bi));
                    emitF32Byte("sta", vid, bi);
                }
                continue;
            }
            emit("lda " + p);
            if (fn.paramTypes[i] != ir::Type::I8) emit("ldx " + p + "+1");
            if (fn.paramTypes[i] == ir::Type::I32) {
                emit("ldy " + p + "+2");
                emit("pha"); emit("lda " + p + "+3"); emit("taz"); emit("pla");  // no LDZ base-page mode
            }
            storeVreg(vid);
        }
    }

    // Emit SAC debug entry marker (if SAC mode and debug enabled)
    if (currentFunctionUseSAC_ && sacDebugMode_) {
        emitSACDebugEnter(fn.name, localFrameSize);
    }

    // Emit blocks (with instruction index tracking for allocator queries)
    currentInstIdx_ = 0;
    currentFn_ = &fn;
    loopBlocks_ = ir::loopBlocks(fn);
    // how often each label is a jump target (inline asm may name any: none
    // counts as single then)
    labelRefs_.clear();
    emittedLabelRefs_.clear();
    bool anyAsm = false;
    for (const auto& b : fn.blocks)
        for (const auto& in : b.insts) {
            if (in.op == ir::Op::ASM_INLINE) anyAsm = true;
            for (const ir::Operand* o : {&in.dest, &in.src1, &in.src2})
                if (o->kind == ir::OperandKind::LABEL) labelRefs_[o->name]++;
            for (const auto& o : in.args)
                if (o.kind == ir::OperandKind::LABEL) labelRefs_[o.name]++;
            for (const auto& c : in.switchCases) labelRefs_[c.second] += 2;
            for (const auto& ph : in.phiIncoming) labelRefs_[ph.second] += 2;
        }
    // Common return point — clean up frame, then endproc emits rts. After
    // the blocks, or (cold blocks at the end) before the cold ones, so the
    // expected path falls into it
    auto emitEpilogue = [&]() {
        emitLabel("@__return");

        // Emit SAC debug exit marker (if SAC mode and debug enabled)
        if (currentFunctionUseSAC_ && sacDebugMode_) {
            emitSACDebugExit(fn.name);
        }

        // SAC functions don't have a stack frame to clean up
        if (!useSAC && localFrameSize > 0) {
            if (savedCallerFP_) {
                // Drop the frame and restore the caller's FP using only A (saved in
                // scratch), so X/Y/Z of the return value are preserved
                emit("sta __zp_scratch");
                if (usesAlloca || localFrameSize >= 8) {
                    // alloca moved SP, or a large frame: set SP to just below the
                    // saved FP (FP + size - 1: the frame starts at FP) instead of
                    // pulling the frame byte by byte. SP rises, so SPL is written
                    // first (an interrupt between the writes pushes below the
                    // frame). X/Y of the result are kept.
                    int off = localFrameSize - 1;
                    emit("stx __zp_scratch+1");
                    emit("sty __zp_scratch+2");
                    emit("lda $FD");
                    emit("clc");
                    emit("adc #" + std::to_string(off & 0xFF));
                    emit("tax");
                    emit("lda $FE");
                    emit("adc #" + std::to_string((off >> 8) & 0xFF));
                    emit("tay");
                    emit("txs");
                    emit("tys");
                    emit("ldx __zp_scratch+1");
                    emit("ldy __zp_scratch+2");
                } else {
                    for (int i = 0; i < localFrameSize; i++) emit("pla");
                }
                emit("pla");
                emit("sta $FE");   // prologue pushed $FD then $FE
                emit("pla");
                emit("sta $FD");
                emit("lda __zp_scratch");
            } else {
                emitStackCleanup(localFrameSize);
            }
        }

        // __interrupt: restore registers and return with RTI
        if (fn.isInterrupt) {
            if (relocMode_) { emit("jsr __rc_isr_restore"); emit("jsr __jsr_isr_restore"); }
            emit("dec __isr_depth");
            emit("pla"); emit("tab");
            emit("plz"); emit("ply"); emit("plx"); emit("pla");
            emit("rti");
        }

        // Non-interrupt functions return with RTS (SAC or stack-based)
        // (Plain rts without plz for SAC; with plz cleanup for stack-based functions above)
        emit("rts");
    };
    size_t coldStart = fn.blocks.size();
    if (fn.coldAtEnd)
        while (coldStart > 1 && fn.blocks[coldStart - 1].cold) coldStart--;
    for (size_t bi = 0; bi < fn.blocks.size(); bi++) {
        const auto& block = fn.blocks[bi];
        currentBlockIdx_ = bi;
        // Track the next block's label for no-op branch elimination
        nextBlockLabel_ = (bi + 1 < fn.blocks.size()) ? fn.blocks[bi + 1].label : "__return";
        // the epilogue between the expected blocks and the cold ones
        if (coldStart < fn.blocks.size()) {
            if (bi + 1 == coldStart) nextBlockLabel_ = "__return";
            else if (bi + 1 == fn.blocks.size()) nextBlockLabel_.clear();
            if (bi == coldStart) emitEpilogue();
        }
        // the element address cache carries into a block reached only by
        // the conditional branch that ends the block before it
        ElemCache carried;
        if (elemCache_.valid && bi > 0 && bi != coldStart && !anyAsm && !fn.addressTakenLabels.count(block.label) &&
            labelRefs_[block.label] == 1 && !fn.blocks[bi - 1].insts.empty()) {
            const auto& last = fn.blocks[bi - 1].insts.back();
            if (last.op == ir::Op::BR_COND &&
                ((last.dest.kind == ir::OperandKind::LABEL && last.dest.name == block.label) !=
                 (last.src2.kind == ir::OperandKind::LABEL && last.src2.name == block.label)))
                carried = elemCache_;
        }
        // A block reached only by falling through from the conditional branch
        // before it (a switch's next comparison): no label, so what is known
        // about the registers carries on (the switch value is not reloaded,
        // ca45's passes see one block)
        bool fallOnly = false;
        if (bi > 0 && bi != coldStart && !anyAsm && !fn.addressTakenLabels.count(block.label) && labelRefs_[block.label] == 1 &&
            !emittedLabelRefs_.count(block.label) && !fn.blocks[bi - 1].insts.empty()) {
            const auto& last = fn.blocks[bi - 1].insts.back();
            fallOnly = last.op == ir::Op::BR_COND && last.src2.kind == ir::OperandKind::LABEL &&
                       last.src2.name == block.label &&
                       !(last.dest.kind == ir::OperandKind::LABEL && last.dest.name == block.label);
        }
        if (fallOnly) {
            emitComment("@" + block.label + " (reached by falling through only)");
            carried = ElemCache();
        } else
        emitLabel("@" + block.label);
        if (carried.valid) {
            elemCache_ = carried;
            emitComment("address of " + carried.base + "[...] still in __zp_scratch (single predecessor)");
        }
        // A label used from data (static &&label table): a global alias
        if (fn.addressTakenLabels.count(block.label)) {
            ms_.invalidateAll();
            emit(".global " + fn.labelAlias(block.label));  // not scoped to the proc
            emitLabel(fn.labelAlias(block.label));
        }
        for (size_t ii = 0; ii < block.insts.size(); ii++) {
            currentInstInBlock_ = ii;
            if (skipInsts_ > 0) { skipInsts_--; currentInstIdx_++; continue; }   // emitted with the one before
            emitInst(block.insts[ii]);
            currentInstIdx_++;
        }
    }
    nextBlockLabel_.clear();

    if (coldStart == fn.blocks.size()) emitEpilogue();

    // Phase 78: Finalize SMC parameter metadata based on access counts
    // Set useSMC = true for parameters accessed >3 times (sufficient to save >= 1 byte)
    if (useSAC && trackSMCOffsetsEnabled_ && functionSMCMetadata_.count(fn.name)) {
        auto& params = functionSMCMetadata_[fn.name];
        for (auto& [paramIdx, param] : params) {
            // SMC decision threshold: >3 accesses
            // This typically saves 2-3 bytes per parameter (load instruction elimination)
            if (param.accessCount > 3) {
                param.useSMC = true;
            }
        }
    }

    // Emit parameter constant metadata for cross-file optimization analysis
    // Linker can use this to detect parameters that are ALWAYS constant across all files
    if (useSAC && relocMode_ && sacConstParams_.count(fn.name)) {
        const auto& params = sacConstParams_[fn.name];
        for (const auto& [paramIdx, info] : params) {
            if (info.isConstant) {
                // Emit metadata: .param_const function_name param_index value
                // Linker collects this from all object files to find truly-constant params
                emit(".param_const " + fn.name + " " + std::to_string(paramIdx) +
                     " " + std::to_string(info.value));
            }
        }
    }

    // Function attribute directives with per-function clobber analysis
    // Use pre-computed clobber info (which includes union of callee clobbers for non-leaf functions)
    FuncClobbers fc;
    if (functionClobberInfo_.count(fn.name) > 0) {
        fc = functionClobberInfo_[fn.name];
    } else {
        // Fallback: compute on demand (shouldn't happen in normal flow)
        fc = computeFuncClobbers(fn);
    }

    {
        std::string funcFlags = fnZp ? "zp_call" : "stack_call";
        if (useSAC) funcFlags += ", static_alloc";
        if (zeroAllocLeaves_.count(fn.name)) funcFlags += ", zeroalloc";
        if (fn.isInterrupt) funcFlags += ", isr";
        if (fn.originalIsLeaf) funcFlags += ", leaf";
        emit(".func_flags " + funcFlags);
    }
    // Emit parameter sizes so assembler can track actual sizes instead of assuming 2 bytes
    {
        std::string paramSizes;
        for (size_t i = 0; i < fn.paramTypes.size(); i++) {
            int ps = ir::typeSize(fn.paramTypes[i]);
            if (ps < 2) ps = 2;
            if (i > 0) paramSizes += ", ";
            paramSizes += std::to_string(ps);
        }
        if (!paramSizes.empty()) {
            emit(".param_sizes " + paramSizes);
        }
    }
    {
        std::string regs;
        if (fc.regs & 0x01) regs += "A, ";
        if (fc.regs & 0x02) regs += "X, ";
        if (fc.regs & 0x04) regs += "Y, ";
        if (fc.regs & 0x08) regs += "Z, ";
        if (!regs.empty()) { regs.pop_back(); regs.pop_back(); emit(".reg_clobbers " + regs); }
    }
    {
        std::string flags;
        if (fc.flags & 0x01) flags += "C, ";
        if (fc.flags & 0x02) flags += "N, ";
        if (fc.flags & 0x04) flags += "Z, ";
        if (fc.flags & 0x08) flags += "V, ";
        if (!flags.empty()) { flags.pop_back(); flags.pop_back(); emit(".flag_clobbers " + flags); }
    }
    {
        // Emit frame size for Phase 2 call-graph overlay coloring
        emit(".frame_size " + std::to_string(localFrameSize_));
    }

    if (fn.noOptimize) emit(".noopt_end");
    emit("endproc");
    flushDebugVars();
    emitBlank();

    // Phase 78: Finalize SMC metadata for this function
    // Apply decision logic: use SMC if parameter accessed > 3 times (saves >= 1 byte)
    if (staticAllocMode_ && trackSMCOffsetsEnabled_ &&
        functionSMCMetadata_.count(currentFunctionName_) > 0) {
        auto& paramMap = functionSMCMetadata_[currentFunctionName_];
        for (auto& [paramID, param] : paramMap) {
            // Decision: use SMC if parameter accessed > 3 times
            param.useSMC = param.accessCount > 3;

            // Debug output if requested
            if (sacDebugMode_) {
                // Note: Debug output goes to stderr, actual code is clean
                // emitComment("SMC: param " + std::to_string(paramID) +
                //            " accessed " + std::to_string(param.accessCount) +
                //            " times, useSMC=" + (param.useSMC ? "yes" : "no"));
            }
        }
    }
    trackSMCOffsetsEnabled_ = false;

    // Phase 47: Finalize IR tracking for this function
    finalizeIRForFunction();
}

// ============================================================================
// Instruction emission
// ============================================================================

// Check if frame-relative addressing (.fp pseudo-ops) is used after the current instruction
bool IRCodeGen::frameAddrUsedAfterCall() const {
    if (!currentFn_) return false;

    // Look ahead from current position to find frame-relative addressing usage
    for (size_t bi = currentBlockIdx_; bi < currentFn_->blocks.size(); bi++) {
        const auto& block = currentFn_->blocks[bi];
        size_t startIdx = (bi == currentBlockIdx_) ? currentInstInBlock_ + 1 : 0;

        for (size_t ii = startIdx; ii < block.insts.size(); ii++) {
            const auto& inst = block.insts[ii];

            // Stop at control flow instructions (next call, branch, return)
            if (inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID ||
                inst.op == ir::Op::CALL_INDIRECT || inst.op == ir::Op::RET ||
                inst.op == ir::Op::RET_VOID || inst.op == ir::Op::BR ||
                inst.op == ir::Op::BR_COND) {
                return false;  // Reached control flow without seeing .fp usage
            }

            // Check if this instruction uses frame-relative addressing
            // Look for vregs that are frame-based (positive offset in vregOffset_)
            if (inst.src1.isVreg() && vregOffset_.count(inst.src1.vregId)) {
                if (vregOffset_.at(inst.src1.vregId) > 0) return true;  // Frame-based
            }
            if (inst.src2.isVreg() && vregOffset_.count(inst.src2.vregId)) {
                if (vregOffset_.at(inst.src2.vregId) > 0) return true;
            }
            if (inst.dest.isVreg() && vregOffset_.count(inst.dest.vregId)) {
                if (vregOffset_.at(inst.dest.vregId) > 0) return true;
            }
            for (const auto& arg : inst.args) {
                if (arg.isVreg() && vregOffset_.count(arg.vregId)) {
                    if (vregOffset_.at(arg.vregId) > 0) return true;
                }
            }
        }
    }
    return false;  // No frame usage found after the call
}

void IRCodeGen::emitInst(const ir::Inst& inst) {
    // (measure/opsize: CC45_OPMARK=1 marks each instruction's code, for
    // ca45 --source-map to attribute bytes to IR operations)
    static const bool opmark = getenv("CC45_OPMARK") != nullptr;
    if (opmark) {
        auto kind = [](const ir::Operand& o) {
            switch (o.kind) {
                case ir::OperandKind::VREG: return 'v';
                case ir::OperandKind::IMM: return 'i';
                case ir::OperandKind::GLOBAL: return 'g';
                case ir::OperandKind::LABEL: return 'l';
                case ir::OperandKind::FRAME_RELATIVE: return 'f';
                default: return '-';
            }
        };
        ir::Type w = inst.resultType;
        for (const ir::Operand* o : {&inst.dest, &inst.src1, &inst.src2})
            if (!o->isNone() && o->kind != ir::OperandKind::LABEL && ir::typeSize(o->type) > ir::typeSize(w)) w = o->type;
        emit(std::string("; @op ") + ir::opName(inst.op) + " " + ir::typeName(w) + " " + kind(inst.src1) + kind(inst.src2));
    }
    // the element address in __zp_scratch survives only from one fused
    // element load to the next
    if (elemCache_.valid && !elemCacheSurvives(inst)) {
        // a compare or a conditional branch of bytes / words keeps it when
        // what it emits does not touch __zp_scratch (checked after)
        bool cmpLike = (inst.op >= ir::Op::CMP_EQ && inst.op <= ir::Op::CMP_GEU) || inst.op == ir::Op::BR_COND;
        auto small = [](const ir::Operand& o) {
            return o.isNone() || o.kind == ir::OperandKind::LABEL || o.type == ir::Type::I8 ||
                   o.type == ir::Type::I16 || o.type == ir::Type::PTR;
        };
        if (cmpLike && small(inst.src1) && small(inst.src2) && inst.args.empty() && !inBss_) {
            ElemCache saved = elemCache_;
            elemCache_.valid = false;
            scratchTouched_ = false;
            emitInstBody(inst);
            if (!scratchTouched_) elemCache_ = saved;
            return;
        }
        elemCache_.valid = false;
    }
    emitInstBody(inst);
}

void IRCodeGen::emitInstBody(const ir::Inst& inst) {
    if (skipThrough_) {
        if (&inst == skipThrough_) skipThrough_ = nullptr;
        return;
    }
    // (a node of a byte join: built by its root)
    if (inst.dest.isVreg() && byteJoinSkip_.count(inst.dest.vregId)) return;
    // Reset value-role tracking for each new instruction
    clearValueRoles();

    // Emit .loc directive when source location changes. An inlined
    // function's code has the line of the call (its statement): a debugger
    // steps over it as part of that line, in the function it is in
    const ir::SourceLoc& at = inst.callLoc.valid() ? inst.callLoc : inst.loc;
    if (at.valid() && (at.line != lastLocLine_ || at.file != lastLocFile_)) {
        lastLocLine_ = at.line;
        lastLocFile_ = at.file;
        emit(".loc \"" + at.file + "\", " + std::to_string(at.line));
    }

    // Build IR instruction description for -Rcodegen annotations
    auto irDesc = [&](const std::string& extra = "") -> std::string {
        if (!emitReasons_) return "";
        std::string name;
        switch (inst.op) {
            case ir::Op::NOP: name = "NOP"; break;
            case ir::Op::CONST: name = "CONST"; break;
            case ir::Op::ADD: name = "ADD"; break;
            case ir::Op::SUB: name = "SUB"; break;
            case ir::Op::MUL: name = "MUL"; break;
            case ir::Op::MUL_U: name = "MUL_U"; break;
            case ir::Op::DIV: name = "DIV"; break;
            case ir::Op::DIV_U: name = "DIV_U"; break;
            case ir::Op::MOD: name = "MOD"; break;
            case ir::Op::MOD_U: name = "MOD_U"; break;
            case ir::Op::AND: name = "AND"; break;
            case ir::Op::OR: name = "OR"; break;
            case ir::Op::XOR: name = "XOR"; break;
            case ir::Op::NOT: name = "NOT"; break;
            case ir::Op::NEG: name = "NEG"; break;
            case ir::Op::SHL: name = "SHL"; break;
            case ir::Op::SHR: name = "SHR"; break;
            case ir::Op::ASR: name = "ASR"; break;
            case ir::Op::CMP_EQ: name = "CMP_EQ"; break;
            case ir::Op::CMP_NE: name = "CMP_NE"; break;
            case ir::Op::CMP_LT: name = "CMP_LT"; break;
            case ir::Op::CMP_LE: name = "CMP_LE"; break;
            case ir::Op::CMP_GT: name = "CMP_GT"; break;
            case ir::Op::CMP_GE: name = "CMP_GE"; break;
            case ir::Op::CMP_LTU: name = "CMP_LTU"; break;
            case ir::Op::CMP_LEU: name = "CMP_LEU"; break;
            case ir::Op::CMP_GTU: name = "CMP_GTU"; break;
            case ir::Op::CMP_GEU: name = "CMP_GEU"; break;
            case ir::Op::LOAD: name = "LOAD"; break;
            case ir::Op::STORE: name = "STORE"; break;
            case ir::Op::ADDR_GLOBAL: name = "ADDR_GLOBAL"; break;
            case ir::Op::ADDR_LOCAL: name = "ADDR_LOCAL"; break;
            case ir::Op::ADDR_ELEM: name = "ADDR_ELEM"; break;
            case ir::Op::TRUNC: name = "TRUNC"; break;
            case ir::Op::ZEXT: name = "ZEXT"; break;
            case ir::Op::SEXT: name = "SEXT"; break;
            case ir::Op::BR: name = "BR"; break;
            case ir::Op::BR_COND: name = "BR_COND"; break;
            case ir::Op::CALL: name = "CALL"; break;
            case ir::Op::CALL_VOID: name = "CALL_VOID"; break;
            case ir::Op::RET: name = "RET"; break;
            case ir::Op::RET_VOID: name = "RET_VOID"; break;
            case ir::Op::COPY: name = "COPY"; break;
            case ir::Op::DEREF: name = "DEREF"; break;
            default: name = "OP" + std::to_string((int)inst.op); break;
        }
        std::string s = name;
        if (inst.dest.isVreg()) s += " → " + vregDesc(inst.dest.vregId);
        if (!extra.empty()) s += " " + extra;
        return s;
    };

    // Reset store-forwarding state unless this is a STORE that will consume it
    if (inst.op != ir::Op::STORE && resultInAX_ != -1) {
        resultInAX_ = -1;
    }

    switch (inst.op) {
        case ir::Op::NOP:
            break;

        case ir::Op::CONST: {
            int val = (int)inst.src1.immVal;
            if (inst.dest.isVreg()) {
                if (multiDefVregs_.count(inst.dest.vregId)) vregConstVal_.erase(inst.dest.vregId);
                else vregConstVal_[inst.dest.vregId] = val;
            }
            // Skip emission for suppressed CONST vregs (address-only, used by direct store)
            if (inst.dest.isVreg() && suppressedVregs_.count(inst.dest.vregId)) break;

            // --- CONST direct store optimization ---
            // If the next instruction is STORE of this CONST's result to a ZP local,
            // store directly to the local's ZP address without round-tripping through temp.
            if (inst.dest.isVreg() && (inst.resultType == ir::Type::I16 || inst.resultType == ir::Type::I8)) {
                auto* nextInst = peekNextInst();
                // (only when the store is the CONST's one use: the value is not
                // put in the CONST's own register)
                if (nextInst && nextInst->op == ir::Op::STORE &&
                    nextInst->src1.isVreg() && nextInst->src1.vregId == inst.dest.vregId &&
                    nextInst->src2.isVreg() && localSlotVregs_.count(nextInst->src2.vregId) &&
                    !vregUsedOutside(inst.dest.vregId, &inst, nextInst)) {
                    auto destAlloc = alloc_.getAlloc(nextInst->src2.vregId);
                    if (destAlloc.loc == VRegAllocator::IN_ZP) {
                        std::string r = irDesc("val=" + std::to_string(val) + " → direct store");
                        uint8_t b0 = val & 0xFF;
                        uint8_t b1 = (val >> 8) & 0xFF;
                        std::stringstream ss;
                        ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << destAlloc.offset;
                        emit("lda #" + std::to_string((int)b0), r);
                        emit("sta " + ss.str());
                        if (inst.resultType != ir::Type::I8) {
                            ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (destAlloc.offset + 1);
                            if (b1 == b0) {
                                emit("sta " + ss.str());
                            } else {
                                emit("ldx #" + std::to_string((int)b1));
                                emit("stx " + ss.str());
                            }
                        }
                        // Mark that the next STORE should be skipped
                        resultInAX_ = -2; // special: means "next STORE already handled"
                        ms_.invalidateAll();
                        break;
                    }
                    if (destAlloc.loc == VRegAllocator::IN_FRAME && inst.resultType == ir::Type::I16) {
                        // Direct store to frame — use SAC addressing if enabled, otherwise .fp pseudo-op
                        std::string r = irDesc("val=" + std::to_string(val) + " → direct frame store");
                        uint8_t b0 = val & 0xFF;
                        uint8_t b1 = (val >> 8) & 0xFF;
                        emit("lda #" + std::to_string((int)b0), r);

                        if (currentFunctionUseSAC_ && vregOffset_.count(nextInst->src2.vregId)) {
                            // SAC mode: Use direct inline storage addressing
                            uint32_t vregId = nextInst->src2.vregId;
                            std::string storageSymbol;
                            if (vregId < currentFn_->paramTypes.size()) {
                                std::string pName = (vregId < currentFn_->paramNames.size() && !currentFn_->paramNames[vregId].empty())
                                    ? currentFn_->paramNames[vregId] : std::to_string(vregId);
                                storageSymbol = currentFunctionName_ + "__param_" + pName;
                            } else {
                                storageSymbol = currentFunctionName_ + "__local_" + std::to_string(vregId);
                            }
                            emit("sta " + storageSymbol, r);
                            if (b1 == b0) {
                                emit("sta " + storageSymbol + "+1", r);
                            } else {
                                emit("lda #" + std::to_string((int)b1), r);
                                emit("sta " + storageSymbol + "+1", r);
                            }
                        } else {
                            // Non-SAC: Frame-relative via staz.fp pseudo-op
                            std::string sym = "__vr" + std::to_string(nextInst->src2.vregId);
                            if (b1 == b0) {
                                emit("taz", r);
                            } else {
                                emit("ldz #" + std::to_string((int)b1), r);
                            }
                            emit("staz.local " + sym, r);
                        }
                        resultInAX_ = -2;
                        ms_.invalidateAll();
                        break;
                    }
                }
            }

            std::string r = irDesc("val=" + std::to_string(val));
            uint8_t b0 = val & 0xFF;
            uint8_t b1 = (val >> 8) & 0xFF;
            uint8_t b2 = (val >> 16) & 0xFF;
            uint8_t b3 = (val >> 24) & 0xFF;
            ms_.setConst(REG_A, b0);
            emit("lda #" + std::to_string((int)b0), r);
            valueByte_[0] = REG_A;
            // Can skip tax/tay/taz when byte matches A only if storeVreg
            // uses per-byte stores (IN_ZP). Frame stores use stax.fp/staxyz.fp
            // which require actual register values in X (or X/Y/Z for I32).
            bool canSkipTransfer = inst.dest.isVreg() &&
                alloc_.getAlloc(inst.dest.vregId).loc == VRegAllocator::IN_ZP;
            if (inst.resultType == ir::Type::I32) {
                if (b1 == b0 && canSkipTransfer) { ms_.setConst(REG_X, b1); } else if (b1 == b0) { emit("tax", r); ms_.setTransfer(REG_X, REG_A); valueByte_[1] = REG_X; } else { emit("ldx #" + std::to_string((int)b1), r); ms_.setConst(REG_X, b1); valueByte_[1] = REG_X; }
                if (b2 == b0 && canSkipTransfer) { ms_.setConst(REG_Y, b2); } else if (b2 == b0) { emit("tay", r); ms_.setTransfer(REG_Y, REG_A); valueByte_[2] = REG_Y; } else { emit("ldy #" + std::to_string((int)b2), r); ms_.setConst(REG_Y, b2); valueByte_[2] = REG_Y; }
                if (b3 == b0 && canSkipTransfer) { ms_.setConst(REG_Z, b3); } else if (b3 == b0) { emit("taz", r); ms_.setTransfer(REG_Z, REG_A); valueByte_[3] = REG_Z; } else { emit("ldz #" + std::to_string((int)b3), r); ms_.setConst(REG_Z, b3); valueByte_[3] = REG_Z; }
            } else if (inst.resultType != ir::Type::I8) {
                if (b1 == b0 && canSkipTransfer) {
                    ms_.setConst(REG_X, b1);
                    // X not actually loaded — valueByte_[1] stays VB_NONE
                    // storeVreg for IN_ZP uses sta for both bytes when A==X
                } else if (b1 == b0) {
                    emit("tax", r); ms_.setTransfer(REG_X, REG_A);
                    valueByte_[1] = REG_X;
                } else {
                    emit("ldx #" + std::to_string((int)b1), r); ms_.setConst(REG_X, b1);
                    valueByte_[1] = REG_X;
                }
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            // A skipped transfer (equal bytes stored from A) leaves X/Y/Z unloaded
            bool skippedTransfer = canSkipTransfer && inst.resultType != ir::Type::I8 &&
                (b1 == b0 || (inst.resultType == ir::Type::I32 && (b2 == b0 || b3 == b0)));
            // stax.fp clobbers X via TSX — don't claim result is in AX after frame store
            if (inst.dest.isVreg() && !skippedTransfer &&
                alloc_.getAlloc(inst.dest.vregId).loc != VRegAllocator::IN_FRAME)
                resultInAX_ = (int32_t)inst.dest.vregId;
            else
                resultInAX_ = -1;
            ms_.invalidateAll(); // conservative: reset after CONST+store
            break;
        }

        case ir::Op::ADD:
        case ir::Op::SUB: {
            // A pointer + offset folded into its loads/stores
            if (inst.op == ir::Op::ADD && inst.dest.isVreg() && foldedAddrs_.count(inst.dest.vregId)) break;
            // an int's x + c stored through a far pointer next, its one use:
            // added there byte by byte (#348)
            if ((inst.resultType == ir::Type::I16 || inst.resultType == ir::Type::PTR) && inst.dest.isVreg() &&
                inst.src2.isImm() && !directByte(inst.src1, 0).empty() && !directByte(inst.src1, 1).empty()) {
                const ir::Inst* nx = peekNextInst();
                if (nx && nx->op == ir::Op::FAR_STORE && (nx->resultType == ir::Type::I16 || nx->resultType == ir::Type::PTR) &&
                    nx->src1.isVreg() && nx->src1.vregId == inst.dest.vregId &&
                    !(nx->src2.isVreg() && nx->src2.vregId == inst.dest.vregId) &&
                    !(inst.src1.isVreg() && nx->src2.isVreg() && nx->src2.vregId == inst.src1.vregId) &&
                    !vregUsedOutside(inst.dest.vregId, &inst, nx)) {
                    farAddPending_ = &inst;
                    break;
                }
            }
            // --- I16 INC/DEC peephole for local += 1 / local -= 1 ---
            // Pattern: %n = add/sub i16 %local, 1; store %n, %local
            // Emit: inc $ZP; bne +2; inc $ZP+1 (or dec variant)
            // Check if src2 is immediate 1, or a CONST vreg holding 1
            bool src2IsOne = (inst.src2.isImm() && inst.src2.immVal == 1);
            if (!src2IsOne && inst.src2.isVreg()) {
                auto cit = vregConstVal_.find(inst.src2.vregId);
                if (cit != vregConstVal_.end() && cit->second == 1) src2IsOne = true;
            }
            if ((inst.resultType == ir::Type::I16 || inst.resultType == ir::Type::PTR) && src2IsOne &&
                inst.src1.isVreg() && inst.dest.isVreg()) {
                auto* nextInst = peekNextInst();
                if (nextInst && nextInst->op == ir::Op::STORE &&
                    nextInst->src1.isVreg() && nextInst->src1.vregId == inst.dest.vregId &&
                    nextInst->src2.isVreg() && nextInst->src2.vregId == inst.src1.vregId &&
                    localSlotVregs_.count(nextInst->src2.vregId) &&
                    !vregUsedOutside(inst.dest.vregId, &inst, nextInst)) {
                    auto srcAlloc = alloc_.getAlloc(inst.src1.vregId);
                    if (srcAlloc.loc == VRegAllocator::IN_ZP) {
                        std::stringstream lo, hi;
                        lo << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << srcAlloc.offset;
                        hi << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (srcAlloc.offset + 1);
                        std::string r = irDesc(inst.op == ir::Op::ADD ? "inc16 ZP" : "dec16 ZP");
                        emit((inst.op == ir::Op::ADD ? "inw " : "dew ") + lo.str(), r);
                        resultInAX_ = -2;
                        ms_.invalidateAll();
                        break;
                    }
                    if (srcAlloc.loc == VRegAllocator::IN_FRAME) {
                        if (currentFunctionUseSAC_) {
                            // SAC mode: use absolute BSS symbol
                            std::string sym = sacSymbol(inst.src1.vregId);
                            std::string r = irDesc(inst.op == ir::Op::ADD ? "inc16 SAC" : "dec16 SAC");
                            if (inst.op == ir::Op::ADD) {
                                emit("inc " + sym, r);
                                emit("bne *+5");
                                emit("inc " + sym + "+1");
                            } else {
                                emit("lda " + sym, r);
                                emit("bne *+5");
                                emit("dec " + sym + "+1");
                                emit("dec " + sym);
                            }
                        } else {
                            std::string sym = "__vr" + std::to_string(inst.src1.vregId);
                            std::string r = irDesc(inst.op == ir::Op::ADD ? "inc16 frame" : "dec16 frame");
                            emit(inst.op == ir::Op::ADD ? "inc.16f " + sym : "dec.16f " + sym, r);
                        }
                        resultInAX_ = -2;
                        ms_.invalidateAll();
                        break;
                    }
                }
            }

            // I8 local += 1 / -= 1 (++c, c--): inc/dec the byte in place
            if (inst.resultType == ir::Type::I8 && inst.src1.isVreg() && inst.dest.isVreg() &&
                inst.src2.isImm()) {
                int64_t c = inst.src2.immVal;
                bool up = (inst.op == ir::Op::ADD && (c & 0xFF) == 1) || (inst.op == ir::Op::SUB && (c & 0xFF) == 0xFF);
                bool down = (inst.op == ir::Op::SUB && (c & 0xFF) == 1) || (inst.op == ir::Op::ADD && (c & 0xFF) == 0xFF);
                auto* nextInst = peekNextInst();
                std::string at = directBase(inst.src1.vregId);
                if ((up || down) && !at.empty() && nextInst && nextInst->op == ir::Op::STORE &&
                    nextInst->src1.isVreg() && nextInst->src1.vregId == inst.dest.vregId &&
                    nextInst->src2.isVreg() && nextInst->src2.vregId == inst.src1.vregId &&
                    localSlotVregs_.count(nextInst->src2.vregId) &&
                    !vregUsedOutside(inst.dest.vregId, &inst, nextInst)) {
                    emit((up ? "inc " : "dec ") + at, irDesc(up ? "inc8 in place" : "dec8 in place"));
                    resultInAX_ = -2;
                    ms_.invalidateAll();
                    break;
                }
            }

            // I8 fast path: use native 8-bit add/sub
            if (inst.resultType == ir::Type::I8) {
                // Check for inc a / dec a optimization
                bool isInc = (inst.op == ir::Op::ADD && inst.src2.isImm() && inst.src2.immVal == 1);
                bool isDec = (inst.op == ir::Op::SUB && inst.src2.isImm() && inst.src2.immVal == 1);
                // src2's operand first: a value not in ZP is loaded (through A)
                // into scratch, which would clobber src1 already loaded in A
                std::string src2 = (isInc || isDec) ? std::string() : src2MemOperand(inst.src2);
                loadOperandA(inst.src1);
                if (isInc) {
                    emit("inc a");
                } else if (isDec) {
                    emit("dec a");
                } else {
                    if (inst.op == ir::Op::ADD) {
                        emit("clc");
                        emit("adc " + src2);
                    } else {
                        emit("sec");
                        emit("sbc " + src2);
                    }
                }
                if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
                // stax.fp clobbers X via TSX — don't claim result is in AX after frame store
                if (inst.dest.isVreg() && alloc_.getAlloc(inst.dest.vregId).loc != VRegAllocator::IN_FRAME)
                    resultInAX_ = (int32_t)inst.dest.vregId;
                else
                    resultInAX_ = -1;
                break;
            }

            // Operands and destination all in memory (ZP, SAC storage) or
            // immediate: add byte by byte straight into the destination
            //   lda a; clc; adc b; sta d; lda a+1; adc b+1; sta d+1
            // or for 32 bits in memory: clc; ldq a; adcq b; stq d. The
            // destination is the local slot of a following STORE of the result
            // (the STORE is skipped) or the result's own storage.
            if ((inst.resultType == ir::Type::I16 || inst.resultType == ir::Type::PTR ||
                 inst.resultType == ir::Type::I32) && inst.dest.isVreg()) {
                int nb = inst.resultType == ir::Type::I32 ? 4 : 2;
                auto* nextInst = peekNextInst();
                std::string dst;
                bool skipStore = false;
                if (nextInst && nextInst->op == ir::Op::STORE && nextInst->resultType == inst.resultType &&
                    nextInst->args.empty() &&
                    nextInst->src1.isVreg() && nextInst->src1.vregId == inst.dest.vregId &&
                    nextInst->src2.isVreg() && localSlotVregs_.count(nextInst->src2.vregId) &&
                    !suppressedVregs_.count(nextInst->src2.vregId) &&
                    !vregConstVal_.count(nextInst->src2.vregId) &&
                    !vregUsedOutside(inst.dest.vregId, &inst, nextInst)) {
                    dst = directBase(nextInst->src2.vregId);
                    skipStore = !dst.empty();
                }
                if (dst.empty() && !localSlotVregs_.count(inst.dest.vregId))
                    dst = directBase(inst.dest.vregId);
                bool ok = !dst.empty();
                for (int k = 0; ok && k < nb; k++)
                    ok = !directByte(inst.src1, k).empty() && !directByte(inst.src2, k).empty();
                if (ok) {
                    bool add = inst.op == ir::Op::ADD;
                    std::string r = irDesc("add/sub into memory");
                    auto byteOf = [&](const std::string& base, int k) {
                        if (k == 0) return base;
                        if (base[0] == '$') return "$" + hex8((uint8_t)(std::stoi(base.substr(1), nullptr, 16) + k));
                        return base + "+" + std::to_string(k);
                    };
                    // a long +/- 1: inq / deq (in place, or in Q between ldq and stq)
                    int step = 0;
                    if (nb == 4 && directByte(inst.src1, 0)[0] != '#') {
                        // (a constant, immediate or in a vreg: its bytes)
                        std::string b0 = directByte(inst.src2, 0), b1 = directByte(inst.src2, 1),
                                    b2 = directByte(inst.src2, 2), b3 = directByte(inst.src2, 3);
                        if (b0 == "#1" && b1 == "#0" && b2 == "#0" && b3 == "#0") step = add ? 1 : -1;
                        else if (b0 == "#255" && b1 == "#255" && b2 == "#255" && b3 == "#255") step = add ? -1 : 1;
                    }
                    if (step) {
                        std::string src = directByte(inst.src1, 0);
                        std::string m = step > 0 ? "inq" : "deq";
                        if (src == dst) {
                            emit(m + " " + dst, irDesc(step > 0 ? "a long + 1 in place" : "a long - 1 in place"));
                        } else {
                            emit("ldq " + src, irDesc(step > 0 ? "a long + 1" : "a long - 1"));
                            emit(m);
                            emit("stq " + dst);
                        }
                    } else if (nb == 4 && directByte(inst.src1, 0)[0] != '#' && directByte(inst.src2, 0)[0] != '#') {
                        emit(add ? "clc" : "sec", r);
                        emit("ldq " + directByte(inst.src1, 0));
                        emit((add ? "adcq " : "sbcq ") + directByte(inst.src2, 0));
                        emit("stq " + dst);
                    } else if (nb == 4 && add && directByte(inst.src1, 0)[0] != '#' && directByte(inst.src2, 0)[0] == '#' &&
                               directByte(inst.src2, 1) == "#0" && directByte(inst.src2, 2) == "#0" &&
                               directByte(inst.src2, 3) == "#0") {
                        // + a byte: in Q, the carry into X, Y, Z only as far
                        // as it goes (20 bytes instead of 25-29)
                        std::string l = "@__addq_" + std::to_string(labelCounter_++);
                        emit("ldq " + directByte(inst.src1, 0), irDesc("add a byte to a long"));
                        emit("clc");
                        emit("adc " + directByte(inst.src2, 0));
                        emit("bcc " + l);
                        emit("inx"); emit("bne " + l);
                        emit("iny"); emit("bne " + l);
                        emit("inz");
                        emitLabel(l);
                        emit("stq " + dst);
                    } else if (add) {
                        // byte by byte with what is known (#348, #350): a byte
                        // where one side is 0 and no carry comes in is the
                        // other's (both 0: 0); bits that cannot overlap are
                        // or'ed; an adc only where a carry can arrive (clc
                        // before the first)
                        int w = nb * 8;
                        uint64_t ma = kb_.maxOf(inst.src1, w), mb = kb_.maxOf(inst.src2, w);
                        // in place (the result in x's bytes, #350): the bytes
                        // an add does not change are left; + (1 << 8k): inc
                        bool inPlace = true;
                        for (int k = 0; k < nb && inPlace; k++) inPlace = directByte(inst.src1, k) == byteOf(dst, k);
                        if (inPlace && directByte(inst.src2, 0)[0] == '#') {
                            int one = -1;
                            bool single = true;
                            for (int k = 0; k < nb && single; k++) {
                                std::string b = directByte(inst.src2, k);
                                if (b == "#0") continue;
                                if (b == "#1" && one < 0) one = k; else single = false;
                            }
                            if (single && one >= 0) {
                                std::string l = "@__inc_" + std::to_string(labelCounter_++);
                                for (int k = one; k < nb; k++) {
                                    emit("inc " + byteOf(dst, k), k == one ? r : "");
                                    if (k + 1 < nb) emit("bne " + l);
                                }
                                if (one + 1 < nb) emitLabel(l);
                                ms_.invalidateAll();
                                storeCache_.vreg = UINT32_MAX;
                                resultInAX_ = skipStore ? -2 : -1;
                                break;
                            }
                        }
                        bool carryIn = false, cleared = false;
                        for (int k = 0; k < nb; k++) {
                            unsigned a = (ma >> (8 * k)) & 0xFF, b = (mb >> (8 * k)) & 0xFF;
                            std::string sa = a ? directByte(inst.src1, k) : "#0", sb = b ? directByte(inst.src2, k) : "#0";
                            if (!carryIn && !b && inPlace) {
                                continue;   // (the byte as it is)
                            } else if (!carryIn && (!a || !b)) {
                                emit("lda " + (a ? sa : sb), k == 0 ? r : "");
                                carryIn = false;
                            } else if (!carryIn && (a & b) == 0 && sb[0] == '#') {
                                emit("lda " + sa, k == 0 ? r : "");
                                emit("ora " + sb);
                            } else {
                                emit("lda " + sa, k == 0 ? r : "");
                                if (!carryIn && !cleared) { emit("clc"); cleared = true; }
                                else if (!carryIn) emit("clc");
                                emit("adc " + sb);
                                carryIn = a + b + 1 > 255;
                                emit("sta " + byteOf(dst, k));
                                continue;
                            }
                            carryIn = false;
                            emit("sta " + byteOf(dst, k));
                        }
                    } else {
                        for (int k = 0; k < nb; k++) {
                            emit("lda " + directByte(inst.src1, k), k == 0 ? r : "");
                            if (k == 0) emit("sec");
                            emit("sbc " + directByte(inst.src2, k));
                            emit("sta " + byteOf(dst, k));
                        }
                    }
                    ms_.invalidateAll();
                    storeCache_.vreg = UINT32_MAX;
                    resultInAX_ = skipStore ? -2 : -1;
                    break;
                }
            }

            // A zero-extended byte (not emitted): the other operand in A/X,
            // the byte added, the carry into X
            if (inst.resultType == ir::Type::I16 || inst.resultType == ir::Type::PTR) {
                bool z2 = inst.src2.isVreg() && zextAlias_.count(inst.src2.vregId);
                bool z1 = !z2 && inst.src1.isVreg() && zextAlias_.count(inst.src1.vregId);
                if (z1 && inst.op == ir::Op::SUB) {
                    // byte - x: the high byte 0 - x's high byte
                    std::string b = directByte(inst.src1, 0);
                    std::string lo = directByte(inst.src2, 0), hi = directByte(inst.src2, 1);
                    if (lo.empty() || hi.empty()) {
                        lo = src2MemOperand(inst.src2);
                        hi = lo + "+1";
                    }
                    emit("lda " + b, irDesc("zero-extended byte minus a value"));
                    emit("sec");
                    emit("sbc " + lo);
                    emit("pha");
                    emit("lda #0");
                    emit("sbc " + hi);
                    emit("tax");
                    emit("pla");
                    ms_.invalidateAll();
                    if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
                    if (inst.dest.isVreg() && alloc_.getAlloc(inst.dest.vregId).loc != VRegAllocator::IN_FRAME)
                        resultInAX_ = (int32_t)inst.dest.vregId;
                    else
                        resultInAX_ = -1;
                    break;
                }
                if (z1 || z2) {
                    bool add = inst.op == ir::Op::ADD;
                    std::string b = directByte(z2 ? inst.src2 : inst.src1, 0);
                    loadOperand(z2 ? inst.src1 : inst.src2);
                    std::string l = "@__zx_" + std::to_string(labelCounter_++);
                    emit(add ? "clc" : "sec", irDesc(add ? "add a zero-extended byte" : "sub a zero-extended byte"));
                    emit((add ? "adc " : "sbc ") + b);
                    emit((add ? "bcc " : "bcs ") + l);
                    emit(add ? "inx" : "dex");
                    emitLabel(l);
                    ms_.invalidateAll();
                    if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
                    if (inst.dest.isVreg() && alloc_.getAlloc(inst.dest.vregId).loc != VRegAllocator::IN_FRAME)
                        resultInAX_ = (int32_t)inst.dest.vregId;
                    else
                        resultInAX_ = -1;
                    break;
                }
            }

            // For commutative ADD: if src1 is in frame (would clobber scratch),
            // swap operands — load src1 as memory, src2 into AX
            bool src1InFrame = inst.src1.isVreg() &&
                alloc_.getAlloc(inst.src1.vregId).loc == VRegAllocator::IN_FRAME;
            bool src2InFrame = inst.src2.isVreg() &&
                alloc_.getAlloc(inst.src2.vregId).loc == VRegAllocator::IN_FRAME;

            // --- I16 add/sub store-fused peephole ---
            // When result is immediately stored to a ZP local, avoid pha/txa/adc/tax/pla.
            // Load only lo byte of src1, do lo add+store, then load hi byte of src1
            // directly (skip ldx + txa):
            //   lda src1_lo; clc; adc src2_lo; sta dest_lo; lda src1_hi; adc src2_hi; sta dest_hi
            // Store-fused check: can we emit inline add and store directly to ZP?
            bool addSubFused = false;
            if (inst.resultType == ir::Type::I16 && inst.dest.isVreg() && !src1InFrame && !src2InFrame) {
                // Check 1: next instruction is STORE to a ZP local
                auto* nextInst = peekNextInst();
                bool storeToLocal = nextInst && nextInst->op == ir::Op::STORE &&
                    nextInst->src1.isVreg() && nextInst->src1.vregId == inst.dest.vregId &&
                    nextInst->src2.isVreg() && localSlotVregs_.count(nextInst->src2.vregId) &&
                    alloc_.getAlloc(nextInst->src2.vregId).loc == VRegAllocator::IN_ZP;
                // Check 2: dest vreg itself is ZP-allocated (result stored to own slot)
                bool destIsZP = alloc_.getAlloc(inst.dest.vregId).loc == VRegAllocator::IN_ZP;

                VRegAllocator::Allocation destAlloc;
                if (storeToLocal) {
                    destAlloc = alloc_.getAlloc(nextInst->src2.vregId);
                } else if (destIsZP) {
                    destAlloc = alloc_.getAlloc(inst.dest.vregId);
                }

                if (storeToLocal || destIsZP) {
                    if (destAlloc.loc == VRegAllocator::IN_ZP) {
                        // Determine src1 hi byte source
                        std::string src1hi;
                        if (inst.src1.isImm()) {
                            src1hi = "#" + std::to_string((int)((inst.src1.immVal >> 8) & 0xFF));
                        } else if (inst.src1.isVreg() && globalLoadAlias_.count(inst.src1.vregId)) {
                            src1hi = directByte(inst.src1, 1);
                        } else if (inst.src1.isVreg()) {
                            auto s1alloc = alloc_.getAlloc(inst.src1.vregId);
                            if (s1alloc.loc == VRegAllocator::IN_ZP) {
                                std::stringstream ss;
                                ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (s1alloc.offset + 1);
                                src1hi = ss.str();
                            }
                        }
                        if (!src1hi.empty()) {
                            std::string r = irDesc("store-fused add/sub.16");
                            // Use immediate for CONST vreg src2
                            std::string src2mem;
                            if (inst.src2.isVreg()) {
                                auto cit = vregConstVal_.find(inst.src2.vregId);
                                if (cit != vregConstVal_.end()) {
                                    src2mem = "#" + std::to_string((int)(cit->second & 0xFFFF));
                                } else {
                                    src2mem = src2MemOperand(inst.src2);
                                }
                            } else {
                                src2mem = src2MemOperand(inst.src2);
                            }
                            // Load only lo byte of src1 (no ldx for hi — we use lda src1hi later)
                            if (inst.src1.isImm()) {
                                emit("lda #" + std::to_string((int)(inst.src1.immVal & 0xFF)), r);
                            } else if (inst.src1.isVreg() && globalLoadAlias_.count(inst.src1.vregId)) {
                                emit("lda " + directByte(inst.src1, 0), r);
                            } else if (inst.src1.isVreg()) {
                                auto s1a = alloc_.getAlloc(inst.src1.vregId);
                                if (s1a.loc == VRegAllocator::IN_ZP) {
                                    std::stringstream ss;
                                    ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << s1a.offset;
                                    emit("lda " + ss.str(), r);
                                } else {
                                    loadOperand(inst.src1); // fallback: loads A:X
                                }
                            } else {
                                loadOperand(inst.src1);
                            }
                            std::stringstream dlo, dhi;
                            dlo << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << destAlloc.offset;
                            dhi << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (destAlloc.offset + 1);
                            emit(inst.op == ir::Op::ADD ? "clc" : "sec");
                            emit(inst.op == ir::Op::ADD ? "adc " + src2mem : "sbc " + src2mem);
                            emit("sta " + dlo.str());
                            emit("lda " + src1hi);
                            // Hi byte of src2
                            if (src2mem[0] == '#') {
                                int64_t immVal = 0;
                                try { immVal = std::stol(src2mem.substr(1)); } catch (...) {}
                                emit(inst.op == ir::Op::ADD ?
                                    "adc #" + std::to_string((int)((immVal >> 8) & 0xFF)) :
                                    "sbc #" + std::to_string((int)((immVal >> 8) & 0xFF)));
                            } else {
                                emit(inst.op == ir::Op::ADD ? "adc " + src2mem + "+1" : "sbc " + src2mem + "+1");
                            }
                            emit("sta " + dhi.str());
                            if (storeToLocal) resultInAX_ = -2; // skip next STORE
                            ms_.invalidateAll();
                            addSubFused = true;
                        }
                    }
                }
            }
            if (addSubFused) break;

            std::string mn;
            if (inst.resultType == ir::Type::I32) {
                mn = (inst.op == ir::Op::ADD) ? "add.32" : "sub.32";
            } else {
                mn = (inst.op == ir::Op::ADD) ? "add.16" : "sub.16";
            }
            std::string reg = (inst.resultType == ir::Type::I32) ? ".AXYZ" : ".AX";

            if (inst.resultType == ir::Type::I32 && src1InFrame && src2InFrame) {
                // Both I32 operands in frame: ldaxyz.fp clobbers __zp_scratch2/__zp_scratch3.
                // Load src2 first, save to safe ZP area ($10-$13), then load src1 into AXYZ.
                auto hex8 = [](int v) { std::stringstream s; s << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << v; return s.str(); };
                int safeZP = ZP_TEMP32;  // dedicated 4-byte temp
                loadOperand(inst.src2);
                emit("sta " + hex8(safeZP));
                emit("stx " + hex8(safeZP + 1));
                emit("sty " + hex8(safeZP + 2));
                emit("stz " + hex8(safeZP + 3));
                loadOperand(inst.src1);
                emit(mn + " " + reg + ", " + hex8(safeZP));
            } else {
                // Swap for an ADD: load src2 into AX, use src1 as memory operand
                bool swap = inst.op == ir::Op::ADD && src2InFrame && !src1InFrame;
                std::string mem = src2MemOperand(swap ? inst.src1 : inst.src2);
                loadOperand(swap ? inst.src2 : inst.src1);
                // 16 bits into a destination in ZP or static storage: computed
                // into it, without add.16's pha/pla (A/X do not keep the result)
                std::string dbase = inst.dest.isVreg() ? directBase(inst.dest.vregId) : "";
                if (inst.resultType != ir::Type::I32 && !dbase.empty() && !mem.empty()) {
                    std::string lo = mem, hi = mem + "+1";
                    if (mem[0] == '#') {
                        long v = std::stol(mem.substr(1));
                        lo = "#" + std::to_string(v & 0xFF);
                        hi = "#" + std::to_string((v >> 8) & 0xFF);
                    }
                    std::string d0 = dbase;
                    std::string d1 = dbase[0] == '$' ? "$" + hex8((uint8_t)(std::stoi(dbase.substr(1), nullptr, 16) + 1))
                                                     : dbase + "+1";
                    bool add = inst.op == ir::Op::ADD;
                    emit(add ? "clc" : "sec", irDesc(add ? "add into the destination" : "sub into the destination"));
                    emit((add ? "adc " : "sbc ") + lo);
                    emit("sta " + d0);
                    emit("txa");
                    emit((add ? "adc " : "sbc ") + hi);
                    emit("sta " + d1);
                    ms_.invalidateAll();
                    storeCache_.vreg = UINT32_MAX;
                    resultInAX_ = -1;
                    break;
                }
                emit(mn + " " + reg + ", " + mem);
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            // stax.fp clobbers X via TSX — don't claim result is in AX after frame store
            if (inst.dest.isVreg() && alloc_.getAlloc(inst.dest.vregId).loc != VRegAllocator::IN_FRAME)
                resultInAX_ = (int32_t)inst.dest.vregId;
            else
                resultInAX_ = -1;
            break;
        }

        case ir::Op::AND:
        case ir::Op::OR:
        case ir::Op::XOR: {
            // A zero-extended byte (not emitted): with the other operand's
            // low byte (and: the high byte 0; xor: the other's high byte)
            if (inst.op != ir::Op::OR && (inst.resultType == ir::Type::I16 || inst.resultType == ir::Type::PTR)) {
                bool z2 = inst.src2.isVreg() && zextAlias_.count(inst.src2.vregId);
                bool z1 = !z2 && inst.src1.isVreg() && zextAlias_.count(inst.src1.vregId);
                if (z1 || z2) {
                    bool isAnd = inst.op == ir::Op::AND;
                    std::string b = directByte(z2 ? inst.src2 : inst.src1, 0);
                    const ir::Operand& other = z2 ? inst.src1 : inst.src2;
                    // (and: only the other's low byte matters)
                    std::string olo = isAnd ? directByte(other, 0) : "";
                    if (!olo.empty()) emit("lda " + olo, irDesc("and a zero-extended byte"));
                    else loadOperand(other);
                    if (isAnd) emit("ldx #0", olo.empty() ? irDesc("and a zero-extended byte") : "");
                    emit((isAnd ? "and " : "eor ") + b, isAnd ? "" : irDesc("xor a zero-extended byte"));
                    ms_.invalidateAll();
                    if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
                    if (inst.dest.isVreg() && alloc_.getAlloc(inst.dest.vregId).loc != VRegAllocator::IN_FRAME)
                        resultInAX_ = (int32_t)inst.dest.vregId;
                    else
                        resultInAX_ = -1;
                    lastCmp_.valid = false;
                    break;
                }
            }
            if (inst.op == ir::Op::OR && inst.dest.isVreg() && byteJoins_.count(inst.dest.vregId)) {
                // each byte from where it lives (high bytes first: Z through A)
                const auto& bytes = byteJoins_[inst.dest.vregId].bytes;
                auto src = [&](int k) {
                    const auto& b = bytes[k];
                    if (b.kind == 0) return std::string("#0");
                    if (b.kind == 2) return "#" + std::to_string(b.byte);
                    return directByte(ir::Operand::vreg(b.vreg, ir::Type::I32), b.byte);
                };
                ms_.invalidateAll();
                if (bytes.size() == 4) {
                    // (ldz has no base-page mode)
                    std::string z = src(3);
                    if (z[0] == '#') emit("ldz " + z, irDesc("bytes joined"));
                    else { emit("lda " + z, irDesc("bytes joined")); emit("taz"); }
                    emit("ldy " + src(2));
                }
                emit("ldx " + src(1), bytes.size() == 4 ? "" : irDesc("bytes joined"));
                emit("lda " + src(0));
                if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
                lastCmp_.valid = false;
                break;
            }
            // I8 fast path: use native 8-bit bitwise ops
            if (inst.resultType == ir::Type::I8) {
                // Check for single-bit AND test fused with BR_COND → BBS/BBR
                bool canFuse = false;
                if (inst.dest.isVreg() && currentFn_) {
                    auto& blk = currentFn_->blocks[currentBlockIdx_];
                    size_t instIdx = &inst - &blk.insts[0];
                    if (instIdx + 1 < blk.insts.size()) {
                        const auto& next = blk.insts[instIdx + 1];
                        if (next.op == ir::Op::BR_COND && next.src1.isVreg() &&
                            next.src1.vregId == inst.dest.vregId) {
                            canFuse = true;
                        }
                    }
                }

                // Single-bit AND with fused branch: use BBS/BBR if source is in ZP
                // Check both immediate operands and vreg operands holding known constants
                int64_t andMask = -1;
                if (inst.src2.isImm()) {
                    andMask = inst.src2.immVal;
                } else if (inst.src2.isVreg()) {
                    // Check if the vreg was defined by a CONST in this block
                    auto& blk = currentFn_->blocks[currentBlockIdx_];
                    size_t instIdx = &inst - &blk.insts[0];
                    for (size_t k = 0; k < instIdx; k++) {
                        auto& prev = blk.insts[k];
                        if (prev.op == ir::Op::CONST && prev.dest.isVreg() &&
                            prev.dest.vregId == inst.src2.vregId) {
                            andMask = prev.src1.immVal;
                        }
                    }
                }
                if (canFuse && inst.op == ir::Op::AND && andMask >= 0) {
                    int64_t mask = andMask;
                    int bitN = -1;
                    if (mask > 0 && (mask & (mask - 1)) == 0) {
                        // Power of 2 — find bit number
                        for (int b = 0; b < 8; b++) {
                            if (mask == (1 << b)) { bitN = b; break; }
                        }
                    }
                    // (a __zp byte not loaded: zpBitAlias_)
                    if (bitN >= 0 && inst.src1.isVreg() && zpBitAlias_.count(inst.src1.vregId)) {
                        lastCmp_ = {ir::Op::CMP_NE, inst.dest.vregId, true};
                        lastCmp_.bbsZpAddr = zpBitAlias_[inst.src1.vregId];
                        lastCmp_.bbsBitN = bitN;
                        break;
                    }
                    if (bitN >= 0 && inst.src1.isVreg()) {
                        auto srcAlloc = alloc_.getAlloc(inst.src1.vregId);
                        if (srcAlloc.loc == VRegAllocator::IN_ZP) {
                            // BBS/BBR: test bit directly in ZP, branch without touching registers
                            std::string zpAddr = "$" + hex8((uint8_t)srcAlloc.offset);
                            // Record for BR_COND to emit bbs/bbr
                            lastCmp_ = {ir::Op::CMP_NE, inst.dest.vregId, true};
                            lastCmp_.bbsZpAddr = zpAddr;
                            lastCmp_.bbsBitN = bitN;
                            break;
                        }
                    }
                }

                // General I8 bitwise op
                std::string src2 = src2MemOperand(inst.src2);
                loadOperandA(inst.src1);
                if (inst.op == ir::Op::AND) emit("and " + src2, irDesc());
                else if (inst.op == ir::Op::OR) emit("ora " + src2, irDesc());
                else emit("eor " + src2, irDesc());

                if (canFuse) {
                    // Record for BR_COND: treat as CMP_NE (nonzero = true)
                    lastCmp_ = {ir::Op::CMP_NE, inst.dest.vregId, true};
                } else {
                    if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
                    lastCmp_.valid = false;
                }
                break;
            }

            // an int's bytes known 0 on one side (#350: y << 9 | x << 2): the
            // result's byte is the other side's (and: 0), built in A/X
            if ((inst.resultType == ir::Type::I16 || inst.resultType == ir::Type::PTR) && inst.dest.isVreg()) {
                std::string byteCode[2];   // "" = an operation; "#0" / an operand = a plain load
                bool simpler = false, ok = true;
                for (int k = 0; k < 2 && ok; k++) {
                    bool za = kb_.byteZero(inst.src1, 16, k), zb = kb_.byteZero(inst.src2, 16, k);
                    std::string ba = directByte(inst.src1, k), bb = directByte(inst.src2, k);
                    if (inst.op == ir::Op::AND && (za || zb)) { byteCode[k] = "#0"; simpler = true; }
                    else if (inst.op != ir::Op::AND && za) { byteCode[k] = zb ? "#0" : bb; simpler = true; }
                    else if (inst.op != ir::Op::AND && zb) { byteCode[k] = ba; simpler = true; }
                    if (ba.empty() || bb.empty()) ok = false;
                }
                if (ok && simpler) {
                    const char* mn8 = inst.op == ir::Op::AND ? "and " : inst.op == ir::Op::OR ? "ora " : "eor ";
                    auto byteTo = [&](int k) {
                        if (!byteCode[k].empty()) { emit("lda " + byteCode[k], k ? irDesc("bytes known 0") : ""); return; }
                        emit("lda " + directByte(inst.src1, k), k ? irDesc("bytes known 0") : "");
                        emit(mn8 + directByte(inst.src2, k));
                    };
                    byteTo(1);
                    emit("tax");
                    byteTo(0);
                    ms_.invalidateAll();
                    storeVreg(inst.dest.vregId);
                    break;
                }
            }

            // Commutative: if src2 is in frame, swap to avoid scratch conflict
            bool src2InFrame = inst.src2.isVreg() &&
                alloc_.getAlloc(inst.src2.vregId).loc == VRegAllocator::IN_FRAME;
            bool src1InFrame = inst.src1.isVreg() &&
                alloc_.getAlloc(inst.src1.vregId).loc == VRegAllocator::IN_FRAME;

            std::string mn;
            if (inst.resultType == ir::Type::I32) {
                if (inst.op == ir::Op::AND) mn = "and.32";
                else if (inst.op == ir::Op::OR) mn = "ora.32";
                else mn = "eor.32";
            } else {
                if (inst.op == ir::Op::AND) mn = "and.16";
                else if (inst.op == ir::Op::OR) mn = "ora.16";
                else mn = "eor.16";
            }
            std::string reg = (inst.resultType == ir::Type::I32) ? ".AXYZ" : ".AX";

            if (src2InFrame && !src1InFrame) {
                std::string src1mem = src2MemOperand(inst.src1);
                loadOperand(inst.src2);
                emit(mn + " " + reg + ", " + src1mem);
            } else {
                std::string src2mem = src2MemOperand(inst.src2);
                loadOperand(inst.src1);
                emit(mn + " " + reg + ", " + src2mem);
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::NEG: {
            if (inst.resultType == ir::Type::I8) {
                loadOperandA(inst.src1);
                emit("neg a", irDesc("neg8"));
                if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
                break;
            }
            loadOperand(inst.src1);
            emit(inst.resultType == ir::Type::I32 ? "neg.32 .AXYZ" : "neg.16 .AX");
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::NOT: {
            if (inst.resultType == ir::Type::I8) {
                loadOperandA(inst.src1);
                emit("eor #$FF", irDesc("not8"));
                if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
                break;
            }
            loadOperand(inst.src1);
            emit(inst.resultType == ir::Type::I32 ? "not.32 .AXYZ" : "not.16 .AX");
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::SHL: {
            bool is32 = (inst.resultType == ir::Type::I32);
            std::string shOp = is32 ? "lsl.32 .AXYZ" : "lsl.16 .AX";
            int shiftCount = -1;
            if (inst.src2.isImm()) {
                shiftCount = (int)(inst.src2.immVal & 0xFF);
            }
            loadOperand(inst.src1);
            // a 32-bit value's bytes moved up 8, 16 or 24 bits
            auto shl32Bytes = [&](int k) {
                if (k == 8) {
                    // Shift bytes up: Z=Y, Y=X, X=A, A=0
                    emit("pha"); emit("tya"); emit("taz"); emit("txa"); emit("tay"); emit("pla"); emit("tax"); emit("lda #0");
                } else if (k == 16) {
                    // A→Y, X→Z, clear A,X
                    emit("tay"); emit("txa"); emit("taz"); emit("lda #0"); emit("ldx #0");
                } else {
                    // A→Z, clear A,X,Y
                    emit("taz"); emit("lda #0"); emit("ldx #0"); emit("ldy #0");
                }
            };
            if (shiftCount >= 0 && inst.resultType == ir::Type::I8) {
                // a byte: A alone
                if (shiftCount >= 8) emit("lda #0");
                else for (int i = 0; i < shiftCount; i++) emit("asl", i ? "" : "<< " + std::to_string(shiftCount));
                emit("ldx #0");
            } else if (shiftCount >= 0) {
                if (shiftCount == 8 && !is32) {
                    emit("tax"); emit("lda #0");
                } else if (is32 && (shiftCount == 8 || shiftCount == 16 || shiftCount == 24)) {
                    shl32Bytes(shiftCount);
                } else if (is32 && shiftCount > 8 && shiftCount < 32) {
                    // the bytes up, then the bits: after 24 only Z has any
                    // (shifted alone in A: asl, 1 byte each)
                    int k = shiftCount & ~7, r = shiftCount & 7;
                    if (k == 24) {
                        // (A, byte 0, becomes byte 3)
                        for (int i = 0; i < r; i++) emit("asl", i ? "" : "<< " + std::to_string(shiftCount));
                        emit("taz"); emit("lda #0"); emit("ldx #0"); emit("ldy #0");
                    } else {
                        shl32Bytes(k);
                        for (int i = 0; i < r; i++) emit(shOp);
                    }
                } else if (shiftCount >= (is32 ? 32 : 16)) {
                    // Shift entirely out — result is 0
                    emit("lda #0"); emit("ldx #0");
                    if (is32) { emit("ldy #0"); emit("ldz #0"); }
                } else if (!is32 && shiftCount >= 2) {
                    // the high byte shifted in a scratch byte (no pha/pla); a
                    // zero-extended byte's high byte is 0
                    bool xZero = (inst.src1.isVreg() && zextAlias_.count(inst.src1.vregId) &&
                                  zextAlias_[inst.src1.vregId].type == ir::Type::I8) ||
                                 kb_.byteZero(inst.src1, 16, 1);   // (known bits, #350)
                    emitMulAXConst(1L << shiftCount, xZero, "<< " + std::to_string(shiftCount));
                } else {
                    int maxBits = is32 ? 32 : 16;
                    for (int i = 0; i < shiftCount && i < maxBits; i++) emit(shOp);
                }
            } else {
                // The count first (it may be loaded through A/X), then the
                // value; the count is tested in memory (inc/dec)
                std::string cntOp = src2MemOperand(inst.src2);
                std::string lbl = "@__shl_loop_" + std::to_string(labelCounter_++);
                std::string done = "@__shl_done_" + std::to_string(labelCounter_++);
                emit("lda " + cntOp);
                emit("sta __zp_scratch4");
                loadOperand(inst.src1);
                emit("inc __zp_scratch4");
                emit("dec __zp_scratch4");
                emit("beq " + done);
                emitLabel(lbl);
                emit(shOp);
                emit("dec __zp_scratch4");
                emit("bne " + lbl);
                emitLabel(done);
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::SHR:
        case ir::Op::ASR: {
            bool is32 = (inst.resultType == ir::Type::I32);
            std::string mn;
            if (is32) mn = (inst.op == ir::Op::ASR) ? "asr.32 .AXYZ" : "lsr.32 .AXYZ";
            else mn = (inst.op == ir::Op::ASR) ? "asr.16 .AX" : "lsr.16 .AX";
            int shiftCount = -1;
            if (inst.src2.isImm()) {
                shiftCount = (int)(inst.src2.immVal & 0xFF);
            }
            loadOperand(inst.src1);
            // a 32-bit value's bytes moved down 8, 16 or 24 bits (signed:
            // the top bytes the sign)
            auto shr32Bytes = [&](int k, bool signedShift) {
                if (k == 8 && !signedShift) {
                    // I32 >> 8 unsigned: A=X, X=Y, Y=Z, Z=0
                    emit("txa"); emit("pha"); emit("tya"); emit("tax");
                    emit("tza"); emit("tay"); emit("ldz #0"); emit("pla");
                } else if (k == 8) {
                    // I32 >> 8 signed: A=X, X=Y, Y=Z, Z=sign_ext(Z)
                    emit("txa"); emit("pha"); emit("tya"); emit("tax");
                    emit("tza"); emit("tay");             // Y = old Z (byte 3)
                    emit("cmp #$80"); emit("lda #0"); emit("adc #$FF"); emit("eor #$FF"); // sign extend: $FF if neg
                    emit("taz"); emit("pla");             // Z = sign, A = old X
                } else if (k == 16 && !signedShift) {
                    // I32 >> 16 unsigned: A=Y, X=Z, Y=Z=0
                    emit("tza"); emit("tax");
                    emit("tya");
                    emit("ldy #0"); emit("ldz #0");
                } else if (k == 16) {
                    // I32 >> 16 signed: A=Y, X=Z, Y=Z=sign_ext(Z)
                    emit("tya"); emit("pha");             // save Y (new byte 0)
                    emit("tza"); emit("tax");             // X = old Z (new byte 1)
                    emit("cmp #$80"); emit("lda #0"); emit("adc #$FF"); emit("eor #$FF"); // sign extend: $FF if neg
                    emit("tay"); emit("taz");             // Y=Z=sign
                    emit("pla");                          // A = old Y (byte 0)
                } else if (k == 24 && !signedShift) {
                    // I32 >> 24 unsigned: A=Z, X=Y=Z=0
                    emit("tza"); emit("ldx #0"); emit("ldy #0"); emit("ldz #0");
                } else {
                    // I32 >> 24 signed: A=Z, X=Y=Z=sign_ext(Z)
                    emit("tza"); emit("cmp #$80"); emit("pha"); // save Z, set sign
                    emit("lda #0"); emit("adc #$FF"); emit("eor #$FF"); // A = sign ($FF if neg)
                    emit("tax"); emit("tay"); emit("taz"); // X=Y=Z=sign
                    emit("pla");                           // A = old Z (byte 0)
                }
            };
            if (shiftCount >= 0 && inst.resultType == ir::Type::I8 && inst.op == ir::Op::SHR) {
                // an unsigned byte: A alone
                if (shiftCount >= 8) emit("lda #0");
                else for (int i = 0; i < shiftCount; i++) emit("lsr", i ? "" : ">> " + std::to_string(shiftCount));
                emit("ldx #0");
            } else if (shiftCount >= 0) {
                bool isASR = (inst.op == ir::Op::ASR);
                // (an arithmetic shift by width - 1 is the sign fill too)
                if (shiftCount >= (is32 ? 32 : 16) || (isASR && shiftCount == (is32 ? 31 : 15))) {
                    // Shift entirely out — result is 0 (SHR) or 0/-1 (ASR)
                    if (isASR) {
                        if (is32) emit("tza"); else emit("txa"); // sign byte → A
                        emit("cmp #$80"); emit("lda #0"); emit("adc #$FF"); emit("eor #$FF"); // $FF if neg (C set), $00 if pos
                        emit("tax");
                        if (is32) { emit("tay"); emit("taz"); }
                    } else {
                        emit("lda #0"); emit("ldx #0");
                        if (is32) { emit("ldy #0"); emit("ldz #0"); }
                    }
                } else if (shiftCount == 8 && !is32) {
                    // I16 >> 8: high byte → low byte
                    emit("txa");
                    emit(isASR ? "sxt.8" : "ldx #0");
                } else if (is32 && (shiftCount == 8 || shiftCount == 16 || shiftCount == 24)) {
                    shr32Bytes(shiftCount, isASR);
                } else if (is32 && shiftCount > 8 && shiftCount < 32) {
                    // the bytes down, then the bits: after 24 only A has
                    // any (the others 0, or the sign: lsr / asr, 1 byte each)
                    int k = shiftCount & ~7, r = shiftCount & 7;
                    shr32Bytes(k, isASR);
                    for (int i = 0; i < r; i++) emit(k == 24 ? (isASR ? "asr" : "lsr") : mn);
                } else if (!is32 && shiftCount >= 2 && shiftCount < 8) {
                    // the high byte shifted in a scratch byte (lsr / the
                    // 45GS02's asr on base page), the low byte in A: no pha/pla
                    const std::string S = "__zp_scratch4+1";
                    emit("stx " + S, ">> " + std::to_string(shiftCount));
                    for (int i = 0; i < shiftCount; i++) {
                        emit((isASR ? "asr " : "lsr ") + S);
                        emit("ror");
                    }
                    emit("ldx " + S);
                } else if (!is32 && shiftCount > 8 && shiftCount < 16) {
                    // the high byte down, then the low byte alone
                    emit("txa", ">> " + std::to_string(shiftCount));
                    for (int i = 8; i < shiftCount; i++) emit(isASR ? "asr" : "lsr");
                    emit("ldx #0");
                    if (isASR) {   // the high byte: the sign (A kept its top bit)
                        std::string l = "@__shr_pos_" + std::to_string(labelCounter_++);
                        emit("cmp #$80"); emit("bcc " + l); emit("dex"); emitLabel(l);
                    }
                } else {
                    // General case: loop of single-bit shifts
                    int maxBits = is32 ? 32 : 16;
                    for (int i = 0; i < shiftCount && i < maxBits; i++) emit(mn);
                }
            } else {
                // The count first (loading it may use A/X), kept in zero
                // page; then the value. inc/dec test the count without
                // touching the value's registers.
                std::string cntOp = src2MemOperand(inst.src2);
                std::string lbl = "@__shr_loop_" + std::to_string(labelCounter_++);
                std::string done = "@__shr_done_" + std::to_string(labelCounter_++);
                emit("lda " + cntOp);
                emit("sta __zp_scratch4");
                loadOperand(inst.src1);
                emit("inc __zp_scratch4");
                emit("dec __zp_scratch4");
                emit("beq " + done);
                emitLabel(lbl);
                emit(mn);
                emit("dec __zp_scratch4");
                emit("bne " + lbl);
                emitLabel(done);
            }

            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::MUL:
        case ir::Op::MUL_U: {
            // (a factor from a suppressed CONST: its value, as an immediate)
            ir::Inst mi = inst;
            for (ir::Operand* o : {&mi.src1, &mi.src2})
                if (o->isVreg() && suppressedVregs_.count(o->vregId) && vregConstVal_.count(o->vregId) &&
                    !localSlotVregs_.count(o->vregId))
                    *o = ir::Operand::imm(vregConstVal_[o->vregId], o->type);
            // a byte times a constant (#352: x % c as x - (x / c) * c): in A,
            // shifts and the adds of the bits below the top one (Horner)
            if (inst.resultType == ir::Type::I8 && (mi.src2.isImm() || mi.src1.isImm())) {
                const ir::Operand& f = mi.src2.isImm() ? mi.src1 : mi.src2;
                int c = (int)((mi.src2.isImm() ? mi.src2.immVal : mi.src1.immVal) & 0xFF);
                if (c == 0) {
                    emit("lda #0", irDesc("a byte * 0"));
                } else {
                    int top = 7;
                    while (!((c >> top) & 1)) top--;
                    loadOperandA(f);
                    bool adds = (c & ((1 << top) - 1)) != 0;
                    if (adds) emit("sta __zp_scratch4", irDesc("a byte * " + std::to_string(c)));
                    for (int b = top - 1; b >= 0; b--) {
                        emit("asl", adds ? "" : irDesc("a byte * " + std::to_string(c)));
                        if ((c >> b) & 1) { emit("clc"); emit("adc __zp_scratch4"); }
                    }
                }
                ms_.invalidateAll();
                if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
                break;
            }
            // A call of __mul16 / __mul32 (divmod.s45; the multiplier in
            // __zp_scratch2; __mul16y: a byte factor in Y) instead of the
            // inline multiply (22 / 50 bytes):
            // -fmul-call / -Os everywhere, -fno-mul-call nowhere
            // (CC45_MUL_CALL 1 / 0); by default outside loops (#317), where a
            // constant factor is shifts and adds only when they are no bigger
            // than the call. As for division: not without a library, nor for
            // a 32-bit multiplicand in a stack frame
            const char* mulCallEnv = getenv("CC45_MUL_CALL");
            bool mulCallForced = mulCallEnv && mulCallEnv[0] != '0';
            bool mulCallOk = relocMode_ &&
                (inst.resultType == ir::Type::I16 || inst.resultType == ir::Type::PTR || inst.resultType == ir::Type::I32) &&
                !(inst.resultType == ir::Type::I32 && mi.src1.isVreg() &&
                  alloc_.getAlloc(mi.src1.vregId).loc == VRegAllocator::IN_FRAME && !currentFunctionUseSAC_);
            bool mulCallHere = mulCallOk && !mulCallEnv && !loopBlocks_.count(currentBlockIdx_);
            bool constFactor = inst.resultType != ir::Type::I32 && (mi.src2.isImm() || mi.src1.isImm());
            if (mulCallOk && (mulCallForced || (mulCallHere && !constFactor))) {
                bool is32 = inst.resultType == ir::Type::I32;
                std::string src2 = src2MemOperand(mi.src2);
                int nb = is32 ? 4 : 2;
                if (src2 != "__zp_scratch2") {
                    long long iv = 0;
                    bool imm = src2[0] == '#';
                    if (imm) iv = std::stoll(src2.substr(1));
                    for (int k = 0; k < nb; k++) {
                        emit(imm ? "lda #" + std::to_string((int)((iv >> (8 * k)) & 0xFF))
                                 : "lda " + src2 + (k ? "+" + std::to_string(k) : std::string()));
                        emit("sta __zp_scratch2" + (k ? "+" + std::to_string(k) : std::string()));
                    }
                }
                loadOperand(mi.src1);
                std::string fnName = is32 ? "__mul32" : "__mul16";
                emit(".extern " + fnName);
                emit("jsr " + fnName);
                ms_.invalidateAll();
                if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
                break;
            }
            // Signed or not: the low 16 (32) bits of a product do not depend
            // on the operands' signs, so the unsigned multiply (the hardware
            // multiplier, 22 bytes) gives them; mul.s16 (89 bytes) negated
            // around it
            // A constant factor (16 bits): shifts and adds when cheaper
            if (constFactor) {
                const ir::Operand& v = mi.src2.isImm() ? mi.src1 : mi.src2;
                long c = (long)(mi.src2.isImm() ? mi.src2.immVal : mi.src1.immVal);
                if (!v.isImm()) {
                    bool byteVal = v.isVreg() && alloc_.getAlloc(v.vregId).type == ir::Type::I8;
                    loadOperand(v);
                    if (byteVal) emit("ldx #0");
                    bool byteFactor = !(c & 0xFF00);
                    if (!emitMulAXConst(c, byteVal, "* " + std::to_string(c), mulCallHere ? (byteFactor ? 5 : 11) : 22)) {
                        if (mulCallHere && byteFactor) {
                            emit("ldy #" + std::to_string((int)(c & 0xFF)), "* " + std::to_string(c));
                            emit(".extern __mul16y");
                            emit("jsr __mul16y");
                            ms_.invalidateAll();
                        } else if (mulCallHere) {
                            // (Y carries the factor: A/X hold the multiplicand)
                            emit("ldy #" + std::to_string((int)(c & 0xFF)), "* " + std::to_string(c));
                            emit("sty __zp_scratch2");
                            emit("ldy #" + std::to_string((int)((c >> 8) & 0xFF)));
                            emit("sty __zp_scratch2+1");
                            emit(".extern __mul16");
                            emit("jsr __mul16");
                            ms_.invalidateAll();
                        } else {
                            emit("mul.16 .AX, #" + std::to_string(c & 0xFFFF));
                        }
                    }
                    if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
                    break;
                }
            }
            std::string src2 = src2MemOperand(mi.src2);
            loadOperand(mi.src1);
            if (inst.resultType == ir::Type::I32) {
                emit("mul.32 .AXYZ, " + src2);
            } else {
                emit("mul.16 .AX, " + src2);
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::DIV:
        case ir::Op::DIV_U:
        case ir::Op::MOD:
        case ir::Op::MOD_U: {
            bool is32 = inst.resultType == ir::Type::I32;
            bool sg = inst.op == ir::Op::DIV || inst.op == ir::Op::MOD;
            bool div = inst.op == ir::Op::DIV || inst.op == ir::Op::DIV_U;
            bool frame32early = is32 && inst.src1.isVreg() &&
                                alloc_.getAlloc(inst.src1.vregId).loc == VRegAllocator::IN_FRAME && !currentFunctionUseSAC_;
            if (frame32early) {
                // The dividend's frame load (ldaxyz.fp) uses __zp_scratch2/3:
                // the divisor waits in the dedicated 32-bit temporary
                loadOperand(inst.src2);
                emit("stq $" + hex8((uint8_t)ZP_TEMP32), "divisor (frame dividend)");
                loadOperand(inst.src1);
                std::string mn = std::string(div ? "div" : "mod") + (sg ? ".s32" : ".32") + " .AXYZ, $" + hex8((uint8_t)ZP_TEMP32);
                emit(mn);
                ms_.invalidateAll();
                if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
                break;
            }
            std::string src2 = src2MemOperand(inst.src2);
            // A call of the library routine (lib/stdlib_asm/divmod.s45): the
            // divisor in __zp_scratch2, the dividend in A/X(/Y/Z). Inline
            // (ca45's simulated instruction, 43-222 bytes) without a library
            // (standalone -S) and for a 32-bit dividend in a stack frame
            // (its load uses __zp_scratch2)
            bool frame32 = is32 && inst.src1.isVreg() &&
                           alloc_.getAlloc(inst.src1.vregId).loc == VRegAllocator::IN_FRAME && !currentFunctionUseSAC_;
            // both operands bytes (their high bytes known 0, #352: k / 3,
            // k % 3 of an unsigned char): the 8-bit routine, quotient in A,
            // remainder in X; the result's high byte 0
            if (relocMode_ && !is32 && !sg && kb_.byteZero(inst.src1, 16, 1) &&
                ((inst.src2.isImm() && inst.src2.immVal > 0 && inst.src2.immVal < 256) ||
                 (!inst.src2.isImm() && kb_.byteZero(inst.src2, 16, 1))) &&
                !directByte(inst.src1, 0).empty() && !directByte(inst.src2, 0).empty()) {
                emit("lda " + directByte(inst.src2, 0), irDesc(div ? "a byte / a byte" : "a byte % a byte"));
                emit("sta __zp_scratch2");
                emit("lda " + directByte(inst.src1, 0));
                emit(".extern __divmodu8");
                emit("jsr __divmodu8");
                if (!div) emit("txa");
                emit("ldx #0");
                ms_.invalidateAll();
                if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
                break;
            }
            if (relocMode_ && !frame32) {
                int nb = is32 ? 4 : 2;
                if (src2 != "__zp_scratch2") {
                    long long iv = 0;
                    bool imm = src2[0] == '#';
                    if (imm) iv = std::stoll(src2.substr(1));
                    for (int k = 0; k < nb; k++) {
                        emit(imm ? "lda #" + std::to_string((int)((iv >> (8 * k)) & 0xFF))
                                 : "lda " + src2 + (k ? "+" + std::to_string(k) : std::string()));
                        emit("sta __zp_scratch2" + (k ? "+" + std::to_string(k) : std::string()));
                    }
                }
                loadOperand(inst.src1);
                if (!is32 && inst.src1.isVreg() && alloc_.getAlloc(inst.src1.vregId).type == ir::Type::I8) emit("ldx #0");
                std::string fnName = std::string("__") + (div ? "div" : "mod") + (sg ? "s" : "u") + (is32 ? "32" : "16");
                emit(".extern " + fnName);
                emit("jsr " + fnName);
                ms_.invalidateAll();
            } else {
                loadOperand(inst.src1);
                std::string mn = std::string(div ? "div" : "mod") + (sg ? ".s" : ".") + (is32 ? "32 .AXYZ, " : "16 .AX, ");
                emit(mn + src2);
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::CMP_EQ:
        case ir::Op::CMP_NE:
        case ir::Op::CMP_LT:
        case ir::Op::CMP_LE:
        case ir::Op::CMP_GT:
        case ir::Op::CMP_GE:
        case ir::Op::CMP_LTU:
        case ir::Op::CMP_LEU:
        case ir::Op::CMP_GTU:
        case ir::Op::CMP_GEU: {
            // Load src1, compare with src2, set result to 0 or 1
            // (cmpOp: the comparison the flags answer; <= / > against a
            // constant may become < / >= with the next constant)
            ir::Op cmpOp = inst.op;
            if (inst.src2.isVreg() && zextAlias_.count(inst.src2.vregId)) {
                // a zero-extended byte (not emitted): the high byte against 0
                std::string b = directByte(inst.src2, 0);
                loadOperand(inst.src1);
                if (inst.op == ir::Op::CMP_LT || inst.op == ir::Op::CMP_GE) {
                    emit("cmp " + b, irDesc("signed compare with a zero-extended byte: C = not less"));
                    emit("txa");
                    emit("sbc #0");
                    emit("bvs *+4");
                    emit("eor #$80");
                    emit("asl a");
                    ms_.invalidateAll();
                } else {
                    // as cmp.16: the high bytes decide unless equal
                    std::string l = "@__zx_" + std::to_string(labelCounter_++);
                    emit("cpx #0", irDesc("compare with a zero-extended byte"));
                    emit("bne " + l);
                    emit("cmp " + b);
                    emitLabel(l);
                }
            } else if (inst.src1.isVreg() && zextAlias_.count(inst.src1.vregId)) {
                // a zero-extended byte (not emitted) against a 16-bit value:
                // 0 against its high byte, then the byte against its low byte
                std::string b = directByte(inst.src1, 0);
                std::string lo = directByte(inst.src2, 0), hi = directByte(inst.src2, 1);
                if (lo.empty() || hi.empty()) {
                    lo = src2MemOperand(inst.src2);
                    hi = lo[0] == '#' ? "#" + std::to_string((std::stol(lo.substr(1)) >> 8) & 0xFF) : lo + "+1";
                    if (lo[0] == '#') lo = "#" + std::to_string(std::stol(lo.substr(1)) & 0xFF);
                }
                if (inst.op == ir::Op::CMP_LT || inst.op == ir::Op::CMP_GE) {
                    emit("lda " + b, irDesc("signed compare of a zero-extended byte: C = not less"));
                    emit("cmp " + lo);
                    emit("lda #0");
                    emit("sbc " + hi);
                    emit("bvs *+4");
                    emit("eor #$80");
                    emit("asl a");
                } else if (hi == "#0") {
                    emit("lda " + b, irDesc("compare a zero-extended byte"));
                    emit("cmp " + lo);
                } else {
                    std::string l = "@__zx_" + std::to_string(labelCounter_++);
                    emit("lda #0", irDesc("compare a zero-extended byte"));
                    emit("cmp " + hi);
                    emit("bne " + l);
                    emit("lda " + b);
                    emit("cmp " + lo);
                    emitLabel(l);
                }
                ms_.invalidateAll();
            } else {
            // Check if src2 is a vreg holding a known constant — use immediate directly
            std::string src2;
            bool src2IsConst = inst.src2.isImm();
            int64_t src2ConstVal = inst.src2.isImm() ? inst.src2.immVal : 0;
            if (inst.src2.isVreg() && currentFn_) {
                for (const auto& blk : currentFn_->blocks) {
                    for (const auto& ii : blk.insts) {
                        if (ii.op == ir::Op::CONST && ii.dest.isVreg() &&
                            ii.dest.vregId == inst.src2.vregId) {
                            src2IsConst = true;
                            src2ConstVal = ii.src1.immVal;
                        }
                    }
                }
            }
            if (src2IsConst) {
                src2 = "#" + std::to_string((int)src2ConstVal);
            } else {
                src2 = src2MemOperand(inst.src2);
            }
            bool isSigned = (inst.op == ir::Op::CMP_LT || inst.op == ir::Op::CMP_LE ||
                             inst.op == ir::Op::CMP_GT || inst.op == ir::Op::CMP_GE);

            // Optimization: CMP_EQ/CMP_NE with 0 → use ORA to test zero
            // For ZP-allocated I16: lda zp_lo; ora zp_hi (3 instructions, 5 bytes)
            // For other I16: loadOperand then stx scratch; ora scratch
            int zeroTestBytes = inst.src1.type == ir::Type::I32 ? 4 : inst.src1.type == ir::Type::I8 ? 1 : 2;
            bool directZeroTest = src2IsConst && src2ConstVal == 0 && zeroTestBytes > 1 &&
                (inst.op == ir::Op::CMP_EQ || inst.op == ir::Op::CMP_NE) && inst.src1.isVreg() &&
                !justStored(inst.src1.vregId);
            for (int k = 0; directZeroTest && k < zeroTestBytes; k++) {
                std::string b = directByte(inst.src1, k);
                directZeroTest = !b.empty() && b[0] != '#';
            }
            // x < 0 / x >= 0 (signed): only the sign decides; C = not negative
            // (the branch: bcc for <, bcs for >=)
            bool signTest = isSigned && src2IsConst && src2ConstVal == 0 &&
                            (inst.op == ir::Op::CMP_LT || inst.op == ir::Op::CMP_GE) &&
                            (inst.src1.type == ir::Type::I16 || inst.src1.type == ir::Type::I32) && !directZeroTest;
            if (signTest) {
                int top = inst.src1.type == ir::Type::I32 ? 3 : 1;
                std::string hb = inst.src1.isVreg() && !justStored(inst.src1.vregId) ? directByte(inst.src1, top) : "";
                if (!hb.empty() && hb[0] != '#') {
                    emit("lda " + hb, irDesc("sign test"));
                } else {
                    loadOperand(inst.src1);
                    emit(top == 3 ? "tza" : "txa", irDesc("sign test"));
                }
                emit("eor #$80");
                emit("asl a");
            } else if (directZeroTest) {
                // a value in memory is 0 when all its bytes OR to 0
                emit("lda " + directByte(inst.src1, 0));
                for (int k = 1; k < zeroTestBytes; k++) emit("ora " + directByte(inst.src1, k));
            } else if (src2IsConst && src2ConstVal == 0 &&
                (inst.op == ir::Op::CMP_EQ || inst.op == ir::Op::CMP_NE) &&
                inst.src1.type != ir::Type::I32) {
                if (inst.src1.type == ir::Type::I8) {
                    loadOperandA(inst.src1);   // (a byte: A only, no ldx #0, #341)
                    emit("cmp #0");
                } else if (inst.src1.isVreg() && globalLoadAlias_.count(inst.src1.vregId)) {
                    emit("lda " + directByte(inst.src1, 0));
                    emit("ora " + directByte(inst.src1, 1));
                } else if (inst.src1.isVreg()) {
                    auto alloc = alloc_.getAlloc(inst.src1.vregId);
                    if (alloc.loc == VRegAllocator::IN_ZP) {
                        std::string lo = "$" + hex8(alloc.offset);
                        std::string hi = "$" + hex8(alloc.offset + 1);
                        emit("lda " + lo);
                        emit("ora " + hi);
                    } else {
                        loadOperand(inst.src1);
                        emit("stx __zp_scratch");
                        emit("ora __zp_scratch");
                    }
                } else {
                    loadOperand(inst.src1);
                    emit("stx __zp_scratch");
                    emit("ora __zp_scratch");
                }
            } else {
                if (inst.src1.type == ir::Type::I8) {
                    loadOperandA(inst.src1); // A-only: preserves N/Z flags for peephole
                    emit("cmp " + src2);
                } else if (inst.src1.type == ir::Type::I32) {
                    loadOperand(inst.src1);
                    emit(std::string(isSigned ? "cmp.s32" : "cmp.32") + " .AXYZ, " + src2);
                } else {
                    loadOperand(inst.src1);
                    // Signed < / >= by subtraction: C = not less (N xor V of
                    // the high-byte subtraction, into C): 10 bytes, not
                    // cmp.s16's 13-19 (which also keeps Z for = / <= / >)
                    ir::Op op = inst.op;
                    long long c = (int16_t)(src2ConstVal & 0xFFFF);
                    if (isSigned && src2IsConst && (op == ir::Op::CMP_LE || op == ir::Op::CMP_GT) && c != 32767) {
                        c++;
                        op = op == ir::Op::CMP_LE ? ir::Op::CMP_LT : ir::Op::CMP_GE;
                    }
                    std::string lo, hi;
                    if (src2IsConst) {
                        lo = "#" + std::to_string((int)(c & 0xFF));
                        hi = "#" + std::to_string((int)((c >> 8) & 0xFF));
                    } else if (!src2.empty() && src2[0] != '#') {
                        lo = src2;
                        hi = src2 + "+1";
                    }
                    if (isSigned && (op == ir::Op::CMP_LT || op == ir::Op::CMP_GE) && !lo.empty()) {
                        emit("cmp " + lo, irDesc("signed compare: C = not less"));
                        emit("txa");
                        emit("sbc " + hi);
                        emit("bvs *+4");
                        emit("eor #$80");
                        emit("asl a");
                        ms_.invalidateAll();
                        cmpOp = op;
                    } else {
                        emit(std::string(isSigned ? "cmp.s16" : "cmp.16") + " .AX, " + src2);
                    }
                }
            }
            } // end src2/signed scope

            // Check if this CMP result is consumed solely by the next BR_COND.
            // If so, skip boolean materialization and let BR_COND use flags directly.
            bool canFuse = false;
            if (inst.dest.isVreg() && currentFn_) {
                // Peek at next instruction in current block
                auto& blk = currentFn_->blocks[currentBlockIdx_];
                size_t instIdx = &inst - &blk.insts[0];
                if (instIdx + 1 < blk.insts.size()) {
                    const auto& next = blk.insts[instIdx + 1];
                    if (next.op == ir::Op::BR_COND && next.src1.isVreg() &&
                        next.src1.vregId == inst.dest.vregId) {
                        canFuse = true;
                    }
                }
            }

            if (canFuse) {
                // Record for the following BR_COND to use
                lastCmp_ = {cmpOp, inst.dest.vregId, true};
                // Don't materialize boolean — BR_COND will branch on flags.
                // NOTE: For I16 CMP_EQ/CMP_NE against 0, the compare code above
                // (ora __zp_scratch) already set Z correctly. For other comparisons,
                // cmp.16/cmp.s16 set flags via the simop. Flags are live here.
            } else if (cmpOp == ir::Op::CMP_GE || cmpOp == ir::Op::CMP_GEU ||
                       cmpOp == ir::Op::CMP_LT || cmpOp == ir::Op::CMP_LTU) {
                // the result is C (C = not less): A = C, without branches
                lastCmp_.valid = false;
                emit("lda #0");
                emit("rol", "C (not less)");
                if (cmpOp == ir::Op::CMP_LT || cmpOp == ir::Op::CMP_LTU) emit("eor #1");
                emit("ldx #0");
                if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            } else if (cmpOp == ir::Op::CMP_EQ || cmpOp == ir::Op::CMP_NE) {
                // the result is Z (bit 1 of the status byte), without branches
                lastCmp_.valid = false;
                emit("php");
                emit("pla");
                emit("lsr", "Z (equal)");
                emit("and #1");
                if (cmpOp == ir::Op::CMP_NE) emit("eor #1");
                emit("ldx #0");
                ms_.invalidateAll();
                if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            } else {
                lastCmp_.valid = false;
                // Branch IMMEDIATELY (flags live), set result to 0 or 1
                std::string setLabel = "@__cmp_set_" + std::to_string(stringCount_);
                std::string zeroLabel = "@__cmp_zero_" + std::to_string(stringCount_);
                std::string doneLabel = "@__cmp_done_" + std::to_string(stringCount_++);
                switch (cmpOp) {
                    case ir::Op::CMP_EQ:
                        emit("beq " + setLabel);
                        emit("bra " + zeroLabel);
                        break;
                    case ir::Op::CMP_NE:
                        emit("bne " + setLabel);
                        emit("bra " + zeroLabel);
                        break;
                    case ir::Op::CMP_LT:
                    case ir::Op::CMP_LTU:
                        emit("bcc " + setLabel);
                        emit("bra " + zeroLabel);
                        break;
                    case ir::Op::CMP_LE:
                    case ir::Op::CMP_LEU:
                        emit("bcc " + setLabel);
                        emit("beq " + setLabel);
                        emit("bra " + zeroLabel);
                        break;
                    case ir::Op::CMP_GT:
                    case ir::Op::CMP_GTU:
                        emit("beq " + zeroLabel);
                        emit("bcs " + setLabel);
                        emit("bra " + zeroLabel);
                        break;
                    case ir::Op::CMP_GE:
                    case ir::Op::CMP_GEU:
                        emit("bcs " + setLabel);
                        emit("bra " + zeroLabel);
                        break;
                    default:
                        emit("beq " + setLabel);
                        emit("bra " + zeroLabel);
                        break;
                }
                emitLabel(setLabel);
                emit("lda #1");
                emit("ldx #0");
                emit("bra " + doneLabel);
                emitLabel(zeroLabel);
                emit("lda #0");
                emit("ldx #0");
                emitLabel(doneLabel);
                if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            }
            break;
        }

        case ir::Op::LOAD: {
            // Fused ADDR_ELEM+LOAD: args carry base/index/stride from the
            // eliminated ADDR_ELEM. Compute address inline to __zp_scratch.
            if (inst.args.size() >= 3 && inst.resultType != ir::Type::F32) {
                // args[0]=base, args[1]=index, args[2]=stride
                auto& base = inst.args[0];
                auto& index = inst.args[1];
                int stride = (int)inst.args[2].immVal;

                // goto *table[i] (computed goto through a static label table
                // of at most 256 bytes): X = i * 2; jmp (table,x), no
                // self-modified jump. The BR_INDIRECT that follows is done.
                if (inst.resultType == ir::Type::PTR && stride == 2 && inst.dest.isVreg() &&
                    base.kind == ir::OperandKind::GLOBAL && smallGlobals_.count(base.name) && index.isVreg() &&
                    inst.elemOffset >= 0 && inst.elemOffset + 2 <= 256) {
                    const ir::Inst* nx = peekNextInst();
                    bool onlyUse = nx && nx->op == ir::Op::BR_INDIRECT && nx->args.empty() && nx->src1.isVreg() &&
                                   nx->src1.vregId == inst.dest.vregId;
                    if (onlyUse) {
                        int uses = 0;
                        for (const auto& b : currentFn_->blocks)
                            for (const auto& in : b.insts) {
                                for (const ir::Operand* o : {&in.src1, &in.src2})
                                    if (o->isVreg() && o->vregId == inst.dest.vregId) uses++;
                                for (const auto& a : in.args) if (a.isVreg() && a.vregId == inst.dest.vregId) uses++;
                            }
                        onlyUse = uses == 1;
                    }
                    if (onlyUse) {
                        loadOperand(index);
                        emit("asl");
                        emit("tax", "dispatch through " + base.name);
                        std::string op = inst.elemOffset ? base.name + "+" + std::to_string(inst.elemOffset) : base.name;
                        emit("jmp (" + op + ",x)");
                        skipBrIndirect_ = true;
                        elemCache_.valid = false;
                        ms_.invalidateAll();
                        break;
                    }
                }
                // a small global array: name+off,y
                if (inst.resultType == ir::Type::I8 || inst.resultType == ir::Type::I16 ||
                    inst.resultType == ir::Type::PTR) {
                    std::string op;
                    int es = inst.resultType == ir::Type::I8 ? 1 : 2;
                    if (smallArrayY(base, index, stride, inst.elemOffset, es, op)) {
                        if (es == 1) { emit("lda " + op + ",y"); emit("ldx #0"); }
                        else { emit("lda " + op + ",y"); emit("ldx " + op + "+1,y"); }
                        if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
                        ms_.invalidateAll();
                        break;
                    }
                }
                {
                    std::string zp;
                    if (inst.resultType == ir::Type::I8 && zpPtrByteIndex(base, index, stride, inst.elemOffset, zp)) {
                        std::string direct = directBase(index.vregId);
                        if (!direct.empty()) emit("ldy " + direct, "the index ((p),y)");
                        else { loadOperand(index); emit("tay", "the index ((p),y)"); }
                        emit("lda (" + zp + "),y");
                        emit("ldx #0");
                        if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
                        ms_.invalidateAll();
                        break;
                    }
                }
                std::string baseStr = (base.kind == ir::OperandKind::GLOBAL) ?
                    globalBase(base, inst.elemOffset) : src2MemOperand(base);
                // __zp_scratch may already point into the same array at the
                // same index (a[j] then a[j + 1]): only Y differs
                int yOff = 0;
                bool cacheable = base.kind == ir::OperandKind::GLOBAL && index.isVreg() &&
                                 inst.resultType != ir::Type::I32;
                int d = inst.elemOffset - elemCache_.offset;
                if (cacheable && elemCache_.valid && elemCache_.base == base.name &&
                    elemCache_.index == index.vregId && elemCache_.stride == stride && d >= 0 && d <= 250) {
                    yOff = d;
                    emitComment("address in __zp_scratch: " + base.name + " + " + std::to_string(elemCache_.offset) +
                                " + index*" + std::to_string(stride) + ", Y + " + std::to_string(d));
                } else {
                    emitArrayElemAddr(baseStr, index, stride, "__zp_scratch");
                    elemCache_.valid = cacheable;
                    elemCache_.base = base.name;
                    elemCache_.index = index.isVreg() ? index.vregId : 0;
                    elemCache_.stride = stride;
                    elemCache_.offset = inst.elemOffset;
                }
                if (inst.dest.isVreg() && index.isVreg() && inst.dest.vregId == index.vregId) elemCache_.valid = false;
                // Now load from (__zp_scratch),Y
                if (inst.resultType == ir::Type::I32) {
                    // Top byte first, so Y stays the index until the end:
                    // Z = b3, X = b1, A = b0, Y = b2
                    emit("ldy #3"); emit("lda (__zp_scratch),y"); emit("taz");
                    emit("dey");    emit("lda (__zp_scratch),y"); emit("pha");
                    emit("dey");    emit("lda (__zp_scratch),y"); emit("tax");
                    emit("dey");    emit("lda (__zp_scratch),y");
                    emit("ply");
                    if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
                    ms_.invalidateAll();
                    break;
                }
                if (inst.resultType != ir::Type::I8) {
                    // high byte first: no pha/pla
                    emit("ldy #" + std::to_string(yOff + 1)); emit("lda (__zp_scratch),y"); emit("tax");
                    emit("dey");    emit("lda (__zp_scratch),y");
                } else {
                    emit("ldy #" + std::to_string(yOff));
                    emit("lda (__zp_scratch),y");
                    emit("ldx #0");
                }
                if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
                ms_.invalidateAll();
                break;
            }

            if (inst.resultType == ir::Type::F32 && inst.dest.isVreg()) {
                // 5-byte float load: copy to dest vreg
                ms_.invalidateAll();
                if (inst.src1.kind == ir::OperandKind::GLOBAL) {
                    emitF32AbsToVreg(inst.src1.name, inst.dest.vregId);
                } else {
                    // Indirect load via (ZP),Y
                    std::string zpPair;
                    if (inst.src1.isVreg()) {
                        auto addrAlloc = alloc_.getAlloc(inst.src1.vregId);
                        if (addrAlloc.loc == VRegAllocator::IN_ZP)
                            zpPair = "$" + hex8((uint8_t)addrAlloc.offset);
                        else { loadVreg(inst.src1.vregId); emit("sta __zp_scratch"); emit("stx __zp_scratch+1"); zpPair = "__zp_scratch"; }
                    } else { loadOperand(inst.src1); emit("sta __zp_scratch"); emit("stx __zp_scratch+1"); zpPair = "__zp_scratch"; }
                    for (int i = 0; i < 5; i++) {
                        emit("ldy #" + std::to_string(i));
                        emit("lda (" + zpPair + "),y");
                        emitF32Byte("sta", inst.dest.vregId, i);
                    }
                }
                break;
            }
            // A bit of a __zp byte (a __zp __bit, #73; __bit(z, n)): set or
            // cleared in place (load, or/and one bit, store back: smb / rmb),
            // or tested by a branch (load, and one bit, br_cond: bbs on the
            // variable itself, not a copy)
            // (a byte elsewhere, or bits of a mask: lda #m / tsb or trb)
            if (inst.src1.kind == ir::OperandKind::GLOBAL && inst.resultType == ir::Type::I8 && inst.dest.isVreg() &&
                inst.args.empty() && currentFn_ && inst.src1.name.find("__param_") == std::string::npos &&
                (zpPlace_.count(inst.src1.name) ||
                 (symbolAddressSpace_.count(inst.src1.name) && symbolAddressSpace_[inst.src1.name] == 0))) {
                bool zp = zpPlace_.count(inst.src1.name) > 0;
                const auto& blk = currentFn_->blocks[currentBlockIdx_];
                size_t at = currentInstInBlock_;
                auto after = [&](size_t j) {
                    for (++j; j < blk.insts.size() && blk.insts[j].op == ir::Op::NOP; ++j) {}
                    return j;
                };
                auto uses = [&](uint32_t v) {
                    int n = 0;
                    for (const auto& b : currentFn_->blocks)
                        for (const auto& in : b.insts) {
                            for (const ir::Operand* o : {&in.src1, &in.src2}) if (o->isVreg() && o->vregId == v) n++;
                            for (const auto& o : in.args) if (o.isVreg() && o.vregId == v) n++;
                        }
                    return n;
                };
                auto bitOf = [](int64_t m) {
                    m &= 0xFF;
                    for (int b = 0; b < 8; b++) if (m == (1 << b)) return b;
                    return -1;
                };
                // every use a one-bit test of a branch, the byte left alone by
                // the function (no store, address, call or asm: an interrupt
                // handler cannot use a __zp variable): no load, bbs on the byte
                if (zp) {
                    const std::string& gname = inst.src1.name;
                    bool alone = true;
                    for (const auto& bb : currentFn_->blocks)
                        for (const auto& in : bb.insts) {
                            if ((in.op == ir::Op::STORE && in.src2.kind == ir::OperandKind::GLOBAL && in.src2.name == gname) ||
                                (in.op == ir::Op::ADDR_GLOBAL && in.src1.kind == ir::OperandKind::GLOBAL && in.src1.name == gname) ||
                                in.op == ir::Op::CALL || in.op == ir::Op::CALL_VOID || in.op == ir::Op::CALL_INDIRECT ||
                                in.op == ir::Op::ASM_INLINE)
                                alone = false;
                        }
                    int tests = 0;
                    bool onlyTests = alone;
                    for (const auto& bb : currentFn_->blocks)
                        for (size_t k = 0; onlyTests && k < bb.insts.size(); k++) {
                            const auto& in = bb.insts[k];
                            bool reads = false;
                            for (const ir::Operand* o : {&in.src1, &in.src2}) if (o->isVreg() && o->vregId == inst.dest.vregId) reads = true;
                            for (const auto& o : in.args) if (o.isVreg() && o.vregId == inst.dest.vregId) reads = true;
                            if (!reads) continue;
                            bool test = in.op == ir::Op::AND && in.resultType == ir::Type::I8 && in.src1.isVreg() &&
                                        in.src1.vregId == inst.dest.vregId && in.src2.isImm() && bitOf(in.src2.immVal) >= 0 &&
                                        in.dest.isVreg() && k + 1 < bb.insts.size() && bb.insts[k + 1].op == ir::Op::BR_COND &&
                                        bb.insts[k + 1].src1.isVreg() && bb.insts[k + 1].src1.vregId == in.dest.vregId &&
                                        uses(in.dest.vregId) == 1;
                            if (!test) onlyTests = false;
                            else tests++;
                        }
                    if (onlyTests && tests > 0) {
                        zpBitAlias_[inst.dest.vregId] = gname;
                        break;
                    }
                }
                size_t ia = after(at), ib = after(ia);
                const ir::Inst* a = ia < blk.insts.size() ? &blk.insts[ia] : nullptr;
                const ir::Inst* b = ib < blk.insts.size() ? &blk.insts[ib] : nullptr;
                if (a && b && (a->op == ir::Op::OR || a->op == ir::Op::AND) && a->resultType == ir::Type::I8 &&
                    a->src1.isVreg() && a->src1.vregId == inst.dest.vregId && a->src2.isImm() && a->dest.isVreg() &&
                    uses(inst.dest.vregId) == 1) {
                    const std::string& g = inst.src1.name;
                    int setBit = a->op == ir::Op::OR ? bitOf(a->src2.immVal) : -1;
                    int clrBit = a->op == ir::Op::AND ? bitOf(~a->src2.immVal) : -1;
                    int testBit = a->op == ir::Op::AND ? bitOf(a->src2.immVal) : -1;
                    bool storeBack = b->op == ir::Op::STORE && b->src1.isVreg() &&
                        b->src1.vregId == a->dest.vregId && b->src2.kind == ir::OperandKind::GLOBAL &&
                        b->src2.name == g && b->resultType == ir::Type::I8 && b->args.empty() &&
                        uses(a->dest.vregId) == 1;
                    if (storeBack && zp && (setBit >= 0 || clrBit >= 0)) {
                        emit((setBit >= 0 ? "smb" + std::to_string(setBit) : "rmb" + std::to_string(clrBit)) + " " + g,
                             irDesc(setBit >= 0 ? "set a bit in place" : "clear a bit in place"));
                        ms_.invalidateAll();
                        skipInsts_ = (int)(ib - at);
                        break;
                    }
                    if (storeBack) {
                        int64_t m = (a->op == ir::Op::OR ? a->src2.immVal : ~a->src2.immVal) & 0xFF;
                        emit("lda #" + std::to_string((int)m), irDesc(a->op == ir::Op::OR ? "set bits in place" : "clear bits in place"));
                        emit((a->op == ir::Op::OR ? "tsb " : "trb ") + g);
                        ms_.invalidateAll();
                        skipInsts_ = (int)(ib - at);
                        break;
                    }
                    if (zp && testBit >= 0 && b->op == ir::Op::BR_COND && b->src1.isVreg() && b->src1.vregId == a->dest.vregId &&
                        uses(a->dest.vregId) == 1) {
                        lastCmp_ = {ir::Op::CMP_NE, a->dest.vregId, true};
                        lastCmp_.bbsZpAddr = g;
                        lastCmp_.bbsBitN = testBit;
                        skipInsts_ = (int)(ia - at);   // (the branch is the next one)
                        break;
                    }
                }
            }
            // g += c on a global of this file (load, add a constant, store
            // back, the values used nowhere else): inc / dec or an add in
            // place, not through temporaries
            if (inst.src1.kind == ir::OperandKind::GLOBAL && inst.dest.isVreg() && inst.args.empty() &&
                (inst.resultType == ir::Type::I8 || inst.resultType == ir::Type::I16 ||
                 inst.resultType == ir::Type::I32) &&
                symbolAddressSpace_.count(inst.src1.name) && symbolAddressSpace_[inst.src1.name] == 0 &&
                inst.src1.name.find("__param_") == std::string::npos && currentFn_) {
                const auto& blk = currentFn_->blocks[currentBlockIdx_];
                size_t at = currentInstInBlock_;
                // the next instruction after j that is not a NOP
                auto after = [&](size_t j) {
                    for (++j; j < blk.insts.size() && blk.insts[j].op == ir::Op::NOP; ++j) {}
                    return j;
                };
                // (x++ copies the old value first: a deref/copy of it)
                size_t ic = after(at);
                const ir::Inst* cp = ic < blk.insts.size() ? &blk.insts[ic] : nullptr;
                bool hasCopy = cp && (cp->op == ir::Op::DEREF || cp->op == ir::Op::COPY) && cp->dest.isVreg() &&
                               cp->src1.isVreg() && cp->src1.vregId == inst.dest.vregId && cp->resultType == inst.resultType;
                size_t ia = hasCopy ? after(ic) : ic, ist = after(ia);
                size_t k = ia - at;
                uint32_t val = hasCopy ? cp->dest.vregId : inst.dest.vregId;
                const ir::Inst* a = ia < blk.insts.size() ? &blk.insts[ia] : nullptr;
                const ir::Inst* st = ist < blk.insts.size() ? &blk.insts[ist] : nullptr;
                long long c = 0;
                bool ok = a && st && (a->op == ir::Op::ADD || a->op == ir::Op::SUB) && a->resultType == inst.resultType &&
                          a->dest.isVreg() && a->src1.isVreg() && a->src1.vregId == val && a->src2.isImm() &&
                          st->op == ir::Op::STORE && st->src1.isVreg() && st->src1.vregId == a->dest.vregId &&
                          st->src2.kind == ir::OperandKind::GLOBAL && st->src2.name == inst.src1.name &&
                          st->resultType == inst.resultType && st->args.empty();
                if (ok) {
                    int uses = 0;
                    for (const auto& b : currentFn_->blocks)
                        for (const auto& in : b.insts) {
                            for (const ir::Operand* o : {&in.src1, &in.src2})
                                if (o->isVreg() && (o->vregId == inst.dest.vregId || o->vregId == val ||
                                                    o->vregId == a->dest.vregId)) uses++;
                            for (const auto& o : in.args)
                                if (o.isVreg() && (o.vregId == inst.dest.vregId || o.vregId == val ||
                                                   o.vregId == a->dest.vregId)) uses++;
                        }
                    ok = uses == (hasCopy ? 3 : 2);
                }
                bool w32 = inst.resultType == ir::Type::I32;
                if (ok && w32) {
                    // a long: += 1 / -= 1 only (inq / deq), others the usual way
                    c = (a->op == ir::Op::ADD ? a->src2.immVal : -a->src2.immVal) & 0xFFFFFFFFLL;
                    ok = c == 1 || c == 0xFFFFFFFFLL;
                }
                if (ok && w32) {
                    emit((c == 1 ? "inq " : "deq ") + inst.src1.name, irDesc(c == 1 ? "+= 1 in place" : "-= 1 in place"));
                    ms_.invalidateAll();
                    skipInsts_ = (int)(ist - at);
                    break;
                }
                if (ok) {
                    bool w16 = inst.resultType == ir::Type::I16;
                    long long mask = w16 ? 0xFFFF : 0xFF;
                    c = (a->op == ir::Op::ADD ? a->src2.immVal : -a->src2.immVal) & mask;
                    const std::string& g = inst.src1.name;
                    if (c == 0) {
                        // nothing
                    } else if (c == 1 && !w16) {
                        emit("inc " + g, irDesc("+= 1 in place"));
                    } else if (c == mask && !w16) {
                        emit("dec " + g, irDesc("-= 1 in place"));
                    } else if (c == 1) {
                        std::string l = "@__inc_" + std::to_string(labelCounter_++);
                        emit("inc " + g, irDesc("+= 1 in place"));
                        emit("bne " + l); emit("inc " + g + "+1"); emitLabel(l);
                    } else if (c == mask) {
                        std::string l = "@__dec_" + std::to_string(labelCounter_++);
                        emit("lda " + g, irDesc("-= 1 in place"));
                        emit("bne " + l); emit("dec " + g + "+1"); emitLabel(l);
                        emit("dec " + g);
                    } else {
                        emit("lda " + g, irDesc("+= " + std::to_string(c) + " in place"));
                        emit("clc"); emit("adc #" + std::to_string((int)(c & 0xFF))); emit("sta " + g);
                        if (w16) {
                            emit("lda " + g + "+1"); emit("adc #" + std::to_string((int)((c >> 8) & 0xFF)));
                            emit("sta " + g + "+1");
                        }
                    }
                    ms_.invalidateAll();
                    skipInsts_ = (int)(ist - at);
                    break;
                }
            }
            // a long read by its one use, from the global (globalLoadAlias_)
            if (inst.dest.isVreg() && globalLoadAlias_.count(inst.dest.vregId)) break;
            if (inst.src1.kind == ir::OperandKind::GLOBAL) {
                // Phase 78: Check if this is a parameter using SMC optimization
                bool useSMC = false;
                std::string paramName = inst.src1.name;
                if (paramName.find("__param_") != std::string::npos && trackSMCOffsetsEnabled_) {
                    // Check if this parameter is marked for SMC in metadata
                    auto metaIt = functionSMCMetadata_.find(currentFunctionName_);
                    if (metaIt != functionSMCMetadata_.end()) {
                        // Check each parameter in the metadata
                        for (const auto& [paramID, param] : metaIt->second) {
                            if (param.symbolName == paramName && param.useSMC) {
                                useSMC = true;
                                break;
                            }
                        }
                    }
                }

                // Load directly from global address (or use SMC placeholder)
                if (useSMC) {
                    // Emit placeholder immediates for SMC parameter optimization
                    // The assembler will collect these as immediate relocations
                    emit("lda #$00");
                    if (inst.resultType == ir::Type::I32) {
                        emit("ldx #$00");
                        emit("ldy #$00");
                        emit("ldz #$00");
                    } else if (inst.resultType != ir::Type::I8) {
                        emit("ldx #$00");
                    } else {
                        emit("ldx #0");
                    }
                } else {
                    // Normal load from parameter storage
                    // Phase 97.5: Apply .zp suffix for __zp globals
                    emitOptimized("lda " + inst.src1.name);
                    if (inst.resultType == ir::Type::I32) {
                        emitOptimized("ldx " + inst.src1.name + "+1");
                        emitOptimized("ldy " + inst.src1.name + "+2");
                        emitOptimized("ldz " + inst.src1.name + "+3");
                    } else if (inst.resultType != ir::Type::I8) {
                        emitOptimized("ldx " + inst.src1.name + "+1");
                    } else {
                        emit("ldx #0");
                    }
                }
            } else if (inst.src1.isVreg() && vregConstVal_.count(inst.src1.vregId) &&
                       !localSlotVregs_.count(inst.src1.vregId)) {
                // A constant address (*(volatile char *)0xD012): absolute
                // loads, as a store to one is (four hex digits keep an
                // address below $100 absolute: not the program's base page)
                std::string addr = "$" + hex16((uint16_t)vregConstVal_[inst.src1.vregId]);
                ms_.invalidateAll();
                if (inst.resultType == ir::Type::I32) {
                    emit("ldz " + addr + "+3"); emit("ldy " + addr + "+2");
                    emit("ldx " + addr + "+1"); emit("lda " + addr);
                } else if (inst.resultType != ir::Type::I8) {
                    emit("ldx " + addr + "+1"); emit("lda " + addr);
                } else {
                    emit("lda " + addr); emit("ldx #0");
                }
            } else if (inst.src1.isVreg() && elemAbsY_.count(inst.src1.vregId)) {
                // an element of a global: base+off,y (#318)
                std::string op = elemYOperand(elemAbsY_.at(inst.src1.vregId));
                emit("lda " + op + ",y");
                if (inst.resultType == ir::Type::I8) emit("ldx #0");
                else emit("ldx " + op + "+1,y");
            } else if (inst.src1.kind != ir::OperandKind::NONE) {
                // Load value from address held in an operand via (ZP),Y
                std::string zpPair;
                int yOff = 0;
                if (inst.src1.isVreg() && foldedAddrs_.count(inst.src1.vregId)) {
                    zpPair = foldedPtr(inst.src1.vregId, "__zp_scratch", yOff);
                } else if (inst.src1.isVreg()) {
                    auto addrAlloc = alloc_.getAlloc(inst.src1.vregId);
                    if (addrAlloc.loc == VRegAllocator::IN_ZP) {
                        std::stringstream ss;
                        ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << addrAlloc.offset;
                        zpPair = ss.str();
                    } else {
                        loadVreg(inst.src1.vregId);
                        emit("sta __zp_scratch");
                        emit("stx __zp_scratch+1");
                        zpPair = "__zp_scratch";
                    }
                } else {
                    loadOperand(inst.src1);
                    emit("sta __zp_scratch");
                    emit("stx __zp_scratch+1");
                    zpPair = "__zp_scratch";
                }
                
                if (tryFusedRMW(inst, zpPair, yOff)) break;
                ms_.invalidateAll();
                if (inst.resultType == ir::Type::I8) {
                    emit("ldy #" + std::to_string(yOff));
                    emit("lda (" + zpPair + "),y");
                    // (into a byte register: X is not part of it, #344)
                    if (!(inst.dest.isVreg() && alloc_.getAlloc(inst.dest.vregId).type == ir::Type::I8 &&
                          alloc_.getAlloc(inst.dest.vregId).loc == VRegAllocator::IN_ZP))
                        emit("ldx #0");
                } else if (inst.resultType == ir::Type::I32 && yOff == 0) {
                    // Z = 0: (zp),z of LDQ adds Z on the MEGA65 core and xemu
                    // (the MEGA65 book documents no index): the same either way
                    emit("ldz #0");
                    emit("ldq (" + zpPair + "),z");
                } else if (inst.resultType == ir::Type::I32) {
                    // high bytes first; byte 2 waits on the stack while Y indexes
                    emit("ldy #" + std::to_string(yOff + 3));
                    emit("lda (" + zpPair + "),y"); emit("taz");
                    emit("dey"); emit("lda (" + zpPair + "),y"); emit("pha");
                    emit("dey"); emit("lda (" + zpPair + "),y"); emit("tax");
                    emit("dey"); emit("lda (" + zpPair + "),y");
                    emit("ply");
                } else {
                    // high byte first: no pha/pla
                    emit("ldy #" + std::to_string(yOff + 1));
                    emit("lda (" + zpPair + "),y"); emit("tax");
                    emit("dey"); emit("lda (" + zpPair + "),y");
                }
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::STORE: {
            // computed in the local's home already (findStoreTargets)
            if (inst.src1.isVreg() && inst.src2.isVreg() && inst.args.empty() &&
                storeTargets_.count(inst.src1.vregId) && zarVregs_.count(inst.src2.vregId) &&
                storeTargets_[inst.src1.vregId] == zarVregs_[inst.src2.vregId]) {
                // (the instruction before may have stored it there itself:
                // its "skip the next store" is for this one)
                if (resultInAX_ == -2) resultInAX_ = -1;
                break;
            }
            std::string storeR = irDesc(inst.src2.kind == ir::OperandKind::GLOBAL ? "→ " + inst.src2.name : "");

            // Fused ADDR_ELEM+STORE: compute address inline to __zp_scratch
            // (a long goes the normal way: its bytes 2 and 3 need Y and Z)
            if (inst.args.size() >= 3 && inst.resultType != ir::Type::F32 && inst.resultType != ir::Type::I32) {
                auto& base = inst.args[0];
                auto& index = inst.args[1];
                int stride = (int)inst.args[2].immVal;

                // A constant or a value in ZP is loaded after the address:
                // no save on the stack
                bool constVal = inst.src1.isImm() || (inst.src1.isVreg() && vregConstVal_.count(inst.src1.vregId));
                bool zpVal = inst.src1.isVreg() && !constVal &&
                             alloc_.getAlloc(inst.src1.vregId).loc == VRegAllocator::IN_ZP;
                // a small global array: name+off,y (the value parked in
                // __zp_scratch2 while the index is loaded, unless it is a
                // constant or in ZP: loaded after)
                {
                    int es = inst.resultType == ir::Type::I8 ? 1 : 2;
                    bool smallOk = arrayYOk(base, index, stride, inst.elemOffset, es);
                    if (smallOk) {
                        bool late = constVal || zpVal;
                        if (!late) {
                            loadOperand(inst.src1);
                            emit("sta __zp_scratch2");
                            if (es == 2) emit("stx __zp_scratch2+1");
                        }
                        std::string op;
                        smallArrayY(base, index, stride, inst.elemOffset, es, op);
                        if (late) {
                            if (constVal && es == 1) {
                                int64_t v = inst.src1.isImm() ? inst.src1.immVal : vregConstVal_[inst.src1.vregId];
                                emit("lda #" + std::to_string((int)(v & 0xFF)));
                            } else {
                                loadOperand(inst.src1);
                            }
                            emit("sta " + op + ",y");
                            if (es == 2) { emit("txa"); emit("sta " + op + "+1,y"); }
                        } else {
                            emit("lda __zp_scratch2");
                            emit("sta " + op + ",y");
                            if (es == 2) { emit("lda __zp_scratch2+1"); emit("sta " + op + "+1,y"); }
                        }
                        ms_.invalidateAll();
                        break;
                    }
                }
                {
                    std::string zp;
                    std::string direct = index.isVreg() ? directBase(index.vregId) : "";
                    // (a byte cut from a variable: loaded from there, its own register unwritten)
                    bool cut = inst.src1.isVreg() && truncAlias_.count(inst.src1.vregId);
                    bool early = constVal || (zpVal && !cut);
                    if (inst.resultType == ir::Type::I8 && zpPtrByteIndex(base, index, stride, inst.elemOffset, zp) &&
                        (early || !direct.empty())) {
                        // (Y first when loading the value cannot change it)
                        if (early) {
                            if (!direct.empty()) emit("ldy " + direct, "the index ((p),y)");
                            else { loadOperand(index); emit("tay", "the index ((p),y)"); }
                        }
                        if (constVal) {
                            int64_t v = inst.src1.isImm() ? inst.src1.immVal : vregConstVal_[inst.src1.vregId];
                            emit("lda #" + std::to_string((int)(v & 0xFF)));
                        } else if (cut) {
                            loadVregA(inst.src1.vregId);
                        } else {
                            loadOperand(inst.src1);
                        }
                        if (!early) emit("ldy " + direct, "the index ((p),y)");
                        emit("sta (" + zp + "),y");
                        ms_.invalidateAll();
                        break;
                    }
                }
                if (constVal || zpVal) {
                    std::string baseStr = (base.kind == ir::OperandKind::GLOBAL) ?
                        globalBase(base, inst.elemOffset) : src2MemOperand(base);
                    // the address may be in __zp_scratch already (a load of
                    // the same element just before: a[j + 1] = t after a[j + 1])
                    int yOff = 0;
                    bool cacheable = base.kind == ir::OperandKind::GLOBAL && index.isVreg();
                    int d = inst.elemOffset - elemCache_.offset;
                    if (cacheable && elemCache_.valid && elemCache_.base == base.name &&
                        elemCache_.index == index.vregId && elemCache_.stride == stride && d >= 0 && d <= 250) {
                        yOff = d;
                        emitComment("address in __zp_scratch, Y + " + std::to_string(d));
                    } else {
                        emitArrayElemAddr(baseStr, index, stride, "__zp_scratch");
                        elemCache_.valid = cacheable;
                        elemCache_.base = base.name;
                        elemCache_.index = index.isVreg() ? index.vregId : 0;
                        elemCache_.stride = stride;
                        elemCache_.offset = inst.elemOffset;
                    }
                    ms_.invalidateAll();
                    if (constVal && inst.resultType == ir::Type::I8) {
                        // a byte: A only
                        int64_t v = inst.src1.isImm() ? inst.src1.immVal : vregConstVal_[inst.src1.vregId];
                        emit("lda #" + std::to_string((int)(v & 0xFF)));
                    } else {
                        loadOperand(inst.src1);
                    }
                    emit("ldy #" + std::to_string(yOff));
                    emit("sta (__zp_scratch),y");
                    if (inst.resultType != ir::Type::I8) {
                        emit("txa"); emit("iny"); emit("sta (__zp_scratch),y");
                    }
                    ms_.invalidateAll();
                    break;
                }

                // Load value first (before addr_elem clobbers A:X)
                loadOperand(inst.src1);
                emit("pha"); // save value on stack
                if (inst.resultType != ir::Type::I8) emit("phx");

                std::string baseStr = (base.kind == ir::OperandKind::GLOBAL) ?
                    globalBase(base, inst.elemOffset) : src2MemOperand(base);
                emitArrayElemAddr(baseStr, index, stride, "__zp_scratch");

                // Restore value and store via (__zp_scratch),Y
                if (inst.resultType != ir::Type::I8) emit("plx");
                emit("pla");
                emit("ldy #0");
                emit("sta (__zp_scratch),y");
                if (inst.resultType == ir::Type::I32) {
                    emit("txa"); emit("iny"); emit("sta (__zp_scratch),y");
                    emit("tya"); emit("iny"); emit("sta (__zp_scratch),y"); // wait, Y is used for index
                    // I32 fused store is complex — skip for now, fall through to normal path
                } else if (inst.resultType != ir::Type::I8) {
                    emit("txa"); emit("iny"); emit("sta (__zp_scratch),y");
                }
                ms_.invalidateAll();
                break;
            }

            if (inst.resultType == ir::Type::F32 && inst.src1.isVreg()) {
                // 5-byte float store from src1 vreg to dest address
                ms_.invalidateAll();
                if (inst.src2.kind == ir::OperandKind::GLOBAL) {
                    emitF32VregToAbs(inst.src1.vregId, inst.src2.name);
                } else if (inst.src2.isVreg()) {
                    bool isLocalSlot = localSlotVregs_.count(inst.src2.vregId) > 0;
                    if (isLocalSlot) {
                        // Store into a float local variable
                        std::string dstBase;
                        if (f32AbsBase(inst.src2.vregId, dstBase)) {
                            emitF32VregToAbs(inst.src1.vregId, dstBase);
                        } else {
                            for (int i = 0; i < 5; i++) {
                                emitF32Byte("lda", inst.src1.vregId, i);
                                emitF32Byte("sta", inst.src2.vregId, i);
                            }
                        }
                    } else {
                        // Indirect store via (ZP),Y — dest vreg holds a pointer address
                        std::string zpPair;
                        auto addrAlloc = alloc_.getAlloc(inst.src2.vregId);
                        if (addrAlloc.loc == VRegAllocator::IN_ZP)
                            zpPair = "$" + hex8((uint8_t)addrAlloc.offset);
                        else { loadVreg(inst.src2.vregId); emit("sta __zp_scratch"); emit("stx __zp_scratch+1"); zpPair = "__zp_scratch"; }
                        for (int i = 0; i < 5; i++) {
                            emitF32Byte("lda", inst.src1.vregId, i);
                            emit("ldy #" + std::to_string(i));
                            emit("sta (" + zpPair + "),y");
                        }
                    }
                }
                break;
            }
            if (inst.src2.kind == ir::OperandKind::GLOBAL) {
                // Store directly to global address
                if (inst.resultType == ir::Type::I8 && inst.src1.isVreg() && (truncAlias_.count(inst.src1.vregId) || suppressedVregs_.count(inst.src1.vregId) ||
                                                                              globalLoadAlias_.count(inst.src1.vregId))) {
                    loadVregA(inst.src1.vregId);
                } else if (inst.resultType == ir::Type::I8 && inst.src1.isVreg()) {
                    // I8: only need A, skip ldx #0
                    auto valAlloc = alloc_.getAlloc(inst.src1.vregId);
                    if (valAlloc.loc == VRegAllocator::IN_ZP) {
                        std::stringstream vs;
                        vs << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << valAlloc.offset;
                        emit("lda " + vs.str());
                    } else if (valAlloc.loc == VRegAllocator::IN_FRAME) {
                        if (vregOffset_.count(inst.src1.vregId)) {
                            if (currentFunctionUseSAC_) {
                                std::string sym = sacSymbol(inst.src1.vregId);
                                emit("lda " + sym);
                            } else {
                                emit("lda.local " + std::to_string(vregOffset_[inst.src1.vregId]));
                            }
                        }
                    } else {
                        loadOperand(inst.src1);
                    }
                } else if (inst.resultType == ir::Type::I8 && inst.src1.isImm()) {
                    emit("lda #" + std::to_string((int)(inst.src1.immVal & 0xFF)));
                } else {
                    loadOperand(inst.src1);
                }
                // Phase 97.5: Apply .zp suffix for __zp globals
                emitOptimized("sta " + inst.src2.name, storeR);
                if (inst.resultType == ir::Type::I32) {
                    emitOptimized("stx " + inst.src2.name + "+1", storeR);
                    emitOptimized("sty " + inst.src2.name + "+2", storeR);
                    emitOptimized("stz " + inst.src2.name + "+3", storeR);
                } else if (inst.resultType != ir::Type::I8) {
                    emit("stx " + inst.src2.name + "+1", storeR);
                }
            } else if (inst.src2.isVreg()) {
                // Optimization: if the address vreg is a known constant,
                // emit direct absolute store instead of indirect.
                auto constIt = vregConstVal_.find(inst.src2.vregId);
                if (constIt != vregConstVal_.end() &&
                    !localSlotVregs_.count(inst.src2.vregId)) {
                    std::string addr = "$" + hex16((uint16_t)constIt->second);
                    // Load value — use constant directly if available
                    auto valConst = (inst.src1.isVreg()) ? vregConstVal_.find(inst.src1.vregId) : vregConstVal_.end();
                    if (inst.src1.isImm()) {
                        int v = (int)inst.src1.immVal;
                        emit("lda #" + std::to_string(v & 0xFF));
                        if (inst.resultType == ir::Type::I32) {
                            emit("ldx #" + std::to_string((v >> 8) & 0xFF));
                            emit("ldy #" + std::to_string((v >> 16) & 0xFF));
                            emit("ldz #" + std::to_string((v >> 24) & 0xFF));
                        } else if (inst.resultType != ir::Type::I8) {
                            emit("ldx #" + std::to_string((v >> 8) & 0xFF));
                        }
                    } else if (valConst != vregConstVal_.end()) {
                        int v = (int)valConst->second;
                        emit("lda #" + std::to_string(v & 0xFF));
                        if (inst.resultType == ir::Type::I32) {
                            emit("ldx #" + std::to_string((v >> 8) & 0xFF));
                            emit("ldy #" + std::to_string((v >> 16) & 0xFF));
                            emit("ldz #" + std::to_string((v >> 24) & 0xFF));
                        } else if (inst.resultType != ir::Type::I8) {
                            emit("ldx #" + std::to_string((v >> 8) & 0xFF));
                        }
                    } else if (inst.resultType == ir::Type::I8 && inst.src1.isVreg()) {
                        loadVregA(inst.src1.vregId);
                    } else {
                        loadOperand(inst.src1);
                    }
                    emit("sta " + addr);
                    if (inst.resultType == ir::Type::I32) {
                        emit("stx " + addr + "+1");
                        emit("sty " + addr + "+2");
                        emit("stz " + addr + "+3");
                    } else if (inst.resultType != ir::Type::I8) {
                        emit("stx " + addr + "+1");
                    }
                    break;
                }

                // Skip STORE to suppressed vregs (dead call arg slots)
                if (suppressedVregs_.count(inst.src2.vregId)) break;

                // Is the target a local variable slot (direct write)
                // or a computed address (indirect write through pointer)?
                bool isLocalSlot = localSlotVregs_.count(inst.src2.vregId) > 0;
                if (isLocalSlot) {
                    // Check if previous instruction already handled this STORE
                    if (resultInAX_ == -2) {
                        resultInAX_ = -1;
                        break;
                    }
                    // Store-forwarding: if src1 result is still in A:X (or A:Z), skip reload
                    if (inst.src1.isVreg() && (int32_t)inst.src1.vregId == resultInAX_) {
                        // Result still in registers from previous instruction — store directly.
                        // If the previous storeVreg used stax.fp (frame store), X was clobbered
                        // by TSX but the hi byte is in Z (from the internal TXA;TAZ sequence).
                        valueByte_[0] = REG_A;
                        auto prevAlloc = alloc_.getAlloc(inst.src1.vregId);
                        valueByte_[1] = (prevAlloc.loc == VRegAllocator::IN_FRAME) ? REG_Z : REG_X;
                        storeVreg(inst.src2.vregId);
                    } else {
                        // (a byte into a byte local: A alone, #344)
                        if (inst.resultType == ir::Type::I8 && alloc_.getAlloc(inst.src2.vregId).type == ir::Type::I8)
                            loadOperandA(inst.src1);
                        else
                            loadOperand(inst.src1);
                        valueByte_[0] = REG_A;
                        valueByte_[1] = REG_X;
                        storeVreg(inst.src2.vregId);
                    }
                } else if (elemAbsY_.count(inst.src2.vregId)) {
                    // an element of a global: base+off,y (#318)
                    const ElemY& ea = elemAbsY_.at(inst.src2.vregId);
                    bool inA = inst.src1.isVreg() && (int32_t)inst.src1.vregId == resultInAX_;
                    // (a byte cut from a variable or a suppressed constant: A
                    // from where it lives, loadVregA)
                    auto loadValue = [&]() {
                        if (inst.src1.isVreg() && (truncAlias_.count(inst.src1.vregId) ||
                                                   suppressedVregs_.count(inst.src1.vregId)))
                            loadVregA(inst.src1.vregId);
                        else
                            loadOperand(inst.src1);
                    };
                    std::string op;
                    if (ea.stride == 1 && alloc_.getAlloc(ea.index.vregId).type == ir::Type::I8) {
                        if (!inA) loadValue();              // (first: its load may use Y)
                        op = elemYOperand(ea);
                    } else {
                        op = elemYOperand(ea);              // (through A: the value is in the
                        loadValue();                        // base page or a constant, Y kept)
                    }
                    emit("sta " + op + ",y");
                    ms_.invalidateAll();
                } else {
                    // Indirect store: write value through pointer address
                    auto addrAlloc = alloc_.getAlloc(inst.src2.vregId);
                    std::string zpPair;
                    int yOff = 0;
                    if (foldedAddrs_.count(inst.src2.vregId)) {
                        zpPair = foldedPtr(inst.src2.vregId, "__zp_scratch2", yOff);
                    } else if (addrAlloc.loc == VRegAllocator::IN_ZP) {
                        std::stringstream ss;
                        ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << addrAlloc.offset;
                        zpPair = ss.str();
                    } else {
                        loadVreg(inst.src2.vregId);
                        emit("sta __zp_scratch2");
                        emit("stx __zp_scratch2+1");
                        zpPair = "__zp_scratch2";
                    }

                    // 32-bit value in memory or constant: byte by byte, no
                    // register shuffling (a repeated constant byte is not reloaded)
                    bool direct32 = inst.resultType == ir::Type::I32;
                    if (direct32 && inst.src1.isVreg())
                        direct32 = ir::typeSize(alloc_.getAlloc(inst.src1.vregId).type) == 4 ||
                                   vregConstVal_.count(inst.src1.vregId);
                    for (int k = 0; direct32 && k < 4; k++) direct32 = !directByte(inst.src1, k).empty();
                    if (direct32) {
                        ms_.invalidateAll();
                        emit("ldy #" + std::to_string(yOff));
                        std::string prev;
                        for (int k = 0; k < 4; k++) {
                            std::string b = directByte(inst.src1, k);
                            if (k) emit("iny");
                            if (b != prev || b[0] != '#') emit("lda " + b);
                            prev = b;
                            emit("sta (" + zpPair + "),y");
                        }
                        break;
                    }

                    // Now load the VALUE to store
                    if (inst.resultType == ir::Type::I8) {
                        // 8-bit store: only need A, skip X load
                        if (inst.src1.isImm()) {
                            emit("lda #" + std::to_string((int)(inst.src1.immVal & 0xFF)));
                        } else if (inst.src1.isVreg() && (truncAlias_.count(inst.src1.vregId) || suppressedVregs_.count(inst.src1.vregId) ||
                                                          globalLoadAlias_.count(inst.src1.vregId))) {
                            loadVregA(inst.src1.vregId);
                        } else if (inst.src1.isVreg()) {
                            auto valAlloc = alloc_.getAlloc(inst.src1.vregId);
                            if (valAlloc.loc == VRegAllocator::IN_ZP) {
                                std::stringstream vs;
                                vs << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << valAlloc.offset;
                                emit("lda " + vs.str());
                            } else if (valAlloc.loc == VRegAllocator::IN_FRAME) {
                                if (vregOffset_.count(inst.src1.vregId)) {
                                    if (currentFunctionUseSAC_) {
                                        std::string sym = sacSymbol(inst.src1.vregId);
                                        emit("lda " + sym);
                                    } else {
                                        emit("lda.local " + std::to_string(vregOffset_[inst.src1.vregId]));
                                    }
                                }
                            } else {
                                loadOperand(inst.src1);
                            }
                        } else {
                            loadOperand(inst.src1);
                        }
                    } else {
                        loadOperand(inst.src1);
                    }

                    if (inst.resultType == ir::Type::I32) {
                        // Bytes 2/3 are in Y/Z: save them before Y becomes the index.
                        // A narrower value (e.g. an int constant initialising a long)
                        // only set A/X: zero-extend it.
                        bool narrow = inst.src1.isVreg() ? ir::typeSize(alloc_.getAlloc(inst.src1.vregId).type) < 4
                                                         : (inst.src1.isImm() && inst.src1.type != ir::Type::I32);
                        if (narrow) { emit("ldy #0"); emit("ldz #0"); }
                        emit("sty __zp_scratch3");
                        emit("stz __zp_scratch3+1");
                    }
                    emit("ldy #" + std::to_string(yOff));
                    if (inst.resultType == ir::Type::I8) {
                        emit("sta (" + zpPair + "),y");
                    } else if (inst.resultType == ir::Type::I32) {
                        emit("sta (" + zpPair + "),y"); // lo
                        emit("txa"); emit("iny"); emit("sta (" + zpPair + "),y"); // byte 1
                        emit("lda __zp_scratch3"); emit("iny"); emit("sta (" + zpPair + "),y"); // byte 2
                        emit("lda __zp_scratch3+1"); emit("iny"); emit("sta (" + zpPair + "),y"); // byte 3
                    } else {
                        emit("sta (" + zpPair + "),y"); // store lo byte
                        emit("txa");
                        emit("iny");
                        emit("sta (" + zpPair + "),y"); // store hi byte
                    }
                }
            } else {
                // STORE to non-vReg address (direct memory write)
                // This could be: STORE(value, global_addr) or STORE(value, imm_addr)
                if (inst.dest.kind == ir::OperandKind::IMM) {
                    throw std::runtime_error("IRCodeGen: STORE to immediate address 0x" +
                        std::to_string(inst.dest.immVal) + " — not yet implemented (use pointer vregs)");
                } else if (inst.dest.kind == ir::OperandKind::GLOBAL || inst.dest.kind == ir::OperandKind::LABEL) {
                    throw std::runtime_error("IRCodeGen: STORE to '" + inst.dest.name +
                        "' (global/label address) — not yet implemented (use pointer vregs)");
                } else {
                    throw std::runtime_error("IRCodeGen: STORE to non-vReg operand (kind=" +
                        std::to_string((int)inst.dest.kind) + ") — not yet implemented (use pointer vregs)");
                }
            }
            break;
        }

        case ir::Op::FAR_LOAD:
        case ir::Op::FAR_STORE: {
            // T __far *p (#68): the 32-bit address in four base-page bytes
            // (its register's, or $F2-$F5), read and written with 32-bit
            // indirect addressing, [bp],z
            const ir::Operand& addr = inst.op == ir::Op::FAR_LOAD ? inst.src1 : inst.src2;
            std::string bp;
            if (addr.isVreg() && !zextAlias_.count(addr.vregId) && !globalLoadAlias_.count(addr.vregId) &&
                !(suppressedVregs_.count(addr.vregId) && vregConstVal_.count(addr.vregId))) {
                auto al = alloc_.getAlloc(addr.vregId);
                if (al.loc == VRegAllocator::IN_ZP && al.type == ir::Type::I32) bp = "$" + hex8((uint8_t)al.offset);
            }
            if (bp.empty()) {
                if (addr.isImm()) loadOperand(ir::Operand::imm(addr.immVal, ir::Type::I32));
                else loadOperand(addr);
                emit("stq $F2", "far address");
                bp = "$F2";
            }
            int size = inst.resultType == ir::Type::I8 ? 1 : inst.resultType == ir::Type::I32 ? 4 : 2;
            // (p[c]: Z starts at the byte offset, foldFarOffset, #348)
            int off = inst.elemOffset;
            auto z = [&](int k) { return "ldz #" + std::to_string(off + k); };
            if (inst.op == ir::Op::FAR_LOAD) {
                if (size == 1) { emit(z(0)); emit("lda [" + bp + "],z"); emit("ldx #0"); }
                else if (size == 2) {
                    emit(z(1)); emit("lda [" + bp + "],z"); emit("tax");
                    emit("dez"); emit("lda [" + bp + "],z");
                } else { emit("ldz #0"); emit("ldq [" + bp + "],z"); }
                ms_.invalidateAll();
                if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
                break;
            }
            // a store: each byte from where the value is, else through A/X
            // (a long through $F6-$F9: Z indexes)
            const ir::Operand& val = inst.src1;
            if (farAddPending_ && val.isVreg() && farAddPending_->dest.isVreg() &&
                val.vregId == farAddPending_->dest.vregId && size == 2) {
                // x + c added byte by byte as it is stored (ldz / inz keep C)
                const ir::Inst& a = *farAddPending_;
                farAddPending_ = nullptr;
                bool add = a.op == ir::Op::ADD;
                // (x's low bits known 0 and c within them: no carry, ora; #350)
                int c0 = (int)(a.src2.immVal & 0xFFFF);
                if (add && c0 < 256 && (kb_.of(a.src1, 16) & c0) == (uint64_t)c0) {
                    emit("lda " + directByte(a.src1, 0), irDesc("x + c stored through a far pointer (no carry)"));
                    if (c0) emit("ora #" + std::to_string(c0));
                    emit(z(0));
                    emit("sta [" + bp + "],z");
                    emit("lda " + directByte(a.src1, 1));
                    emit("inz");
                    emit("sta [" + bp + "],z");
                    ms_.invalidateAll();
                    break;
                }
                for (int k = 0; k < 2; k++) {
                    emit("lda " + directByte(a.src1, k), k ? "" : irDesc(add ? "x + c stored through a far pointer" : "x - c stored through a far pointer"));
                    if (k == 0) emit(add ? "clc" : "sec");
                    emit(std::string(add ? "adc #" : "sbc #") + std::to_string((int)((a.src2.immVal >> (8 * k)) & 0xFF)));
                    emit(k ? "inz" : z(0));
                    emit("sta [" + bp + "],z");
                }
                ms_.invalidateAll();
                break;
            }
            farAddPending_ = nullptr;
            bool direct = true;
            std::vector<std::string> bytes;
            for (int k = 0; k < size && direct; k++) {
                std::string b = val.isImm() ? "#" + std::to_string((int)((val.immVal >> (8 * k)) & 0xFF))
                                            : directByte(val, k);
                if (b.empty()) direct = false;
                bytes.push_back(b);
            }
            if (!direct) {
                bytes.clear();
                if (size == 1) { loadOperandA(val); emit(z(0)); emit("sta [" + bp + "],z"); }
                else if (size == 2) {
                    loadOperand(val);
                    emit(z(0)); emit("sta [" + bp + "],z");
                    emit("inz"); emit("txa"); emit("sta [" + bp + "],z");
                } else {
                    loadOperand(val);
                    emit("stq $F6");
                    for (int k = 0; k < 4; k++) bytes.push_back("$" + hex8((uint8_t)(0xF6 + k)));
                }
            }
            for (int k = 0; k < (int)bytes.size(); k++) {
                emit("lda " + bytes[k]);
                emit(k ? "inz" : z(k));   // (lda leaves Z)
                emit("sta [" + bp + "],z");
            }
            ms_.invalidateAll();
            break;
        }

        case ir::Op::NEAR_LOAD:
        case ir::Op::NEAR_STORE: {
            // T __near *p: the offset in X, the bytes base-page indexed
            // ($00,x: the program's base page)
            const ir::Operand& off = inst.op == ir::Op::NEAR_LOAD ? inst.src1 : inst.src2;
            int size = inst.resultType == ir::Type::I8 ? 1 : inst.resultType == ir::Type::I32 ? 4 : 2;
            auto loadX = [&]() {
                std::string b = off.isImm() ? "#" + std::to_string((int)(off.immVal & 0xFF)) : directByte(off, 0);
                if (!b.empty()) emit("ldx " + b);
                else { loadOperandA(off); emit("tax"); }
            };
            auto at = [](int k) { return "$" + hex8((uint8_t)k) + ",x"; };
            if (inst.op == ir::Op::NEAR_LOAD) {
                loadX();
                if (size == 1) { emit("lda " + at(0)); emit("ldx #0"); }
                else if (size == 2) { emit("lda " + at(1)); emit("pha"); emit("lda " + at(0)); emit("plx"); }
                else {
                    emit("lda " + at(3)); emit("taz"); emit("lda " + at(2)); emit("tay");
                    emit("lda " + at(1)); emit("pha"); emit("lda " + at(0)); emit("plx");
                }
                ms_.invalidateAll();
                if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
                break;
            }
            // a store: the value's bytes from where they are, else from $F6-$F9
            const ir::Operand& val = inst.src1;
            std::vector<std::string> bytes;
            for (int k = 0; k < size; k++) {
                std::string b = val.isImm() ? "#" + std::to_string((int)((val.immVal >> (8 * k)) & 0xFF))
                                            : directByte(val, k);
                if (b.empty()) { bytes.clear(); break; }
                bytes.push_back(b);
            }
            if (bytes.empty()) {
                if (size == 1) { loadOperandA(val); emit("sta $F6"); }
                else if (size == 2) { loadOperand(val); emit("sta $F6"); emit("stx $F7"); }
                else { loadOperand(val); emit("stq $F6"); }
                for (int k = 0; k < size; k++) bytes.push_back("$" + hex8((uint8_t)(0xF6 + k)));
            }
            loadX();
            for (int k = 0; k < size; k++) {
                emit("lda " + bytes[k]);
                emit("sta " + at(k));
            }
            ms_.invalidateAll();
            break;
        }

        case ir::Op::NEAR_ADDR: {
            // a near pointer as a 16-bit one: the base page (B) its high byte
            loadOperandA(inst.src1);
            emit("pha");
            emit("tba");
            emit("tax");
            emit("pla");
            ms_.invalidateAll();
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::LOAD_ZP: {
            std::string addr = inst.src1.name;
            emit("lda " + addr);
            emit("ldx " + addr + "+1");
            storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::STORE_ZP: {
            loadVreg(inst.src1.vregId);
            std::string addr = inst.src2.name; // target ZP name
            emit("sta " + addr);
            emit("stx " + addr + "+1");
            break;
        }

        case ir::Op::VEC_BIN: {
            // r[i] = a[i] op b[i] (or op scalar b) on 1- or 2-byte elements:
            // (a),y / (b),y / (r),y with the pointers in the scratch pairs
            int op = (int)inst.args[1].immVal, es = (int)inst.args[2].immVal, n = (int)inst.args[3].immVal;
            bool scalar = inst.args[4].immVal != 0, unroll = inst.args[5].immVal != 0;
            auto toZp = [&](const ir::Operand& o, const std::string& zp) {
                loadOperand(o);
                emit("sta " + zp);
                emit("stx " + zp + "+1");
            };
            toZp(inst.src1, "__zp_scratch");
            toZp(inst.src2, "__zp_scratch2");   // B's address, or the scalar value
            toZp(inst.args[0], "__zp_scratch3");
            static const char* mn[] = {"adc", "sbc", "and", "ora", "eor"};
            auto element = [&](bool first) {
                for (int b = 0; b < es; b++) {
                    if (!first || b) emit("iny");
                    emit("lda (__zp_scratch),y");
                    if (b == 0 && op == 0) emit("clc");
                    if (b == 0 && op == 1) emit("sec");
                    if (scalar) emit(std::string(mn[op]) + " __zp_scratch2" + (b ? "+1" : ""));
                    else emit(std::string(mn[op]) + " (__zp_scratch2),y");
                    emit("sta (__zp_scratch3),y");
                }
            };
            emit("ldy #0");
            if (unroll) {
                for (int i = 0; i < n; i++) element(i == 0);
            } else {
                std::string top = "@__vec_" + std::to_string(labelCounter_++);
                emitLabel(top);
                element(true);
                emit("iny");
                emit("cpy #" + std::to_string(n * es));
                emit("bne " + top);
            }
            ms_.invalidateAll();
            break;
        }

        case ir::Op::TRAMPOLINE: {
            // src1 = target func, src2 = link, args[0] = buffer vreg
            uint32_t bufVregId = inst.args[0].vregId;

            // Step 1: Get address of buffer on stack into scratch $08/$09
            if (vregOffset_.find(bufVregId) == vregOffset_.end()) {
                throw std::runtime_error("TRAMPOLINE: buffer vreg " + std::to_string(bufVregId) +
                    " not allocated on stack (vregOffset_ map has " + std::to_string(vregOffset_.size()) + " entries)");
            }
            if (currentFunctionUseSAC_)
                emit("ldax #" + currentFunctionName_ + "__local_" + std::to_string(bufVregId));  // static storage
            else
                emit("leax.local " + std::to_string(vregOffset_.at(bufVregId)));
            emit("sta $08");
            emit("stx $09");

            // Step 2: Initialize trampoline code (11 bytes)
            // 0: $A9 <lo_link, 2: $85 <chain, 4: $A9 <hi_link, 6: $85 <chain+1, 8: $4C <lo_tgt >hi_tgt

            // Load link into AX
            loadOperand(inst.src2);

            // link lo (A)
            emit("pha");
            emit("ldy #0");
            emit("lda #$A9"); // lda #lo
            emit("sta ($08),y");
            emit("pla");
            emit("iny");
            emit("sta ($08),y");

            // sta __static_chain
            emit("iny");
            emit("lda #$85");
            emit("sta ($08),y");
            emit("iny");
            emit("lda #<__static_chain");
            emit("sta ($08),y");

            // link hi (X)
            emit("txa");
            emit("pha");
            emit("iny");
            emit("lda #$A9"); // lda #hi
            emit("sta ($08),y");
            emit("pla");
            emit("iny");
            emit("sta ($08),y");

            // sta __static_chain+1
            emit("iny");
            emit("lda #$85");
            emit("sta ($08),y");
            emit("iny");
            emit("lda #<(__static_chain+1)");
            emit("sta ($08),y");

            // jmp _target
            emit("iny");
            emit("lda #$4C"); // jmp abs
            emit("sta ($08),y");
            emit("iny");
            emit("lda #<" + inst.src1.name);
            emit("sta ($08),y");
            emit("iny");
            emit("lda #>" + inst.src1.name);
            emit("sta ($08),y");

            // Result: A:X = address of trampoline
            emit("lda $08");
            emit("ldx $09");
            storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::ADDR_GLOBAL: {
            emit("ldax #" + inst.src1.name);
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::ADDR_LABEL: {
            // A label of another function (a nested function's &&label of
            // its parent): that function's global alias for it
            bool own = false;
            if (currentFn_)
                for (const auto& b : currentFn_->blocks) if (b.label == inst.src1.name) { own = true; break; }
            std::string target = "@" + inst.src1.name;
            if (!own && currentModule_)
                for (const auto& f : currentModule_->functions)
                    if (&f != currentFn_ && f.addressTakenLabels.count(inst.src1.name)) { target = f.labelAlias(inst.src1.name); break; }
            emit("ldax #" + target);
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::ADDR_UPLEVEL: {
            int levels = (int)inst.args[1].immVal;
            const ir::Operand& targetVar = inst.args[0];
            int targetOffset = 0;
            if (targetVar.vregId != 0xFFFFFFFF) {
                auto fit = frameOffsetsByFn_.find(targetVar.name);
                if (fit != frameOffsetsByFn_.end() && fit->second.count(targetVar.vregId)) {
                    targetOffset = fit->second.at(targetVar.vregId);
                } else if (functionMap_.count(targetVar.name)) {
                    const auto* targetFn = functionMap_.at(targetVar.name);
                    if (targetFn->vregOffsets.count(targetVar.vregId))
                        targetOffset = targetFn->vregOffsets.at(targetVar.vregId);
                }
            }

            // Start by loading the static link (pointer to direct parent frame)
            loadVreg(inst.src1.vregId);

            // Follow static chain if target is higher than direct parent:
            // each frame passed through holds its function's static link
            for (int i = 0; i < levels - 1; i++) {
                emit("sta $08");
                emit("stx $09");
                int slOff = 0;
                if ((size_t)(2 + i) < inst.args.size()) {
                    const std::string& fname = inst.args[2 + i].name;
                    auto mit = functionMap_.find(fname);
                    auto fit = frameOffsetsByFn_.find(fname);
                    if (mit != functionMap_.end() && fit != frameOffsetsByFn_.end() &&
                        mit->second->staticLinkVreg >= 0 &&
                        fit->second.count((uint32_t)mit->second->staticLinkVreg))
                        slOff = fit->second.at((uint32_t)mit->second->staticLinkVreg);
                }
                emit("ldy #" + std::to_string(slOff));
                emit("lda ($08),y");
                emit("pha");
                emit("iny");
                emit("lda ($08),y");
                emit("tax");
                emit("pla");
                // Result in AX
            }

            if (targetOffset != 0) {
                emit("clc");
                emit("adc #" + std::to_string(targetOffset & 0xFF));
                emit("pha");
                emit("txa");
                emit("adc #" + std::to_string((targetOffset >> 8) & 0xFF));
                emit("tax");
                emit("pla");
            }

            storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::ADDR_LOCAL: {
            // Helper: emit address of a SAC static storage symbol for a vreg
            auto emitSACAddr = [&](uint32_t vregId) {
                std::string storageSymbol;
                if (vregId < currentFn_->paramTypes.size()) {
                    std::string pName = (vregId < currentFn_->paramNames.size() && !currentFn_->paramNames[vregId].empty())
                        ? currentFn_->paramNames[vregId] : std::to_string(vregId);
                    storageSymbol = currentFunctionName_ + "__param_" + pName;
                } else {
                    storageSymbol = currentFunctionName_ + "__local_" + std::to_string(vregId);
                }
                emit("ldax #" + storageSymbol);
            };

            if (inst.src1.isVreg()) {
                // SAC mode: use static storage symbol address instead of frame-relative
                if (currentFunctionUseSAC_ && vregOffset_.count(inst.src1.vregId)) {
                    emitSACAddr(inst.src1.vregId);
                } else {
                    // Use vregOffset_ (consistent with storeVreg/loadVreg frame offsets)
                    auto vit = vregOffset_.find(inst.src1.vregId);
                    if (vit != vregOffset_.end()) {
                        int offset = vit->second;
                        if (frameAddrZPIndex_ >= 0 && offset == 0) {
                            std::string frameAddrZP = zpAddr(frameAddrZPIndex_);
                            emit("lda " + frameAddrZP);
                            emit("ldx " + frameAddrZP + "+1");
                        } else {
                            emit("leax.local " + std::to_string(offset));
                        }
                    } else {
                        auto alloc = alloc_.getAlloc(inst.src1.vregId);
                        if (alloc.loc == VRegAllocator::IN_ZP) {
                            emit("lda #" + std::to_string(alloc.offset));
                            emit("ldx #0");
                        } else if (alloc.loc == VRegAllocator::IN_FRAME) {
                            int offset = alloc.offset;
                            if (frameAddrZPIndex_ >= 0 && offset == 0) {
                                std::string frameAddrZP = zpAddr(frameAddrZPIndex_);
                                emit("lda " + frameAddrZP);
                                emit("ldx " + frameAddrZP + "+1");
                            } else {
                                emit("leax.local " + std::to_string(offset));
                            }
                        } else {
                            emit("; ERROR: vreg not allocated");
                        }
                    }
                }
            } else {
                int offset = (int)inst.src1.immVal;
                if (frameAddrZPIndex_ >= 0 && offset == 0) {
                    std::string frameAddrZP = zpAddr(frameAddrZPIndex_);
                    emit("lda " + frameAddrZP);
                    emit("ldx " + frameAddrZP + "+1");
                } else {
                    emit("leax.local " + std::to_string(offset));
                }
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::BR_INDIRECT: {
            if (skipBrIndirect_) { skipBrIndirect_ = false; break; }   // jmp (table,x) done
            loadVreg(inst.src1.vregId);
            // Self-modified JMP, as for indirect calls: JMP (abs) can't
            // address the relocated base page.
            std::string jmpLabel = "@__goto_site_" + std::to_string(labelCounter_++);
            emit("sta " + jmpLabel + "+1");
            emit("stx " + jmpLabel + "+2");
            if (inst.args.size() == 2) {
                // Non-local goto: args = static link, levels. The target
                // function's frame pointer is the static link (followed up
                // the chain); its stack pointer at a statement is FP - 1
                loadVreg(inst.args[0].vregId);
                for (int i = 0; i < (int)inst.args[1].immVal - 1; i++) {
                    emit("sta $08");
                    emit("stx $09");
                    emit("ldy #1");
                    emit("lda ($08),y");
                    emit("tax");
                    emit("dey");
                    emit("lda ($08),y");
                }
                emit("sta $FD");
                emit("stx $FE");
                std::string nb = "@__nlg_" + std::to_string(labelCounter_++);
                emit("sec");
                emit("sbc #1");
                emit("bcs " + nb);
                emit("dex");
                emitLabel(nb);
                emit("phx");
                emit("ply");
                emit("tax");
                emit("sei");
                emit("txs");
                emit("tys");
                emit("cli");
            }
            emitLabel(jmpLabel);
            emit("jmp $0000");
            break;
        }

        case ir::Op::GET_FP: {
            emit("lda $FD");
            emit("ldx $FE");
            storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::ADDR_ELEM: {
            // used as base+off,y by its loads and stores (elemAbsY_)
            if (inst.dest.isVreg() && elemAbsY_.count(inst.dest.vregId)) break;
            int fromCache = elemAddrFromCache(inst);
            if (fromCache >= 0) {
                std::string d = "$" + hex8((uint8_t)alloc_.getAlloc(inst.dest.vregId).offset);
                emitComment("address in __zp_scratch already");
                emit("lda __zp_scratch");
                if (fromCache) { emit("clc"); emit("adc #" + std::to_string(fromCache)); }
                emit("sta " + d);
                emit("lda __zp_scratch+1");
                if (fromCache) emit("adc #0");
                emit("sta " + d + "+1");
                break;
            }
            int elemSize = (inst.args.size() > 0) ? (int)inst.args[0].immVal : 2;
            
            auto operandToString = [&](const ir::Operand& op) -> std::string {
                if (op.isImm()) {
                    return "#" + std::to_string((int)op.immVal);
                }
                if (op.isVreg()) {
                    auto alloc = alloc_.getAlloc(op.vregId);
                    if (alloc.loc == VRegAllocator::IN_ZP) {
                        std::stringstream ss;
                        ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << alloc.offset;
                        return ss.str();
                    } else if (alloc.loc == VRegAllocator::IN_AX) {
                        return ".AX";
                    }
                }
                if (op.kind == ir::OperandKind::GLOBAL) {
                    return op.name;
                }
                return src2MemOperand(op);
            };

            std::string baseStr;
            if (inst.src1.kind == ir::OperandKind::GLOBAL) {
                baseStr = globalBase(inst.src1, inst.elemOffset);
            } else {
                baseStr = operandToString(inst.src1);
            }
            
            std::string destStr = ".AX";
            bool storeNeeded = true;
            if (inst.dest.isVreg()) {
                auto destAlloc = alloc_.getAlloc(inst.dest.vregId);
                if (destAlloc.loc == VRegAllocator::IN_ZP) {
                    std::stringstream ss;
                    ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << destAlloc.offset;
                    destStr = ss.str();
                    storeNeeded = false;
                }
            }
            
            // Compute array element address
            if (destStr == ".AX") {
                // Result goes to A:X register pair
                emitArrayElemAddr(baseStr, inst.src2, elemSize, "");  // Don't store to ZP
            } else {
                // Result goes to destZP (ZP address)
                emitArrayElemAddr(baseStr, inst.src2, elemSize, destStr);
            }

            if (storeNeeded && inst.dest.isVreg()) {
                storeVreg(inst.dest.vregId);
            }
            break;
        }

        case ir::Op::COPY: {
            // a pointer copy folded into its loads/stores
            if (inst.dest.isVreg() && foldedAddrs_.count(inst.dest.vregId)) break;
            // Check for struct copy: src1 is PTR (address returned from struct function),
            // src2 has struct size, dest is destination vreg
            if (inst.src1.type == ir::Type::PTR && inst.src2.kind == ir::OperandKind::IMM &&
                inst.resultType == ir::Type::I32 && inst.dest.isVreg()) {
                // Struct copy: copy bytes from (AX) to destination frame location
                // AX contains address returned by struct function
                // Copy bytes: load from (AX+i), store to dest+i
                loadOperand(inst.src1);  // Load address into AX
                emit("sta __zp_scratch");         // Save address to ZP (symbolic reference)
                emit("stx __zp_scratch+1");

                // Get destination vreg frame location
                auto destAlloc = alloc_.getAlloc(inst.dest.vregId);
                std::string destPtr;
                if (destAlloc.loc == VRegAllocator::IN_ZP) {
                    destPtr = "$" + hex8((uint8_t)destAlloc.offset);
                } else {
                    // Frame destination: load the 4 bytes into AXYZ and store the vreg
                    emit("ldy #3");
                    emit("lda (__zp_scratch),y");
                    emit("taz");
                    emit("dey");
                    emit("lda (__zp_scratch),y");
                    emit("pha");
                    emit("dey");
                    emit("lda (__zp_scratch),y");
                    emit("tax");
                    emit("dey");
                    emit("lda (__zp_scratch),y");
                    emit("ply");
                    storeVreg(inst.dest.vregId);
                    break;
                }

                // Copy 4 bytes from (address) to (dest)
                for (int i = 0; i < 4; i++) {
                    if (destAlloc.loc == VRegAllocator::IN_ZP) {
                        // Direct copy to ZP
                        emit("ldy #" + std::to_string(i));
                        emit("lda (__zp_scratch),y");
                        emit("sta " + destPtr + "+" + std::to_string(i));
                    } else {
                        // Copy via indirect addressing
                        emit("ldy #" + std::to_string(i));
                        emit("lda (__zp_scratch),y");
                        emit("ldy #" + std::to_string(i));
                        emit("sta (" + destPtr + "),y");
                    }
                }
            } else if (inst.dest.isVreg() && inst.src1.isVreg() &&
                       (inst.resultType == ir::Type::F32 || inst.dest.type == ir::Type::F32)) {
                // Float copy: all 5 bytes (AX would carry only two)
                ms_.invalidateAll();
                for (int b = 0; b < 5; b++) {
                    emitF32Byte("lda", inst.src1.vregId, b);
                    emitF32Byte("sta", inst.dest.vregId, b);
                }
            } else {
                // Regular copy (a byte into a byte: A alone, #342)
                if (inst.resultType == ir::Type::I8 && inst.src1.type == ir::Type::I8 && inst.dest.isVreg() &&
                    alloc_.getAlloc(inst.dest.vregId).type == ir::Type::I8)
                    loadOperandA(inst.src1);
                else
                    loadOperand(inst.src1);
                if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            }
            break;
        }

        case ir::Op::DEREF: {
            // a pointer read folded into its loads/stores
            if (inst.dest.isVreg() && foldedAddrs_.count(inst.dest.vregId)) break;
            loadOperand(inst.src1);
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::BR: {
            if (inst.src1.kind == ir::OperandKind::LABEL) {
                // Skip branch if target is the immediately following block
                if (inst.src1.name != nextBlockLabel_) {
                    emit("bra @" + inst.src1.name);
                }
            }
            break;
        }

        case ir::Op::BR_COND: {
            // src1 = condition, dest = true label, src2 = false label
            std::string trueTarget = (inst.dest.kind == ir::OperandKind::LABEL) ? "@" + inst.dest.name : "";
            std::string falseTarget = (inst.src2.kind == ir::OperandKind::LABEL) ? "@" + inst.src2.name : "";
            bool falseFallsThrough = (inst.src2.kind == ir::OperandKind::LABEL && inst.src2.name == nextBlockLabel_);

            // Fused compare-and-branch: if the condition is from a CMP that just set flags,
            // branch directly using the comparison flags instead of materializing a boolean.
            if (lastCmp_.valid && inst.src1.isVreg() && inst.src1.vregId == lastCmp_.destVreg) {
                // BBS/BBR single-bit test: branch directly on ZP bit, no register usage
                if (!lastCmp_.bbsZpAddr.empty() && lastCmp_.bbsBitN >= 0) {
                    std::string bitStr = std::to_string(lastCmp_.bbsBitN);
                    // CMP_NE means "bit set = true", so bbs = true target, bbr = false target
                    emit("bbs" + bitStr + " " + lastCmp_.bbsZpAddr + ", " + trueTarget,
                         "single-bit test: bit " + bitStr);
                    if (!falseFallsThrough && !falseTarget.empty()) {
                        emit("bra " + falseTarget);
                    }
                    lastCmp_.valid = false;
                    lastCmp_.bbsZpAddr.clear();
                    lastCmp_.bbsBitN = -1;
                    break;
                }

                switch (lastCmp_.op) {
                    case ir::Op::CMP_EQ:
                        emit("beq " + trueTarget);
                        break;
                    case ir::Op::CMP_NE:
                        emit("bne " + trueTarget);
                        break;
                    case ir::Op::CMP_LT:
                    case ir::Op::CMP_LTU:
                        emit("bcc " + trueTarget);
                        break;
                    case ir::Op::CMP_LE:
                    case ir::Op::CMP_LEU:
                        emit("bcc " + trueTarget);
                        emit("beq " + trueTarget);
                        break;
                    case ir::Op::CMP_GT:
                    case ir::Op::CMP_GTU:
                        emit("beq " + falseTarget);
                        emit("bcs " + trueTarget);
                        falseFallsThrough = false; // already handled
                        break;
                    case ir::Op::CMP_GE:
                    case ir::Op::CMP_GEU:
                        emit("bcs " + trueTarget);
                        break;
                    default:
                        emit("bne " + trueTarget);
                        break;
                }
                if (!falseFallsThrough && !falseTarget.empty()) {
                    emit("bra " + falseTarget);
                }
                lastCmp_.valid = false;
                break;
            }

            // Load and test the condition value
            if (inst.src1.isVreg()) {
                auto alloc = alloc_.getAlloc(inst.src1.vregId);
                if (alloc.type == ir::Type::I8) {
                    // I8: load A only (no ldx #0), Z set from lda — branch directly
                    loadOperandA(inst.src1);
                    if (!trueTarget.empty()) emit("bne " + trueTarget);
                } else if (directTest(inst.src1)) {
                    // a value in memory: its bytes OR-ed (lda b0 / ora b1 ...,
                    // not loaded into A/X/Y/Z and OR-ed through scratch: #295)
                    int nb = alloc.type == ir::Type::I32 ? 4 : 2;
                    emit("lda " + directByte(inst.src1, 0));
                    for (int k = 1; k < nb; k++) emit("ora " + directByte(inst.src1, k));
                    if (!trueTarget.empty()) emit("bne " + trueTarget);
                } else {
                    // I16: need proper nonzero test.
                    // loadOperand loads A then X, but Z from ldx only tests high byte.
                    // Must test A|X for correct 16-bit nonzero check.
                    loadOperand(inst.src1);
                    emit("stx __zp_scratch");
                    emit("ora __zp_scratch");  // Z = (A|X == 0)
                    if (alloc.type == ir::Type::I32) {
                        // and the upper bytes in Y/Z
                        emit("sty __zp_scratch");
                        emit("ora __zp_scratch");
                        emit("stz __zp_scratch");
                        emit("ora __zp_scratch");
                    }
                    if (!trueTarget.empty()) emit("bne " + trueTarget);
                }
            } else if (inst.src1.kind == ir::OperandKind::IMM) {
                // a constant: one way only
                if (inst.src1.immVal) {
                    if (!trueTarget.empty()) emit("bra " + trueTarget);
                    break;
                }
            } else {
                // a global's address (16 bits: $0800 is not zero)
                loadOperand(inst.src1);
                if (ir::typeSize(inst.src1.type) >= 2) {
                    emit("stx __zp_scratch");
                    emit("ora __zp_scratch");
                } else {
                    emit("cmp #$00");
                }
                if (!trueTarget.empty()) emit("bne " + trueTarget);
            }
            if (!falseFallsThrough && !falseTarget.empty()) {
                emit("bra " + falseTarget);
            }
            break;
        }

        case ir::Op::RET: {
            // (a char / _Bool result is A alone: calling-conventions.md §2, #342)
            if (currentFn_ && currentFn_->returnType == ir::Type::I8 && inst.src1.type == ir::Type::I8) loadOperandA(inst.src1);
            else loadOperand(inst.src1);
            // Jump to common return point (endproc handles frame cleanup + rts).
            // Falling through is only safe when nothing follows in this block.
            if (nextBlockLabel_ != "__return" || peekNextInst()) {
                emit("bra @__return");
            }
            break;
        }

        case ir::Op::RET_VOID: {
            if (nextBlockLabel_ != "__return" || peekNextInst()) {
                emit("bra @__return");
            }
            break;
        }

        case ir::Op::CALL:
        case ir::Op::CALL_VOID:
        case ir::Op::CALL_INDIRECT: {
            if (inst.callConv == ir::CallConv::ZP) {
                // ZP convention: store args to the ZP parameter block ($10-$2F),
                // one slot per argument of at least 2 bytes
                int zpOff = (int)zeroPageStart_ + 8; // 8 bytes of scratch
                for (size_t argIdx = 0; argIdx < inst.args.size(); argIdx++) {
                    const auto& arg = inst.args[argIdx];
                    int ps = ir::typeSize(arg.type);
                    int slot = ps < 2 ? 2 : ps;
                    if (inst.isRegparm && argIdx == 0) {
                        // First param: in A/AX, loaded after the others
                        // (loading them goes through A/X); its slot stays free
                        zpOff += slot;
                        continue;
                    }
                    // (computed in its slot: findArgTargets)
                    if (arg.isVreg() && argTargets_.count(arg.vregId) && argTargets_[arg.vregId] == zpOff) {
                        zpOff += slot;
                        continue;
                    }
                    auto zp = [&](int b) { return "$" + hex8((uint8_t)(zpOff + b)); };
                    if (arg.type == ir::Type::F32) {
                        if (arg.isVreg()) emitF32VregToAbs(arg.vregId, zp(0));
                        zpOff += slot;
                        continue;
                    }
                    if (arg.type == ir::Type::I8) loadOperandA(arg);   // (the slot's low byte only)
                    else loadOperand(arg);
                    if (arg.type == ir::Type::I32) {
                        emit("stq " + zp(0));
                    } else if (arg.type != ir::Type::I8) {
                        emit("sta " + zp(0));
                        emit("stx " + zp(1));
                    } else {
                        emit("sta " + zp(0));
                    }
                    zpOff += slot;
                }
                // __regparm: the first argument goes in A (char) or A:X last
                if (inst.isRegparm && !inst.args.empty()) loadOperand(inst.args[0]);
            } else {
                // STACK convention: Push arguments right-to-left (C convention).
                //
                // "Simple" args (immediates, known constants, globals) can be
                // pushed directly without ZP temp round-trip.  "Complex" args
                // (vregs from computation that may reference the stack frame)
                // still use the two-phase ZP temp approach to avoid _fp corruption.
                int argBytes = 0;
                int nArgs = (int)inst.args.size();

                // Classify args: true = simple (can push inline)
                // Check if this is a SAC-to-SAC call — both caller and callee must be SAC.
                // If caller is stack-convention (e.g., variadic), use normal push + call thunk entry.
                bool isCallingSAC = currentFunctionUseSAC_ &&
                                   (inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID) &&
                                   inst.src1.kind == ir::OperandKind::GLOBAL &&
                                   sacFunctions_.count(inst.src1.name) &&
                                   !weakDefaults_.count(inst.src1.name);

                if ((inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID) &&
                    inst.src1.kind == ir::OperandKind::GLOBAL) {
                    //fprintf(stderr, "DEBUG: Call to '%s' isCallingSAC=%d (sacFunctions_size=%lu)\n",
                    //        inst.src1.name.c_str(), (int)isCallingSAC, sacFunctions_.size());
                }

                auto isSimpleArg = [&](const ir::Operand& arg) -> bool {
                    if (arg.isImm()) return true;
                    if (arg.kind == ir::OperandKind::GLOBAL) return true;
                    if (arg.isVreg() && vregConstVal_.count(arg.vregId)) return true;
                    return false;
                };

                // Check if ALL args are simple — if so, skip ZP temps entirely
                bool allSimple = true;
                for (int ai = 0; ai < nArgs; ai++) {
                    if (!isSimpleArg(inst.args[ai])) { allSimple = false; break; }
                }

                if (isCallingSAC) {
                    // SAC call: store parameters directly to parameter storage
                    // For immediate/constant VREG arguments, emit direct loads using vregConstVal_

                    std::vector<std::string> paramNames;
                    // Arguments beyond the callee's parameters (an old-style
                    // f() called as f(x)) have nowhere to go: dropped
                    int calleeParams = nArgs;
                    if (functionParameterNames_.count(inst.src1.name)) {
                        paramNames = functionParameterNames_[inst.src1.name];
                        calleeParams = std::min(nArgs, (int)paramNames.size());
                    }
                    // Fall back to indices if no parameter names available
                    while (paramNames.size() < (size_t)nArgs) {
                        paramNames.push_back(std::to_string(paramNames.size()));
                    }

                    // Process arguments right-to-left (C convention)
                    // For constant VREGs, emit direct loads using tracked constant values
                    for (int ai = calleeParams - 1; ai >= 0; ai--) {
                        const auto& arg = inst.args[ai];
                        int ps = ir::typeSize(arg.type);
                        if (ps < 2) ps = 2;

                        std::string paramSymbol = inst.src1.name + "__param_" + paramNames[ai];

                        // Check if this is a constant parameter (optimization: skip initialization)
                        bool isConstParameter = false;
                        if (sacConstParams_.count(inst.src1.name)) {
                            const auto& params = sacConstParams_[inst.src1.name];
                            if (params.count(ai) && params.at(ai).isConstant) {
                                // This parameter always receives the same constant value
                                // Skip initialization — parameter storage is pre-initialized by linker
                                isConstParameter = true;
                            }
                        }

                        // Skip both load and store for constant parameters
                        if (isConstParameter) {
                            continue;
                        }

                        if (arg.type == ir::Type::F32 && arg.isVreg()) {
                            emitF32VregToAbs(arg.vregId, paramSymbol);
                            continue;
                        }

                        // Check if we can emit a direct constant load
                        bool emittedConst = false;
                        if (arg.isImm()) {
                            // Direct immediate argument
                            emit("lda #" + std::to_string((int)(arg.immVal & 0xFF)));
                            if (arg.type == ir::Type::I32) {
                                emit("ldx #" + std::to_string((int)((arg.immVal >> 8) & 0xFF)));
                                emit("ldy #" + std::to_string((int)((arg.immVal >> 16) & 0xFF)));
                                emit("ldz #" + std::to_string((int)((arg.immVal >> 24) & 0xFF)));
                            } else if (arg.type != ir::Type::I8) {
                                emit("ldx #" + std::to_string((int)((arg.immVal >> 8) & 0xFF)));
                            }
                            emittedConst = true;
                        } else if (arg.isVreg()) {
                            // Check if this VREG has a tracked constant value
                            auto cit = vregConstVal_.find(arg.vregId);
                            if (cit != vregConstVal_.end()) {
                                // Emit direct constant load using tracked value
                                int constVal = (int)cit->second;
                                emit("lda #" + std::to_string((int)(constVal & 0xFF)));
                                if (arg.type == ir::Type::I32) {
                                    emit("ldx #" + std::to_string((int)((constVal >> 8) & 0xFF)));
                                    emit("ldy #" + std::to_string((int)((constVal >> 16) & 0xFF)));
                                    emit("ldz #" + std::to_string((int)((constVal >> 24) & 0xFF)));
                                } else if (arg.type != ir::Type::I8) {
                                    emit("ldx #" + std::to_string((int)((constVal >> 8) & 0xFF)));
                                }
                                emittedConst = true;
                            }
                        }

                        // If not a constant, load normally from the VREG
                        if (!emittedConst) {
                            loadOperand(arg);
                        }

                        // Store to parameter symbol (a char parameter: its byte
                        // alone, the slot's other byte is never read)
                        emit("sta " + paramSymbol);
                        bool charParam = arg.type == ir::Type::I8 && functionParameterTypes_.count(inst.src1.name) &&
                                         ai < (int)functionParameterTypes_[inst.src1.name].size() &&
                                         functionParameterTypes_[inst.src1.name][ai] == ir::Type::I8;
                        if (!charParam) emit("stx " + paramSymbol + "+1");
                        if (arg.type == ir::Type::I32) {
                            emit("sty " + paramSymbol + "+2");
                            emit("stz " + paramSymbol + "+3");
                        }
                    }
                    argBytes = 0;  // No stack cleanup needed
                } else if (allSimple) {
                    // Fast path: push all args inline right-to-left
                    // Use phw #imm16 for constant 16-bit args (3 bytes vs 6)
                    for (int ai = nArgs - 1; ai >= 0; ai--) {
                        if (inst.isRegparm && ai == 0) continue;
                        const auto& arg = inst.args[ai];
                        int ps = ir::typeSize(arg.type);
                        if (ps < 2) ps = 2;

                        // Get constant value if available
                        bool hasConst = false;
                        int constVal = 0;
                        if (arg.isImm()) { hasConst = true; constVal = (int)arg.immVal; }
                        else if (arg.isVreg() && vregConstVal_.count(arg.vregId)) {
                            hasConst = true; constVal = (int)vregConstVal_[arg.vregId];
                        }

                        if (hasConst && arg.type == ir::Type::I32) {
                            // I32: push high word then low word
                            pushWordConst((constVal >> 16) & 0xFFFF);
                            pushWordConst(constVal & 0xFFFF);
                        } else if (hasConst) {
                            pushWordConst(constVal & 0xFFFF);
                        } else if (arg.kind == ir::OperandKind::GLOBAL && arg.type != ir::Type::I32) {
                            pushWordAddr(arg.name);
                        } else if (arg.type == ir::Type::F32 && arg.isVreg()) {
                            // Float: push 5 bytes (push byte 4 first, byte 0 last)
                            ms_.invalidateAll();
                            for (int bi = 4; bi >= 0; bi--) {
                                emitF32Byte("lda", arg.vregId, bi);
                                emit("pha");
                            }
                        } else {
                            // Complex operand — load and push
                            loadOperand(arg);
                            emit("push .ax");
                        }
                        argBytes += ps;
                    }
                    // For regparm: load first arg into A/AX last
                    if (inst.isRegparm && nArgs > 0) {
                        const auto& arg0 = inst.args[0];
                        if (arg0.isImm()) {
                            int val = (int)arg0.immVal;
                            emit("lda #" + std::to_string(val & 0xFF));
                            if (arg0.type != ir::Type::I8)
                                emit("ldx #" + std::to_string((val >> 8) & 0xFF));
                        } else if (arg0.isVreg() && vregConstVal_.count(arg0.vregId)) {
                            int val = (int)vregConstVal_[arg0.vregId];
                            emit("lda #" + std::to_string(val & 0xFF));
                            if (arg0.type != ir::Type::I8)
                                emit("ldx #" + std::to_string((val >> 8) & 0xFF));
                        } else {
                            loadOperand(arg0);
                        }
                    }
                } else if (currentFunctionUseSAC_) {
                    // No stack frame here (static allocation): the arguments
                    // are not addressed through SP or FP, so each is pushed
                    // as it is loaded (no staging in the parameter block)
                    for (int ai = nArgs - 1; ai >= 0; ai--) {
                        if (inst.isRegparm && ai == 0) continue;
                        const auto& arg = inst.args[ai];
                        int ps = ir::typeSize(arg.type);
                        if (ps < 2) ps = 2;
                        bool hasConst = false;
                        int64_t constVal = 0;
                        if (arg.isImm()) { hasConst = true; constVal = arg.immVal; }
                        else if (arg.isVreg() && vregConstVal_.count(arg.vregId)) {
                            hasConst = true; constVal = vregConstVal_[arg.vregId];
                        }
                        if (hasConst && arg.type != ir::Type::F32) {
                            if (arg.type == ir::Type::I32) pushWordConst((int)((constVal >> 16) & 0xFFFF));
                            pushWordConst((int)(constVal & 0xFFFF));
                        } else if (arg.kind == ir::OperandKind::GLOBAL && arg.type != ir::Type::I32) {
                            pushWordAddr(arg.name);
                        } else if (arg.type == ir::Type::F32 && arg.isVreg()) {
                            ms_.invalidateAll();
                            for (int bi = 4; bi >= 0; bi--) {
                                emitF32Byte("lda", arg.vregId, bi);
                                emit("pha");
                            }
                        } else {
                            loadOperand(arg);
                            emit(arg.type == ir::Type::I32 ? "push .axyz" : "push .ax");
                        }
                        argBytes += ps;
                    }
                    if (inst.isRegparm && nArgs > 0) loadOperand(inst.args[0]);
                } else {
                    // Two-phase path: load all args into ZP temp slots BEFORE any pushes,
                    // so frame-relative loads aren't affected by SP/fp changes.
                    // Compute per-slot offsets based on actual arg sizes
                    std::vector<int> slotOffsets(nArgs, 0);
                    {
                        int off = 0;
                        for (int ai = nArgs - 1; ai >= 0; ai--) {
                            int slot = nArgs - 1 - ai;
                            slotOffsets[slot] = off;
                            int sz = ir::typeSize(inst.args[ai].type);
                            if (sz < 2) sz = 2;
                            off += sz;
                        }
                    }
                    auto zpSlotAddr = [&](int idx, int byteOff) {
                        std::stringstream ss;
                        ss << "$" << std::hex << std::uppercase << std::setfill('0')
                           << std::setw(2) << ((int)zeroPageStart_ + 8 + slotOffsets[idx] + byteOff);  // parameter block
                        return ss.str();
                    };
                    // Phase 1: load all args (right-to-left) into ZP scratch slots
                    for (int ai = nArgs - 1; ai >= 0; ai--) {
                        const auto& arg = inst.args[ai];
                        int slot = nArgs - 1 - ai;
                        if (arg.type == ir::Type::F32 && arg.isVreg()) {
                            // F32: copy 5 bytes from vreg to scratch slot
                            ms_.invalidateAll();
                            for (int bi = 0; bi < 5; bi++) {
                                emitF32Byte("lda", arg.vregId, bi);
                                emit("sta " + zpSlotAddr(slot, bi));
                            }
                        } else {
                            loadOperand(arg);
                            emit("sta " + zpSlotAddr(slot, 0));
                            emit("stx " + zpSlotAddr(slot, 1));
                            if (arg.type == ir::Type::I32) {
                                emit("sty " + zpSlotAddr(slot, 2));
                                emit("stz " + zpSlotAddr(slot, 3));
                            }
                        }
                    }
                    // Phase 2: push from ZP slots (same order — already right-to-left)
                    for (int i = 0; i < nArgs; i++) {
                        int origIdx = nArgs - 1 - i;
                        if (inst.isRegparm && origIdx == 0) continue;
                        const auto& arg = inst.args[origIdx];
                        int ps = ir::typeSize(arg.type);
                        if (ps < 2) ps = 2;
                        if (arg.type == ir::Type::F32) {
                            // F32: push 5 bytes (byte 4 first, byte 0 last)
                            for (int bi = 4; bi >= 0; bi--) {
                                emit("lda " + zpSlotAddr(i, bi));
                                emit("pha");
                            }
                        } else if (arg.type == ir::Type::I32) {
                            emit("ldq " + zpSlotAddr(i, 0));  // LDZ has no base-page mode
                            emit("push .axyz");
                        } else {
                            emit("lda " + zpSlotAddr(i, 0));
                            emit("ldx " + zpSlotAddr(i, 1));
                            emit("push .ax");
                        }
                        argBytes += ps;
                    }
                    // For regparm: load first arg into A/AX last
                    if (inst.isRegparm && nArgs > 0) {
                        int slot0 = nArgs - 1;
                        emit("lda " + zpSlotAddr(slot0, 0));
                        if (inst.args[0].type != ir::Type::I8)
                            emit("ldx " + zpSlotAddr(slot0, 1));
                    }
                }
                
                // JSR — SAC callers use __sac entry point to skip thunk
                if (inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID) {
                    if (inst.src1.kind == ir::OperandKind::GLOBAL) {
                        std::string target = inst.src1.name;
                        if (isCallingSAC) target += "__sac";
                        emit("jsr " + target);
                    }
                } else {
                    // CALL_INDIRECT
                    loadOperand(inst.src1);
                    std::string callLabel = "@__call_site_" + std::to_string(labelCounter_++);
                    emit("sta " + callLabel + "+1");
                    emit("stx " + callLabel + "+2");
                    emitLabel(callLabel);
                    emit("jsr $0000");
                }

                // Caller-side stack cleanup: skip for SAC calls (no stack parameters were used)
                // For non-SAC: save result to ZP scratch, pop parameters, restore result
                if (argBytes > 0 && !isCallingSAC) {
                    // PLZ changes only Z: a result in A/X needs no saving; a
                    // long (A/X/Y/Z) is saved and reloaded with LDQ
                    bool needZ = (inst.op != ir::Op::CALL_VOID && inst.resultType == ir::Type::I32);
                    bool needY = false;
                    if (needZ) emit("stq __zp_scratch4");

                    // Pop parameters using PLZ (1 byte per PLZ instruction)
                    for (int i = 0; i < argBytes; i++) {
                        emit("plz");
                    }

                    // Restore result from __zp_scratch4 (LDQ: LDZ has no base-page mode)
                    if (needZ) emit("ldq __zp_scratch4");
                    (void)needY;
                }

                // NOTE: Do NOT recalculate frame pointer after function calls!
                // The frame-relative offsets (.local directives) are calculated based on the
                // frame pointer value at function ENTRY. If we recalculate the FP here, those
                // offsets become invalid and frame-relative dereferencing reads from wrong addresses.
                // The frame pointer must remain stable throughout the entire function.
                //
                // Previous implementation recalculated FP here (tsy; tsx; inx; etc.),
                // which broke frame-relative addressing for variables accessed after function calls.
            }

            // If it's a direct call and we didn't jsr yet (ZP case)
            if (inst.callConv == ir::CallConv::ZP && (inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID)) {
                if (inst.src1.kind == ir::OperandKind::GLOBAL) {
                    // (its arguments are in $10-$2F: X is not one, so a load
                    // of X just before is dead: pruneDeadLdx, Issue #296)
                    // (only when the call clobbers X: else what is known of X
                    // carries past the call, and the load is relied on)
                    auto cm = functionClobberMasks_.find(inst.src1.name);
                    bool clobbersX = cm == functionClobberMasks_.end() || (cm->second & (1 << REG_X));
                    if (!inst.isRegparm && clobbersX) emitComment(kXFreeCall);
                    emit("jsr " + inst.src1.name);
                }
            } else if (inst.callConv == ir::CallConv::ZP && inst.op == ir::Op::CALL_INDIRECT) {
                // Through a function pointer: self-modified JSR (as for stack calls)
                loadOperand(inst.src1);
                std::string callLabel = "@__call_site_" + std::to_string(labelCounter_++);
                emit("sta " + callLabel + "+1");
                emit("stx " + callLabel + "+2");
                emitLabel(callLabel);
                emit("jsr $0000");
            }

            // Invalidate frame pointer cache after function calls
            // Stack pointer temporarily changes during call handling (save return value, etc.)
            // BUT: Skip invalidation for terminal leaf functions with no parameters
            // (they don't push anything, so stack pointer never changes)
            if (inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID || inst.op == ir::Op::CALL_INDIRECT) {
                bool shouldInvalidate = true;

                // For direct calls, check if it's a terminal leaf with no parameters
                if (inst.src1.kind == ir::OperandKind::GLOBAL && !functionMap_.empty()) {
                    auto it = functionMap_.find(inst.src1.name);
                    if (it != functionMap_.end()) {
                        const ir::Function* calledFn = it->second;
                        // Check: no parameters + no calls made (leaf function)
                        FuncClobbers clobbers = computeFuncClobbers(*calledFn);
                        if (calledFn->paramTypes.empty() && clobbers.isLeaf) {
                            shouldInvalidate = false;  // No stack changes for terminal leaf with no params
                        }
                    }
                }

                if (shouldInvalidate) {
                    frameAddrCacheValid_ = false;
                }
            }

            // Phase 2: Selective register and flag invalidation (fine-grained clobber tracking)
            // Instead of invalidating all registers/flags, only invalidate those the function actually clobbers
            if (inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID || inst.op == ir::Op::CALL_INDIRECT) {
                int regClobberMask = 0;
                int flagClobberMask = 0;
                const int ALL_REGS = (1 << REG_A) | (1 << REG_X) | (1 << REG_Y) | (1 << REG_Z);
                const int ALL_FLAGS = 0x0F;  // All 4 flags

                // For direct calls, look up the function's clobber masks
                if (inst.src1.kind == ir::OperandKind::GLOBAL) {
                    auto it = functionClobberMasks_.find(inst.src1.name);
                    if (it != functionClobberMasks_.end()) {
                        regClobberMask = it->second;
                    } else {
                        // External function or function not found: conservatively assume all registers clobbered
                        regClobberMask = ALL_REGS;
                    }

                    // Look up flag clobber mask for the same function
                    auto fit = functionFlagClobberMasks_.find(inst.src1.name);
                    if (fit != functionFlagClobberMasks_.end()) {
                        flagClobberMask = fit->second;
                    } else {
                        // External function or not found: conservatively assume all flags clobbered
                        flagClobberMask = ALL_FLAGS;
                    }
                } else {
                    // Indirect call: conservatively assume all registers and flags clobbered
                    regClobberMask = ALL_REGS;
                    flagClobberMask = ALL_FLAGS;
                }

                // Selectively invalidate only the clobbered registers and flags
                ms_.invalidateSelectiveWithFlags(regClobberMask, flagClobberMask);
            }

            // Return value is in A,X,Y,Z (for 32-bit) or A:X (for 16-bit)
            // Skip storeVreg if the return value is never used (dead vreg).
            if (inst.op != ir::Op::CALL_VOID && inst.dest.isVreg()) {
                bool isUsed = false;
                if (currentFn_) {
                    for (const auto& blk : currentFn_->blocks) {
                        for (const auto& ii : blk.insts) {
                            if (&ii == &inst) continue;
                            if (ii.src1.isVreg() && ii.src1.vregId == inst.dest.vregId) { isUsed = true; break; }
                            if (ii.src2.isVreg() && ii.src2.vregId == inst.dest.vregId) { isUsed = true; break; }
                            for (const auto& a : ii.args)
                                if (a.isVreg() && a.vregId == inst.dest.vregId) { isUsed = true; break; }
                            if (isUsed) break;
                        }
                        if (isUsed) break;
                    }
                } else {
                    isUsed = true; // conservative: assume used if no function context
                }
                // used only by the next instruction, a store into a variable:
                // the store takes the result from A/X (store forwarding)
                if (isUsed && (inst.resultType == ir::Type::I8 || inst.resultType == ir::Type::I16 ||
                               inst.resultType == ir::Type::PTR)) {
                    const ir::Inst* nx = peekNextInst();
                    if (nx && nx->op == ir::Op::STORE && nx->src1.isVreg() && nx->src1.vregId == inst.dest.vregId &&
                        nx->src2.isVreg() && localSlotVregs_.count(nx->src2.vregId) && nx->args.empty() &&
                        nx->resultType == inst.resultType &&
                        alloc_.getAlloc(inst.dest.vregId).loc == VRegAllocator::IN_ZP &&
                        !vregUsedOutside(inst.dest.vregId, &inst, nx)) {
                        resultInAX_ = (int32_t)inst.dest.vregId;
                        isUsed = false;
                    }
                }
                if (isUsed) storeVreg(inst.dest.vregId);
            }
            break;
        }

        case ir::Op::SEXT: {
            loadOperand(inst.src1);
            if (inst.src1.type == ir::Type::I8 && inst.resultType == ir::Type::I16) {
                emit("sxt.8");
            } else if (inst.src1.type == ir::Type::I8 && inst.resultType == ir::Type::I32) {
                emit("sxt.8");   // byte -> word, then word -> long
                emit("sxt.16");
            } else if (inst.src1.type == ir::Type::I16 && inst.resultType == ir::Type::I32) {
                emit("sxt.16");
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::ZEXT: {
            // a byte widened only for an element index kept as name+off,y
            if (inst.dest.isVreg() && zextSkip_.count(inst.dest.vregId)) break;
            if (inst.dest.isVreg() && zextAlias_.count(inst.dest.vregId)) {
                if (!directByte(inst.src1, 0).empty()) break;   // read by its one use
                zextAlias_.erase(inst.dest.vregId);             // (a stack frame)
            }
            loadOperand(inst.src1);
            if (inst.src1.type == ir::Type::I8) {
                emit("ldx #0"); // zero-extend: clear high byte
            }
            if (inst.resultType == ir::Type::I32 && inst.src1.type != ir::Type::I32) {
                emit("ldy #0");
                emit("ldz #0");
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::TRUNC: {
            if (inst.dest.isVreg() && truncAlias_.count(inst.dest.vregId)) {
                if (!directByte(inst.src1, 0).empty()) break;   // read by its STORE
                truncAlias_.erase(inst.dest.vregId);            // (a stack frame)
            }
            loadOperand(inst.src1);
            // Truncation: just keep the low byte(s) — already in A
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::BFEXT: {
            loadOperand(inst.src1);
            int offset = (int)inst.args[0].immVal;
            int width = (int)inst.args[1].immVal;
            if (inst.src1.type == ir::Type::I32) {
                emit("bfext32 #" + std::to_string(offset) + ", #" + std::to_string(width));
            } else if (inst.src1.type == ir::Type::I16) {
                emit("bfext16 #" + std::to_string(offset) + ", #" + std::to_string(width));
            } else {
                emit("bfext #" + std::to_string(offset) + ", #" + std::to_string(width));
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::BFINS: {
            // src1: new field value, src2: address of storage unit
            // dest: vreg to hold the modified value
            // NOTE: Issue #193 fix — use dedicated ZP slot for frame address pointer
            // to prevent collision with bitfield value temporaries
            std::string addrStr;
            bool indirect = false;
            if (inst.src2.kind == ir::OperandKind::GLOBAL) {
                addrStr = inst.src2.name;
            } else {
                loadVreg(inst.src2.vregId);
                // Use __zp_scratch3 for frame address (reserved, won't be reused for temps)
                emit("sta __zp_scratch3");
                emit("stx __zp_scratch3+1");
                addrStr = "__zp_scratch3";
                indirect = true;
            }
            loadOperand(inst.src1); // AX = new value
            int offset = (int)inst.args[0].immVal;
            int width = (int)inst.args[1].immVal;
            std::string suffix = indirect ? ".ind" : "";
            if (inst.resultType == ir::Type::I32) {
                emit("bfins32" + suffix + " " + addrStr + ", #" + std::to_string(offset) + ", #" + std::to_string(width));
            } else if (inst.resultType == ir::Type::I16) {
                emit("bfins16" + suffix + " " + addrStr + ", #" + std::to_string(offset) + ", #" + std::to_string(width));
            } else {
                emit("bfins" + suffix + " " + addrStr + ", #" + std::to_string(offset) + ", #" + std::to_string(width));
            }
            // bfins modifies in-place and leaves result in A/AX; store to dest vreg
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::ASM_INLINE: {
            emit(inst.asmText);
            break;
        }

        case ir::Op::VA_START: {
            // Compute stack address of first variadic argument.
            // Variadic functions always receive ALL args on the stack (even in zpCall mode,
            // the caller pushes everything for variadic calls). So named params are on the
            // stack at their proc-declared offsets, and variadic args follow.
            std::string pName = inst.asmText;
            // Step over the last named parameter's stack slot: 2 bytes for
            // char/int/pointers (char is promoted), 4 for long, 5 for float
            int slot = 2;
            if (currentFn_) {
                for (size_t i = 0; i < currentFn_->paramNames.size() && i < currentFn_->paramTypes.size(); i++) {
                    if (currentFn_->paramNames[i] == pName) {
                        int sz = ir::typeSize(currentFn_->paramTypes[i]);
                        slot = sz < 2 ? 2 : sz;
                        break;
                    }
                }
            }
            if (currentFunctionUseSAC_) {
                // static allocation: the entry thunk kept the address
                emit("lda " + currentFunctionName_ + "__va");
                emit("ldx " + currentFunctionName_ + "__va+1");
            } else {
                emit("leax.param @_p_" + pName);
                emit("add.16 .AX, #" + std::to_string(slot));
            }
            // A/X now hold the address, not whatever vreg they held before
            resultInAX_ = -1;
            ms_.invalidateAll();
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::CPU_REG_READ: {
            std::string reg = inst.asmText;
            std::transform(reg.begin(), reg.end(), reg.begin(), ::toupper);
            if (reg == "A") ;
            else if (reg == "X") emit("txa");
            else if (reg == "Y") emit("tya");
            else if (reg == "Z") emit("tza");
            else if (reg == "AX") ;
            else if (reg == "AY") { emit("phy"); emit("plx"); }
            else if (reg == "AZ") { emit("phz"); emit("plx"); }
            else if (reg == "XY") { emit("txa"); emit("phy"); emit("plx"); emit("pla"); } // Swap AX/XY
            else if (reg == "RETADDR") { emit("lda __zp_scratch3"); emit("ldx __zp_scratch3+1"); }
            else if (reg == "SP") { emit("tsx"); emit("tsy"); emit("txa"); emit("phy"); emit("plx"); }  // A = SPL, X = SPH
            else if (reg == "Q" || reg == "AXYZ") ; // already in AXYZ
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::CPU_REG_WRITE: {
            std::string reg = inst.asmText;
            std::transform(reg.begin(), reg.end(), reg.begin(), ::toupper);
            
            bool optimized = false;
            if (inst.src1.isImm()) {
                uint32_t val = (uint32_t)inst.src1.immVal;
                if (reg == "A") { emit("lda #$" + hex8((uint8_t)val)); optimized = true; }
                else if (reg == "X") { emit("ldx #$" + hex8((uint8_t)val)); optimized = true; }
                else if (reg == "Y") { emit("ldy #$" + hex8((uint8_t)val)); optimized = true; }
                else if (reg == "Z") { emit("ldz #$" + hex8((uint8_t)val)); optimized = true; }
                else if (reg == "AX") { emit("ldax #$" + hex16((uint16_t)val)); optimized = true; }
                else if (reg == "AY") { emit("lday #$" + hex16((uint16_t)val)); optimized = true; }
                else if (reg == "AZ") { emit("ldaz #$" + hex16((uint16_t)val)); optimized = true; }
                else if (reg == "XY") { emit("ldxy #$" + hex16((uint16_t)val)); optimized = true; }
                else if (reg == "Q" || reg == "AXYZ") { emit("ldq #$" + hex32(val)); optimized = true; }
            } else if (inst.src1.isVreg()) {
                // A constant, base-page or static source loads straight into
                // the register, leaving the others (A among them) alone
                bool wide = inst.src1.type != ir::Type::I8;
                std::string b0 = directByte(inst.src1, 0);
                std::string b1 = wide ? directByte(inst.src1, 1) : std::string("#0");
                // LDZ has no base-page mode: a base-page source goes through A, preserving A
                auto ldz = [&](const std::string& b) {
                    if (b[0] == '$' && b.size() <= 3) { emit("pha"); emit("lda " + b); emit("taz"); emit("pla"); }
                    else emit("ldz " + b);
                };
                if (!b0.empty() && !b1.empty()) {
                    optimized = true;
                    if (reg == "A") emit("lda " + b0);
                    else if (reg == "X") emit("ldx " + b0);
                    else if (reg == "Y") emit("ldy " + b0);
                    else if (reg == "Z") ldz(b0);
                    else if (reg == "AX") { emit("lda " + b0); emit("ldx " + b1); }
                    else if (reg == "AY") { emit("lda " + b0); emit("ldy " + b1); }
                    else if (reg == "AZ") { ldz(b1); emit("lda " + b0); }
                    else if (reg == "XY") { emit("ldx " + b0); emit("ldy " + b1); }
                    else optimized = false;
                }
                auto alloc = alloc_.getAlloc(inst.src1.vregId);
                if (!optimized && (reg == "Q" || reg == "AXYZ") && alloc.loc == VRegAllocator::IN_ZP) {
                    emit("ldq $" + hex8(alloc.offset)); optimized = true;
                }
            }

            if (!optimized) {
                if (inst.src1.isVreg()) loadVreg(inst.src1.vregId);
                else if (inst.src1.isImm()) {
                    if (inst.src1.type == ir::Type::I16) emit("ldax #$" + hex16((uint16_t)inst.src1.immVal));
                    else if (inst.src1.type == ir::Type::I32) emit("ldq #$" + hex32((uint32_t)inst.src1.immVal));
                    else emit("lda #$" + hex8((uint8_t)inst.src1.immVal));
                }
                if (reg == "A") ;
                else if (reg == "X") emit("tax");
                else if (reg == "Y") emit("tay");
                else if (reg == "Z") emit("taz");
                else if (reg == "AX") ;
                else if (reg == "AY") { emit("phx"); emit("ply"); }
                else if (reg == "AZ") { emit("phx"); emit("plz"); }
                else if (reg == "XY") { emit("phx"); emit("ply"); emit("tax"); }
                // SP = AX: low byte first, then the high byte (raising SP:
                // the in-between value stays within the area being freed)
                else if (reg == "SP") { emit("phx"); emit("ply"); emit("tax"); emit("txs"); emit("tys"); }
            }
            break;
        }

        case ir::Op::CPU_FLAG_READ: {
            std::string flag = inst.asmText;
            std::transform(flag.begin(), flag.end(), flag.begin(), ::toupper);
            emit("lda #0");
            if (flag == "CARRY") emit("adc #0");
            else if (flag == "ZERO") { emit("bne *+3"); emit("lda #1"); }
            else if (flag == "NEGATIVE") { emit("bpl *+3"); emit("lda #1"); }
            else if (flag == "OVERFLOW") { emit("bvc *+3"); emit("lda #1"); }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::CPU_FLAG_WRITE: {
            std::string flag = inst.asmText;
            std::transform(flag.begin(), flag.end(), flag.begin(), ::toupper);

            bool optimized = false;
            if (inst.src1.isImm()) {
                int val = (int)inst.src1.immVal;
                if (flag == "CARRY") { emit(val ? "sec" : "clc"); optimized = true; }
                else if (flag == "DECIMAL") { emit(val ? "sed" : "cld"); optimized = true; }
                else if (flag == "INTERRUPT") { emit(val ? "sei" : "cli"); optimized = true; }
                else if (flag == "OVERFLOW" && val == 0) { emit("clv"); optimized = true; }
            }

            if (!optimized) {
                if (inst.src1.isVreg()) loadVreg(inst.src1.vregId);
                else if (inst.src1.isImm()) emit("lda #$" + hex8((uint8_t)inst.src1.immVal));
                if (flag == "CARRY") { emit("lsr a"); }
                else if (flag == "ZERO") { emit("cmp #0"); }
            }
            break;
        }

        case ir::Op::PHI: {
            // PHI nodes are not directly executable in linear code.
            // For now, skip — the IRBuilder's control flow already sets
            // values before branching.
            break;
        }

        // Float operations
        case ir::Op::FCONST: {
            int64_t bits = inst.src1.immVal; double val;
            std::memcpy(&val, &bits, sizeof(double));
            uint8_t cbm[5]; doubleToCBM40(val, cbm);
            for (int i = 0; i < 5; i++) {
                emit("lda #$" + hex8(cbm[i]));
                emitF32Byte("sta", inst.dest.vregId, i);
            }
            break;
        }
        case ir::Op::FADD: case ir::Op::FSUB:
        case ir::Op::FMUL: case ir::Op::FDIV:
        case ir::Op::FNEG: case ir::Op::FCMP:
        case ir::Op::ITOF: case ir::Op::FTOI: {
            // Helper: copy 5-byte float from vreg to absolute symbol (e.g., __float_a)
            // Uses byte-by-byte LDA/STA (DMA MOVE has ZP clobber issues).
            // Invalidates machine state to prevent optimizer from eliminating loads.
            auto emitFloatToAbs = [&](const VRegAllocator::Allocation&, uint32_t vregId, const std::string& dst) {
                emitF32VregToAbs(vregId, dst);
            };
            // Helper: copy 5-byte float from absolute symbol to vreg
            auto emitAbsToFloat = [&](const std::string& src, const VRegAllocator::Allocation&, uint32_t vregId) {
                emitF32AbsToVreg(src, vregId);
            };

            if (inst.op >= ir::Op::FADD && inst.op <= ir::Op::FDIV) {
                auto s1 = alloc_.getAlloc(inst.src1.vregId);
                auto s2 = alloc_.getAlloc(inst.src2.vregId);
                emitFloatToAbs(s1, inst.src1.vregId, "__float_a");
                emitFloatToAbs(s2, inst.src2.vregId, "__float_b");
                if (inst.op == ir::Op::FADD) emit("jsr __float_add");
                else if (inst.op == ir::Op::FSUB) emit("jsr __float_sub");
                else if (inst.op == ir::Op::FMUL) emit("jsr __float_mul");
                else emit("jsr __float_div");
                auto da = alloc_.getAlloc(inst.dest.vregId);
                emitAbsToFloat("__float_a", da, inst.dest.vregId);
            } else if (inst.op == ir::Op::FNEG) {
                auto s1 = alloc_.getAlloc(inst.src1.vregId);
                emitFloatToAbs(s1, inst.src1.vregId, "__float_a");
                emit("jsr __float_neg");
                auto da = alloc_.getAlloc(inst.dest.vregId);
                emitAbsToFloat("__float_a", da, inst.dest.vregId);
            } else if (inst.op == ir::Op::FCMP) {
                auto s1 = alloc_.getAlloc(inst.src1.vregId);
                auto s2 = alloc_.getAlloc(inst.src2.vregId);
                emitFloatToAbs(s1, inst.src1.vregId, "__float_a");
                emitFloatToAbs(s2, inst.src2.vregId, "__float_b");
                emit("jsr __float_cmp");
                auto da = alloc_.getAlloc(inst.dest.vregId);
                if (da.loc == VRegAllocator::IN_ZP) {
                    emit("sta $" + hex8((uint8_t)da.offset));
                } else if (currentFunctionUseSAC_) {
                    emit("sta " + currentFunctionName_ + "__local_" + std::to_string(inst.dest.vregId));
                } else {
                    emit("ldy #" + std::to_string(da.offset));
                    emit("sta ($fd),y");
                }
            } else if (inst.op == ir::Op::ITOF) {
                // Integer -> float: 8/16/32-bit, signed or unsigned (src2)
                bool sgn = inst.src2.isImm() ? inst.src2.immVal != 0 : true;
                ir::Type st = inst.src1.type;
                loadOperand(inst.src1);
                if (st == ir::Type::I32) {
                    emit(sgn ? "jsr __float_ltof" : "jsr __float_ultof");
                } else if (st == ir::Type::I8) {
                    emit("ldx #0");
                    if (sgn) {
                        std::string l = "@__itof_" + std::to_string(labelCounter_++);
                        emit("cmp #$80");
                        emit("bcc " + l);
                        emit("ldx #$FF");
                        emitLabel(l);
                        emit("jsr __float_itof");
                    } else {
                        emit("jsr __float_utof");
                    }
                } else {
                    emit((sgn && st != ir::Type::PTR) ? "jsr __float_itof" : "jsr __float_utof");
                }
                auto da = alloc_.getAlloc(inst.dest.vregId);
                emitAbsToFloat("__float_a", da, inst.dest.vregId);
            } else { // FTOI
                // Float -> integer, truncated toward zero: 32 bits in A/X/Y/Z
                emitFloatToAbs(alloc_.getAlloc(inst.src1.vregId), inst.src1.vregId, "__float_a");
                emit("jsr __float_ftol");
                if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            }
            break;
        }

        default:
            throw std::runtime_error("IRCodeGen: unimplemented IR opcode " +
                std::to_string((int)inst.op) + " — check IR.hpp for opcode definition and implement in emitInst()");
            break;
    }
}

int IRCodeGen::allocateZP(int size) {
    for (int i = 0; i <= (int)zpRegs_.size() - size; ++i) {
        bool found = true;
        for (int j = 0; j < size; ++j)
            if (zpRegs_[i + j].inUse) { found = false; break; }
        if (found) {
            for (int j = 0; j < size; ++j) zpRegs_[i + j].inUse = true;
            return i;
        }
    }
    int oldSize = zpRegs_.size();
    for (int i = 0; i < size; ++i) zpRegs_.push_back({true});
    return oldSize;
}

void IRCodeGen::freeZP(int index, int size) {
    for (int i = 0; i < size; ++i)
        if (index + i < (int)zpRegs_.size())
            zpRegs_[index + i].inUse = false;
}

std::string IRCodeGen::zpAddr(int index) const {
    std::stringstream ss;
    ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2)
       << ((zeroPageStart_ + index * 1) & 0xFF);
    return ss.str();
}

// Load frame address into A:X, either from cache or use leax.fp pseudo-instruction
// If frameAddrZPIndex_ >= 0 and cache is valid, load from allocated ZP slot
// Otherwise use leax.fp which references the cached value in $FD/$FE (for stack params)
// or recalculates fresh (for zpCall mode)
void IRCodeGen::loadFrameAddr(int offset) {
    if (frameAddrZPIndex_ >= 0 && frameAddrCacheValid_) {
        // Use cached frame pointer from allocated ZP slot (valid before any function calls)
        std::string frameAddrZP = zpAddr(frameAddrZPIndex_);
        if (offset == 0) {
            emit("lda " + frameAddrZP);
            emit("ldx " + frameAddrZP + "+1");
        } else {
            emit("lda " + frameAddrZP);
            emit("ldx " + frameAddrZP + "+1");
            emit("clc");
            emit("adc #" + std::to_string(offset & 0xFF));
            emit("pha");
            emit("txa");
            emit("adc #" + std::to_string((offset >> 8) & 0xFF));
            emit("tax");
            emit("pla");
        }
    } else {
        // Cache is invalid (after function calls)
        // Use leax.fp pseudo-instruction which handles both:
        // - Stack mode: uses recalculated $FD/$FE from return value handling
        // - ZpCall mode: computes fresh from current stack pointer
        if (offset == 0) {
            emit("leax.local 0");
        } else {
            emit("leax.local " + std::to_string(offset));
        }
    }
}

// Get optimal ZP address pair for indirect addressing
// If vreg is a frame address at offset 0 and we have a cache, use cached location directly
// Otherwise, load vreg and return location where it was stored

// Does the function assign its parameter or take its address? Its storage
// then cannot be preset to a constant the callers no longer store
static bool paramWritten(const ir::Module& mod, const std::string& funcName, size_t paramIdx) {
    for (const auto& fn : mod.functions) {
        if (fn.name != funcName) continue;
        if (paramIdx >= fn.paramNames.size()) return true;
        auto it = fn.localNames.find(fn.paramNames[paramIdx]);
        if (it == fn.localNames.end()) return true;
        uint32_t v = it->second;
        for (const auto& block : fn.blocks)
            for (const auto& inst : block.insts) {
                if (inst.op == ir::Op::STORE && inst.src2.isVreg() && inst.src2.vregId == v) return true;
                if (inst.op == ir::Op::ADDR_LOCAL && inst.src1.isVreg() && inst.src1.vregId == v) return true;
                if (inst.op == ir::Op::ADDR_UPLEVEL) return true;
                if (inst.dest.isVreg() && inst.dest.vregId == v) return true;
            }
        return false;
    }
    return true;
}

// Analyze all CALL instructions to detect SAC functions with constant parameters
void IRCodeGen::analyzeConstantParameters(const ir::Module& mod) {
    // First pass: build a map of VREG ID → constant value for each function
    std::map<std::string, std::map<uint32_t, int64_t>> vregConstVals;

    for (const auto& fn : mod.functions) {
        for (const auto& block : fn.blocks) {
            for (const auto& inst : block.insts) {
                // Track CONST instructions to build vregConstVals map
                if (inst.op == ir::Op::CONST && inst.dest.isVreg()) {
                    vregConstVals[fn.name][inst.dest.vregId] = inst.src1.immVal;
                }
            }
        }
    }

    // Second pass: collect parameter values from all call sites
    std::map<std::string, std::vector<std::map<int64_t, int>>> paramValueFreq;

    for (const auto& fn : mod.functions) {
        for (const auto& block : fn.blocks) {
            for (const auto& inst : block.insts) {
                if ((inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID) &&
                    sacFunctions_.count(inst.src1.name) && !weakDefaults_.count(inst.src1.name)) {

                    // This is a call to a SAC function
                    const std::string& calleeFunc = inst.src1.name;
                    int paramIdx = 0;

                    // Process each argument
                    for (const auto& arg : inst.args) {
                        if (paramValueFreq[calleeFunc].size() <= (size_t)paramIdx) {
                            paramValueFreq[calleeFunc].resize(paramIdx + 1);
                        }

                        // Check if argument is a constant
                        if (arg.isImm()) {
                            paramValueFreq[calleeFunc][paramIdx][(int64_t)arg.immVal]++;
                        } else if (arg.isVreg() && vregConstVals[fn.name].count(arg.vregId)) {
                            // VREG with tracked constant value
                            int64_t constVal = vregConstVals[fn.name][arg.vregId];
                            paramValueFreq[calleeFunc][paramIdx][constVal]++;
                        } else {
                            // Non-constant parameter
                            paramValueFreq[calleeFunc][paramIdx][-999999]++;
                        }
                        paramIdx++;
                    }
                }
            }
        }
    }

    // Only a function whose every caller is in this file may have constant
    // parameters: a static function whose address is never taken. Another
    // module (or an indirect call) enters through the normal entry, which
    // writes the parameter storage; later calls from here would skip the
    // store and read that value.
    std::set<std::string> referenced;  // names used other than as a direct call target
    auto noteOperand = [&](const ir::Operand& o) {
        if (!o.name.empty()) referenced.insert(o.name);
    };
    for (const auto& fn : mod.functions) {
        for (const auto& block : fn.blocks) {
            for (const auto& inst : block.insts) {
                bool isCall = inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID;
                if (!isCall) noteOperand(inst.src1);
                noteOperand(inst.src2);
                noteOperand(inst.dest);
                for (const auto& a : inst.args) noteOperand(a);
                if (!inst.asmText.empty()) {
                    for (const auto& f2 : mod.functions)
                        if (inst.asmText.find(f2.name) != std::string::npos) referenced.insert(f2.name);
                }
            }
        }
    }
    for (const auto& g : mod.globals) {
        for (const auto& l : g.initLabels) referenced.insert(l);
        for (const auto& m : g.vtableMethodNames) referenced.insert(m);
        for (const auto& [off, w] : g.initWords) referenced.insert(w.substr(0, w.find_first_of("+-", 1)));
    }
    auto allCallersVisible = [&](const std::string& funcName) {
        for (const auto& fn : mod.functions)
            if (fn.name == funcName)
                return fn.isStatic && !fn.isWeak && !referenced.count(funcName);
        return false;
    };

    // Third pass: for each SAC function, check if each parameter always gets the same constant
    for (const auto& [funcName, paramFreqs] : paramValueFreq) {
        if (!allCallersVisible(funcName)) continue;
        for (size_t paramIdx = 0; paramIdx < paramFreqs.size(); paramIdx++) {
            const auto& freqMap = paramFreqs[paramIdx];

            // If this parameter has exactly one value across all call sites
            // AND that value is not the non-constant marker, it's a constant parameter
            if (freqMap.size() == 1 && freqMap.begin()->first != -999999 &&
                !paramWritten(mod, funcName, paramIdx)) {
                sacConstParams_[funcName][(int)paramIdx].isConstant = true;
                sacConstParams_[funcName][(int)paramIdx].value = freqMap.begin()->first;
            }
        }
    }
}

// Detect which functions are leaves (don't call any other functions)
// Phase 2, Phase 3: Use original leaf status from AST instead of post-optimization IR
// This ensures leaves are detected before inlining transforms the code
void IRCodeGen::detectLeafFunctions(const ir::Module& mod) {
    for (const auto& fn : mod.functions) {
        // Use the originalIsLeaf flag computed by IRBuilder from the original AST
        // This is more reliable than checking IR after inlining has removed calls
        if (fn.originalIsLeaf) {
            leafFunctions_.insert(fn.name);
        }
    }
}

// Detect zero-alloc leaf functions: leaf + no locals + all constant params
void IRCodeGen::detectZeroAllocLeaves(const ir::Function& fn) {
    // Must be a leaf function
    if (!leafFunctions_.count(fn.name)) return;
    
    // Must have no local variables (or only register-allocated ones)
    bool hasLocals = false;
    for (const auto& [vregId, offset] : vregOffset_) {
        auto alloc = alloc_.getAlloc(vregId);
        if (alloc.loc == VRegAllocator::IN_FRAME) {
            hasLocals = true;
            break;
        }
    }
    if (hasLocals) return;
    
    // If no parameters, this is a pure zero-alloc leaf
    if (fn.paramTypes.empty()) {
        zeroAllocLeaves_.insert(fn.name);
        return;
    }
    
    // Check if ALL parameters are constant
    if (!sacConstParams_.count(fn.name)) return;
    
    const auto& params = sacConstParams_[fn.name];
    for (size_t i = 0; i < fn.paramTypes.size(); i++) {
        if (!params.count((int)i) || !params.at((int)i).isConstant) {
            return;  // Found a non-constant parameter
        }
    }
    
    // All checks passed - this is a zero-alloc leaf
    zeroAllocLeaves_.insert(fn.name);
}

// Phase 53: Generate specialization name from pattern
// E.g., _calculate + {10, 2} → _calculate_10_2
std::string IRCodeGen::generateSpecializationName(const std::string& funcName, const SpecializationPattern& pattern) const {
    std::string name = funcName;
    for (size_t i = 0; i < pattern.size(); i++) {
        name += "_" + std::to_string(pattern[i]);
    }
    return name;
}

// Phase 51: Zero-alloc leaf detection
// Returns true if ALL parameters of a SAC function are constant (no AR allocation needed)
bool IRCodeGen::isZeroAllocLeaf(const std::string& funcName, const ir::Function& fn) const {
    // Only applies to SAC functions with parameters
    if (fn.paramTypes.empty()) {
        return true;  // Functions with no parameters are zero-alloc leaves
    }

    // Check if all parameters are in specializedParams
    auto it = specializedParams_.find(funcName);
    if (it == specializedParams_.end()) {
        return false;  // No specialization info for this function
    }

    const auto& paramMap = it->second;

    // Every parameter must be specialized (constant)
    for (size_t paramIdx = 0; paramIdx < fn.paramTypes.size(); paramIdx++) {
        if (paramMap.find(paramIdx) == paramMap.end()) {
            return false;  // Parameter not in specialization map
        }
    }

    return true;  // All parameters are constant
}

// Phase 47: IR Metadata Collection
// Initialize IR tracking for a function
void IRCodeGen::initIRForFunction(const std::string& funcName, const ir::Function& fn) {
    if (!emitIRMetadata_) return;

    currentFunctionForIR_ = funcName;
    currentFunctionCallCount_ = 0;

    O45IRFunction irFunc;
    irFunc.functionName = funcName;

    // Compute signature hash (simple: hash of parameter count + types)
    uint32_t hash = 0x5381;  // DJB2 initial value
    for (const auto& paramType : fn.paramTypes) {
        hash = ((hash << 5) + hash) ^ (uint32_t)paramType;
    }
    irFunc.signatureHash = hash;

    // Collect parameter information
    for (size_t i = 0; i < fn.paramTypes.size(); i++) {
        O45IRParam param;
        if (i < fn.paramNames.size()) {
            param.name = fn.paramNames[i];
        }

        // Map IR type to O45IRType
        switch (fn.paramTypes[i]) {
            case ir::Type::I8:    param.type = IR_TYPE_I8; break;
            case ir::Type::I16:   param.type = IR_TYPE_I16; break;
            case ir::Type::I32:   param.type = IR_TYPE_I32; break;
            case ir::Type::I_N:   param.type = IR_TYPE_I32; break;  // Treat as I32
            case ir::Type::F32:   param.type = IR_TYPE_FLOAT; break;
            case ir::Type::PTR:   param.type = IR_TYPE_PTR; break;
            default:              param.type = IR_TYPE_UNKNOWN; break;
        }

        // Check if this parameter is marked as constant
        if (sacConstParams_.count(funcName) && sacConstParams_[funcName].count(i)) {
            const auto& constInfo = sacConstParams_[funcName][i];
            if (constInfo.isConstant) {
                param.flags |= O45_IR_PARAM_IS_CONST;
                param.constValue = constInfo.value;
            }
        }

        param.flags |= O45_IR_PARAM_IS_USED;  // Mark as used (verified by compiler)
        irFunc.parameters.push_back(param);
    }

    irFunctionMap_[funcName] = irFunc;
}

// Record a call site with actual parameter values
void IRCodeGen::recordCallSite(const std::string& calleeFunc, uint32_t instructionOffset,
                               const std::vector<ir::Operand>& args) {
    if (!emitIRMetadata_ || currentFunctionForIR_.empty()) return;

    auto it = irFunctionMap_.find(currentFunctionForIR_);
    if (it == irFunctionMap_.end()) return;

    O45IRCallSite callSite;
    callSite.instructionOffset = instructionOffset;
    callSite.calleeName = calleeFunc;

    // Analyze parameter values
    for (const auto& arg : args) {
        callSite.paramIsConst.push_back(0);  // Optimistically assume not constant
        callSite.paramValues.push_back(0);

        // Try to extract constant value if this is an immediate
        if (arg.isImm()) {
            callSite.paramIsConst.back() = 1;
            callSite.paramValues.back() = arg.immVal;
        }
    }

    it->second.callSites.push_back(callSite);

    // Update call graph
    bool foundEntry = false;
    for (auto& entry : it->second.callGraph) {
        if (entry.calleeName == calleeFunc) {
            entry.callCount++;
            foundEntry = true;
            break;
        }
    }

    if (!foundEntry) {
        O45IRCallGraphEntry entry;
        entry.calleeName = calleeFunc;
        entry.callCount = 1;
        it->second.callGraph.push_back(entry);
    }

    currentFunctionCallCount_++;
}

// Finalize IR for current function
void IRCodeGen::finalizeIRForFunction() {
    if (!emitIRMetadata_ || currentFunctionForIR_.empty()) return;

    auto it = irFunctionMap_.find(currentFunctionForIR_);
    if (it != irFunctionMap_.end()) {
        // Analyze call patterns for constant parameters
        for (auto& entry : it->second.callGraph) {
            // Check if all calls pass same constants for each parameter
            bool allCallsConstant = true;
            int64_t firstConstValue = 0;

            for (const auto& site : it->second.callSites) {
                if (site.calleeName == entry.calleeName) {
                    // Check if first parameter is constant in all calls
                    if (site.paramIsConst.empty() || !site.paramIsConst[0]) {
                        allCallsConstant = false;
                        break;
                    }
                }
            }

            entry.allCallsConstant = allCallsConstant;
        }
    }

    currentFunctionForIR_.clear();
    currentFunctionCallCount_ = 0;
}

// Get collected IR metadata for .o45 output
O45IRMetadata IRCodeGen::getIRMetadata() const {
    O45IRMetadata metadata;

    if (!emitIRMetadata_) {
        metadata.majorVersion = 0;  // No IR
        return metadata;
    }

    metadata.majorVersion = O45_IR_VERSION_MAJOR;
    metadata.minorVersion = O45_IR_VERSION_MINOR;

    for (const auto& [funcName, irFunc] : irFunctionMap_) {
        metadata.functions.push_back(irFunc);
    }

    return metadata;
}

