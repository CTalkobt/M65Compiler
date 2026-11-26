// PcodeEmit.cpp — research/pcode: a function's IR as p-code
//
// The p-code is the interpreter's (research/pcode/pcode_rt.s45, which has
// the encoding): an opcode byte (the operation in bits 5-0; bit 7 the word
// form, bit 6 the long one, neither the byte), then its operands: a
// variable is a byte, its offset in the function's frame; an immediate is
// as wide as the operation; an address or a branch target 2 bytes.
//
// The frame is static (<f>__pf in BSS): the function must not be recursive.
// A vreg used in one block only (a temporary) shares the frame's bytes with
// the temporaries of other blocks (first fit over its instructions); the
// rest have their own: the parameters, the locals, anything whose address is
// taken. The native entry <f> (the ZP convention) copies the parameters
// from the ZP parameter block into the frame and enters the interpreter
// (A/X the p-code, Y/Z the frame; jmp __pc_enter: RET returns to the caller).
//
// Anything it does not translate (a stack-convention call, a switch, floats,
// inline asm, far pointers, ...) leaves the function native: why says what.
#include "PcodeEmit.hpp"
#include <algorithm>
#include <cstdio>
#include <map>
#include <set>
#include <sstream>
#include <vector>
#include <cstdlib>

namespace {

enum Pc : int {
    P_CONST = 1, P_MOV, P_LDG, P_STG, P_LD, P_ST, P_LEAL, P_ELEM,
    P_ADD, P_ADDI, P_SUB, P_SUBI, P_AND, P_ANDI, P_OR, P_ORI, P_XOR, P_XORI,
    P_SHL, P_SHLI, P_SHR, P_SHRI, P_ASR, P_ASRI, P_MUL, P_MULI,
    P_DIVU, P_DIVUI, P_MODU, P_MODUI, P_NEG, P_NOT, P_CMP, P_CMPI,
    P_BR, P_BNZ, P_BZ, P_CALL, P_RET, P_RETV, P_ZEXT, P_SEXT, P_ADV,
    P_DIV, P_DIVI, P_MOD, P_MODI, P_CBR, P_CBRI, P_CALLC, P_LDO, P_STO, P_ELEMG, P_CALLC4
};

// the condition codes of CMP / CMPI
enum Cc : int { CC_EQ, CC_NE, CC_LTU, CC_LEU, CC_GTU, CC_GEU, CC_LT, CC_LE, CC_GT, CC_GE };

int widthOf(const ir::Operand& o) {
    if (o.type == ir::Type::I_N) return o.byteWidth;
    return ir::typeSize(o.type);
}
int widthOfType(ir::Type t, int bw = 0) { return t == ir::Type::I_N ? bw : ir::typeSize(t); }

struct Emitter {
    const ir::Function& fn;
    std::string why;
    std::map<uint32_t, int> width;      // a vreg's bytes in the frame
    std::map<uint32_t, int> off;        // its offset
    std::map<std::string, int> blockIx;
    int frame = 0;
    int k0 = 0, k1 = 0, k2 = 0, ka = 0; // scratch: an operand, an address, another operand, call arguments
    std::ostringstream pc;              // the p-code's lines
    int bytes = 0, run = 0;
    std::vector<std::string> line;      // the current instruction's items

    explicit Emitter(const ir::Function& f) : fn(f) {}

    bool fail(const std::string& w) { if (why.empty()) why = w; return false; }

    std::string label(const std::string& l) {
        auto it = blockIx.find(l);
        return fn.name + "__pcB" + std::to_string(it == blockIx.end() ? 0 : it->second);
    }

    // --- the instruction being written: items, then flush
    void b(int v) { line.push_back(std::to_string(v & 0xFF)); }
    void imm(int64_t v, int w) { for (int i = 0; i < w; i++) b((int)((uint64_t)v >> (8 * i))); }
    // (an address: ".word sym", 2 bytes; the bytes around it in .byte lines)
    void word(const std::string& sym) { line.push_back("W" + sym); }
    // the block's instructions are kept until its end (endBlock): the
    // peepholes, then written
    struct PI { std::vector<std::string> items; bool resets = false; };   // resets: a branch or return
    std::vector<PI> blk;
    void flush() { blk.push_back({line, false}); line.clear(); }
    void resetRun() { if (!blk.empty()) blk.back().resets = true; }

