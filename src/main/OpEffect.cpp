#include "OpEffect.hpp"
#include <map>
#include <algorithm>

// Canonical instruction effect table, keyed by mnemonic (lowercase).
// Covers both native 45GS02 instructions and simulated pseudo-ops.
// This is the single source of truth — used by both the compiler (IRCodeGen)
// and assembler (AssemblerOptimizer) for MachineState tracking.

static const std::map<std::string, OpEffect>& getTable() {
    using E = OpEffect;
    static const std::map<std::string, OpEffect> table = {

        // =====================================================================
        // Native 6502/45GS02 instructions
        // =====================================================================

        // --- Loads ---
        {"lda",   {E::A,             true,  false, false}},
        {"ldx",   {E::X,             true,  false, false}},
        {"ldy",   {E::Y,             true,  false, false}},
        {"ldz",   {E::Z,             true,  false, false}},

        // --- Stores (don't clobber registers) ---
        {"sta",   {0,                false, true,  false}},
        {"stx",   {0,                false, true,  false}},
        {"sty",   {0,                false, true,  false}},
        {"stz",   {0,                false, true,  false}},

        // --- Transfers ---
        {"tax",   {E::X,             true,  false, false}},
        {"tay",   {E::Y,             true,  false, false}},
        {"taz",   {E::Z,             true,  false, false}},
        {"txa",   {E::A,             true,  false, false}},
        {"tya",   {E::A,             true,  false, false}},
        {"tza",   {E::A,             true,  false, false}},
        {"tsx",   {E::X,             true,  false, false}},
        {"txs",   {0,                false, false, false}},  // doesn't set flags
        {"tsy",   {E::Y,             true,  false, false}},
        {"tys",   {0,                false, false, false}},
        {"tab",   {0,                false, false, false}},  // base page reg

        // --- Arithmetic ---
        {"adc",   {E::A,             true,  false, false}},
        {"sbc",   {E::A,             true,  false, false}},
        {"inc",   {0,                true,  true,  false}},  // memory or accumulator
        {"dec",   {0,                true,  true,  false}},
        {"inx",   {E::X,             true,  false, false}},
        {"dex",   {E::X,             true,  false, false}},
        {"iny",   {E::Y,             true,  false, false}},
        {"dey",   {E::Y,             true,  false, false}},
        {"inz",   {E::Z,             true,  false, false}},
        {"dez",   {E::Z,             true,  false, false}},
        {"ina",   {E::A,             true,  false, false}},
        {"dea",   {E::A,             true,  false, false}},

        // --- Logic ---
        {"and",   {E::A,             true,  false, false}},
        {"ora",   {E::A,             true,  false, false}},
        {"eor",   {E::A,             true,  false, false}},

        // --- Shifts/Rotates ---
        {"asl",   {E::A,             true,  true,  false}},  // A or memory
        {"lsr",   {E::A,             true,  true,  false}},
        {"rol",   {E::A,             true,  true,  false}},
        {"ror",   {E::A,             true,  true,  false}},
        {"asw",   {0,                true,  true,  false}},  // memory only
        {"row",   {0,                true,  true,  false}},

        // --- Compare ---
        {"cmp",   {0,                true,  false, false}},
        {"cpx",   {0,                true,  false, false}},
        {"cpy",   {0,                true,  false, false}},
        {"cpz",   {0,                true,  false, false}},

        // --- Bit test ---
        {"bit",   {0,                true,  false, false}},
        {"trb",   {0,                true,  true,  false}},
        {"tsb",   {0,                true,  true,  false}},

        // --- Stack ---
        {"pha",   {0,                false, false, true}},
        {"pla",   {E::A,             true,  false, true}},
        {"phx",   {0,                false, false, true}},
        {"plx",   {E::X,             true,  false, true}},
        {"phy",   {0,                false, false, true}},
        {"ply",   {E::Y,             true,  false, true}},
        {"phz",   {0,                false, false, true}},
        {"plz",   {E::Z,             true,  false, true}},
        {"php",   {0,                false, false, true}},
        {"plp",   {0,                true,  false, true}},  // restores flags
        {"phw",   {0,                false, false, true}},

        // --- Flags ---
        {"clc",   {0,                true,  false, false}},
        {"sec",   {0,                true,  false, false}},
        {"cld",   {0,                true,  false, false}},
        {"sed",   {0,                true,  false, false}},
        {"cli",   {0,                false, false, false}},
        {"sei",   {0,                false, false, false}},
        {"clv",   {0,                true,  false, false}},

        // --- Branches (don't clobber anything) ---
        {"bra",   {0,                false, false, false}},
        {"bcc",   {0,                false, false, false}},
        {"bcs",   {0,                false, false, false}},
        {"beq",   {0,                false, false, false}},
        {"bne",   {0,                false, false, false}},
        {"bmi",   {0,                false, false, false}},
        {"bpl",   {0,                false, false, false}},
        {"bvc",   {0,                false, false, false}},
        {"bvs",   {0,                false, false, false}},
        {"bsr",   {0,                false, false, true}},   // branch subroutine

        // --- Jumps/Returns ---
        {"jmp",   {0,                false, false, false}},
        {"jsr",   {E::A|E::X|E::Y|E::Z, true, true, true}},  // callee may clobber anything
        {"rts",   {0,                false, false, true}},
        {"rti",   {0,                true,  false, true}},
        {"brk",   {0,                false, false, true}},

        // --- No-ops ---
        {"nop",   {0,                false, false, false}},
        {"eom",   {0,                false, false, false}},

        // --- MEGA65 specific ---
        {"neg",   {E::A,             true,  false, false}},
        {"asr",   {E::A,             true,  false, false}},  // arithmetic shift right
        {"map",   {0,                false, false, false}},

        // =====================================================================
        // Simulated pseudo-ops (expanded by AssemblerSimulatedOps)
        // =====================================================================

        // --- 16-bit ALU: result in A:X ---
        {"add.16",   {E::A|E::X,       true,  false, false}},
        {"sub.16",   {E::A|E::X,       true,  false, false}},
        {"add.s16",  {E::A|E::X,       true,  false, false}},
        {"sub.s16",  {E::A|E::X,       true,  false, false}},
        {"adds.16",  {E::A|E::X,       true,  false, false}},
        {"subs.16",  {E::A|E::X,       true,  false, false}},
        {"and.16",   {E::A|E::X,       true,  false, false}},
        {"ora.16",   {E::A|E::X,       true,  false, false}},
        {"eor.16",   {E::A|E::X,       true,  false, false}},
        {"neg.16",   {E::A|E::X,       true,  false, false}},
        {"neg.s16",  {E::A|E::X,       true,  false, false}},
        {"not.16",   {E::A|E::X,       true,  false, false}},
        {"abs.16",   {E::A|E::X,       true,  false, false}},
        {"abs.s16",  {E::A|E::X,       true,  false, false}},
        {"lsl.16",   {E::A|E::X,       true,  false, false}},
        {"lsr.16",   {E::A|E::X,       true,  false, false}},
        {"asr.16",   {E::A|E::X,       true,  false, false}},
        {"rol.16",   {E::A|E::X,       true,  false, false}},
        {"ror.16",   {E::A|E::X,       true,  false, false}},
        {"lsl.s16",  {E::A|E::X,       true,  false, false}},
        {"lsr.s16",  {E::A|E::X,       true,  false, false}},
        {"asr.s16",  {E::A|E::X,       true,  false, false}},
        {"rol.s16",  {E::A|E::X,       true,  false, false}},
        {"ror.s16",  {E::A|E::X,       true,  false, false}},
        {"sxt.8",    {E::A|E::X,       true,  false, false}},
        {"sxt.16",   {E::A|E::X|E::Y|E::Z, true, false, false}},

        // --- 16-bit compare ---
        {"cmp.16",   {E::A|E::X,       true,  false, false}},
        {"cpw",      {E::A|E::X,       true,  false, false}},  // alias for cmp.16
        {"cmp.s16",  {E::A|E::X,       true,  true,  false}},  // uses scratch ZP

        // --- 16-bit mul/div/mod (use scratch ZP, may clobber Y) ---
        {"mul.16",   {E::A|E::X|E::Y,  true,  true,  false}},
        {"mul.s16",  {E::A|E::X|E::Y,  true,  true,  false}},
        {"div.16",   {E::A|E::X|E::Y,  true,  true,  false}},
        {"div.s16",  {E::A|E::X|E::Y,  true,  true,  false}},
        {"mod.16",   {E::A|E::X|E::Y,  true,  true,  false}},
        {"mod.s16",  {E::A|E::X|E::Y,  true,  true,  false}},

        // --- 32-bit ALU: result in AXYZ ---
        {"add.32",   {E::ALL,           true,  true,  false}},
        {"sub.32",   {E::ALL,           true,  true,  false}},
        {"add.s32",  {E::ALL,           true,  true,  false}},
        {"sub.s32",  {E::ALL,           true,  true,  false}},
        {"and.32",   {E::ALL,           true,  false, false}},
        {"ora.32",   {E::ALL,           true,  false, false}},
        {"eor.32",   {E::ALL,           true,  false, false}},
        {"neg.32",   {E::ALL,           true,  true,  false}},
        {"neg.s32",  {E::ALL,           true,  true,  false}},
        {"not.32",   {E::ALL,           true,  false, false}},
        {"abs.32",   {E::ALL,           true,  true,  false}},
        {"abs.s32",  {E::ALL,           true,  true,  false}},
        {"cmp.32",   {E::ALL,           true,  true,  false}},
        {"cmp.s32",  {E::ALL,           true,  true,  false}},
        {"mul.32",   {E::ALL,           true,  true,  false}},
        {"mul.s32",  {E::ALL,           true,  true,  false}},
        {"div.32",   {E::ALL,           true,  true,  false}},
        {"div.s32",  {E::ALL,           true,  true,  false}},
        {"mod.32",   {E::ALL,           true,  true,  false}},
        {"mod.s32",  {E::ALL,           true,  true,  false}},
        {"lsl.32",   {E::ALL,           true,  false, false}},
        {"lsr.32",   {E::ALL,           true,  false, false}},
        {"asr.32",   {E::ALL,           true,  false, false}},
        {"rol.32",   {E::ALL,           true,  false, false}},
        {"ror.32",   {E::ALL,           true,  false, false}},
        {"lsl.s32",  {E::ALL,           true,  false, false}},
        {"lsr.s32",  {E::ALL,           true,  false, false}},
        {"asr.s32",  {E::ALL,           true,  false, false}},
        {"rol.s32",  {E::ALL,           true,  false, false}},
        {"ror.s32",  {E::ALL,           true,  false, false}},

        // --- Check zero/nonzero ---
        {"chkzero.8",     {E::A,              true, false, false}},
        {"chkzero.16",    {E::A|E::X,         true, false, false}},
        {"chknonzero.8",  {E::A,              true, false, false}},
        {"chknonzero.16", {E::A|E::X,         true, false, false}},

        // --- Select ---
        {"select",   {E::A|E::X,       true,  false, false}},

        // --- Load/Store word ---
        {"ldw",      {E::A|E::X,       true,  false, false}},
        {"ldw.sp",   {E::A|E::X,       true,  false, false}},
        {"stw",      {0,               false, true,  false}},
        {"stw.sp",   {0,               false, true,  false}},
        {"ldax",     {E::A|E::X,       true,  false, false}},
        {"stax",     {0,               false, true,  false}},

        // --- Swap/Zero ---
        {"swap",     {E::A|E::X|E::Y|E::Z, true, false, false}},
        {"zero",     {E::A|E::X|E::Y|E::Z, true, false, false}},

        // --- Push/Pop (simulated multi-byte) ---
        {"push",     {0,               false, false, true}},
        {"pop",      {E::A|E::X|E::Y|E::Z, true, false, true}},

        // --- Fill/Copy (DMA or loop) ---
        {"fill",     {E::A|E::X|E::Y|E::Z, true, true, false}},
        {"fill.sp",  {E::A|E::X|E::Y|E::Z, true, true, false}},
        {"move",     {E::A|E::X|E::Y|E::Z, true, true, false}},
        {"move.sp",  {E::A|E::X|E::Y|E::Z, true, true, false}},

        // --- Stack-relative load/store (uses TSX → clobbers X) ---
        {"lda.sp",   {E::A|E::X,            true,  false, false}},
        {"sta.sp",   {E::X,                  false, true,  false}},
        {"phw.sp",   {E::A,                  false, false, true}},

        // --- Flat memory ops (absolute addressing, no frame pointer) ---
        {"ldw.f",    {E::A|E::X,             true,  false, false}},
        {"stw.f",    {0,                     false, true,  false}},
        {"inc.f",    {0,                     true,  true,  false}},
        {"dec.f",    {0,                     true,  true,  false}},

        // --- Pointer ops ---
        {"ptrstack", {E::A|E::X,             true,  false, false}},
        {"ptrderef", {E::A|E::X|E::Y,        true,  false, false}},

        // --- Branch pseudo-ops ---
        {"branch.16",{0,                     false, false, false}},

        // --- Struct/array element address computation ---
        {"struct_elem",   {E::A|E::X,        true,  true,  false}},
        {"struct_elem.16",{E::A|E::X,        true,  true,  false}},
        {"addr_elem",     {E::A|E::X,        true,  true,  false}},
        {"addr_elem.16",  {E::A|E::X,        true,  true,  false}},

        // --- Frame load ops: use tsx internally → clobber X ---
        {"lda.fp",   {E::A|E::X,             true,  false, false}},
        {"ldax.fp",  {E::A|E::X,             true,  false, false}},
        {"lday.fp",  {E::A|E::X|E::Y,        true,  false, false}},
        {"ldaz.fp",  {E::A|E::X|E::Z,        true,  false, false}},
        {"ldaxyz.fp",{E::A|E::X|E::Y|E::Z,   true,  true,  false}},

        // --- Frame store ops: use tsx → clobber X; stax.fp also clobbers Z ---
        {"sta.fp",   {E::X,                   true,  true,  false}},
        {"stax.fp",  {E::X|E::Z,              true,  true,  false}},
        {"stay.fp",  {E::X,                   true,  true,  false}},
        {"staz.fp",  {E::X,                   true,  true,  false}},
        {"staxyz.fp",{E::A|E::X|E::Y|E::Z,   true,  true,  false}},

        // --- Frame address/copy ---
        {"leax.fp",  {E::A|E::X,              true,  false, false}},
        {"move.fp",  {E::A|E::X,              true,  true,  false}},

        // --- Frame inc/dec ---
        {"inc.fp",   {E::A|E::X,              true,  true,  false}},
        {"dec.fp",   {E::A|E::X,              true,  true,  false}},
        {"inc.16f",  {E::A|E::X,              true,  true,  false}},
        {"dec.16f",  {E::A|E::X,              true,  true,  false}},

        // --- Stack-relative ops ---
        {"inw",      {E::A|E::X,              true,  true,  false}},
        {"dew",      {E::A|E::X,              true,  true,  false}},

        // --- Bitfield ---
        {"bfext",    {E::A|E::X,              true,  true,  false}},
        {"bfext16",  {E::A|E::X,              true,  true,  false}},
        {"bfext32",  {E::A|E::X|E::Y|E::Z,   true,  true,  false}},
        {"bfins",    {E::A|E::X,              true,  true,  false}},
        {"bfins.sp", {E::A|E::X,              true,  true,  false}},
        {"bfins.ind",{E::A|E::X,              true,  true,  false}},
        {"bfins16",  {E::A|E::X,              true,  true,  false}},
        {"bfins16.sp",  {E::A|E::X,           true,  true,  false}},
        {"bfins16.ind", {E::A|E::X,           true,  true,  false}},
        {"bfins32",  {E::A|E::X|E::Y|E::Z,   true,  true,  false}},

        // --- Proc/endproc (structural, not real ops) ---
        {"proc",     {0,                      false, false, false}},
        {"endproc",  {0,                      false, false, true}},
    };
    return table;
}

// Default effect for unknown mnemonics: clobber everything (safe)
static const OpEffect unknownEffect = {OpEffect::ALL, true, true, true};

const OpEffect& getOpEffect(const std::string& mnemonic) {
    // Normalize to lowercase for lookup
    std::string lower = mnemonic;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // Strip trailing operand text if present (e.g., "add.16 .AX, $20" → "add.16")
    auto space = lower.find(' ');
    if (space != std::string::npos) lower = lower.substr(0, space);

    const auto& table = getTable();
    auto it = table.find(lower);
    if (it != table.end()) return it->second;
    return unknownEffect;
}
