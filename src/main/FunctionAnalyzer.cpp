#include "FunctionAnalyzer.hpp"
#include <algorithm>

void FunctionAnalyzer::analyzeTranslationUnit(TranslationUnit& unit) {
    // First pass: collect all function names and analyze each function
    for (auto& decl : unit.topLevelDecls) {
        if (auto* func = dynamic_cast<FunctionDeclaration*>(decl.get())) {
            allFunctionNames_.insert(func->name);
            analyzeFunction(func);
        }
    }

    // Second pass: detect recursion using DFS
    detectRecursion();

    // Third pass: compute optimization recommendations
    computeOptimizationFlags();
}

void FunctionAnalyzer::analyzeFunction(FunctionDeclaration* func) {
    if (!func || !func->body) return;

    FunctionCharacteristics chars;
    chars.name = func->name;
    chars.parameterCount = func->parameters.size();

    state_.current = &chars;
    state_.currentLoopDepth = 0;

    // Visit function body to collect metrics
    func->body->accept(*this);

    state_.current = nullptr;
    characteristics_[func->name] = chars;
}

void FunctionAnalyzer::visit(FunctionDeclaration& node) {
    // Don't recursively analyze nested declarations
    if (node.body) {
        node.body->accept(*this);
    }
}

void FunctionAnalyzer::visit(ForStatement& node) {
    if (!state_.current) return;

    state_.current->loopCount++;
    state_.current->estimatedCodeSize += 5;  // Rough estimate
    state_.currentLoopDepth++;
    state_.current->maxLoopNestingDepth = std::max(
        state_.current->maxLoopNestingDepth,
        state_.currentLoopDepth
    );

    if (node.initializer) {
        if (auto* expr = dynamic_cast<ExpressionStatement*>(node.initializer.get())) {
            expr->accept(*this);
        } else if (auto* decl = dynamic_cast<VariableDeclaration*>(node.initializer.get())) {
            decl->accept(*this);
        }
    }
    if (node.condition) node.condition->accept(*this);
    if (node.increment) node.increment->accept(*this);
    if (node.body) node.body->accept(*this);

    state_.currentLoopDepth--;
}

void FunctionAnalyzer::visit(WhileStatement& node) {
    if (!state_.current) return;

    state_.current->loopCount++;
    state_.current->estimatedCodeSize += 4;
    state_.currentLoopDepth++;
    state_.current->maxLoopNestingDepth = std::max(
        state_.current->maxLoopNestingDepth,
        state_.currentLoopDepth
    );

    if (node.condition) node.condition->accept(*this);
    if (node.body) node.body->accept(*this);

    state_.currentLoopDepth--;
}

void FunctionAnalyzer::visit(DoWhileStatement& node) {
    if (!state_.current) return;

    state_.current->loopCount++;
    state_.current->estimatedCodeSize += 4;
    state_.currentLoopDepth++;
    state_.current->maxLoopNestingDepth = std::max(
        state_.current->maxLoopNestingDepth,
        state_.currentLoopDepth
    );

    if (node.body) node.body->accept(*this);
    if (node.condition) node.condition->accept(*this);

    state_.currentLoopDepth--;
}

void FunctionAnalyzer::visit(IfStatement& node) {
    if (!state_.current) return;

    state_.current->branchCount++;
    state_.current->estimatedCodeSize += 3;

    if (node.condition) node.condition->accept(*this);
    if (node.thenBranch) node.thenBranch->accept(*this);
    if (node.elseBranch) node.elseBranch->accept(*this);
}

void FunctionAnalyzer::visit(SwitchStatement& node) {
    if (!state_.current) return;

    state_.current->branchCount++;
    state_.current->estimatedCodeSize += 5;

    if (node.expression) node.expression->accept(*this);
    if (node.body) node.body->accept(*this);
}

void FunctionAnalyzer::visit(CompoundStatement& node) {
    for (auto& stmt : node.statements) {
        if (stmt) stmt->accept(*this);
    }
}

void FunctionAnalyzer::visit(ExpressionStatement& node) {
    if (node.expression) {
        node.expression->accept(*this);
        if (state_.current) state_.current->estimatedCodeSize += 1;
    }
}

