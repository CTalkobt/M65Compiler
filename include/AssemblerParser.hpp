#pragma once
#include <vector>
#include <string>
#include <set>
#include <map>
#include <deque>
#include <memory>
#include <cstdint>
#include "AssemblerToken.hpp"
#include "AssemblerTypes.hpp"
#include "AssemblerOpcodeDatabase.hpp"

class M65Emitter;  // forward declaration for SimOpEmitFn

struct Instruction {
    std::string mnemonic;
    AddressingMode mode;
    std::string operand;
    int operandTokenIndex = -1;
    std::string bitBranchTarget;
    std::vector<std::string> callArgs;
    int procParamSize = 0;
    bool forceMode = false;
};

struct Directive {
    std::string name;
    std::vector<std::string> arguments;
    std::string varName;
    enum VarType { NONE, ASSIGN, INC, DEC } varType = NONE;
    int tokenIndex = -1;
};

class AssemblerParser {
public:
    friend class AssemblerOptimizer;
    friend class AssemblerSimulatedOps;
    friend class AssemblerGenerator;
    friend std::vector<uint8_t> emitO45(AssemblerParser&, const std::string&);
    AssemblerParser(const std::vector<AssemblerToken>& tokens);
    AssemblerParser(const std::vector<AssemblerToken>& tokens, const std::map<std::string, uint32_t>& predefinedSymbols);
    void pass1();
    bool optimize();
    bool verboseOptimizer = false;
    bool traceMachState = false;
    bool enableExperimental = false;
    std::vector<uint8_t> pass2(bool isPrg = false);
    uint32_t getZPStart() const;
    uint16_t getSpBase() const;
    uint8_t getScratchZP() const;
    uint32_t getPC() const { return pc; }
    uint32_t getFirstOrgAddress() const { return firstOrgAddress; }
    Symbol* resolveSymbol(const std::string& name, const std::string& scopePrefix = "");
    bool hasErrors() const { return !errors.empty(); }
    const std::vector<std::string>& getErrors() const { return errors; }
    void addError(const std::string& msg) { errors.push_back(msg); }
    void addWarning(const std::string& msg) { warnings.push_back(msg); }
    const std::vector<std::string>& getWarnings() const { return warnings; }

    // .global / .extern symbol tracking (for .o45 output)
    const std::set<std::string>& getGlobalSymbols() const { return globalSymbols; }
    const std::set<std::string>& getWeakSymbols() const { return weakSymbols; }
    bool isWeakSymbol(const std::string& name) const { return weakSymbols.count(name) > 0; }
    const std::vector<std::string>& getExternSymbols() const { return externSymbols; }
    bool isGlobalSymbol(const std::string& name) const { return globalSymbols.count(name) > 0; }
    bool isExternSymbol(const std::string& name) const { return externIndex.count(name) > 0; }
    // Returns true if the symbol's final address is not yet known (extern or in a non-org'd segment like BSS)
    bool isRelocatableSymbol(const std::string& name) const;
    uint32_t getExternIndex(const std::string& name) const;

    bool isPass1() const { return isPass1_; }
    bool areAddressesFinalized() const { return addressesFinalized_; }
    void finalizeAddresses() { addressesFinalized_ = true; }

    struct ProcContext {
        std::string name;
        int totalParamSize;
        std::map<std::string, int> localArgs;

        // Function attribute metadata (ZP calling convention)
        uint32_t zpUsesMask = 0;
        uint32_t zpClobbersMask = 0;
        uint32_t zpReleaseMask = 0;
        uint8_t regClobbersMask = 0;   // bit 0=A, 1=X, 2=Y, 3=Z
        uint8_t flagClobbersMask = 0;  // bit 0=C, 1=N, 2=Z, 3=V
        uint8_t funcFlags = 0;         // bit 0=leaf, bit 1=interrupt-safe
        bool hasFuncAttrs = false;     // true if any .zp_*/reg_*/flag_* directive was used
    };

    // Function attribute metadata (for .o45 export)
    const std::map<uint32_t, std::shared_ptr<ProcContext>>& getProcedures() const { return procedures; }

