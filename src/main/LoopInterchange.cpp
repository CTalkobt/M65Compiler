#include "LoopInterchange.hpp"
#include <algorithm>
#include <memory>
#include <iostream>

void LoopInterchange::optimizeTranslationUnit(TranslationUnit& unit) {
    unit.accept(*this);
}

bool LoopInterchange::referencesVariable(Expression* expr, const std::string& varName) {
    if (!expr) return false;
    
    if (auto* ref = dynamic_cast<VariableReference*>(expr))
        return ref->name == varName;
    
    if (auto* binOp = dynamic_cast<BinaryOperation*>(expr))
        return referencesVariable(binOp->left.get(), varName) || 
               referencesVariable(binOp->right.get(), varName);
    
    if (auto* unOp = dynamic_cast<UnaryOperation*>(expr))
        return referencesVariable(unOp->operand.get(), varName);
    
    if (auto* arr = dynamic_cast<ArrayAccess*>(expr))
        return referencesVariable(arr->arrayExpr.get(), varName) || 
               referencesVariable(arr->indexExpr.get(), varName);
    
    if (auto* mem = dynamic_cast<MemberAccess*>(expr))
        return referencesVariable(mem->structExpr.get(), varName);
    
    if (auto* cond = dynamic_cast<ConditionalExpression*>(expr))
        return referencesVariable(cond->condition.get(), varName) ||
               referencesVariable(cond->thenExpr.get(), varName) ||
               referencesVariable(cond->elseExpr.get(), varName);
    
    if (auto* cast = dynamic_cast<CastExpression*>(expr))
        return referencesVariable(cast->expression.get(), varName);
    
    if (auto* assign = dynamic_cast<Assignment*>(expr))
        return referencesVariable(assign->target.get(), varName) ||
               referencesVariable(assign->expression.get(), varName);
    
    if (auto* call = dynamic_cast<FunctionCall*>(expr)) {
        for (auto& arg : call->arguments)
            if (referencesVariable(arg.get(), varName))
                return true;
        return false;
    }
    
    return false;
}

bool LoopInterchange::hasDependencyOnOuterLoop(Statement* body, const std::string& outerVar) {
    if (!body) return false;
    
    // Check if body references outer loop variable
    if (auto* expr = dynamic_cast<ExpressionStatement*>(body))
        return referencesVariable(expr->expression.get(), outerVar);
    
    if (auto* comp = dynamic_cast<CompoundStatement*>(body)) {
        for (auto& stmt : comp->statements)
            if (hasDependencyOnOuterLoop(stmt.get(), outerVar))
                return true;
        return false;
    }
    
    if (auto* ifStmt = dynamic_cast<IfStatement*>(body)) {
        if (referencesVariable(ifStmt->condition.get(), outerVar))
            return true;
        if (hasDependencyOnOuterLoop(ifStmt->thenBranch.get(), outerVar))
            return true;
        if (ifStmt->elseBranch && hasDependencyOnOuterLoop(ifStmt->elseBranch.get(), outerVar))
            return true;
        return false;
    }
    
    if (auto* whileStmt = dynamic_cast<WhileStatement*>(body)) {
        if (referencesVariable(whileStmt->condition.get(), outerVar))
            return true;
        return hasDependencyOnOuterLoop(whileStmt->body.get(), outerVar);
    }
    
    if (auto* doWhileStmt = dynamic_cast<DoWhileStatement*>(body)) {
        if (referencesVariable(doWhileStmt->condition.get(), outerVar))
            return true;
        return hasDependencyOnOuterLoop(doWhileStmt->body.get(), outerVar);
    }
    
    // Other statement types: conservatively assume no dependency
    return false;
}