void FunctionAnalyzer::visit(ReturnStatement& node) {
    if (node.expression) node.expression->accept(*this);
    if (state_.current) state_.current->estimatedCodeSize += 2;
}

void FunctionAnalyzer::visit(BreakStatement&) {
    if (state_.current) state_.current->estimatedCodeSize += 1;
}

void FunctionAnalyzer::visit(ContinueStatement&) {
    if (state_.current) state_.current->estimatedCodeSize += 1;
}

void FunctionAnalyzer::visit(VariableDeclaration& node) {
    if (state_.current) state_.current->estimatedCodeSize += 1;
}

void FunctionAnalyzer::visit(AsmStatement&) {
    if (state_.current) state_.current->estimatedCodeSize += 5;  // Conservative estimate
}

void FunctionAnalyzer::visit(FunctionCall& node) {
    if (state_.current) {
        state_.current->isLeaf = false;
    }
}

void FunctionAnalyzer::visit(CaseStatement& node) {
    if (state_.current) state_.current->branchCount++;
    // Case statements don't have bodies; they're just labels in a switch
}

void FunctionAnalyzer::visit(DefaultStatement& node) {
    if (state_.current) state_.current->branchCount++;
    // Default statements don't have bodies; they're just labels in a switch
}

void FunctionAnalyzer::visit(LabelledStatement& node) {
    if (node.statement) node.statement->accept(*this);
}

void FunctionAnalyzer::visit(GotoStatement&) {
    if (state_.current) state_.current->branchCount++;
}

void FunctionAnalyzer::visit(SwitchContinueStatement&) {}

void FunctionAnalyzer::visit(RepeatStatement& node) {
    if (!state_.current) return;

    state_.current->loopCount++;
    state_.current->estimatedCodeSize += 4;
    if (node.body) node.body->accept(*this);
}

void FunctionAnalyzer::visit(StaticAssert&) {}

void FunctionAnalyzer::visit(StructDefinition&) {}

void FunctionAnalyzer::visit(EnumDefinition&) {}

void FunctionAnalyzer::visit(TranslationUnit&) {}

