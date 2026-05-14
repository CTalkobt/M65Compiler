#pragma once
#include "IR.hpp"
#include "VRegAllocator.hpp"
#include <ostream>
#include <map>
#include <string>
#include <vector>

class IRCodeGen {
public:
    IRCodeGen(std::ostream& out);

    // Generate assembly for the entire module.
    // relocMode: true = .o45 (emit .global/.extern, no startup stub)
    //            false = PRG (emit .org $2000 + startup stub)
    void generate(const ir::Module& mod, bool relocMode = false, bool zpCallMode = false);

private:
    std::ostream& out_;

    // Assembly emission helpers
    void emit(const std::string& line);
    void emitLabel(const std::string& label);
    void emitComment(const std::string& text);
    void emitBlank();

    // Module-level emission
    void emitStartupStub(const ir::Module& mod);
    void emitGlobals(const ir::Module& mod, bool relocMode);
    void emitStrings();
    void emitFunction(const ir::Function& fn, bool relocMode);
    void emitInst(const ir::Inst& inst);

    // vReg frame slot management (per-function)
    std::map<uint32_t, int> vregOffset_;  // vregId → frame offset
    std::map<uint32_t, ir::Type> vregType_; // vregId → type
    int frameSize_ = 0;

    void resetFrame();
    int allocSlot(uint32_t vregId, ir::Type type);
    int slotOf(uint32_t vregId);

    // Load a vReg value into A (I8) or A:X (I16) or A:X:Y:Z (I32)
    void loadVreg(uint32_t vregId);
    // Store from A / A:X / A:X:Y:Z into a vReg frame slot
    void storeVreg(uint32_t vregId);
    // Load any operand into A:X
    void loadOperand(const ir::Operand& op);
    // Get a memory operand string for src2 in binary ops (for simulated ops like add.16)
    // Returns "#imm" for immediates, "$ZZ" for ZP vRegs, "symbol, s" for frame vRegs.
    // If the vReg is in A:X, spills it to __zp_scratch first and returns the ZP addr.
    std::string src2MemOperand(const ir::Operand& op);

    // Pre-scan a function to allocate frame slots for all vRegs
    void prescanFunction(const ir::Function& fn);

    // String pool
    std::map<std::string, std::string> stringPool_;
    int stringCount_ = 0;

    // Current function metadata
    bool relocMode_ = false;
    bool zpCallMode_ = false;

    // Register allocator
    VRegAllocator alloc_;
    int currentInstIdx_ = 0;  // tracks position during emission
};
