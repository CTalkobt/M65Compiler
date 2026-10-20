#pragma once
#include "StructLayout.hpp"
#include "AST.hpp"
#include "TypeInfo.hpp"
#include "AddressTemplateDetector.hpp"
#include "Diagnostic.hpp"
#include "ConstEval.hpp"
#include <memory>
#include <map>
#include <set>
#include <iostream>
#include <cstdint>
#include <algorithm>

class ConstantFolder : public ASTVisitor {
public:
    ConstantFolder();
    std::unique_ptr<Expression> lastExpr;
    std::unique_ptr<Statement> lastStmt;
    struct ConstantInfo {
        int value;
        std::string type;
        int pointerLevel = 0;
        bool isSigned = false;
    };
    std::map<std::string, ConstantInfo> knownConstants;
    std::map<std::string, TypeInfo::VarInfo> variableTypes;
    std::set<std::string> addressEscapedVars; // variables whose address was taken (non-const)
    std::set<std::string> volatileVars;
    std::set<std::string> constVars;        // non-pointer const vars (prevents x = ...)
    std::set<std::string> constPointerVars;  // pointer-const vars (prevents p = ...)
    std::set<std::string> boolVars;
    std::map<std::string, std::shared_ptr<TypeInfo::StructInfo>> structs;
    std::set<std::string> vmStructs_;  // structs with run-time member sizes
    std::set<std::string> usedVars_;
    // Warnings (OptimizeStage prints them): negative constants stored in or
    // compared with a variable of a plain integer type, which cc45 makes
    // unsigned (VariableDeclaration::isPlainUnsigned). Checked here, before
    // folding and constant propagation can remove the comparison.
    std::vector<std::string> warnings;

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
    void visit(FloatLiteral& node) override {
        lastExpr = copyPos(std::make_unique<FloatLiteral>(node.value), node);
    }

    void visit(StringLiteral& node) override {
        lastExpr = copyPos(StringLiteral::copyOf(node), node);
    }

    void visit(VariableReference& node) override {
        usedVars_.insert(node.name);
        // PHASE 2 FIX: Don't replace variable references with their initialization values.
        // Variables can be modified, and their values can change at runtime.
        // Replacing them with literals breaks code that accesses variables after function calls.
        // Just return the variable reference as-is.
        lastExpr = copyPos(std::make_unique<VariableReference>(node.name), node);
    }