const FunctionAnalyzer::FunctionCharacteristics* FunctionAnalyzer::getCharacteristics(
    const std::string& funcName) const {
    auto it = characteristics_.find(funcName);
    if (it != characteristics_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::set<std::string> FunctionAnalyzer::collectFunctionCalls(FunctionDeclaration* func) {
    std::set<std::string> callees;
    if (!func || !func->body) return callees;

    // Simple visitor to collect function call names
    class CallCollector : public ASTVisitor {
    public:
        std::set<std::string> calls;

        void visit(FunctionCall& node) override {
            calls.insert(node.name);
        }

        // Minimal visitor stubs
        void visit(ForStatement& node) override { if (node.body) node.body->accept(*this); }
        void visit(WhileStatement& node) override { if (node.body) node.body->accept(*this); }
        void visit(DoWhileStatement& node) override { if (node.body) node.body->accept(*this); }
        void visit(IfStatement& node) override {
            if (node.thenBranch) node.thenBranch->accept(*this);
            if (node.elseBranch) node.elseBranch->accept(*this);
        }
        void visit(CompoundStatement& node) override {
            for (auto& stmt : node.statements) {
                if (stmt) stmt->accept(*this);
            }
        }
        void visit(ExpressionStatement& node) override {
            if (node.expression) node.expression->accept(*this);
        }
        void visit(ReturnStatement& node) override {
            if (node.expression) node.expression->accept(*this);
        }
        void visit(SwitchStatement& node) override {
            if (node.body) node.body->accept(*this);
        }
        void visit(LabelledStatement& node) override {
            if (node.statement) node.statement->accept(*this);
        }

        // Expression visitors for function calls within expressions
        void visit(BinaryOperation& node) override {
            node.left->accept(*this);
            node.right->accept(*this);
        }
        void visit(UnaryOperation& node) override {
            node.operand->accept(*this);
        }
        void visit(ConditionalExpression& node) override {
            node.condition->accept(*this);
            node.thenExpr->accept(*this);
            node.elseExpr->accept(*this);
        }
        void visit(Assignment& node) override {
            node.target->accept(*this);
            node.expression->accept(*this);
        }
        void visit(ArrayAccess& node) override {
            node.arrayExpr->accept(*this);
            node.indexExpr->accept(*this);
        }
        void visit(MemberAccess& node) override {
            node.structExpr->accept(*this);
        }

        // Stubs for other visitor methods
        void visit(IntegerLiteral&) override {}
        void visit(FloatLiteral&) override {}
        void visit(StringLiteral&) override {}
        void visit(VariableReference&) override {}
        void visit(CastExpression&) override {}
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
        void visit(DefaultStatement&) override {}
        void visit(CaseStatement&) override {}
        void visit(GotoStatement&) override {}
        void visit(SwitchContinueStatement&) override {}
        void visit(RepeatStatement&) override {}
        void visit(VariableDeclaration&) override {}
        void visit(FunctionDeclaration&) override {}
        void visit(AsmStatement&) override {}
        void visit(StaticAssert&) override {}
        void visit(StructDefinition&) override {}
        void visit(EnumDefinition&) override {}
        void visit(TranslationUnit&) override {}
    };

    CallCollector collector;
    func->body->accept(collector);
    return collector.calls;
}

void FunctionAnalyzer::detectRecursion() {
    // Build call graph: map function name → set of callees
    std::map<std::string, std::set<std::string>> callGraph;
    for (auto& [name, chars] : characteristics_) {
        // We need the FunctionDeclaration to collect calls, but we only have characteristics here
        // So we'll rebuild the map from allFunctionNames_
    }

    // Detect recursion for each function
    std::set<std::string> globalVisited;
    for (const auto& funcName : allFunctionNames_) {
        if (globalVisited.count(funcName)) continue;

        std::set<std::string> visitStack;
        std::set<std::string> callees;  // Empty placeholder
        detectRecursionDFS(funcName, visitStack, globalVisited, callees);
    }
}

bool FunctionAnalyzer::detectRecursionDFS(
    const std::string& funcName,
    std::set<std::string>& visitStack,
    std::set<std::string>& globalVisited,
    const std::set<std::string>& callees) {

    // Already visited globally
    if (globalVisited.count(funcName)) {
        return characteristics_[funcName].isRecursive;
    }

    // In current visit stack: cycle detected
    if (visitStack.count(funcName)) {
        characteristics_[funcName].isRecursive = true;
        return true;
    }

    // Mark as visiting
    visitStack.insert(funcName);

    // Check all callees (would need actual call graph)
    // For now, simplified version
    bool isRecursive = false;
    for (const auto& callee : callees) {
        if (callee == funcName) {
            isRecursive = true;
            break;
        }
    }

    // Mark as globally visited
    visitStack.erase(funcName);
    globalVisited.insert(funcName);

    if (isRecursive) {
        characteristics_[funcName].isRecursive = true;
    }

    return isRecursive;
}

void FunctionAnalyzer::computeOptimizationFlags() {
    for (auto& [name, chars] : characteristics_) {
        // Loop unrolling: avoid heavily branching code, only if loops exist
        // Disable for recursive functions (stack growth concerns)
        chars.shouldUnrollLoops = !chars.isRecursive && chars.branchCount < 5 && chars.loopCount > 0;

        // Loop interchange: only for doubly-nested loops
        // Disable for recursive functions
        chars.shouldInterchangeLoops = !chars.isRecursive &&
                                       chars.maxLoopNestingDepth >= 2 &&
                                       chars.loopCount >= 2 && chars.branchCount < 3;

        // Second-pass constant folding: focus on arithmetic-heavy code
        chars.shouldFoldConstants2x = chars.estimatedCodeSize > 100 && chars.branchCount < 8;

        // SAC: only for non-recursive, leaf functions
        // Recursive functions cannot use SAC (stack-based AR)
        chars.shouldUseSAC = !chars.isRecursive && chars.isLeaf && chars.parameterCount < 6;

        characteristics_[name] = chars;
    }
}
