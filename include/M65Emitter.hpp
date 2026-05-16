#pragma once
#include <vector>
#include <string>
#include <ostream>
#include <cstdint>
#include "AssemblerTypes.hpp"

class M65Emitter {
public:
    enum class Mode { TEXT, BINARY };

    M65Emitter(std::ostream& out, uint32_t zpStart = 0x02);
    M65Emitter(std::vector<uint8_t>& binary, uint32_t zpStart = 0x02);

    uint8_t getZP(int offset) const { return (uint8_t)(zeroPageStart + offset); }
    Mode getMode() const { return mode; }

    void setSpBase(uint16_t base) { spBase_ = base; }
    uint16_t spBase() const { return spBase_; }

    void setScratchZP(uint8_t addr) { scratchZP_ = addr; }
    uint8_t scratchZP() const { return scratchZP_; }

    // Frame pointer: ZP pair used for (ZP),Y stack access
    // When active (non-zero), stack-relative ops use ($FP),Y instead of TSX+abs,X
    void setFramePointerZP(uint8_t addr) { framePointerZP_ = addr; }
    uint8_t framePointerZP() const { return framePointerZP_; }
    bool hasFramePointer() const { return framePointerZP_ != 0; }

    // Emit FP setup: save old FP, compute new FP from current SP
    void setupFramePointer();
    // Emit FP restore: pop old FP from stack
    void restoreFramePointer();

    // Scratch ZP access — single byte used as temp by simulated ops
    void lda_scratch() { lda_zp(scratchZP_); }
    void sta_scratch() { sta_zp(scratchZP_); }
    void stx_scratch() { stx_zp(scratchZP_); }
    void sty_scratch() { emitInstruction("sty", AddressingMode::BASE_PAGE, scratchZP_, true); }
    void ldx_scratch() { emitInstruction("ldx", AddressingMode::BASE_PAGE, scratchZP_, true); }
    void cmp_scratch() { cmp_zp(scratchZP_); }

    void emitInstruction(const std::string& mnemonic, AddressingMode mode, uint32_t value = 0, bool hasValue = false);
    void emitLabel(const std::string& label);
    void emitComment(const std::string& comment);

    // --- Immediate Mode ---
    void lda_imm(uint8_t val);
    void ldx_imm(uint8_t val);
    void ldy_imm(uint8_t val);
    void ldz_imm(uint8_t val);
    void phw_imm(uint16_t val);
    void adc_imm(uint8_t val);
    void sbc_imm(uint8_t val);
    void and_imm(uint8_t val);
    void ora_imm(uint8_t val);
    void eor_imm(uint8_t val);
    void cmp_imm(uint8_t val);
    void cpx_imm(uint8_t val);
    void cpy_imm(uint8_t val);

    // --- Absolute Mode ---
    void lda_abs(uint16_t addr);
    void ldx_abs(uint16_t addr);
    void ldy_abs(uint16_t addr);
    void ldz_abs(uint16_t addr);
    void sta_abs(uint16_t addr);
    void stx_abs(uint16_t addr);
    void sty_abs(uint16_t addr);
    void stz_abs(uint16_t addr);
    void lda_abs_x(uint16_t addr);
    void sta_abs_x(uint16_t addr);
    void stz_abs_x(uint16_t addr);
    void adc_abs(uint16_t addr);
    void sbc_abs(uint16_t addr);
    void and_abs(uint16_t addr);
    void ora_abs(uint16_t addr);
    void eor_abs(uint16_t addr);
    void cmp_abs(uint16_t addr);
    void asl_abs(uint16_t addr);
    void rol_abs(uint16_t addr);
    void lsr_abs(uint16_t addr);
    void ror_abs(uint16_t addr);
    void asw_abs(uint16_t addr);
    void row_abs(uint16_t addr);

    // --- Zero Page Mode ---
    void lda_zp(uint8_t addr);
    void sta_zp(uint8_t addr);
    void stx_zp(uint8_t addr);
    void stz_zp(uint8_t addr);
    void adc_zp(uint8_t addr);
    void sbc_zp(uint8_t addr);
    void and_zp(uint8_t addr);
    void ora_zp(uint8_t addr);
    void eor_zp(uint8_t addr);
    void inc_zp(uint8_t addr);
    void dec_zp(uint8_t addr);
    void inc_abs(uint16_t addr);
    void dec_abs(uint16_t addr);
    void bit_zp(uint8_t addr);
    void cmp_zp(uint8_t addr);
    void inc_abs_x(uint16_t addr);
    void dec_abs_x(uint16_t addr);

    // --- Scratchpad (Relative to zeroPageStart) ---
    void lda_s(uint8_t index) { lda_zp(getZP(index)); }
    void sta_s(uint8_t index) { sta_zp(getZP(index)); }
    void stx_s(uint8_t index) { stx_zp(getZP(index)); }
    void stz_s(uint8_t index) { stz_zp(getZP(index)); }
    void lda_ind_zs(uint8_t index, bool flat = false) { lda_ind_z(getZP(index), flat); }
    void sta_ind_zs(uint8_t index, bool flat = false) { sta_ind_z(getZP(index), flat); }
    void adc_s(uint8_t index) { adc_zp(getZP(index)); }
    void sbc_s(uint8_t index) { sbc_zp(getZP(index)); }
    void and_s(uint8_t index) { and_zp(getZP(index)); }
    void ora_s(uint8_t index) { ora_zp(getZP(index)); }
    void eor_s(uint8_t index) { eor_zp(getZP(index)); }
    void inc_s(uint8_t index) { inc_zp(getZP(index)); }
    void dec_s(uint8_t index) { dec_zp(getZP(index)); }
    void bit_s(uint8_t index) { bit_zp(getZP(index)); }
    void cmp_s(uint8_t index) { cmp_zp(getZP(index)); }

