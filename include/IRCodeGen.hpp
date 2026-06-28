#pragma once
#include "IR.hpp"
#include "VRegAllocator.hpp"
#include "MachineState.hpp"
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
    void generate(const ir::Module& mod, uint32_t zpStart = 0x08, bool relocMode = false, bool zpCallMode = false, bool emitReasons = false);

    // Set the line-to-file mapping from Lexer for proper source file attribution in .loc directives
    void setLineToFileMap(const std::map<int, std::pair<std::string, int>>& map) {
        lineToFileMap_ = map;
    }
    void setFunctionMap(const std::map<std::string, const ir::Function*>* map) {
        functionMap_ = map;
    }

private:
    std::ostream& out_;
    uint32_t zeroPageStart_ = 0x08;

    // Assembly emission helpers
    void emit(const std::string& line, const std::string& reason = "");
    void emitLabel(const std::string& label);
    void emitComment(const std::string& text);
    void emitBlank();

    // Codegen reasoning trace (enabled by -Rcodegen)
    bool emitReasons_ = false;

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

    // Describe a vReg for codegen reasoning comments
    std::string vregDesc(uint32_t vregId);
    // Load a vReg value into A (I8) or A:X (I16) or A:X:Y:Z (I32)
    void loadVreg(uint32_t vregId);
    // Load only A from an I8 vReg (no ldx #0 zero-extension). Falls back to loadVreg for non-I8.
    void loadVregA(uint32_t vregId);
    // Store from A / A:X / A:X:Y:Z into a vReg frame slot
    void storeVreg(uint32_t vregId);
    // Load any operand into A:X
    void loadOperand(const ir::Operand& op);
    // Load only A from an operand (I8 only, no ldx #0)
    void loadOperandA(const ir::Operand& op);
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

    // Track vregs defined by CONST instructions (for direct-address store optimization)
    // Phase 1: Keep for backward compatibility; paralleled by MachineState tracking
    std::map<uint32_t, int64_t> vregConstVal_;
    // CONST vregs only used as STORE addresses — skip emission and frame allocation
    std::set<uint32_t> suppressedVregs_;

    // Phase 1: MachineState-based helpers for constant queries
    // Returns true if a vreg's value is a known constant and optionally retrieves it
    bool vregIsConst(uint32_t vregId, int64_t* outVal = nullptr) const;
    // Track which register holds a vreg's value (if it's a constant or in a register)
    RegId findVregInRegister(uint32_t vregId) const;
    // Update MachineState when a constant is loaded into a register
    void updateMachineStateForLoad(uint32_t vregId, RegId destReg);

    // Phase 2: MachineState-based helpers for memory queries
    // Returns true if a ZP location holds a known constant
    bool zpIsConst(uint8_t addr, int64_t* outVal = nullptr) const;
    // Returns true if a stack offset holds a known constant
    bool stackIsConst(uint8_t offset, int64_t* outVal = nullptr) const;
    // Returns true if an absolute address holds a known constant
    bool absMemIsConst(uint16_t addr, int64_t* outVal = nullptr) const;
    // Update MachineState after loading from a ZP location
    void updateZPFromLoad(uint8_t addr, RegId destReg);
    // Update MachineState after loading from a stack offset
    void updateStackFromLoad(uint8_t offset, RegId destReg);
    // Check if a register already holds a specific ZP value
    bool regHoldsZPValue(RegId r, uint8_t zpAddr) const;

    // Phase 3: MachineState-based helpers for range queries
    // Returns true if a vreg is known to be within [lo..hi]
    bool vregInRange(uint32_t vregId, int64_t lo, int64_t hi) const;
    // Returns true if a ZP location is known to be within [lo..hi]
    bool zpInRange(uint8_t addr, int64_t lo, int64_t hi) const;
    // Check if a comparison instruction is redundant given a known value
    // outAlwaysTrue: set to true if condition is always true, false if always false
    bool compareCanBeEliminated(const ir::Inst& cmp, int64_t val, bool& outAlwaysTrue) const;
    // Update range after CONST emission (exact constant = range [val,val])
    void updateRangeFromConstant(uint32_t vregId, int64_t val);
    // Update range from detected loop bounds
    void updateRangeFromLoop(uint32_t vregId, int64_t lo, int64_t hi);

    // Phase 4: Register capability tracking for smart register selection
    // Query if a register can perform a specific operation
    bool registerCanDoALU(RegId r) const;
    // Query if a register can be incremented/decremented
    bool registerCanIncrement(RegId r) const;
    // Query if a register can do shift operations
    bool registerCanShift(RegId r) const;
    // Get priority score for register (higher = prefer when equal cost)
    int getRegisterPriority(RegId r) const;

    // Phase 5: Memory operation optimization helpers
    // Check if a memory location can be directly incremented (inc $addr instead of load/inc/store)
    bool memLocationCanDirectIncrement(uint32_t vregId) const;
    // Check if a memory location can be directly shifted (asl $addr instead of load/asl/store)
    bool memLocationCanDirectShift(uint32_t vregId) const;

    // Phase 5a: Smart register selection and memory operation infrastructure
    // Select best register to load a value based on next operation's requirements
    RegId selectLoadDestinationReg(uint32_t vregId, ir::Type type);
    // Find best register given next operation type (ALU/shift/inc/etc)
    RegId findBestRegisterForLoad(uint32_t vregId, ir::Type type, const ir::Inst* nextOp);
    // Check if direct inc/dec is safe and beneficial
    bool canUseDirectMemIncrement(uint32_t vregId) const;
    // Check if direct shift is safe and beneficial
    bool canUseDirectMemShift(uint32_t vregId, ir::Op shiftOp) const;
    // Cost-benefit: should we prefer direct memory operation?
    bool shouldPreferMemoryOp(uint32_t vregId, ir::Op opType) const;
    // Check if a register can perform a given operation
    bool canRegPerformOp(RegId r, ir::Op op) const;
    // Get register selection priority (lower = prefer)
    int getRegisterSelectPriority(RegId r) const;
    // Emit direct memory increment: inc/dec at vReg location
    void emitDirectMemIncrement(uint32_t vregId, ir::Op opType);
    // Emit direct memory shift: asl/lsr/etc at vReg location
    void emitDirectMemShift(uint32_t vregId, ir::Op shiftOp);
    // Update MachineState after direct memory operation
    void updateMachineStateAfterMemOp(uint32_t vregId);
    // Record which register holds a vreg's value (for store forwarding)
    void recordVregInRegister(uint32_t vregId, RegId reg);
    // Clear register tracking when clobbered
    void clearRegTracking(RegId reg);

    // Source location tracking for .loc directives
    int lastLocLine_ = -1;
    std::string lastLocFile_;
    std::string sourceFile_; // module source file for function declaration .loc
    std::map<int, std::pair<std::string, int>> lineToFileMap_; // Maps abs line to (filename, lineOffset)
    const std::map<std::string, const ir::Function*>* functionMap_ = nullptr;
    MachineState ms_;        // register/flag value tracking for codegen optimizations

    // Value-role tracking: which register holds each byte of the current
    // multi-byte value being constructed. Set by CONST/load/ALU emitters,
    // read by storeVreg to pick the right frame store op (stax.fp vs staz.fp).
    static constexpr int8_t VB_NONE = -1;
    int8_t valueByte_[4] = { VB_NONE, VB_NONE, VB_NONE, VB_NONE };
    void clearValueRoles() { valueByte_[0] = valueByte_[1] = valueByte_[2] = valueByte_[3] = VB_NONE; }

    // Store-forwarding: track which vreg's result is currently live in A:X
    // from the most recent instruction. -1 = unknown/not in AX.
    int32_t resultInAX_ = -1;

    // Phase 5a: Track which vreg's value each register holds (for store forwarding)
    // regHoldsVreg_[r] = vregId (-1 if unknown/multiple/clobbered)
    // Indexed by RegId: 0=A, 1=X, 2=Y, 3=Z, 4=SP
    int32_t regHoldsVreg_[5] = {-1, -1, -1, -1, -1};

    // Next block label for no-op branch elimination
    std::string nextBlockLabel_;

    // Track last CMP instruction for fused compare-and-branch
    struct LastCmp {
        ir::Op op = ir::Op::NOP;
        uint32_t destVreg = 0;
        bool valid = false;
        // For BBS/BBR single-bit branch optimization
        std::string bbsZpAddr;  // non-empty = use BBS/BBR instead of flags
        int bbsBitN = -1;       // bit number (0-7)
    } lastCmp_;

    // Current function, block, and instruction index for peephole lookahead
    const ir::Function* currentFn_ = nullptr;
    size_t currentBlockIdx_ = 0;
    size_t currentInstInBlock_ = 0; // index within current block's inst vector

    // Peek at the next instruction in the current block (or null if at end)
    const ir::Inst* peekNextInst() const;

    // Per-function clobber analysis
    struct FuncClobbers {
        uint8_t regs = 0;   // bit 0=A, 1=X, 2=Y, 3=Z
        uint8_t flags = 0;  // bit 0=C, 1=N, 2=Z, 3=V
        bool isLeaf = true;
    };
    FuncClobbers computeFuncClobbers(const ir::Function& fn);
};