    static int size(const PI& x) { int n = 0; for (const auto& it : x.items) n += it[0] == 'W' ? 2 : 1; return n; }
    static int code(const PI& x) { return std::stoi(x.items[0]) & 0x3F; }
    static int wOf(const PI& x) { int c = std::stoi(x.items[0]); return c & 0x80 ? 2 : c & 0x40 ? 4 : 1; }
    static int num(const PI& x, size_t k) { return std::stoi(x.items[k]); }
    // the frame slot an instruction writes (the first operand), -1 for none
    static int dstOf(const PI& x) {
        switch (code(x)) {
            case P_STG: case P_ST: case P_BR: case P_BNZ: case P_BZ: case P_RET: case P_RETV: case P_ADV:
            case P_CBR: case P_CBRI: case P_STO:
                return -1;
            case P_CALL: {   // op, callee, n, n x (slot, width), result slot, result width
                size_t k = 3 + 2 * (size_t)num(x, 2);
                return num(x, k + 1) ? num(x, k) : -1;
            }
            default: return num(x, 1);
        }
    }
    // the bytes it writes there
    // (CMP: a flag byte; LEAL, ELEM: an address, whatever the index's width; CALL: the result's)
    static int dstWidth(const PI& x) {
        switch (code(x)) {
            case P_CMP: case P_CMPI: return 1;
            case P_LEAL: case P_ELEM: case P_ELEMG: return 2;
            case P_CALL: return num(x, 4 + 2 * (size_t)num(x, 2));
            default: return wOf(x);
        }
    }
    // the frame slots it reads
    static std::vector<int> srcOf(const PI& x) {
        switch (code(x)) {
            case P_CONST: case P_LDG: case P_LEAL: case P_BR: case P_RETV: case P_ADV: return {};
            case P_STG: return {num(x, 2)};
            case P_ST: return {num(x, 1), num(x, 2)};
            case P_BNZ: case P_BZ: case P_RET: return {num(x, 1)};
            case P_ELEM: return {num(x, 2), num(x, 3)};
            case P_CALL: {
                int k = num(x, 2);
                std::vector<int> r;
                for (int j = 0; j < k; j++)
                    if (num(x, 4 + 2 * j) != 3) r.push_back(num(x, 3 + 2 * j));   // (3: an immediate)
                return r;
            }
            case P_MOV: case P_LD: case P_NEG: case P_NOT: case P_ZEXT: case P_SEXT: return {num(x, 2)};
            case P_CMP: return {num(x, 2), num(x, 3)};
            case P_CMPI: return {num(x, 2)};
            case P_CBR: return {num(x, 1), num(x, 2)};
            case P_CBRI: return {num(x, 1)};
            case P_LDO: return {num(x, 2)};
            case P_ELEMG: return {num(x, 3)};
            case P_STO: return {num(x, 1), num(x, 3)};
            default:   // the ALU operations: dst, a, b or dst, a, imm
                if (code(x) % 2 == 0 && code(x) >= P_ADDI && code(x) <= P_MODUI) return {num(x, 2)};
                if (code(x) == P_DIVI || code(x) == P_MODI) return {num(x, 2)};
                return {num(x, 2), num(x, 3)};
        }
    }
    // slot t (a block-local scratch or temporary) not read after instruction j before it is written again
    bool deadAfter(size_t j, int t) const {
        if (t < k0) return false;
        for (size_t k = j + 1; k < blk.size(); k++) {
            auto s = srcOf(blk[k]);
            if (std::find(s.begin(), s.end(), t) != s.end()) return false;
            if (dstOf(blk[k]) == t) return true;
        }
        return true;
    }

    // a CALL's argument (it has at most 4) set by a CONST of a byte, or of a
    // word under 256, that nothing else reads: the CALLC immediate (width 3)
    bool constArgs() {
        for (size_t c = 0; c < blk.size(); c++) {
            PI& y = blk[c];
            if (code(y) != P_CALL || num(y, 2) > 4) continue;
            int n = num(y, 2);
            for (int j = 0; j < n; j++) {
                size_t ks = 3 + 2 * (size_t)j;
                int slot = num(y, ks), aw = num(y, ks + 1);
                if (aw != 1 && aw != 2) continue;
                int uses = 0;
                for (int q = 0; q < n; q++) if (num(y, 4 + 2 * q) != 3 && num(y, 3 + 2 * q) == slot) uses++;
                if (uses != 1 || !deadAfter(c, slot)) continue;
                // the slot's last writer before the call, nothing reading it between
                size_t i = c;
                bool ok = false;
                while (i-- > 0) {
                    auto sr = srcOf(blk[i]);
                    if (dstOf(blk[i]) == slot) { ok = true; break; }
                    if (std::find(sr.begin(), sr.end(), slot) != sr.end()) break;
                }
                if (!ok || code(blk[i]) != P_CONST || wOf(blk[i]) != aw || blk[i].items.size() != 2 + (size_t)aw) continue;   // (not an address)
                int v = num(blk[i], 2) | (aw == 2 ? num(blk[i], 3) << 8 : 0);
                if (v > 255) continue;
                y.items[ks] = std::to_string(v);
                y.items[ks + 1] = "3";
                blk.erase(blk.begin() + i);
                return true;
            }
        }
        return false;
    }

