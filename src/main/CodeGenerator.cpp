#include "CodeGenerator.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <set>

CodeGenerator::CodeGenerator(std::ostream& out) : out(out) {}

void CodeGenerator::setSourceInfo(const std::string& filename, const std::vector<std::string>& lines) {
    sourceFilename = filename;
    sourceLines = lines;
}

void CodeGenerator::embedSource(ASTNode& node) {
    if (node.line <= 0 || node.line > (int)sourceLines.size()) return;
    if (node.line == lastEmbeddedLine) return;
    lastEmbeddedLine = node.line;
    out << "; [" << sourceFilename << ":" << node.line << "] " << sourceLines[node.line - 1] << std::endl;
}

void CodeGenerator::emitNarrowingWarning(ASTNode& node, const std::string& fromType, int fromPtr, const std::string& toType, int toPtr) {
    if (toType == "_Bool") return; // conversion to _Bool is always valid (C99)
    int fromSize = (fromPtr > 0 || fromType == "int") ? 2 : 1;
    int toSize = (toPtr > 0 || toType == "int") ? 2 : 1;
    if (fromSize > toSize) {
        std::string loc = sourceFilename.empty() ? "" : sourceFilename + ":";
        if (node.line > 0) loc += std::to_string(node.line) + ":" + std::to_string(node.column) + ": ";
        std::string msg = loc + "warning: implicit conversion loses data from '" +
            fromType + (fromPtr > 0 ? std::string(fromPtr, '*') : "") + "' to '" +
            toType + (toPtr > 0 ? std::string(toPtr, '*') : "") + "'";
        warnings.push_back(msg);
        std::cerr << msg << std::endl;
    }
}

void CodeGenerator::emitBoolNormalize(int srcSize) {
    // Normalize value in A (8-bit) or A:X (16-bit) to 0 or 1
    std::string labelOne = newDontCareLabel();
    std::string labelDone = newDontCareLabel();
    if (srcSize == 2) {
        emitter->cpx_imm(0);
        emitter->bne_label(labelOne);
    }
    emitter->cmp_imm(0);
    emitter->bne_label(labelOne);
    emitter->lda_imm(0);
    emitter->bra_label(labelDone);
    emitter->emitLabel(labelOne);
    emitter->lda_imm(1);
    emitter->emitLabel(labelDone);
    invalidateRegs();
}

void CodeGenerator::generate(TranslationUnit& unit) {
    emitter = std::make_unique<M65Emitter>(out, zeroPageStart);
    zpRegs.clear();
    for (uint32_t i = 0; i < zeroPageAvail; ++i) zpRegs.push_back({false});
    invalidateRegs();
    resultNeeded = true;
    unit.accept(*this);
    emitData();
}

void CodeGenerator::emit(const std::string& line) {
    out << "    " << line << std::endl;
}

void CodeGenerator::emitBranch16Beq(const std::string& target) {
    // Branch to target if 16-bit A:X == 0 (assumes cmp #$00 already set flags for A)
    std::string skip = newDontCareLabel();
    emit("bne " + skip);     // low byte != 0 → not zero, skip
    emit("cpx #$00");        // check high byte
    emit("beq " + target);   // both zero → branch
    out << skip << ":" << std::endl;
}

void CodeGenerator::emitBranch16Bne(const std::string& target) {
    // Branch to target if 16-bit A:X != 0 (assumes cmp #$00 already set flags for A)
    emit("bne " + target);   // low byte != 0 → non-zero, branch
    emit("cpx #$00");        // check high byte
    emit("bne " + target);   // high byte != 0 → non-zero, branch
}

std::string CodeGenerator::newLabel() {
    return "L" + std::to_string(labelCount++);
}

std::string CodeGenerator::newDontCareLabel() {
    return "@L" + std::to_string(labelCount++);
}

bool CodeGenerator::isStruct(const std::string& type) {
    if (type.length() >= 7 && type.substr(0, 7) == "struct ") return true;
    if (type.length() >= 6 && type.substr(0, 6) == "union ") return true;
    return false;
}

bool CodeGenerator::isEnum(const std::string& type) {
    if (type.length() >= 5 && type.substr(0, 5) == "enum ") return true;
    return false;
}

std::string CodeGenerator::getAggregateName(const std::string& type) {
    if (type.length() >= 7 && type.substr(0, 7) == "struct ") return type.substr(7);
    if (type.length() >= 6 && type.substr(0, 6) == "union ") return type.substr(6);
    return type;
}

std::string CodeGenerator::resolveVarName(const std::string& name) {
    if (name.length() >= 3) {
        std::string prefix = name.substr(0, 3);
        if (prefix == "_p_" || prefix == "_l_") {
            return name;
        }
    }
    std::string pName = "_p_" + name;
    if (variableTypes.count(pName)) return pName;
    std::string lName = "_l_" + name;
    if (variableTypes.count(lName)) return lName;
    std::string gName = "_" + name;
    if (globalVariableTypes.count(gName)) return gName;
    return name;
}

bool CodeGenerator::matchType(const ExpressionType& t1, const std::string& t2Name, int t2Ptr) {
    if (t1.type != t2Name) return false;
    if (t1.pointerLevel != t2Ptr) return false;
    return true;
}

int CodeGenerator::getTypeSize(const std::string& type, int ptrLevel, int arraySize, const std::map<std::string, std::shared_ptr<CodeGenerator::StructInfo>>& structs) {
    int size = 0;
    if (ptrLevel > 0) size = 2;
    else if (type == "char") size = 1;
    else if (type == "_Bool") size = 1;
    else if (type == "int") size = 2;
    else if (type.length() >= 5 && type.substr(0, 5) == "enum ") size = 2;
    else if (type.substr(0, 7) == "struct " || type.substr(0, 6) == "union ") {
        std::string sName = type.substr(type.find(' ') + 1);
        if (structs.count(sName)) size = structs.at(sName)->totalSize;
        else throw std::runtime_error("Unknown struct/union type: " + sName);
    }
    if (arraySize >= 0) size *= arraySize;
    return size;
}

CodeGenerator::ExpressionType CodeGenerator::getExprType(Expression* expr) {
    if (!expr) return {"int", 0, false, false};
    if (auto* cast = dynamic_cast<CastExpression*>(expr)) {
        return {cast->targetType, cast->pointerLevel, cast->isSigned, false};
    }
    if (auto* gs = dynamic_cast<GenericSelection*>(expr)) {
        // Resolve based on control type
        ExpressionType controlType = getExprType(gs->control.get());
        for (auto& assoc : gs->associations) {
            if (!assoc.isDefault && CodeGenerator::matchType(controlType, assoc.typeName, assoc.pointerLevel)) {
                return getExprType(assoc.result.get());
            }
        }
        for (auto& assoc : gs->associations) {
            if (assoc.isDefault) return getExprType(assoc.result.get());
        }
        throw std::runtime_error("No matching association in _Generic selection");
    }
    if (auto* ref = dynamic_cast<VariableReference*>(expr)) {
        std::string pName = "_p_" + ref->name;
        if (variableTypes.count(pName)) {
            VarInfo& vi = variableTypes.at(pName);
            int pl = vi.pointerLevel + (int)vi.arrayDims.size();
            return {vi.type, pl, vi.isSigned, vi.isConst, vi.isPointerConst};
        }
        std::string rName = "_l_" + ref->name;
        if (variableTypes.count(rName)) {
            VarInfo& vi = variableTypes.at(rName);
            int pl = vi.pointerLevel + (int)vi.arrayDims.size();
            return {vi.type, pl, vi.isSigned, vi.isConst, vi.isPointerConst};
        }
        if (globalVariableTypes.count("_" + ref->name)) {
            VarInfo& vi = globalVariableTypes.at("_" + ref->name);
            int pl = vi.pointerLevel + (int)vi.arrayDims.size();
            return {vi.type, pl, vi.isSigned, vi.isConst, vi.isPointerConst};
        }
    }
    if (auto* vd = dynamic_cast<VariableDeclaration*>(expr)) {
        return {vd->type, vd->pointerLevel, vd->isSigned, vd->isConst, vd->isPointerConst};
    }
    if (auto* aa = dynamic_cast<ArrayAccess*>(expr)) {
        ExpressionType base = getExprType(aa->arrayExpr.get());
        if (base.pointerLevel > 0) return {base.type, base.pointerLevel - 1, base.isSigned, base.isConst};
        return {base.type, 0, base.isSigned, base.isConst};
    }
    if (auto* cond = dynamic_cast<ConditionalExpression*>(expr)) {
        return getExprType(cond->thenExpr.get());
    }
    if (auto* bin = dynamic_cast<BinaryOperation*>(expr)) { 
        ExpressionType lhs = getExprType(bin->left.get());
        ExpressionType rhs = getExprType(bin->right.get());
        bool resSigned = lhs.isSigned || rhs.isSigned;
        return {lhs.type, lhs.pointerLevel, resSigned, lhs.isConst};
    }
    if (auto* un = dynamic_cast<UnaryOperation*>(expr)) {
        if (un->op == "!") return {"char", 0, false, false};
        CodeGenerator::ExpressionType sub = getExprType(un->operand.get());
        if (un->op == "*") return {sub.type, sub.pointerLevel > 0 ? sub.pointerLevel - 1 : 0, sub.isSigned, sub.isConst};
        if (un->op == "&") return {sub.type, sub.pointerLevel + 1, sub.isSigned, sub.isConst};
        return sub;
    }
    if (auto* ma = dynamic_cast<MemberAccess*>(expr)) {
        ExpressionType baseType = getExprType(ma->structExpr.get());
        if (!isStruct(baseType.type)) {
             if (auto* ref = dynamic_cast<VariableReference*>(ma->structExpr.get())) {
                 if (globalVariableTypes.count("_" + ref->name)) {
                     VarInfo& gv = globalVariableTypes.at("_" + ref->name);
                     baseType = {gv.type, gv.pointerLevel, gv.isSigned, gv.isConst};
                 }
             }
        }
        if (isStruct(baseType.type)) {
            std::string sName = getAggregateName(baseType.type);
            if (structs.count(sName)) {
                auto& sInfo = *structs[sName];
                if (sInfo.members.count(ma->memberName)) {
                    MemberInfo& mInfo = sInfo.members[ma->memberName];
                    if (mInfo.arraySize() >= 0) return {mInfo.type, mInfo.pointerLevel + 1, mInfo.isSigned, mInfo.isConst};
                    return {mInfo.type, mInfo.pointerLevel, mInfo.isSigned, mInfo.isConst};
                }
            }
        }
    }
    return {"int", 0, false, false};
}

void CodeGenerator::emitAddress(Expression* expr) {
    if (auto* ref = dynamic_cast<VariableReference*>(expr)) {
        emit("ptrstack " + resolveVarName(ref->name));
    } else if (auto* ma = dynamic_cast<MemberAccess*>(expr)) {
        if (ma->isArrow) {
            bool oldNeeded = resultNeeded;
            resultNeeded = true;
            ma->structExpr->accept(*this);
            resultNeeded = oldNeeded;
        } else {
            emitAddress(ma->structExpr.get());
        }
        ExpressionType baseType = getExprType(ma->structExpr.get());
        if (!isStruct(baseType.type)) {
             if (auto* ref = dynamic_cast<VariableReference*>(ma->structExpr.get())) {
                 if (globalVariableTypes.count("_" + ref->name)) {
                     baseType = {globalVariableTypes.at("_" + ref->name).type, globalVariableTypes.at("_" + ref->name).pointerLevel};
                 }
             }
        }
        if (isStruct(baseType.type)) {
            std::string sName = getAggregateName(baseType.type);
            if (structs.count(sName)) {
                auto& sInfo = *structs[sName];
                if (sInfo.members.count(ma->memberName)) {
                    MemberInfo& mInfo = sInfo.members[ma->memberName];
                    if (mInfo.offset > 0) {
                        emitter->add_16_imm(mInfo.offset);
                    }
                }
            }
        }
    } else if (auto* aa = dynamic_cast<ArrayAccess*>(expr)) {
        bool oldNeeded = resultNeeded;
        resultNeeded = true;
        // For chained array access (multi-dim), recurse via emitAddress to get
        // the sub-array address without dereferencing
        if (dynamic_cast<ArrayAccess*>(aa->arrayExpr.get())) {
            emitAddress(aa->arrayExpr.get());
        } else {
            aa->arrayExpr->accept(*this); // Get base address in AX (pointer decay)
        }
        resultNeeded = oldNeeded;
        int zpBase = allocateZP(2);
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpBase);
        emit("stax $" + ss.str());

        resultNeeded = true;
        aa->indexExpr->accept(*this); // Get index in AX
        resultNeeded = oldNeeded;

        // Compute element size (stride) for this dimension level
        // For multi-dim arrays like int a[3][4], a[i] has stride = 4*sizeof(int) = 8
        int elementSize = 1;
        {
            // Walk the ArrayAccess chain to find the root variable and depth
            int depth = 0;
            Expression* base = aa->arrayExpr.get();
            while (auto* inner = dynamic_cast<ArrayAccess*>(base)) {
                depth++;
                base = inner->arrayExpr.get();
            }

            std::vector<int> dims;
            VarInfo* vi = nullptr;
            if (auto* ref = dynamic_cast<VariableReference*>(base)) {
                std::string rName = resolveVarName(ref->name);
                if (variableTypes.count(rName)) vi = &variableTypes.at(rName);
                else if (globalVariableTypes.count(rName)) vi = &globalVariableTypes.at(rName);
                if (vi) dims = vi->arrayDims;
            }

            if (!dims.empty() && depth < (int)dims.size() && vi) {
                // Multi-dim array: stride = product(dims[depth+1..]) * scalar_element_size
                int scalarSize = getTypeSize(vi->type, vi->pointerLevel, -1, structs);
                elementSize = scalarSize;
                for (int d = depth + 1; d < (int)dims.size(); d++)
                    elementSize *= dims[d];
            } else {
                // Plain pointer indexing or single-dim (fallback to original behavior)
                ExpressionType baseType = getExprType(aa->arrayExpr.get());
                if (baseType.pointerLevel > 1 || baseType.type == "int") {
                    elementSize = 2;
                } else if (isStruct(baseType.type)) {
                    std::string sName = getAggregateName(baseType.type);
                    if (structs.count(sName)) elementSize = structs[sName]->totalSize;
                }
            }
        }

        if (elementSize == 2) {
            emitter->asl_a(); emitter->pha(); emitter->txa(); emitter->rol_a(); emitter->tax(); emitter->pla();
        } else if (elementSize > 2) {
             // Index is in AX; multiply by stride using immediate operand
             std::stringstream ssVal; ssVal << "#$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << elementSize;
             emit("mul.16 .ax, " + ssVal.str());
        }

        std::stringstream ss2;
        ss2 << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpBase);
        emit("add.16 .ax, $" + ss2.str());
        freeZP(zpBase, 2);
    } else if (auto* un = dynamic_cast<UnaryOperation*>(expr)) {
        if (un->op == "*") {
            bool oldNeeded = resultNeeded;
            resultNeeded = true;
            un->operand->accept(*this);
            resultNeeded = oldNeeded;
        }
    }
    invalidateRegs();
}

