#pragma once
#include "IR.hpp"
#include "VRegAllocator.hpp"
#include "MachineState.hpp"
#include "O45IRSerializer.hpp"
#include <ostream>
#include <map>
#include <string>
#include <vector>

class IRCodeGen {
public:
    IRCodeGen(std::ostream& out);

    // Generate assembly for the entire module.
    // relocMode: true = .o45 (emit .global/.extern, no startup stub)
    //            false = PRG (emit .org + startup stub)
    // prgBase: load address for PRG mode (default 0x2000), ignored if relocMode=true
    // sacDebugMode: true = emit runtime debug output for SAC AR buffers (function entry/exit, AR addr, param values)
    void generate(const ir::Module& mod, uint32_t zpStart = 0x08, bool relocMode = false, bool zpCallMode = false, bool emitReasons = false, bool staticAllocMode = false, bool sacDebugMode = false, uint32_t prgBase = 0x2000);

    // Set the line-to-file mapping from Lexer for proper source file attribution in .loc directives
    void setLineToFileMap(const std::map<int, std::pair<std::string, int>>& map) {
        lineToFileMap_ = map;
    }
    // cppcheck-suppress danglingLifetime
    void setFunctionMap(const std::map<std::string, const ir::Function*>* map) {
        if (map) functionMap_ = *map;
        else functionMap_.clear();
    }

    // Phase 49: Get collected IR metadata for output
    O45IRMetadata getIRMetadata() const;

    // Phase 51: Set specialized (constant) parameters for functions
    // Maps function name → parameter index → constant value
    // Used during code generation to skip parameter loading for constant parameters
    void setSpecializedParams(const std::map<std::string, std::map<int, int64_t>>& params) {
        specializedParams_ = params;
    }

    // Phase 51: Check if a function is a zero-alloc leaf
    // Returns true if ALL parameters are constant (no AR needed)
    bool isZeroAllocLeaf(const std::string& funcName, const ir::Function& fn) const;

    // Phase 51: Get constant value for a parameter (if it's specialized)
    bool getParameterConstant(const std::string& funcName, int paramIdx, int64_t& outValue) const {
        auto it = specializedParams_.find(funcName);
        if (it == specializedParams_.end()) return false;
        auto pit = it->second.find(paramIdx);
        if (pit == it->second.end()) return false;
        outValue = pit->second;
        return true;
    }

    // Phase 53: Set specialization info for function version generation
    // Maps function name → {pattern → specialization info}
    void setSpecializationAnalysis(const std::map<std::string, SpecializationAnalysis>& analysis) {
        specializationAnalysis_ = analysis;
    }

    // Phase 53: Generate specialization name from pattern
    // E.g., _calculate + {10, 2} → _calculate_10_2
    std::string generateSpecializationName(const std::string& funcName, const SpecializationPattern& pattern) const;

    // Phase 91.3: Set functions to skip during code generation (dead code elimination)
    void setDeadCodeFunctions(const std::set<std::string>& deadFuncs) {
        deadCodeFunctions_ = deadFuncs;
    }

private:
    std::ostream& out_;
    uint32_t zeroPageStart_ = 0x08;

    // Assembly emission helpers
    void emit(const std::string& line, const std::string& reason = "");
    void emitLabel(const std::string& label);
    void emitComment(const std::string& text);
    void emitBlank();
    void emitStackCleanup(int frameSize);

    // Debug metadata emission
    std::string formatDebugType(ir::Type type);
    void emitDebugVariable(const std::string& functionName, const std::string& varName,
                          int offset, ir::Type type, const std::string& scope);

    // SAC debug output emission (runtime AR buffer debugging)
    void emitSACDebugEnter(const std::string& funcName, int arSize);
    void emitSACDebugExit(const std::string& funcName);

    // Codegen reasoning trace (enabled by -Rcodegen)
    bool emitReasons_ = false;

    // Module-level emission
    void emitStartupStub(const ir::Module& mod);
    void emitGlobals(const ir::Module& mod, bool relocMode);
    void emitStrings(const ir::Module& mod);
    void emitFunction(const ir::Function& fn, bool relocMode, bool isMainWithZPSave = false);
    void emitInst(const ir::Inst& inst);

    // Check if frame-relative addressing is used after the current instruction
    bool frameAddrUsedAfterCall() const;

    // vReg frame slot management (per-function)
    std::map<uint32_t, int> vregOffset_;  // vregId → frame offset
    std::map<uint32_t, ir::Type> vregType_; // vregId → type
    int frameSize_ = 0;
    int labelCounter_ = 0;

    void resetFrame();

    // ZP allocator for frame address tracking
    struct ZPReg { bool inUse = false; };
    std::vector<ZPReg> zpRegs_;
    uint32_t zeroPageAvail_ = 248;  // Available ZP space (usually $08-$FF)
    int frameAddrZPIndex_ = -1;  // allocated ZP index for frame address (2 bytes), -1 if not allocated
    bool frameAddrCacheValid_ = true;  // Frame pointer cache valid flag (invalidated after function calls)

    int allocateZP(int size);
    void freeZP(int index, int size);
    std::string zpAddr(int index) const;  // Return ZP address string like "$08"
    void loadFrameAddr(int offset = 0);  // Load frame address into A:X, using cache if available

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


    // Bug #3 fix: Allocate register variables to zero page
    // Called during emitFunction to ensure register variables get ZP allocation
    // even though prescanFunction is skipped (to avoid Bug #179 frame conflicts)
    void allocateRegisterVariablesZP(const ir::Function& fn);

