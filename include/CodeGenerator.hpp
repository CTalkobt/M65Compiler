#pragma once
#include "AST.hpp"
#include "M65Emitter.hpp"
#include <iostream>
#include <ostream>
#include <vector>
#include <deque>
#include <string>
#include <map>
#include <set>
#include <memory>

class CodeGenerator : public ASTVisitor {
public:
    enum class TriState { UNKNOWN, SET, CLEAR };
    enum class FlagSource { NONE, A, X, Y, Z };

    CodeGenerator(std::ostream& out);
    void generate(TranslationUnit& unit);
    void setSourceInfo(const std::string& filename, const std::vector<std::string>& lines);

    // Set the line-to-file mapping from Lexer for proper source file attribution
    void setLineToFileMap(const std::map<int, std::pair<std::string, int>>& map) {
        lineToFileMap = map;
    }

    struct VarInfo {
        std::string type;
        int pointerLevel;
        bool isSigned = false;
        bool isVolatile = false;
        bool isConst = false;         // base type is const (prevents *p = x)
        bool isPointerConst = false;  // pointer itself is const (prevents p = x)
        bool isRegister = false;      // allocated in zero page
        bool isStriped = false;       // Phase 92: Striped array optimization
        int elementSize = 0;          // Phase 94: For striped struct arrays (0 = not striped or int)
        bool isFieldStriped = false;  // Phase 95: Field-level striping within striped struct arrays
        std::vector<std::string> fieldNames;  // Phase 95: Names of struct fields (if field-striped)
        std::vector<int> fieldSizes;  // Phase 95: Sizes of struct fields in bytes
        std::vector<int> fieldOffsets; // Phase 95: Offsets of field regions in memory

        // Phase 96: Union support
        bool isUnionStriped = false;        // Union variant of striped array
        std::vector<std::string> unionFields;  // All union field names
        std::vector<int> unionFieldSizes;  // Size of each union field
        int largestUnionFieldSize = 0;     // Largest field size (memory footprint)

        std::vector<int> arrayDims;   // empty = not array; {3,4} = int[3][4]
        bool isFunctionPointer = false;
        std::shared_ptr<FuncPtrSignature> funcPtrSig;

        VarInfo() = default;
        VarInfo(const std::string& t, int p, bool s = false, bool v = false, bool c = false,
                bool pc = false, bool r = false, const std::vector<int>& a = {},
                bool fp = false, std::shared_ptr<FuncPtrSignature> fpSig = nullptr, int es = 0)
            : type(t), pointerLevel(p), isSigned(s), isVolatile(v), isConst(c),
              isPointerConst(pc), isRegister(r), arrayDims(a), isFunctionPointer(fp),
              funcPtrSig(fpSig), isStriped(false), elementSize(es), isFieldStriped(false) {}

        int arraySize() const { if (arrayDims.empty()) return -1; int s=1; for (int d:arrayDims) s*=d; return s; }
    };
    struct ExpressionType {
        std::string type;
        int pointerLevel;
        bool isSigned = false;
        bool isConst = false;
        bool isPointerConst = false;
        bool isFunctionPointer = false;
        std::shared_ptr<FuncPtrSignature> funcPtrSig;
    };
    struct MemberInfo {
        std::string type;
        int pointerLevel;
        bool isSigned = false;
        bool isConst = false;
        int offset;
        int alignment = 1;
        std::vector<int> arrayDims;
        int arraySize() const { if (arrayDims.empty()) return -1; int s=1; for (int d:arrayDims) s*=d; return s; }
        int bitWidth = 0;   // 0 = not a bitfield; >0 = bitfield width in bits
        int bitOffset = 0;  // bit offset within the storage unit
    };
    struct StructInfo {
        std::string name;
        std::map<std::string, MemberInfo> members;
        int totalSize;
        int alignment = 1;
        bool isFieldStriped = false;  // Phase 95: Whether this struct supports field-level striping
        bool isUnion = false;         // Phase 96: True if this is a union (overlay) rather than struct
        int largestFieldSize = 0;     // Phase 96: For unions, the largest field size (memory footprint)
    };
    std::map<std::string, VarInfo> variableTypes;
    std::map<std::string, VarInfo> globalVariableTypes;
    std::map<std::string, std::shared_ptr<StructInfo>> structs;
    static int getTypeSize(const std::string& type, int ptrLevel, int arraySize, const std::map<std::string, std::shared_ptr<CodeGenerator::StructInfo>>& structs);
    static bool is8BitType(const std::string& type) { return type == "char" || type == "_Bool"; }
    static bool is32BitType(const std::string& type) { return type == "long"; }
    uint32_t zeroPageStart = 0x02;
    uint32_t zeroPageAvail = 9;
    bool relocMode = false; // When true, emit .global/.extern and skip .org/$2000 stub
    bool weakNext = false;  // When true, next function/global emits .weak instead of .global
    bool zpCallMode = false; // When true, use ZP parameter block calling convention (-fzpcall)