void CodeGenerator::emitIndirectIncDec(UnaryOperation& node, bool isInc, bool isPost) {
    // Handle ++/-- on indirect lvalues: (*p)++, arr[i]--, p->field++, etc.
    // Strategy: compute lvalue address → ZP, load value, inc/dec, store back.
    ExpressionType valType = getExprType(node.operand.get());
    if (valType.isConst) throw std::runtime_error("Compile Error: Increment/decrement of read-only location");
    bool is16 = (valType.pointerLevel > 0 || valType.type == "int");
    if (isStruct(valType.type)) {
        std::string sName = getAggregateName(valType.type);
        if (structs.count(sName) && structs[sName]->totalSize > 1) is16 = true;
    }

    // Compute the address of the lvalue into AX
    bool oldNeeded = resultNeeded;
    resultNeeded = true;
    emitAddress(node.operand.get());
    resultNeeded = oldNeeded;

    int zpAddr = allocateZP(2);
    std::stringstream ssAddr;
    ssAddr << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpAddr);
    emit("stax $" + ssAddr.str());

    // Load current value through pointer
    if (is16) {
        emit("ptrderef $" + ssAddr.str());
    } else {
        emitter->lda_ind_z(emitter->getZP(zpAddr), false);
        updateRegY(0); emitter->ldx_imm(0); updateRegX(0);
    }

    if (isPost && resultNeeded) {
        // Save old value for postfix return
        emitter->push_ax();
    }

    // Increment or decrement the value in AX
    if (is16) {
        if (isInc) { emitter->add_16_imm(1); }
        else { emitter->sub_16_imm(1); }
    } else {
        if (isInc) { emitter->clc(); emitter->adc_imm(1); }
        else { emitter->sec(); emitter->sbc_imm(1); }
    }

    // Store updated value back through the pointer
    emitter->sta_ind_z(emitter->getZP(zpAddr), false);
    if (is16) {
        emitter->txa();
        emitter->ldy_imm(1);
        emit("sta ($" + ssAddr.str() + "),y");
    }

    if (isPost && resultNeeded) {
        // Restore old value as the expression result
        emitter->pop_ax();
    } else if (!isPost && resultNeeded) {
        // Prefix: reload the updated value (already in AX for 8-bit, need reload for 16-bit)
        if (is16) {
            emit("ptrderef $" + ssAddr.str());
        } else {
            emitter->lda_ind_z(emitter->getZP(zpAddr), false);
            updateRegY(0); emitter->ldx_imm(0); updateRegX(0);
        }
    }

    freeZP(zpAddr, 2);
    invalidateRegs();
}

// Collect all FunctionCall names from an AST subtree.
namespace {
class CallCollector : public ASTVisitor {
public:
    std::set<std::string> calledFunctions;
    void visit(FunctionCall& node) override {
        calledFunctions.insert(node.name);
        for (auto& arg : node.arguments) arg->accept(*this);
    }
    // Walk all node types that can contain expressions
    void visit(IntegerLiteral&) override {}
    void visit(StringLiteral&) override {}
    void visit(VariableReference&) override {}
    void visit(Assignment& n) override { n.target->accept(*this); n.expression->accept(*this); }
    void visit(BinaryOperation& n) override { n.left->accept(*this); n.right->accept(*this); }
    void visit(UnaryOperation& n) override { n.operand->accept(*this); }
    void visit(ConditionalExpression& n) override { n.condition->accept(*this); n.thenExpr->accept(*this); n.elseExpr->accept(*this); }
    void visit(GenericSelection& n) override { n.control->accept(*this); for (auto& a : n.associations) a.result->accept(*this); }
    void visit(ArrayAccess& n) override { n.arrayExpr->accept(*this); n.indexExpr->accept(*this); }
    void visit(MemberAccess& n) override { n.structExpr->accept(*this); }
    void visit(CastExpression& n) override { n.expression->accept(*this); }
    void visit(AlignofExpression&) override {}
    void visit(SizeofExpression& n) override { if (n.expression) n.expression->accept(*this); }
    void visit(VariableDeclaration& n) override { if (n.initializer) n.initializer->accept(*this); }
    void visit(ReturnStatement& n) override { if (n.expression) n.expression->accept(*this); }
    void visit(BreakStatement&) override {}
    void visit(ContinueStatement&) override {}
    void visit(SwitchContinueStatement& n) override { if (n.target) n.target->accept(*this); }
    void visit(GotoStatement&) override {}
    void visit(LabelledStatement& n) override { n.statement->accept(*this); }
    void visit(ExpressionStatement& n) override { n.expression->accept(*this); }
    void visit(IfStatement& n) override { n.condition->accept(*this); n.thenBranch->accept(*this); if (n.elseBranch) n.elseBranch->accept(*this); }
    void visit(WhileStatement& n) override { n.condition->accept(*this); n.body->accept(*this); }
    void visit(DoWhileStatement& n) override { n.body->accept(*this); n.condition->accept(*this); }
    void visit(ForStatement& n) override { if (n.initializer) n.initializer->accept(*this); if (n.condition) n.condition->accept(*this); if (n.increment) n.increment->accept(*this); n.body->accept(*this); }
    void visit(SwitchStatement& n) override { n.expression->accept(*this); n.body->accept(*this); }
    void visit(CaseStatement& n) override { n.value->accept(*this); }
    void visit(DefaultStatement&) override {}
    void visit(AsmStatement&) override {}
    void visit(StaticAssert&) override {}
    void visit(EnumDefinition&) override {}
    void visit(StructDefinition&) override {}
    void visit(CompoundStatement& n) override { for (auto& s : n.statements) s->accept(*this); }
    void visit(FunctionDeclaration& n) override { n.body->accept(*this); }
    void visit(TranslationUnit& n) override { for (auto& d : n.topLevelDecls) d->accept(*this); }
};
}

void CodeGenerator::visit(TranslationUnit& node) {
    out << "; Generated by cc45" << std::endl;
    out << ".segmentOrder code, data, bss" << std::endl;
    out << ".code" << std::endl;

    // Collect all known function names (definitions + prototypes) for call validation
    knownFunctions.clear();
    for (auto& decl : node.topLevelDecls) {
        if (auto* fn = dynamic_cast<FunctionDeclaration*>(decl.get())) {
            knownFunctions.insert(fn->name);
            std::vector<VarInfo> paramTypes;
            for (auto& p : fn->parameters) {
                paramTypes.push_back({p.type, p.pointerLevel, p.isSigned, p.isVolatile, p.isConst, p.isPointerConst});
            }
            functionParamTypes[fn->name] = std::move(paramTypes);
        }
    }

    if (relocMode) {
        // Collect defined functions, weak functions, and called functions
        std::set<std::string> definedFunctions;
        std::set<std::string> weakFunctions;
        bool nextIsWeak = false;
        for (auto& decl : node.topLevelDecls) {
            if (auto* asmStmt = dynamic_cast<AsmStatement*>(decl.get())) {
                if (asmStmt->code == ".weak_next") { nextIsWeak = true; continue; }
                if (asmStmt->code == ".crt_no_0100_stack") { crtNoPageOneStack = true; continue; }
                if (asmStmt->code == ".crt_exit_halt") { crtExit = CrtExit::HALT; continue; }
                if (asmStmt->code == ".crt_exit_rts") { crtExit = CrtExit::RTS; continue; }
                if (asmStmt->code == ".crt_exit_brk") { crtExit = CrtExit::BRK; continue; }
                if (asmStmt->code == ".crt_no_bssinit") { crtNoBssInit = true; continue; }
            }
            if (auto* fn = dynamic_cast<FunctionDeclaration*>(decl.get())) {
                if (!fn->isPrototype) {
                    definedFunctions.insert(fn->name);
                    if (nextIsWeak) { weakFunctions.insert(fn->name); }
                }
            }
            nextIsWeak = false;
        }

        CallCollector cc;
        node.accept(cc);

        // Emit .extern for called-but-not-defined functions
        for (const auto& name : cc.calledFunctions) {
            if (!definedFunctions.count(name)) {
                out << ".extern _" << name << std::endl;
            }
        }

        // Emit .global/.weak for defined functions
        for (const auto& name : definedFunctions) {
            if (weakFunctions.count(name)) {
                out << ".weak _" << name << std::endl;
            } else {
                out << ".global _" << name << std::endl;
            }
        }

        // .global for global variables will be emitted by emitData()

        // Emit .extern __sp_base when stack relocation is requested
        if (crtNoPageOneStack) {
            out << ".extern __sp_base" << std::endl;
        }

        out << std::endl;
    } else {
        out << ".org $2000" << std::endl;

        // Emit startup stub if a main function is present
        bool hasMain = false;
        for (auto& decl : node.topLevelDecls) {
            if (auto* fn = dynamic_cast<FunctionDeclaration*>(decl.get())) {
                if (fn->name == "main") { hasMain = true; break; }
            }
        }
        if (hasMain) {
            // Inline crt0: __init calls _init_features (weak stub), then _main
            // Returns to caller with .AX = main's return value
            if (crtExit == CrtExit::RTS) {
                // Save caller's SP into the immediate operands of the restore instructions
                out << "    tsx" << std::endl;
                out << "    stx __saved_spl + 1" << std::endl;
                out << "    tsy" << std::endl;
                out << "    sty __saved_sph + 1" << std::endl;
            }
            out << "    jsr __init" << std::endl;
            out << "__exit:" << std::endl;
            switch (crtExit) {
                case CrtExit::RTS:
                    // Restore caller's SP (self-modified immediates) and return
                    out << "__saved_spl:" << std::endl;
                    out << "    ldx #$ff" << std::endl;
                    out << "    txs" << std::endl;
                    out << "__saved_sph:" << std::endl;
                    out << "    ldy #$01" << std::endl;
                    out << "    tys" << std::endl;
                    out << "    rts" << std::endl;
                    break;
                case CrtExit::BRK:
                    out << "    brk" << std::endl;
                    break;
                default: // HALT
                    out << "_halt:" << std::endl;
                    out << "    bra _halt" << std::endl;
                    break;
            }
            out << "__init:" << std::endl;
            out << "    jsr _init_bss" << std::endl;
            out << "    jsr _init_features" << std::endl;
            out << "    jmp _main" << std::endl;
            out << "_init_features:" << std::endl;
            out << "    rts" << std::endl;
            // Emit _init_bss inline in CRT area so its address is stable
            // (avoids simulated opcode size drift when placed after functions).
            // Pre-scan for uninitialized globals to decide if BSS zeroing is needed.
            bool hasBssVars = false;
            if (!crtNoBssInit && !relocMode) {
                for (auto& decl : node.topLevelDecls) {
                    if (auto* vd = dynamic_cast<VariableDeclaration*>(decl.get())) {
                        if ((vd->isGlobal || currentFunction == nullptr) && !vd->isExtern && !vd->initializer) {
                            hasBssVars = true;
                            break;
                        }
                    }
                }
            }
            out << "_init_bss:" << std::endl;
            if (hasBssVars) {
                out << "    lda #<__bss_start" << std::endl;
                out << "    sta $02" << std::endl;
                out << "    lda #>__bss_start" << std::endl;
                out << "    sta $03" << std::endl;
                out << "    lda #<__bss_end" << std::endl;
                out << "    sta $04" << std::endl;
                out << "    lda #>__bss_end" << std::endl;
                out << "    sta $05" << std::endl;
                out << "    lda #0" << std::endl;
                out << "    ldy #0" << std::endl;
                out << "@__bss_loop:" << std::endl;
                out << "    ldx $02" << std::endl;
                out << "    cpx $04" << std::endl;
                out << "    bne @__bss_store" << std::endl;
                out << "    ldx $03" << std::endl;
                out << "    cpx $05" << std::endl;
                out << "    beq @__bss_done" << std::endl;
                out << "@__bss_store:" << std::endl;
                out << "    sta ($02),y" << std::endl;
                out << "    inc $02" << std::endl;
                out << "    bne @__bss_loop" << std::endl;
                out << "    inc $03" << std::endl;
                out << "    bra @__bss_loop" << std::endl;
                out << "@__bss_done:" << std::endl;
            }
            out << "    rts" << std::endl;
        }
        out << std::endl;
    }

    for (auto& decl : node.topLevelDecls) decl->accept(*this);
}

void CodeGenerator::visit(FunctionDeclaration& node) {
    if (node.isPrototype) return; // Forward declaration — no code emitted
    out << ".code" << std::endl;
    variableTypes.clear();
    currentVars.clear();
    std::string procLine = "proc _" + node.name;

    currentFunction = &node;
    currentParamByteSize = 0;
    currentLocalByteSize = 0;

    // Collect param info and build proc line
    struct ParamInfo { std::string pName; int size; };
    std::vector<ParamInfo> paramInfos;
    for (auto& param : node.parameters) {
        std::string pName = "_p_" + param.name;
        variableTypes[pName] = {param.type, param.pointerLevel, param.isSigned, param.isVolatile, param.isConst, param.isPointerConst};
        int pSize;
        if (param.pointerLevel > 0) {
            procLine += ", W#" + pName;
            pSize = 2;
        } else if (is8BitType(param.type)) {
            procLine += ", B#" + pName;
            pSize = 1;
        } else {
            procLine += ", W#" + pName;
            pSize = 2;
        }
        currentParamByteSize += pSize;
        paramInfos.push_back({pName, pSize});
    }
    out << procLine << std::endl;
    // _fp tracks current local frame size (no saved frame pointer)
    emit(".var _fp = 0");
    currentVars.push_back("_fp");
    // Params are stack-relative: past return addr (2 bytes) + cumulative offset.
    // They get bumped alongside _fp when locals are declared.
    {
        int pOff = 2; // past 2-byte return address
        for (int i = (int)paramInfos.size() - 1; i >= 0; --i) {
            emit(".var " + paramInfos[i].pName + " = " + std::to_string(pOff));
            currentVars.push_back(paramInfos[i].pName);
            pOff += paramInfos[i].size;
        }
    }
    node.body->accept(*this);
    if (!node.isNoreturn) {
        // Check if the last statement in the body was a return statement.
        // If it was, we don't need to emit an implicit one.
        bool lastWasReturn = false;
        if (!node.body->statements.empty()) {
            if (dynamic_cast<ReturnStatement*>(node.body->statements.back().get())) {
                lastWasReturn = true;
            }
        }
        if (!lastWasReturn) {
            int cleanupSize = currentLocalByteSize;
            if (cleanupSize > 0) {
                emitter->taz();
                for (int i = 0; i < cleanupSize; ++i) emitter->pla();
                emitter->tza();
            }
            emit("rtn #0");
        }
    }
    emit("endproc");
    out << std::endl;

    currentFunction = nullptr;
}

