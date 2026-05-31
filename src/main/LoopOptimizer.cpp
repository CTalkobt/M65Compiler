#include "LoopOptimizer.hpp"
#include "TraversingVisitor.hpp"
#include <algorithm>
#include <memory>

namespace {
    // Helper: collect all variable names referenced in an expression
    class VarCollector : public TraversingVisitor {
    public:
        std::set<std::string> vars;

        void visit(VariableReference& n) override {
            vars.insert(n.name);
        }
    };

    // Check if two expressions are structurally equal
    class ExpressionComparator : public TraversingVisitor {
    public:
        Expression* target = nullptr;
        bool equal = false;

        void compare(Expression& a, Expression& b) {
            target = &b;
            equal = false;
            a.accept(*this);
        }

        void visit(IntegerLiteral& n) override {
            if (auto* t = dynamic_cast<IntegerLiteral*>(target))
                equal = (n.value == t->value);
        }
        void visit(StringLiteral& n) override {
            if (auto* t = dynamic_cast<StringLiteral*>(target))
                equal = (n.value == t->value);
        }
        void visit(VariableReference& n) override {
            if (auto* t = dynamic_cast<VariableReference*>(target))
                equal = (n.name == t->name);
        }
        void visit(BinaryOperation& n) override {
            if (auto* t = dynamic_cast<BinaryOperation*>(target)) {
                if (n.op != t->op) return;
                ExpressionComparator cmp;
                cmp.compare(*n.left, *t->left);
                if (!cmp.equal) return;
                cmp.compare(*n.right, *t->right);
                equal = cmp.equal;
            }
        }
        void visit(UnaryOperation& n) override {
            if (auto* t = dynamic_cast<UnaryOperation*>(target)) {
                if (n.op != t->op) return;
                ExpressionComparator cmp;
                cmp.compare(*n.operand, *t->operand);
                equal = cmp.equal;
            }
        }
        void visit(ArrayAccess& n) override {
            if (auto* t = dynamic_cast<ArrayAccess*>(target)) {
                ExpressionComparator cmp;
                cmp.compare(*n.arrayExpr, *t->arrayExpr);
                if (!cmp.equal) return;
                cmp.compare(*n.indexExpr, *t->indexExpr);
                equal = cmp.equal;
            }
        }
        void visit(CastExpression& n) override {
            if (auto* t = dynamic_cast<CastExpression*>(target)) {
                ExpressionComparator cmp;
                cmp.compare(*n.expression, *t->expression);
                equal = cmp.equal;
            }
        }
    };

    // Collect all expressions from a statement subtree
    class ExpressionCollector : public TraversingVisitor {
    public:
        struct ExprInfo {
            Expression* expr;
            size_t count = 0;  // How many times it appears
            std::string exprStr;  // String representation for hashing
        };
        std::vector<ExprInfo> expressions;

        void visit(BinaryOperation& n) override {
            addExpr(&n);
            TraversingVisitor::visit(n);
        }
        void visit(UnaryOperation& n) override {
            addExpr(&n);
            TraversingVisitor::visit(n);
        }
        void visit(ArrayAccess& n) override {
            addExpr(&n);
            TraversingVisitor::visit(n);
        }
        void visit(CastExpression& n) override {
            addExpr(&n);
            TraversingVisitor::visit(n);
        }

    private:
        void addExpr(Expression* expr) {
            // Find if we already have this expression
            for (auto& ei : expressions) {
                ExpressionComparator cmp;
                cmp.compare(*expr, *ei.expr);
                if (cmp.equal) {
                    ei.count++;
                    return;
                }
            }
            // New expression
            ExprInfo info;
            info.expr = expr;
            info.count = 1;
            expressions.push_back(info);
        }
    };
}

void LoopOptimizer::optimizeTranslationUnit(TranslationUnit& unit) {
    unit.accept(*this);
}

