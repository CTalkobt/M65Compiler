#pragma once
#include "AST.hpp"
#include "CodeGenerator.hpp"
#include <memory>
#include <map>
#include <set>
#include <iostream>

class ConstantFolder : public ASTVisitor {
public:
    ConstantFolder();
    std::unique_ptr<Expression> lastExpr;
    std::unique_ptr<Statement> lastStmt;
    std::map<std::string, int> knownConstants;
    std::set<std::string> addressEscapedVars; // variables whose address was taken (non-const)
    std::set<std::string> volatileVars;
    std::set<std::string> constVars;        // non-pointer const vars (prevents x = ...)
    std::set<std::string> constPointerVars;  // pointer-const vars (prevents p = ...)
    std::set<std::string> boolVars;
    std::map<std::string, std::shared_ptr<CodeGenerator::StructInfo>> structs;

    std::unique_ptr<Expression> fold(std::unique_ptr<Expression> expr) {
        if (!expr) return nullptr;
        expr->accept(*this);
        return std::move(lastExpr);
    }

    std::unique_ptr<Statement> fold(std::unique_ptr<Statement> stmt) {
        if (!stmt) return nullptr;
        // CompoundStatement and FunctionDeclaration are modified in-place.
        // Their visit methods don't set lastStmt.
        if (dynamic_cast<CompoundStatement*>(stmt.get()) || dynamic_cast<FunctionDeclaration*>(stmt.get())) {
            stmt->accept(*this);
            return stmt;
        } else {
            stmt->accept(*this);
            return std::move(lastStmt);
        }
    }

    void visit(IntegerLiteral& node) override {
        auto result = copyPos(std::make_unique<IntegerLiteral>(node.value), node);
        result->castType = node.castType;
        result->castPointerLevel = node.castPointerLevel;
        result->castIsSigned = node.castIsSigned;
        lastExpr = std::move(result);
    }

    void visit(StringLiteral& node) override {
        lastExpr = copyPos(std::make_unique<StringLiteral>(node.value), node);
    }

    void visit(VariableReference& node) override {
        if (knownConstants.count(node.name)) {
            lastExpr = copyPos(std::make_unique<IntegerLiteral>(knownConstants[node.name]), node);
        } else {
            lastExpr = copyPos(std::make_unique<VariableReference>(node.name), node);
        }
    }

    void visit(Assignment& node) override {
        // Check for const assignment
        if (auto* ref = dynamic_cast<VariableReference*>(node.target.get())) {
            if (constVars.count(ref->name) || constPointerVars.count(ref->name)) {
                throw std::runtime_error("Compile Error: Assignment to read-only location '" + ref->name + "'");
            }
        }
        // Don't fold the target! It must remain a VariableReference or MemberAccess.
        auto expression = fold(std::move(node.expression));
        
        if (auto* ref = dynamic_cast<VariableReference*>(node.target.get())) {
            auto* lit = dynamic_cast<IntegerLiteral*>(expression.get());
            if (lit && !volatileVars.count(ref->name) && !addressEscapedVars.count(ref->name)) {
                // Only propagate simple assignments that aren't inside potentially complex control flow.
                // For now, we clear constants in loops anyway, but this is a safeguard.
                if (node.op == "=") {
                    int val = lit->value;
                    if (boolVars.count(ref->name)) { val = (val != 0) ? 1 : 0; lit->value = val; }
                    knownConstants[ref->name] = val;
                } else {
                    knownConstants.erase(ref->name);
                }
            } else {
                knownConstants.erase(ref->name);
            }
        }
        lastExpr = copyPos(std::make_unique<Assignment>(std::move(node.target), std::move(expression), node.op), node);
    }

