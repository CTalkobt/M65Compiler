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
            if (n.condition) n.condition->accept(*this);
            if (n.thenExpr) n.thenExpr->accept(*this);
            if (n.elseExpr) n.elseExpr->accept(*this);
        }
        void visit(FunctionCall& n) override { for (auto& arg : n.arguments) arg->accept(*this); }
        void visit(CastExpression& n) override { n.expression->accept(*this); }
        void visit(SizeofExpression& n) override { if (n.expression) n.expression->accept(*this); }
        void visit(GenericSelection& n) override {
            n.control->accept(*this);
            for (auto& assoc : n.associations) assoc.result->accept(*this);
        }
        void visit(InitializerList& n) override { for (auto& e : n.elements) e->accept(*this); }
        void visit(CompoundLiteral& n) override { for (auto& e : n.initializer->elements) e->accept(*this); }
        void visit(BuiltinVaStart& n) override { n.ap->accept(*this); }
        void visit(BuiltinVaArg& n) override { n.ap->accept(*this); }

        // No-ops for non-expressions or nodes without sub-expressions
        void visit(IntegerLiteral&) override {}
        void visit(StringLiteral&) override {}
        void visit(AlignofExpression&) override {}
        void visit(CpuRegisterAccess&) override {}
        void visit(CpuFlagAccess&) override {}
        void visit(LabelAddressExpression&) override {}
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
        void visit(MemberAccess& n) override {
            if (auto* t = dynamic_cast<MemberAccess*>(target)) {
                if (n.memberName != t->memberName || n.isArrow != t->isArrow) return;
                ExpressionComparator cmp;
                cmp.compare(*n.structExpr, *t->structExpr);
                equal = cmp.equal;
            }
        }
        void visit(CastExpression& n) override {
            if (auto* t = dynamic_cast<CastExpression*>(target)) {
                if (n.targetType != t->targetType || n.pointerLevel != t->pointerLevel) return;
                ExpressionComparator cmp;
                cmp.compare(*n.expression, *t->expression);
                equal = cmp.equal;
            }
        }
        // No-ops for complex expressions
        void visit(ConditionalExpression&) override {}
        void visit(GenericSelection&) override {}
        void visit(FunctionCall&) override {}
        void visit(InitializerList&) override {}
        void visit(CompoundLiteral&) override {}
        void visit(SizeofExpression&) override {}
        void visit(AlignofExpression&) override {}
        void visit(BuiltinVaStart&) override {}
        void visit(BuiltinVaArg&) override {}
        void visit(CpuRegisterAccess&) override {}
        void visit(CpuFlagAccess&) override {}
        void visit(LabelAddressExpression&) override {}
        
        // Statements/Decls
        void visit(IfStatement&) override {}
        void visit(WhileStatement&) override {}
        void visit(DoWhileStatement&) override {}
        void visit(ForStatement&) override {}
        void visit(RepeatStatement&) override {}
        void visit(SwitchStatement&) override {}
        void visit(CaseStatement&) override {}
        void visit(DefaultStatement&) override {}
        void visit(BreakStatement&) override {}
        void visit(ContinueStatement&) override {}
        void visit(SwitchContinueStatement&) override {}
        void visit(GotoStatement&) override {}
        void visit(LabelledStatement&) override {}
        void visit(ExpressionStatement&) override {}
        void visit(ReturnStatement&) override {}
        void visit(VariableDeclaration&) override {}
        void visit(FunctionDeclaration&) override {}
        void visit(CompoundStatement&) override {}
        void visit(AsmStatement&) override {}
        void visit(StaticAssert&) override {}
        void visit(StructDefinition&) override {}
        void visit(EnumDefinition&) override {}
        void visit(TranslationUnit&) override {}
        void visit(Assignment&) override {}
    };

    // Helper: find all expressions that could be hoisted
    class ExpressionCollector : public ASTVisitor {
    public:
        std::vector<Expression*> candidates;
        std::set<std::string> mutatedVars;

        void visit(Assignment& n) override {
            if (auto* ref = dynamic_cast<VariableReference*>(n.target.get()))
                mutatedVars.insert(ref->name);
            n.expression->accept(*this);
        }
        void visit(UnaryOperation& n) override {
            if (n.op == "++" || n.op == "--") {
                 if (auto* ref = dynamic_cast<VariableReference*>(n.operand.get()))
                    mutatedVars.insert(ref->name);
            }
            n.operand->accept(*this);
        }
        void visit(BinaryOperation& n) override {
            addExpr(&n);
            n.left->accept(*this);
            n.right->accept(*this);
        }
        void visit(ArrayAccess& n) override {
            addExpr(&n);
            n.arrayExpr->accept(*this);
            n.indexExpr->accept(*this);
        }
        void visit(MemberAccess& n) override {
            addExpr(&n);
            n.structExpr->accept(*this);
        }
        void visit(CastExpression& n) override {
            addExpr(&n);
            n.expression->accept(*this);
        }
        void visit(VariableReference& n) override {
            // Variable references are candidates for hoisting if they are loop-invariant
            // (but simple ones are usually handled by register allocator)
        }
        void visit(IntegerLiteral&) override {}
        void visit(StringLiteral&) override {}

        void visit(ConditionalExpression& n) override {
            n.condition->accept(*this); if (n.thenExpr) n.thenExpr->accept(*this); n.elseExpr->accept(*this);
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
        void visit(WhileStatement& n) override {
            if (n.condition) n.condition->accept(*this);
            if (n.body) n.body->accept(*this);
        }
        void visit(DoWhileStatement& n) override {
            if (n.body) n.body->accept(*this);
            if (n.condition) n.condition->accept(*this);
        }
        void visit(ForStatement& n) override {
            if (n.initializer) n.initializer->accept(*this);
            if (n.condition) n.condition->accept(*this);
            if (n.increment) n.increment->accept(*this);
            if (n.body) n.body->accept(*this);
        }
        void visit(RepeatStatement& n) override { if (n.body) n.body->accept(*this); }
        void visit(SwitchStatement& n) override {
            n.expression->accept(*this);
            n.body->accept(*this);
        }
        void visit(CaseStatement& n) override {
            n.value->accept(*this);
            if (n.rangeEnd) n.rangeEnd->accept(*this);
        }
        void visit(SwitchContinueStatement& n) override {
            if (n.target) n.target->accept(*this);
        }
        void visit(LabelledStatement& n) override {
            if (n.statement) n.statement->accept(*this);
        }
        void visit(ExpressionStatement& n) override {
            if (n.expression) n.expression->accept(*this);
        }

        // No-ops for non-expressions
        void visit(AlignofExpression&) override {}
        void visit(GenericSelection&) override {}
        void visit(InitializerList&) override {}
        void visit(CompoundLiteral&) override {}
        void visit(BuiltinVaStart&) override {}
        void visit(BuiltinVaArg&) override {}
        void visit(CpuRegisterAccess&) override {}
        void visit(CpuFlagAccess&) override {}
        void visit(LabelAddressExpression&) override {}
        void visit(BreakStatement&) override {}
        void visit(ContinueStatement&) override {}
        void visit(DefaultStatement&) override {}
        void visit(GotoStatement&) override {}
        void visit(VariableDeclaration&) override {}
        void visit(FunctionDeclaration&) override {}
        void visit(AsmStatement&) override {}
        void visit(StaticAssert&) override {}
        void visit(StructDefinition&) override {}
        void visit(EnumDefinition&) override {}
        void visit(TranslationUnit&) override {}
        void visit(ReturnStatement& n) override { if (n.expression) n.expression->accept(*this); }
        void visit(SizeofExpression& n) override { if (n.expression) n.expression->accept(*this); }

    private:
        void addExpr(Expression* e) {
            // Check if already in candidates
            for (auto* c : candidates) {
                ExpressionComparator cmp;
                cmp.compare(*c, *e);
                if (cmp.equal) return;
            }
            candidates.push_back(e);
        }
    };
}

