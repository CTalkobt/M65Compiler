#pragma once

#include "AST.hpp"
#include "TraversingVisitor.hpp"
#include <set>
#include <string>
#include <memory>

// Loop-Invariant Code Motion (LICM) optimizer
// Detects expressions that don't change within a loop and hoists them outside
// Example: for(r=0; r<H; r++) { for(c=0; c<W; c++) { grid[r*40+c] } }
// Transforms to: for(r=0; r<H; r++) { int tmp=r*40; for(c=0; c<W; c++) { grid[tmp+c] } }

class LoopOptimizer : public TraversingVisitor {
public:
    void optimizeTranslationUnit(TranslationUnit& unit);

    // Visitor methods for all statement types
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

    // Expression visitors (no-op for LICM)
    void visit(IntegerLiteral& node) override {}
    void visit(StringLiteral& node) override {}
    void visit(VariableReference& node) override {}
    void visit(ArrayAccess& node) override {}
    void visit(MemberAccess& node) override {}
    void visit(FunctionCall& node) override {}
    void visit(BinaryOperation& node) override {}
    void visit(UnaryOperation& node) override {}
    void visit(CastExpression& node) override {}
    void visit(SizeofExpression& node) override {}
    void visit(AlignofExpression& node) override {}
    void visit(ConditionalExpression& node) override {}
    void visit(Assignment& node) override {}
    void visit(InitializerList& node) override {}
    void visit(CompoundLiteral& node) override {}
    void visit(GenericSelection& node) override {}
    void visit(BuiltinVaStart& node) override {}
    void visit(BuiltinVaArg& node) override {}
    void visit(CpuRegisterAccess& node) override {}
    void visit(CpuFlagAccess& node) override {}

private:
    // Collect all variables referenced by an expression
    class VariableCollector : public TraversingVisitor {
    public:
        std::set<std::string> variables;

        void visit(VariableReference& node) override {
            variables.insert(node.name);
        }
        void visit(WhileStatement&) override {}
        void visit(DoWhileStatement&) override {}
        void visit(TranslationUnit&) override {}
    };
};