bool LoopInterchange::detectInterchangeable(ForStatement* stmt, LoopPair& pair) {
    if (!stmt) return false;
    
    // Outer loop must have a body
    if (!stmt->body) return false;
    
    // Body should be a compound statement or single for statement
    ForStatement* innerLoop = nullptr;
    CompoundStatement* bodyComp = nullptr;
    
    if (auto* comp = dynamic_cast<CompoundStatement*>(stmt->body.get())) {
        bodyComp = comp;
        // Look for exactly one for statement (no other statements)
        if (comp->statements.size() != 1) return false;
        innerLoop = dynamic_cast<ForStatement*>(comp->statements[0].get());
    } else {
        // Body is directly a for statement
        innerLoop = dynamic_cast<ForStatement*>(stmt->body.get());
    }
    
    if (!innerLoop || !innerLoop->body) return false;
    
    // Extract loop variables
    if (auto* outerDecl = dynamic_cast<VariableDeclaration*>(stmt->initializer.get())) {
        pair.outerVar = outerDecl->name;
    } else {
        return false;
    }
    
    if (auto* innerDecl = dynamic_cast<VariableDeclaration*>(innerLoop->initializer.get())) {
        pair.innerVar = innerDecl->name;
    } else {
        return false;
    }
    
    // Both loops must be for loops with simple bounds
    if (!stmt->condition || !innerLoop->condition) return false;
    
    // Check: inner loop body doesn't depend on outer loop variable
    if (hasDependencyOnOuterLoop(innerLoop->body.get(), pair.outerVar))
        return false;
    
    // Check: inner loop initializer doesn't reference outer variable
    // (initializer is a Statement, so we skip this check for now)
    
    pair.outer = stmt;
    pair.inner = innerLoop;
    return true;
}

void LoopInterchange::visit(ForStatement& node) {
    // First, visit nested statements
    if (node.body) {
        node.body->accept(*this);
    }
    
    // Then check for interchange opportunity
    LoopPair pair = {nullptr, nullptr, "", ""};
    if (detectInterchangeable(&node, pair)) {
        // Mark for later transformation
        // For now, just log detection
        // std::cerr << "Detected interchangeable loops: " << pair.outerVar << " / " << pair.innerVar << "\n";
    }
}

void LoopInterchange::visit(CompoundStatement& node) {
    for (auto& stmt : node.statements) {
        if (stmt) stmt->accept(*this);
    }
}

void LoopInterchange::visit(IfStatement& node) {
    if (node.thenBranch) node.thenBranch->accept(*this);
    if (node.elseBranch) node.elseBranch->accept(*this);
}

void LoopInterchange::visit(WhileStatement& node) {
    if (node.body) node.body->accept(*this);
}

void LoopInterchange::visit(DoWhileStatement& node) {
    if (node.body) node.body->accept(*this);
}

void LoopInterchange::visit(SwitchStatement& node) {
    if (node.body) node.body->accept(*this);
}

void LoopInterchange::visit(FunctionDeclaration& node) {
    if (node.body) node.body->accept(*this);
}

void LoopInterchange::visit(TranslationUnit& unit) {
    for (auto& decl : unit.topLevelDecls) {
        if (decl) decl->accept(*this);
    }
}

// Stub implementations for other visit methods
void LoopInterchange::visit(ExpressionStatement&) {}
void LoopInterchange::visit(ReturnStatement&) {}
void LoopInterchange::visit(BreakStatement&) {}
void LoopInterchange::visit(ContinueStatement&) {}
void LoopInterchange::visit(CaseStatement&) {}
void LoopInterchange::visit(DefaultStatement&) {}
void LoopInterchange::visit(LabelledStatement&) {}
void LoopInterchange::visit(GotoStatement&) {}
void LoopInterchange::visit(SwitchContinueStatement&) {}
void LoopInterchange::visit(RepeatStatement&) {}
void LoopInterchange::visit(VariableDeclaration&) {}
void LoopInterchange::visit(AsmStatement&) {}
void LoopInterchange::visit(StaticAssert&) {}
void LoopInterchange::visit(StructDefinition&) {}
void LoopInterchange::visit(EnumDefinition&) {}