void CodeGenerator::visit(CompoundStatement& node) {
    for (auto& stmt : node.statements) stmt->accept(*this);
}

void CodeGenerator::visit(VariableDeclaration& node) {
    embedSource(node);

    if (node.isGlobal || currentFunction == nullptr) {
        std::string gName = "_" + node.name;
        globalVariableTypes[gName] = {node.type, node.pointerLevel, node.isSigned, node.isVolatile, node.isConst, node.isPointerConst, node.arrayDims};
        if (node.isExtern) {
            // extern declaration — type is known but no storage emitted
            return;
        }
        if (node.isGlobal) {
            globalVars.push_back(&node);
            if (weakNext) { weakGlobals.insert(node.name); weakNext = false; }
            return;
        }
    }

    std::string lName = "_l_" + node.name;
    variableTypes[lName] = {node.type, node.pointerLevel, node.isSigned, node.isVolatile, node.isConst, node.isPointerConst, node.arrayDims};

    if (node.initializer && !dynamic_cast<CastExpression*>(node.initializer.get())) {
        if (auto* lit = dynamic_cast<IntegerLiteral*>(node.initializer.get())) {
            int dstSize = (node.pointerLevel > 0 || node.type == "int") ? 2 : 1;
            if (dstSize == 1 && (lit->value < 0 || lit->value > 255)) {
                std::string loc = sourceFilename.empty() ? "" : sourceFilename + ":";
                if (node.line > 0) loc += std::to_string(node.line) + ":" + std::to_string(node.column) + ": ";
                std::string msg = loc + "warning: implicit conversion from constant " + std::to_string(lit->value) + " loses data (truncated to 'char')";
                warnings.push_back(msg);
                std::cerr << msg << std::endl;
            }
        } else {
            ExpressionType initType = getExprType(node.initializer.get());
            emitNarrowingWarning(node, initType.type, initType.pointerLevel, node.type, node.pointerLevel);
        }
    }

    int size = 0;
    if (node.pointerLevel > 0) size = 2;
    else if (node.type == "char") size = 1;
    else if (node.type == "_Bool") size = 1;
    else if (node.type == "int") size = 2;
    else if (isStruct(node.type)) {
        std::string sName = getAggregateName(node.type);
        if (structs.count(sName)) size = structs[sName]->totalSize;
        else throw std::runtime_error("Unknown struct/union type: " + sName);
    }
    if (node.arraySize() >= 0) size *= node.arraySize();

    if (node.isVolatile) {
        if (node.initializer) {
            if (auto* lit = dynamic_cast<IntegerLiteral*>(node.initializer.get())) {
                 if (size == 2) {
                    emitter->phw_imm((uint16_t)(int16_t)lit->value);
                } else {
                    uint8_t val = lit->value & 0xFF;
                    if (node.type == "_Bool") val = (val != 0) ? 1 : 0;
                    emitter->lda_imm(val);
                    emitter->pha();
                }
            } else {
                bool oldNeeded = resultNeeded;
                resultNeeded = true;
                node.initializer->accept(*this);
                resultNeeded = oldNeeded;
                if (node.type == "_Bool" && node.pointerLevel == 0) {
                    ExpressionType srcType = getExprType(node.initializer.get());
                    int srcSize = (srcType.pointerLevel > 0 || srcType.type == "int") ? 2 : 1;
                    emitBoolNormalize(srcSize);
                }
                if (size == 2) emitter->push_ax();
                else emitter->pha();
            }
        } else {
            if (size == 2) emitter->phw_imm(0);
            else { emitter->lda_imm(0); emitter->pha(); }
        }
    } else {

        if (node.initializer && dynamic_cast<IntegerLiteral*>(node.initializer.get())) {
            if (!isVariableUsed(node.name, *currentFunction)) {
                return;
            }
        }

        if (node.initializer) {
            if (auto* lit = dynamic_cast<IntegerLiteral*>(node.initializer.get())) {
                if (size == 2) {
                    emitter->phw_imm((uint16_t)(int16_t)lit->value);
                } else {
                    uint8_t val = lit->value & 0xFF;
                    if (node.type == "_Bool") val = (val != 0) ? 1 : 0;
                    emitter->lda_imm(val);
                    emitter->pha();
                }
            } else {
                bool oldNeeded = resultNeeded;
                resultNeeded = true;
                node.initializer->accept(*this);
                resultNeeded = oldNeeded;
                if (node.type == "_Bool" && node.pointerLevel == 0) {
                    ExpressionType srcType = getExprType(node.initializer.get());
                    int srcSize = (srcType.pointerLevel > 0 || srcType.type == "int") ? 2 : 1;
                    emitBoolNormalize(srcSize);
                }
                if (size == 2) emitter->push_ax();
                else emitter->pha();
            }
        } else {
            if (size >= 9) {
                emitter->phw_imm(0);
                for (int i = 0; i < (size - 2) / 2; ++i) emitter->phw_imm(0);
                if (size % 2) { emit("lda #0"); emit("pha"); }
                emit("lda #0");
                emit("FILL.SP " + lName + ", #" + std::to_string(size));
            } else {
                if (size == 2) emitter->phw_imm(0);
                else { emitter->lda_imm(0); emitter->pha(); }
            }
        }
    }
    for (const auto& varName : currentVars) {
        emit(".var " + varName + " = " + varName + " + " + std::to_string(size));
    }
    currentLocalByteSize += size;
    emit(".var " + lName + " = 0");
    currentVars.push_back(lName);
}

void CodeGenerator::emitOperation(const std::string& op, int zpLeft, ExpressionType lhsType, ExpressionType rhsType) {
    int scale = 1;
    if (lhsType.pointerLevel > 0 && rhsType.pointerLevel == 0 && (op == "+" || op == "-")) {
        scale = (is8BitType(lhsType.type) && lhsType.pointerLevel == 1) ? 1 : 2;
    }
    if (scale > 1) {
        emitter->asl_a(); emitter->pha(); emitter->txa(); emitter->rol_a(); emitter->tax(); emitter->pla();
    }

    std::stringstream ssLeft;
    ssLeft << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpLeft);

    if (op == "+" || op == "&" || op == "|" || op == "^") {
        if (op == "+") emit("add.16 .ax, $" + ssLeft.str());
        else if (op == "&") emit("and.16 .ax, $" + ssLeft.str());
        else if (op == "|") emit("ora.16 .ax, $" + ssLeft.str());
        else if (op == "^") emit("eor.16 .ax, $" + ssLeft.str());
    } else {
        int zpRight = allocateZP(2);
        std::stringstream ssRight;
        ssRight << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpRight);
        emit("stax $" + ssRight.str());
        emit("ldax $" + ssLeft.str());

        bool isSigned = (lhsType.isSigned || rhsType.isSigned) && lhsType.pointerLevel == 0;
        if (op == "-") emit("sub.16 .ax, $" + ssRight.str());
        else if (op == "*") emit((isSigned ? "mul.s16" : "mul.16") + std::string(" .ax, $") + ssRight.str());
        else if (op == "/" || op == "%") {
            std::string divOp = isSigned ? "div.s16" : "div.16";
            std::string modOp = isSigned ? "mod.s16" : "mod.16";
            if (op == "/") emit(divOp + " .ax, $" + ssRight.str());
            else emit(modOp + " .ax, $" + ssRight.str());
        } else if (op == "<<") {
            std::string labelStart = newDontCareLabel(); std::string labelEnd = newDontCareLabel();
            int shiftZp = allocateZP(1); std::stringstream ssSh; ssSh << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(shiftZp);
            emit("lda $" + ssRight.str()); emit("sta $" + ssSh.str());
            out << labelStart << ":" << std::endl;
            emit("lda $" + ssSh.str()); emit("beq " + labelEnd); emit("dec $" + ssSh.str());
            emit("lsl.16 .ax");
            emit("bra " + labelStart); out << labelEnd << ":" << std::endl;
            freeZP(shiftZp, 1);
        } else if (op == ">>") {
            bool isSigned = lhsType.isSigned && lhsType.pointerLevel == 0;
            std::string shiftOp = isSigned ? "asr.16 .ax" : "lsr.16 .ax";
            std::string labelStart = newDontCareLabel(); std::string labelEnd = newDontCareLabel();
            int shiftZp = allocateZP(1); std::stringstream ssSh; ssSh << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(shiftZp);
            emit("lda $" + ssRight.str()); emit("sta $" + ssSh.str());
            out << labelStart << ":" << std::endl;
            emit("lda $" + ssSh.str()); emit("beq " + labelEnd); emit("dec $" + ssSh.str());
            emit(shiftOp);
            emit("bra " + labelStart); out << labelEnd << ":" << std::endl;
            freeZP(shiftZp, 1);
        }
        freeZP(zpRight, 2);
    }
}