    void visit(IntegerLiteral& node) override;
    void visit(FloatLiteral& node) override;
    void visit(StringLiteral& node) override;
    void visit(VariableReference& node) override;
    void visit(Assignment& node) override;
    void visit(BinaryOperation& node) override;
    void visit(UnaryOperation& node) override;
    void visit(ConditionalExpression& node) override;
    void visit(GenericSelection& node) override;
    void visit(InitializerList& node) override;
    void visit(ArrayAccess& node) override;
    void visit(FunctionCall& node) override;
    void visit(MemberAccess& node) override;
    void visit(CastExpression& node) override;
    void visit(CompoundLiteral& node) override;
    void visit(AlignofExpression& node) override;
    void visit(SizeofExpression& node) override;
    void visit(VariableDeclaration& node) override;
    void visit(ReturnStatement& node) override;
    void visit(BreakStatement& node) override;
    void visit(ContinueStatement& node) override;
    void visit(SwitchContinueStatement& node) override;
    void visit(GotoStatement& node) override;
    void visit(LabelledStatement& node) override;
    void visit(ExpressionStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(DoWhileStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(RepeatStatement& node) override;
    void visit(SwitchStatement& node) override;
    void visit(CaseStatement& node) override;
    void visit(DefaultStatement& node) override;
    void visit(AsmStatement& node) override;
    void visit(StaticAssert& node) override;
    void visit(EnumDefinition& node) override;
    void visit(StructDefinition& node) override;
    void visit(CompoundStatement& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(BuiltinVaStart& node) override;
    void visit(BuiltinVaArg& node) override;
    void visit(CpuRegisterAccess& node) override;
    void visit(CpuFlagAccess& node) override;
    void visit(LabelAddressExpression& node) override;
    void visit(TranslationUnit& node) override;
    void emitAddress(Expression* expr);
    void emitIndirectIncDec(UnaryOperation& node, bool isInc, bool isPost);
    void emitStripedArrayAccess(ArrayAccess& node, VarInfo& varInfo, VariableReference* baseRef);
    bool tryEmitFieldStripedArrayMemberAccess(ArrayAccess& node, VarInfo& varInfo, VariableReference* baseRef, const std::string& memberName, const MemberInfo& mInfo);  // Phase 95.3: Field-level striping
    bool tryEmitUnionStripedArrayMemberAccess(ArrayAccess& node, VarInfo& varInfo, VariableReference* baseRef, const std::string& memberName);  // Phase 96.1: Union-striped array member access
    std::vector<int> reorganizeUnionStripedArrayData(const std::vector<int>& userData, int elementCount, int largestFieldSize, const std::vector<std::string>& fieldNames);  // Phase 96.1: Union data reorganization

    // Phase 92.4: 2D array reorganization (backward compat)
    std::vector<int> reorganizeStripedArrayData(const std::vector<int>& userData, int height, int width);

    // Phase 93: Multi-dimensional array reorganization (3D+)
    std::vector<int> reorganizeStripedArrayData(const std::vector<int>& userData, const std::vector<int>& dims);
    // Phase 95.4: Field-striped struct array data reorganization
    std::vector<int> reorganizeFieldStripedArrayData(const std::vector<int>& userData, int structSize, const std::vector<int>& fieldSizes, const std::vector<int>& dims);
    void emitOperation(const std::string& op, int zpLeft, ExpressionType lhsType, ExpressionType rhsType);
    void embedSource(ASTNode& node);
    ExpressionType getExprType(Expression* expr);
    void emitNarrowingWarning(ASTNode& node, const std::string& fromType, int fromPtr, const std::string& toType, int toPtr);
    void emitBoolNormalize(int srcSize);
    std::string formatDebugType(const std::string& type, int pointerLevel, const std::vector<int>& arrayDims);
    void emitDebugVariable(const std::string& functionName, const std::string& varName, uint32_t offset,
                           const std::string& type, int pointerLevel, const std::string& scope,
                           const std::vector<int>& arrayDims, int srcLine = -1, const std::string& displayName = "");
    std::vector<std::string> warnings;
    bool isStruct(const std::string& type);
    bool isEnum(const std::string& type);
    std::string resolveVarName(const std::string& name);
    std::string getAggregateName(const std::string& type);
    VarInfo lookupVar(const std::string& rName, ASTNode* node = nullptr);
    static bool matchType(const ExpressionType& t1, const std::string& t2Name, int t2Ptr);

    // Per-function clobber tracking (Phase 1 of fine-grained invalidation)
    static constexpr uint8_t CLOBBER_A = 0x01;
    static constexpr uint8_t CLOBBER_X = 0x02;
    static constexpr uint8_t CLOBBER_Y = 0x04;
    static constexpr uint8_t CLOBBER_Z = 0x08;
    static constexpr uint8_t CLOBBER_C = 0x01;  // carry flag bit
    static constexpr uint8_t CLOBBER_N = 0x02;  // negative flag bit
    static constexpr uint8_t CLOBBER_ZF = 0x04; // zero flag bit
    static constexpr uint8_t CLOBBER_V = 0x08;  // overflow flag bit

    struct FuncClobberInfo {
        uint8_t regMask = 0;    // CLOBBER_A/X/Y/Z
        uint8_t flagMask = 0;   // CLOBBER_C/N/ZF/V
        bool isLeaf = true;     // no calls to other functions
        bool complete = false;  // true once function body fully visited
    };
    std::map<std::string, FuncClobberInfo> funcClobbers_;
    FuncClobberInfo* currentClobbers_ = nullptr;

    struct RegisterVarInfo {
        int zpIndex;
        int size;
    };
    struct ZpParamInfo {
        uint8_t zpAddr;   // absolute ZP address (e.g., $03)
        int size;          // 1, 2, or 4 bytes
    };
    struct ZpSpillInfo {
        int frameOffset;  // offset within frame (for .local / sta.fp / leax.fp)
        int size;         // 1, 2, or 4 bytes
    };

    struct FunctionScope {
        FunctionDeclaration* func;
        std::map<std::string, VarInfo> variableTypes;
        std::map<std::string, RegisterVarInfo> registerVars;
        std::map<std::string, int> frameLocals;
        int frameSize;
        bool useZpCall;
        std::map<std::string, ZpParamInfo> zpParams;
        std::map<std::string, ZpSpillInfo> zpSpilledParams;
    };
    std::deque<FunctionScope> functionStack_;

    void clobberReg(uint8_t mask);
    void clobberFlag(uint8_t mask);
    void invalidateFromClobbers(uint8_t regMask, uint8_t flagMask);

    // Register tracking
    struct RegState {
        bool known = false;
        bool isVariable = false;
        std::string varName;
        int varOffset = 0;
        uint8_t value = 0;
    };
    RegState regA, regX, regY, regZ;
    void updateRegA(uint8_t val);
    void updateRegX(uint8_t val);
    void updateRegY(uint8_t val);
    void updateRegZ(uint8_t val);
    void updateRegAVar(const std::string& name, int offset);
    void updateRegXVar(const std::string& name, int offset);
    void updateRegYVar(const std::string& name, int offset);
    void updateRegZVar(const std::string& name, int offset);
    void transferRegs(FlagSource dest, FlagSource src);
    void invalidateVar(const std::string& name);
    void invalidateRegs();

    // Flag tracking
    struct ProcessorStatus {
        TriState carry = TriState::UNKNOWN;
        TriState zero = TriState::UNKNOWN;
        TriState negative = TriState::UNKNOWN;
        TriState overflow = TriState::UNKNOWN;
        FlagSource znSource = FlagSource::NONE;
    };
    ProcessorStatus flags;
    void updateFlags(TriState c, TriState z, TriState n, TriState v = TriState::UNKNOWN);
    void updateZNFlags(FlagSource source, TriState z = TriState::UNKNOWN, TriState n = TriState::UNKNOWN);
    void invalidateFlags();

    private:
    std::ostream& out;
    std::unique_ptr<M65Emitter> emitter;
    int stringCount = 0;
    int labelCount = 0;
    std::map<std::string, std::string> stringPool;
    std::set<std::string> asciiStrings;  // pool keys for @"..." strings
    enum class StringEncoding { PETSCII, ASCII, SCREENCODE };
    StringEncoding currentStringEncoding_ = StringEncoding::PETSCII;
    std::set<std::string> screencodeStrings;  // pool keys for screencode strings
    std::vector<std::string> currentVars;
    std::string sourceFilename;
    std::vector<std::string> sourceLines;
    std::map<int, std::pair<std::string, int>> lineToFileMap;  // Maps abs line to (filename, line_offset)
    int lastEmbeddedLine = -1;
    bool resultNeeded = true;

    struct LoopLabels {
        std::string continueLabel;
        std::string breakLabel;
    };
    std::vector<LoopLabels> loopStack;

    struct SwitchInfo {
        int zpExpr; // Zero page register holding the switch expression value
        std::string breakLabel;
        std::string defaultLabel;
        struct Case {
            uint32_t value;
            uint32_t rangeEndValue; // == value for single cases
            bool isRange;
            std::string label;
        };
        std::vector<Case> cases;
        bool hasDefault = false;
    };
    std::vector<SwitchInfo*> switchStack;

    void emit(const std::string& line);
    void emitData();
    void emitBranch16Beq(const std::string& target);
    void emitBranch16Bne(const std::string& target);
    void emitJumpIfTrue(Expression* cond, const std::string& labelTrue);
    void emitJumpIfFalse(Expression* cond, const std::string& labelElse);
    std::string newLabel();
    std::string newDontCareLabel();

    struct LoopUnrollInfo {
        std::string counterVar;
        int64_t startVal = 0;
        int64_t endVal = 0;
        int64_t stepVal = 1;
        bool isUnrollable = false;
        int unrollCount = 0;
    };
    LoopUnrollInfo analyzeForUnrolling(ForStatement* node);
    void emitUnrolledLoop(ForStatement& node, const LoopUnrollInfo& info);
    bool tryEmitAddressTemplate(BinaryOperation& node);  // Phase 89: Address template optimization

    int allocateZP(int size);
    void freeZP(int index, int size);

    struct ZPReg {
        bool inUse = false;
    };
    std::vector<ZPReg> zpRegs;
    std::map<std::string, RegisterVarInfo> registerVars; // resolved name → ZP allocation
    void freeRegisterVars();

    std::vector<VariableDeclaration*> globalVars;
    std::set<std::string> weakGlobals; // global vars marked with #pragma weak
    std::set<std::string> staticGlobals; // global vars/funcs with static linkage
    std::set<std::string> staticFunctions; // functions with static linkage
    bool crtNoPageOneStack = false; // #pragma cc45 no_0100_stack
    enum class CrtExit { HALT, RTS, BRK } crtExit = CrtExit::RTS;
    bool crtNoBssInit = false; // #pragma cc45 no_bssinit
    bool crtHeap = false;      // #pragma cc45 heap
    bool crtStdio = false;     // #pragma cc45 stdio
    int loopUnrollDefault = 0; // #pragma cc45 unroll N (0 = disabled, 3-8 typical)
    int loopUnrollNext = 0;    // #pragma cc45 unroll (applies to next loop only, one-shot)
    std::set<std::string> knownFunctions; // defined + prototyped function names
    std::set<std::string> variadicFunctions; // functions declared with ...
    std::set<std::string> fastcallFunctions; // functions declared with __fastcall__
    std::set<std::string> structReturningFunctions; // functions that return a struct by value
    struct FuncReturnInfo { std::string type; int pointerLevel; bool isSigned; };
    std::map<std::string, FuncReturnInfo> functionReturnTypes; // function name → return type
    std::map<std::string, std::vector<VarInfo>> functionParamTypes; // parameter types per function
    FunctionDeclaration* currentFunction = nullptr;
    int currentParamByteSize = 0;
    int currentLocalByteSize = 0;

    // Frame layout from pre-scan (local name → frame offset)
    std::map<std::string, int> frameLocals_;
    int frameSize_ = 0;
    int structRetDest_ = -1; // frame offset for struct return destination (-1 = none)
    int compoundLiteralCount_ = 0; // unique ID counter for compound literal temporaries
    bool isVariableUsed(const std::string& varName, FunctionDeclaration& func);
    std::string getLocalOffsetSymbol(int offset); // Convert frame offset to symbolic name (e.g., _l_x or _l_x+1)

    // ZP calling convention (zpCallMode)
    std::map<std::string, ZpParamInfo> zpParams_; // _p_name → ZP location (current function)
    int zpParamTotalBytes_ = 0;   // total param bytes in ZP block
    bool useZpCall_ = false; // true when current function uses ZP calling (zpCallMode or __fastcall__)
    bool isZpParam(const std::string& rName) const { return useZpCall_ && zpParams_.count(rName) > 0; }
    std::string zpHex(uint8_t addr) const;  // format as "$XX"
    int zpCallerSaveSize_ = 0;  // frame bytes reserved for caller-save of ZP params
    // Params whose address is taken — spilled from ZP to frame
    std::map<std::string, ZpSpillInfo> zpSpilledParams_; // _p_name → frame location
    bool isZpSpilledParam(const std::string& rName) const { return useZpCall_ && zpSpilledParams_.count(rName) > 0; }
};