    // --- Other Addressing Modes ---
    void lda_stack(uint8_t offset);
    void ldx_stack(uint8_t offset);
    void ldy_stack(uint8_t offset);
    void ldz_stack(uint8_t offset);
    void sta_stack(uint8_t offset);
    void stx_stack(uint8_t offset);
    void sty_stack(uint8_t offset);
    void stz_stack(uint8_t offset);

    // Frame-pointer-relative access via ($nn,SP),Y addressing
    void lda_frame(uint8_t fpOff, uint8_t yOff);
    void sta_frame(uint8_t fpOff, uint8_t yOff);
    void lda_ind_z(uint8_t addr, bool flat = false);
    void sta_ind_z(uint8_t addr, bool flat = false);
    void bit_abs(uint16_t addr);

    // --- Register Transfers ---
    void tax();
    void txa();
    void tay();
    void tya();
    void taz();
    void tza();
    void tsx();
    void txs();
    void tsy();
    void tys();
    void inx();
    void dex();
    void iny();
    void dey();
    void inz();
    void dez();

    // --- Stack Operations ---
    void push(const std::string& reg);
    void pop(const std::string& reg);
    void push_ax() { push(".ax"); }
    void pop_ax() { pop(".ax"); }
    void pha();
    void pla();
    void phx();
    void plx();
    void phy();
    void ply();
    void phz();
    void plz();

    // --- ALU & Branching ---
    void clc();
    void sec();
    void cla();
    void clx();
    void cly();
    void clz();
    void neg_a();
    void asl_a();
    void rol_a();
    void lsr_a();
    void ror_a();
    void inc_a();
    void dec_a();
    void eom();

    void bra(int8_t offset);
    void bne(int8_t offset);
    void beq(int8_t offset);
    void bcc(int8_t offset);
    void bcs(int8_t offset);
    void bmi(int8_t offset);
    void bpl(int8_t offset);

    // Label-based branches (TEXT mode only)
    void bra_label(const std::string& label);
    void bne_label(const std::string& label);
    void beq_label(const std::string& label);
    void bcc_label(const std::string& label);
    void bcs_label(const std::string& label);
    void bmi_label(const std::string& label);
    void bpl_label(const std::string& label);

    // --- Branch patching (binary mode) ---
    // Use these to avoid hardcoded branch offsets in simulated ops.
    // emitBranchPlaceholder() emits a 2-byte branch with offset 0, returns the position.
    // patchBranchTarget() patches the offset to jump to the current position.
    size_t pos() const { return binary ? binary->size() : 0; }
    size_t emitBranchPlaceholder(uint8_t opcode);
    void patchBranchTarget(size_t branchPos);

    // --- High-level Helpers (16-bit) ---
    void add_16_imm(uint16_t val);
    void sub_16_imm(uint16_t val);
    void neg_16();
    void not_16();
    void transfer_ax_to_zp(uint8_t addr);

    // --- High-level Helpers (32-bit, .AXYZ) ---
    void add_32_imm(uint32_t val);
    void sub_32_imm(uint32_t val);
    void neg_32();
    void not_32();
    void sxt_16();  // sign-extend AX to AXYZ

    void emitDirective(const std::string& name, const std::string& arg = "");
    void emitRaw(const std::string& text);
    void emitByte(uint8_t b);
    void emitWord(uint16_t w);
    void setAddress(uint32_t addr);
    uint32_t getAddress() const { return currentAddress; }

    // Stack-base relocation tracking for O45 emission.
    // Each entry records the address of a 16-bit operand that references __sp_base+addend.
    struct SpBaseReloc {
        uint32_t address;   // address of the 16-bit operand in the binary
        uint16_t addend;    // offset from __sp_base (e.g. 3 for __sp_base+3)
    };
    const std::vector<SpBaseReloc>& spBaseRelocs() const { return spBaseRelocs_; }
    void clearSpBaseRelocs() { spBaseRelocs_.clear(); }
    void recordSpBaseReloc(uint16_t addend);

    // Symbol relocation tracking for simulated ops that emit absolute address references
    // to named symbols (global variables accessed via ldax/stax/ptrstack etc.)
    struct SymbolReloc {
        uint32_t address;      // address of the operand byte(s) in the binary
        std::string symbolName;
        uint8_t relocType = 0x80; // R_WORD default; can be R_LOW (0x20) or R_HIGH (0x40)
        uint8_t extra = 0;        // low byte for R_HIGH carry correction
    };
    const std::vector<SymbolReloc>& symbolRelocs() const { return symbolRelocs_; }
    void recordSymbolReloc(const std::string& name);
    void recordSymbolRelocLo(const std::string& name);
    void recordSymbolRelocHi(const std::string& name, uint8_t lowByte);
    void recordSymbolReloc32Bit(const std::string& name);

private:
    std::vector<SpBaseReloc> spBaseRelocs_;
    std::vector<SymbolReloc> symbolRelocs_;
    std::ostream* out = nullptr;
    std::vector<uint8_t>* binary = nullptr;
    Mode mode;
    uint32_t zeroPageStart;
    uint16_t spBase_ = 0x0101;
    uint8_t scratchZP_ = 0x02;  // default; overridden by compiler via __zp_scratch
    uint8_t framePointerZP_ = 0; // 0 = disabled; when set, stack ops use ($FP),Y
    uint32_t currentAddress = 0;
    bool addressSet = false;
    void emitText(const std::string& mnemonic, const std::string& operand = "");
};