void CodeGenerator::visit(Assignment& node) {
    embedSource(node);

    // Check for const assignment
    ExpressionType targetType = getExprType(node.target.get());
    if (auto* ref = dynamic_cast<VariableReference*>(node.target.get())) {
        // Direct variable assignment: check isConst for non-pointers, isPointerConst for pointers
        if (targetType.pointerLevel > 0) {
            if (targetType.isPointerConst) throw std::runtime_error("Compile Error: Assignment to read-only location '" + ref->name + "'");
        } else {
            if (targetType.isConst) throw std::runtime_error("Compile Error: Assignment to read-only location '" + ref->name + "'");
        }
    } else if (targetType.isConst) {
        // Dereference or member access: isConst means pointed-to/member type is const
        throw std::runtime_error("Compile Error: Assignment to read-only location");
    }

    if (node.op == "=" && !dynamic_cast<CastExpression*>(node.expression.get())) {
        if (auto* lit = dynamic_cast<IntegerLiteral*>(node.expression.get())) {
            ExpressionType dstType = getExprType(node.target.get());
            int dstSize = (dstType.pointerLevel > 0 || dstType.type == "int") ? 2 : 1;
            if (dstSize == 1 && (lit->value < 0 || lit->value > 255)) {
                std::string loc = sourceFilename.empty() ? "" : sourceFilename + ":";
                if (node.line > 0) loc += std::to_string(node.line) + ":" + std::to_string(node.column) + ": ";
                std::string msg = loc + "warning: implicit conversion from constant " + std::to_string(lit->value) + " loses data (truncated to 'char')";
                warnings.push_back(msg);
                std::cerr << msg << std::endl;
            }
        } else {
            ExpressionType srcType = getExprType(node.expression.get());
            ExpressionType dstType = getExprType(node.target.get());
            emitNarrowingWarning(node, srcType.type, srcType.pointerLevel, dstType.type, dstType.pointerLevel);
        }
    }

    if (node.op != "=") {
        std::string actualOp = node.op.substr(0, node.op.length() - 1);
        ExpressionType targetType = getExprType(node.target.get());
        bool is16 = (targetType.pointerLevel > 0 || targetType.type == "int");
        if (isStruct(targetType.type)) {
            std::string sName = getAggregateName(targetType.type);
            if (structs.count(sName) && structs[sName]->totalSize > 1) is16 = true;
        }

        // Evaluate RHS first to avoid ZP clobber by function calls
        bool oldNeeded = resultNeeded;
        resultNeeded = true;
        node.expression->accept(*this);
        resultNeeded = oldNeeded;
        emitter->push_ax(); // save RHS on stack

        // Now compute destination address (ZP only used transiently)
        emitAddress(node.target.get());
        int zpAddr = allocateZP(2);
        std::stringstream ssAddr;
        ssAddr << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpAddr);
        emit("stax $" + ssAddr.str());

        // Load current value at destination
        if (is16) {
            emit("ptrderef $" + ssAddr.str());
        } else {
            emitter->lda_ind_z(emitter->getZP(zpAddr), false);
            updateRegY(0); emitter->ldx_imm(0); updateRegX(0);
        }

        int zpLeft = allocateZP(2);
        std::stringstream ssLeft;
        ssLeft << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpLeft);
        emit("stax $" + ssLeft.str());

        // Restore RHS value from stack
        emitter->pop_ax();

        emitOperation(actualOp, zpLeft, targetType, getExprType(node.expression.get()));

        emitter->sta_ind_z(emitter->getZP(zpAddr), false);
        if (is16) {
            emitter->txa();
            emitter->ldy_imm(1);
            emit("sta ($" + ssAddr.str() + "),y");
        }

        freeZP(zpLeft, 2);
        freeZP(zpAddr, 2);
        invalidateRegs();
        return;
    }

    if (auto* ref = dynamic_cast<VariableReference*>(node.target.get())) {
        std::string rName = resolveVarName(ref->name);
        bool isGlobal = globalVariableTypes.count(rName);
        std::string suffix = isGlobal ? "" : ", s";

        // Optimization: x = x;
        if (auto* rhsRef = dynamic_cast<VariableReference*>(node.expression.get())) {
            if (resolveVarName(rhsRef->name) == rName) {
                VarInfo vi = variableTypes.count(rName) ? variableTypes.at(rName) : globalVariableTypes.at(rName);
                if (!vi.isVolatile) {
                    if (resultNeeded) node.target->accept(*this);
                    return;
                }
            }
        }

        if (auto* bin = dynamic_cast<BinaryOperation*>(node.expression.get())) {
            if (bin->op == "+" || bin->op == "-") {
                if (auto* leftRef = dynamic_cast<VariableReference*>(bin->left.get())) {
                    if (auto* rightLit = dynamic_cast<IntegerLiteral*>(bin->right.get())) {
                        if (resolveVarName(leftRef->name) == rName && rightLit->value == 1) {
                            VarInfo vi = variableTypes.count(rName) ? variableTypes.at(rName) : globalVariableTypes.at(rName);
                            bool is16 = (vi.pointerLevel > 0 || vi.type == "int");
                            if (isStruct(vi.type)) {
                                std::string sName = getAggregateName(vi.type);
                                if (structs.count(sName) && structs[sName]->totalSize > 1) is16 = true;
                            }
                            if (bin->op == "+") {
                                if (is16) emit("inw " + rName + suffix);
                                else emit("inc " + rName + suffix);
                            } else {
                                if (is16) emit("dew " + rName + suffix);
                                else emit("dec " + rName + suffix);
                            }
                            invalidateRegs();
                            if (resultNeeded) ref->accept(*this);
                            return;
                        }
                    }
                }
            }
        }

        if (auto* lit = dynamic_cast<IntegerLiteral*>(node.expression.get())) {
            VarInfo vi = variableTypes.count(rName) ? variableTypes.at(rName) : globalVariableTypes.at(rName);
            bool is16 = (vi.pointerLevel > 0 || vi.type == "int");
            if (isStruct(vi.type)) {
                std::string sName = getAggregateName(vi.type);
                if (structs.count(sName) && structs[sName]->totalSize > 1) is16 = true;
            }
            if (is16) {
                std::stringstream ss;
                ss << "#$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (uint16_t)(int16_t)lit->value;
                if (isGlobal) {
                    emit("stw " + ss.str() + ", " + rName);
                    invalidateRegs(); // stw to global clobbers A
                } else {
                    emit("stw.sp " + ss.str() + ", " + rName);
                    invalidateRegs(); // stw.sp clobbers A
                }
            } else {
                uint8_t val = lit->value & 0xFF;
                if (vi.type == "_Bool") val = (val != 0) ? 1 : 0;
                if (!regA.known || (regA.isVariable && regA.varName != rName) || (!regA.isVariable && regA.value != val)) {
                    emitter->lda_imm(val);
                }
                if (isGlobal) emit("sta " + rName);
                else emit("sta.sp " + rName);
                updateRegAVar(rName, 0); regA.value = val;
            }
            return;
        }

        VarInfo vi = variableTypes.count(rName) ? variableTypes.at(rName) : globalVariableTypes.at(rName);
        if (isStruct(vi.type)) {
            std::string sName = getAggregateName(vi.type);
            if (structs.count(sName)) {
                int structSize = structs[sName]->totalSize;
                if (structSize >= 9) {
                    if (auto* sourceRef = dynamic_cast<VariableReference*>(node.expression.get())) {
                        std::string srcName = resolveVarName(sourceRef->name);
                        bool sourceGlobal = globalVariableTypes.count(srcName);
                        
                        std::stringstream ssX, ssY;
                        ssX << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (structSize & 0xFF);
                        ssY << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << ((structSize >> 8) & 0xFF);
                        emit("ldx #$" + ssX.str());
                        emit("ldy #$" + ssY.str());
                        emit("MOVE " + srcName + (sourceGlobal ? "" : ", s") + ", " + rName + (isGlobal ? "" : ", s"));
                        invalidateVar(rName);
                        return;
                    }
                }
            }
        }

        bool oldNeeded = resultNeeded;
        resultNeeded = true;
        node.expression->accept(*this);
        resultNeeded = oldNeeded;

        // _Bool normalization: any non-zero value becomes 1
        if (vi.type == "_Bool" && vi.pointerLevel == 0) {
            ExpressionType srcType = getExprType(node.expression.get());
            int srcSize = (srcType.pointerLevel > 0 || srcType.type == "int") ? 2 : 1;
            emitBoolNormalize(srcSize);
        }

        bool is16 = (vi.pointerLevel > 0 || vi.type == "int");
        if (isStruct(vi.type)) {
            std::string sName = getAggregateName(vi.type);
            if (structs.count(sName) && structs[sName]->totalSize > 1) is16 = true;
        }
        if (is16) {
            bool lowCorrect = (regA.known && regA.isVariable && regA.varName == rName && regA.varOffset == 0);
            bool highCorrect = (regX.known && regX.isVariable && regX.varName == rName && regX.varOffset == 1);
            if (vi.isVolatile) { lowCorrect = false; highCorrect = false; }

            if (lowCorrect && highCorrect) {
                // Redundant store
            } else if (lowCorrect) {
                if (isGlobal) emit("stx " + rName + "+1");
                else emit("stx.sp " + rName + "+1");
            } else if (highCorrect) {
                if (isGlobal) emit("sta " + rName);
                else emit("sta.sp " + rName);
            } else {
                emit("stax " + rName + suffix);
            }
            if (vi.isVolatile) invalidateRegs();
            else { updateRegAVar(rName, 0); updateRegXVar(rName, 1); }
        } else {
            bool lowCorrect = (regA.known && regA.isVariable && regA.varName == rName && regA.varOffset == 0);
            if (vi.isVolatile) lowCorrect = false;

            if (lowCorrect) {
                // Redundant store
            } else {
                if (isGlobal) emit("sta " + rName);
                else emit("sta.sp " + rName);
            }
            if (vi.isVolatile) invalidateRegs();
            else updateRegAVar(rName, 0);
        }
        return;
    } else if (auto* ma = dynamic_cast<MemberAccess*>(node.target.get())) {
        if (!ma->isArrow) {
            if (auto* ref = dynamic_cast<VariableReference*>(ma->structExpr.get())) {
                std::string rName = resolveVarName(ref->name);
                bool isGlobal = globalVariableTypes.count(rName);
                std::string suffix = isGlobal ? "" : ", s";

                ExpressionType baseType = getExprType(ref);
                if (!isStruct(baseType.type)) {
                    if (globalVariableTypes.count("_" + ref->name)) {
                        baseType = {globalVariableTypes.at("_" + ref->name).type, globalVariableTypes.at("_" + ref->name).pointerLevel};
                    }
                }
                if (isStruct(baseType.type)) {
                    std::string sName = getAggregateName(baseType.type);
                    if (structs.count(sName) && structs[sName]->members.count(ma->memberName)) {
                        MemberInfo& mInfo = structs[sName]->members[ma->memberName];
                        bool oldNeeded = resultNeeded;
                        resultNeeded = true;
                        node.expression->accept(*this);
                        resultNeeded = oldNeeded;
                        bool is16 = (mInfo.pointerLevel > 0 || mInfo.type == "int");
                        if (isStruct(mInfo.type)) {
                            std::string nestedSName = getAggregateName(mInfo.type);
                            if (structs.count(nestedSName) && structs[nestedSName]->totalSize > 1) is16 = true;
                        }
                        if (is16) {
                            emit("stax " + rName + "+" + std::to_string(mInfo.offset) + suffix);
                            updateRegAVar(rName, mInfo.offset); updateRegXVar(rName, mInfo.offset + 1);
                        } else {
                            if (isGlobal) emit("sta " + rName + "+" + std::to_string(mInfo.offset));
                            else emit("sta.sp " + rName + "+" + std::to_string(mInfo.offset));
                            updateRegAVar(rName, mInfo.offset);
                        }
                        invalidateRegs();
                        return;
                    }
                }
            }
        }
    }

    // Evaluate RHS first, save to allocated ZP (not hardware stack, which
    // would shift SP and break stack-relative index reads in emitAddress)
    bool oldNeeded = resultNeeded;
    resultNeeded = true;
    node.expression->accept(*this);
    resultNeeded = oldNeeded;
    int zpRHS = allocateZP(2);
    {
        std::stringstream ssR;
        ssR << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpRHS);
        emit("stax $" + ssR.str());
    }

    // Compute destination address (emitAddress uses allocateZP internally,
    // so its slots won't conflict with zpRHS)
    emitAddress(node.target.get());
    int zpIdx = allocateZP(2);
    {
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpIdx);
        emit("stax $" + ss.str());
    }

    // Restore RHS from ZP
    {
        std::stringstream ssR;
        ssR << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpRHS);
        emit("ldax $" + ssR.str());
    }
    freeZP(zpRHS, 2);

    emitter->sta_ind_z(emitter->getZP(zpIdx), false);
    updateRegY(0);
    bool is16 = (targetType.pointerLevel > 0 || targetType.type == "int");
    if (isStruct(targetType.type)) {
        std::string sName = getAggregateName(targetType.type);
        if (structs.count(sName) && structs[sName]->totalSize > 1) is16 = true;
    }
    if (is16) {
        emitter->txa(); regA.known = false;
        emitter->ldy_imm(1); updateRegY(1);
        std::stringstream ss2;
        ss2 << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpIdx);
        emit("sta ($" + ss2.str() + "),y");
    }
    freeZP(zpIdx, 2);
    invalidateRegs();
}

void CodeGenerator::visit(BinaryOperation& node) {
    ExpressionType lhsType = getExprType(node.left.get());
    ExpressionType rhsType = getExprType(node.right.get());
    int scale = 1;
    if (lhsType.pointerLevel > 0 && rhsType.pointerLevel == 0 && (node.op == "+" || node.op == "-")) {
        scale = (is8BitType(lhsType.type) && lhsType.pointerLevel == 1) ? 1 : 2;
    }
    bool isLiteralOne = false;
    if (scale == 1) {
        if (auto* lit = dynamic_cast<IntegerLiteral*>(node.right.get())) {
            if (lit->value == 1) isLiteralOne = true;
        }
    }
    if (isLiteralOne && (node.op == "+" || node.op == "-")) {
        bool oldNeeded = resultNeeded;
        resultNeeded = true;
        node.left->accept(*this);
        resultNeeded = oldNeeded;
        if (node.op == "+") {
            std::string label = newDontCareLabel();
            emitter->inc_a(); emitter->bne(0x01); emit("inx");
            out << label << ":" << std::endl;
        } else {
            std::string label = newDontCareLabel();
            emit("cmp #$00"); emit("bne " + label); emit("dex");
            out << label << ":" << std::endl; emitter->dec_a();
        }
        invalidateRegs();
        return;
    }
    if (node.op == "||") {
        std::string labelTrue = newDontCareLabel();
        std::string labelEnd = newDontCareLabel();
        bool oldNeeded = resultNeeded;
        resultNeeded = true;
        node.left->accept(*this);
        resultNeeded = oldNeeded;
        if (is8BitType(getExprType(node.left.get()).type) && getExprType(node.left.get()).pointerLevel == 0) {
            if (flags.znSource != FlagSource::A) emit("cmp #$00");
            emit("bne " + labelTrue);
        } else {
            if (flags.znSource != FlagSource::A) emit("cmp #$00");
            emitBranch16Bne(labelTrue);
        }
        resultNeeded = true;
        node.right->accept(*this);
        resultNeeded = oldNeeded;
        if (is8BitType(getExprType(node.right.get()).type) && getExprType(node.right.get()).pointerLevel == 0) {
            if (flags.znSource != FlagSource::A) emit("cmp #$00");
            emit("bne " + labelTrue);
        } else {
            if (flags.znSource != FlagSource::A) emit("cmp #$00");
            emitBranch16Bne(labelTrue);
        }
        emit("zero a, x");
        emit("bra " + labelEnd);
        out << labelTrue << ":" << std::endl;
        emit("lda #$01");
        emit("ldx #$00");
        updateRegA(1); updateRegX(0);
        out << labelEnd << ":" << std::endl;
        invalidateRegs();
        return;
    } else if (node.op == "&&") {
        std::string labelFalse = newDontCareLabel();
        std::string labelEnd = newDontCareLabel();
        bool oldNeeded = resultNeeded;
        resultNeeded = true;
        node.left->accept(*this);
        resultNeeded = oldNeeded;
        if (is8BitType(getExprType(node.left.get()).type) && getExprType(node.left.get()).pointerLevel == 0) {
            if (flags.znSource != FlagSource::A) emit("cmp #$00");
            emit("beq " + labelFalse);
        } else {
            if (flags.znSource != FlagSource::A) emit("cmp #$00");
            emitBranch16Beq(labelFalse);
        }
        resultNeeded = true;
        node.right->accept(*this);
        resultNeeded = oldNeeded;
        if (is8BitType(getExprType(node.right.get()).type) && getExprType(node.right.get()).pointerLevel == 0) {
            if (flags.znSource != FlagSource::A) emit("cmp #$00");
            emit("beq " + labelFalse);
        } else {
            if (flags.znSource != FlagSource::A) emit("cmp #$00");
            emitBranch16Beq(labelFalse);
        }
        emit("lda #$01");
        emit("ldx #$00");
        updateRegA(1); updateRegX(0);
        emit("bra " + labelEnd);
        out << labelFalse << ":" << std::endl;
        emit("zero a, x");
        updateRegA(0); updateRegX(0);
        out << labelEnd << ":" << std::endl;
        invalidateRegs();
        return;
    }

    bool isMultiplicativeLiteral = false;
    if (dynamic_cast<IntegerLiteral*>(node.right.get())) {
        if (node.op == "*" || node.op == "/" || node.op == "%" || node.op == "<<" || node.op == ">>") {
            isMultiplicativeLiteral = true;
        }
    }

    bool oldNeeded = resultNeeded;
    resultNeeded = true;
    node.left->accept(*this);

    if (isMultiplicativeLiteral) {
        int val = dynamic_cast<IntegerLiteral*>(node.right.get())->value;
        if (node.op == "*") {
            if (val == 0) { emit("zero a, x"); updateRegA(0); updateRegX(0); }
            else if (val == 1) { /* Already in AX */ }
            else if (val > 0 && (val & (val - 1)) == 0) {
                int shifts = 0; while (val > 1) { val >>= 1; shifts++; }
                if (shifts == 1 && getExprType(node.left.get()).pointerLevel == 0 && (getExprType(node.left.get()).type == "int")) {
                    emit("lsl.16 .ax");
                } else {
                    for (int i = 0; i < shifts; i++) {
                        emit("lsl.16 .ax");
                    }
                }
            } else {
                int zpIdx = allocateZP(2);
                std::stringstream ss; ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpIdx);
                emit("stax $" + ss.str());
                std::stringstream ssVal; ssVal << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << val;
                emit("ldax #$" + ssVal.str());
                bool isSigned = lhsType.isSigned && lhsType.pointerLevel == 0;
                emit((isSigned ? "mul.s16" : "mul.16") + std::string(" .ax, $") + ss.str());
                freeZP(zpIdx, 2);
            }
            invalidateFlags(); resultNeeded = oldNeeded; return;
        } else if (node.op == "/" || node.op == "%") {
            bool isSigned = lhsType.isSigned && lhsType.pointerLevel == 0;
            if (!isSigned && val > 0 && (val & (val - 1)) == 0) {
                if (node.op == "/") {
                    int shifts = 0; while (val > 1) { val >>= 1; shifts++; }
                    for (int i = 0; i < shifts; i++) {
                        emit("lsr.16 .ax");
                    }
                } else { // %
                    int mask = val - 1;
                    emitter->and_imm(mask & 0xFF); emitter->pha(); emitter->txa(); emitter->and_imm((mask >> 8) & 0xFF); emitter->tax(); emitter->pla();
                }
            } else {
                int zpIdx = allocateZP(2);
                std::stringstream ss; ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpIdx);
                emit("stax $" + ss.str());
                std::stringstream ss2;
                ss2 << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << val;
                emit("ldax #$" + ss2.str());
                std::string divOp = isSigned ? "div.s16" : "div.16";
                std::string modOp = isSigned ? "mod.s16" : "mod.16";
                if (node.op == "/") emit(divOp + " .ax, $" + ss.str());
                else emit(modOp + " .ax, $" + ss.str());
                freeZP(zpIdx, 2);
            }
            invalidateFlags(); resultNeeded = oldNeeded; return;
        } else if (node.op == "<<" || node.op == ">>") {
            bool isSigned = lhsType.isSigned && lhsType.pointerLevel == 0;
            for (int i = 0; i < val; i++) {
                if (node.op == "<<") { emit("lsl.16 .ax"); }
                else { emit(isSigned ? "asr.16 .ax" : "lsr.16 .ax"); }
            }
            invalidateFlags(); resultNeeded = oldNeeded; return;
        }
    }

    int zpIdx = allocateZP(2);
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpIdx);
    emit("stax $" + ss.str());
    node.right->accept(*this);
    resultNeeded = oldNeeded;
    if (scale > 1) {
        emitter->asl_a(); emitter->pha(); emitter->txa(); emitter->rol_a(); emitter->tax(); emitter->pla();
    }

    if (node.op == "+" || node.op == "&" || node.op == "|" || node.op == "^") {
        std::stringstream ss2; ss2 << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpIdx);
        if (node.op == "+") emit("add.16 .ax, $" + ss2.str());
        else if (node.op == "&") emit("and.16 .ax, $" + ss2.str());
        else if (node.op == "|") emit("ora.16 .ax, $" + ss2.str());
        else if (node.op == "^") emit("eor.16 .ax, $" + ss2.str());
    } else if (node.op == "-" || node.op == "*" || node.op == "/" || node.op == "%" || node.op == "<<" || node.op == ">>" || 
               node.op == "==" || node.op == "!=" || node.op == "<" || node.op == ">" || node.op == "<=" || node.op == ">=") {
        int zpRight = allocateZP(2);
        std::stringstream ssRight; ssRight << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpRight);
        emit("stax $" + ssRight.str());
        emit("ldax $" + ss.str()); // Load left back into AX

        bool isSigned = (lhsType.isSigned || rhsType.isSigned) && lhsType.pointerLevel == 0;
        if (node.op == "-") {
            emit("sub.16 .ax, $" + ssRight.str());
        } else if (node.op == "*") {
            emit((isSigned ? "mul.s16" : "mul.16") + std::string(" .ax, $") + ssRight.str());
        } else if (node.op == "/" || node.op == "%") {
            std::string divOp = isSigned ? "div.s16" : "div.16";
            std::string modOp = isSigned ? "mod.s16" : "mod.16";
            if (node.op == "/") emit(divOp + " .ax, $" + ssRight.str());
            else emit(modOp + " .ax, $" + ssRight.str());
        } else if (node.op == "<<") {
            std::string labelStart = newDontCareLabel(); std::string labelEnd = newDontCareLabel();
            int shiftZp = allocateZP(1); std::stringstream ssSh; ssSh << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(shiftZp);
            emit("lda $" + ssRight.str()); emit("sta $" + ssSh.str());
            out << labelStart << ":" << std::endl;
            emit("lda $" + ssSh.str()); emit("beq " + labelEnd); emit("dec $" + ssSh.str());
            emit("lsl.16 .ax");
            emit("bra " + labelStart); out << labelEnd << ":" << std::endl;
            freeZP(shiftZp, 1);
        } else if (node.op == ">>") {
            bool isSigned = lhsType.isSigned && lhsType.pointerLevel == 0;
            std::string shiftOp = isSigned ? "asr.16 .ax" : "lsr.16 .ax";
            std::string labelStart = newDontCareLabel(); std::string labelEnd = newDontCareLabel();
            int shiftZp = allocateZP(1); std::stringstream ssSh; ssSh << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(shiftZp);
            emit("lda $" + ssRight.str()); emit("sta $" + ssSh.str());
            out << labelStart << ":" << std::endl;
            emit("lda $" + ssSh.str()); emit("beq " + labelEnd); emit("dec $" + ssSh.str());
            emit(shiftOp);
            emit("bra " + labelStart); out << labelEnd << ":" << std::endl;
            freeZP(shiftZp, 1);
        } else {
            // Relational
            std::stringstream ss2; ss2 << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpRight);
            bool eitherSigned = lhsType.isSigned || rhsType.isSigned;
            if (eitherSigned) {
                emit("cmp.s16 .ax, $" + ss2.str());
            } else {
                emit("cmp.16 .ax, $" + ss2.str());
            }
            std::string labelFalse = newDontCareLabel(); std::string labelEnd = newDontCareLabel();
            if (node.op == "==") emit("bne " + labelFalse);
            else if (node.op == "!=") emit("beq " + labelFalse);
            else if (node.op == "<") emit("bcs " + labelFalse);
            else if (node.op == ">") {
                emit("beq " + labelFalse);
                emit("bcc " + labelFalse);
            }
            else if (node.op == "<=") {
                 std::string labelTrue = newDontCareLabel();
                 emit("beq " + labelTrue);
                 emit("bcs " + labelFalse);
                 out << labelTrue << ":" << std::endl;
            }
            else if (node.op == ">=") emit("bcc " + labelFalse);

            emitter->lda_imm(1); emitter->bra(0x02); out << labelFalse << ":" << std::endl; emitter->lda_imm(0); out << labelEnd << ":" << std::endl;
            emitter->ldx_imm(0); updateRegX(0);
        }
        freeZP(zpRight, 2);
    }
    invalidateFlags();
    freeZP(zpIdx, 2);
    invalidateRegs();
}