    // Array metadata (for expr array indexing)
    struct ArrayInfo {
        uint32_t elementSize;
        std::vector<uint32_t> dimensions;
        std::vector<uint32_t> strides;  // stride[i] = product(dimensions[i+1..]) * elementSize
    };
    const ArrayInfo* getArrayInfo(const std::string& name) const {
        auto it = arrayInfos.find(name);
        return it != arrayInfos.end() ? &it->second : nullptr;
    }

    // Segment info accessors (for .o45 output)
    struct SegmentView {
        std::string name;
        uint32_t startAddress;
        uint32_t endAddress; // startAddress + size
        uint32_t size;
    };
    std::vector<SegmentView> getSegmentViews() const;
    const std::map<std::string, Symbol>& getSymbolTable() const { return symbolTable; }

    // Source-level line map (for debug info)
    struct LineEntry {
        uint32_t address;
        std::string file;
        int line;
    };
    const std::vector<LineEntry>& getLineMap() const { return lineMap_; }

private:
    bool isPass1_ = true; // Flag to indicate if we are in pass 1 of assembly
    bool addressesFinalized_ = false; // Flag to prevent symbol address updates after pass2
    std::vector<AssemblerToken> tokens;
    size_t pos;
    uint32_t pc;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    uint32_t firstOrgAddress = 0xFFFFFFFF;
    std::map<std::string, Symbol> symbolTable;
    std::vector<std::string> scopeStack;
    int nextScopeId = 0;
    std::string currentLocalScope_; // last non-@ label for auto-scoping @ labels

    std::shared_ptr<ProcContext> currentProc;
    std::map<uint32_t, std::shared_ptr<ProcContext>> procedures;

    struct Segment {
        std::string name;
        uint32_t pc = 0;
        uint32_t startAddress = 0xFFFFFFFF;
        bool hasOrg = false;
    };
    std::map<std::string, std::shared_ptr<Segment>> segments;
    std::shared_ptr<Segment> currentSegment;
    std::vector<std::shared_ptr<Segment>> segmentOrder;
    std::vector<std::string> segmentStack;
    std::vector<std::string> requestedSegmentOrder;

    // .global / .extern / .weak tracking
    std::set<std::string> globalSymbols;         // symbols declared with .global
    std::set<std::string> weakSymbols;           // symbols declared with .weak
    std::vector<std::string> externSymbols;      // symbols declared with .extern (ordered)
    std::map<std::string, uint32_t> externIndex; // name -> index in externSymbols

    std::map<std::string, ArrayInfo> arrayInfos;

    // Source-level debug tracking
    std::string currentSourceFile_;
    int currentSourceLine_ = 0;
    std::vector<LineEntry> lineMap_;

public:
    struct Statement {
        enum Type { NONE, INSTRUCTION, DIRECTIVE, EXPR, BASIC_UPSTART, MUL, DIV, STACK_INC, STACK_DEC, STACK_INC8, STACK_DEC8,
                    ADD16, SUB16, AND16, ORA16, EOR16, CMP16, LDW, STW, SWAP, ZERO,
                    NEG16, NOT16, CHKZERO8, CHKZERO16, CHKNONZERO8, CHKNONZERO16, BRANCH16, SELECT,
                    PTRSTACK, PTRDEREF, LDWF, STWF, INCF, DECF, PHW_STACK, ASW, ROW, ASR16, LSL16, LSR16, ROL16, ROR16, ABS16,
                    ADDS16, SUBS16, CMP_S16, NEG_S16, ABS_S16, ASR_S16, LSL_S16, LSR_S16, ROL_S16, ROR_S16, SXT8,
                    LDA_STACK, STA_STACK,
                    LDX_STACK, LDY_STACK, LDZ_STACK,
                    STX_STACK, STY_STACK, STZ_STACK,
                    LDAX, LDAY, LDAZ, STAX, STAY, STAZ, FILL, COPY, PUSH, POP,
                    MUL_S16, MUL_S32, DIV_S16, DIV_S32, MOD16, MOD_S16, MOD32, MOD_S32,
                    LDA_FP, STA_FP, LDAX_FP, STAX_FP, LDAY_FP, STAY_FP, LDAZ_FP, STAZ_FP, LDAXYZ_FP, STAXYZ_FP, LEAX_FP, MOVE_FP, INC_FP, DEC_FP, INC16_FP, DEC16_FP,
                    BFEXT, BFEXT16, BFEXT32, BFINS, BFINS_SP, BFINS_IND, BFINS16, BFINS16_SP, BFINS16_IND, BFINS32,
                    ADD32, SUB32, AND32, ORA32, EOR32, CMP32, NEG32, NOT32, ABS32,
                    LSL32, LSR32, ROL32, ROR32, ASR32, SXT16,
                    ADDS32, SUBS32, CMP_S32, NEG_S32, ABS_S32,
                    ASR_S32, LSL_S32, LSR_S32, ROL_S32, ROR_S32,
                    STRUCT_ELEM, ADDR_ELEM_SIM } type = NONE;
        Instruction instr;
        Directive dir;
        std::string label;
        uint32_t address = 0;
        int size = 0;
        int line = 0;
        std::string scopePrefix;
        std::string localLabelScope; // auto-scope for @ label resolution
        std::string segmentName;
        std::shared_ptr<ProcContext> procCtx;