    void visit(BinaryOperation& node) override {
        auto left = fold(std::move(node.left));
        auto right = fold(std::move(node.right));

        auto* leftLit = dynamic_cast<IntegerLiteral*>(left.get());
        auto* rightLit = dynamic_cast<IntegerLiteral*>(right.get());

        if (leftLit && rightLit) {
            int result = 0;
            if (node.op == "+") result = leftLit->value + rightLit->value;
            else if (node.op == "-") result = leftLit->value - rightLit->value;
            else if (node.op == "*") result = leftLit->value * rightLit->value;
            else if (node.op == "/") {
                if (rightLit->value != 0) result = leftLit->value / rightLit->value;
                else {
                    lastExpr = copyPos(std::make_unique<BinaryOperation>(node.op, std::move(left), std::move(right)), node);
                    return;
                }
            }
            else if (node.op == "%") {
                if (rightLit->value != 0) result = leftLit->value % rightLit->value;
                else {
                    lastExpr = copyPos(std::make_unique<BinaryOperation>(node.op, std::move(left), std::move(right)), node);
                    return;
                }
            }
            else if (node.op == "&") result = leftLit->value & rightLit->value;
            else if (node.op == "|") result = leftLit->value | rightLit->value;
            else if (node.op == "^") result = leftLit->value ^ rightLit->value;
            else if (node.op == "<<") result = leftLit->value << rightLit->value;
            else if (node.op == ">>") result = leftLit->value >> rightLit->value;
            else if (node.op == "==") result = leftLit->value == rightLit->value;
            else if (node.op == "!=") result = leftLit->value != rightLit->value;
            else if (node.op == "<") result = leftLit->value < rightLit->value;
            else if (node.op == ">") result = leftLit->value > rightLit->value;
            else if (node.op == "<=") result = leftLit->value <= rightLit->value;
            else if (node.op == ">=") result = leftLit->value >= rightLit->value;
            else if (node.op == "&&") result = leftLit->value && rightLit->value;
            else if (node.op == "||") result = leftLit->value || rightLit->value;
            else {
                lastExpr = copyPos(std::make_unique<BinaryOperation>(node.op, std::move(left), std::move(right)), node);
                return;
            }

            lastExpr = copyPos(std::make_unique<IntegerLiteral>(result), node);
            return;
        } else {
            lastExpr = copyPos(std::make_unique<BinaryOperation>(node.op, std::move(left), std::move(right)), node);
        }
    }

    void visit(UnaryOperation& node) override {
        if (node.op == "++" || node.op == "--" || node.op == "++_POST" || node.op == "--_POST") {
            if (auto* ref = dynamic_cast<VariableReference*>(node.operand.get())) {
                if (constVars.count(ref->name)) {
                    throw std::runtime_error("Compile Error: Increment/decrement of read-only location '" + ref->name + "'");
                }
            }
        }
        // Don't fold operands of address-of — the variable reference must survive.
        // For non-const variables, remove from constant propagation since any
        // function could modify it through the pointer. Const variables are safe
        // to keep propagating since they can't be legally modified.
        if (node.op == "&") {
            if (auto* ref = dynamic_cast<VariableReference*>(node.operand.get())) {
                if (!constVars.count(ref->name)) {
                    knownConstants.erase(ref->name);
                    addressEscapedVars.insert(ref->name);
                }
            }
            lastExpr = copyPos(std::make_unique<UnaryOperation>(node.op, std::move(node.operand)), node);
            return;
        }
        auto operand = fold(std::move(node.operand));
        auto* lit = dynamic_cast<IntegerLiteral*>(operand.get());

        if (lit) {
            int result = 0;
            if (node.op == "-") result = -lit->value;
            else if (node.op == "!") result = !lit->value;
            else if (node.op == "~") result = ~lit->value;
            else {
                lastExpr = copyPos(std::make_unique<UnaryOperation>(node.op, std::move(operand)), node);
                return;
            }
            lastExpr = copyPos(std::make_unique<IntegerLiteral>(result), node);
        } else {
            lastExpr = copyPos(std::make_unique<UnaryOperation>(node.op, std::move(operand)), node);
        }
    }

    void visit(FunctionCall& node) override {
        auto call = copyPos(std::make_unique<FunctionCall>(node.name), node);
        for (auto& arg : node.arguments) {
            call->arguments.push_back(fold(std::move(arg)));
        }
        if (node.callExpr) call->callExpr = fold(std::move(node.callExpr));
        lastExpr = std::move(call);
    }

    void visit(BuiltinVaStart& node) override {
        lastExpr = copyPos(std::make_unique<BuiltinVaStart>(fold(std::move(node.ap)), node.lastParamName), node);
    }

    void visit(BuiltinVaArg& node) override {
        lastExpr = copyPos(std::make_unique<BuiltinVaArg>(fold(std::move(node.ap)), node.typeName, node.pointerLevel, node.isSigned), node);
    }

