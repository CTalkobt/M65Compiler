#include "LoopOptimizer.hpp"
#include <algorithm>
#include <memory>

namespace {
    // Helper: collect all variable names referenced in an expression
    class VarCollector : public ASTVisitor {
    public:
        std::set<std::string> vars;

        void visit(VariableReference& n) override { vars.insert(n.name); }
        void visit(ArrayAccess& n) override { n.arrayExpr->accept(*this); n.indexExpr->accept(*this); }
        void visit(MemberAccess& n) override { n.structExpr->accept(*this); }
        void visit(BinaryOperation& n) override { n.left->accept(*this); n.right->accept(*this); }
        void visit(UnaryOperation& n) override { n.operand->accept(*this); }
        void visit(ConditionalExpression& n) override {
            n.condition->accept(*this); n.thenExpr->accept(*this); n.elseExpr->accept(*this);
        }
        void visit(FunctionCall& n) override { for (auto& arg : n.arguments) arg->accept(*this); }
        void visit(CastExpression& n) override { n.expression->accept(*this); }
        void visit(SizeofExpression& n) override { if (n.expression) n.expression->accept(*this); }
        void visit(GenericSelection& n) override {
            n.control->accept(*this);
            for (auto& a : n.associations) a.result->accept(*this);
        }
        void visit(InitializerList& n) override { for (auto& e : n.elements) e->accept(*this); }
        void visit(CompoundLiteral& n) override { for (auto& e : n.initializer->elements) e->accept(*this); }
        void visit(BuiltinVaStart& n) override { n.ap->accept(*this); }
        void visit(BuiltinVaArg& n) override { n.ap->accept(*this); }

        // No-ops for non-expressions
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
        void visit(Assignment&) override {}
        void visit(ForStatement&) override {}
    };

    // Check if two expressions are structurally equal
    class ExpressionComparator : public ASTVisitor {
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

        // Simplified - just return false for other types
        void visit(Assignment&) override {}
        void visit(ConditionalExpression&) override {}
        void visit(FunctionCall&) override {}
        void visit(MemberAccess&) override {}
        void visit(SizeofExpression&) override {}
        void visit(AlignofExpression&) override {}
        void visit(GenericSelection&) override {}
        void visit(InitializerList&) override {}
        void visit(CompoundLiteral&) override {}
        void visit(BuiltinVaStart&) override {}
        void visit(BuiltinVaArg&) override {}
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
        void visit(ForStatement&) override {}
    };

    // Collect all expressions from a statement subtree
    class ExpressionCollector : public ASTVisitor {
    public:
        struct ExprInfo {
            Expression* expr;
            size_t count = 0;  // How many times it appears
            std::string exprStr;  // String representation for hashing
        };
        std::vector<ExprInfo> expressions;

        void visit(BinaryOperation& n) override {
            addExpr(&n);
            n.left->accept(*this);
            n.right->accept(*this);
        }
        void visit(UnaryOperation& n) override {
            addExpr(&n);
            n.operand->accept(*this);
        }
        void visit(ArrayAccess& n) override {
            addExpr(&n);
            n.arrayExpr->accept(*this);
            n.indexExpr->accept(*this);
        }
        void visit(CastExpression& n) override {
            addExpr(&n);
            n.expression->accept(*this);
        }
        void visit(Assignment& n) override {
            n.target->accept(*this);
            n.expression->accept(*this);
        }
        void visit(ConditionalExpression& n) override {
            n.condition->accept(*this);
            n.thenExpr->accept(*this);
            n.elseExpr->accept(*this);
        }
        void visit(FunctionCall& n) override {
            for (auto& arg : n.arguments) arg->accept(*this);
        }

        // Statement traversal
        void visit(CompoundStatement& n) override {
            for (auto& stmt : n.statements) if (stmt) stmt->accept(*this);
        }
        void visit(IfStatement& n) override {
            if (n.condition) n.condition->accept(*this);
            if (n.thenBranch) n.thenBranch->accept(*this);
            if (n.elseBranch) n.elseBranch->accept(*this);
        }
        void visit(ForStatement& n) override {
            if (n.initializer) n.initializer->accept(*this);
            if (n.condition) n.condition->accept(*this);
            if (n.increment) n.increment->accept(*this);
            if (n.body) n.body->accept(*this);
        }
        void visit(WhileStatement& n) override {
            if (n.condition) n.condition->accept(*this);
            if (n.body) n.body->accept(*this);
        }
        void visit(DoWhileStatement& n) override {
            if (n.condition) n.condition->accept(*this);
            if (n.body) n.body->accept(*this);
        }
        void visit(ExpressionStatement& n) override {
            if (n.expression) n.expression->accept(*this);
        }
        void visit(ReturnStatement& n) override {
            if (n.expression) n.expression->accept(*this);
        }
        void visit(SwitchStatement& n) override {
            if (n.expression) n.expression->accept(*this);
            if (n.body) n.body->accept(*this);
        }
        void visit(SwitchContinueStatement& n) override {
            if (n.target) n.target->accept(*this);
        }
        void visit(LabelledStatement& n) override {
            if (n.statement) n.statement->accept(*this);
        }

        // No-ops
        void visit(IntegerLiteral&) override {}
        void visit(StringLiteral&) override {}
        void visit(VariableReference&) override {}
        void visit(MemberAccess&) override {}
        void visit(SizeofExpression&) override {}
        void visit(AlignofExpression&) override {}
        void visit(GenericSelection&) override {}
        void visit(InitializerList&) override {}
        void visit(CompoundLiteral&) override {}
        void visit(BuiltinVaStart&) override {}
        void visit(BuiltinVaArg&) override {}
        void visit(CpuRegisterAccess&) override {}
        void visit(CpuFlagAccess&) override {}
        void visit(BreakStatement&) override {}
        void visit(ContinueStatement&) override {}
        void visit(CaseStatement&) override {}
        void visit(DefaultStatement&) override {}
        void visit(GotoStatement&) override {}
        void visit(RepeatStatement&) override {}
        void visit(VariableDeclaration&) override {}
        void visit(FunctionDeclaration&) override {}
        void visit(AsmStatement&) override {}
        void visit(StaticAssert&) override {}
        void visit(StructDefinition&) override {}
        void visit(EnumDefinition&) override {}
        void visit(TranslationUnit&) override {}

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
        if (exprInfo.count < 2) continue;  // Skip expressions that appear < 2 times
        if (!exprInfo.expr) continue;

        // Check if this expression is loop-invariant
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

        // Skip trivial expressions (literals, single variable refs)
        if (dynamic_cast<IntegerLiteral*>(exprInfo.expr)) continue;
        if (dynamic_cast<StringLiteral*>(exprInfo.expr)) continue;
        if (dynamic_cast<VariableReference*>(exprInfo.expr)) continue;

        hoistCount++;
        // Note: Actual AST transformation (creating temp vars, replacing occurrences)
        // would go here. This is deferred due to complexity of AST mutation.
    }

    // Log optimization opportunity (for debugging/metrics)
    if (hoistCount > 0) {
        // Potential optimization: hoist hoistCount expressions
        // This would save approximately hoistCount * (expr_size - 2) bytes
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
