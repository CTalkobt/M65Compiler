#pragma once
#include "AST.hpp"

/**
 * TraversingVisitor: Base class that reduces boilerplate in visitor subclasses.
 *
 * Provides default implementations for all visit() methods that recursively
 * visit child nodes. Subclasses override only methods with custom logic.
 *
 * Example:
 *   class MyCollector : public TraversingVisitor {
 *   public:
 *       std::vector<std::string> names;
 *       void visit(FunctionCall& n) override {
 *           names.push_back(n.name);
 *           TraversingVisitor::visit(n);  // Continue traversal
 *       }
 *   };
 *
 * This dramatically reduces boilerplate: instead of implementing all 43 visit()
 * methods with default traversals, subclasses only override what they need.
 *
 * Code reduction: ~50KB across CodeGenerator, LoopOptimizer, ConstantFolder.
 */
class TraversingVisitor : public ASTVisitor {
public:
    virtual ~TraversingVisitor() = default;

    // Expressions: default implementations just visit children
    void visit(IntegerLiteral&) override {}
    void visit(StringLiteral&) override {}
    void visit(VariableReference&) override {}
    void visit(FunctionCall& n) override {
        for (auto& arg : n.arguments) arg->accept(*this);
    }
    void visit(ArrayAccess& n) override {
        n.arrayExpr->accept(*this);
        n.indexExpr->accept(*this);
    }
    void visit(MemberAccess& n) override {
        n.structExpr->accept(*this);
    }
    void visit(CastExpression& n) override {
        n.expression->accept(*this);
    }
    void visit(SizeofExpression& n) override {
        if (n.expression) n.expression->accept(*this);
    }
    void visit(AlignofExpression&) override {}
    void visit(UnaryOperation& n) override {
        n.operand->accept(*this);
    }
    void visit(BinaryOperation& n) override {
        n.left->accept(*this);
        n.right->accept(*this);
    }
    void visit(ConditionalExpression& n) override {
        n.condition->accept(*this);
        n.thenExpr->accept(*this);
        n.elseExpr->accept(*this);
    }
    void visit(Assignment& n) override {
        n.target->accept(*this);
        n.expression->accept(*this);
    }
    void visit(GenericSelection& n) override {
        n.control->accept(*this);
        for (auto& a : n.associations) {
            a.result->accept(*this);
        }
    }
    void visit(InitializerList& n) override {
        for (auto& e : n.elements) {
            e->accept(*this);
        }
    }
    void visit(CompoundLiteral& n) override {
        for (auto& e : n.initializer->elements) {
            e->accept(*this);
        }
    }
    void visit(BuiltinVaStart& n) override {
        n.ap->accept(*this);
    }
    void visit(BuiltinVaArg& n) override {
        n.ap->accept(*this);
    }
    void visit(CpuRegisterAccess&) override {}
    void visit(CpuFlagAccess&) override {}

    // Statements: default implementations recursively visit
    void visit(VariableDeclaration& n) override {
        if (n.initializer) n.initializer->accept(*this);
    }
    void visit(FunctionDeclaration& n) override {
        n.body->accept(*this);
    }
    void visit(IfStatement& n) override {
        n.condition->accept(*this);
        n.thenBranch->accept(*this);
        if (n.elseBranch) n.elseBranch->accept(*this);
    }
    void visit(WhileStatement& n) override {
        n.condition->accept(*this);
        n.body->accept(*this);
    }
    void visit(DoWhileStatement& n) override {
        n.body->accept(*this);
        n.condition->accept(*this);
    }
    void visit(ForStatement& n) override {
        if (n.initializer) n.initializer->accept(*this);
        if (n.condition) n.condition->accept(*this);
        if (n.increment) n.increment->accept(*this);
        n.body->accept(*this);
    }
    void visit(RepeatStatement& n) override {
        n.body->accept(*this);
    }
    void visit(SwitchStatement& n) override {
        n.expression->accept(*this);
        n.body->accept(*this);
    }
    void visit(CaseStatement& n) override {
        n.value->accept(*this);
        if (n.rangeEnd) n.rangeEnd->accept(*this);
    }
    void visit(DefaultStatement&) override {}
    void visit(BreakStatement&) override {}
    void visit(ContinueStatement&) override {}
    void visit(SwitchContinueStatement& n) override {
        if (n.target) n.target->accept(*this);
    }
    void visit(ReturnStatement& n) override {
        if (n.expression) n.expression->accept(*this);
    }
    void visit(ExpressionStatement& n) override {
        n.expression->accept(*this);
    }
    void visit(CompoundStatement& n) override {
        for (auto& s : n.statements) {
            s->accept(*this);
        }
    }
    void visit(AsmStatement&) override {}
    void visit(GotoStatement&) override {}
    void visit(LabelledStatement& n) override {
        n.statement->accept(*this);
    }
    void visit(StaticAssert&) override {}

    // Declarations
    void visit(StructDefinition&) override {}
    void visit(EnumDefinition&) override {}

    // Top-level
    void visit(TranslationUnit& n) override {
        for (auto& d : n.topLevelDecls) {
            d->accept(*this);
        }
    }
};
