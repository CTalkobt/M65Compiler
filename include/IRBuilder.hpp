#pragma once
#include "AST.hpp"
#include "IR.hpp"
#include <map>
#include <set>
#include <string>
#include <vector>

class IRBuilder : public ASTVisitor {
public:
    IRBuilder();

    void generate(TranslationUnit& unit);
    void setSourceInfo(const std::string& filename);
    const ir::Module& getModule() const { return module_; }
    bool hasErrors() const { return !errors_.empty(); }
    const std::vector<std::string>& getErrors() const { return errors_; }
    const std::vector<std::string>& getWarnings() const { return warnings_; }

    bool zpCallMode = false;

    // ASTVisitor interface
    void visit(IntegerLiteral& node) override;
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
    void visit(TranslationUnit& node) override;

private:
    ir::Module module_;
    ir::Function* currentFunc_ = nullptr;
    ir::Block* currentBlock_ = nullptr;

    // Last expression result (for chaining expression evaluation)
    ir::Operand lastValue_;

    // Variable tracking: name → allocated vReg (address operand for locals)
    std::map<std::string, ir::Operand> locals_;
    std::map<std::string, ir::Type> localTypes_;
    std::map<std::string, bool> localSigned_;  // true if variable was declared signed
    std::map<std::string, bool> localConst_;       // true if variable itself is const
    std::map<std::string, bool> localPointsToConst_; // true if pointed-to data is const (const int *p)
    std::map<std::string, std::vector<int>> localArrayDims_; // for stride computation

    // Track signedness of last expression result (for comparison op selection)
    bool lastValueSigned_ = false;

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
        std::vector<std::pair<int64_t, std::string>> cases;
        bool hasDefault = false;
    };
    std::vector<SwitchCtx> switchStack_;

    // Label generation
    int nextLabel_ = 0;
    std::string newLabel(const std::string& prefix = "L");

    // Track called function names for extern resolution
    std::set<std::string> calledFunctions_;
    std::set<std::string> definedFunctions_;

    // Track function parameter info for const-qualification warnings
    struct ParamInfo { bool isConst = false; int pointerLevel = 0; };
    std::map<std::string, std::vector<ParamInfo>> funcParamInfo_;

    // Error and warning reporting
    std::vector<std::string> errors_;
    std::vector<std::string> warnings_;

    // Helper: emit an instruction to the current block
    void emit(ir::Inst inst);

    // Helper: create a new block and make it current
    ir::Block& startBlock(const std::string& label);

    // Helper: allocate a new vReg
    ir::Operand allocVreg(ir::Type t);

    // Helper: map C type to IR type
    ir::Type mapType(const std::string& typeName, int ptrLevel);

    // Helper: get size for a type
    int getTypeSize(const std::string& typeName, int ptrLevel);

    // Helper: source location from AST node
    ir::SourceLoc loc(ASTNode& node);
};