    void visit(Assignment& node) override {
        if (node.op == "=") checkPlainStore(node.target.get(), node.expression.get(), node);
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
                    
                    if (variableTypes.count(ref->name)) {
                        auto& vi = variableTypes[ref->name];
                        knownConstants[ref->name] = { val, vi.type, vi.pointerLevel, vi.isSigned };
                    } else {
                        knownConstants[ref->name] = { val, "int", 0, false };
                    }
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
        checkPlainCompare(node);
        // Phase 89: Check for address template patterns BEFORE constant folding
        // This preserves patterns that would otherwise be optimized away
        static AddressTemplateDetector detector;
        auto pattern = detector.detectPattern(node);

        if (pattern.canOptimize) {
            // Pattern found - preserve original expression without folding
            lastExpr = copyPos(std::make_unique<BinaryOperation>(node.op, std::move(node.left), std::move(node.right)), node);
            return;
        }

        auto left = fold(std::move(node.left));
        auto right = fold(std::move(node.right));

        auto* leftLit = dynamic_cast<IntegerLiteral*>(left.get());
        auto* rightLit = dynamic_cast<IntegerLiteral*>(right.get());

        if (leftLit && rightLit) {
            // (the rules: ConstEval::binary)
            ConstValue r;
            if (ConstEval::binary(node.op, ConstValue::ofLiteral(*leftLit), ConstValue::ofLiteral(*rightLit), r)) {
                lastExpr = copyPos(r.toLiteral(), node);
                return;
            }
            lastExpr = copyPos(std::make_unique<BinaryOperation>(node.op, std::move(left), std::move(right)), node);
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
                // Don't fold the operand — variable reference must survive for store-back
                knownConstants.erase(ref->name);
            }
            lastExpr = copyPos(std::make_unique<UnaryOperation>(node.op, std::move(node.operand)), node);
            return;
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

        ConstValue r;
        if (lit && (node.op == "-" || node.op == "!" || node.op == "~") &&
            ConstEval::unary(node.op, ConstValue::ofLiteral(*lit), r)) {
            lastExpr = copyPos(r.toLiteral(), node);
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

    void visit(CpuRegisterAccess& node) override {
        lastExpr = copyPos(std::make_unique<CpuRegisterAccess>(node.regName), node);
    }

    void visit(CpuFlagAccess& node) override {
        lastExpr = copyPos(std::make_unique<CpuFlagAccess>(node.flagName), node);
    }

    void visit(LabelAddressExpression& node) override {
        lastExpr = copyPos(std::make_unique<LabelAddressExpression>(node.label), node);
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
        result->designators = node.designators;
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
        // (the rules: ConstEval::cast; a cast to a float type gives a FloatLiteral)
        auto* lit = dynamic_cast<IntegerLiteral*>(operand.get());
        ConstValue r;
        if (lit && ConstEval::cast(node.targetType, node.pointerLevel, node.isSigned, ConstValue::ofLiteral(*lit), r)) {
            if (r.isFloat()) lastExpr = copyPos(std::make_unique<FloatLiteral>(r.fvalue), node);
            else lastExpr = copyPos(r.toLiteral(), node);
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
        plainUnsigned_[node.name] = node.isPlainUnsigned ? node.type : std::string();
        if (node.isPlainUnsigned && node.arrayDims.empty() && node.initializer) {
            VariableReference ref(node.name);
            checkPlainStore(&ref, node.initializer.get(), *node.initializer);
        }
        // Track the variable's type even if it's not currently constant
        TypeInfo::VarInfo vi;
        vi.type = node.type;
        vi.pointerLevel = node.pointerLevel;
        vi.isSigned = node.isSigned;
        vi.isVolatile = node.isVolatile;
        vi.isConst = node.isConst;
        vi.isPointerConst = node.isPointerConst;
        vi.arrayDims = node.arrayDims;
        vi.isFunctionPointer = node.isFunctionPointer;
        vi.funcPtrSig = node.funcPtrSig;
        variableTypes[node.name] = vi;

        auto alignmentExpr = node.alignmentExpr ? fold(std::move(node.alignmentExpr)) : nullptr;
        if (alignmentExpr) {
            if (auto* lit = dynamic_cast<IntegerLiteral*>(alignmentExpr.get())) {
                node.alignment = lit->value;
            }
        }
        auto initializer = node.initializer ? fold(std::move(node.initializer)) : nullptr;
        // A declaration hides an outer one of the same name: a non-const
        // local is writable even when it shadows a const global.
        constVars.erase(node.name);
        constPointerVars.erase(node.name);
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
                knownConstants[node.name] = { val, node.type, node.pointerLevel, node.isSigned };
            } else {
                knownConstants.erase(node.name);
            }
        } else {
            knownConstants.erase(node.name);
        }
        auto decl = copyPos(std::make_unique<VariableDeclaration>(node.type, node.name, node.pointerLevel), node);
        decl->isSigned = node.isSigned;
        decl->isPlainUnsigned = node.isPlainUnsigned;
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
        decl->vlaDimExprs = node.vlaDimExprs;
        decl->ptrArrayDims = node.ptrArrayDims;
        decl->isFunctionPointer = node.isFunctionPointer;
        decl->funcPtrSig = node.funcPtrSig;
        decl->addressSpace = node.addressSpace;  // Phase 97: Preserve address space qualifier
        decl->isNoescape = node.isNoescape;
        decl->isStriped = node.isStriped;
        lastStmt = std::move(decl);
    }

    void visit(ReturnStatement& node) override {
        lastStmt = copyPos(std::make_unique<ReturnStatement>(fold(std::move(node.expression))), node);
    }

    void visit(BreakStatement& node) override {
        lastStmt = copyPos(std::make_unique<BreakStatement>(node.levels), node);
    }

    void visit(ContinueStatement& node) override {
        lastStmt = copyPos(std::make_unique<ContinueStatement>(), node);
    }

    void visit(SwitchContinueStatement& node) override {
        lastStmt = copyPos(std::make_unique<SwitchContinueStatement>(node.target ? fold(std::move(node.target)) : nullptr), node);
    }

    void visit(GotoStatement& node) override {
        auto target = node.target ? fold(std::move(node.target)) : nullptr;
        lastStmt = copyPos(std::make_unique<GotoStatement>(node.label, std::move(target)), node);
    }

    void visit(LabelledStatement& node) override {
        lastStmt = copyPos(std::make_unique<LabelledStatement>(node.label, fold(std::move(node.statement))), node);
    }

    void visit(ExpressionStatement& node) override {
        lastStmt = copyPos(std::make_unique<ExpressionStatement>(fold(std::move(node.expression))), node);
    }

    // A statement that can be entered by a jump: it contains a case/default
    // of the enclosing switch or a goto label
    static bool hasJumpTarget(Statement* st, bool inNestedSwitch = false) {
        if (!st) return false;
        if (dynamic_cast<LabelledStatement*>(st)) return true;
        if (!inNestedSwitch && (dynamic_cast<CaseStatement*>(st) || dynamic_cast<DefaultStatement*>(st))) return true;
        if (auto* c = dynamic_cast<CompoundStatement*>(st)) {
            for (auto& x : c->statements) if (hasJumpTarget(x.get(), inNestedSwitch)) return true;
            return false;
        }
        if (auto* i = dynamic_cast<IfStatement*>(st))
            return hasJumpTarget(i->thenBranch.get(), inNestedSwitch) || hasJumpTarget(i->elseBranch.get(), inNestedSwitch);
        if (auto* w = dynamic_cast<WhileStatement*>(st)) return hasJumpTarget(w->body.get(), inNestedSwitch);
        if (auto* d = dynamic_cast<DoWhileStatement*>(st)) return hasJumpTarget(d->body.get(), inNestedSwitch);
        if (auto* f = dynamic_cast<ForStatement*>(st)) return hasJumpTarget(f->body.get(), inNestedSwitch);
        if (auto* r = dynamic_cast<RepeatStatement*>(st)) return hasJumpTarget(r->body.get(), inNestedSwitch);
        if (auto* sw = dynamic_cast<SwitchStatement*>(st)) return hasJumpTarget(sw->body.get(), true);
        return false;
    }

    void visit(IfStatement& node) override {
        auto condition = fold(std::move(node.condition));
        auto* lit = dynamic_cast<IntegerLiteral*>(condition.get());
        // if (0) { ... case 1: ... }: the branch is still reachable by a jump
        if (lit && (hasJumpTarget(node.thenBranch.get()) || hasJumpTarget(node.elseBranch.get()))) lit = nullptr;
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
            auto preIfConstants = knownConstants;
            auto thenBranch = fold(std::move(node.thenBranch));
            auto postThenConstants = knownConstants;

            knownConstants = preIfConstants;
            auto elseBranch = node.elseBranch ? fold(std::move(node.elseBranch)) : nullptr;
            auto postElseConstants = knownConstants;

            knownConstants.clear();
            for (auto const& [name, info] : postThenConstants) {
                if (postElseConstants.count(name)) {
                    auto& other = postElseConstants[name];
                    if (info.value == other.value && info.type == other.type && 
                        info.pointerLevel == other.pointerLevel && info.isSigned == other.isSigned) {
                        knownConstants[name] = info;
                    }
                }
            }
            auto ifs = std::make_unique<IfStatement>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
            ifs->hint = node.hint;
            lastStmt = copyPos(std::move(ifs), node);
        }
    }