    // String pool
    std::map<std::string, std::string> stringPool_;
    int stringCount_ = 0;

    // Current function metadata
    std::string currentFunctionName_;  // name of function being emitted
    bool currentFunctionUseSAC_ = false;  // whether current function uses SAC
    bool relocMode_ = false;
    bool zpCallMode_ = false;
    bool staticAllocMode_ = false;  // -fstaticalloc (SAC)
    bool sacDebugMode_ = false;  // -fsac-debug (emit runtime debug output for SAC)
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

    // Track which vRegs are function parameters (for SAC parameter addressing)
    // Parameters always use FP-relative addressing, never AR-relative
    std::set<uint32_t> parameterVregs_;

    // Track vregs defined by CONST instructions (for direct-address store optimization)
    // Phase 1: Keep for backward compatibility; paralleled by MachineState tracking
    std::map<uint32_t, int64_t> vregConstVal_;
    // CONST vregs only used as STORE addresses — skip emission and frame allocation
    std::set<uint32_t> suppressedVregs_;

    // Track which functions use SAC (Static Allocation Convention)
    // Used to emit .global declarations for __ar symbols in preamble
    std::set<std::string> sacFunctions_;

    // Track parameter names for each function (for SAC naming consistency)
    // Maps function name → vector of parameter names
    std::map<std::string, std::vector<std::string>> functionParameterNames_;

    // SAC constant parameter optimization: track which parameters always receive the same constant
    // Maps function_name → parameter_index → constant_value (optional)
    // If a parameter has an entry, all call sites pass that constant; -1 means no constant
    struct ConstParamInfo {
        bool isConstant = false;
        int64_t value = 0;
    };
    std::map<std::string, std::map<int, ConstParamInfo>> sacConstParams_;

    // SAC leaf function optimization: functions that don't call any other functions
    // Leaf functions can use simpler SAC code (no need for recursive AR setup)
    // Maps function_name → is_leaf (true if function makes no CALL/CALL_VOID instructions)
    std::set<std::string> leafFunctions_;

    // SAC zero-alloc leaf functions: leaf functions with only constant params and no locals
    // These skip AR allocation entirely for maximum code reduction (4-12 bytes per function)
    // Maps function_name → has_no_alloc_needed
    std::set<std::string> zeroAllocLeaves_;

    // Phase 91.3: Functions to skip during code generation (dead code elimination via IPOAnalyzer)
    // During generate(), emitFunction() skips functions in this set
    std::set<std::string> deadCodeFunctions_;

    // Analyze function calls to detect constant parameters (pre-pass before code generation)
    void analyzeConstantParameters(const ir::Module& mod);

    // Detect which functions are leaves (don't call other functions)
    void detectLeafFunctions(const ir::Module& mod);

    // Detect zero-alloc leaves (leaf + no locals + all constant params)
    void detectZeroAllocLeaves(const ir::Function& fn);

    // Phase 78: SMC (Self-Modifying Code) parameter optimization
    // Track parameter access patterns to embed parameters in instruction immediates
    std::map<std::string, std::map<int, O45SACParam>> functionSMCMetadata_;  // funcName → paramID → metadata
    uint32_t currentInstructionOffset_ = 0;  // Byte offset during code generation
    std::vector<std::string> currentFunctionParams_;  // Current function's parameter names
    bool trackSMCOffsetsEnabled_ = false;  // Enable SMC offset tracking for current function

    // Helper to record a parameter access for SMC analysis
    void recordParameterAccess(int paramID, uint32_t accessSize) {
        if (!trackSMCOffsetsEnabled_) return;
        if (paramID < 0 || paramID >= (int)currentFunctionParams_.size()) return;

        auto& metadata = functionSMCMetadata_[currentFunctionName_][paramID];
        metadata.accessCount++;
        metadata.accessOffsets.push_back(currentInstructionOffset_);
        if (accessSize > 0 && accessSize <= 4) {
            metadata.accessSizes.push_back((uint8_t)accessSize);
        }
    }

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
    std::map<std::string, const ir::Function*> functionMap_;
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

    // Phase 2: Fine-grained register invalidation tracking
    // Maps function name → clobber mask for selective invalidation at call sites
    std::map<std::string, int> functionClobberMasks_;

    // Phase 47: IR Metadata Tracking for .o45 Serialization
    // Track IR information for each function to enable cross-file optimizations
    bool emitIRMetadata_ = true;  // Enable IR emission by default

    // IR tracking: function name → O45IRFunction metadata
    std::map<std::string, O45IRFunction> irFunctionMap_;

    // Track current function being emitted (for call site tracking)
    std::string currentFunctionForIR_;
    uint32_t currentFunctionCallCount_ = 0;

    // Methods for IR collection and emission
    void initIRForFunction(const std::string& funcName, const ir::Function& fn);
    void recordCallSite(const std::string& calleeFunc, uint32_t instructionOffset,
                       const std::vector<ir::Operand>& args);
    void finalizeIRForFunction();

    // Phase 51: Specialized parameters for cross-file optimization
    // Maps function name → parameter index → constant value
    // Used to skip parameter initialization for constant parameters
    std::map<std::string, std::map<int, int64_t>> specializedParams_;

    // Phase 53: Specialization analysis from linker
    // Maps function name → specialization analysis (patterns, frequencies)
    std::map<std::string, SpecializationAnalysis> specializationAnalysis_;

    // Phase 53: Track current specialization context during code generation
    // Current function being emitted with specialization
    std::string currentSpecializationName_;
    SpecializationPattern currentSpecializationPattern_;
};