        // EXPR specific
        std::string exprTarget;
        int exprTokenIndex = -1;

        // BASIC_UPSTART specific
        int basicUpstartTokenIndex = -1;

        // MUL specific
        int mulWidth = 8;

        // Source-level debug info (set by .loc directive)
        std::string sourceFile;
        int sourceLine = 0;

        bool deleted = false;
        std::vector<uint8_t> bytes; // Captured machine code for listing

        // Simulated op dispatch: set at parse time, used for both sizing and emission
        using SimOpEmitFn = void(*)(AssemblerParser*, M65Emitter&, Statement*);
        SimOpEmitFn emitFn = nullptr;

        bool isSimulatedOp() const { return emitFn != nullptr; }

        bool is16BitOp() const {
            switch (type) {
                case ADD16: case SUB16: case AND16: case ORA16: case EOR16:
                case CMP16: case NEG16: case NOT16: case ABS16:
                case ADDS16: case SUBS16: case CMP_S16: case NEG_S16: case ABS_S16:
                case ASR16: case LSL16: case LSR16: case ROL16: case ROR16:
                case ASR_S16: case LSL_S16: case LSR_S16: case ROL_S16: case ROR_S16:
                case LDW: case STW: case LDAX: case LDAY: case LDAZ:
                case STAX: case STAY: case STAZ:
                    return true;
                default:
                    return false;
            }
        }
    };
    std::deque<std::unique_ptr<Statement>> statements;

private:
    void switchSegment(const std::string& name);

    const AssemblerToken& peek() const;
    const AssemblerToken& advance();
    bool match(AssemblerTokenType type);
    const AssemblerToken& expect(AssemblerTokenType type, const std::string& message);