void CodeGenerator::visit(ConditionalExpression& node) {
    embedSource(node);
    std::string labelElse = newLabel();
    std::string labelEnd = newLabel();
    emitJumpIfFalse(node.condition.get(), labelElse);
    node.thenExpr->accept(*this);
    emit("bra " + labelEnd);
    out << labelElse << ":" << std::endl;
    invalidateRegs();
    node.elseExpr->accept(*this);
    out << labelEnd << ":" << std::endl;
    invalidateRegs();
}

void CodeGenerator::visit(GenericSelection& node) {
    ExpressionType controlType = getExprType(node.control.get());
    for (auto& assoc : node.associations) {
        if (!assoc.isDefault && CodeGenerator::matchType(controlType, assoc.typeName, assoc.pointerLevel)) {
            assoc.result->accept(*this);
            return;
        }
    }
    for (auto& assoc : node.associations) {
        if (assoc.isDefault) {
            assoc.result->accept(*this);
            return;
        }
    }
    throw std::runtime_error("No matching association in _Generic selection");
}

void CodeGenerator::visit(ArrayAccess& node) {
    embedSource(node);
    if (!resultNeeded) {
        node.arrayExpr->accept(*this);
        node.indexExpr->accept(*this);
        return;
    }
    emitAddress(&node); // Get element address in AX
    int zpAddr = allocateZP(2);
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpAddr);
    emit("stax $" + ss.str());

    ExpressionType resType = getExprType(&node);
    if (resType.pointerLevel > 0 || resType.type == "int") {
        std::stringstream ss2;
        ss2 << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpAddr);
        emit("ptrderef $" + ss2.str());
    } else {
        emitter->lda_ind_z(emitter->getZP(zpAddr), false);
        updateRegY(0); emitter->ldx_imm(0); updateRegX(0);
    }
    freeZP(zpAddr, 2);
    invalidateRegs();
}

void CodeGenerator::visit(UnaryOperation& node) {
    if (node.op == "*") {
        bool oldNeeded = resultNeeded;
        resultNeeded = true;
        node.operand->accept(*this);
        resultNeeded = oldNeeded;
        if (!resultNeeded) { invalidateRegs(); return; }
        int zpIdx = allocateZP(2);
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpIdx);
        emit("stax $" + ss.str());
        ExpressionType subType = getExprType(node.operand.get());
        if (is8BitType(subType.type) && subType.pointerLevel == 1) {
            emitter->lda_ind_z(emitter->getZP(zpIdx), false);
            updateRegY(0); emitter->ldx_imm(0); updateRegX(0);
        } else {
            std::stringstream ss2;
            ss2 << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpIdx);
            emit("ptrderef $" + ss2.str());
        }
        freeZP(zpIdx, 2);
    } else if (node.op == "&") {
        emitAddress(node.operand.get());
    } else if (node.op == "++" || node.op == "--" || node.op == "++_POST" || node.op == "--_POST") {
        bool isInc = (node.op.substr(0, 2) == "++");
        bool isPost = (node.op.find("_POST") != std::string::npos);
        if (auto* ref = dynamic_cast<VariableReference*>(node.operand.get())) {
            std::string rName = resolveVarName(ref->name);
            VarInfo vi = variableTypes.count(rName) ? variableTypes.at(rName) : globalVariableTypes.at(rName);
            if (vi.isConst) throw std::runtime_error("Compile Error: Increment/decrement of read-only variable '" + ref->name + "'");
            bool is16Bit = (vi.pointerLevel > 0 || vi.type == "int");
            if (isStruct(vi.type)) {
                std::string sName = getAggregateName(vi.type);
                if (structs.count(sName) && structs[sName]->totalSize > 1) is16Bit = true;
            }
            if (isPost && resultNeeded) ref->accept(*this);
            invalidateRegs();
            bool isGlobal = globalVariableTypes.count(rName);
            std::string suffix = isGlobal ? "" : ", s";
            if (is16Bit) {
                if (isInc) emit("inw " + rName + suffix);
                else emit("dew " + rName + suffix);
            } else {
                if (isInc) emit("inc " + rName + suffix);
                else emit("dec " + rName + suffix);
            }
            if (!isPost && resultNeeded) ref->accept(*this);
        } else if (auto* ma = dynamic_cast<MemberAccess*>(node.operand.get())) {
            if (!ma->isArrow) {
                if (auto* ref = dynamic_cast<VariableReference*>(ma->structExpr.get())) {
                    std::string rName = resolveVarName(ref->name);
                    ExpressionType baseType = getExprType(ref);
                    if (isStruct(baseType.type)) {
                        std::string sName = getAggregateName(baseType.type);
                        if (structs.count(sName) && structs[sName]->members.count(ma->memberName)) {
                            MemberInfo& mInfo = structs[sName]->members[ma->memberName];
                            if (mInfo.isConst) throw std::runtime_error("Compile Error: Increment/decrement of read-only member '" + ma->memberName + "'");
                            bool is16Bit = (mInfo.pointerLevel > 0 || mInfo.type == "int");
                            if (isStruct(mInfo.type)) {
                                std::string nestedSName = getAggregateName(mInfo.type);
                                if (structs.count(nestedSName) && structs[nestedSName]->totalSize > 1) is16Bit = true;
                            }
                            if (isPost && resultNeeded) node.operand->accept(*this);
                            invalidateRegs();
                            bool isGlobal = globalVariableTypes.count(rName);
                            std::string suffix = isGlobal ? "" : ", s";
                            if (is16Bit) {
                                if (isInc) emit("inw " + rName + "+" + std::to_string(mInfo.offset) + suffix);
                                else emit("dew " + rName + "+" + std::to_string(mInfo.offset) + suffix);
                            } else {
                                if (isInc) emit("inc " + rName + "+" + std::to_string(mInfo.offset) + suffix);
                                else emit("dec " + rName + "+" + std::to_string(mInfo.offset) + suffix);
                            }
                            if (!isPost && resultNeeded) node.operand->accept(*this);
                        }
                    }
                }
            } else {
                // Arrow member: p->field++ — compute address, load, inc/dec, store back
                emitIndirectIncDec(node, isInc, isPost);
            }
        } else {
            // Generic lvalue: (*p)++, arr[i]++, etc. — compute address, load, inc/dec, store back
            emitIndirectIncDec(node, isInc, isPost);
        }
    } else {
        bool oldNeeded = resultNeeded; resultNeeded = true; node.operand->accept(*this); resultNeeded = oldNeeded;
        if (node.op == "!") {
            ExpressionType subType = getExprType(node.operand.get());
            bool is8Bit = (is8BitType(subType.type) && subType.pointerLevel == 0);
            if (is8Bit) emit("chkzero.8"); else emit("chkzero.16");
            invalidateRegs(); updateZNFlags(FlagSource::A); emitter->ldx_imm(0); updateRegX(0);
        } else if (node.op == "~") { emitter->not_16(); }
        else if (node.op == "-") { emitter->neg_16(); }
    }
    invalidateRegs();
}

void CodeGenerator::visit(AsmStatement& node) {
    if (node.code == ".weak_next") {
        weakNext = true;
        return; // Don't emit — it's a compiler internal directive
    }
    if (node.code == ".crt_no_0100_stack") {
        crtNoPageOneStack = true;
        return; // Don't emit — it's a compiler internal directive
    }
    if (node.code == ".crt_exit_halt" || node.code == ".crt_exit_rts" || node.code == ".crt_exit_brk") {
        if (node.code == ".crt_exit_halt") crtExit = CrtExit::HALT;
        else if (node.code == ".crt_exit_rts") crtExit = CrtExit::RTS;
        else crtExit = CrtExit::BRK;
        return; // Don't emit — it's a compiler internal directive
    }
    if (node.code == ".crt_no_bssinit") {
        crtNoBssInit = true;
        return; // Don't emit — it's a compiler internal directive
    }
    embedSource(node);
    emit(node.code);
    invalidateRegs();
}

void CodeGenerator::visit(StaticAssert& node) {}

void CodeGenerator::visit(ReturnStatement& node) {
    embedSource(node);
    if (node.expression) {
        bool oldNeeded = resultNeeded;
        resultNeeded = true;
        node.expression->accept(*this);
        resultNeeded = oldNeeded;
    }
    // Clean up local variables from stack before returning.
    // AX holds the return value, so use Z register to preserve A.
    int cleanupSize = currentLocalByteSize;
    if (cleanupSize > 0) {
        emitter->taz();
        for (int i = 0; i < cleanupSize; ++i) emitter->pla();
        emitter->tza();
    }
    emit("rtn #0");
    invalidateRegs();
}

