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

// Clone an expression (reuse from LoopOptimizer pattern)
std::unique_ptr<Expression> cloneExpr(Expression* expr) {
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
        return std::make_unique<BinaryOperation>(binOp->op, cloneExpr(binOp->left.get()), cloneExpr(binOp->right.get()));
    if (auto* unOp = dynamic_cast<UnaryOperation*>(expr))
        return std::make_unique<UnaryOperation>(unOp->op, cloneExpr(unOp->operand.get()));
    if (auto* cast = dynamic_cast<CastExpression*>(expr))
        return std::make_unique<CastExpression>(cast->targetType, cast->pointerLevel, cast->isSigned, cloneExpr(cast->expression.get()));
    if (auto* arr = dynamic_cast<ArrayAccess*>(expr))
        return std::make_unique<ArrayAccess>(cloneExpr(arr->arrayExpr.get()), cloneExpr(arr->indexExpr.get()));
    if (auto* mem = dynamic_cast<MemberAccess*>(expr))
        return std::make_unique<MemberAccess>(cloneExpr(mem->structExpr.get()), mem->memberName, mem->isArrow);
    if (auto* cond = dynamic_cast<ConditionalExpression*>(expr))
        return std::make_unique<ConditionalExpression>(cloneExpr(cond->condition.get()), cloneExpr(cond->thenExpr.get()), cloneExpr(cond->elseExpr.get()));
    if (auto* assign = dynamic_cast<Assignment*>(expr))
        return std::make_unique<Assignment>(cloneExpr(assign->target.get()), cloneExpr(assign->expression.get()));
    return nullptr;
}

// Clone a statement (reuse from LoopOptimizer pattern)
std::unique_ptr<Statement> cloneStmt(Statement* stmt) {
    if (!stmt) return nullptr;
    if (auto* comp = dynamic_cast<CompoundStatement*>(stmt)) {
        auto cloned = std::make_unique<CompoundStatement>();
        for (auto& s : comp->statements)
            cloned->statements.push_back(cloneStmt(s.get()));
        return cloned;
    }
    if (auto* expr = dynamic_cast<ExpressionStatement*>(stmt))
        return std::make_unique<ExpressionStatement>(cloneExpr(expr->expression.get()));
    if (auto* ifStmt = dynamic_cast<IfStatement*>(stmt)) {
        auto cloned = std::make_unique<IfStatement>(cloneExpr(ifStmt->condition.get()), cloneStmt(ifStmt->thenBranch.get()));
        if (ifStmt->elseBranch)
            cloned->elseBranch = cloneStmt(ifStmt->elseBranch.get());
        return cloned;
    }
    if (auto* ret = dynamic_cast<ReturnStatement*>(stmt)) {
        std::unique_ptr<Expression> retExpr;
        if (ret->expression)
            retExpr = cloneExpr(ret->expression.get());
        return std::make_unique<ReturnStatement>(std::move(retExpr));
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
        return std::make_unique<ForStatement>(
            cloneStmt(forStmt->initializer.get()),
            cloneExpr(forStmt->condition.get()),
            cloneExpr(forStmt->increment.get()),
            cloneStmt(forStmt->body.get())
        );
    }
    if (auto* whileStmt = dynamic_cast<WhileStatement*>(stmt)) {
        return std::make_unique<WhileStatement>(
            cloneExpr(whileStmt->condition.get()),
            cloneStmt(whileStmt->body.get())
        );
    }
    if (auto* doWhileStmt = dynamic_cast<DoWhileStatement*>(stmt)) {
        return std::make_unique<DoWhileStatement>(
            cloneStmt(doWhileStmt->body.get()),
            cloneExpr(doWhileStmt->condition.get())
        );
    }
    if (auto* asmStmt = dynamic_cast<AsmStatement*>(stmt))
        return std::make_unique<AsmStatement>(asmStmt->code);
    if (auto* breakStmt = dynamic_cast<BreakStatement*>(stmt))
        return std::make_unique<BreakStatement>();
    if (auto* continueStmt = dynamic_cast<ContinueStatement*>(stmt))
        return std::make_unique<ContinueStatement>();
    return nullptr;
}

std::unique_ptr<ForStatement> LoopInterchange::interchangeLoops(const LoopPair& pair) {
    if (!pair.outer || !pair.inner) return nullptr;

    // Create new inner loop (was outer):
    // for (outerVar = ...; outer_cond; outer_inc) { innerBody }
    auto newInnerLoop = std::make_unique<ForStatement>(
        cloneStmt(pair.outer->initializer.get()),
        cloneExpr(pair.outer->condition.get()),
        cloneExpr(pair.outer->increment.get()),
        cloneStmt(pair.inner->body.get())  // Use original inner loop body
    );

    // Create new outer loop (was inner):
    // for (innerVar = ...; inner_cond; inner_inc) { newInnerLoop }
    auto newOuterBody = std::make_unique<CompoundStatement>();
    newOuterBody->statements.push_back(std::move(newInnerLoop));

    auto newOuterLoop = std::make_unique<ForStatement>(
        cloneStmt(pair.inner->initializer.get()),
        cloneExpr(pair.inner->condition.get()),
        cloneExpr(pair.inner->increment.get()),
        std::move(newOuterBody)
    );

    return newOuterLoop;
}

void LoopInterchange::visit(ForStatement& node) {
    // First, visit nested statements
    if (node.body) {
        node.body->accept(*this);
    }
}

void LoopInterchange::visit(CompoundStatement& node) {
    // Process statements, checking for loop interchange opportunities
    std::vector<std::unique_ptr<Statement>> newStatements;

    for (auto& stmt : node.statements) {
        if (!stmt) {
            newStatements.push_back(nullptr);
            continue;
        }

        // Check if this statement is an interchangeable loop
        auto* forStmt = dynamic_cast<ForStatement*>(stmt.get());
        if (forStmt) {
            LoopPair pair = {nullptr, nullptr, "", ""};
            if (detectInterchangeable(forStmt, pair)) {
                // Perform interchange transformation
                auto interchanged = interchangeLoops(pair);
                if (interchanged) {
                    // Visit the transformed loop for further optimizations
                    interchanged->accept(*this);
                    newStatements.push_back(std::move(interchanged));
                    continue;
                }
            }
        }

        // No interchange, process normally
        stmt->accept(*this);
        newStatements.push_back(std::move(stmt));
    }

    // Replace the statement list with the processed one
    node.statements = std::move(newStatements);
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
