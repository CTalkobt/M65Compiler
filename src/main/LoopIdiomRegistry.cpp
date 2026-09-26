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

    // =====================================================================
    // DETECTOR CLASSES (extracted from LoopOptimizer.cpp)
    // =====================================================================

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

    class MemsetPatternDetector {
    public:
        bool detect(const ForStatement& loop, std::string& arr, std::string& idx, int& value) {
            // Check if body is: arr[i] = constant;
            if (!loop.body) return false;

            ExpressionStatement* exprStmt = nullptr;
            if (auto* compStmt = dynamic_cast<CompoundStatement*>(loop.body.get())) {
                if (compStmt->statements.size() == 1 && compStmt->statements[0]) {
                    exprStmt = dynamic_cast<ExpressionStatement*>(compStmt->statements[0].get());
                }
            } else {
                exprStmt = dynamic_cast<ExpressionStatement*>(loop.body.get());
            }

            if (!exprStmt || !exprStmt->expression) return false;

            auto* assign = dynamic_cast<Assignment*>(exprStmt->expression.get());
            if (!assign) return false;

            // LHS must be array access: arr[i]
            auto* lhsArray = dynamic_cast<ArrayAccess*>(assign->target.get());
            if (!lhsArray) return false;

            auto* arrRef = dynamic_cast<VariableReference*>(lhsArray->arrayExpr.get());
            if (!arrRef) return false;
            arr = arrRef->name;

            auto* idxRef = dynamic_cast<VariableReference*>(lhsArray->indexExpr.get());
            if (!idxRef) return false;
            idx = idxRef->name;

            // RHS must be integer literal
            auto* lit = dynamic_cast<IntegerLiteral*>(assign->expression.get());
            if (!lit) return false;
            value = lit->value;

            return true;
        }
    };

    class SumReductionDetector {
    public:
        bool detect(const ForStatement& loop, std::string& accum, std::string& array, std::string& idx) {
            // Check if body is: accum += arr[i];
            if (!loop.body) return false;

            ExpressionStatement* exprStmt = nullptr;
            if (auto* compStmt = dynamic_cast<CompoundStatement*>(loop.body.get())) {
                if (compStmt->statements.size() == 1 && compStmt->statements[0]) {
                    exprStmt = dynamic_cast<ExpressionStatement*>(compStmt->statements[0].get());
                }
            } else {
                exprStmt = dynamic_cast<ExpressionStatement*>(loop.body.get());
            }

            if (!exprStmt || !exprStmt->expression) return false;

            auto* assign = dynamic_cast<Assignment*>(exprStmt->expression.get());
            if (!assign || assign->op != "+=") return false;

            // LHS must be a variable (accumulator)
            auto* accumRef = dynamic_cast<VariableReference*>(assign->target.get());
            if (!accumRef) return false;
            accum = accumRef->name;

            // RHS must be array access: arr[i]
            auto* rhsArray = dynamic_cast<ArrayAccess*>(assign->expression.get());
            if (!rhsArray) return false;

            auto* arrayRef = dynamic_cast<VariableReference*>(rhsArray->arrayExpr.get());
            if (!arrayRef) return false;
            array = arrayRef->name;

            auto* idxRef = dynamic_cast<VariableReference*>(rhsArray->indexExpr.get());
            if (!idxRef) return false;
            idx = idxRef->name;

            return accum != array;  // accumulator and array must be different
        }
    };

    class SearchLoopDetector {
    public:
        bool detect(const ForStatement& loop, std::string& array, std::string& target, std::string& idx, std::string& result) {
            if (!loop.body) return false;

            // Body should be: if (arr[i] == target) { result = i; break; }
            IfStatement* ifStmt = nullptr;

            if (auto* compStmt = dynamic_cast<CompoundStatement*>(loop.body.get())) {
                if (compStmt->statements.size() == 1 && compStmt->statements[0]) {
                    ifStmt = dynamic_cast<IfStatement*>(compStmt->statements[0].get());
                }
            } else {
                ifStmt = dynamic_cast<IfStatement*>(loop.body.get());
            }

            if (!ifStmt || !ifStmt->condition) return false;

            // Condition must be: arr[i] == target
            auto* binOp = dynamic_cast<BinaryOperation*>(ifStmt->condition.get());
            if (!binOp || binOp->op != "==") return false;

            auto* lhsArray = dynamic_cast<ArrayAccess*>(binOp->left.get());
            auto* rhsTarget = dynamic_cast<VariableReference*>(binOp->right.get());

            if (!lhsArray || !rhsTarget) return false;

            auto* arrayRef = dynamic_cast<VariableReference*>(lhsArray->arrayExpr.get());
            if (!arrayRef) return false;
            array = arrayRef->name;

            auto* idxRef = dynamic_cast<VariableReference*>(lhsArray->indexExpr.get());
            if (!idxRef) return false;
            idx = idxRef->name;

            target = rhsTarget->name;

            // Then-branch must be: result = i; break;
            if (!ifStmt->thenBranch) return false;

            CompoundStatement* thenComp = nullptr;
            if (auto* comp = dynamic_cast<CompoundStatement*>(ifStmt->thenBranch.get())) {
                if (comp->statements.size() == 2) {
                    thenComp = comp;
                }
            }

            if (!thenComp) return false;

            auto* assignStmt = dynamic_cast<ExpressionStatement*>(thenComp->statements[0].get());
            auto* breakStmt = dynamic_cast<BreakStatement*>(thenComp->statements[1].get());

            if (!assignStmt || !breakStmt || !assignStmt->expression) return false;

            auto* assign = dynamic_cast<Assignment*>(assignStmt->expression.get());
            if (!assign) return false;

            auto* resultRef = dynamic_cast<VariableReference*>(assign->target.get());
            if (!resultRef) return false;
            result = resultRef->name;

            auto* assignIdxRef = dynamic_cast<VariableReference*>(assign->expression.get());
            if (!assignIdxRef || assignIdxRef->name != idx) return false;

            return true;
        }
    };

    class CountLoopDetector {
    public:
        bool detect(const ForStatement& loop, std::string& array, std::string& target, std::string& idx, std::string& counter) {
            if (!loop.body) return false;

            // Body should be: if (arr[i] == target) counter++;
            IfStatement* ifStmt = nullptr;

            if (auto* compStmt = dynamic_cast<CompoundStatement*>(loop.body.get())) {
                if (compStmt->statements.size() == 1 && compStmt->statements[0]) {
                    ifStmt = dynamic_cast<IfStatement*>(compStmt->statements[0].get());
                }
            } else {
                ifStmt = dynamic_cast<IfStatement*>(loop.body.get());
            }

            if (!ifStmt || !ifStmt->condition) return false;

            // Condition: arr[i] == target
            auto* binOp = dynamic_cast<BinaryOperation*>(ifStmt->condition.get());
            if (!binOp || binOp->op != "==") return false;

            auto* lhsArray = dynamic_cast<ArrayAccess*>(binOp->left.get());
            auto* rhsTarget = dynamic_cast<VariableReference*>(binOp->right.get());

            if (!lhsArray || !rhsTarget) return false;

            auto* arrayRef = dynamic_cast<VariableReference*>(lhsArray->arrayExpr.get());
            if (!arrayRef) return false;
            array = arrayRef->name;

            auto* idxRef = dynamic_cast<VariableReference*>(lhsArray->indexExpr.get());
            if (!idxRef) return false;
            idx = idxRef->name;

            target = rhsTarget->name;

            // Then-branch: counter++
            if (!ifStmt->thenBranch) return false;

            ExpressionStatement* exprStmt = nullptr;
            if (auto* comp = dynamic_cast<CompoundStatement*>(ifStmt->thenBranch.get())) {
                if (comp->statements.size() == 1 && comp->statements[0]) {
                    exprStmt = dynamic_cast<ExpressionStatement*>(comp->statements[0].get());
                }
            } else {
                exprStmt = dynamic_cast<ExpressionStatement*>(ifStmt->thenBranch.get());
            }

            if (!exprStmt || !exprStmt->expression) return false;

            auto* unaryOp = dynamic_cast<UnaryOperation*>(exprStmt->expression.get());
            if (!unaryOp || (unaryOp->op != "++" && unaryOp->op != "++_POST")) return false;

            auto* counterRef = dynamic_cast<VariableReference*>(unaryOp->operand.get());
            if (!counterRef) return false;
            counter = counterRef->name;

            return true;
        }
    };

    class DotProductDetector {
    public:
        bool detect(const ForStatement& loop, std::string& accum, std::string& arrayA, std::string& arrayB, std::string& idx) {
            if (!loop.body) return false;

            ExpressionStatement* exprStmt = nullptr;
            if (auto* compStmt = dynamic_cast<CompoundStatement*>(loop.body.get())) {
                if (compStmt->statements.size() == 1 && compStmt->statements[0]) {
                    exprStmt = dynamic_cast<ExpressionStatement*>(compStmt->statements[0].get());
                }
            } else {
                exprStmt = dynamic_cast<ExpressionStatement*>(loop.body.get());
            }

            if (!exprStmt || !exprStmt->expression) return false;

            auto* assign = dynamic_cast<Assignment*>(exprStmt->expression.get());
            if (!assign || assign->op != "+=") return false;

            // LHS: accumulator
            auto* accumRef = dynamic_cast<VariableReference*>(assign->target.get());
            if (!accumRef) return false;
            accum = accumRef->name;

            // RHS: a[i] * b[i]
            auto* binOp = dynamic_cast<BinaryOperation*>(assign->expression.get());
            if (!binOp || binOp->op != "*") return false;

            auto* lhsArray = dynamic_cast<ArrayAccess*>(binOp->left.get());
            auto* rhsArray = dynamic_cast<ArrayAccess*>(binOp->right.get());

            if (!lhsArray || !rhsArray) return false;

            auto* aRef = dynamic_cast<VariableReference*>(lhsArray->arrayExpr.get());
            auto* bRef = dynamic_cast<VariableReference*>(rhsArray->arrayExpr.get());

            if (!aRef || !bRef) return false;
            arrayA = aRef->name;
            arrayB = bRef->name;

            auto* aIdx = dynamic_cast<VariableReference*>(lhsArray->indexExpr.get());
            auto* bIdx = dynamic_cast<VariableReference*>(rhsArray->indexExpr.get());

            if (!aIdx || !bIdx || aIdx->name != bIdx->name) return false;
            idx = aIdx->name;

            return arrayA != arrayB && accum != arrayA && accum != arrayB;
        }
    };

    // =====================================================================
    // TRANSFORM HELPER FUNCTIONS (extracted from LoopOptimizer.cpp)
    // =====================================================================

    std::unique_ptr<Statement> buildMemcpyCall(const ForStatement& stmt, const std::string& dest, const std::string& src) {
        InitializerAnalyzer initAnalyzer;
        std::string loopVar;
        int initValue;
        if (!initAnalyzer.extract(stmt.initializer.get(), loopVar, initValue)) {
            return nullptr;
        }

        BoundExpressionExtractor boundExtractor;
        std::unique_ptr<Expression> boundExpr;
        std::string op;
        if (!boundExtractor.extract(stmt.condition.get(), loopVar, boundExpr, op) || !boundExpr) {
            return nullptr;
        }

        auto memcpyCall = std::make_unique<FunctionCall>("memcpy");
        memcpyCall->arguments.push_back(std::make_unique<VariableReference>(dest));
        memcpyCall->arguments.push_back(std::make_unique<VariableReference>(src));
        memcpyCall->arguments.push_back(std::move(boundExpr));

        return std::make_unique<ExpressionStatement>(std::move(memcpyCall));
    }

    std::unique_ptr<Statement> buildMemsetCall(const ForStatement& stmt, const std::string& arr, int value) {
        InitializerAnalyzer initAnalyzer;
        std::string loopVar;
        int initValue;
        if (!initAnalyzer.extract(stmt.initializer.get(), loopVar, initValue)) {
            return nullptr;
        }

        BoundExpressionExtractor boundExtractor;
        std::unique_ptr<Expression> boundExpr;
        std::string op;
        if (!boundExtractor.extract(stmt.condition.get(), loopVar, boundExpr, op) || !boundExpr) {
            return nullptr;
        }

        auto memsetCall = std::make_unique<FunctionCall>("memset");
        memsetCall->arguments.push_back(std::make_unique<VariableReference>(arr));
        memsetCall->arguments.push_back(std::make_unique<IntegerLiteral>(value));
        memsetCall->arguments.push_back(std::move(boundExpr));

        return std::make_unique<ExpressionStatement>(std::move(memsetCall));
    }

    std::unique_ptr<Statement> buildSumReductionCall(const ForStatement& stmt, const std::string& accum, const std::string& array) {
        InitializerAnalyzer initAnalyzer;
        std::string loopVar;
        int initValue;
        if (!initAnalyzer.extract(stmt.initializer.get(), loopVar, initValue)) {
            return nullptr;
        }

        BoundExpressionExtractor boundExtractor;
        std::unique_ptr<Expression> boundExpr;
        std::string op;
        if (!boundExtractor.extract(stmt.condition.get(), loopVar, boundExpr, op) || !boundExpr) {
            return nullptr;
        }

        auto idiomCall = std::make_unique<FunctionCall>("__idiom_sum16");
        idiomCall->arguments.push_back(std::make_unique<VariableReference>(array));
        idiomCall->arguments.push_back(std::move(boundExpr));

        auto assign = std::make_unique<Assignment>(
            std::make_unique<VariableReference>(accum),
            std::move(idiomCall)
        );
        assign->op = "=";

        return std::make_unique<ExpressionStatement>(std::move(assign));
    }

    std::unique_ptr<Statement> buildSearchCall(const ForStatement& stmt, const std::string& array, const std::string& target, const std::string& result) {
        InitializerAnalyzer initAnalyzer;
        std::string loopVar;
        int initValue;
        if (!initAnalyzer.extract(stmt.initializer.get(), loopVar, initValue)) {
            return nullptr;
        }

        BoundExpressionExtractor boundExtractor;
        std::unique_ptr<Expression> boundExpr;
        std::string op;
        if (!boundExtractor.extract(stmt.condition.get(), loopVar, boundExpr, op) || !boundExpr) {
            return nullptr;
        }

        auto idiomCall = std::make_unique<FunctionCall>("__idiom_find8");
        idiomCall->arguments.push_back(std::make_unique<VariableReference>(array));
        idiomCall->arguments.push_back(std::move(boundExpr));
        idiomCall->arguments.push_back(std::make_unique<VariableReference>(target));

        auto assign = std::make_unique<Assignment>(
            std::make_unique<VariableReference>(result),
            std::move(idiomCall)
        );
        assign->op = "=";

        return std::make_unique<ExpressionStatement>(std::move(assign));
    }

    std::unique_ptr<Statement> buildCountCall(const ForStatement& stmt, const std::string& array, const std::string& target, const std::string& counter) {
        InitializerAnalyzer initAnalyzer;
        std::string loopVar;
        int initValue;
        if (!initAnalyzer.extract(stmt.initializer.get(), loopVar, initValue)) {
            return nullptr;
        }

        BoundExpressionExtractor boundExtractor;
        std::unique_ptr<Expression> boundExpr;
        std::string op;
        if (!boundExtractor.extract(stmt.condition.get(), loopVar, boundExpr, op) || !boundExpr) {
            return nullptr;
        }

        auto idiomCall = std::make_unique<FunctionCall>("__idiom_count8");
        idiomCall->arguments.push_back(std::make_unique<VariableReference>(array));
        idiomCall->arguments.push_back(std::move(boundExpr));
        idiomCall->arguments.push_back(std::make_unique<VariableReference>(target));

        auto assign = std::make_unique<Assignment>(
            std::make_unique<VariableReference>(counter),
            std::move(idiomCall)
        );
        assign->op = "=";

        return std::make_unique<ExpressionStatement>(std::move(assign));
    }

    std::unique_ptr<Statement> buildDotProductCall(const ForStatement& stmt, const std::string& accum, const std::string& arrayA, const std::string& arrayB) {
        InitializerAnalyzer initAnalyzer;
        std::string loopVar;
        int initValue;
        if (!initAnalyzer.extract(stmt.initializer.get(), loopVar, initValue)) {
            return nullptr;
        }

        BoundExpressionExtractor boundExtractor;
        std::unique_ptr<Expression> boundExpr;
        std::string op;
        if (!boundExtractor.extract(stmt.condition.get(), loopVar, boundExpr, op) || !boundExpr) {
            return nullptr;
        }

        auto idiomCall = std::make_unique<FunctionCall>("__idiom_dot16");
        idiomCall->arguments.push_back(std::make_unique<VariableReference>(arrayA));
        idiomCall->arguments.push_back(std::make_unique<VariableReference>(arrayB));
        idiomCall->arguments.push_back(std::move(boundExpr));

        auto assign = std::make_unique<Assignment>(
            std::make_unique<VariableReference>(accum),
            std::move(idiomCall)
        );
        assign->op = "=";

        return std::make_unique<ExpressionStatement>(std::move(assign));
    }

    // =====================================================================
    // LOOPIDIOM ADAPTER SUBCLASSES
    // =====================================================================

    class MemcpyIdiom : public LoopIdiom {
    public:
        const char* name() const override { return "memcpy"; }

        bool detect(const ForStatement& loop, IdiomMatch& match) const override {
            MemcpyPatternDetector detector;
            std::string dest, src, idx;
            if (!detector.detect(loop, dest, src, idx)) return false;
            match.vars["dest"] = dest;
            match.vars["src"] = src;
            match.vars["idx"] = idx;
            return true;
        }

        std::unique_ptr<Statement> transform(const ForStatement& loop, const IdiomMatch& match) const override {
            return buildMemcpyCall(loop, match.vars.at("dest"), match.vars.at("src"));
        }
    };

    class MemsetIdiom : public LoopIdiom {
    public:
        const char* name() const override { return "memset"; }

        bool detect(const ForStatement& loop, IdiomMatch& match) const override {
            MemsetPatternDetector detector;
            std::string arr, idx;
            int value;
            if (!detector.detect(loop, arr, idx, value)) return false;
            match.vars["arr"] = arr;
            match.vars["idx"] = idx;
            match.intValue = value;
            return true;
        }

        std::unique_ptr<Statement> transform(const ForStatement& loop, const IdiomMatch& match) const override {
            return buildMemsetCall(loop, match.vars.at("arr"), match.intValue);
        }
    };

    class SumReductionIdiom : public LoopIdiom {
    public:
        const char* name() const override { return "sum-reduction"; }

        bool detect(const ForStatement& loop, IdiomMatch& match) const override {
            SumReductionDetector detector;
            std::string accum, array, idx;
            if (!detector.detect(loop, accum, array, idx)) return false;
            match.vars["accum"] = accum;
            match.vars["array"] = array;
            match.vars["idx"] = idx;
            return true;
        }

        std::unique_ptr<Statement> transform(const ForStatement& loop, const IdiomMatch& match) const override {
            return buildSumReductionCall(loop, match.vars.at("accum"), match.vars.at("array"));
        }
    };

    class SearchIdiom : public LoopIdiom {
    public:
        const char* name() const override { return "linear-search"; }

        bool detect(const ForStatement& loop, IdiomMatch& match) const override {
            SearchLoopDetector detector;
            std::string array, target, idx, result;
            if (!detector.detect(loop, array, target, idx, result)) return false;
            match.vars["array"] = array;
            match.vars["target"] = target;
            match.vars["idx"] = idx;
            match.vars["result"] = result;
            return true;
        }

        std::unique_ptr<Statement> transform(const ForStatement& loop, const IdiomMatch& match) const override {
            return buildSearchCall(loop, match.vars.at("array"), match.vars.at("target"), match.vars.at("result"));
        }
    };

    class CountIdiom : public LoopIdiom {
    public:
        const char* name() const override { return "counting"; }

        bool detect(const ForStatement& loop, IdiomMatch& match) const override {
            CountLoopDetector detector;
            std::string array, target, idx, counter;
            if (!detector.detect(loop, array, target, idx, counter)) return false;
            match.vars["array"] = array;
            match.vars["target"] = target;
            match.vars["idx"] = idx;
            match.vars["counter"] = counter;
            return true;
        }

        std::unique_ptr<Statement> transform(const ForStatement& loop, const IdiomMatch& match) const override {
            return buildCountCall(loop, match.vars.at("array"), match.vars.at("target"), match.vars.at("counter"));
        }
    };

    class DotProductIdiom : public LoopIdiom {
    public:
        const char* name() const override { return "dot-product"; }

        bool detect(const ForStatement& loop, IdiomMatch& match) const override {
            DotProductDetector detector;
            std::string accum, arrayA, arrayB, idx;
            if (!detector.detect(loop, accum, arrayA, arrayB, idx)) return false;
            match.vars["accum"] = accum;
            match.vars["arrayA"] = arrayA;
            match.vars["arrayB"] = arrayB;
            match.vars["idx"] = idx;
            return true;
        }

        std::unique_ptr<Statement> transform(const ForStatement& loop, const IdiomMatch& match) const override {
            return buildDotProductCall(loop, match.vars.at("accum"), match.vars.at("arrayA"), match.vars.at("arrayB"));
        }
    };

}  // end anonymous namespace

// Singleton accessor
LoopIdiomRegistry& LoopIdiomRegistry::instance() {
    static LoopIdiomRegistry registry;
    return registry;
}

// Constructor: register all built-in idioms
LoopIdiomRegistry::LoopIdiomRegistry() {
    idioms_.push_back(std::make_unique<MemcpyIdiom>());
    idioms_.push_back(std::make_unique<MemsetIdiom>());
    idioms_.push_back(std::make_unique<SumReductionIdiom>());
    idioms_.push_back(std::make_unique<SearchIdiom>());
    idioms_.push_back(std::make_unique<CountIdiom>());
    idioms_.push_back(std::make_unique<DotProductIdiom>());
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