    void visit(ConditionalExpression& node) override {
        auto condition = fold(std::move(node.condition));
        auto* lit = dynamic_cast<IntegerLiteral*>(condition.get());
        if (lit) {
            if (lit->value) lastExpr = fold(std::move(node.thenExpr));
            else lastExpr = fold(std::move(node.elseExpr));
        } else {
            lastExpr = copyPos(std::make_unique<ConditionalExpression>(std::move(condition), fold(std::move(node.thenExpr)), fold(std::move(node.elseExpr))), node);
        }
    }

    void visit(GenericSelection& node) override;

    void visit(InitializerList& node) override {
        auto result = copyPos(std::make_unique<InitializerList>(), node);
        for (auto& elem : node.elements) {
            result->elements.push_back(fold(std::move(elem)));
        }
        lastExpr = std::move(result);
    }

    void visit(ArrayAccess& node) override {
        lastExpr = copyPos(std::make_unique<ArrayAccess>(fold(std::move(node.arrayExpr)), fold(std::move(node.indexExpr))), node);
    }

    void visit(MemberAccess& node) override {
        lastExpr = copyPos(std::make_unique<MemberAccess>(fold(std::move(node.structExpr)), node.memberName, node.isArrow), node);
    }

    void visit(CastExpression& node) override {
        auto operand = fold(std::move(node.expression));
        if (auto* lit = dynamic_cast<IntegerLiteral*>(operand.get())) {
            int val = lit->value;
            if (node.pointerLevel == 0 && node.targetType == "_Bool") {
                val = (val != 0) ? 1 : 0;
            } else if (node.pointerLevel == 0 && node.targetType == "char") {
                val = val & 0xFF;
            }
            auto result = copyPos(std::make_unique<IntegerLiteral>(val), node);
            result->castType = node.targetType;
            result->castPointerLevel = node.pointerLevel;
            result->castIsSigned = node.isSigned;
            lastExpr = std::move(result);
        } else {
            lastExpr = copyPos(std::make_unique<CastExpression>(node.targetType, node.pointerLevel, node.isSigned, std::move(operand)), node);
        }
    }

    void visit(CompoundLiteral& node) override {
        auto result = copyPos(std::make_unique<CompoundLiteral>(node.targetType, node.pointerLevel, node.isSigned, nullptr), node);
        result->arrayDims = node.arrayDims;
        result->tempId = node.tempId;
        // Fold the initializer list
        auto foldedInit = std::make_unique<InitializerList>();
        foldedInit->line = node.initializer->line;
        foldedInit->column = node.initializer->column;
        for (auto& elem : node.initializer->elements) {
            foldedInit->elements.push_back(fold(std::move(elem)));
        }
        result->initializer = std::move(foldedInit);
        lastExpr = std::move(result);
    }

    void visit(AlignofExpression& node) override;
    void visit(SizeofExpression& node) override;

    void visit(VariableDeclaration& node) override {
        auto alignmentExpr = node.alignmentExpr ? fold(std::move(node.alignmentExpr)) : nullptr;
        if (alignmentExpr) {
            if (auto* lit = dynamic_cast<IntegerLiteral*>(alignmentExpr.get())) {
                node.alignment = lit->value;
            }
        }
        auto initializer = node.initializer ? fold(std::move(node.initializer)) : nullptr;
        if (node.pointerLevel > 0) {
            if (node.isPointerConst) constPointerVars.insert(node.name);
        } else {
            if (node.isConst) constVars.insert(node.name);
        }
        if (node.isVolatile || node.isStatic) {
            volatileVars.insert(node.name);
            knownConstants.erase(node.name);
        } else if (initializer) {
            if (node.type == "_Bool") boolVars.insert(node.name);
            if (auto* lit = dynamic_cast<IntegerLiteral*>(initializer.get())) {
                int val = lit->value;
                if (node.type == "_Bool") { val = (val != 0) ? 1 : 0; lit->value = val; }
                knownConstants[node.name] = val;
            } else {
                knownConstants.erase(node.name);
            }
        } else {
            knownConstants.erase(node.name);
        }
        auto decl = copyPos(std::make_unique<VariableDeclaration>(node.type, node.name, node.pointerLevel), node);
        decl->isSigned = node.isSigned;
        decl->initializer = std::move(initializer);
        decl->alignmentExpr = std::move(alignmentExpr);
        decl->alignment = node.alignment;
        decl->isVolatile = node.isVolatile;
        decl->isConst = node.isConst;
        decl->isPointerConst = node.isPointerConst;
        decl->isGlobal = node.isGlobal;
        decl->isExtern = node.isExtern;
        decl->isStatic = node.isStatic;
        decl->isRegister = node.isRegister;
        decl->arrayDims = node.arrayDims;
        decl->isFunctionPointer = node.isFunctionPointer;
        decl->funcPtrSig = node.funcPtrSig;
        lastStmt = std::move(decl);
    }