void CodeGenerator::visit(GotoStatement& node) {
    embedSource(node);
    emit("bra " + node.label);
}

void CodeGenerator::visit(LabelledStatement& node) {
    embedSource(node);
    out << node.label << ":" << std::endl;
    node.statement->accept(*this);
}

void CodeGenerator::visit(SizeofExpression& node) {
    embedSource(node);
    int size = 0;
    if (node.isType) {
        size = getTypeSize(node.typeName, node.pointerLevel, -1, structs);
    } else {
        ExpressionType et = getExprType(node.expression.get());
        // We need the array size if it's a variable
        int arrSize = -1;
        if (auto* ref = dynamic_cast<VariableReference*>(node.expression.get())) {
            std::string rName = resolveVarName(ref->name);
            if (variableTypes.count(rName)) arrSize = variableTypes.at(rName).arraySize();
            else if (globalVariableTypes.count("_" + ref->name)) arrSize = globalVariableTypes.at("_" + ref->name).arraySize();
        }
        size = getTypeSize(et.type, et.pointerLevel, arrSize, structs);
    }
    std::stringstream ss; ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << size;
    emit("ldax #$" + ss.str());
    updateRegA(size & 0xFF); updateRegX(size >> 8);
}

void CodeGenerator::visit(BreakStatement& node) {
    embedSource(node);
    if (!loopStack.empty()) emit("bra " + loopStack.back().breakLabel);
}

void CodeGenerator::visit(ContinueStatement& node) {
    embedSource(node);
    if (!loopStack.empty() && !loopStack.back().continueLabel.empty()) emit("bra " + loopStack.back().continueLabel);
}

void CodeGenerator::visit(SwitchContinueStatement& node) {
    embedSource(node);
    if (switchStack.empty()) return;
    SwitchInfo* info = switchStack.back();
    if (!node.target) {
        if (info->hasDefault) emit("bra " + info->defaultLabel);
    } else {
        if (auto* lit = dynamic_cast<IntegerLiteral*>(node.target.get())) {
            for (auto& c : info->cases) if (c.value == (uint32_t)lit->value) { emit("bra " + c.label); break; }
        }
    }
}

void CodeGenerator::visit(ExpressionStatement& node) {
    embedSource(node);
    bool oldNeeded = resultNeeded;
    resultNeeded = false;
    node.expression->accept(*this);
    resultNeeded = oldNeeded;
}

void CodeGenerator::visit(IfStatement& node) {
    embedSource(node);
    
    if (auto* lit = dynamic_cast<IntegerLiteral*>(node.condition.get())) {
        if (lit->value != 0) {
            node.thenBranch->accept(*this);
        } else if (node.elseBranch) {
            node.elseBranch->accept(*this);
        }
        return;
    }

    std::string labelElse = newLabel();
    std::string labelEnd = newLabel();
    emitJumpIfFalse(node.condition.get(), labelElse);
    node.thenBranch->accept(*this);
    if (node.elseBranch) {
        emit("bra " + labelEnd);
        out << labelElse << ":" << std::endl;
        invalidateRegs();
        node.elseBranch->accept(*this);
        out << labelEnd << ":" << std::endl;
    } else {
        out << labelElse << ":" << std::endl;
    }
    invalidateRegs();
}

void CodeGenerator::visit(WhileStatement& node) {
    embedSource(node);
    
    // Optimization for while(1) or while(constant)
    if (auto* lit = dynamic_cast<IntegerLiteral*>(node.condition.get())) {
        if (lit->value != 0) {
            std::string labelStart = newLabel();
            std::string labelEnd = newLabel();
            out << labelStart << ":" << std::endl;
            invalidateRegs();
            loopStack.push_back({labelStart, labelEnd});
            node.body->accept(*this);
            loopStack.pop_back();
            emit("bra " + labelStart);
            out << labelEnd << ":" << std::endl;
            invalidateRegs();
            return;
        } else {
            return;
        }
    }

    std::string labelStart = newLabel();
    std::string labelEnd = newLabel();
    out << labelStart << ":" << std::endl;
    invalidateRegs();
    emitJumpIfFalse(node.condition.get(), labelEnd);
    loopStack.push_back({labelStart, labelEnd});
    node.body->accept(*this);
    loopStack.pop_back();
    emit("bra " + labelStart);
    out << labelEnd << ":" << std::endl;
    invalidateRegs();
}

void CodeGenerator::visit(DoWhileStatement& node) {
    embedSource(node);
    std::string labelStart = newLabel();
    std::string labelCondition = newLabel();
    std::string labelEnd = newLabel();
    out << labelStart << ":" << std::endl;
    invalidateRegs();
    loopStack.push_back({labelCondition, labelEnd});
    node.body->accept(*this);
    loopStack.pop_back();
    out << labelCondition << ":" << std::endl;
    emitJumpIfTrue(node.condition.get(), labelStart);
    out << labelEnd << ":" << std::endl;
    invalidateRegs();
}

void CodeGenerator::visit(ForStatement& node) {
    embedSource(node);
    
    // Scoping for for-loop initializer variables
    auto oldVars = currentVars;
    auto oldVarTypes = variableTypes;
    auto oldZpRegs = zpRegs;

    if (node.initializer) {
        bool oldNeeded = resultNeeded;
        resultNeeded = false;
        node.initializer->accept(*this);
        resultNeeded = oldNeeded;
    }

    std::string labelStart = newLabel();
    std::string labelIncrement = newLabel();
    std::string labelEnd = newLabel();

    out << labelStart << ":" << std::endl;
    invalidateRegs();
    if (node.condition) emitJumpIfFalse(node.condition.get(), labelEnd);

    loopStack.push_back({labelIncrement, labelEnd});
    node.body->accept(*this);
    loopStack.pop_back();

    out << labelIncrement << ":" << std::endl;
    invalidateRegs();
    if (node.increment) {
        bool oldNeeded = resultNeeded;
        resultNeeded = false;
        node.increment->accept(*this);
        resultNeeded = oldNeeded;
    }
    emit("bra " + labelStart);

    out << labelEnd << ":" << std::endl;
    invalidateRegs();

    // Cleanup scope
    if (currentVars.size() > oldVars.size()) {
        emit(".cleanup " + std::to_string(currentVars.size() - oldVars.size()));
    }
    currentVars = std::move(oldVars);
    variableTypes = std::move(oldVarTypes);
    zpRegs = std::move(oldZpRegs);
}

void CodeGenerator::visit(SwitchStatement& node) {
    embedSource(node);
    bool oldNeeded = resultNeeded;
    resultNeeded = true;
    node.expression->accept(*this);
    resultNeeded = oldNeeded;
    int zpExpr = allocateZP(2);
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpExpr);
    emit("sta $" + ss.str()); emit("txa");
    std::stringstream ssHigh;
    ssHigh << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpExpr) + 1;
    emit("sta $" + ssHigh.str());
    invalidateRegs();
    std::string labelBreak = newLabel();
    SwitchInfo info;
    info.zpExpr = zpExpr;
    info.breakLabel = labelBreak;
    switchStack.push_back(&info);
    loopStack.push_back({"", labelBreak});
    class CaseCollector : public ASTVisitor {
    public:
        SwitchInfo& info;
        CodeGenerator& gen;
        CaseCollector(SwitchInfo& i, CodeGenerator& g) : info(i), gen(g) {}
        void visit(IntegerLiteral&) override {}
        void visit(StringLiteral&) override {}
        void visit(VariableReference&) override {}
        void visit(Assignment& node) override { node.expression->accept(*this); }
        void visit(BinaryOperation& node) override { node.left->accept(*this); node.right->accept(*this); }
        void visit(UnaryOperation& node) override { node.operand->accept(*this); }
        void visit(FunctionCall& node) override { for (auto& arg : node.arguments) arg->accept(*this); }
        void visit(MemberAccess& node) override { node.structExpr->accept(*this); }
        void visit(ConditionalExpression& node) override { node.condition->accept(*this); node.thenExpr->accept(*this); node.elseExpr->accept(*this); }
        void visit(GenericSelection& node) override {
            CodeGenerator::ExpressionType controlType = gen.getExprType(node.control.get());
            for (auto& assoc : node.associations) {
                if (!assoc.isDefault && CodeGenerator::matchType(controlType, assoc.typeName, assoc.pointerLevel)) {
                    assoc.result->accept(*this); return;
                }
            }
            for (auto& assoc : node.associations) {
                if (assoc.isDefault) { assoc.result->accept(*this); return; }
            }
        }
        void visit(ArrayAccess& node) override { node.arrayExpr->accept(*this); node.indexExpr->accept(*this); }
        void visit(CastExpression& node) override { node.expression->accept(*this); }
        void visit(AlignofExpression&) override {}
        void visit(SizeofExpression& node) override { if (!node.isType) node.expression->accept(*this); }
        void visit(VariableDeclaration& node) override { if (node.initializer) node.initializer->accept(*this); }
        void visit(ReturnStatement& node) override { if(node.expression) node.expression->accept(*this); }
        void visit(BreakStatement&) override {}
        void visit(ContinueStatement&) override {}
        void visit(SwitchContinueStatement&) override {}
        void visit(GotoStatement&) override {}
        void visit(LabelledStatement& node) override { node.statement->accept(*this); }
        void visit(ExpressionStatement& node) override { node.expression->accept(*this); }
        void visit(IfStatement& node) override { node.condition->accept(*this); node.thenBranch->accept(*this); if(node.elseBranch) node.elseBranch->accept(*this); }
        void visit(WhileStatement& node) override { node.condition->accept(*this); node.body->accept(*this); }
        void visit(DoWhileStatement& node) override { node.body->accept(*this); node.condition->accept(*this); }
        void visit(ForStatement& node) override {
        if (node.initializer) node.initializer->accept(*this);
        if (node.condition) node.condition->accept(*this);
        if (node.increment) node.increment->accept(*this);
        node.body->accept(*this);
    }
        void visit(SwitchStatement&) override {}
        void visit(CaseStatement& node) override {
            uint32_t val = 0; if (auto* lit = dynamic_cast<IntegerLiteral*>(node.value.get())) val = lit->value;
            std::string l = gen.newLabel(); info.cases.push_back({val, l}); node.label = l;
        }
        void visit(DefaultStatement& node) override {
            std::string l = gen.newLabel(); info.defaultLabel = l; info.hasDefault = true; node.label = l;
        }
        void visit(AsmStatement&) override {}
        void visit(StaticAssert&) override {}
        void visit(EnumDefinition&) override {}
        void visit(StructDefinition&) override {}
        void visit(CompoundStatement& node) override { for(auto& s : node.statements) s->accept(*this); }
        void visit(FunctionDeclaration&) override {}
        void visit(TranslationUnit&) override {}
    };
    CaseCollector collector(info, *this);
    node.body->accept(collector);
    for (auto& c : info.cases) {
        emit("ldax $" + ss.str());
        std::stringstream ssVal;
        ssVal << "#$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << c.value;
        emit("cmp.16 .ax, " + ssVal.str()); emit("beq " + c.label);
    }
    if (info.hasDefault) emit("bra " + info.defaultLabel);
    else emit("bra " + labelBreak);
    node.body->accept(*this);
    out << labelBreak << ":" << std::endl;
    invalidateRegs();
    freeZP(zpExpr, 2);
    switchStack.pop_back();
    loopStack.pop_back();
}

void CodeGenerator::visit(CaseStatement& node) {
    if (!node.label.empty()) { out << node.label << ":" << std::endl; invalidateRegs(); }
}

void CodeGenerator::visit(DefaultStatement& node) {
    if (!node.label.empty()) { out << node.label << ":" << std::endl; invalidateRegs(); }
}

void CodeGenerator::visit(EnumDefinition& node) {}

void CodeGenerator::visit(StructDefinition& node) {
    auto info = std::make_shared<StructInfo>();
    info->name = node.name;
    int currentOffset = 0;
    int maxAlign = 1;
    int maxSize = 0;

    for (auto& member : node.members) {
        int mAlign = 1;
        int mSize = 0;

        if (member.pointerLevel > 0 || member.type == "int") {
            mAlign = 2;
            mSize = 2;
        } else if (is8BitType(member.type)) {
            mAlign = 1;
            mSize = 1;
        } else if (isStruct(member.type)) {
            std::string sName = getAggregateName(member.type);
            if (structs.count(sName)) {
                mAlign = structs[sName]->alignment;
                mSize = structs[sName]->totalSize;
            } else {
                throw std::runtime_error("Unknown struct/union type in member: " + sName);
            }
        }

        if (member.alignment > mAlign) mAlign = member.alignment;
        if (mAlign > maxAlign) maxAlign = mAlign;
        
        if (member.arraySize() >= 0) mSize *= member.arraySize();

        if (!node.isUnion) {
            if (currentOffset % mAlign != 0) currentOffset += mAlign - (currentOffset % mAlign);
        }

        if (member.isAnonymous && isStruct(member.type)) {
            std::string nestedSName = getAggregateName(member.type);
            if (structs.count(nestedSName)) {
                auto& nestedInfo = *structs[nestedSName];
                for (auto const& [mName, mInfo] : nestedInfo.members) {
                    MemberInfo flattenedInfo = mInfo;
                    flattenedInfo.offset += node.isUnion ? 0 : currentOffset;
                    info->members[mName] = flattenedInfo;
                }
                if (!node.isUnion) currentOffset += mSize;
                else if (mSize > maxSize) maxSize = mSize;
            }
        } else {
            MemberInfo mInfo;
            mInfo.type = member.type;
            mInfo.pointerLevel = member.pointerLevel;
            mInfo.isSigned = member.isSigned;
            mInfo.isConst = member.isConst;
            mInfo.offset = node.isUnion ? 0 : currentOffset;
            mInfo.alignment = mAlign;
            mInfo.arrayDims = member.arrayDims;
            if (!member.name.empty()) info->members[member.name] = mInfo;
            if (!node.isUnion) currentOffset += mSize;
            else if (mSize > maxSize) maxSize = mSize;
        }
    }
    if (node.isUnion) currentOffset = maxSize;
    if (currentOffset % maxAlign != 0) currentOffset += maxAlign - (currentOffset % maxAlign);
    info->totalSize = currentOffset;
    info->alignment = maxAlign;
    structs[node.name] = info;
}