    // a pair of instructions as one: the first's result used by the second only
    void peephole() {
        for (bool changed = true; changed;) {
            changed = constArgs();
            for (size_t j = 0; j + 1 < blk.size(); j++) {
                PI& x = blk[j];
                PI& y = blk[j + 1];
                int t = dstOf(x);
                if (t < 0 || !deadAfter(j + 1, t)) continue;
                // 1. X t, ...; MOV v, t -> X v, ...
                if (code(y) == P_MOV && num(y, 2) == t && wOf(y) == dstWidth(x)) {
                    x.items[code(x) == P_CALL ? 3 + 2 * (size_t)num(x, 2) : 1] = y.items[1];
                    blk.erase(blk.begin() + j + 1);
                    changed = true;
                    break;
                }
                // 2. CONST.2 t, addr; LD d, t / ST t, v -> LDG d, addr / STG addr, v
                if (code(x) == P_CONST && wOf(x) == 2) {
                    std::string addr = x.items.size() == 3 ? x.items[2]
                                     : "W" + std::to_string(num(x, 2) | (num(x, 3) << 8));
                    if (code(y) == P_LD && num(y, 2) == t) {
                        y.items = {std::to_string(P_LDG | (std::stoi(y.items[0]) & 0xC0)), y.items[1], addr};
                        blk.erase(blk.begin() + j);
                        changed = true;
                        break;
                    }
                    if (code(y) == P_ST && num(y, 1) == t && num(y, 2) != t) {
                        y.items = {std::to_string(P_STG | (std::stoi(y.items[0]) & 0xC0)), addr, y.items[2]};
                        blk.erase(blk.begin() + j);
                        changed = true;
                        break;
                    }
                }
                // 4. CMP(I) t, a, b, cc; BNZ / BZ t, @l -> CBR(I) a, b, cc', @l
                //    (BZ: the condition inverted; the flag a byte)
                if ((code(x) == P_CMP || code(x) == P_CMPI) && (code(y) == P_BNZ || code(y) == P_BZ) &&
                    num(y, 1) == t && wOf(y) == 1) {
                    int cc = num(x, x.items.size() - 1);
                    if (code(y) == P_BZ) cc = invert(cc);
                    std::vector<std::string> it{std::to_string((code(x) == P_CMP ? P_CBR : P_CBRI) |
                                                               (std::stoi(x.items[0]) & 0xC0))};
                    it.insert(it.end(), x.items.begin() + 2, x.items.end() - 1);
                    it.push_back(std::to_string(cc));
                    it.push_back(y.items[2]);
                    x.items = it;
                    x.resets = y.resets;
                    blk.erase(blk.begin() + j + 1);
                    changed = true;
                    break;
                }
                // 5. ADDI.2 t, p, #off; LD d, t / ST t, v -> LDO d, p, off / STO p, off, v
                //    (off a byte; off + the width within it: (PTR),y)
                if (code(x) == P_ADDI && wOf(x) == 2 && (code(y) == P_LD || code(y) == P_ST)) {
                    int o = num(x, 3) | (num(x, 4) << 8);
                    int yw = std::stoi(y.items[0]) & 0xC0;
                    if (o + wOf(y) <= 256) {
                        if (code(y) == P_LD && num(y, 2) == t) {
                            y.items = {std::to_string(P_LDO | yw), y.items[1], x.items[2], std::to_string(o)};
                            blk.erase(blk.begin() + j);
                            changed = true;
                            break;
                        }
                        if (code(y) == P_ST && num(y, 1) == t && num(y, 2) != t) {
                            y.items = {std::to_string(P_STO | yw), x.items[2], std::to_string(o), y.items[2]};
                            blk.erase(blk.begin() + j);
                            changed = true;
                            break;
                        }
                    }
                }
                // 6. CONST.2 t, addr; ELEM d, t, i, size, off -> ELEMG d, addr, i, size, off
                if (code(x) == P_CONST && wOf(x) == 2 && code(y) == P_ELEM && num(y, 2) == t && num(y, 3) != t) {
                    std::string addr = x.items.size() == 3 ? x.items[2]
                                     : "W" + std::to_string(num(x, 2) | (num(x, 3) << 8));
                    y.items = {std::to_string(P_ELEMG | (std::stoi(y.items[0]) & 0xC0)), y.items[1], addr,
                               y.items[3], y.items[4], y.items[5], y.items[6]};
                    blk.erase(blk.begin() + j);
                    changed = true;
                    break;
                }
                // 7. CONST t, #v; OP d, a, t -> OPI d, a, #v (the same width; the IR gives a
                //    divisor as a variable)
                if (code(x) == P_CONST && x.items.size() == 2 + (size_t)wOf(x) && wOf(y) == wOf(x) &&
                    y.items.size() == 4 && num(y, 3) == t && num(y, 2) != t) {
                    int c = code(y);
                    if (c == P_ADD || c == P_SUB || c == P_AND || c == P_OR || c == P_XOR || c == P_MUL ||
                        c == P_DIVU || c == P_MODU || c == P_DIV || c == P_MOD) {
                        std::vector<std::string> it{std::to_string((c + 1) | (std::stoi(y.items[0]) & 0xC0)), y.items[1], y.items[2]};
                        it.insert(it.end(), x.items.begin() + 2, x.items.end());
                        y.items = it;
                        blk.erase(blk.begin() + j);
                        changed = true;
                        break;
                    }
                }
                // 3. LEAL t, s; ADDI.2 d, t, off -> LEAL d, s + off (the frame is static)
                if (code(x) == P_LEAL && code(y) == P_ADDI && wOf(y) == 2 && num(y, 2) == t) {
                    int o = num(y, 3) | (num(y, 4) << 8);
                    if (num(x, 2) + o <= 255) {
                        x.items = {x.items[0], y.items[1], std::to_string(num(x, 2) + o)};
                        blk.erase(blk.begin() + j + 1);
                        changed = true;
                        break;
                    }
                }
            }
        }
    }

