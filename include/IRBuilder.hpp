#pragma once
#include "AST.hpp"
#include "IR.hpp"
#include <map>
#include <string>
#include <vector>

class IRBuilder : public ASTVisitor {
public:
    IRBuilder();

    void generate(TranslationUnit& unit);
    void setSourceInfo(const std::string& filename);
    const ir::Module& getModule() const { return module_; }

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

    // Label generation
    int nextLabel_ = 0;
    std::string newLabel(const std::string& prefix = "L");

    // Helper: emit an instruction to the current block
    void emit(ir::Inst inst);

    // Helper: create a new block and make it current
    ir::Block& startBlock(const std::string& label);

    // Helper: allocate a new vReg
    ir::Operand allocVreg(ir::Type t);

    // Helper: map C type to IR type
    ir::Type mapType(const std::string& typeName, int ptrLevel);

    // Helper: source location from AST node
    ir::SourceLoc loc(ASTNode& node);
};
