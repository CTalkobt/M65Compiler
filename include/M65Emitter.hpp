#pragma once
#include <vector>
#include <string>
#include <ostream>
#include <cstdint>
#include "AssemblerTypes.hpp"
#include "MachineState.hpp"

class M65Emitter {
public:
    enum class Mode { TEXT, BINARY };

    M65Emitter(std::ostream& out, uint32_t zpStart = 0x02);
    M65Emitter(std::vector<uint8_t>& binary, uint32_t zpStart = 0x02);

    uint8_t getZP(int offset) const { return (uint8_t)(zeroPageStart + offset); }
    Mode getMode() const { return mode; }

    void setSpBase(uint16_t base) { spBase_ = base; }
    uint16_t spBase() const { return spBase_; }

    void setScratchZP(uint8_t addr) { scratchZP_ = addr; scratchZP2_ = addr + 2; scratchZP3_ = addr + 4; }
    uint8_t scratchZP() const { return scratchZP_; }
    uint8_t scratchZP2() const { return scratchZP2_; }
    uint8_t scratchZP3() const { return scratchZP3_; }

    // Frame pointer: ZP pair used for (ZP),Y stack access
    // When active (non-zero), stack-relative ops use ($FP),Y instead of TSX+abs,X
    void setFramePointerZP(uint8_t addr) { framePointerZP_ = addr; }
    uint8_t framePointerZP() const { return framePointerZP_; }
    bool hasFramePointer() const { return framePointerZP_ != 0; }

    // Emit FP setup: save old FP, compute new FP from current SP
    void setupFramePointer();
    // Emit FP restore: pop old FP from stack
    void restoreFramePointer();

    // Scratch ZP access — single bytes used as temps by simulated ops
    void lda_scratch() { lda_zp(scratchZP_); }
    void sta_scratch() { sta_zp(scratchZP_); }
    void ldx_scratch() { ldx_zp(scratchZP_); }
    void stx_scratch() { stx_zp(scratchZP_); }
    void ldy_scratch() { ldy_zp(scratchZP_); }
    void sty_scratch() { sty_zp(scratchZP_); }

    void lda_scratch2() { lda_zp(scratchZP2_); }
    void sta_scratch2() { sta_zp(scratchZP2_); }
    void ldx_scratch2() { ldx_zp(scratchZP2_); }
    void stx_scratch2() { stx_zp(scratchZP2_); }

    void lda_scratch3() { lda_zp(scratchZP3_); }
    void sta_scratch3() { sta_zp(scratchZP3_); }
    void ldy_scratch3() { ldy_zp(scratchZP3_); }
    void sty_scratch3() { sty_zp(scratchZP3_); }

    void lda_scratch3_hi() { lda_zp(scratchZP3_ + 1); }
    void sta_scratch3_hi() { sta_zp(scratchZP3_ + 1); }
    void ldz_scratch3_hi() { ldz_zp(scratchZP3_ + 1); }
    void stz_scratch3_hi() { stz_zp(scratchZP3_ + 1); }

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
    void inc_abs(uint16_t addr);
    void dec_abs(uint16_t addr);
    void inc_abs_x(uint16_t addr);
    void dec_abs_x(uint16_t addr);
    void asw_abs(uint16_t addr);
    void row_abs(uint16_t addr);

    // --- Zero Page Mode ---
    void lda_zp(uint8_t addr);
    void ldx_zp(uint8_t addr);
    void ldy_zp(uint8_t addr);
    void ldz_zp(uint8_t addr);
    void sta_zp(uint8_t addr);
    void stx_zp(uint8_t addr);
    void sty_zp(uint8_t addr);
    void stz_zp(uint8_t addr);
    void adc_zp(uint8_t addr);
    void sbc_zp(uint8_t addr);
    void and_zp(uint8_t addr);
    void ora_zp(uint8_t addr);
    void eor_zp(uint8_t addr);
    void cmp_zp(uint8_t addr);
    void bit_zp(uint8_t addr);
    void asl_zp(uint8_t addr);
    void lsr_zp(uint8_t addr);
    void rol_zp(uint8_t addr);
    void ror_zp(uint8_t addr);
    void inc_zp(uint8_t addr);
    void dec_zp(uint8_t addr);