    void visit(WhileStatement& node) override;
    void visit(DoWhileStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(RepeatStatement& node) override;

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
        std::unique_ptr<Expression> rangeEnd;
        if (node.rangeEnd) rangeEnd = fold(std::move(node.rangeEnd));
        auto newNode = std::make_unique<CaseStatement>(std::move(value), std::move(rangeEnd));
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
        int startOffset = 0, startAlign = 1, startNatural = 1;

        auto sInfo = std::make_shared<TypeInfo::StructInfo>();
        sInfo->name = node.name;

        auto structInfoOf = [&](const std::string& type) -> const TypeInfo::StructInfo* {
            std::string tn = type;
            for (const char* kw : {"struct ", "union "})
                if (tn.rfind(kw, 0) == 0) tn = tn.substr(std::string(kw).size());
            auto sit = structs.find(tn);
            return sit != structs.end() ? sit->second.get() : nullptr;
        };
        // Natural alignment of a member type (as IRBuilder::naturalAlign)
        auto naturalAlign = [&](const std::string& type, int ptrLevel) {
            if (ptrLevel > 0) return 2;
            if (type.rfind("struct ", 0) == 0 || type.rfind("union ", 0) == 0) {
                const TypeInfo::StructInfo* si = structInfoOf(type);
                return si ? std::max(1, si->naturalAlign) : 1;
            }
            if (type == "float" || type == "double" || type == "long double") return 1;
            int n = TypeInfo::getTypeSize(type, 0, -1, structs);
            return (n == 2 || n == 4) ? n : 1;
        };

        // struct D : B { ... }: B's members come first, at their own offsets
        // (as IRBuilder lays it out)
        if (!node.parentStruct.empty()) {
            auto pit = structs.find(node.parentStruct);
            if (pit != structs.end() && pit->second) {
                sInfo->members = pit->second->members;
                startOffset = pit->second->totalSize;
                startAlign = pit->second->alignment;
                startNatural = pit->second->naturalAlign;
            }
        }

        std::vector<StructLayout::Member> lm;
        std::vector<std::unique_ptr<Expression>> alignExprs;
        for (auto& m : node.members) {
            auto alignmentExpr = m.alignmentExpr ? fold(std::move(m.alignmentExpr)) : nullptr;
            // __attribute__((aligned)) arrives as m.alignment, _Alignas as an expression
            int alignment = m.alignment > 1 ? m.alignment : 1;
            if (alignmentExpr) {
                if (auto* lit = dynamic_cast<IntegerLiteral*>(alignmentExpr.get())) {
                    if (lit->value > alignment) alignment = (int)lit->value;
                }
            }
            StructLayout::Member l;
            if (m.bitWidth > 0) {
                l.unitType = m.type;
                l.size = TypeInfo::getTypeSize(m.type, 0, -1, structs);
                l.bitWidth = m.bitWidth;
                l.naturalAlign = naturalAlign(m.type, 0);
            } else {
                l.size = TypeInfo::getTypeSize(m.type, m.pointerLevel, m.arraySize(), structs);
                l.explicitAlign = alignment;
                // A member of an aligned struct type takes that struct's alignment
                if (m.pointerLevel == 0)
                    if (const TypeInfo::StructInfo* si = structInfoOf(m.type))
                        l.explicitAlign = std::max(l.explicitAlign, si->alignment);
                l.naturalAlign = naturalAlign(m.type, m.pointerLevel);
            }
            lm.push_back(l);
            alignExprs.push_back(std::move(alignmentExpr));
        }
        StructLayout::Result lr = StructLayout::layout(lm, node.isUnion, node.isUnpacked, node.alignment,
                                                       startOffset, startAlign, startNatural);

        for (size_t i = 0; i < node.members.size(); i++) {
            auto& m = node.members[i];
            TypeInfo::MemberInfo mi;
            mi.type = m.type; mi.pointerLevel = m.pointerLevel; mi.isSigned = m.isSigned;
            mi.isConst = m.isConst; mi.offset = lr.placed[i].offset; mi.alignment = lm[i].explicitAlign;
            mi.arrayDims = m.arrayDims;
            sInfo->members[m.name] = mi;

            StructMember sm;
            sm.type = m.type; sm.pointerLevel = m.pointerLevel; sm.isSigned = m.isSigned;
            sm.name = m.name; sm.isConst = m.isConst;
            sm.alignment = m.alignment > 1 ? m.alignment : 1;
            if (auto* lit = dynamic_cast<IntegerLiteral*>(alignExprs[i].get()))
                if (lit->value > sm.alignment) sm.alignment = (int)lit->value;
            sm.alignmentExpr = std::move(alignExprs[i]); sm.isAnonymous = m.isAnonymous;
            sm.arrayDims = m.arrayDims; sm.bitWidth = m.bitWidth;
            sm.isFlexible = m.isFlexible;
            sm.funcPtrSig = m.funcPtrSig;
            for (auto& d : m.vlaDimExprs) {
                if (d) vmStructs_.insert(node.name);
                sm.vlaDimExprs.push_back(d ? fold(std::move(d)) : nullptr);
            }
            def->members.push_back(std::move(sm));
        }

        sInfo->totalSize = lr.size;
        sInfo->alignment = lr.align;
        sInfo->naturalAlign = lr.naturalAlign;
        structs[node.name] = sInfo;

        // Preserve layout and OOP fields
        def->isUnpacked = node.isUnpacked;
        def->alignment = node.alignment;
        def->parentStruct = node.parentStruct;
        def->hasVirtual = node.hasVirtual;
        def->isFinal = node.isFinal;
        for (auto& m : node.methods) def->methods.push_back(std::move(m));

        lastStmt = std::move(def);
    }

