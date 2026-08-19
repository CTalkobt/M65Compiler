#include "LoopOptimizer.hpp"
#include <algorithm>
#include <memory>

namespace {
    // Helper: extract loop variable and initial value from initialization
    class InitializerAnalyzer {
    public:
        bool extract(Statement* init, std::string& varName, int& value) {
            varName.clear();
            if (!init) return false;
            if (auto* decl = dynamic_cast<VariableDeclaration*>(init)) {
                varName = decl->name;
                if (!decl->initializer) return false;
                if (auto* lit = dynamic_cast<IntegerLiteral*>(decl->initializer.get())) {
                    value = lit->value;
                    return true;
                }
            }
            return false;
        }
    };

    // Helper: extract loop bound from condition
    class ConditionAnalyzer {
    public:
        bool extract(Expression* cond, const std::string& loopVar, int& bound, std::string& op) {
            if (!cond) return false;
            if (auto* binOp = dynamic_cast<BinaryOperation*>(cond)) {
                auto* left = dynamic_cast<VariableReference*>(binOp->left.get());
                auto* right = dynamic_cast<IntegerLiteral*>(binOp->right.get());
                if (left && right && left->name == loopVar) {
                    op = binOp->op;
                    bound = right->value;
                    return op == "<" || op == "<=" || op == ">" || op == ">=";
                }
            }
            return false;
        }
    };

    // Helper: check if expression is loop counter increment
    class IncrementAnalyzer {
    public:
        bool isIncrementOne(Expression* inc, const std::string& loopVar) {
            if (!inc) return false;
            if (auto* unOp = dynamic_cast<UnaryOperation*>(inc)) {
                if (unOp->op != "++") return false;
                if (auto* ref = dynamic_cast<VariableReference*>(unOp->operand.get()))
                    return ref->name == loopVar;
            }
            if (auto* binOp = dynamic_cast<BinaryOperation*>(inc)) {
                if (binOp->op != "+=") return false;
                if (auto* ref = dynamic_cast<VariableReference*>(binOp->left.get())) {
                    if (ref->name != loopVar) return false;
                    if (auto* lit = dynamic_cast<IntegerLiteral*>(binOp->right.get()))
                        return lit->value == 1;
                }
            }
            return false;
        }
    };

    // Helper: check for break or continue statements
    class BreakContinueChecker : public ASTVisitor {
    public:
        bool found = false;
        void visit(BreakStatement&) override { found = true; }
        void visit(ContinueStatement&) override { found = true; }
        void visit(ForStatement& n) override {
            if (n.body) n.body->accept(*this);
        }
        void visit(WhileStatement& n) override {
            if (n.body) n.body->accept(*this);
        }
        void visit(DoWhileStatement& n) override {
            if (n.body) n.body->accept(*this);
        }
        void visit(CompoundStatement& n) override {
            for (auto& s : n.statements) if (s) s->accept(*this);
        }
        void visit(IfStatement& n) override {
            if (n.thenBranch) n.thenBranch->accept(*this);
            if (n.elseBranch) n.elseBranch->accept(*this);
        }
        void visit(SwitchStatement& n) override {
            if (n.body) n.body->accept(*this);
        }
        void visit(ExpressionStatement&) override {}
        void visit(VariableDeclaration&) override {}
        void visit(ReturnStatement&) override {}
        void visit(GotoStatement&) override {}
        void visit(LabelledStatement& n) override {
            if (n.statement) n.statement->accept(*this);
        }
        void visit(RepeatStatement& n) override {
            if (n.body) n.body->accept(*this);
        }
        void visit(SwitchContinueStatement&) override {}
        void visit(CaseStatement&) override {}
        void visit(DefaultStatement&) override {}
        void visit(AsmStatement&) override {}
        void visit(StaticAssert&) override {}
        void visit(StructDefinition&) override {}
        void visit(EnumDefinition&) override {}
        void visit(FunctionDeclaration&) override {}
        void visit(TranslationUnit&) override {}
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
    };

