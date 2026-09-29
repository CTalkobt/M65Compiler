#pragma once

#include "AST.hpp"
#include <memory>

class LoopInterchange : public ASTVisitor {
public:
    void optimizeTranslationUnit(TranslationUnit& unit);

    void visit(ForStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(DoWhileStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(CompoundStatement& node) override;
    void visit(ExpressionStatement& node) override;
    void visit(ReturnStatement& node) override;
    void visit(BreakStatement& node) override;
    void visit(ContinueStatement& node) override;
    void visit(SwitchStatement& node) override;
    void visit(CaseStatement& node) override;
    void visit(DefaultStatement& node) override;
    void visit(LabelledStatement& node) override;
    void visit(GotoStatement& node) override;
    void visit(SwitchContinueStatement& node) override;
    void visit(RepeatStatement& node) override;
    void visit(VariableDeclaration& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(AsmStatement& node) override;
    void visit(StaticAssert& node) override;
    void visit(StructDefinition& node) override;
    void visit(EnumDefinition& node) override;
    void visit(TranslationUnit& node) override;

    void visit(IntegerLiteral&) override {}
    void visit(FloatLiteral&) override {}
    void visit(StringLiteral&) override {}
    void visit(VariableReference&) override {}
    void visit(ArrayAccess&) override {}
    void visit(MemberAccess&) override {}
    void visit(FunctionCall&) override {}
    void visit(BinaryOperation&) override {}
    void visit(UnaryOperation&) override {}
    void visit(CastExpression&) override {}
    void visit(SizeofExpression&) override {}
    void visit(AlignofExpression&) override {}
    void visit(ConditionalExpression&) override {}
    void visit(Assignment&) override {}
    void visit(InitializerList&) override {}
    void visit(CompoundLiteral&) override {}
    void visit(GenericSelection&) override {}
    void visit(BuiltinVaStart&) override {}
    void visit(BuiltinVaArg&) override {}
    void visit(CpuRegisterAccess&) override {}
    void visit(CpuFlagAccess&) override {}
    void visit(LabelAddressExpression&) override {}

private:
    // Pattern detection: check if loop pair is interchangeable
    struct LoopPair {
        ForStatement* outer;
        ForStatement* inner;
        std::string outerVar;
        std::string innerVar;
    };

    // Detect perfectly nested for loops safe for interchange
    bool detectInterchangeable(ForStatement* stmt, LoopPair& pair);
    
    // Check if inner loop has data dependency on outer loop variable
    bool hasDependencyOnOuterLoop(Statement* body, const std::string& outerVar);
    
    // Check if expression references outer loop variable
    bool referencesVariable(Expression* expr, const std::string& varName);
    
    // Perform the interchange transformation
    std::unique_ptr<ForStatement> interchangeLoops(const LoopPair& pair);
};