    int calculateInstructionSize(const Instruction& instr, uint32_t currentAddr = 0, const std::string& scopePrefix = "");
    int calculateDirectiveSize(const Directive& dir, uint32_t currentAddr = 0);
    int calculateExprSize(int tokenIndex, const std::string& scopePrefix = "");
    uint32_t evaluateExpressionAt(int index, const std::string& scopePrefix = "");
    void emitExpressionCode(std::vector<uint8_t>& binary, const std::string& target, int tokenIndex, const std::string& scopePrefix = "");
    void emitMulCode(std::vector<uint8_t>& binary, int width, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitDivCode(std::vector<uint8_t>& binary, int width, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitStackIncDecCode(std::vector<uint8_t>& binary, bool isInc, int tokenIndex, const std::string& scopePrefix = "");
    void emitStackIncDec8Code(std::vector<uint8_t>& binary, bool isInc, int tokenIndex, const std::string& scopePrefix = "");
    void emitAddSub16Code(std::vector<uint8_t>& binary, bool isAdd, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitBitwise16Code(std::vector<uint8_t>& binary, const std::string& mnemonic, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitCMP16Code(std::vector<uint8_t>& binary, const std::string& src1, int tokenIndex, const std::string& scopePrefix = "");
    void emitCMP_S16Code(std::vector<uint8_t>& binary, const std::string& src1, int tokenIndex, const std::string& scopePrefix = "");
    void emitLDWCode(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "", bool forceStack = false);
    void emitSTWCode(std::vector<uint8_t>& binary, const std::string& src, int tokenIndex, const std::string& scopePrefix = "", bool forceStack = false);
    void emitLDA_StackCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitSTA_StackCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitLDX_StackCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitLDY_StackCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitLDZ_StackCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitSTX_StackCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitSTY_StackCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitSTZ_StackCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitSwapCode(std::vector<uint8_t>& binary, const std::string& r1, int tokenIndex, const std::string& scopePrefix = "");
    void emitZeroCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitNegNot16Code(std::vector<uint8_t>& binary, bool isNeg, const std::string& operand, int tokenIndex, const std::string& scopePrefix = "");
    void emitABS16Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitChkZeroCode(std::vector<uint8_t>& binary, bool is16, bool isInverse, int tokenIndex, const std::string& scopePrefix = "");
    void emitBranch16Code(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitSelectCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitPtrStackCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitPtrDerefCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitFillCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "", bool forceStack = false);
    void emitMoveCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "", bool forceStack = false);
    void emitFlatMemoryCode(std::vector<uint8_t>& binary, const std::string& mnemonic, int tokenIndex, const std::string& scopePrefix = "");
    void emitPHWStackCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitASWCode(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitROWCode(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitLSL16Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitLSR16Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitROL16Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitROR16Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitASR16Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitSXT8Code(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitSXT16Code(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitAddSub32Code(std::vector<uint8_t>& binary, bool isAdd, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitBitwise32Code(std::vector<uint8_t>& binary, const std::string& mnemonic, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitCMP32Code(std::vector<uint8_t>& binary, const std::string& src1, int tokenIndex, const std::string& scopePrefix = "");
    void emitCMP_S32Code(std::vector<uint8_t>& binary, const std::string& src1, int tokenIndex, const std::string& scopePrefix = "");
    void emitNegNot32Code(std::vector<uint8_t>& binary, bool isNeg, const std::string& operand, int tokenIndex, const std::string& scopePrefix = "");
    void emitABS32Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitLSL32Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitLSR32Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitROL32Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitROR32Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitASR32Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitPushPopCode(std::vector<uint8_t>& binary, bool isPush, int tokenIndex, const std::string& scopePrefix = "");
    void emitMulS16Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitMulS32Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitDivS16Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitDivS32Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitMod16Code(std::vector<uint8_t>& binary, bool isSigned, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    void emitMod32Code(std::vector<uint8_t>& binary, bool isSigned, const std::string& dest, int tokenIndex, const std::string& scopePrefix = "");
    bool isStackRelativeOperand(int tokenIndex, uint32_t& offset, const std::string& scopePrefix);

    // Frame-pointer pseudo-ops
    void emitLDA_FPCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitSTA_FPCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitLDAX_FPCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitSTAX_FPCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitLDAY_FPCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitSTAY_FPCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitLDAZ_FPCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitSTAZ_FPCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitLDAXYZ_FPCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitSTAXYZ_FPCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitLEAX_FPCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");
    void emitMOVE_FPCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix = "");

    // Bitfield pseudo-ops
    void emitBFExtCode(std::vector<uint8_t>& binary, bool is16, int tokenIndex, const std::string& scopePrefix = "");
    void emitBFInsCode(std::vector<uint8_t>& binary, bool is16, int mode, int tokenIndex, const std::string& scopePrefix = "");

    struct FrameAccessInfo {
        bool isFrame = false;
        uint8_t fpOff = 0;
        uint8_t yOff = 0;
    };
    FrameAccessInfo resolveFrameAccess(int tokenIndex, const std::string& scopePrefix);

    uint32_t moveDmaFirstCopyAddr_ = 0xFFFFFFFF;
    uint32_t moveDmaListAddr_      = 0xFFFFFFFF;
    uint32_t fillDmaFirstFillAddr_ = 0xFFFFFFFF;
    uint32_t fillDmaListAddr_      = 0xFFFFFFFF;
};
