#include "LoopIdiomRegistry.hpp"
#include <algorithm>

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

    // Helper: extract bound expression (for idioms with variable bounds)
    class BoundExpressionExtractor {
    public:
        bool extract(Expression* cond, const std::string& loopVar, std::unique_ptr<Expression>& boundExpr, std::string& op) {
            if (!cond) return false;
            if (auto* binOp = dynamic_cast<BinaryOperation*>(cond)) {
                auto* left = dynamic_cast<VariableReference*>(binOp->left.get());
                if (left && left->name == loopVar) {
                    op = binOp->op;
                    if (op == "<" || op == "<=" || op == ">" || op == ">=") {
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
        void visit(ForStatement& n) override { if (n.body) n.body->accept(*this); }
        void visit(WhileStatement& n) override { if (n.body) n.body->accept(*this); }
        void visit(DoWhileStatement& n) override { if (n.body) n.body->accept(*this); }
        void visit(CompoundStatement& n) override { for (auto& s : n.statements) if (s) s->accept(*this); }
        void visit(IfStatement& n) override {
            if (n.thenBranch) n.thenBranch->accept(*this);
            if (n.elseBranch) n.elseBranch->accept(*this);
        }
        void visit(SwitchStatement& n) override { if (n.body) n.body->accept(*this); }
        void visit(ExpressionStatement&) override {}
        void visit(VariableDeclaration&) override {}
        void visit(ReturnStatement&) override {}
        void visit(GotoStatement&) override {}
        void visit(LabelledStatement& n) override { if (n.statement) n.statement->accept(*this); }
        void visit(RepeatStatement& n) override { if (n.body) n.body->accept(*this); }
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
        void visit(ForStatement& n) override { if (n.body && !found) n.body->accept(*this); }
        void visit(WhileStatement& n) override { if (n.body && !found) n.body->accept(*this); }
        void visit(DoWhileStatement& n) override { if (n.body && !found) n.body->accept(*this); }
        void visit(SwitchStatement& n) override { if (n.body && !found) n.body->accept(*this); }
        void visit(ExpressionStatement& n) override { if (n.expression && !found) n.expression->accept(*this); }
        void visit(ReturnStatement& n) override { if (n.expression && !found) n.expression->accept(*this); }
        void visit(BinaryOperation& n) override {
            if (n.left && !found) n.left->accept(*this);
            if (n.right && !found) n.right->accept(*this);
        }
        void visit(UnaryOperation& n) override { if (n.operand && !found) n.operand->accept(*this); }
        void visit(Assignment& n) override { if (n.expression && !found) n.expression->accept(*this); }
        void visit(ArrayAccess& n) override {
            if (n.arrayExpr && !found) n.arrayExpr->accept(*this);
            if (n.indexExpr && !found) n.indexExpr->accept(*this);
        }
        void visit(RepeatStatement& n) override { if (n.body && !found) n.body->accept(*this); }
        void visit(LabelledStatement& n) override { if (n.statement && !found) n.statement->accept(*this); }
        void visit(CastExpression& n) override { if (n.expression && !found) n.expression->accept(*this); }
        void visit(ConditionalExpression& n) override {
            if (n.condition && !found) n.condition->accept(*this);
            if (n.thenExpr && !found) n.thenExpr->accept(*this);
            if (n.elseExpr && !found) n.elseExpr->accept(*this);
        }
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

    // Placeholder detector stub classes — will be populated from LoopOptimizer.cpp
    // TODO: Extract MemcpyPatternDetector, MemsetPatternDetector, SumReductionDetector,
    // SearchLoopDetector, CountLoopDetector, DotProductDetector from LoopOptimizer.cpp

}  // end anonymous namespace

// Placeholder LoopIdiom adapters — stubs for now, will be completed in Phase 89.2
namespace {
    class PlaceholderIdiom : public LoopIdiom {
    public:
        const char* name() const override { return "placeholder"; }
        bool detect(const ForStatement&, IdiomMatch&) const override { return false; }
        std::unique_ptr<Statement> transform(const ForStatement&, const IdiomMatch&) const override { return nullptr; }
    };
}

// Singleton accessor
LoopIdiomRegistry& LoopIdiomRegistry::instance() {
    static LoopIdiomRegistry registry;
    return registry;
}

// Constructor: register all built-in idioms
LoopIdiomRegistry::LoopIdiomRegistry() {
    // Temporary placeholder to allow compilation
    // idioms_.push_back(std::make_unique<PlaceholderIdiom>());
    // TODO: Register MemcpyIdiom, MemsetIdiom, SumReductionIdiom, SearchIdiom, CountIdiom, DotProductIdiom
}

// Try each idiom in order until one matches and transforms successfully
std::unique_ptr<Statement> LoopIdiomRegistry::tryTransform(const ForStatement& loop) const {
    for (const auto& idiom : idioms_) {
        IdiomMatch match;
        if (idiom->detect(loop, match)) {
            return idiom->transform(loop, match);
        }
    }
    return nullptr;
}
