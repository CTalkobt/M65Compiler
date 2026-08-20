#pragma once

#include "ASTNode.h"
#include <vector>
#include <string>

/**
 * Phase 90.3: Zero-Argument Call Detection
 *
 * Identifies function calls with zero arguments. After a zero-arg call,
 * the stack pointer is unchanged, so frame pointer recalculation is unnecessary.
 *
 * This enables skipping ~9 bytes of FP recalc code per zero-arg call.
 * Common in callbacks, event handlers, and utility functions.
 */

class ZeroArgCallDetector : public ASTVisitor {
public:
    struct ZeroArgCall {
        std::string functionName;
        int lineNumber = 0;
    };

    std::vector<ZeroArgCall> zeroArgCalls;  // All detected zero-arg calls
    int totalFunctionCalls = 0;             // Total function calls in scope
    int zeroArgCallCount = 0;               // Count of zero-arg calls

    // Visitor methods
    void visit(FunctionCall& node) override {
        totalFunctionCalls++;
        if (node.arguments.empty()) {
            zeroArgCallCount++;
            zeroArgCalls.push_back({node.name, node.line});
        }
        // Walk arguments for nested calls
        for (auto& arg : node.arguments) {
            if (arg) arg->accept(*this);
        }
    }

    // Boilerplate: visit all other node types
    void visit(IntegerLiteral&) override {}
    void visit(FloatLiteral&) override {}
    void visit(StringLiteral&) override {}
    void visit(VariableReference&) override {}
    void visit(Assignment& n) override {
        if (n.target) n.target->accept(*this);
        if (n.expression) n.expression->accept(*this);
    }
    void visit(BinaryOperation& n) override {
        if (n.left) n.left->accept(*this);
        if (n.right) n.right->accept(*this);
    }
    void visit(UnaryOperation& n) override {
        if (n.operand) n.operand->accept(*this);
    }
    void visit(ConditionalExpression& n) override {
        if (n.condition) n.condition->accept(*this);
        if (n.thenExpr) n.thenExpr->accept(*this);
        if (n.elseExpr) n.elseExpr->accept(*this);
    }
    void visit(GenericSelection& n) override {
        if (n.control) n.control->accept(*this);
        for (auto& a : n.associations) {
            if (a.result) a.result->accept(*this);
        }
    }
    void visit(InitializerList& n) override {
        for (auto& e : n.elements) {
            if (e) e->accept(*this);
        }
    }
    void visit(ArrayAccess& n) override {
        if (n.arrayExpr) n.arrayExpr->accept(*this);
        if (n.indexExpr) n.indexExpr->accept(*this);
    }
    void visit(MemberAccess& n) override {
        if (n.structExpr) n.structExpr->accept(*this);
    }
    void visit(CastExpression& n) override {
        if (n.expression) n.expression->accept(*this);
    }
    void visit(CompoundLiteral& n) override {
        if (n.initializer) {
            for (auto& e : n.initializer->elements) {
                if (e) e->accept(*this);
            }
        }
    }
    void visit(AlignofExpression&) override {}
    void visit(SizeofExpression& n) override {
        if (!n.isType && n.expression) n.expression->accept(*this);
    }
    void visit(VariableDeclaration& n) override {
        if (n.initializer) n.initializer->accept(*this);
    }
    void visit(ReturnStatement& n) override {
        if (n.expression) n.expression->accept(*this);
    }
    void visit(BreakStatement&) override {}
    void visit(ContinueStatement&) override {}
    void visit(SwitchContinueStatement& n) override {
        if (n.target) n.target->accept(*this);
    }
    void visit(GotoStatement& n) override {
        if (n.target) n.target->accept(*this);
    }
    void visit(LabelledStatement& n) override {
        if (n.statement) n.statement->accept(*this);
    }
    void visit(ExpressionStatement& n) override {
        if (n.expression) n.expression->accept(*this);
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
        if (n.initialization) n.initialization->accept(*this);
        if (n.condition) n.condition->accept(*this);
        if (n.increment) n.increment->accept(*this);
        if (n.body) n.body->accept(*this);
    }
    void visit(SwitchStatement& n) override {
        if (n.expression) n.expression->accept(*this);
        if (n.body) n.body->accept(*this);
    }
    void visit(CompoundStatement& n) override {
        for (auto& s : n.statements) {
            if (s) s->accept(*this);
        }
    }
    void visit(CpuRegisterAccess&) override {}
    void visit(CpuFlagAccess&) override {}
    void visit(LabelAddressExpression&) override {}
    void visit(BuiltinVaStart&) override {}
    void visit(BuiltinVaArg&) override {}
    void visit(StaticAssert&) override {}
    void visit(EnumDefinition&) override {}
};
