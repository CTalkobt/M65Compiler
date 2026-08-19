#include "FunctionAnalyzer.hpp"
#include <algorithm>

void FunctionAnalyzer::analyzeTranslationUnit(TranslationUnit& unit) {
    // First pass: analyze each function
    for (auto& decl : unit.topLevelDecls) {
        if (auto* func = dynamic_cast<FunctionDeclaration*>(decl.get())) {
            analyzeFunction(func);
        }
    }

    // Second pass: compute optimization recommendations
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

void FunctionAnalyzer::computeOptimizationFlags() {
    for (auto& [name, chars] : characteristics_) {
        // Loop unrolling: avoid heavily branching code, only if loops exist
        chars.shouldUnrollLoops = chars.branchCount < 5 && chars.loopCount > 0;

        // Loop interchange: only for doubly-nested loops
        chars.shouldInterchangeLoops = chars.maxLoopNestingDepth >= 2 &&
                                       chars.loopCount >= 2 && chars.branchCount < 3;

        // Second-pass constant folding: focus on arithmetic-heavy code
        chars.shouldFoldConstants2x = chars.estimatedCodeSize > 100 && chars.branchCount < 8;

        // SAC: only for non-recursive (leaf), non-complex functions
        chars.shouldUseSAC = chars.isLeaf && chars.parameterCount < 6;

        characteristics_[name] = chars;
    }
}