    void visit(StatementExpression& node) override {
        auto r = std::make_unique<StatementExpression>();
        if (node.body) node.body->accept(*this);  // folded in place
        r->body = std::move(node.body);
        r->result = fold(std::move(node.result));
        lastExpr = copyPos(std::move(r), node);
    }

    void visit(CompoundStatement& node) override {
        // Track last dead-store-candidate index per variable for elimination.
        // A store is "dead" if a subsequent constant store overwrites it with no
        // intervening read. We conservatively clear all candidates whenever we
        // encounter any statement that isn't a simple constant assignment, since
        // it could read any variable (function calls, control flow, etc.).
        std::map<std::string, int> lastConstStore;
        std::vector<std::unique_ptr<Statement>> newStatements;
        ConstScope scope(*this);   // the block's declarations end with it
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
    }

    void visit(FunctionDeclaration& node) override {
        knownConstants.clear(); // Fresh state for new function
        boolVars.clear();
        auto savedPlain = plainUnsigned_;   // the function's locals stay in it
        ConstScope scope(*this);
        // Record parameter types
        for (const auto& param : node.parameters) {
            plainUnsigned_[param.name].clear();
            constVars.erase(param.name);        // hides a const global
            constPointerVars.erase(param.name);
            TypeInfo::VarInfo vi;
            vi.type = param.type;
            vi.pointerLevel = param.pointerLevel;
            vi.isSigned = param.isSigned;
            vi.isVolatile = param.isVolatile;
            vi.isConst = param.isConst;
            vi.isPointerConst = param.isPointerConst;
            vi.isFunctionPointer = param.isFunctionPointer;
            vi.funcPtrSig = param.funcPtrSig;
            variableTypes[param.name] = vi;
        }
        // The body is a CompoundStatement, which is modified in-place by its visit method.
        // We just need to call accept on it directly, not fold it via the helper.
        if (node.body) node.body->accept(*this);
        plainUnsigned_ = std::move(savedPlain);
        // We don't set lastStmt here, as FunctionDeclaration is modified in place
    }

