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

    // Debug metadata emission
    std::string formatDebugType(ir::Type type);
    void emitDebugVariable(const std::string& functionName, const std::string& varName,
                          int offset, ir::Type type, const std::string& scope);

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

    // Emit inline address calculation: result = base + index*stride
    // Result is returned in A:X and optionally stored to destZP (if non-empty)
    // baseStr can be "#symbol" (immediate) or memory operand
    // indexOp is the array index (can be immediate, operand, or vReg)
    // destZP is optional destination ZP address (e.g., "__zp_scratch"); if empty, result stays in A:X
    void emitArrayElemAddr(const std::string& baseStr, const ir::Operand& indexOp, int stride,
                           const std::string& destZP = "");

    // Pre-scan a function to allocate frame slots for all vRegs
    void prescanFunction(const ir::Function& fn);

    // String pool
    std::map<std::string, std::string> stringPool_;
    int stringCount_ = 0;

    // Current function metadata
    bool relocMode_ = false;
    bool zpCallMode_ = false;
    bool useStackParams_ = false;  // frame pointer setup required for current function

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
    std::map<uint32_t, int64_t> vregConstVal_;
    // CONST vregs only used as STORE addresses — skip emission and frame allocation
    std::set<uint32_t> suppressedVregs_;

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

    // Phase 2: Fine-grained register invalidation tracking
    // Maps function name → clobber mask for selective invalidation at call sites
    std::map<std::string, int> functionClobberMasks_;
};