    void visit(ReturnStatement& node) override {
        lastStmt = copyPos(std::make_unique<ReturnStatement>(fold(std::move(node.expression))), node);
    }

    void visit(BreakStatement& node) override {
        lastStmt = copyPos(std::make_unique<BreakStatement>(), node);
    }

    void visit(ContinueStatement& node) override {
        lastStmt = copyPos(std::make_unique<ContinueStatement>(), node);
    }

    void visit(SwitchContinueStatement& node) override {
        lastStmt = copyPos(std::make_unique<SwitchContinueStatement>(node.target ? fold(std::move(node.target)) : nullptr), node);
    }

    void visit(GotoStatement& node) override {
        lastStmt = copyPos(std::make_unique<GotoStatement>(node.label), node);
    }

    void visit(LabelledStatement& node) override {
        lastStmt = copyPos(std::make_unique<LabelledStatement>(node.label, fold(std::move(node.statement))), node);
    }

    void visit(ExpressionStatement& node) override {
        lastStmt = copyPos(std::make_unique<ExpressionStatement>(fold(std::move(node.expression))), node);
    }

    void visit(IfStatement& node) override {
        auto condition = fold(std::move(node.condition));
        auto* lit = dynamic_cast<IntegerLiteral*>(condition.get());
        if (lit) {
            if (lit->value) {
                lastStmt = fold(std::move(node.thenBranch));
            } else {
                if (node.elseBranch) {
                    lastStmt = fold(std::move(node.elseBranch));
                } else {
                    lastStmt = nullptr;
                }
            }
        } else {
            lastStmt = copyPos(std::make_unique<IfStatement>(std::move(condition), fold(std::move(node.thenBranch)), fold(std::move(node.elseBranch))), node);
        }
    }

    void visit(WhileStatement& node) override;
    void visit(DoWhileStatement& node) override;
    void visit(ForStatement& node) override;

    void visit(SwitchStatement& node) override {
        auto expression = fold(std::move(node.expression));
        auto body = fold(std::move(node.body));
        // After a switch, we don't know which case ran, so variable values are unknown.
        // Clear all tracked constants to avoid incorrect propagation after the switch.
        knownConstants.clear();
        lastStmt = copyPos(std::make_unique<SwitchStatement>(std::move(expression), std::move(body)), node);
    }

    void visit(CaseStatement& node) override {
        // Each case label is a new control flow entry point — don't carry constants
        // from a previous case (they're mutually exclusive paths).
        knownConstants.clear();
        auto value = fold(std::move(node.value));
        auto newNode = std::make_unique<CaseStatement>(std::move(value));
        newNode->label = node.label;
        lastStmt = copyPos(std::move(newNode), node);
    }

    void visit(DefaultStatement& node) override {
        knownConstants.clear();
        auto newNode = std::make_unique<DefaultStatement>();
        newNode->label = node.label;
        lastStmt = copyPos(std::move(newNode), node);
    }

    void visit(AsmStatement& node) override {
        lastStmt = copyPos(std::make_unique<AsmStatement>(node.code), node);
    }

    void visit(StaticAssert& node) override;

    void visit(EnumDefinition& node) override {
        auto def = copyPos(std::make_unique<EnumDefinition>(node.name), node);
        def->enumerators = node.enumerators;
        lastStmt = std::move(def);
    }

