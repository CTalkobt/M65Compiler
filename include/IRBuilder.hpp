#pragma once
#include "AST.hpp"
#include "ConstEval.hpp"
#include "IR.hpp"
#include "TypeSystem.hpp"
#include "SourceLocationTracker.hpp"
#include <map>
#include <functional>
#include <set>
#include <string>
#include <vector>
#include <deque>

// (the context of ConstEval, cc45's constant evaluator: sizes and alignments)
class IRBuilder : public ASTVisitor, public ConstEvalContext {
public:
    IRBuilder();

    void generate(TranslationUnit& unit);
    void setSourceInfo(const std::string& filename);
    void emitWarning(const std::string& warning);
    void setExternalUsedVars(const std::set<std::string>& vars) { externalUsedVars_ = vars; }
    const ir::Module& getModule() const { return module_; }
    ir::Module& getModule() { return module_; }
    SourceLocationTracker& getSourceTracker() { return sourceTracker_; }
    const SourceLocationTracker& getSourceTracker() const { return sourceTracker_; }
    bool hasErrors() const { return !errors_.empty(); }
    const std::vector<std::string>& getErrors() const { return errors_; }
    const std::vector<std::string>& getWarnings() const { return warnings_; }
    bool warningAlreadyPrinted(size_t i) const { return printedWarnings_.count(i) > 0; }

    bool zpCallMode = false;
    bool staticAllocMode = false;  // -fstaticalloc (SAC)
    bool inlineFunctions = false;
    int maxInlineIRSize = 11;  // -fmax-inline-ir-size / #pragma cc45 max_inline_ir_size
    int maxUnrollBytes = 128;  // -fmax-unroll-bytes / #pragma cc45 max_unroll_bytes: largest unrolled code
    bool devirtualize = true;    // -f[no-]devirtualization: single-implementation virtual calls -> direct calls
    bool futureWarnings = false; // --future: warn when #pragma cc45 optimize(...) names an unimplemented optimization
    bool allowImplicitFunctionDecl = false;  // -fimplicit-function-declaration