    // the block written: the peepholes, then each instruction (ADV before
    // one that would take the run past 240 bytes)
    // CALL with at most 4 arguments as CALLC @f, d, v x n, [r] (4: CALLC4, d the widths only): d n and the
    // arguments' widths (2 bits each; 3 an immediate byte, zero-extended into
    // its 2-byte slot), the result's width in the opcode
    static void compactCall(PI& x) {
        if (code(x) != P_CALL) return;
        int n = num(x, 2);
        if (n > 4) return;   // (4: CALLC4, d the widths only)
        auto wc = [](int w) { return w == 1 ? 0 : w == 2 ? 1 : w == 4 ? 2 : w == 3 ? 3 : -1; };
        const int sh = n == 4 ? 0 : 2;
        int d = n == 4 ? 0 : n;
        std::vector<std::string> slots;
        for (int j = 0; j < n; j++) {
            int c = wc(num(x, 4 + 2 * (size_t)j));
            if (c < 0) return;
            d |= c << (sh + 2 * j);
            slots.push_back(x.items[3 + 2 * (size_t)j]);
        }
        int rw = num(x, 4 + 2 * (size_t)n);
        int opc = rw == 0 ? 0xC0 : rw == 1 ? 0 : rw == 2 ? 0x80 : rw == 4 ? 0x40 : -1;
        if (opc < 0) return;
        std::vector<std::string> it{std::to_string((n == 4 ? P_CALLC4 : P_CALLC) | opc), x.items[1], std::to_string(d)};
        it.insert(it.end(), slots.begin(), slots.end());
        if (rw) it.push_back(x.items[3 + 2 * (size_t)n]);
        x.items = it;
    }

    // a word operation's immediate under 256 as a byte: width bits 11 (the
    // interpreter zero-extends it)
    static void byteImm(PI& x) {
        int c = std::stoi(x.items[0]);
        if ((c & 0xC0) != 0x80) return;
        size_t k;
        switch (c & 0x3F) {
            case P_CONST: case P_CBRI: k = 2; break;
            case P_ADDI: case P_SUBI: case P_ANDI: case P_CMPI: k = 3; break;   // (the others rarer than the handler's bytes)
            default: return;
        }
        if (x.items.size() < k + 2 || x.items[k][0] == 'W' || x.items[k + 1] != "0") return;
        x.items[0] = std::to_string(c | 0xC0);
        x.items.erase(x.items.begin() + (long)k + 1);
    }

    void endBlock() {
        peephole();
        for (auto& x : blk) { compactCall(x); byteImm(x); }
        for (const auto& x : blk) {
            int n = size(x);
            if (run + n > 240) { pc << "    .byte " << P_ADV << "            ; ADV\n"; bytes++; run = 0; }
            std::string bytesLine;
            auto out = [&]() { if (!bytesLine.empty()) { pc << "    .byte " << bytesLine << "\n"; bytesLine.clear(); } };
            for (const auto& it : x.items) {
                if (it[0] == 'W') { out(); pc << "    .word " << it.substr(1) << "\n"; }
                else bytesLine += (bytesLine.empty() ? "" : ", ") + it;
            }
            out();
            if (list) {                     // CC45_PCODE_LIST: the instruction as a comment (its items as written)
                pc << "    ; PC";
                for (const auto& it : x.items) pc << " " << it;
                pc << "\n";
            }
            bytes += n;
            run += n;
            if (x.resets) run = 0;
        }
        blk.clear();
    }
    bool list = std::getenv("CC45_PCODE_LIST") != nullptr;
    void op(int code, int w) {
        int c = code;
        if (w == 2) c |= 0x80;
        else if (w == 4) c |= 0x40;
        b(c);
    }
    bool okWidth(int w) { return w == 1 || w == 2 || w == 4 ? true : fail("a " + std::to_string(w) + "-byte operation"); }

    int var(const ir::Operand& o) { return off.at(o.vregId); }

    // an operand that is not a variable, in a scratch slot (CONST into it)
    int materialize(const ir::Operand& o, int w, int slot) {
        if (o.isVreg()) return var(o);
        if (o.kind == ir::OperandKind::GLOBAL) {
            op(P_CONST, 2); b(slot); word(o.name); flush();
            return slot;
        }
        op(P_CONST, w); b(slot); imm(o.immVal, w); flush();
        return slot;
    }

