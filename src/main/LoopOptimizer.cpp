#include "LoopOptimizer.hpp"

void LoopOptimizer::optimizeTranslationUnit(TranslationUnit& unit) {
    unit.accept(*this);
}

void LoopOptimizer::visit(ForStatement& node) {
    // Recursively visit the loop body
    if (node.body) node.body->accept(*this);

    // TODO: Loop-Invariant Code Motion (LICM) implementation
    // Future optimization that will:
    // 1. Identify loop-invariant expressions (don't depend on loop variables)
    // 2. Find expressions that appear 2+ times in the loop
    // 3. Create temporary variables for loop-invariant subexpressions
    // 4. Hoist the computation before the loop
    //
    // Example optimization:
    // FOR r=0 TO HEIGHT:
    //   FOR c=0 TO WIDTH:
    //     grid[r*40+c] = ...
    //
    // Becomes:
    // FOR r=0 TO HEIGHT:
    //   tmp = r*40
    //   FOR c=0 TO WIDTH:
    //     grid[tmp+c] = ...
    //
    // This would eliminate redundant r*40 computation in inner loop
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