void LoopOptimizer::optimizeTranslationUnit(TranslationUnit& unit) {
    for (auto& decl : unit.topLevelDecls) {
        decl->accept(*this);
    }
}

void LoopOptimizer::visit(FunctionDeclaration& node) {
    if (node.body) node.body->accept(*this);
}

void LoopOptimizer::visit(CompoundStatement& node) {
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
}

void LoopOptimizer::visit(ForStatement& node) {
    // 1. Identify expressions within the loop body that are invariant
    ExpressionCollector collector;
    node.body->accept(collector);

    // 2. Filter invariant candidates: none of their referenced variables are mutated in the loop
    std::vector<Expression*> invariant;
    for (auto* e : collector.candidates) {
        VarCollector vars;
        e->accept(vars);
        bool isInvariant = true;
        for (const auto& v : vars.vars) {
            if (collector.mutatedVars.count(v)) {
                isInvariant = false;
                break;
            }
        }
        if (isInvariant) invariant.push_back(e);
    }

    // 3. Hoist invariants (TODO: implement AST transformation for hoisting)
    // For now, this visitor just identifies them.

    if (node.body) node.body->accept(*this);
}

void LoopOptimizer::visit(WhileStatement& node) {
    if (node.body) node.body->accept(*this);
}

void LoopOptimizer::visit(DoWhileStatement& node) {
    if (node.body) node.body->accept(*this);
}

void LoopOptimizer::visit(IfStatement& node) {
    if (node.thenBranch) node.thenBranch->accept(*this);
    if (node.elseBranch) node.elseBranch->accept(*this);
}

void LoopOptimizer::visit(SwitchStatement& node) {
    if (node.body) node.body->accept(*this);
}

void LoopOptimizer::visit(RepeatStatement& node) {
    if (node.body) node.body->accept(*this);
}

void LoopOptimizer::visit(LabelledStatement& node) {
    if (node.statement) node.statement->accept(*this);
}

void LoopOptimizer::visit(BreakStatement& node) {}
void LoopOptimizer::visit(ContinueStatement& node) {}
void LoopOptimizer::visit(SwitchContinueStatement& node) {}
void LoopOptimizer::visit(ReturnStatement& node) {}
void LoopOptimizer::visit(GotoStatement& node) {
    if (node.target) node.target->accept(*this);
}
void LoopOptimizer::visit(VariableDeclaration& node) {}
void LoopOptimizer::visit(ExpressionStatement& node) {}
void LoopOptimizer::visit(AsmStatement& node) {}
void LoopOptimizer::visit(StaticAssert& node) {}
void LoopOptimizer::visit(StructDefinition& node) {}
void LoopOptimizer::visit(EnumDefinition& node) {}
void LoopOptimizer::visit(TranslationUnit& node) {
    for (auto& decl : node.topLevelDecls) decl->accept(*this);
}

void LoopOptimizer::visit(CaseStatement& node) { if (node.value) node.value->accept(*this); if (node.rangeEnd) node.rangeEnd->accept(*this); }
void LoopOptimizer::visit(DefaultStatement& node) {}