    void visit(StructDefinition& node) override {
        auto def = copyPos(std::make_unique<StructDefinition>(node.name, node.isUnion), node);
        int currentOffset = 0;
        int maxAlignment = 1;

        auto sInfo = std::make_shared<CodeGenerator::StructInfo>();
        sInfo->name = node.name;

        for (auto& m : node.members) {
            auto alignmentExpr = m.alignmentExpr ? fold(std::move(m.alignmentExpr)) : nullptr;
            int alignment = 1;
            if (alignmentExpr) {
                if (auto* lit = dynamic_cast<IntegerLiteral*>(alignmentExpr.get())) {
                    alignment = lit->value;
                }
            }

            int mSize = CodeGenerator::getTypeSize(m.type, m.pointerLevel, m.arraySize(), structs);
            int mAlign = alignment; // Simplification, should ideally check type alignment too
            if (mAlign > maxAlignment) maxAlignment = mAlign;

            if (!node.isUnion) {
                if (currentOffset % mAlign != 0) currentOffset += mAlign - (currentOffset % mAlign);
            }

            CodeGenerator::MemberInfo mi;
            mi.type = m.type; mi.pointerLevel = m.pointerLevel; mi.isSigned = m.isSigned;
            mi.isConst = m.isConst; mi.offset = currentOffset; mi.alignment = mAlign; mi.arrayDims = m.arrayDims;
            sInfo->members[m.name] = mi;

            if (!node.isUnion) currentOffset += mSize;
            else if (mSize > currentOffset) currentOffset = mSize;

            {
                StructMember sm;
                sm.type = m.type; sm.pointerLevel = m.pointerLevel; sm.isSigned = m.isSigned;
                sm.name = m.name; sm.isConst = m.isConst; sm.alignment = alignment;
                sm.alignmentExpr = std::move(alignmentExpr); sm.isAnonymous = m.isAnonymous;
                sm.arrayDims = m.arrayDims; sm.bitWidth = m.bitWidth;
                def->members.push_back(std::move(sm));
            }
        }

        if (currentOffset % maxAlignment != 0) currentOffset += maxAlignment - (currentOffset % maxAlignment);
        sInfo->totalSize = currentOffset;
        sInfo->alignment = maxAlignment;
        structs[node.name] = sInfo;

        lastStmt = std::move(def);
    }

    void visit(CompoundStatement& node) override {
        auto oldConstants = knownConstants;
        auto oldVolatile = volatileVars;
        // Track last dead-store-candidate index per variable for elimination.
        // A store is "dead" if a subsequent constant store overwrites it with no
        // intervening read. We conservatively clear all candidates whenever we
        // encounter any statement that isn't a simple constant assignment, since
        // it could read any variable (function calls, control flow, etc.).
        std::map<std::string, int> lastConstStore;
        std::vector<std::unique_ptr<Statement>> newStatements;
        for (auto& stmt : node.statements) {
            auto folded = fold(std::move(stmt));
            if (folded) {
                bool isConstAssign = false;
                if (auto* es = dynamic_cast<ExpressionStatement*>(folded.get())) {
                    if (auto* asgn = dynamic_cast<Assignment*>(es->expression.get())) {
                        if (asgn->op == "=") {
                            if (auto* ref = dynamic_cast<VariableReference*>(asgn->target.get())) {
                                if (dynamic_cast<IntegerLiteral*>(asgn->expression.get()) &&
                                    volatileVars.find(ref->name) == volatileVars.end()) {
                                    isConstAssign = true;
                                    auto it = lastConstStore.find(ref->name);
                                    if (it != lastConstStore.end() && it->second >= 0) {
                                        newStatements[it->second] = nullptr;
                                    }
                                    lastConstStore[ref->name] = (int)newStatements.size();
                                } else {
                                    // Non-constant assignment — variable value now unknown
                                    lastConstStore.erase(ref->name);
                                }
                            }
                        }
                    }
                }
                if (!isConstAssign) {
                    // Any other statement could read any variable — clear all candidates
                    lastConstStore.clear();
                }
                newStatements.push_back(std::move(folded));
            }
        }
        // Remove nulled-out dead stores
        std::vector<std::unique_ptr<Statement>> filtered;
        for (auto& s : newStatements) {
            if (s) filtered.push_back(std::move(s));
        }
        node.statements = std::move(filtered);
        knownConstants = std::move(oldConstants);
        volatileVars = std::move(oldVolatile);
    }

    void visit(FunctionDeclaration& node) override {
        knownConstants.clear(); // Fresh state for new function
        boolVars.clear();
        // The body is a CompoundStatement, which is modified in-place by its visit method.
        // We just need to call accept on it directly, not fold it via the helper.
        node.body->accept(*this);
        // We don't set lastStmt here, as FunctionDeclaration is modified in place
    }

    void visit(TranslationUnit& node) override;

    std::unique_ptr<TranslationUnit> foldTranslationUnit(std::unique_ptr<TranslationUnit> unit);
    std::unique_ptr<ASTNode> fold(std::unique_ptr<ASTNode> node);

private:
    template<typename T, typename U>
    std::unique_ptr<T> copyPos(std::unique_ptr<T> newNode, const U& oldNode) {
        newNode->line = oldNode.line;
        newNode->column = oldNode.column;
        return newNode;
    }
};
