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
    void generate(const ir::Module& mod, uint32_t zpStart = 0x08, bool relocMode = false, bool zpCallMode = false);

private:
    std::ostream& out_;
    uint32_t zeroPageStart_ = 0x08;

    // Assembly emission helpers
    void emit(const std::string& line);
    void emitLabel(const std::string& label);
    void emitComment(const std::string& text);
    void emitBlank();

    // Module-level emission
    void emitStartupStub(const ir::Module& mod);
    void emitGlobals(const ir::Module& mod, bool relocMode);
    void emitStrings(const ir::Module& mod);
    void emitFunction(const ir::Function& fn, bool relocMode, bool isMainWithZPSave = false);
    void emitInst(const ir::Inst& inst);

    // vReg frame slot management (per-function)
    std::map<uint32_t, int> vregOffset_;  // vregId → frame offset
    std::map<uint32_t, ir::Type> vregType_; // vregId → type
    int frameSize_ = 0;
    int labelCounter_ = 0;

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

    // Frame management
    int localFrameSize_ = 0;
    std::map<uint32_t, int> vregSizes_; // override sizes for array vRegs

    // Register allocator
    VRegAllocator alloc_;
    int currentInstIdx_ = 0;  // tracks position during emission

    // Track which vRegs are direct local slots (from params/VariableDeclaration)
    // vs computed addresses (from LOAD/ADDR_*). STORE to a slot = direct write.
    // STORE to a non-slot = indirect write through pointer.
    std::set<uint32_t> localSlotVregs_;

    // Per-function clobber analysis
    struct FuncClobbers {
        uint8_t regs = 0;   // bit 0=A, 1=X, 2=Y, 3=Z
        uint8_t flags = 0;  // bit 0=C, 1=N, 2=Z, 3=V
        bool isLeaf = true;
    };
    FuncClobbers computeFuncClobbers(const ir::Function& fn);
};
