#pragma once

#include "AST.hpp"
#include <set>
#include <string>
#include <memory>

// Loop-Invariant Code Motion (LICM) optimizer
// Detects expressions that don't change within a loop and hoists them outside
// Example: for(r=0; r<H; r++) { for(c=0; c<W; c++) { grid[r*40+c] } }
// Transforms to: for(r=0; r<H; r++) { int tmp=r*40; for(c=0; c<W; c++) { grid[tmp+c] } }

class LoopOptimizer : public ASTVisitor {
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
    void visit(FloatLiteral& node) override {}
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
    void visit(LabelAddressExpression& node) override {}

private:
    // Collect all variables referenced by an expression
    class VariableCollector : public ASTVisitor {
    public:
        std::set<std::string> variables;

        void visit(VariableReference& node) override { variables.insert(node.name); }
        void visit(ArrayAccess& node) override { node.arrayExpr->accept(*this); node.indexExpr->accept(*this); }
        void visit(MemberAccess& node) override { node.structExpr->accept(*this); }
        void visit(BinaryOperation& node) override { node.left->accept(*this); node.right->accept(*this); }
        void visit(UnaryOperation& node) override { node.operand->accept(*this); }
        void visit(ConditionalExpression& node) override {
            node.condition->accept(*this);
            node.thenExpr->accept(*this);
            node.elseExpr->accept(*this);
        }
        void visit(FunctionCall& node) override {
            for (auto& arg : node.arguments) arg->accept(*this);
        }
        void visit(CastExpression& node) override { node.expression->accept(*this); }
        void visit(Assignment& node) override { node.target->accept(*this); node.expression->accept(*this); }
        void visit(InitializerList& node) override { for (auto& e : node.elements) e->accept(*this); }
        void visit(CompoundLiteral& node) override { for (auto& e : node.initializer->elements) e->accept(*this); }
        void visit(GenericSelection& node) override {
            node.control->accept(*this);
            for (auto& a : node.associations) a.result->accept(*this);
        }
        void visit(BuiltinVaStart& node) override { node.ap->accept(*this); }
        void visit(BuiltinVaArg& node) override { node.ap->accept(*this); }
        void visit(SizeofExpression& node) override { if (node.expression) node.expression->accept(*this); }

        // All other visits are no-ops
        void visit(IntegerLiteral&) override {}
        void visit(StringLiteral&) override {}
        void visit(AlignofExpression&) override {}
        void visit(CpuRegisterAccess&) override {}
        void visit(CpuFlagAccess&) override {}
        void visit(IfStatement&) override {}
        void visit(ExpressionStatement&) override {}
        void visit(ReturnStatement&) override {}
        void visit(BreakStatement&) override {}
        void visit(ContinueStatement&) override {}
        void visit(SwitchStatement&) override {}
        void visit(CaseStatement&) override {}
        void visit(DefaultStatement&) override {}
        void visit(LabelledStatement&) override {}
        void visit(GotoStatement&) override {}
        void visit(SwitchContinueStatement&) override {}
        void visit(RepeatStatement&) override {}
        void visit(VariableDeclaration&) override {}
        void visit(FunctionDeclaration&) override {}
        void visit(AsmStatement&) override {}
        void visit(StaticAssert&) override {}
        void visit(StructDefinition&) override {}
        void visit(EnumDefinition&) override {}
        void visit(CompoundStatement&) override {}
        void visit(WhileStatement&) override {}
        void visit(DoWhileStatement&) override {}
        void visit(TranslationUnit&) override {}
    };
};