    // --- the frame -----------------------------------------------------------
    bool layout() {
        const uint32_t np = (uint32_t)fn.paramTypes.size();
        std::map<uint32_t, std::set<size_t>> blocksOf;
        std::map<uint32_t, std::pair<int, int>> span;   // first / last instruction index
        std::set<uint32_t> addressed;
        int ix = 0;
        auto note = [&](const ir::Operand& o, size_t blk, int w) {
            if (!o.isVreg()) return;
            uint32_t v = o.vregId;
            width[v] = std::max(width[v], std::max(w, 1));
            blocksOf[v].insert(blk);
            auto it = span.find(v);
            if (it == span.end()) span[v] = {ix, ix};
            else it->second.second = ix;
        };
        for (uint32_t v = 0; v < np; v++) {
            width[v] = std::max(1, widthOfType(fn.paramTypes[v]));
            blocksOf[v].insert(SIZE_MAX);
        }
        for (size_t bi = 0; bi < fn.blocks.size(); bi++) {
            for (const auto& in : fn.blocks[bi].insts) {
                ix++;
                int rw = in.op >= ir::Op::CMP_EQ && in.op <= ir::Op::CMP_GEU ? 1
                       : (in.op == ir::Op::ADDR_LOCAL || in.op == ir::Op::ADDR_GLOBAL || in.op == ir::Op::ADDR_ELEM) ? 2
                       : widthOfType(in.resultType, in.resultByteWidth);
                note(in.dest, bi, rw);
                note(in.src1, bi, widthOf(in.src1));
                note(in.src2, bi, widthOf(in.src2));
                for (const auto& a : in.args) note(a, bi, widthOf(a));
                for (const auto& ph : in.phiIncoming) note(ph.first, bi, widthOf(ph.first));
                if (in.op == ir::Op::ADDR_LOCAL && in.src1.isVreg()) addressed.insert(in.src1.vregId);
            }
        }
        for (const auto& [v, n] : fn.vregSizes) { width[v] = std::max(width[v], n); blocksOf[v].insert(SIZE_MAX); }
        // the frame: parameters, then the others with bytes of their own, the scratch, the temporaries
        auto temp = [&](uint32_t v) {
            return v >= np && blocksOf[v].size() == 1 && !fn.localSlotVregs.count(v) && !fn.memoryVregs.count(v) &&
                   !fn.vregSizes.count(v) && !addressed.count(v) && (int)v != fn.staticLinkVreg;
        };
        for (uint32_t v = 0; v < np; v++) { off[v] = frame; frame += width[v]; }
        for (const auto& [v, w] : width)
            if (v >= np && !temp(v)) { off[v] = frame; frame += w; }
        k0 = frame; frame += 4;
        k1 = frame; frame += 2;
        k2 = frame; frame += 4;
        ka = frame;
        int callScratch = 0;
        for (const auto& blk : fn.blocks)
            for (const auto& in : blk.insts)
                if (in.op == ir::Op::CALL || in.op == ir::Op::CALL_VOID) {
                    int n = 0;
                    for (const auto& a : in.args) if (!a.isVreg()) n += std::max(2, widthOf(a));
                    callScratch = std::max(callScratch, n);
                }
        frame += callScratch;
        // the temporaries of each block: first fit over their instructions
        const int base = frame;
        int top = base;
        std::map<size_t, std::vector<uint32_t>> byBlock;
        for (const auto& [v, w] : width) if (temp(v)) byBlock[*blocksOf[v].begin()].push_back(v);
        for (auto& [blk, vs] : byBlock) {
            std::sort(vs.begin(), vs.end(), [&](uint32_t a, uint32_t c) { return span[a].first < span[c].first; });
            std::vector<std::pair<std::pair<int, int>, std::pair<int, int>>> placed;   // (span, [lo, hi))
            for (uint32_t v : vs) {
                int at = base;
                for (bool moved = true; moved;) {
                    moved = false;
                    for (const auto& [sp, r] : placed)
                        if (sp.second >= span[v].first && span[v].second >= sp.first &&   // live together
                            r.first < at + width[v] && at < r.second) { at = r.second; moved = true; }
                }
                off[v] = at;
                placed.push_back({span[v], {at, at + width[v]}});
                top = std::max(top, at + width[v]);
            }
        }
        frame = top;
        if (frame > 250) return fail("a frame of " + std::to_string(frame) + " bytes (more than 250)");
        return true;
    }

    // --- one instruction -------------------------------------------------------
    int ccOf(ir::Op o) {
        switch (o) {
            case ir::Op::CMP_EQ: return CC_EQ;   case ir::Op::CMP_NE: return CC_NE;
            case ir::Op::CMP_LTU: return CC_LTU; case ir::Op::CMP_LEU: return CC_LEU;
            case ir::Op::CMP_GTU: return CC_GTU; case ir::Op::CMP_GEU: return CC_GEU;
            case ir::Op::CMP_LT: return CC_LT;   case ir::Op::CMP_LE: return CC_LE;
            case ir::Op::CMP_GT: return CC_GT;   default: return CC_GE;
        }
    }
    static int invert(int cc) {   // !(a cc b)
        switch (cc) {
            case CC_EQ: return CC_NE;   case CC_NE: return CC_EQ;
            case CC_LTU: return CC_GEU; case CC_GEU: return CC_LTU; case CC_LEU: return CC_GTU; case CC_GTU: return CC_LEU;
            case CC_LT: return CC_GE;   case CC_GE: return CC_LT;   case CC_LE: return CC_GT;   default: return CC_LE;
        }
    }
    int mirror(int cc) {   // a < b == b > a
        switch (cc) {
            case CC_LTU: return CC_GTU; case CC_GTU: return CC_LTU; case CC_LEU: return CC_GEU; case CC_GEU: return CC_LEU;
            case CC_LT: return CC_GT;   case CC_GT: return CC_LT;   case CC_LE: return CC_GE;   case CC_GE: return CC_LE;
            default: return cc;
        }
    }

    // an element's address into the scratch k1: base + idx * size + offset
    bool elem(const ir::Operand& base, const ir::Operand& idx, int64_t size, int64_t offset, int dst) {
        if (base.kind == ir::OperandKind::GLOBAL && idx.isVreg() && size >= 1 && size <= 255) {   // ELEMG: the address in it
            op(P_ELEMG, std::min(widthOf(idx), 2)); b(dst); word(base.name); b(var(idx)); b((int)size); imm(offset, 2); flush();
            return true;
        }
        int bv;
        if (base.isVreg()) bv = var(base);
        else if (base.kind == ir::OperandKind::GLOBAL) bv = materialize(base, 2, k0);
        else return fail("an element of a constant address");
        if (idx.isImm()) {
            int64_t o = offset + idx.immVal * size;
            if (o == 0 && bv == dst) return true;
            if (o == 0) { op(P_MOV, 2); b(dst); b(bv); flush(); return true; }
            op(P_ADDI, 2); b(dst); b(bv); imm(o, 2); flush();
            return true;
        }
        if (!idx.isVreg()) return fail("an element index");
        if (size < 1 || size > 255) return fail("an element of " + std::to_string(size) + " bytes");
        int iw = std::min(widthOf(idx), 2);
        op(P_ELEM, iw); b(dst); b(bv); b(var(idx)); b((int)size); imm(offset, 2); flush();
        return true;
    }