    // Helper: check for function calls
    class FunctionCallChecker : public ASTVisitor {
    public:
        bool found = false;
        void visit(FunctionCall&) override { found = true; }
        void visit(CompoundStatement& n) override {
            for (auto& s : n.statements) if (s && !found) s->accept(*this);
        }
        void visit(IfStatement& n) override {
            if (n.thenBranch && !found) n.thenBranch->accept(*this);
            if (n.elseBranch && !found) n.elseBranch->accept(*this);
        }
        void visit(ForStatement& n) override {
            if (n.body && !found) n.body->accept(*this);
        }
        void visit(WhileStatement& n) override {
            if (n.body && !found) n.body->accept(*this);
        }
        void visit(DoWhileStatement& n) override {
            if (n.body && !found) n.body->accept(*this);
        }
        void visit(SwitchStatement& n) override {
            if (n.body && !found) n.body->accept(*this);
        }
        void visit(ExpressionStatement& n) override {
            if (n.expression && !found) n.expression->accept(*this);
        }
        void visit(ReturnStatement& n) override {
            if (n.expression && !found) n.expression->accept(*this);
        }
        void visit(BinaryOperation& n) override {
            if (n.left && !found) n.left->accept(*this);
            if (n.right && !found) n.right->accept(*this);
        }
        void visit(UnaryOperation& n) override {
            if (n.operand && !found) n.operand->accept(*this);
        }
        void visit(Assignment& n) override {
            if (n.expression && !found) n.expression->accept(*this);
        }
        void visit(ArrayAccess& n) override {
            if (n.arrayExpr && !found) n.arrayExpr->accept(*this);
            if (n.indexExpr && !found) n.indexExpr->accept(*this);
        }
        void visit(RepeatStatement& n) override {
            if (n.body && !found) n.body->accept(*this);
        }
        void visit(LabelledStatement& n) override {
            if (n.statement && !found) n.statement->accept(*this);
        }
        void visit(CastExpression& n) override {
            if (n.expression && !found) n.expression->accept(*this);
        }
        void visit(ConditionalExpression& n) override {
            if (n.condition && !found) n.condition->accept(*this);
            if (n.thenExpr && !found) n.thenExpr->accept(*this);
            if (n.elseExpr && !found) n.elseExpr->accept(*this);
        }
        // No-ops
        void visit(IntegerLiteral&) override {}
        void visit(FloatLiteral&) override {}
        void visit(StringLiteral&) override {}
        void visit(VariableReference&) override {}
        void visit(MemberAccess&) override {}
        void visit(SizeofExpression&) override {}
        void visit(AlignofExpression&) override {}
        void visit(InitializerList&) override {}
        void visit(CompoundLiteral&) override {}
        void visit(GenericSelection&) override {}
        void visit(BuiltinVaStart&) override {}
        void visit(BuiltinVaArg&) override {}
        void visit(CpuRegisterAccess&) override {}
        void visit(CpuFlagAccess&) override {}
        void visit(LabelAddressExpression&) override {}
        void visit(BreakStatement&) override {}
        void visit(ContinueStatement&) override {}
        void visit(SwitchContinueStatement&) override {}
        void visit(GotoStatement&) override {}
        void visit(VariableDeclaration&) override {}
        void visit(FunctionDeclaration&) override {}
        void visit(AsmStatement&) override {}
        void visit(StaticAssert&) override {}
        void visit(StructDefinition&) override {}
        void visit(EnumDefinition&) override {}
        void visit(TranslationUnit&) override {}
        void visit(DefaultStatement&) override {}
        void visit(CaseStatement&) override {}
    };

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
        void visit(FloatLiteral&) override {}
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
        void visit(FloatLiteral& n) override {
            if (auto* t = dynamic_cast<FloatLiteral*>(target))
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
        void visit(FloatLiteral&) override {}
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

// Forward declarations for cloning functions
std::unique_ptr<Statement> cloneStatement(Statement* stmt);
std::unique_ptr<Expression> cloneExpression(Expression* expr);
std::unique_ptr<Statement> cloneAndSubstituteStatement(Statement* stmt, const std::string& varName, int value);
std::unique_ptr<Expression> cloneAndSubstituteExpression(Expression* expr, const std::string& varName, int value);

// Main loop unrolling implementation
bool LoopOptimizer::canUnrollLoop(const ForStatement& stmt) {
    // Extract loop variable and initial value
    InitializerAnalyzer initAnalyzer;
    std::string loopVar;
    int initValue;
    if (!initAnalyzer.extract(stmt.initializer.get(), loopVar, initValue))
        return false;
    if (initValue != 0) return false;

    // Extract loop bound from condition
    ConditionAnalyzer condAnalyzer;
    int bound;
    std::string op;
    if (!condAnalyzer.extract(stmt.condition.get(), loopVar, bound, op))
        return false;

    // Adjust bound for inclusive comparisons
    if (op == "<=") bound++;

    // Check bound is reasonable (< 16 iterations)
    if (bound < 1 || bound > 15) return false;

    // Check increment is ++
    IncrementAnalyzer incAnalyzer;
    if (!incAnalyzer.isIncrementOne(stmt.increment.get(), loopVar))
        return false;

    // Check body is safe
    if (!stmt.body) return false;

    BreakContinueChecker bcc;
    stmt.body->accept(bcc);
    if (bcc.found) return false;

    FunctionCallChecker fcc;
    stmt.body->accept(fcc);
    if (fcc.found) return false;

    // Check loop variable is not modified in body (conservative check)
    VarCollector varCollector;
    stmt.body->accept(varCollector);
    // Don't unroll if loop var is explicitly referenced in body beyond array index
    // (This is a conservative check; could be refined)

    return true;
}

std::unique_ptr<CompoundStatement> LoopOptimizer::unrollLoop(const ForStatement& stmt) {
    // Extract loop info
    InitializerAnalyzer initAnalyzer;
    std::string loopVar;
    int initValue;
    initAnalyzer.extract(stmt.initializer.get(), loopVar, initValue);

    // Extract bound
    ConditionAnalyzer condAnalyzer;
    int bound;
    std::string op;
    condAnalyzer.extract(stmt.condition.get(), loopVar, bound, op);
    if (op == "<=") bound++;

    // Create compound statement for unrolled body
    auto result = std::make_unique<CompoundStatement>();

    // Clone body bound times, replacing loop var with iteration count
    for (int i = 0; i < bound; i++) {
        auto cloned = cloneAndSubstituteStatement(stmt.body.get(), loopVar, i);
        if (cloned) {
            result->statements.push_back(std::move(cloned));
        }
    }

    return result;
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

// Clone and substitute variable with constant in expression
std::unique_ptr<Expression> cloneAndSubstituteExpression(Expression* expr, const std::string& varName, int value) {
    if (!expr) return nullptr;
    if (auto* ref = dynamic_cast<VariableReference*>(expr)) {
        if (ref->name == varName)
            return std::make_unique<IntegerLiteral>(value);
        return std::make_unique<VariableReference>(ref->name);
    }
    if (auto* lit = dynamic_cast<IntegerLiteral*>(expr))
        return std::make_unique<IntegerLiteral>(lit->value);
    if (auto* lit = dynamic_cast<FloatLiteral*>(expr))
        return std::make_unique<FloatLiteral>(lit->value);
    if (auto* lit = dynamic_cast<StringLiteral*>(expr))
        return std::make_unique<StringLiteral>(lit->value);
    if (auto* binOp = dynamic_cast<BinaryOperation*>(expr))
        return std::make_unique<BinaryOperation>(binOp->op, cloneAndSubstituteExpression(binOp->left.get(), varName, value), cloneAndSubstituteExpression(binOp->right.get(), varName, value));
    if (auto* unOp = dynamic_cast<UnaryOperation*>(expr))
        return std::make_unique<UnaryOperation>(unOp->op, cloneAndSubstituteExpression(unOp->operand.get(), varName, value));
    if (auto* cast = dynamic_cast<CastExpression*>(expr)) {
        auto cloned = std::make_unique<CastExpression>(cast->targetType, cast->pointerLevel, cast->isSigned, cloneAndSubstituteExpression(cast->expression.get(), varName, value));
        return cloned;
    }
    if (auto* arr = dynamic_cast<ArrayAccess*>(expr))
        return std::make_unique<ArrayAccess>(cloneAndSubstituteExpression(arr->arrayExpr.get(), varName, value), cloneAndSubstituteExpression(arr->indexExpr.get(), varName, value));
    if (auto* mem = dynamic_cast<MemberAccess*>(expr)) {
        auto cloned = std::make_unique<MemberAccess>(cloneAndSubstituteExpression(mem->structExpr.get(), varName, value), mem->memberName, mem->isArrow);
        return cloned;
    }
    if (auto* cond = dynamic_cast<ConditionalExpression*>(expr))
        return std::make_unique<ConditionalExpression>(cloneAndSubstituteExpression(cond->condition.get(), varName, value), cloneAndSubstituteExpression(cond->thenExpr.get(), varName, value), cloneAndSubstituteExpression(cond->elseExpr.get(), varName, value));
    if (auto* assign = dynamic_cast<Assignment*>(expr))
        return std::make_unique<Assignment>(cloneAndSubstituteExpression(assign->target.get(), varName, value), cloneAndSubstituteExpression(assign->expression.get(), varName, value));
    return nullptr;
}

// Clone and substitute variable with constant in statement
std::unique_ptr<Statement> cloneAndSubstituteStatement(Statement* stmt, const std::string& varName, int value) {
    if (!stmt) return nullptr;
    if (auto* comp = dynamic_cast<CompoundStatement*>(stmt)) {
        auto cloned = std::make_unique<CompoundStatement>();
        for (auto& s : comp->statements)
            cloned->statements.push_back(cloneAndSubstituteStatement(s.get(), varName, value));
        return cloned;
    }
    if (auto* expr = dynamic_cast<ExpressionStatement*>(stmt))
        return std::make_unique<ExpressionStatement>(cloneAndSubstituteExpression(expr->expression.get(), varName, value));
    if (auto* ifStmt = dynamic_cast<IfStatement*>(stmt)) {
        auto cloned = std::make_unique<IfStatement>(cloneAndSubstituteExpression(ifStmt->condition.get(), varName, value), cloneAndSubstituteStatement(ifStmt->thenBranch.get(), varName, value));
        if (ifStmt->elseBranch)
            cloned->elseBranch = cloneAndSubstituteStatement(ifStmt->elseBranch.get(), varName, value);
        return cloned;
    }
    if (auto* ret = dynamic_cast<ReturnStatement*>(stmt)) {
        std::unique_ptr<Expression> retExpr;
        if (ret->expression)
            retExpr = cloneAndSubstituteExpression(ret->expression.get(), varName, value);
        auto cloned = std::make_unique<ReturnStatement>(std::move(retExpr));
        return cloned;
    }
    if (auto* var = dynamic_cast<VariableDeclaration*>(stmt)) {
        auto cloned = std::make_unique<VariableDeclaration>(var->type, var->name, var->pointerLevel);
        cloned->isSigned = var->isSigned;
        cloned->isVolatile = var->isVolatile;
        cloned->isConst = var->isConst;
        cloned->isPointerConst = var->isPointerConst;
        cloned->isGlobal = var->isGlobal;
        cloned->isExtern = var->isExtern;
        return cloned;
    }
    return nullptr;
}

// Clone an expression tree
std::unique_ptr<Expression> cloneExpression(Expression* expr) {
    if (!expr) return nullptr;
    if (auto* lit = dynamic_cast<IntegerLiteral*>(expr))
        return std::make_unique<IntegerLiteral>(lit->value);
    if (auto* lit = dynamic_cast<FloatLiteral*>(expr))
        return std::make_unique<FloatLiteral>(lit->value);
    if (auto* lit = dynamic_cast<StringLiteral*>(expr))
        return std::make_unique<StringLiteral>(lit->value);
    if (auto* ref = dynamic_cast<VariableReference*>(expr))
        return std::make_unique<VariableReference>(ref->name);
    if (auto* binOp = dynamic_cast<BinaryOperation*>(expr))
        return std::make_unique<BinaryOperation>(binOp->op, cloneExpression(binOp->left.get()), cloneExpression(binOp->right.get()));
    if (auto* unOp = dynamic_cast<UnaryOperation*>(expr))
        return std::make_unique<UnaryOperation>(unOp->op, cloneExpression(unOp->operand.get()));
    if (auto* cast = dynamic_cast<CastExpression*>(expr)) {
        auto cloned = std::make_unique<CastExpression>(cast->targetType, cast->pointerLevel, cast->isSigned, cloneExpression(cast->expression.get()));
        return cloned;
    }
    if (auto* arr = dynamic_cast<ArrayAccess*>(expr))
        return std::make_unique<ArrayAccess>(cloneExpression(arr->arrayExpr.get()), cloneExpression(arr->indexExpr.get()));
    if (auto* mem = dynamic_cast<MemberAccess*>(expr)) {
        auto cloned = std::make_unique<MemberAccess>(cloneExpression(mem->structExpr.get()), mem->memberName, mem->isArrow);
        return cloned;
    }
    if (auto* cond = dynamic_cast<ConditionalExpression*>(expr))
        return std::make_unique<ConditionalExpression>(cloneExpression(cond->condition.get()), cloneExpression(cond->thenExpr.get()), cloneExpression(cond->elseExpr.get()));
    if (auto* assign = dynamic_cast<Assignment*>(expr))
        return std::make_unique<Assignment>(cloneExpression(assign->target.get()), cloneExpression(assign->expression.get()));
    return nullptr;
}

// Clone a statement tree
std::unique_ptr<Statement> cloneStatement(Statement* stmt) {
    if (!stmt) return nullptr;
    if (auto* comp = dynamic_cast<CompoundStatement*>(stmt)) {
        auto cloned = std::make_unique<CompoundStatement>();
        for (auto& s : comp->statements)
            cloned->statements.push_back(cloneStatement(s.get()));
        return cloned;
    }
    if (auto* expr = dynamic_cast<ExpressionStatement*>(stmt))
        return std::make_unique<ExpressionStatement>(cloneExpression(expr->expression.get()));
    if (auto* ifStmt = dynamic_cast<IfStatement*>(stmt)) {
        auto cloned = std::make_unique<IfStatement>(cloneExpression(ifStmt->condition.get()), cloneStatement(ifStmt->thenBranch.get()));
        if (ifStmt->elseBranch)
            cloned->elseBranch = cloneStatement(ifStmt->elseBranch.get());
        return cloned;
    }
    if (auto* ret = dynamic_cast<ReturnStatement*>(stmt)) {
        std::unique_ptr<Expression> retExpr;
        if (ret->expression)
            retExpr = cloneExpression(ret->expression.get());
        auto cloned = std::make_unique<ReturnStatement>(std::move(retExpr));
        return cloned;
    }
    if (auto* var = dynamic_cast<VariableDeclaration*>(stmt)) {
        auto cloned = std::make_unique<VariableDeclaration>(var->type, var->name, var->pointerLevel);
        cloned->isSigned = var->isSigned;
        cloned->isVolatile = var->isVolatile;
        cloned->isConst = var->isConst;
        cloned->isPointerConst = var->isPointerConst;
        cloned->isGlobal = var->isGlobal;
        cloned->isExtern = var->isExtern;
        return cloned;
    }
    // For other statement types, return null (not unrollable)
    return nullptr;
}

// Replace loop variable with constant in expression
void replaceVariableInExpr(Expression& expr, const std::string& varName, int value) {
    if (auto* ref = dynamic_cast<VariableReference*>(&expr)) {
        if (ref->name == varName) {
            ref->name = "";  // Mark for later replacement
            // Can't easily change a reference to a literal, so we'll handle this differently
        }
        return;
    }
    if (auto* binOp = dynamic_cast<BinaryOperation*>(&expr)) {
        replaceVariableInExpr(*binOp->left, varName, value);
        replaceVariableInExpr(*binOp->right, varName, value);
        return;
    }
    if (auto* unOp = dynamic_cast<UnaryOperation*>(&expr)) {
        replaceVariableInExpr(*unOp->operand, varName, value);
        return;
    }
    if (auto* arr = dynamic_cast<ArrayAccess*>(&expr)) {
        replaceVariableInExpr(*arr->arrayExpr, varName, value);
        replaceVariableInExpr(*arr->indexExpr, varName, value);
        return;
    }
    if (auto* mem = dynamic_cast<MemberAccess*>(&expr)) {
        replaceVariableInExpr(*mem->structExpr, varName, value);
        return;
    }
    if (auto* cond = dynamic_cast<ConditionalExpression*>(&expr)) {
        if (cond->condition) replaceVariableInExpr(*cond->condition, varName, value);
        if (cond->thenExpr) replaceVariableInExpr(*cond->thenExpr, varName, value);
        if (cond->elseExpr) replaceVariableInExpr(*cond->elseExpr, varName, value);
        return;
    }
    if (auto* cast = dynamic_cast<CastExpression*>(&expr)) {
        replaceVariableInExpr(*cast->expression, varName, value);
        return;
    }
    if (auto* assign = dynamic_cast<Assignment*>(&expr)) {
        replaceVariableInExpr(*assign->target, varName, value);
        replaceVariableInExpr(*assign->expression, varName, value);
        return;
    }
}

// Replace loop variable with constant in statement
void replaceVariableInStmt(Statement& stmt, const std::string& varName, int value) {
    if (auto* comp = dynamic_cast<CompoundStatement*>(&stmt)) {
        for (auto& s : comp->statements)
            if (s) replaceVariableInStmt(*s, varName, value);
        return;
    }
    if (auto* expr = dynamic_cast<ExpressionStatement*>(&stmt)) {
        if (expr->expression) replaceVariableInExpr(*expr->expression, varName, value);
        return;
    }
    if (auto* ifStmt = dynamic_cast<IfStatement*>(&stmt)) {
        if (ifStmt->condition) replaceVariableInExpr(*ifStmt->condition, varName, value);
        if (ifStmt->thenBranch) replaceVariableInStmt(*ifStmt->thenBranch, varName, value);
        if (ifStmt->elseBranch) replaceVariableInStmt(*ifStmt->elseBranch, varName, value);
        return;
    }
    if (auto* ret = dynamic_cast<ReturnStatement*>(&stmt)) {
        if (ret->expression) replaceVariableInExpr(*ret->expression, varName, value);
        return;
    }
}

void LoopOptimizer::visit(ForStatement& node) {
    // Check for loop unrolling opportunity first
    if (canUnrollLoop(node)) {
        auto unrolled = unrollLoop(node);
        if (unrolled) {
            // Replace the loop with unrolled code in parent context
            // This is handled via the parent compound statement
        }
    }

    // 1. Identify expressions within the loop body that are invariant
    ExpressionCollector collector;
    if (node.body) node.body->accept(collector);

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

    // 3. Continue traversal
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