    void visit(TranslationUnit& node) override;

    std::unique_ptr<TranslationUnit> foldTranslationUnit(std::unique_ptr<TranslationUnit> unit);
    std::unique_ptr<ASTNode> fold(std::unique_ptr<ASTNode> node);

private:
    // Saves the const name sets and puts them back at the end of a scope.
    struct ConstScope {
        ConstantFolder& cf;
        std::set<std::string> c, cp;
        explicit ConstScope(ConstantFolder& f) : cf(f), c(f.constVars), cp(f.constPointerVars) {}
        ~ConstScope() { cf.constVars = std::move(c); cf.constPointerVars = std::move(cp); }
    };
    friend struct ConstScope;

    // name -> its declared type when it is a plain (default-unsigned) integer, else ""
    std::map<std::string, std::string> plainUnsigned_;
    std::set<std::string> warned_;

    std::string plainVarType(Expression* e, std::string& name) {
        auto* vr = dynamic_cast<VariableReference*>(e);
        if (!vr) return "";
        name = vr->name;
        auto it = plainUnsigned_.find(vr->name);
        return it == plainUnsigned_.end() ? "" : it->second;
    }
    static bool negativeConstant(Expression* e, int64_t& v) {
        if (auto* lit = dynamic_cast<IntegerLiteral*>(e)) {
            if (lit->value < 0 && lit->castType.empty()) { v = lit->value; return true; }
            return false;
        }
        if (auto* u = dynamic_cast<UnaryOperation*>(e); u && u->op == "-")
            if (auto* lit = dynamic_cast<IntegerLiteral*>(u->operand.get()); lit && lit->value > 0 && lit->castType.empty()) {
                v = -lit->value;
                return true;
            }
        return false;
    }
    static bool isZero(Expression* e) {
        auto* lit = dynamic_cast<IntegerLiteral*>(e);
        return lit && lit->value == 0;
    }
    // what v becomes in an unsigned variable of this plain type
    static std::string unsignedImage(int64_t v, const std::string& type) {
        int bits = type == "char" ? 8 : type == "long" ? 32 : 16;
        return std::to_string((uint64_t)v & ((1ull << bits) - 1));
    }
    void warnPlain(const ASTNode& at, const std::string& name, const std::string& type, const std::string& what) {
        std::string key = at.sourceFile + ":" + std::to_string(at.line) + ":" + std::to_string(at.column) + ":" + name;
        if (!warned_.insert(key).second) return;
        warnings.push_back(formatDiagnostic(at.sourceFile, at.line, at.column, Severity::Warning,
            "'" + name + "' is unsigned (plain " + type + " is unsigned in cc45): " + what +
            "; declare it 'signed' (or use -fsigned-default)"));
    }
    // x = -1 / int x = -1
    void checkPlainStore(Expression* target, Expression* value, const ASTNode& at) {
        std::string name;
        std::string type = plainVarType(target, name);
        int64_t v;
        if (!type.empty() && negativeConstant(value, v))
            warnPlain(at, name, type, std::to_string(v) + " becomes " + unsignedImage(v, type));
    }
    // x < 0, x >= 0, x == -1, x <= -1 ...
    void checkPlainCompare(BinaryOperation& node) {
        static const std::set<std::string> cmpOps = {"<", "<=", ">", ">=", "==", "!="};
        if (!cmpOps.count(node.op)) return;
        for (int side = 0; side < 2; side++) {
            Expression* var = side ? node.right.get() : node.left.get();
            Expression* other = side ? node.left.get() : node.right.get();
            std::string name;
            std::string type = plainVarType(var, name);
            if (type.empty()) continue;
            int64_t v;
            // the operator as if the variable were on the left
            std::string op = node.op;
            if (side) op = op == "<" ? ">" : op == ">" ? "<" : op == "<=" ? ">=" : op == ">=" ? "<=" : op;
            if (isZero(other) && (op == "<" || op == ">="))
                warnPlain(node, name, type, "'" + name + " " + op + " 0' is always " + (op == "<" ? "false" : "true"));
            else if (negativeConstant(other, v))
                warnPlain(node, name, type, "compared with " + std::to_string(v) + ", which becomes " + unsignedImage(v, type));
        }
    }

    template<typename T, typename U>
    std::unique_ptr<T> copyPos(std::unique_ptr<T> newNode, const U& oldNode) {
        newNode->line = oldNode.line;
        newNode->column = oldNode.column;
        newNode->sourceFile = oldNode.sourceFile;
        return newNode;
    }
};
