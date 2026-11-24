// The allocators on their own (#248): SacSlotShare (#324) on assembly text,
// VRegAllocator on hand-built IR functions. (ln45's static-allocation and
// base-page overlays, which need a program's call graph, are checked through
// real objects by src/test/test_alloc_report.sh.)
#include <cstdio>
#include <sstream>
#include <string>
#include "test_harness.hpp"
#include "SacSlotShare.hpp"
#include "VRegAllocator.hpp"

namespace {

// A SAC function _f with two slots, 1 and 4, and its code; extra: lines after it
std::string sacFunction(const std::string& code, const std::string& extra = "", int s1 = 2, int s4 = 2) {
    return "    .segment \"bss\"\n"
           "_f__ar:\n"
           "_f__local_1:\n"
           "    .res " + std::to_string(s1) + "\n"
           "_f__local_4:\n"
           "    .res " + std::to_string(s4) + "\n"
           "    .res 1\n"
           "    .global cc45.sacsize._f\n"
           "    cc45.sacsize._f = " + std::to_string(s1 + s4 + 1) + "\n"
           "    .segment \"code\"\n"
           "    proc _f\n" + code +
           "    rts\n"
           "    endproc\n" + extra;
}

// the slots each other: one .res after both labels
bool shared(const std::string& t) {
    return t.find("_f__local_1:\n_f__local_4:\n    .res 2\n") != std::string::npos ||
           t.find("_f__local_4:\n_f__local_1:\n    .res 2\n") != std::string::npos;
}

int sacSize(const std::string& t) {
    size_t at = t.find("cc45.sacsize._f = ");
    return at == std::string::npos ? -1 : std::stoi(t.substr(at + 18));
}

// two values never live together: each branch writes its slot, then reads it back
const char* BRANCHES =
    "    lda $10\n"
    "    beq @else\n"
    "    jsr _h\n"
    "    sta _f__local_1\n"
    "    stx _f__local_1+1\n"
    "    jsr _g\n"
    "    lda _f__local_1\n"
    "    ldx _f__local_1+1\n"
    "    jsr _g\n"
    "    bra @end\n"
    "@else:\n"
    "    jsr _h\n"
    "    sta _f__local_4\n"
    "    stx _f__local_4+1\n"
    "    jsr _g\n"
    "    lda _f__local_4\n"
    "    ldx _f__local_4+1\n"
    "    jsr _g\n"
    "@end:\n";

void test_sac_share() {
    std::string out = shareSacSlots(sacFunction(BRANCHES));
    CHECK(shared(out), "slots of two branches share one storage");
    CHECK(sacSize(out) == 3, "cc45.sacsize is the block's size after sharing (2 + pad)");

    // #790: the slots' debug records and base-page plan records name them
    // outside the function; they are not uses of the bytes
    std::string recs =
        "    .global cc45.zc._f__local_1\n"
        "    cc45.zc._f__local_1 = 514\n"
        "    .global cc45.zc._f__local_4\n"
        "    cc45.zc._f__local_4 = 514\n";
    std::string dbg =
        "    .dbg_var \"local _f a u16 sym:_f__local_1\"\n"
        "    .dbg_var \"local _f b u16 sym:_f__local_4\"\n";
    out = shareSacSlots(recs + sacFunction(BRANCHES, dbg));
    CHECK(shared(out), "#790: shared with cc45.zc.<slot> and .dbg_var records naming the slots");

    // #790: the stack convention's entry thunk, a call with its argument
    // list after it (data never run), does not make every slot live
    std::string thunk = std::string("    .extern __sac_args\n    jsr __sac_args\n    .loc \"a.c\", 3\n"
                                    "    .byte 2\n    .word _f__param_c\n    .byte 0\n_f__sac:\n") + BRANCHES;
    out = shareSacSlots(sacFunction(thunk));
    CHECK(shared(out), "#790: shared after the entry thunk's jsr __sac_args and its argument list");
    out = shareSacSlots(sacFunction(std::string("    jsr __sac_args\n    .word _f__local_1\n") + BRANCHES));
    CHECK(!shared(out), "a slot named in a call's argument list keeps its storage");
    out = shareSacSlots(sacFunction(std::string("    .byte 1\n") + BRANCHES));
    CHECK(!shared(out), "data in the code not after a call: every slot live, nothing shared");

    // values live at the same time keep their storage
    const char* TOGETHER =
        "    jsr _h\n"
        "    sta _f__local_1\n"
        "    jsr _h\n"
        "    sta _f__local_4\n"
        "    lda _f__local_1\n"
        "    adc _f__local_4\n";
    out = shareSacSlots(sacFunction(TOGETHER));
    CHECK(!shared(out) && sacSize(out) == 5, "slots live together keep their own storage");

    // an address taken: the slot keeps its own storage
    std::string addr = std::string(BRANCHES) + "    lda #<_f__local_1\n";
    out = shareSacSlots(sacFunction(addr));
    CHECK(!shared(out), "a slot whose address is taken keeps its storage");

    // named by code outside the function: kept
    out = shareSacSlots(sacFunction(BRANCHES, "    proc _other\n    lda _f__local_4\n    rts\n    endproc\n"));
    CHECK(!shared(out), "a slot another function's code names keeps its storage");

    // an indexed operand reads every byte of the slot (whatever the index)
    const char* INDEXED =
        "    jsr _h\n"
        "    sta _f__local_1\n"
        "    jsr _h\n"
        "    sta _f__local_4\n"
        "    ldy #1\n"
        "    lda _f__local_1,y\n";
    out = shareSacSlots(sacFunction(INDEXED));
    CHECK(!shared(out), "an indexed read keeps the slot live (not shared with one written before it)");

    // setjmp: the function is left alone (longjmp returns into it)
    std::string sj = std::string("    jsr _setjmp\n") + BRANCHES;
    out = shareSacSlots(sacFunction(sj));
    CHECK(!shared(out) && sacSize(out) == 5, "a function that calls setjmp is left as it is");

    // a slot read before any write is live at the entry: kept apart from the others live there
    const char* ENTRY =
        "    lda _f__local_1\n"
        "    lda _f__local_4\n";
    out = shareSacSlots(sacFunction(ENTRY));
    CHECK(!shared(out), "slots live at the entry together are kept apart");

    // no SAC block: the text is unchanged
    std::string plain = "    proc _g\n    rts\n    endproc\n";
    CHECK(shareSacSlots(plain) == plain, "text without SAC slots is unchanged");
}

// A one-block function of the given instructions
ir::Function fnOf(const std::vector<ir::Inst>& insts, uint32_t nextVreg) {
    ir::Function fn;
    fn.name = "_t";
    ir::Block b;
    b.label = "entry";
    b.insts = insts;
    fn.blocks.push_back(b);
    fn.nextVreg = nextVreg;
    for (uint32_t v = 0; v < nextVreg; v++) fn.vregTypes[v] = ir::Type::I16;
    return fn;
}

ir::Inst konst(uint32_t d, int64_t v) {
    ir::Inst i; i.op = ir::Op::CONST; i.dest = ir::Operand::vreg(d, ir::Type::I16);
    i.src1 = ir::Operand::imm(v, ir::Type::I16); i.resultType = ir::Type::I16; return i;
}
ir::Inst add(uint32_t d, uint32_t a, uint32_t b) {
    ir::Inst i; i.op = ir::Op::ADD; i.dest = ir::Operand::vreg(d, ir::Type::I16);
    i.src1 = ir::Operand::vreg(a, ir::Type::I16); i.src2 = ir::Operand::vreg(b, ir::Type::I16);
    i.resultType = ir::Type::I16; return i;
}
ir::Inst ret(uint32_t v) {
    ir::Inst i; i.op = ir::Op::RET; i.src1 = ir::Operand::vreg(v, ir::Type::I16); i.resultType = ir::Type::I16; return i;
}

bool sameLocation(const VRegAllocator& a, uint32_t x, uint32_t y) {
    auto p = a.getAlloc(x), q = a.getAlloc(y);
    if (p.loc != q.loc) return false;
    if (p.loc == VRegAllocator::IN_ZP || p.loc == VRegAllocator::IN_FRAME) return p.offset == q.offset;
    return true;
}

void test_vreg_allocator() {
    // v0, v1 live together (both read by the add); v2 = v0 + v1; v3 = 5; v4 = v2 + v3
    ir::Function fn = fnOf({konst(0, 1), konst(1, 2), add(2, 0, 1), konst(3, 5), add(4, 2, 3), ret(4)}, 5);
    VRegAllocator a;
    a.analyze(fn, 0x30, 40);
    const VRegAllocator::LiveRange* r0 = nullptr;
    for (const auto& r : a.liveRanges()) if (r.vregId == 0) r0 = &r;
    CHECK(r0 && r0->firstDef == 0 && r0->lastUse == 2, "v0's live range: defined at 0, last used at 2");
    for (uint32_t v = 0; v < 5; v++) CHECK(a.hasAlloc(v), ("v" + std::to_string(v) + " has a location").c_str());
    CHECK(!sameLocation(a, 0, 1), "two values live at the same time are in different places");
    CHECK(!sameLocation(a, 2, 3), "v2 and v3 (both read by the last add) are in different places");
    CHECK(a.frameSize() == 0, "a few short-lived values need no frame");

    // a variable in memory (volatile, or its address taken): a frame slot
    fn = fnOf({konst(0, 1), konst(1, 2), add(2, 0, 1), ret(2)}, 3);
    fn.memoryVregs.insert(1);
    VRegAllocator m;
    m.analyze(fn, 0x30, 40);
    CHECK(m.getAlloc(1).loc == VRegAllocator::IN_FRAME, "a memory vreg is given a frame slot");
    CHECK(m.frameSize() >= 2, "the frame holds it");

    // a loop counter marked for X
    fn = fnOf({konst(0, 0), konst(1, 3), add(2, 0, 1), ret(2)}, 3);
    fn.registerXVregs.insert(0);
    fn.vregTypes[0] = ir::Type::I8;
    VRegAllocator x;
    x.setRegisterXVregs({0});
    x.analyze(fn, 0x30, 40);
    CHECK(x.getAlloc(0).loc == VRegAllocator::IN_X, "a vreg marked for X is in X");

    // a fixed ZP vreg (a leaf's parameter in the parameter block) stays where it is
    fn = fnOf({konst(1, 2), add(2, 0, 1), ret(2)}, 3);
    fn.paramTypes = {ir::Type::I16};
    VRegAllocator z;
    z.setFixedZpVregs({{0, 0x10}});
    z.analyze(fn, 0x30, 40);
    CHECK(z.getAlloc(0).loc == VRegAllocator::IN_ZP && z.getAlloc(0).offset == 0x10,
          "a fixed ZP vreg is at its address ($10)");
    CHECK(z.getAlloc(1).loc != VRegAllocator::IN_ZP || (z.getAlloc(1).offset != 0x10 && z.getAlloc(1).offset != 0x11),
          "nothing else is put on the fixed vreg's bytes");
}

}  // namespace

int main() {
    test_sac_share();
    test_vreg_allocator();
    printf("\nAllocator Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