    // --- Auto-selecting ZP/Absolute with constant/reloc-memory forwarding ---
    // For read-type ops: if MachineState knows the ZP location holds a constant
    // or a relocatable symbol byte, use immediate mode instead.
    // For RELOC_CONST, emit immediate + record the appropriate relocation.
private:
    // Helper: emit an immediate load/ALU with relocation if the value is RELOC_CONST.
    // 'emitImm' is the immediate emitter (e.g., lda_imm), 'emitZP'/'emitAbs' are fallbacks.
    template<typename FnImm, typename FnZP, typename FnAbs>
    void addrReadHelper(uint16_t a, FnImm emitImm, FnZP emitZP, FnAbs emitAbs) {
        if (a < 256) {
            const auto& zv = ms_.getZP((uint8_t)a);
            if (zv.isConst()) { emitImm((uint8_t)(zv.constVal & 0xFF)); return; }
            if (zv.isReloc()) {
                // Emit immediate mode with a relocation record
                if (zv.relocByte == ValueInfo::RELOC_LO)
                    recordSymbolRelocLo(zv.relocSymbol);
                else
                    recordSymbolRelocHi(zv.relocSymbol, (uint8_t)(zv.constVal & 0xFF));
                emitImm((uint8_t)(zv.constVal & 0xFF));
                return;
            }
            emitZP((uint8_t)a);
        } else {
            emitAbs(a);
        }
    }
public:
    void lda_addr(uint16_t a) { addrReadHelper(a, [&](uint8_t v){ lda_imm(v); }, [&](uint8_t z){ lda_zp(z); }, [&](uint16_t ab){ lda_abs(ab); }); }
    void ldx_addr(uint16_t a) { addrReadHelper(a, [&](uint8_t v){ ldx_imm(v); }, [&](uint8_t z){ ldx_zp(z); }, [&](uint16_t ab){ ldx_abs(ab); }); }
    void ldy_addr(uint16_t a) { addrReadHelper(a, [&](uint8_t v){ ldy_imm(v); }, [&](uint8_t z){ ldy_zp(z); }, [&](uint16_t ab){ ldy_abs(ab); }); }
    void ldz_addr(uint16_t a) { addrReadHelper(a, [&](uint8_t v){ ldz_imm(v); }, [&](uint8_t z){ ldz_zp(z); }, [&](uint16_t ab){ ldz_abs(ab); }); }
    void adc_addr(uint16_t a) { addrReadHelper(a, [&](uint8_t v){ adc_imm(v); }, [&](uint8_t z){ adc_zp(z); }, [&](uint16_t ab){ adc_abs(ab); }); }
    void sbc_addr(uint16_t a) { addrReadHelper(a, [&](uint8_t v){ sbc_imm(v); }, [&](uint8_t z){ sbc_zp(z); }, [&](uint16_t ab){ sbc_abs(ab); }); }
    void and_addr(uint16_t a) { addrReadHelper(a, [&](uint8_t v){ and_imm(v); }, [&](uint8_t z){ and_zp(z); }, [&](uint16_t ab){ and_abs(ab); }); }
    void ora_addr(uint16_t a) { addrReadHelper(a, [&](uint8_t v){ ora_imm(v); }, [&](uint8_t z){ ora_zp(z); }, [&](uint16_t ab){ ora_abs(ab); }); }
    void eor_addr(uint16_t a) { addrReadHelper(a, [&](uint8_t v){ eor_imm(v); }, [&](uint8_t z){ eor_zp(z); }, [&](uint16_t ab){ eor_abs(ab); }); }
    void cmp_addr(uint16_t a) { addrReadHelper(a, [&](uint8_t v){ cmp_imm(v); }, [&](uint8_t z){ cmp_zp(z); }, [&](uint16_t ab){ cmp_abs(ab); }); }
    // Store and RMW ops: no forwarding (they write to memory)
    void sta_addr(uint16_t a) { if (a < 256) sta_zp((uint8_t)a); else sta_abs(a); }
    void stx_addr(uint16_t a) { if (a < 256) stx_zp((uint8_t)a); else stx_abs(a); }
    void sty_addr(uint16_t a) { if (a < 256) sty_zp((uint8_t)a); else sty_abs(a); }
    void stz_addr(uint16_t a) { if (a < 256) stz_zp((uint8_t)a); else stz_abs(a); }
    void inc_addr(uint16_t a) { if (a < 256) inc_zp((uint8_t)a); else inc_abs(a); }
    void dec_addr(uint16_t a) { if (a < 256) dec_zp((uint8_t)a); else dec_abs(a); }
    void asl_addr(uint16_t a) { if (a < 256) asl_zp((uint8_t)a); else asl_abs(a); }
    void lsr_addr(uint16_t a) { if (a < 256) lsr_zp((uint8_t)a); else lsr_abs(a); }
    void rol_addr(uint16_t a) { if (a < 256) rol_zp((uint8_t)a); else rol_abs(a); }
    void ror_addr(uint16_t a) { if (a < 256) ror_zp((uint8_t)a); else ror_abs(a); }
    void bit_addr(uint16_t a) { if (a < 256) bit_zp((uint8_t)a); else bit_abs(a); }

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
    void lda_stack_noTSX(uint8_t offset); // LDA __sp_base+offset,X without tsx
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
    std::vector<uint8_t>* getBinary() { return binary; }

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
    void emitWordReloc(const std::string& symbolName, uint16_t value = 0);
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
    uint8_t scratchZP_ = 0x02;
    uint8_t scratchZP2_ = 0x04;
    uint8_t scratchZP3_ = 0x06;
    uint8_t framePointerZP_ = 0; // 0 = disabled; when set, stack ops use ($FP),Y
    uint32_t currentAddress = 0;
    bool addressSet = false;
    MachineState ms_;     // Unified register/flag value tracking
    void emitText(const std::string& mnemonic, const std::string& operand = "");
public:
    MachineState& machineState() { return ms_; }
    const MachineState& machineState() const { return ms_; }
    void invalidateXSP(); // Call when X may have been modified externally
    void tsxCached();     // Emit TSX only if X doesn't already hold SP
};