void CodeGenerator::visit(MemberAccess& node) {
    ExpressionType baseType = getExprType(node.structExpr.get());
    if (!isStruct(baseType.type)) {
         if (auto* ref = dynamic_cast<VariableReference*>(node.structExpr.get())) {
             if (globalVariableTypes.count("_" + ref->name)) {
                 baseType = {globalVariableTypes.at("_" + ref->name).type, globalVariableTypes.at("_" + ref->name).pointerLevel};
             }
         }
    }
    if (!isStruct(baseType.type)) throw std::runtime_error("Dot/Arrow operator on non-struct type: " + baseType.type);
    std::string sName = getAggregateName(baseType.type);
    if (!structs.count(sName)) throw std::runtime_error("Unknown struct/union type: " + sName);
    auto& sInfo = *structs[sName];
    if (!sInfo.members.count(node.memberName)) throw std::runtime_error("Member " + node.memberName + " not found in struct/union " + sName);
    MemberInfo& mInfo = sInfo.members[node.memberName];

    if (!node.isArrow) {
        if (auto* ref = dynamic_cast<VariableReference*>(node.structExpr.get())) {
            std::string rName = resolveVarName(ref->name);
            embedSource(node);
            if (!resultNeeded) return;
            bool is16 = (mInfo.pointerLevel > 0 || mInfo.type == "int");
            if (isStruct(mInfo.type)) {
                std::string nestedSName = getAggregateName(mInfo.type);
                if (structs.count(nestedSName) && structs[nestedSName]->totalSize > 1) is16 = true;
            }
            bool isGlobal = globalVariableTypes.count(rName);
            std::string suffix = isGlobal ? "" : ", s";
            if (is16) {
                emit("ldax " + rName + "+" + std::to_string(mInfo.offset) + suffix);
                updateRegAVar(rName, mInfo.offset); updateRegXVar(rName, mInfo.offset + 1);
                updateZNFlags(FlagSource::A);
            } else {
                if (isGlobal) emit("lda " + rName + "+" + std::to_string(mInfo.offset));
                else emit("lda.sp " + rName + "+" + std::to_string(mInfo.offset));
                updateRegAVar(rName, mInfo.offset);
                emitter->ldx_imm(0); updateRegX(0);
            }
            return;
        }
    }

    if (node.isArrow) {
        bool oldNeeded = resultNeeded;
        resultNeeded = true;
        node.structExpr->accept(*this);
        resultNeeded = oldNeeded;
    } else {
        emitAddress(node.structExpr.get());
    }
    if (!resultNeeded) { invalidateRegs(); return; }

    if (mInfo.offset > 0) emitter->add_16_imm(mInfo.offset);
    int zpIdx = allocateZP(2);
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpIdx);
    emit("stax $" + ss.str());
    emitter->lda_ind_z(emitter->getZP(zpIdx), false);
    updateRegY(0); regA.known = false;
    bool is16 = (mInfo.pointerLevel > 0 || mInfo.type == "int");
    if (isStruct(mInfo.type)) {
        std::string nestedSName = getAggregateName(mInfo.type);
        if (structs.count(nestedSName) && structs[nestedSName]->totalSize > 1) is16 = true;
    }
    if (is16) {
        emitter->push("a"); emitter->ldy_imm(1); updateRegY(1);
        std::stringstream ss2;
        ss2 << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpIdx);
        emit("lda ($" + ss2.str() + "),y");
        regA.known = false; emitter->tax(); emitter->pop("a");
    } else {
        emitter->ldx_imm(0); updateRegX(0);
    }
    freeZP(zpIdx, 2);
    invalidateRegs();
}

void CodeGenerator::visit(IntegerLiteral& node) {
    if (!resultNeeded) return;
    if (node.value == 0) {
        emit("zero a, x"); updateRegA(0); updateRegX(0); updateZNFlags(FlagSource::A);
        return;
    }
    std::stringstream ss;
    ss << "#$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (int)node.value;
    emit("ldax " + ss.str());
    updateRegA(node.value & 0xFF); updateRegX((node.value >> 8) & 0xFF); updateZNFlags(FlagSource::A);
}

void CodeGenerator::visit(StringLiteral& node) {
    if (!resultNeeded) return;
    if (stringPool.find(node.value) == stringPool.end()) stringPool[node.value] = "STR" + std::to_string(stringCount++);
    std::string label = stringPool[node.value];
    emit("ldax #<" + label); invalidateRegs();
}

void CodeGenerator::visit(VariableReference& node) {
    if (!resultNeeded) return;
    std::string rName = resolveVarName(node.name);
    bool isGlobal = globalVariableTypes.count(rName);
    std::string suffix = isGlobal ? "" : ", s";
    VarInfo vi = variableTypes.count(rName) ? variableTypes.at(rName) : globalVariableTypes.at(rName);

    if (vi.arraySize() >= 0) {
        emitAddress(&node); // This will put address in AX
        return;
    }

    bool is16Bit = (vi.pointerLevel > 0 || vi.type == "int");
    if (isStruct(vi.type)) {
        std::string sName = getAggregateName(vi.type);
        if (structs.count(sName) && structs[sName]->totalSize > 1) is16Bit = true;
    }
    if (is16Bit) {
        bool lowCorrect = (regA.known && regA.isVariable && regA.varName == rName && regA.varOffset == 0);
        bool highCorrect = (regX.known && regX.isVariable && regX.varName == rName && regX.varOffset == 1);
        if (vi.isVolatile) { lowCorrect = false; highCorrect = false; }

        if (lowCorrect && highCorrect) {}
        else if (lowCorrect) { emit("ldx " + rName + "+1" + suffix); updateRegXVar(rName, 1); }
        else if (highCorrect) { emit(isGlobal ? ("lda " + rName) : ("lda.sp " + rName)); updateRegAVar(rName, 0); }
        else {
            emit("ldax " + rName + suffix); updateRegAVar(rName, 0); updateRegXVar(rName, 1); updateZNFlags(FlagSource::A);
        }
    } else {
        bool lowCorrectA = (regA.known && regA.isVariable && regA.varName == rName && regA.varOffset == 0);
        bool lowCorrectX = (regX.known && regX.isVariable && regX.varName == rName && regX.varOffset == 0);
        bool lowCorrectY = (regY.known && regY.isVariable && regY.varName == rName && regY.varOffset == 0);
        bool lowCorrectZ = (regZ.known && regZ.isVariable && regZ.varName == rName && regZ.varOffset == 0);

        if (vi.isVolatile) { lowCorrectA = false; lowCorrectX = false; lowCorrectY = false; lowCorrectZ = false; }

        if (lowCorrectA) {}
        else if (lowCorrectX) {
            emit("txa"); transferRegs(FlagSource::A, FlagSource::X);
        } else if (lowCorrectY) {
            emit("tya"); transferRegs(FlagSource::A, FlagSource::Y);
        } else if (lowCorrectZ) {
            emit("tza"); transferRegs(FlagSource::A, FlagSource::Z);
        } else {
            emit(isGlobal ? ("lda " + rName) : ("lda.sp " + rName)); updateRegAVar(rName, 0);
        }
        if (!regX.known || regX.isVariable || regX.value != 0) { emitter->ldx_imm(0); updateRegX(0); }
    }
}

void CodeGenerator::visit(FunctionCall& node) {
    if (node.name == "_memset" && node.arguments.size() == 3) {
        bool oldNeeded = resultNeeded; resultNeeded = true;
        node.arguments[0]->accept(*this);
        int zpDest = allocateZP(2);
        std::stringstream ssDest; ssDest << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpDest);
        emit("stax $" + ssDest.str());
        node.arguments[1]->accept(*this);
        if (auto* lit = dynamic_cast<IntegerLiteral*>(node.arguments[2].get())) {
            emit("FILL ($" + ssDest.str() + "), #" + std::to_string(lit->value));
        } else {
            node.arguments[2]->accept(*this);
            emit("taz"); emit("txa"); emit("tay"); emit("tza"); emit("tax");
            emit("FILL ($" + ssDest.str() + "), .XY");
        }
        freeZP(zpDest, 2); resultNeeded = oldNeeded; invalidateRegs(); return;
    }
    if (node.name == "_memcpy" && node.arguments.size() == 3) {
        bool oldNeeded = resultNeeded; resultNeeded = true;
        node.arguments[0]->accept(*this);
        int zpDest = allocateZP(2);
        std::stringstream ssDest; ssDest << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpDest);
        emit("stax $" + ssDest.str());
        node.arguments[1]->accept(*this);
        int zpSrc = allocateZP(2);
        std::stringstream ssSrc; ssSrc << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpSrc);
        emit("stax $" + ssSrc.str());
        if (auto* lit = dynamic_cast<IntegerLiteral*>(node.arguments[2].get())) {
            std::stringstream ssLenX, ssLenY;
            ssLenX << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (lit->value & 0xFF);
            ssLenY << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << ((lit->value >> 8) & 0xFF);
            emit("ldx #$" + ssLenX.str()); emit("ldy #$" + ssLenY.str());
        } else {
            node.arguments[2]->accept(*this);
            emit("taz"); emit("txa"); emit("tay"); emit("tza"); emit("tax");
        }
        emit("MOVE ($" + ssSrc.str() + "), ($" + ssDest.str() + ")");
        freeZP(zpSrc, 2); freeZP(zpDest, 2); resultNeeded = oldNeeded; invalidateRegs(); return;
    }
    // Check that the function is declared (definition or prototype)
    if (!knownFunctions.count(node.name)) {
        std::string loc = sourceFilename.empty() ? "" : sourceFilename + ":";
        if (node.line > 0) loc += std::to_string(node.line) + ":" + std::to_string(node.column) + ": ";
        throw std::runtime_error(loc + "error: implicit declaration of function '" + node.name + "' (missing #include or prototype)");
    }

    // Warn on const-correctness violations (passing const pointer to non-const parameter)
    if (functionParamTypes.count(node.name)) {
        auto& paramTypes = functionParamTypes[node.name];
        for (size_t i = 0; i < node.arguments.size() && i < paramTypes.size(); ++i) {
            ExpressionType argType = getExprType(node.arguments[i].get());
            VarInfo& pType = paramTypes[i];
            if (argType.isConst && argType.pointerLevel > 0 && pType.pointerLevel > 0 && !pType.isConst) {
                std::cerr << "warning: passing argument " << (i + 1) << " of '" << node.name
                          << "' discards 'const' qualifier from pointer target type" << std::endl;
            }
        }
    }

    bool oldNeeded = resultNeeded; resultNeeded = true;
    for (auto& arg : node.arguments) {
        if (auto* ref = dynamic_cast<VariableReference*>(arg.get())) {
            std::string rName = resolveVarName(ref->name);
            VarInfo vi = variableTypes.count(rName) ? variableTypes.at(rName) : globalVariableTypes.at(rName);
            bool is16Bit = (vi.pointerLevel > 0 || vi.type == "int");
            if (isStruct(vi.type)) {
                std::string sName = getAggregateName(vi.type);
                if (structs.count(sName) && structs[sName]->totalSize > 1) is16Bit = true;
            }
            if (is16Bit) emit("phw.sp " + rName + ", s");
            else { arg->accept(*this); emitter->push("a"); }
        } else { arg->accept(*this); emitter->push_ax(); }
    }
    int argBytes = (int)node.arguments.size() * 2;
    resultNeeded = oldNeeded; emit("jsr _" + node.name);
    // Caller cleans up pushed arguments using PLA (A saved in Z)
    if (argBytes > 0) {
        emitter->taz();
        for (int i = 0; i < argBytes; ++i) emitter->pla();
        emitter->tza();
    }
    invalidateRegs();
}

void CodeGenerator::emitJumpIfTrue(Expression* cond, const std::string& labelTrue) {
    if (auto* lit = dynamic_cast<IntegerLiteral*>(cond)) {
        if (lit->value != 0) {
            emit("bra " + labelTrue);
        }
        return;
    }
    if (auto* bin = dynamic_cast<BinaryOperation*>(cond)) {
        if (bin->op == "||") { emitJumpIfTrue(bin->left.get(), labelTrue); emitJumpIfTrue(bin->right.get(), labelTrue); return; }
        if (bin->op == "&&") {
            std::string labelFalse = newDontCareLabel(); emitJumpIfFalse(bin->left.get(), labelFalse);
            emitJumpIfTrue(bin->right.get(), labelTrue); out << labelFalse << ":" << std::endl; return;
        }
        if (bin->op == "==" || bin->op == "!=" || bin->op == "<" || bin->op == ">" || bin->op == "<=" || bin->op == ">=") {
            bool oldNeeded = resultNeeded; resultNeeded = true; cond->accept(*this); resultNeeded = oldNeeded;
            if (flags.znSource != FlagSource::A) emit("cmp #$00");
            emit("bne " + labelTrue); return;
        }
    }
    if (auto* un = dynamic_cast<UnaryOperation*>(cond)) if (un->op == "!") { emitJumpIfFalse(un->operand.get(), labelTrue); return; }
    ExpressionType condType = getExprType(cond); bool is8Bit = (is8BitType(condType.type) && condType.pointerLevel == 0);
    bool oldNeeded = resultNeeded; resultNeeded = true; cond->accept(*this); resultNeeded = oldNeeded;
    if (is8Bit) { if (flags.znSource != FlagSource::A) emit("cmp #$00"); emit("bne " + labelTrue); }
    else { if (flags.znSource != FlagSource::A) emit("cmp #$00"); emitBranch16Bne(labelTrue); }
}

void CodeGenerator::emitJumpIfFalse(Expression* cond, const std::string& labelElse) {
    if (auto* lit = dynamic_cast<IntegerLiteral*>(cond)) {
        if (lit->value == 0) {
            emit("bra " + labelElse);
        }
        return;
    }
    if (auto* bin = dynamic_cast<BinaryOperation*>(cond)) {
        if (bin->op == "||") {
            std::string labelThen = newDontCareLabel(); emitJumpIfTrue(bin->left.get(), labelThen);
            emitJumpIfFalse(bin->right.get(), labelElse); out << labelThen << ":" << std::endl; return;
        }
        if (bin->op == "&&") { emitJumpIfFalse(bin->left.get(), labelElse); emitJumpIfFalse(bin->right.get(), labelElse); return; }
        if (bin->op == "==" || bin->op == "!=" || bin->op == "<" || bin->op == ">" || bin->op == "<=" || bin->op == ">=") {
            bool oldNeeded = resultNeeded; resultNeeded = true; cond->accept(*this); resultNeeded = oldNeeded;
            if (flags.znSource != FlagSource::A) emit("cmp #$00");
            emit("beq " + labelElse); return;
        }
    }
    if (auto* un = dynamic_cast<UnaryOperation*>(cond)) if (un->op == "!") { emitJumpIfTrue(un->operand.get(), labelElse); return; }
    ExpressionType condType = getExprType(cond); bool is8Bit = (is8BitType(condType.type) && condType.pointerLevel == 0);
    bool oldNeeded = resultNeeded; resultNeeded = true; cond->accept(*this); resultNeeded = oldNeeded;
    if (is8Bit) { if (flags.znSource != FlagSource::A) emit("cmp #$00"); emit("beq " + labelElse); }
    else { if (flags.znSource != FlagSource::A) emit("cmp #$00"); emitBranch16Beq(labelElse); }
}