    bool binary(int code, int codeI, const ir::Inst& in, int w, bool commutes, bool shift) {
        ir::Operand a = in.src1, c = in.src2;
        if (!a.isVreg() && c.isVreg() && commutes) std::swap(a, c);
        int av = materialize(a, w, k0);
        if (c.isImm()) {
            op(codeI, w); b(var(in.dest)); b(av); imm(c.immVal, shift ? 1 : w); flush();
            return true;
        }
        int cv = materialize(c, w, k2);
        op(code, w); b(var(in.dest)); b(av); b(cv); flush();
        return true;
    }

    bool inst(const ir::Inst& in, const std::string& next) {
        using ir::Op;
        const int w = widthOfType(in.resultType, in.resultByteWidth);
        switch (in.op) {
            case Op::NOP: return true;
            case Op::CONST:
                if (!okWidth(w)) return false;
                op(P_CONST, w); b(var(in.dest)); imm(in.src1.immVal, w); flush();
                return true;
            case Op::COPY: case Op::TRUNC: case Op::DEREF: {   // (deref: a pointer local's value, as a copy)
                if (!okWidth(w)) return false;
                if (in.src1.isVreg()) { if (var(in.src1) != var(in.dest)) { op(P_MOV, w); b(var(in.dest)); b(var(in.src1)); flush(); } }
                else materialize(in.src1, w, var(in.dest));
                return true;
            }
            case Op::ADDR_GLOBAL:
                op(P_CONST, 2); b(var(in.dest)); word(in.src1.name + (in.elemOffset ? "+" + std::to_string(in.elemOffset) : "")); flush();
                return true;
            case Op::ADDR_LOCAL:
                if (!in.src1.isVreg() || !in.args.empty()) return fail("an addr_local form");
                op(P_LEAL, 2); b(var(in.dest)); b(var(in.src1)); flush();
                return true;
            case Op::ADDR_ELEM:
                return elem(in.src1, in.src2, in.args.empty() ? 2 : in.args[0].immVal, in.elemOffset, var(in.dest));
            case Op::LOAD: {
                if (!okWidth(w)) return false;
                if (in.args.size() >= 3) {
                    if (!elem(in.args[0], in.args[1], in.args[2].immVal, in.elemOffset, k1)) return false;
                    op(P_LD, w); b(var(in.dest)); b(k1); flush();
                } else if (in.src1.kind == ir::OperandKind::GLOBAL) {
                    op(P_LDG, w); b(var(in.dest)); word(in.src1.name + (in.elemOffset ? "+" + std::to_string(in.elemOffset) : "")); flush();
                } else if (in.src1.isVreg()) {
                    int p = var(in.src1);
                    if (in.elemOffset) { op(P_ADDI, 2); b(k1); b(p); imm(in.elemOffset, 2); flush(); p = k1; }
                    op(P_LD, w); b(var(in.dest)); b(p); flush();
                } else return fail("a load from a constant address");
                return true;
            }
            case Op::STORE: {
                if (!okWidth(w)) return false;
                const ir::Operand& t = in.src2;
                bool slot = t.isVreg() && in.args.empty() && (fn.localSlotVregs.count(t.vregId) || fn.memoryVregs.count(t.vregId) ||
                                                            t.vregId < fn.paramTypes.size());
                if (slot) {
                    if (in.src1.isVreg()) { if (var(in.src1) != var(t)) { op(P_MOV, w); b(var(t)); b(var(in.src1)); flush(); } }
                    else materialize(in.src1, w, var(t));
                    return true;
                }
                int v = materialize(in.src1, w, k0);
                if (in.args.size() >= 3) {
                    if (!elem(in.args[0], in.args[1], in.args[2].immVal, in.elemOffset, k1)) return false;
                    op(P_ST, w); b(k1); b(v); flush();
                } else if (t.kind == ir::OperandKind::GLOBAL) {
                    op(P_STG, w); word(t.name + (in.elemOffset ? "+" + std::to_string(in.elemOffset) : "")); b(v); flush();
                } else if (t.isVreg()) {
                    int p = var(t);
                    if (in.elemOffset) { op(P_ADDI, 2); b(k1); b(p); imm(in.elemOffset, 2); flush(); p = k1; }
                    op(P_ST, w); b(p); b(v); flush();
                } else return fail("a store to a constant address");
                return true;
            }
            case Op::ADD: return okWidth(w) && binary(P_ADD, P_ADDI, in, w, true, false);
            case Op::SUB: return okWidth(w) && binary(P_SUB, P_SUBI, in, w, false, false);
            case Op::AND: return okWidth(w) && binary(P_AND, P_ANDI, in, w, true, false);
            case Op::OR:  return okWidth(w) && binary(P_OR, P_ORI, in, w, true, false);
            case Op::XOR: return okWidth(w) && binary(P_XOR, P_XORI, in, w, true, false);
            case Op::MUL: case Op::MUL_U: return okWidth(w) && binary(P_MUL, P_MULI, in, w, true, false);
            case Op::DIV_U: return okWidth(w) && binary(P_DIVU, P_DIVUI, in, w, false, false);
            case Op::MOD_U: return okWidth(w) && binary(P_MODU, P_MODUI, in, w, false, false);
            case Op::DIV: return okWidth(w) && binary(P_DIV, P_DIVI, in, w, false, false);
            case Op::MOD: return okWidth(w) && binary(P_MOD, P_MODI, in, w, false, false);
            case Op::SHL: case Op::LSL: return okWidth(w) && binary(P_SHL, P_SHLI, in, w, false, true);
            case Op::SHR: case Op::LSR: return okWidth(w) && binary(P_SHR, P_SHRI, in, w, false, true);
            case Op::ASR: return okWidth(w) && binary(P_ASR, P_ASRI, in, w, false, true);
            case Op::NEG: case Op::NOT: {
                if (!okWidth(w)) return false;
                int a = materialize(in.src1, w, k0);
                op(in.op == Op::NEG ? P_NEG : P_NOT, w); b(var(in.dest)); b(a); flush();
                return true;
            }
            case Op::ZEXT: case Op::SEXT: {
                int ws = widthOf(in.src1);
                if (!okWidth(w) || (ws != 1 && ws != 2 && ws != 4)) return fail("an extension");
                if (!in.src1.isVreg()) { materialize(in.src1, w, var(in.dest)); return true; }   // (a constant, already extended)
                if (ws >= w) { op(P_MOV, w); b(var(in.dest)); b(var(in.src1)); flush(); return true; }
                op(in.op == Op::ZEXT ? P_ZEXT : P_SEXT, w); b(var(in.dest)); b(var(in.src1)); b(ws); flush();
                return true;
            }
            case Op::CMP_EQ: case Op::CMP_NE: case Op::CMP_LT: case Op::CMP_LE: case Op::CMP_GT: case Op::CMP_GE:
            case Op::CMP_LTU: case Op::CMP_LEU: case Op::CMP_GTU: case Op::CMP_GEU: {
                ir::Operand a = in.src1, c = in.src2;
                int cc = ccOf(in.op);
                int cw = std::max(a.isVreg() ? widthOf(a) : 0, c.isVreg() ? widthOf(c) : 0);
                if (cw == 0) cw = std::max(widthOf(a), widthOf(c));
                if (!okWidth(cw)) return false;
                if (!a.isVreg() && c.isVreg()) { std::swap(a, c); cc = mirror(cc); }
                int av = materialize(a, cw, k0);
                if (c.isImm()) { op(P_CMPI, cw); b(var(in.dest)); b(av); imm(c.immVal, cw); b(cc); flush(); return true; }
                int cv = materialize(c, cw, k2);
                op(P_CMP, cw); b(var(in.dest)); b(av); b(cv); b(cc); flush();
                return true;
            }
            case Op::BR:
                if (in.src1.kind == ir::OperandKind::LABEL && in.src1.name == next) return true;
                op(P_BR, 1); word(label(in.src1.name)); flush();
                resetRun();   // (what follows is reached by a branch only: Z = 0 there)
                return true;
            case Op::BR_COND: {
                // (src1 the condition, dest the true target, src2 the false one)
                const std::string& t = in.dest.name;
                const std::string& f = in.src2.name;
                if (in.src1.isImm()) {
                    const std::string& to = in.src1.immVal ? t : f;
                    if (to != next) { op(P_BR, 1); word(label(to)); flush(); resetRun(); }
                    return true;
                }
                if (!in.src1.isVreg()) return fail("a branch condition");
                int cw = std::min(widthOf(in.src1), 4);
                if (cw == 0) cw = 1;
                if (!okWidth(cw)) return false;
                if (t == next) { op(P_BZ, cw); b(var(in.src1)); word(label(f)); flush(); }
                else {
                    op(P_BNZ, cw); b(var(in.src1)); word(label(t)); flush();
                    if (f != next) { op(P_BR, 1); word(label(f)); flush(); resetRun(); }
                }
                return true;
            }
            case Op::CALL: case Op::CALL_VOID: {
                if (in.src1.kind != ir::OperandKind::GLOBAL) return fail("a call through a pointer");
                if (!in.args.empty() && in.callConv != ir::CallConv::ZP) return fail("a stack-convention call");
                if (in.isRegparm) return fail("a __regparm call");
                // (the arguments that are not variables: into the call scratch first)
                std::vector<std::pair<int, int>> args;   // offset, bytes
                int sc = ka, zp = 0;
                for (const auto& a : in.args) {
                    int aw = widthOf(a);
                    if (aw != 1 && aw != 2 && aw != 4) return fail("a call argument of " + std::to_string(aw) + " bytes");
                    // (a constant byte, or a word under 256, as CALLC's immediate: the width 3; the
                    // call then always CALLC / CALLC4: at most 4 arguments)
                    if (in.args.size() <= 4 && a.isImm() && aw <= 2 &&
                        ((uint64_t)a.immVal & (aw == 1 ? 0xFFu : 0xFFFFu)) < 256) {
                        args.push_back({(int)((uint64_t)a.immVal & 0xFF), 3});
                        zp += 2;
                        continue;
                    }
                    int o = a.isVreg() ? var(a) : materialize(a, aw, sc);
                    if (!a.isVreg()) sc += std::max(2, aw);
                    args.push_back({o, aw});
                    zp += std::max(2, aw);
                }
                if (zp > 32) return fail("more than 32 bytes of arguments");
                int rw = in.op == Op::CALL && in.dest.isVreg() ? widthOfType(in.resultType, in.resultByteWidth) : 0;
                if (rw != 0 && rw != 1 && rw != 2 && rw != 4) return fail("a call result of " + std::to_string(rw) + " bytes");
                op(P_CALL, 1); word(in.src1.name); b((int)args.size());
                for (const auto& [o, n] : args) { b(o); b(n); }
                b(rw ? var(in.dest) : 0); b(rw);
                flush();
                return true;
            }
            case Op::RET: {
                if (in.src1.isNone() || w == 0) { op(P_RETV, 1); flush(); resetRun(); return true; }
                if (!okWidth(w)) return false;
                int a = materialize(in.src1, w, k0);
                op(P_RET, w); b(a); flush();
                resetRun();
                return true;
            }
            case Op::RET_VOID: op(P_RETV, 1); flush(); resetRun(); return true;
            default:
                return fail(std::string("the operation ") + ir::opName(in.op));
        }
    }
};

}  // namespace