    // ASTVisitor interface
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
    void visit(StatementExpression& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(BuiltinVaStart& node) override;
    void visit(BuiltinVaArg& node) override;
    void visit(CpuRegisterAccess& node) override;
    void visit(CpuFlagAccess& node) override;
    void visit(LabelAddressExpression& node) override;
    void visit(TranslationUnit& node) override;
    void checkGlobalRedeclarations(TranslationUnit& node);

    void emitConditionBranches(Expression* cond, const std::string& trueLabel,
                               const std::string& falseLabel, ir::SourceLoc sl);

    // Phase 102: Typedef resolution interface
    void registerAllStructDefinitions(TranslationUnit& unit);
    std::string resolveTypedefToStruct(const std::string& typedefName);
    void registerTypedefToStruct(const std::string& typedefName, const std::string& structName);
    void setTypedefMappings(const std::map<std::string, std::string>& typedefToBaseType);

private:
    struct FunctionScope {
        ir::Function* func = nullptr;
        ir::Block* block = nullptr;
        std::map<std::string, ir::Operand> locals;
        std::map<std::string, ir::Type> localTypes;
        std::map<std::string, std::string> localTypeNames;
        std::map<std::string, bool> localSigned;
        std::map<std::string, bool> localPlainUnsigned;
        std::map<std::string, bool> localConst;
        std::map<std::string, bool> localPointsToConst;
        std::map<std::string, bool> localRegister;
        std::map<std::string, ir::Type> localPointedToType;
        std::map<std::string, std::vector<int>> localArrayDims;
        std::map<std::string, std::string> localPointedToTypeName;
        std::map<std::string, int> localPtrLevel;
        std::set<std::string> uplevelNames;
        std::set<uint32_t> usedVregs;
        std::map<std::string, ir::SourceLoc> localDeclLocs;
    };
    std::deque<FunctionScope> functionStack_;

    ir::Module module_;
    SourceLocationTracker sourceTracker_;  // Phase 113: Tracks source locations for DWARF
    ir::Function* currentFunc_ = nullptr;
    ir::Block* currentBlock_ = nullptr;

    // Last expression result (for chaining expression evaluation)
    ir::Operand lastValue_;
    bool computeAddressOnly_ = false;
    bool weakNextFunction_ = false;
    std::map<std::string, bool> optOverrides_;  // #pragma cc45 optimize(...): for the functions that follow
    bool recurseNextFunction_ = false;  // #pragma cc45 recurse

    // Variable tracking: name → allocated vReg (address operand for locals)
    std::map<std::string, ir::Operand> locals_;
    std::map<std::string, ir::Type> localTypes_;
    std::map<std::string, ir::Type> globalTypes_;
    std::map<std::string, std::string> localTypeNames_;
    std::map<std::string, std::string> globalTypeNames_;
    std::map<std::string, bool> localSigned_;  // true if variable was declared signed
    std::map<std::string, bool> globalSigned_; // true if global variable was declared signed
    // variables of a plain integer type, unsigned only by cc45's default
    // (VariableDeclaration::isPlainUnsigned): ConstantFolder warns about
    // them, so the "unsigned < 0" warning here skips them
    std::map<std::string, bool> localPlainUnsigned_;
    std::map<std::string, bool> globalPlainUnsigned_;
    bool plainUnsignedVar(Expression* e, std::string& name);
    std::map<std::string, bool> localConst_;       // true if variable itself is const
    std::map<std::string, bool> localPointsToConst_; // true if pointed-to data is const (const int *p)
    std::map<std::string, bool> localRegister_;
    std::map<std::string, bool> globalRegister_;
    std::map<std::string, ir::Type> localPointedToType_; // for pointers: the type of *ptr
    std::map<std::string, std::string> localPointedToTypeName_; // for pointers: the type name of *ptr
    std::map<std::string, int64_t> localConstPtrValue_; // constant pointer value (for propagation)
    std::map<std::string, ir::Type> globalPointedToType_; // for global pointers
    std::map<std::string, std::string> globalPointedToTypeName_; // for global pointers: the type name of *ptr
    std::map<std::string, std::vector<int>> localArrayDims_; // for stride computation
    std::map<std::string, std::vector<int>> globalArrayDims_; // for stride computation
    std::map<std::string, int> localPtrLevel_;   // declared pointer level (arrays: of the element)
    // In a nested function: the enclosing functions' variables whose types
    // are in the local type maps (they have no locals_ entry: reached through
    // the static chain)
    std::set<std::string> uplevelNames_;
    std::map<std::string, std::vector<int>> paramArrayDims_;  // int a[][M] parameter: {0, M}
    std::map<std::string, std::vector<ir::Operand>> paramVlaDims_;  // run-time inner dims (slots)
    std::map<std::string, std::string> vlaSizeVars_;  // variable-length array -> its size variable
    std::vector<std::unique_ptr<FunctionDeclaration>> vlaAllocaProtos_;  // kept alive
    // Blocks with variable-length arrays: the saved SP and the loop depth
    struct VlaBlock { ir::Operand spSlot; size_t loopDepth; };
    std::vector<VlaBlock> vlaBlocks_;
    void restoreVlaStack(const ir::Operand& spSlot);
    void releaseVlasForJump(size_t depth);
    std::map<std::string, int> globalPtrLevel_;

    // Unused variable tracking
    std::set<uint32_t> usedVregs_;
    std::set<uint32_t> setVregs_;   // locals assigned to (x = ..., x op= ...): "set but not used"
    std::map<std::string, ir::SourceLoc> localDeclLocs_;
    void resetFunctionLocalState();
    // Block scope: the bindings of the names a block's declarations shadow,
    // restored when it ends ({ int a = 5; { int a = 7; } a; })
    std::vector<std::function<void()>> saveShadowed(Statement* decls, bool list);
    void restoreShadowed(std::vector<std::function<void()>>& r);
    // Struct/union and float returns use a caller-allocated result buffer passed
    // as a hidden first parameter "__sret" (the callee also returns it in AX).
    ir::Operand sretCallBuffer(const std::string& fname, ir::Operand& buf, const ir::SourceLoc& sl);
    void sretCallResult(const std::string& fname, const ir::Operand& buf, const ir::Operand& addr, const ir::SourceLoc& sl);
    // The address of a value with none (a small struct returned in
    // registers): stored into a hidden local first; an address as it is
    ir::Operand addressOfValue(const ir::Operand& v, const ir::SourceLoc& sl);
    void emitMethodCall(const std::string& mangledName, std::vector<ir::Operand> args, const ir::SourceLoc& sl,
                        const ir::Operand* sretBuf = nullptr, const ir::Operand* sretAddr = nullptr);
    std::map<std::string, std::shared_ptr<FuncPtrSignature>> fnPtrSigs_;  // function-pointer variables
    std::map<std::string, std::vector<int>> paramAggSizes_;  // per function: by-copy aggregate param sizes (0 = scalar)
    std::set<std::string> aggCopyParams_;  // current function's by-copy aggregate params
    ir::Operand evalArgument(Expression* arg, int aggSize, const ir::SourceLoc& sl);
    int paramAggSize(const std::string& fname, size_t index);
    static constexpr int ZP_PARAM_BYTES = 32;  // ZP convention parameter block ($10-$2F)
    std::set<std::string> fastcallFunctions_;
    static int zpParamBytes(const std::vector<ir::Type>& types);
    bool usesZpConv(const std::string& fname);
    bool usesZpConvSig(const FuncPtrSignature& sig);
    // Functions the unit declares or defines at file scope (a call without a
    // prototype to one of the library's ZP assembly routines uses the ZP
    // convention only when the unit has no function of that name)
    std::set<std::string> unitFunctionNames_;
    std::set<std::string> unitDefinedFunctions_;  // functions with a body in this unit
    // printf("text") as a statement -> putstr / puts (no formatter linked)
    void simplifyPrintfCall(FunctionCall& call);
    // printf-family calls: the conversion hooks their format strings need
    void noteFormatNeeds(FunctionCall& call);
    void needAllFormats();
    std::set<std::string> stackCalledFunctions_;  // stack-mode: callees called with the stack convention
    static bool isLibraryZpRoutine(const std::string& fname);
    std::map<std::string, std::string> staticRenames_;  // static function -> module-unique symbol
    std::string staticFunctionName(const std::string& name);
    std::string resolvedFunctionName(const std::string& name) const;
    std::string staticAddressLabel(Expression* e);
    // Link-time constant addresses (label + offset) for static initializers
    bool constAddress(Expression* e, std::string& label, int& off);
    bool constLvalue(Expression* e, std::string& label, int& off);
    bool arrayShape(Expression* e, int& elemSize, std::vector<int>& dims);
    int pointeeStride(Expression* e);
    // Compile-time value of a constant expression (ConstEval): a float's,
    // or an integer's as its type says ((signed char)200 is -56); an
    // integer context truncates a float. For static initializers and case
    // labels
    bool evalConstFloat(Expression* e, double& out);
    bool evalConstInt(Expression* e, int64_t& out);
    // sizeof as a compile-time constant (not of a variable-length array or
    // a struct with run-time member sizes): a static initializer's value
    bool constSizeof(SizeofExpression& node, int64_t& out);
    int alignofValue(AlignofExpression& node);
    // ConstEvalContext
    bool sizeOf(SizeofExpression& node, int64_t& out) override { return constSizeof(node, out); }
    bool alignOf(AlignofExpression& node, int64_t& out) override { out = alignofValue(node); return true; }
    // _Complex support (complex types are struct _Complex_float/_Complex_int)
    std::string complexKindOf(Expression* e);
    bool isAggCopyParam(Expression* e);
    int complexTemp_ = 0;
    std::set<std::string> nestedLabelAddrs_;  // &&label taken inside nested functions
    std::unique_ptr<Expression> cloneSimpleExpr(Expression* e);
    void toComplex(std::unique_ptr<Expression>& e, const std::string& kind);
    std::string wideKindOf(Expression* e);
    void toWide(std::unique_ptr<Expression>& e, const std::string& kind, bool braced);
    void narrowWide(std::unique_ptr<Expression>& e, ir::Type t);
    bool currentFuncReturnsStruct();
    bool inlineParamAssigned(Statement* body, const std::string& name);
    bool evalConstComplex(Expression* e, double& re, double& im);
    // C library math functions known without a prototype (as GCC knows its
    // builtins): float signatures registered before the translation unit
    void declareBuiltinPrototypes();
    std::vector<std::unique_ptr<FunctionDeclaration>> builtinProtos_;
    std::set<std::string> builtinProtoNames_;  // not yet declared by the program
    // Constant initializer of an aggregate/array global laid out as bytes
    // (struct layout, nested braces, brace elision, designators); false if
    // some element isn't a link-time constant
    // Every __vt member (struct objects with virtual methods, also in arrays
    // and nested members) of an object of this type: (offset, vtable symbol)
    void collectVtableSlots(const std::string& type, int ptrLevel, const std::vector<int>& dims, int base,
                            std::vector<std::pair<int, std::string>>& out);
    // A global/static object: put its __vt pointers in the initial image
    void addVtableWords(ir::Module::GlobalVar& gv, const std::string& type, int ptrLevel, const std::vector<int>& dims);
    bool buildInitImage(ir::Module::GlobalVar& gv, const std::string& type, int ptrLevel,
                        const std::vector<int>& dims, Expression* init);
    struct InitImageCtx;
    bool initLocalAggregate(const ir::Operand& vreg, const std::string& type, int ptr,
                            const std::vector<int>& dims, Expression* init, const ir::SourceLoc& sl);
    bool initObject(InitImageCtx& c, int off, const std::string& type, int ptr, std::vector<int> dims, Expression* e);
    bool initElement(InitImageCtx& c, int off, const std::string& type, int ptr, std::vector<int> dims,
                     InitializerList& list, size_t& idx);
    bool fillAggregate(InitImageCtx& c, int off, const std::string& type, int ptr, std::vector<int> dims,
                       InitializerList& list, size_t& idx, bool braced);
    bool initScalar(InitImageCtx& c, int off, const std::string& type, int ptr, int bitWidth, int bitOffset,
                    Expression* e);
    int objectSize(const std::string& type, int ptr, const std::vector<int>& dims);
    std::set<std::string> methodsEmitted_;  // structs whose methods were lowered
    std::map<std::string, int> sretSize_;      // function name -> result size
    std::set<std::string> sretFloat_;          // ... of which return float/double
    ir::Operand sretDest_;                      // destination for the next sret call
    bool sretDestSet_ = false;
    // Aggregates (struct/union) whose size is not 1, 2 or 4 bytes are copied
    // by address rather than as a scalar value; returns that size, else 0.
    int aggregateBlockSize(const std::string& typeName, int pointerLevel);
    // A function's or function pointer's parameters as ln45 matches calls
    // through pointers: each slot's size ("2_4", "v" after a variadic one)
    std::string ptrSigKey(const std::vector<std::pair<std::string, int>>& params, bool variadic);
    std::string fpSigKey(const FuncPtrSignature* sig);   // "" for none or an unprototyped one
    ir::Operand emitAddressOf(Expression* expr);
    ir::Operand emitPtrOffset(const ir::Operand& base, int offset, const ir::SourceLoc& sl);
    void emitBlockCopy(const ir::Operand& dst, const ir::Operand& src, int size, const ir::SourceLoc& sl);
    // Evaluate an aggregate-valued expression into the memory at dst
    void emitAggregateInto(Expression* expr, const ir::Operand& dst, int size, const ir::SourceLoc& sl,
                           bool dstMayAlias = false);
    // results of && / || (already 0/1), by function: vreg numbers are each
    // function's own (a set of numbers alone said a vreg of a later function
    // was 0/1: x && p took a pointer p as its own truth value)
    std::set<std::pair<const ir::Function*, uint32_t>> boolVregs_;
    bool isBooleanValue(const ir::Operand& v) const;
    std::set<std::string> reportedUnused_;  // location keys of warnings already issued (unused names, x < 0)
    std::set<std::string> externalUsedVars_;

    // Track signedness of last expression result (for comparison op selection)
    bool lastValueSigned_ = false;
    std::set<std::string> currentFuncParams_;

    // String encoding pragma support
    enum class StringEncoding { PETSCII = 0, ASCII = 1, SCREENCODE = 2 };
    StringEncoding currentStringEncoding_ = StringEncoding::PETSCII;
    // A literal's encoding (0 PETSCII, 1 ASCII, 2 screen codes): its @/a/p/s
    // prefix, else the current #pragma encoding
    int stringEncodingOf(const StringLiteral& s) const {
        if (s.isAscii) return 1;
        if (s.prefixEncoding >= 0) return s.prefixEncoding;
        return (int)currentStringEncoding_;
    }

    // Struct layout tracking
    struct IRMemberInfo {
        std::string type;
        int pointerLevel = 0;
        bool isSigned = false;
        bool isConst = false;
        int offset = 0;
        int size = 0;
        std::vector<int> arrayDims;
        int bitWidth = 0;    // 0 = not a bitfield; >0 = width in bits
        int bitOffset = 0;   // bit offset within storage unit
        std::shared_ptr<FuncPtrSignature> funcPtrSig;   // a function pointer member's signature
    };
    struct IRStructInfo {
        std::string name;
        bool isUnion = false;
        std::map<std::string, IRMemberInfo> members;
        std::vector<std::string> memberOrder;
        int totalSize = 0;
        int align = 1;       // largest member / declared alignment (cc45 packs otherwise)
        int naturalAlign = 1; // alignment as a member of an __unpacked struct
        std::string parent;  // the struct it inherits from (struct D : B), or ""
    };
    std::map<std::string, IRStructInfo> structs_;
    const IRStructInfo* structOf(const std::string& type);  // struct/union or typedef of one
    const IRMemberInfo* arrayMemberOf(MemberAccess* ma);
    std::shared_ptr<FuncPtrSignature> callSignature(FunctionCall& node, std::string& calleeName);
    // Alignment of a type (an aligned struct's alignment; 1 otherwise: cc45 packs)
    int typeAlign(const std::string& type, int ptrLevel);
    void checkStaticInitNames(Expression* e);   // undeclared names in a static initializer
    // Natural alignment of a type, as an __unpacked struct aligns its members:
    // a 1/2/4-byte scalar or pointer its size, a struct its naturalAlign
    int naturalAlign(const std::string& type, int ptrLevel);
    // Value of an _Alignas / aligned expression (a constant or _Alignof(type))
    int alignValue(Expression* e);

    // Phase 102: Typedef to struct mapping for resolving typedef'd struct types
    std::map<std::string, std::string> typedefToStruct_; // typedef name → struct name (e.g., "digi_system_t" → "struct digi_system")

    // Break/continue label stack
    struct LoopLabels {
        std::string breakLabel;
        std::string continueLabel;
    };
    std::vector<LoopLabels> loopStack_;
    // building the initializer of a __noescape object: its string literals
    // are kept with it (Issue #301)
    bool initNoescape_ = false;
    // building a static/global initializer: a literal's address is stored
    // there, so it stays resident unless the object is __noescape itself
    bool inStaticInit_ = false;
    struct NoescapeInit {
        bool& flag; bool& init; bool saved, savedInit;
        NoescapeInit(bool& f, bool& i, bool on) : flag(f), init(i), saved(f), savedInit(i) { flag = on; init = true; }
        ~NoescapeInit() { flag = saved; init = savedInit; }
    };
    bool noescapeGlobal(const VariableDeclaration& node);
    int coldDepth_ = 0;     // > 0: building the branch of an if not expected to run
    size_t loopBase_ = 0;   // loopStack_ entries of the function being built (inlined bodies too)

    // Switch tracking
    struct SwitchCtx {
        std::string breakLabel;
        std::string defaultLabel;
        ir::Operand expr;
        struct CaseEntry {
            int64_t minVal;
            int64_t maxVal;  // == minVal for single cases
            bool isRange;
            std::string label;
        };
        std::vector<CaseEntry> cases;
        bool hasDefault = false;
        // continue <value> / continue default in the body: the selector is a
        // slot the statement stores a new value into before jumping back to
        // the dispatch (dispatchLabel)
        bool hasSwitchContinue = false;
        ir::Operand selSlot;
        std::string dispatchLabel;
    };
    std::vector<SwitchCtx> switchStack_;

    // Label generation
    int nextLabel_ = 0;
    std::string newLabel(const std::string& prefix = "L");

    // Track called function names for extern resolution
    std::set<std::string> calledFunctions_;
    std::set<std::string> definedFunctions_;
    std::map<std::string, ir::Type> functionReturnTypes_;
    std::map<std::string, bool> functionReturnSigned_;  // Signedness of return type
    std::map<std::string, std::vector<ir::Type>> functionParamTypes_;
    std::map<std::string, std::vector<bool>> functionParamSigned_;
    std::set<std::string> variadicFunctions_;
    std::set<std::string> regparmFunctions_;
    std::set<std::string> structReturningFunctions_;  // Functions that return struct by value
    ir::Operand structReturnPtr_;  // Hidden return pointer parameter for current function (-1 if none)

    // Inline function support: store AST nodes for inline-eligible functions
    std::map<std::string, FunctionDeclaration*> inlineCandidates_;
    std::set<std::string> alwaysInlineNames_;   // always_inline: bypass size limits
    std::set<std::string> noInlineNames_;       // noinline: never expand
    std::set<std::string> warnedRecursiveInline_;
    void warnRecursiveForcedInline(const std::string& name, ASTNode& at);
    std::map<std::string, FunctionDeclaration*> allFunctions_;
    std::map<std::string, bool> parsedStructIsFinal_; // struct name → isFinal
    std::set<std::string> inlineExpansionStack_;  // recursion guard
    static constexpr int INLINE_MAX_STMTS = 20;   // max body statements for auto-inline
    ir::Operand inlineReturnTarget_;               // vreg for inlined return value
    std::string inlineReturnLabel_;                // merge label for inlined returns
    std::string inlineSourceFunc_;                 // callee function name during inline expansion (for asm rewriting)

    // Track function parameter info for const-qualification warnings
    struct ParamInfo { bool isConst = false; int pointerLevel = 0; };
    std::map<std::string, std::vector<ParamInfo>> funcParamInfo_;

    // Error and warning reporting
    std::vector<std::string> errors_;
    std::vector<std::string> warnings_;
    std::set<size_t> printedWarnings_;  // indices emitted immediately by emitWarning

    friend class CaseCollector;

    // Helper: emit an instruction to the current block
    void emit(ir::Inst inst);

    // Helper: create a new block and make it current
    ir::Block& startBlock(const std::string& label);

    // Helper: allocate a new vReg
    ir::Operand allocVreg(ir::Type t);

    struct IRTypeInfo {
        ir::Type type = ir::Type::VOID;
        std::string typeName; // Original C type name (e.g. "struct Point")
        bool isSigned = false;
        ir::Type pointedToType = ir::Type::VOID; // if type == PTR
        int totalSize = 0;
        // a pointer's levels down to typeName (1: it points at a typeName;
        // 0: not known), so *p of a char ** is a char *, and **p a char
        int ptrDepth = 0;
    };

    IRTypeInfo getExprTypeInfo(Expression* expr);
    void markStaticLinkProvider();
    // GCC vectors (struct __vec_<T>_<s|u>_<n> { T e[n]; })
    std::string vecOfType(const std::string& typeName, int ptrLevel);
    std::string vecOf(Expression* e);
    void vecElem(const std::string& vn, std::string& type, bool& isSigned, int& count);
    std::string vecMaskType(const std::string& vn);
    std::string vecTemp(const ir::Operand& v, ir::Type t, const std::string& typeName, int ptrLevel, bool sg);
    std::string vecOperand(Expression* e, bool isVec, const std::string& et, bool sg);
public:
    using VecIndex = std::function<std::unique_ptr<Expression>()>;  // a fresh element index expression
private:
    void vecResult(const std::string& rn, const std::function<std::unique_ptr<Expression>(const VecIndex&)>& elem, ASTNode& at);
    void emitVectorBinary(BinaryOperation& node);
    std::string vecResultArray(const std::string& rn, ASTNode& at);
    void vecResultValue(const std::string& rn, const std::string& res, ASTNode& at);
    void emitVectorUnary(UnaryOperation& node);
    void emitVectorShuffle(FunctionCall& node);
    void vecSubscript(ArrayAccess& node);
    std::vector<std::unique_ptr<ASTNode>> vecNodes_;  // AST built for vector operations (kept alive)
    int vecTempCount_ = 0;
    // Structs with run-time member sizes: hidden locals with each member's
    // offset and the size; variables of such a type are pointers to alloca'd
    // storage (vmVars_: variable -> struct name)
    struct VmStruct {
        std::string sizeVar;
        std::map<std::string, std::string> offsetVar;
        std::map<std::string, std::vector<std::string>> memberDims;  // "" = constant dimension
    };
    std::map<std::string, VmStruct> vmStructs_;
    std::map<std::string, std::string> vmVars_;
    ir::Operand vmCopySize_;
    bool inVmVarLoad_ = false;
    std::string vmStructOf(Expression* e);
    bool registeringStructs_ = false;  // registerAllStructDefinitions pre-pass
    int ptrArrayRowSize(Expression* e);
    std::map<std::string, int> declaredAlign_;  // variable -> declared alignment
    std::map<std::string, int> funcAlign_;      // function -> __attribute__((aligned))
    std::map<std::string, std::vector<int>> globalPtrArrayDims_;  // global T (*p)[M] -> {0, M}
    void keepStaticLink(ir::Function* f);
    void addUplevelChain(ir::Inst& inst, int levels);

    // Helper: map C type to IR type
    ir::Type mapType(const std::string& typeName, int ptrLevel);

    // Helper: get size for a type (using unified TypeSystem)
    int getTypeSize(const std::string& typeName, int ptrLevel);

    // Helper: source location from AST node
    ir::SourceLoc loc(ASTNode& node);
    ir::SourceLoc declLoc(ASTNode& node);  // loc() plus the node's own file and column, for diagnostics

    // Helper: emit explicit or implicit cast
    ir::Operand emitCast(ir::Operand src, ir::Type targetType, bool isSigned);

    // Helper: normalize struct/union names
    std::string getAggregateName(const std::string& type);

    // Helper: recursively find a member in a struct/union, returning its info and total offset
    IRMemberInfo* findStructMember(const std::string& sName, const std::string& memberName, int& accumulatedOffset);
};
