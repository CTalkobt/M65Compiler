#pragma once
#include "AST.hpp"
#include "IR.hpp"
#include <map>
#include <set>
#include <string>
#include <vector>
#include <deque>

class IRBuilder : public ASTVisitor {
public:
    IRBuilder();

    void generate(TranslationUnit& unit);
    void setSourceInfo(const std::string& filename);
    void setExternalUsedVars(const std::set<std::string>& vars) { externalUsedVars_ = vars; }
    const ir::Module& getModule() const { return module_; }
    bool hasErrors() const { return !errors_.empty(); }
    const std::vector<std::string>& getErrors() const { return errors_; }
    const std::vector<std::string>& getWarnings() const { return warnings_; }

    bool zpCallMode = false;
    bool inlineFunctions = false;

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
    void visit(FunctionDeclaration& node) override;
    void visit(BuiltinVaStart& node) override;
    void visit(BuiltinVaArg& node) override;
    void visit(CpuRegisterAccess& node) override;
    void visit(CpuFlagAccess& node) override;
    void visit(LabelAddressExpression& node) override;
    void visit(TranslationUnit& node) override;

    void emitConditionBranches(Expression* cond, const std::string& trueLabel,
                               const std::string& falseLabel, ir::SourceLoc sl);
private:
    struct FunctionScope {
        ir::Function* func = nullptr;
        ir::Block* block = nullptr;
        std::map<std::string, ir::Operand> locals;
        std::map<std::string, ir::Type> localTypes;
        std::map<std::string, std::string> localTypeNames;
        std::map<std::string, bool> localSigned;
        std::map<std::string, bool> localConst;
        std::map<std::string, bool> localPointsToConst;
        std::map<std::string, ir::Type> localPointedToType;
        std::map<std::string, std::vector<int>> localArrayDims;
        std::set<uint32_t> usedVregs;
        std::map<std::string, ir::SourceLoc> localDeclLocs;
    };
    std::deque<FunctionScope> functionStack_;

    ir::Module module_;
    ir::Function* currentFunc_ = nullptr;
    ir::Block* currentBlock_ = nullptr;

    // Last expression result (for chaining expression evaluation)
    ir::Operand lastValue_;
    bool computeAddressOnly_ = false;

    // Variable tracking: name → allocated vReg (address operand for locals)
    std::map<std::string, ir::Operand> locals_;
    std::map<std::string, ir::Type> localTypes_;
    std::map<std::string, ir::Type> globalTypes_;
    std::map<std::string, std::string> localTypeNames_;
    std::map<std::string, std::string> globalTypeNames_;
    std::map<std::string, bool> localSigned_;  // true if variable was declared signed
    std::map<std::string, bool> localConst_;       // true if variable itself is const
    std::map<std::string, bool> localPointsToConst_; // true if pointed-to data is const (const int *p)
    std::map<std::string, ir::Type> localPointedToType_; // for pointers: the type of *ptr
    std::map<std::string, int64_t> localConstPtrValue_; // constant pointer value (for propagation)
    std::map<std::string, ir::Type> globalPointedToType_; // for global pointers
    std::map<std::string, std::vector<int>> localArrayDims_; // for stride computation
    std::map<std::string, std::vector<int>> globalArrayDims_; // for stride computation

    // Unused variable tracking
    std::set<uint32_t> usedVregs_;
    std::map<std::string, ir::SourceLoc> localDeclLocs_;
    std::set<std::string> externalUsedVars_;

    // Track signedness of last expression result (for comparison op selection)
    bool lastValueSigned_ = false;

    // String encoding pragma support
    enum class StringEncoding { PETSCII = 0, ASCII = 1, SCREENCODE = 2 };
    StringEncoding currentStringEncoding_ = StringEncoding::PETSCII;

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
    };
    struct IRStructInfo {
        std::string name;
        bool isUnion = false;
        std::map<std::string, IRMemberInfo> members;
        std::vector<std::string> memberOrder;
        int totalSize = 0;
    };
    std::map<std::string, IRStructInfo> structs_;

    // Break/continue label stack
    struct LoopLabels {
        std::string breakLabel;
        std::string continueLabel;
    };
    std::vector<LoopLabels> loopStack_;

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
    };
    std::vector<SwitchCtx> switchStack_;

    // Label generation
    int nextLabel_ = 0;
    std::string newLabel(const std::string& prefix = "L");

    // Track called function names for extern resolution
    std::set<std::string> calledFunctions_;
    std::set<std::string> definedFunctions_;
    std::map<std::string, ir::Type> functionReturnTypes_;
    std::map<std::string, std::vector<ir::Type>> functionParamTypes_;
    std::map<std::string, std::vector<bool>> functionParamSigned_;
    std::set<std::string> variadicFunctions_;
    std::set<std::string> regparmFunctions_;

    // Inline function support: store AST nodes for inline-eligible functions
    std::map<std::string, FunctionDeclaration*> inlineCandidates_;
    std::map<std::string, FunctionDeclaration*> allFunctions_;
    std::map<std::string, bool> parsedStructIsFinal_; // struct name → isFinal
    std::set<std::string> inlineExpansionStack_;  // recursion guard
    static constexpr int INLINE_MAX_STMTS = 20;   // max body statements for auto-inline
    ir::Operand inlineReturnTarget_;               // vreg for inlined return value
    std::string inlineReturnLabel_;                // merge label for inlined returns

    // Track function parameter info for const-qualification warnings
    struct ParamInfo { bool isConst = false; int pointerLevel = 0; };
    std::map<std::string, std::vector<ParamInfo>> funcParamInfo_;

    // Error and warning reporting
    std::vector<std::string> errors_;
    std::vector<std::string> warnings_;

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
    };

    IRTypeInfo getExprTypeInfo(Expression* expr);

    // Helper: map C type to IR type
    ir::Type mapType(const std::string& typeName, int ptrLevel);

    // Helper: get size for a type
    int getTypeSize(const std::string& typeName, int ptrLevel);

    // Helper: source location from AST node
    ir::SourceLoc loc(ASTNode& node);

    // Helper: emit explicit or implicit cast
    ir::Operand emitCast(ir::Operand src, ir::Type targetType, bool isSigned);

    // Helper: normalize struct/union names
    std::string getAggregateName(const std::string& type);
};