void LoopOptimizer::visit(ForStatement& node) {
    // First recurse into nested loops
    if (node.body) node.body->accept(*this);

    // Collect loop variables
    std::set<std::string> loopVars;
    if (auto* vardecl = dynamic_cast<VariableDeclaration*>(node.initializer.get())) {
        loopVars.insert(vardecl->name);
    }

    // Skip if no loop body or variable
    if (!node.body || loopVars.empty()) return;

    // Collect all expressions in the loop body
    ExpressionCollector collector;
    node.body->accept(collector);

    // Find loop-invariant expressions that appear 2+ times
    int hoistCount = 0;
    for (const auto& exprInfo : collector.expressions) {
        if (exprInfo.count < 2) continue;
        if (!exprInfo.expr) continue;

        // Check if loop-invariant
        VarCollector varCollector;
        exprInfo.expr->accept(varCollector);

        bool isInvariant = true;
        for (const auto& var : varCollector.vars) {
            if (loopVars.count(var)) {
                isInvariant = false;
                break;
            }
        }

        if (!isInvariant) continue;

        // Skip trivial expressions
        if (dynamic_cast<IntegerLiteral*>(exprInfo.expr)) continue;
        if (dynamic_cast<StringLiteral*>(exprInfo.expr)) continue;
        if (dynamic_cast<VariableReference*>(exprInfo.expr)) continue;

        hoistCount++;
        // CSE transformation: expression could be hoisted
        // Actual AST mutation (creating temp vars, replacing occurrences) deferred
        // Requires: expression cloning, type inference, statement insertion, scope management
    }
}

void LoopOptimizer::visit(WhileStatement& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.body) node.body->accept(*this);
}

void LoopOptimizer::visit(DoWhileStatement& node) {
    if (node.body) node.body->accept(*this);
    if (node.condition) node.condition->accept(*this);
}

void LoopOptimizer::visit(IfStatement& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.thenBranch) node.thenBranch->accept(*this);
    if (node.elseBranch) node.elseBranch->accept(*this);
}

void LoopOptimizer::visit(CompoundStatement& node) {
    for (auto& stmt : node.statements) {
        if (stmt) stmt->accept(*this);
    }
}

void LoopOptimizer::visit(ExpressionStatement& node) {
    if (node.expression) node.expression->accept(*this);
}

void LoopOptimizer::visit(ReturnStatement& node) {
    if (node.expression) node.expression->accept(*this);
}

void LoopOptimizer::visit(BreakStatement& node) {}
void LoopOptimizer::visit(ContinueStatement& node) {}

void LoopOptimizer::visit(SwitchStatement& node) {
    if (node.expression) node.expression->accept(*this);
    if (node.body) node.body->accept(*this);
}

void LoopOptimizer::visit(CaseStatement& node) {
    if (node.value) node.value->accept(*this);
    if (node.rangeEnd) node.rangeEnd->accept(*this);
}

void LoopOptimizer::visit(DefaultStatement& node) {}

void LoopOptimizer::visit(LabelledStatement& node) {
    if (node.statement) node.statement->accept(*this);
}

void LoopOptimizer::visit(GotoStatement& node) {}

void LoopOptimizer::visit(SwitchContinueStatement& node) {
    if (node.target) node.target->accept(*this);
}

void LoopOptimizer::visit(RepeatStatement& node) {
    if (node.body) node.body->accept(*this);
}

void LoopOptimizer::visit(VariableDeclaration& node) {
    if (node.initializer) node.initializer->accept(*this);
}

void LoopOptimizer::visit(FunctionDeclaration& node) {
    if (node.body) node.body->accept(*this);
}

void LoopOptimizer::visit(AsmStatement& node) {}
void LoopOptimizer::visit(StaticAssert& node) {}
void LoopOptimizer::visit(StructDefinition& node) {}
void LoopOptimizer::visit(EnumDefinition& node) {}

void LoopOptimizer::visit(TranslationUnit& node) {
    for (auto& decl : node.topLevelDecls) {
        if (decl) decl->accept(*this);
    }
}