void CodeGenerator::visit(CastExpression& node) {
    embedSource(node);
    bool oldNeeded = resultNeeded;
    resultNeeded = true;
    node.expression->accept(*this);
    resultNeeded = oldNeeded;

    if (!resultNeeded) return;

    ExpressionType srcType = getExprType(node.expression.get());
    int srcSize = (srcType.pointerLevel > 0 || srcType.type == "int") ? 2 : 1;
    int dstSize = (node.pointerLevel > 0 || node.targetType == "int" || node.targetType == "void") ? 2 : 1;

    if (node.targetType == "_Bool") {
        emitBoolNormalize(srcSize);
        emitter->ldx_imm(0);
        updateRegX(0);
    } else if (srcSize == 2 && dstSize == 1) {
        // Narrowing: int/pointer -> char, just keep A (low byte), ignore X
        emitter->ldx_imm(0);
        updateRegX(0);
    } else if (srcSize == 1 && dstSize == 2) {
        // Widening: char -> int/pointer, zero-extend X
        emitter->ldx_imm(0);
        updateRegX(0);
    }
    // Same size: no-op (the value is already in A or A:X)
}

void CodeGenerator::visit(AlignofExpression& node) {
    int alignment = 1;
    if (node.pointerLevel > 0 || node.typeName == "int") alignment = 2;
    else if (is8BitType(node.typeName)) alignment = 1;
    else { std::string sName = getAggregateName(node.typeName); if (structs.count(sName)) alignment = structs[sName]->alignment; }
    emitter->lda_imm(alignment & 0xFF); updateRegA(alignment & 0xFF); invalidateFlags();
}

void CodeGenerator::emitData() {
    // Emit .global/.weak for all global variables in relocatable mode
    if (relocMode) {
        for (auto* gVar : globalVars) {
            if (weakGlobals.count(gVar->name)) {
                out << ".weak _" << gVar->name << std::endl;
            } else {
                out << ".global _" << gVar->name << std::endl;
            }
        }
    }

    // Partition globals into initialized (data) and uninitialized (bss)
    std::vector<VariableDeclaration*> uninitializedVars;
    std::vector<VariableDeclaration*> initializedVars;
    for (auto* gVar : globalVars) {
        if (!gVar->initializer) uninitializedVars.push_back(gVar);
        else initializedVars.push_back(gVar);
    }

    // _init_bss is now emitted inline in the CRT stub (before functions)

    // Data section — initialized globals
    out << std::endl << ".data" << std::endl;
    out << "; Data Section" << std::endl;

    for (auto* gVar : initializedVars) {
        if (gVar->alignment > 1) out << "    .align " << std::to_string(gVar->alignment) << std::endl;
        out << "_" << gVar->name << ":" << std::endl;
        int size = 0;
        if (gVar->pointerLevel > 0) size = 2;
        else if (is8BitType(gVar->type)) size = 1;
        else if (gVar->type == "int") size = 2;
        else if (isStruct(gVar->type)) { std::string sName = getAggregateName(gVar->type); if (structs.count(sName)) size = structs[sName]->totalSize; }

        if (gVar->arraySize() >= 0) size *= gVar->arraySize();

        if (auto* lit = dynamic_cast<IntegerLiteral*>(gVar->initializer.get())) {
            if (size == 1) out << "    .byte $" << std::right << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (lit->value & 0xFF) << std::dec << std::endl;
            else if (size == 2) out << "    .word $" << std::right << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (lit->value & 0xFFFF) << std::dec << std::endl;
            else out << "    .res " << std::to_string(size) << ", 0" << std::endl;
        } else {
            out << "    .res " << std::to_string(size) << ", 0" << std::endl;
        }
    }

    // BSS section — uninitialized globals
    if (!uninitializedVars.empty()) {
        out << std::endl << ".bss" << std::endl;
        out << "; BSS Section" << std::endl;
        out << "__bss_start:" << std::endl;
        for (auto* gVar : uninitializedVars) {
            if (gVar->alignment > 1) out << "    .align " << std::to_string(gVar->alignment) << std::endl;
            out << "_" << gVar->name << ":" << std::endl;
            int size = 0;
            if (gVar->pointerLevel > 0) size = 2;
            else if (is8BitType(gVar->type)) size = 1;
            else if (gVar->type == "int") size = 2;
            else if (isStruct(gVar->type)) { std::string sName = getAggregateName(gVar->type); if (structs.count(sName)) size = structs[sName]->totalSize; }
            if (gVar->arraySize() >= 0) size *= gVar->arraySize();
            out << "    .res " << std::to_string(size) << std::endl;
        }
        out << "__bss_end:" << std::endl;
    }

    out << std::endl << ".data" << std::endl;
    for (const auto& entry : stringPool) { out << entry.second << ":" << std::endl; out << "    .text \"" << entry.first << "\"" << std::endl; out << "    .byte 0" << std::endl; }
}

void CodeGenerator::invalidateRegs() {
    regA.known = false; regA.isVariable = false; regA.varName = ""; regA.varOffset = 0; regA.value = 0;
    regX.known = false; regX.isVariable = false; regX.varName = ""; regX.varOffset = 0; regX.value = 0;
    regY.known = false; regY.isVariable = false; regY.varName = ""; regY.varOffset = 0; regY.value = 0;
    regZ.known = false; regZ.isVariable = false; regZ.varName = ""; regZ.varOffset = 0; regZ.value = 0;
    invalidateFlags();
}

void CodeGenerator::invalidateVar(const std::string& name) {
    if (regA.known && regA.isVariable && regA.varName == name) regA.known = false;
    if (regX.known && regX.isVariable && regX.varName == name) regX.known = false;
    if (regY.known && regY.isVariable && regY.varName == name) regY.known = false;
    if (regZ.known && regZ.isVariable && regZ.varName == name) regZ.known = false;
}

void CodeGenerator::transferRegs(FlagSource dest, FlagSource src) {
    RegState* s = nullptr; RegState* d = nullptr;
    if (src == FlagSource::A) s = &regA; else if (src == FlagSource::X) s = &regX; else if (src == FlagSource::Y) s = &regY; else if (src == FlagSource::Z) s = &regZ;
    if (dest == FlagSource::A) d = &regA; else if (dest == FlagSource::X) d = &regX; else if (dest == FlagSource::Y) d = &regY; else if (dest == FlagSource::Z) d = &regZ;
    if (s && d) { *d = *s; updateZNFlags(dest, flags.zero, flags.negative); }
}

void CodeGenerator::updateFlags(TriState c, TriState z, TriState n, TriState v) {
    if (c != TriState::UNKNOWN) flags.carry = c;
    if (z != TriState::UNKNOWN) flags.zero = z;
    if (n != TriState::UNKNOWN) flags.negative = n;
    if (v != TriState::UNKNOWN) flags.overflow = v;
    if (z != TriState::UNKNOWN || n != TriState::UNKNOWN) flags.znSource = FlagSource::NONE;
}

void CodeGenerator::updateZNFlags(FlagSource source, TriState z, TriState n) {
    flags.znSource = source; flags.zero = z; flags.negative = n;
}

void CodeGenerator::invalidateFlags() {
    flags.carry = TriState::UNKNOWN; flags.zero = TriState::UNKNOWN; flags.negative = TriState::UNKNOWN; flags.overflow = TriState::UNKNOWN; flags.znSource = FlagSource::NONE;
}

void CodeGenerator::updateRegA(uint8_t val) { 
    regA.known = true; regA.isVariable = false; regA.value = val; 
    updateZNFlags(FlagSource::A, val == 0 ? TriState::SET : TriState::CLEAR, (val & 0x80) ? TriState::SET : TriState::CLEAR);
}
void CodeGenerator::updateRegX(uint8_t val) { 
    regX.known = true; regX.isVariable = false; regX.value = val; 
    updateZNFlags(FlagSource::X, val == 0 ? TriState::SET : TriState::CLEAR, (val & 0x80) ? TriState::SET : TriState::CLEAR);
}
void CodeGenerator::updateRegY(uint8_t val) { 
    regY.known = true; regY.isVariable = false; regY.value = val; 
    updateZNFlags(FlagSource::Y, val == 0 ? TriState::SET : TriState::CLEAR, (val & 0x80) ? TriState::SET : TriState::CLEAR);
}
void CodeGenerator::updateRegZ(uint8_t val) { 
    regZ.known = true; regZ.isVariable = false; regZ.value = val; 
    updateZNFlags(FlagSource::Z, val == 0 ? TriState::SET : TriState::CLEAR, (val & 0x80) ? TriState::SET : TriState::CLEAR);
}
void CodeGenerator::updateRegAVar(const std::string& name, int offset) { regA.known = true; regA.isVariable = true; regA.varName = name; regA.varOffset = offset; updateZNFlags(FlagSource::A); }
void CodeGenerator::updateRegXVar(const std::string& name, int offset) { regX.known = true; regX.isVariable = true; regX.varName = name; regX.varOffset = offset; updateZNFlags(FlagSource::X); }
void CodeGenerator::updateRegYVar(const std::string& name, int offset) { regY.known = true; regY.isVariable = true; regY.varName = name; regY.varOffset = offset; updateZNFlags(FlagSource::Y); }
void CodeGenerator::updateRegZVar(const std::string& name, int offset) { regZ.known = true; regZ.isVariable = true; regZ.varName = name; regZ.varOffset = offset; updateZNFlags(FlagSource::Z); }

int CodeGenerator::allocateZP(int size) {
    for (int i = 0; i <= (int)zpRegs.size() - size; ++i) {
        bool found = true; for (int j = 0; j < size; ++j) if (zpRegs[i + j].inUse) { found = false; break; }
        if (found) { for (int j = 0; j < size; ++j) zpRegs[i + j].inUse = true; return i; }
    }
    int oldSize = zpRegs.size();
    for (int i = 0; i < size; ++i) zpRegs.push_back({true});
    return oldSize;
}

void CodeGenerator::freeZP(int index, int size) {
    for (int i = 0; i < size; ++i) zpRegs[index + i].inUse = false;
}

class VariableUseChecker : public ASTVisitor {
public:
    std::string targetVarName; bool used = false; std::string currentDeclVarName;
    CodeGenerator& gen;
    VariableUseChecker(const std::string& name, const std::string& currentDecl, CodeGenerator& g) : targetVarName(name), currentDeclVarName(currentDecl), gen(g) {}
    void visit(IntegerLiteral&) override {}
    void visit(StringLiteral&) override {}
    void visit(VariableReference& node) override { if (node.name == targetVarName) used = true; }
    void visit(Assignment& node) override { if (auto* ref = dynamic_cast<VariableReference*>(node.target.get())) if (ref->name == targetVarName) used = true; node.expression->accept(*this); if (node.target) node.target->accept(*this); }
    void visit(BinaryOperation& node) override { node.left->accept(*this); node.right->accept(*this); }
    void visit(UnaryOperation& node) override { node.operand->accept(*this); }
    void visit(FunctionCall& node) override { for (auto& arg : node.arguments) arg->accept(*this); }
    void visit(MemberAccess& node) override { node.structExpr->accept(*this); }
    void visit(ConditionalExpression& node) override { node.condition->accept(*this); node.thenExpr->accept(*this); node.elseExpr->accept(*this); }
    void visit(GenericSelection& node) override {
        CodeGenerator::ExpressionType controlType = gen.getExprType(node.control.get());
        for (auto& assoc : node.associations) {
            if (!assoc.isDefault && CodeGenerator::matchType(controlType, assoc.typeName, assoc.pointerLevel)) {
                assoc.result->accept(*this); return;
            }
        }
        for (auto& assoc : node.associations) {
            if (assoc.isDefault) { assoc.result->accept(*this); return; }
        }
    }
    void visit(ArrayAccess& node) override { node.arrayExpr->accept(*this); node.indexExpr->accept(*this); }
    void visit(CastExpression& node) override { node.expression->accept(*this); }
    void visit(AlignofExpression&) override {}
    void visit(SizeofExpression& node) override { if (!node.isType) node.expression->accept(*this); }
    void visit(VariableDeclaration& node) override { if (node.initializer) node.initializer->accept(*this); }
    void visit(ReturnStatement& node) override { if(node.expression) node.expression->accept(*this); }
    void visit(BreakStatement&) override {}
    void visit(ContinueStatement&) override {}
    void visit(SwitchContinueStatement& node) override { if (node.target) node.target->accept(*this); }
    void visit(GotoStatement&) override {}
    void visit(LabelledStatement& node) override { node.statement->accept(*this); }
    void visit(ExpressionStatement& node) override { if (node.expression) node.expression->accept(*this); }
    void visit(IfStatement& node) override { node.condition->accept(*this); node.thenBranch->accept(*this); if(node.elseBranch) node.elseBranch->accept(*this); }
    void visit(WhileStatement& node) override { node.condition->accept(*this); node.body->accept(*this); }
    void visit(DoWhileStatement& node) override { node.body->accept(*this); node.condition->accept(*this); }
    void visit(ForStatement& node) override {
        if (node.initializer) node.initializer->accept(*this);
        if (node.condition) node.condition->accept(*this);
        if (node.increment) node.increment->accept(*this);
        node.body->accept(*this);
    }
    void visit(SwitchStatement& node) override { node.expression->accept(*this); node.body->accept(*this); }
    void visit(CaseStatement& node) override { node.value->accept(*this); }
    void visit(DefaultStatement&) override {}
    void visit(AsmStatement&) override {}
    void visit(StaticAssert&) override {}
    void visit(EnumDefinition&) override {}
    void visit(StructDefinition&) override {}
    void visit(CompoundStatement& node) override { for(auto& s : node.statements) s->accept(*this); }
    void visit(FunctionDeclaration& node) override { node.body->accept(*this); }
    void visit(TranslationUnit& node) override { for(auto& decl : node.topLevelDecls) decl->accept(*this); }
};

bool CodeGenerator::isVariableUsed(const std::string& varName, FunctionDeclaration& func) {
    VariableUseChecker checker(varName, varName, *this);
    func.body->accept(checker);
    return checker.used;
}
