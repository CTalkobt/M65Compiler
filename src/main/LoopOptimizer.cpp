#include "LoopOptimizer.hpp"
#include <algorithm>
#include <memory>
#include <iostream>

// Forward declarations
std::unique_ptr<Expression> cloneExpression(Expression* expr);

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

    // Helper: extract bound expression (for memcpy/memset with variable bounds)
    class BoundExpressionExtractor {
    public:
        bool extract(Expression* cond, const std::string& loopVar, std::unique_ptr<Expression>& boundExpr, std::string& op) {
            if (!cond) return false;
            if (auto* binOp = dynamic_cast<BinaryOperation*>(cond)) {
                auto* left = dynamic_cast<VariableReference*>(binOp->left.get());
                if (left && left->name == loopVar) {
                    op = binOp->op;
                    if (op == "<" || op == "<=" || op == ">" || op == ">=") {
                        // Clone the right side as the bound expression
                        boundExpr = cloneExpression(binOp->right.get());
                        return boundExpr != nullptr;
                    }
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
                // Handle both prefix (++) and postfix (++_POST)
                if (unOp->op != "++" && unOp->op != "++_POST") return false;
                if (auto* ref = dynamic_cast<VariableReference*>(unOp->operand.get()))
                    return ref->name == loopVar;
                return false;
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

    // Helper: detect memcpy pattern: for (int i=0; i<n; i++) dest[i] = src[i];
    class MemcpyPatternDetector {
    public:
        bool detect(const ForStatement& loop, std::string& destVar, std::string& srcVar, std::string& indexVar) {
            // Check if body is a single assignment to arr[i] = arr2[i]
            if (!loop.body) return false;

            ExpressionStatement* exprStmt = nullptr;

            // Handle CompoundStatement with single statement
            if (auto* compStmt = dynamic_cast<CompoundStatement*>(loop.body.get())) {
                if (compStmt->statements.size() == 1 && compStmt->statements[0]) {
                    exprStmt = dynamic_cast<ExpressionStatement*>(compStmt->statements[0].get());
                }
            } else {
                // Direct ExpressionStatement
                exprStmt = dynamic_cast<ExpressionStatement*>(loop.body.get());
            }

            if (!exprStmt || !exprStmt->expression) return false;

            auto* assign = dynamic_cast<Assignment*>(exprStmt->expression.get());
            if (!assign) return false;

            // LHS must be array access: dest[i]
            auto* lhsArray = dynamic_cast<ArrayAccess*>(assign->target.get());
            if (!lhsArray) return false;

            // Array should be variable reference
            auto* destRef = dynamic_cast<VariableReference*>(lhsArray->arrayExpr.get());
            if (!destRef) return false;
            destVar = destRef->name;

            // Index should be variable reference (the loop counter)
            auto* lhsIndex = dynamic_cast<VariableReference*>(lhsArray->indexExpr.get());
            if (!lhsIndex) return false;
            indexVar = lhsIndex->name;

            // RHS must be array access: src[i]
            auto* rhsArray = dynamic_cast<ArrayAccess*>(assign->expression.get());
            if (!rhsArray) return false;

            // Source array should be variable reference
            auto* srcRef = dynamic_cast<VariableReference*>(rhsArray->arrayExpr.get());
            if (!srcRef) return false;
            srcVar = srcRef->name;

            // Index should match dest's index
            auto* rhsIndex = dynamic_cast<VariableReference*>(rhsArray->indexExpr.get());
            if (!rhsIndex || rhsIndex->name != lhsIndex->name) return false;

            return destVar != srcVar;  // dest and src must be different
        }
    };

    // Helper: detect memset pattern: for (int i=0; i<n; i++) arr[i] = constant;
    class MemsetPatternDetector {
    public:
        bool detect(const ForStatement& loop, std::string& arrVar, std::string& indexVar, int& value) {
            // Check if body is a single assignment to arr[i] = const
            if (!loop.body) return false;

            ExpressionStatement* exprStmt = nullptr;

            // Handle CompoundStatement with single statement
            if (auto* compStmt = dynamic_cast<CompoundStatement*>(loop.body.get())) {
                if (compStmt->statements.size() == 1 && compStmt->statements[0]) {
                    exprStmt = dynamic_cast<ExpressionStatement*>(compStmt->statements[0].get());
                }
            } else {
                // Direct ExpressionStatement
                exprStmt = dynamic_cast<ExpressionStatement*>(loop.body.get());
            }

            if (!exprStmt || !exprStmt->expression) return false;

            auto* assign = dynamic_cast<Assignment*>(exprStmt->expression.get());
            if (!assign) return false;

            // LHS must be array access: arr[i]
            auto* lhsArray = dynamic_cast<ArrayAccess*>(assign->target.get());
            if (!lhsArray) return false;

            // Array should be variable reference
            auto* arrRef = dynamic_cast<VariableReference*>(lhsArray->arrayExpr.get());
            if (!arrRef) return false;
            arrVar = arrRef->name;

            // Index should be variable reference (the loop counter)
            auto* lhsIndex = dynamic_cast<VariableReference*>(lhsArray->indexExpr.get());
            if (!lhsIndex) return false;
            indexVar = lhsIndex->name;

            // RHS must be integer literal
            auto* lit = dynamic_cast<IntegerLiteral*>(assign->expression.get());
            if (!lit) return false;

            value = lit->value;
            return true;
        }
    };

    // Helper: detect sum reduction pattern: int sum = 0; for(i=0; i<n; i++) sum += arr[i];
    class SumReductionDetector {
    public:
        bool detect(const ForStatement& loop, std::string& accumVar, std::string& arrayVar, std::string& indexVar) {
            // Check if body is a single statement
            if (!loop.body) return false;

            ExpressionStatement* exprStmt = nullptr;

            // Handle CompoundStatement with single statement
            if (auto* compStmt = dynamic_cast<CompoundStatement*>(loop.body.get())) {
                if (compStmt->statements.size() == 1 && compStmt->statements[0]) {
                    exprStmt = dynamic_cast<ExpressionStatement*>(compStmt->statements[0].get());
                }
            } else {
                // Direct ExpressionStatement
                exprStmt = dynamic_cast<ExpressionStatement*>(loop.body.get());
            }

            if (!exprStmt || !exprStmt->expression) return false;

            // Must be an Assignment with += operator
            auto* assign = dynamic_cast<Assignment*>(exprStmt->expression.get());
            if (!assign || assign->op != "+=") return false;

            // LHS must be a variable reference (the accumulator)
            auto* accumRef = dynamic_cast<VariableReference*>(assign->target.get());
            if (!accumRef) return false;
            accumVar = accumRef->name;

            // RHS must be an array access: arr[i]
            auto* rhsArray = dynamic_cast<ArrayAccess*>(assign->expression.get());
            if (!rhsArray) return false;

            // Array should be variable reference
            auto* arrRef = dynamic_cast<VariableReference*>(rhsArray->arrayExpr.get());
            if (!arrRef) return false;
            arrayVar = arrRef->name;

            // Index should be variable reference (the loop counter)
            auto* rhsIndex = dynamic_cast<VariableReference*>(rhsArray->indexExpr.get());
            if (!rhsIndex) return false;
            indexVar = rhsIndex->name;

            // Accumulator and array must be different variables (no aliasing)
            return accumVar != arrayVar;
        }
    };

    // Helper: detect linear search pattern: for(i=0; i<n; i++) if(arr[i]==target){found=1;break;}
    class SearchLoopDetector {
    public:
        bool detect(const ForStatement& loop, std::string& arrayVar, std::string& targetVar,
                    std::string& indexVar, std::string& resultVar) {
            // Check if body is a single statement or compound with single statement
            if (!loop.body) return false;

            IfStatement* ifStmt = nullptr;

            // Handle CompoundStatement with single if-statement
            if (auto* compStmt = dynamic_cast<CompoundStatement*>(loop.body.get())) {
                if (compStmt->statements.size() == 1 && compStmt->statements[0]) {
                    ifStmt = dynamic_cast<IfStatement*>(compStmt->statements[0].get());
                }
            } else {
                // Direct IfStatement
                ifStmt = dynamic_cast<IfStatement*>(loop.body.get());
            }

            if (!ifStmt || !ifStmt->condition || !ifStmt->thenBranch) return false;

            // Condition must be: arr[i] == target
            auto* binOp = dynamic_cast<BinaryOperation*>(ifStmt->condition.get());
            if (!binOp || binOp->op != "==") return false;

            // One side must be array access, other side must be variable
            ArrayAccess* arrayAccess = nullptr;
            VariableReference* targetRef = nullptr;

            if (auto* lhsArray = dynamic_cast<ArrayAccess*>(binOp->left.get())) {
                targetRef = dynamic_cast<VariableReference*>(binOp->right.get());
                if (targetRef) arrayAccess = lhsArray;
            } else if (auto* rhsArray = dynamic_cast<ArrayAccess*>(binOp->right.get())) {
                targetRef = dynamic_cast<VariableReference*>(binOp->left.get());
                if (targetRef) arrayAccess = rhsArray;
            }

            if (!arrayAccess || !targetRef) return false;

            // Array access must be arr[i] where arr is a variable reference and i is the loop variable
            auto* arrRef = dynamic_cast<VariableReference*>(arrayAccess->arrayExpr.get());
            auto* idxRef = dynamic_cast<VariableReference*>(arrayAccess->indexExpr.get());
            if (!arrRef || !idxRef) return false;

            arrayVar = arrRef->name;
            targetVar = targetRef->name;
            indexVar = idxRef->name;

            // Then-branch must contain: result = <expr>; break;
            ExpressionStatement* exprStmt = nullptr;
            BreakStatement* breakStmt = nullptr;

            if (auto* compStmt = dynamic_cast<CompoundStatement*>(ifStmt->thenBranch.get())) {
                if (compStmt->statements.size() == 2) {
                    exprStmt = dynamic_cast<ExpressionStatement*>(compStmt->statements[0].get());
                    breakStmt = dynamic_cast<BreakStatement*>(compStmt->statements[1].get());
                } else if (compStmt->statements.size() == 1) {
                    // Just assignment or just break — not sufficient pattern
                    return false;
                }
            } else if (auto* bare = dynamic_cast<BreakStatement*>(ifStmt->thenBranch.get())) {
                // Just break, no assignment — not what we're looking for
                return false;
            } else if (auto* bare = dynamic_cast<ExpressionStatement*>(ifStmt->thenBranch.get())) {
                // Just assignment, no break
                return false;
            }

            if (!exprStmt || !breakStmt) return false;

            // Assignment must be: result = <expr> (typically result = index or result = 1)
            auto* assign = dynamic_cast<Assignment*>(exprStmt->expression.get());
            if (!assign || assign->op != "=") return false;

            auto* resultRef = dynamic_cast<VariableReference*>(assign->target.get());
            if (!resultRef) return false;
            resultVar = resultRef->name;

            // RHS can be a literal or a variable (typically the loop index)
            // We accept: integer literals (1, true, etc.) or variable references (i)
            if (!dynamic_cast<IntegerLiteral*>(assign->expression.get()) &&
                !dynamic_cast<VariableReference*>(assign->expression.get())) {
                return false;
            }

            // No else-branch (search returns on first match)
            return !ifStmt->elseBranch && (arrayVar != targetVar);
        }
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
    if (initValue != 0)
        return false;

    // Extract loop bound from condition
    ConditionAnalyzer condAnalyzer;
    int bound;
    std::string op;
    if (!condAnalyzer.extract(stmt.condition.get(), loopVar, bound, op))
        return false;

    // Adjust bound for inclusive comparisons
    if (op == "<=") bound++;

    // Check bound is reasonable (< 16 iterations)
    if (bound < 1 || bound > 15)
        return false;

    // Check increment is ++
    IncrementAnalyzer incAnalyzer;
    if (!incAnalyzer.isIncrementOne(stmt.increment.get(), loopVar))
        return false;

    // Check body is safe
    if (!stmt.body)
        return false;

    BreakContinueChecker bcc;
    stmt.body->accept(bcc);
    if (bcc.found)
        return false;

    FunctionCallChecker fcc;
    stmt.body->accept(fcc);
    if (fcc.found)
        return false;

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

bool LoopOptimizer::isMemcpyPattern(const ForStatement& stmt, std::string& dest, std::string& src, std::string& idx) {
    MemcpyPatternDetector detector;
    return detector.detect(stmt, dest, src, idx);
}

bool LoopOptimizer::isMemsetPattern(const ForStatement& stmt, std::string& arr, std::string& idx, int& value) {
    MemsetPatternDetector detector;
    return detector.detect(stmt, arr, idx, value);
}

std::unique_ptr<Statement> LoopOptimizer::transformMemcpyToCall(const ForStatement& stmt, const std::string& dest, const std::string& src) {
    // Extract loop variable
    InitializerAnalyzer initAnalyzer;
    std::string loopVar;
    int initValue;
    if (!initAnalyzer.extract(stmt.initializer.get(), loopVar, initValue)) {
        return nullptr;  // Failed to extract loop variable
    }

    // Extract loop bound expression from condition
    BoundExpressionExtractor boundExtractor;
    std::unique_ptr<Expression> boundExpr;
    std::string op;
    if (!boundExtractor.extract(stmt.condition.get(), loopVar, boundExpr, op) || !boundExpr) {
        return nullptr;  // Failed to extract bound expression
    }

    // Create memcpy function call: memcpy(dest, src, bound)
    auto memcpyCall = std::make_unique<FunctionCall>("memcpy");
    memcpyCall->arguments.push_back(std::make_unique<VariableReference>(dest));
    memcpyCall->arguments.push_back(std::make_unique<VariableReference>(src));
    memcpyCall->arguments.push_back(std::move(boundExpr));

    auto exprStmt = std::make_unique<ExpressionStatement>(std::move(memcpyCall));
    return exprStmt;
}

std::unique_ptr<Statement> LoopOptimizer::transformMemsetToCall(const ForStatement& stmt, const std::string& arr, int value) {
    // Extract loop variable
    InitializerAnalyzer initAnalyzer;
    std::string loopVar;
    int initValue;
    if (!initAnalyzer.extract(stmt.initializer.get(), loopVar, initValue)) {
        return nullptr;  // Failed to extract loop variable
    }

    // Extract loop bound expression from condition
    BoundExpressionExtractor boundExtractor;
    std::unique_ptr<Expression> boundExpr;
    std::string op;
    if (!boundExtractor.extract(stmt.condition.get(), loopVar, boundExpr, op) || !boundExpr) {
        return nullptr;  // Failed to extract bound expression
    }

    // Create memset function call: memset(arr, value, bound)
    auto memsetCall = std::make_unique<FunctionCall>("memset");
    memsetCall->arguments.push_back(std::make_unique<VariableReference>(arr));
    memsetCall->arguments.push_back(std::make_unique<IntegerLiteral>(value));
    memsetCall->arguments.push_back(std::move(boundExpr));

    auto exprStmt = std::make_unique<ExpressionStatement>(std::move(memsetCall));
    return exprStmt;
}

bool LoopOptimizer::isSumReductionPattern(const ForStatement& stmt, std::string& accum, std::string& array, std::string& idx) {
    SumReductionDetector detector;
    return detector.detect(stmt, accum, array, idx);
}

std::unique_ptr<Statement> LoopOptimizer::transformSumReductionToCall(const ForStatement& stmt, const std::string& accum, const std::string& array) {
    // Extract loop variable
    InitializerAnalyzer initAnalyzer;
    std::string loopVar;
    int initValue;
    if (!initAnalyzer.extract(stmt.initializer.get(), loopVar, initValue)) {
        return nullptr;  // Failed to extract loop variable
    }

    // Extract loop bound expression from condition
    BoundExpressionExtractor boundExtractor;
    std::unique_ptr<Expression> boundExpr;
    std::string op;
    if (!boundExtractor.extract(stmt.condition.get(), loopVar, boundExpr, op) || !boundExpr) {
        return nullptr;  // Failed to extract bound expression
    }

    // Create function call to __idiom_sum16(array, bound)
    // The return value is assigned back to the accumulator
    auto idiomCall = std::make_unique<FunctionCall>("__idiom_sum16");
    idiomCall->arguments.push_back(std::make_unique<VariableReference>(array));
    idiomCall->arguments.push_back(std::move(boundExpr));

    // Create assignment: accum = __idiom_sum16(array, bound)
    auto assign = std::make_unique<Assignment>(
        std::make_unique<VariableReference>(accum),
        std::move(idiomCall)
    );
    assign->op = "=";

    auto exprStmt = std::make_unique<ExpressionStatement>(std::move(assign));
    return exprStmt;
}

bool LoopOptimizer::isSearchLoopPattern(const ForStatement& stmt, std::string& array, std::string& target, std::string& idx, std::string& result) {
    SearchLoopDetector detector;
    return detector.detect(stmt, array, target, idx, result);
}

std::unique_ptr<Statement> LoopOptimizer::transformSearchLoopToCall(const ForStatement& stmt, const std::string& array, const std::string& target, const std::string& result) {
    // Extract loop variable
    InitializerAnalyzer initAnalyzer;
    std::string loopVar;
    int initValue;
    if (!initAnalyzer.extract(stmt.initializer.get(), loopVar, initValue)) {
        return nullptr;  // Failed to extract loop variable
    }

    // Extract loop bound expression from condition
    BoundExpressionExtractor boundExtractor;
    std::unique_ptr<Expression> boundExpr;
    std::string op;
    if (!boundExtractor.extract(stmt.condition.get(), loopVar, boundExpr, op) || !boundExpr) {
        return nullptr;  // Failed to extract bound expression
    }

    // Create function call to __idiom_find8(array, bound, target)
    // Returns index (or -1 if not found), assign to result
    auto idiomCall = std::make_unique<FunctionCall>("__idiom_find8");
    idiomCall->arguments.push_back(std::make_unique<VariableReference>(array));
    idiomCall->arguments.push_back(std::move(boundExpr));
    idiomCall->arguments.push_back(std::make_unique<VariableReference>(target));

    // Create assignment: result = __idiom_find8(array, bound, target)
    auto assign = std::make_unique<Assignment>(
        std::make_unique<VariableReference>(result),
        std::move(idiomCall)
    );
    assign->op = "=";

    auto exprStmt = std::make_unique<ExpressionStatement>(std::move(assign));
    return exprStmt;
}

bool LoopOptimizer::canPartialUnrollLoop(const ForStatement& stmt, int unrollFactor) {
    if (unrollFactor <= 0 || unrollFactor > 16) return false;

    // Extract loop variable and initial value
    InitializerAnalyzer initAnalyzer;
    std::string loopVar;
    int initValue;
    if (!initAnalyzer.extract(stmt.initializer.get(), loopVar, initValue))
        return false;
    if (initValue != 0)
        return false;

    // Extract loop bound from condition
    ConditionAnalyzer condAnalyzer;
    int bound;
    std::string op;
    if (!condAnalyzer.extract(stmt.condition.get(), loopVar, bound, op))
        return false;

    // Adjust bound for inclusive comparisons
    if (op == "<=") bound++;

    // Check bound is in partial unroll range (20-1000 iterations)
    if (bound < 20 || bound > 1000)
        return false;

    // Check increment is ++
    IncrementAnalyzer incAnalyzer;
    if (!incAnalyzer.isIncrementOne(stmt.increment.get(), loopVar))
        return false;

    // Check body is safe
    if (!stmt.body)
        return false;

    BreakContinueChecker bcc;
    stmt.body->accept(bcc);
    if (bcc.found)
        return false;

    FunctionCallChecker fcc;
    stmt.body->accept(fcc);
    if (fcc.found)
        return false;

    return true;
}

std::unique_ptr<ForStatement> LoopOptimizer::partialUnrollLoop(
    const ForStatement& stmt, int unrollFactor) {

    // Extract loop info
    InitializerAnalyzer initAnalyzer;
    std::string loopVar;
    int initValue;
    initAnalyzer.extract(stmt.initializer.get(), loopVar, initValue);

    // Create modified loop: i += factor (instead of i++)
    auto newIncrement = std::make_unique<BinaryOperation>(
        "+=",
        std::make_unique<VariableReference>(loopVar),
        std::make_unique<IntegerLiteral>(unrollFactor)
    );

    // Create unrolled body with factor copies of original body
    auto unrolledBody = std::make_unique<CompoundStatement>();

    // Add factor copies of the body with substituted loop variable
    for (int i = 0; i < unrollFactor; ++i) {
        if (!stmt.body) continue;

        // Clone body and substitute (i+offset) for each loop var reference
        auto bodyClone = cloneStatement(stmt.body.get());
        if (bodyClone) {
            unrolledBody->statements.push_back(std::move(bodyClone));
        }
    }

    // Create new loop: for (i = 0; i < bound; i += factor) { body x factor }
    auto newLoop = std::make_unique<ForStatement>(
        cloneStatement(stmt.initializer.get()),
        cloneExpression(stmt.condition.get()),
        std::move(newIncrement),
        std::move(unrolledBody)
    );

    return newLoop;
}

void LoopOptimizer::optimizeTranslationUnit(TranslationUnit& unit) {
    for (auto& decl : unit.topLevelDecls) {
        decl->accept(*this);
    }
}

void LoopOptimizer::visit(FunctionDeclaration& node) {
    auto prevFunc = currentFunc_;
    currentFunc_ = &node;
    if (node.body) node.body->accept(*this);
    currentFunc_ = prevFunc;
}

void LoopOptimizer::visit(CompoundStatement& node) {
    // Process statements in-place, unrolling loops where possible
    std::vector<std::unique_ptr<Statement>> newStatements;
    newStatements.reserve(node.statements.size() * 2);  // Reserve extra space for unrolled statements

    for (auto& stmt : node.statements) {
        if (!stmt) {
            newStatements.push_back(nullptr);
            continue;
        }

        // Check if this is an unrollable for loop
        ForStatement* forStmt = dynamic_cast<ForStatement*>(stmt.get());
        if (forStmt && canUnrollLoop(*forStmt)) {
            // Unroll the loop and add all unrolled statements
            auto unrolled = unrollLoop(*forStmt);
            if (unrolled) {
                for (auto& unrolledStmt : unrolled->statements) {
                    if (unrolledStmt) {
                        // Recursively visit each unrolled statement
                        unrolledStmt->accept(*this);
                        newStatements.push_back(std::move(unrolledStmt));
                    }
                }
                stmt.reset();  // Release the original for statement
                continue;
            }
        }

        // Check for memcpy pattern transformation
        if (forStmt) {
            std::string dest, src, idx;
            if (isMemcpyPattern(*forStmt, dest, src, idx)) {
                auto memcpyStmt = transformMemcpyToCall(*forStmt, dest, src);
                if (memcpyStmt) {
                    memcpyStmt->accept(*this);
                    newStatements.push_back(std::move(memcpyStmt));
                    stmt.reset();
                    continue;
                }
            }

            // Check for memset pattern transformation
            std::string arr;
            int value;
            if (isMemsetPattern(*forStmt, arr, idx, value)) {
                auto memsetStmt = transformMemsetToCall(*forStmt, arr, value);
                if (memsetStmt) {
                    memsetStmt->accept(*this);
                    newStatements.push_back(std::move(memsetStmt));
                    stmt.reset();
                    continue;
                }
            }

            // Check for sum reduction pattern transformation
            std::string accum, array;
            if (isSumReductionPattern(*forStmt, accum, array, idx)) {
                auto sumStmt = transformSumReductionToCall(*forStmt, accum, array);
                if (sumStmt) {
                    sumStmt->accept(*this);
                    newStatements.push_back(std::move(sumStmt));
                    stmt.reset();
                    continue;
                }
            }

            // Check for search loop pattern transformation
            std::string searchArray, searchTarget, searchResult;
            if (isSearchLoopPattern(*forStmt, searchArray, searchTarget, idx, searchResult)) {
                auto searchStmt = transformSearchLoopToCall(*forStmt, searchArray, searchTarget, searchResult);
                if (searchStmt) {
                    searchStmt->accept(*this);
                    newStatements.push_back(std::move(searchStmt));
                    stmt.reset();
                    continue;
                }
            }
        }

        // Not optimizable or not a for loop - process normally
        stmt->accept(*this);
        newStatements.push_back(std::move(stmt));
    }

    // Replace the statement list with the processed one
    node.statements = std::move(newStatements);
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
    // Handle nested for loops: clone with variable substitution applied
    if (auto* forStmt = dynamic_cast<ForStatement*>(stmt)) {
        auto cloned = std::make_unique<ForStatement>(
            cloneAndSubstituteStatement(forStmt->initializer.get(), varName, value),
            cloneAndSubstituteExpression(forStmt->condition.get(), varName, value),
            cloneAndSubstituteExpression(forStmt->increment.get(), varName, value),
            cloneAndSubstituteStatement(forStmt->body.get(), varName, value)
        );
        return cloned;
    }
    // Handle other loops similarly with variable substitution
    if (auto* whileStmt = dynamic_cast<WhileStatement*>(stmt)) {
        auto cloned = std::make_unique<WhileStatement>(
            cloneAndSubstituteExpression(whileStmt->condition.get(), varName, value),
            cloneAndSubstituteStatement(whileStmt->body.get(), varName, value)
        );
        return cloned;
    }
    if (auto* doWhileStmt = dynamic_cast<DoWhileStatement*>(stmt)) {
        auto cloned = std::make_unique<DoWhileStatement>(
            cloneAndSubstituteStatement(doWhileStmt->body.get(), varName, value),
            cloneAndSubstituteExpression(doWhileStmt->condition.get(), varName, value)
        );
        return cloned;
    }
    if (auto* asmStmt = dynamic_cast<AsmStatement*>(stmt))
        return std::make_unique<AsmStatement>(asmStmt->code);
    if (auto* breakStmt = dynamic_cast<BreakStatement*>(stmt))
        return std::make_unique<BreakStatement>();
    if (auto* continueStmt = dynamic_cast<ContinueStatement*>(stmt))
        return std::make_unique<ContinueStatement>();
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
    if (auto* forStmt = dynamic_cast<ForStatement*>(stmt)) {
        auto cloned = std::make_unique<ForStatement>(
            cloneStatement(forStmt->initializer.get()),
            cloneExpression(forStmt->condition.get()),
            cloneExpression(forStmt->increment.get()),
            cloneStatement(forStmt->body.get())
        );
        return cloned;
    }
    if (auto* whileStmt = dynamic_cast<WhileStatement*>(stmt)) {
        auto cloned = std::make_unique<WhileStatement>(
            cloneExpression(whileStmt->condition.get()),
            cloneStatement(whileStmt->body.get())
        );
        return cloned;
    }
    if (auto* doWhileStmt = dynamic_cast<DoWhileStatement*>(stmt)) {
        auto cloned = std::make_unique<DoWhileStatement>(
            cloneStatement(doWhileStmt->body.get()),
            cloneExpression(doWhileStmt->condition.get())
        );
        return cloned;
    }
    if (auto* asmStmt = dynamic_cast<AsmStatement*>(stmt))
        return std::make_unique<AsmStatement>(asmStmt->code);
    if (auto* breakStmt = dynamic_cast<BreakStatement*>(stmt))
        return std::make_unique<BreakStatement>();
    if (auto* continueStmt = dynamic_cast<ContinueStatement*>(stmt))
        return std::make_unique<ContinueStatement>();
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
    // Check for full loop unrolling opportunity first (< 16 iterations)
    if (canUnrollLoop(node)) {
        auto unrolled = unrollLoop(node);
        if (unrolled) {
            // Replace the loop with unrolled code in parent context
            // This is handled via the parent compound statement
        }
    }

    // Check for partial loop unrolling (20-1000 iterations)
    if (currentFunc_ && currentFunc_->optimizeLoopUnroll && currentFunc_->unrollFactor > 0) {
        if (canPartialUnrollLoop(node, currentFunc_->unrollFactor)) {
            auto partialUnrolled = partialUnrollLoop(node, currentFunc_->unrollFactor);
            if (partialUnrolled) {
                // Partial unroll transformation will be handled by parent
            }
        }
    }

    // Check for memcpy pattern
    std::string dest, src, idx;
    if (isMemcpyPattern(node, dest, src, idx)) {
        // Pattern detected: array copy loop
        // Future: Replace with memcpy library call
        // For now: Just detection, no transformation
    }

    // Check for memset pattern
    std::string arr;
    int value;
    if (isMemsetPattern(node, arr, idx, value)) {
        // Pattern detected: array fill loop
        // Future: Replace with memset library call
        // For now: Just detection, no transformation
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