PcodeResult pcodeEmit(const ir::Function& fn) {
    PcodeResult r;
    if (fn.blocks.empty()) { r.why = "no code"; return r; }
    if (fn.isVariadic || fn.isInterrupt || fn.isNaked || fn.isNested || fn.staticLinkVreg != -1)
        { r.why = "variadic, __interrupt, __naked or nested"; return r; }
    if (fn.conv != ir::CallConv::ZP && !fn.paramTypes.empty()) { r.why = "the stack convention"; return r; }
    for (const auto& blk : fn.blocks)
        for (const auto& in : blk.insts)
            if ((in.op == ir::Op::CALL || in.op == ir::Op::CALL_VOID) && in.src1.kind == ir::OperandKind::GLOBAL &&
                in.src1.name == fn.name)
                { r.why = "recursive (the frame is static)"; return r; }

    Emitter e(fn);
    for (size_t i = 0; i < fn.blocks.size(); i++) e.blockIx[fn.blocks[i].label] = (int)i;
    if (!e.layout()) { r.why = e.why; return r; }
    for (size_t i = 0; i < fn.blocks.size(); i++) {
        e.pc << e.label(fn.blocks[i].label) << ":\n";
        const std::string next = i + 1 < fn.blocks.size() ? fn.blocks[i + 1].label : "";
        for (const auto& in : fn.blocks[i].insts)
            if (!e.inst(in, next)) { r.why = e.why; return r; }
        e.endBlock();
    }

    // the native entry: the parameters from the ZP block into the frame
    std::ostringstream t;
    const std::string pf = fn.name + "__pf";
    t << "; " << fn.name << ": p-code (research/pcode), its frame " << e.frame << " bytes\n";
    t << "    .segment \"bss\"\n" << pf << ":\n    .res " << std::max(1, e.frame) << "\n";
    t << "    .segment \"code\"\n";
    t << "    .global " << fn.name << "\n    .extern __pc_enter\n";
    t << "    proc " << fn.name << "\n";
    int zp = 0, stub = 0;
    for (uint32_t v = 0; v < fn.paramTypes.size(); v++) {
        int w = std::max(1, widthOfType(fn.paramTypes[v]));
        for (int k = 0; k < w; k++) {
            t << "    lda $" << std::hex << (0x10 + zp + k) << std::dec << "\n";
            t << "    sta " << pf << "+" << (e.off[v] + k) << "\n";
            stub += 5;
        }
        zp += std::max(2, w);
    }
    // (the p-code and the frame in registers, not as data after a jsr:
    // ca45's TailDedup took stubs that differed only in that data for one)
    t << "    lda #<" << fn.name << "__pc\n    ldx #>" << fn.name << "__pc\n";
    t << "    ldy #<" << pf << "\n    ldz #>" << pf << "\n    jmp __pc_enter\n";
    stub += 11;
    t << "    .reg_clobbers A, X, Y, Z\n    .flag_clobbers C, N, Z, V\n";
    // the base page from $08: the interpreter's bytes, the parameter block it
    // fills for its calls, whatever those callees change (without this, a
    // caller keeps an argument in $10 over the call, as it may over a
    // native function that leaves the block alone)
    for (int a = 0x08; a <= 0xFF; a += 16) {
        t << "    .zp_clobbers ";
        for (int k = a; k < a + 16 && k <= 0xFF; k++) {
            char h[8];
            snprintf(h, sizeof h, "$%02X", k);
            t << (k > a ? ", " : "") << h;
        }
        t << "\n";
    }
    t << "    .func_flags zp_call\n    endproc\n";
    t << fn.name << "__pc:\n" << (e.list ? "    ; PCTEMP " + std::to_string(e.k0) + "\n" : std::string()) << e.pc.str() << "\n";
    r.ok = true;
    r.text = t.str();
    r.pcodeBytes = e.bytes;
    r.stubBytes = stub;
    r.frameBytes = e.frame;
    return r;
}
