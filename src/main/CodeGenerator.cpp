#include "CodeGenerator.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <set>

CodeGenerator::CodeGenerator(std::ostream& out) : out(out) {}

static std::string hex8(uint8_t val) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)val;
    return ss.str();
}

static std::string hex16(uint16_t val) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (int)val;
    return ss.str();
}

static std::string hex32(uint32_t val) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << (int)val;
    return ss.str();
}

std::string CodeGenerator::zpHex(uint8_t addr) const {
    std::stringstream ss;
    ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)addr;
    return ss.str();
}

int CodeGenerator::getTypeSizeWithStructs(const std::string& type, int ptrLevel, int arraySize) {
    // Build aggregate map from our structs
    std::map<std::string, TypeSystem::AggregateInfo> aggregates;
    for (const auto& [name, structPtr] : structs) {
        aggregates[name] = {name, structPtr->totalSize};
    }

    // Get base type size from TypeSystem
    int size = TypeSystem::getTypeSize(type, ptrLevel, aggregates);

    // Multiply by array size if provided (arraySize >= 0 means it's specified)
    if (arraySize >= 0) {
        size *= arraySize;
    }

    return size;
}

void CodeGenerator::setSourceInfo(const std::string& filename, const std::vector<std::string>& lines) {
    sourceFilename = filename;
    sourceLines = lines;
}

void CodeGenerator::embedSource(ASTNode& node) {
    if (node.line <= 0 || node.line > (int)sourceLines.size()) return;
    if (node.line == lastEmbeddedLine) return;
    lastEmbeddedLine = node.line;
    std::string locFile = node.sourceFile.empty() ? sourceFilename : node.sourceFile;

    // If we have a line mapping, use it to find the context for this line
    int displayLine = node.line;
    if (!lineToFileMap.empty()) {
        // Find the context that applies to this line
        // lineToFileMap stores the starting absolute line → (filename, lineOffset)
        // We need to find the most recent context that is <= our line
        for (auto it = lineToFileMap.rbegin(); it != lineToFileMap.rend(); ++it) {
            if (it->first <= node.line) {
                const auto& context = it->second;
                locFile = context.first;
                // Convert absolute line to file-relative line
                displayLine = context.second + (node.line - it->first);
                break;
            }
        }
    }

    if (displayLine > 0 && displayLine <= (int)sourceLines.size()) {
        out << "; [" << locFile << ":" << displayLine << "] " << sourceLines[displayLine - 1] << std::endl;
    }
    out << ".loc \"" << locFile << "\", " << displayLine << std::endl;
}

void CodeGenerator::emitNarrowingWarning(ASTNode& node, const std::string& fromType, int fromPtr, const std::string& toType, int toPtr) {
    if (toType == "_Bool") return; // conversion to _Bool is always valid (C99)
    if (toType == "float" || fromType == "float") return; // float handled by IRCodeGen
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
    // Reserve first ZP slot as permanent scratch byte for simulated ops
    int scratchIdx = allocateZP(1);
    uint8_t scratchAddr = emitter->getZP(scratchIdx);
    emitter->setScratchZP(scratchAddr);
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

    // Search parent scopes for nested functions
    for (int i = (int)functionStack_.size() - 1; i >= 0; i--) {
        auto& scope = functionStack_[i];
        if (scope.variableTypes.count(pName)) return pName;
        if (scope.variableTypes.count(lName)) return lName;
    }

    // Check for static local variable (stored as global with mangled name)
    if (currentFunction) {
        std::string slName = "_" + currentFunction->name + "__" + name;
        if (globalVariableTypes.count(slName)) return slName;
    }
    std::string gName = "_" + name;
    if (globalVariableTypes.count(gName)) return gName;
    return name;
}

CodeGenerator::VarInfo CodeGenerator::lookupVar(const std::string& rName, ASTNode* node) {
    auto it = variableTypes.find(rName);
    if (it != variableTypes.end()) return it->second;

    // Search parent scopes for nested functions
    for (int i = (int)functionStack_.size() - 1; i >= 0; i--) {
        auto& scope = functionStack_[i];
        auto pit = scope.variableTypes.find(rName);
        if (pit != scope.variableTypes.end()) return pit->second;
    }

    auto git = globalVariableTypes.find(rName);
    if (git != globalVariableTypes.end()) return git->second;
    // Strip prefixes for the error message
    std::string displayName = rName;
    if (displayName.length() >= 3 && (displayName.substr(0, 3) == "_p_" || displayName.substr(0, 3) == "_l_"))
        displayName = displayName.substr(3);
    else if (!displayName.empty() && displayName[0] == '_')
        displayName = displayName.substr(1);
    std::string loc = sourceFilename.empty() ? "" : sourceFilename + ":";
    if (node && node->line > 0) loc += std::to_string(node->line) + ": ";
    else if (!loc.empty()) loc += " ";
    throw std::runtime_error(loc + "error: use of undeclared identifier '" + displayName + "'");
}

std::string CodeGenerator::getLocalOffsetSymbol(int offset) {
    for (const auto& [name, varOffset] : frameLocals_) {
        if (varOffset == offset) {
            return name;
        }
        if (varOffset < offset) {
            int diff = offset - varOffset;
            if (diff < 256) {
                return name + "+" + std::to_string(diff);
            }
        }
    }
    return std::to_string(offset);
}

bool CodeGenerator::matchType(const ExpressionType& t1, const std::string& t2Name, int t2Ptr) {
    if (t1.type != t2Name) return false;
    if (t1.pointerLevel != t2Ptr) return false;
    return true;
}


CodeGenerator::ExpressionType CodeGenerator::getExprType(Expression* expr) {
    if (!expr) return {"int", 0, false, false, false, false, {}}; 
    if (auto* cra = dynamic_cast<CpuRegisterAccess*>(expr)) {
        std::string reg = cra->regName;
        std::transform(reg.begin(), reg.end(), reg.begin(), ::toupper);
        if (reg == "AX" || reg == "AY" || reg == "AZ" || reg == "XY" || reg == "SP") return {"int", 0, false, false, false, false, {}}; 
        if (reg == "Q" || reg == "AXYZ") return {"long", 0, false, false, false, false, {}}; 
        return {"char", 0, false, false, false, false, {}}; 
    }
    if (auto* cfa = dynamic_cast<CpuFlagAccess*>(expr)) {
        return {"_Bool", 0, false, false, false, false, {}}; 
    }
    if (auto* lit = dynamic_cast<IntegerLiteral*>(expr)) {
        if (!lit->castType.empty()) {
            return {lit->castType, lit->castPointerLevel, lit->castIsSigned, false, false, false, {}};
        }
        return {"int", 0, false, false, false, false, {}};
    }
    if (auto* cast = dynamic_cast<CastExpression*>(expr)) {
        return {cast->targetType, cast->pointerLevel, cast->isSigned, false, false, false, {}};
    }
    if (auto* cl = dynamic_cast<CompoundLiteral*>(expr)) {
        // Scalar compound literal produces a value; struct/array produces a pointer
        if (cl->arrayDims.empty() && !isStruct(cl->targetType) && cl->pointerLevel == 0) {
            return {cl->targetType, 0, cl->isSigned, false, false, false, {}};
        }
        return {cl->targetType, cl->pointerLevel + 1, cl->isSigned, false, false, false, {}};
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
            return {vi.type, pl, vi.isSigned, vi.isConst, vi.isPointerConst, false, {}}; 
        }
        std::string rName = "_l_" + ref->name;
        if (variableTypes.count(rName)) {
            VarInfo& vi = variableTypes.at(rName);
            int pl = vi.pointerLevel + (int)vi.arrayDims.size();
            return {vi.type, pl, vi.isSigned, vi.isConst, vi.isPointerConst, false, {}}; 
        }
        // Check static local variable (mangled as global)
        if (currentFunction) {
            std::string slName = "_" + currentFunction->name + "__" + ref->name;
            if (globalVariableTypes.count(slName)) {
                VarInfo& vi = globalVariableTypes.at(slName);
                int pl = vi.pointerLevel + (int)vi.arrayDims.size();
                return {vi.type, pl, vi.isSigned, vi.isConst, vi.isPointerConst, false, {}}; 
            }
        }
        if (globalVariableTypes.count("_" + ref->name)) {
            VarInfo& vi = globalVariableTypes.at("_" + ref->name);
            int pl = vi.pointerLevel + (int)vi.arrayDims.size();
            return {vi.type, pl, vi.isSigned, vi.isConst, vi.isPointerConst, false, {}}; 
        }
    }
    if (auto* vd = dynamic_cast<VariableDeclaration*>(expr)) {
        return {vd->type, vd->pointerLevel, vd->isSigned, vd->isConst, vd->isPointerConst, false, {}}; 
    }
    if (auto* aa = dynamic_cast<ArrayAccess*>(expr)) {
        ExpressionType base = getExprType(aa->arrayExpr.get());
        if (base.pointerLevel > 0) return {base.type, base.pointerLevel - 1, base.isSigned, base.isConst, false, false, {}}; 
        return {base.type, 0, base.isSigned, base.isConst, false, false, {}}; 
    }
    if (auto* vaArg = dynamic_cast<BuiltinVaArg*>(expr)) {
        return {vaArg->typeName, vaArg->pointerLevel, vaArg->isSigned, false, false, false, {}}; 
    }
    if (auto* vaStart = dynamic_cast<BuiltinVaStart*>(expr)) {
        return {"void", 0, false, false, false, false, {}};  
    }
    if (auto* fc = dynamic_cast<FunctionCall*>(expr)) {
        if (functionReturnTypes.count(fc->name)) {
            auto& ri = functionReturnTypes[fc->name];
            return {ri.type, ri.pointerLevel, ri.isSigned, false, false, false, {}}; 
        }
    }
    if (auto* cond = dynamic_cast<ConditionalExpression*>(expr)) {
        return getExprType(cond->thenExpr.get());
    }
    if (auto* bin = dynamic_cast<BinaryOperation*>(expr)) { 
        ExpressionType lhs = getExprType(bin->left.get());
        ExpressionType rhs = getExprType(bin->right.get());
        bool resSigned = lhs.isSigned || rhs.isSigned;
        return {lhs.type, lhs.pointerLevel, resSigned, lhs.isConst, false, false, {}}; 
    }
    if (auto* un = dynamic_cast<UnaryOperation*>(expr)) {
        if (un->op == "!") return {"char", 0, false, false, false, false, {}}; 
        CodeGenerator::ExpressionType sub = getExprType(un->operand.get());
        if (un->op == "*") return {sub.type, sub.pointerLevel > 0 ? sub.pointerLevel - 1 : 0, sub.isSigned, sub.isConst, false, false, {}}; 
        if (un->op == "&") return {sub.type, sub.pointerLevel + 1, sub.isSigned, sub.isConst, false, false, {}}; 
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
                    if (mInfo.arraySize() >= 0) return {mInfo.type, mInfo.pointerLevel + 1, mInfo.isSigned, mInfo.isConst, false, false, {}}; 
                    return {mInfo.type, mInfo.pointerLevel, mInfo.isSigned, mInfo.isConst, false, false, {}}; 
                }
            }
        }
    }
    return {"int", 0, false, false, false, false, {}}; 
}

void CodeGenerator::emitAddress(Expression* expr) {
    // Compound literal already produces its address via leax.fp
    if (auto* cl = dynamic_cast<CompoundLiteral*>(expr)) {
        cl->accept(*this);
        return;
    }
    if (auto* ref = dynamic_cast<VariableReference*>(expr)) {
        // Check if this is a function name (address-of-function)
        if (knownFunctions.count(ref->name) && !variableTypes.count("_l_" + ref->name) &&
            !variableTypes.count("_p_" + ref->name) && !globalVariableTypes.count("_" + ref->name)) {
            emit("lda #<_" + ref->name);
            emit("ldx #>_" + ref->name);
            invalidateRegs();
            return;
        }
        // ZP calling convention: spilled params have a stable frame address
        std::string resolvedName = resolveVarName(ref->name);
        if (isZpSpilledParam(resolvedName)) {
            auto& sp = zpSpilledParams_[resolvedName];
            emit("leax.fp " + getLocalOffsetSymbol(sp.frameOffset));
            invalidateRegs();
            return;
        }
        // ZP calling convention: non-spilled param address is a fixed ZP address
        if (isZpParam(resolvedName)) {
            auto& zpi = zpParams_[resolvedName];
            emitter->lda_imm(zpi.zpAddr);
            emitter->ldx_imm(0);
            invalidateRegs();
            return;
        }
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
                int scalarSize = getTypeSizeWithStructs(vi->type, vi->pointerLevel, -1);
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
        for (auto& arg : node.arguments) if (arg) arg->accept(*this);
    }
    void visit(BuiltinVaStart& node) override { if (node.ap) node.ap->accept(*this); }
    void visit(BuiltinVaArg& node) override { if (node.ap) node.ap->accept(*this); }
    void visit(CpuRegisterAccess&) override {}
    void visit(CpuFlagAccess&) override {}
    void visit(LabelAddressExpression&) override {}
    // Walk all node types that can contain expressions
    void visit(IntegerLiteral&) override {}
    void visit(FloatLiteral&) override {}
    void visit(StringLiteral&) override {}
    void visit(VariableReference&) override {}
    void visit(Assignment& n) override { if (n.target) n.target->accept(*this); if (n.expression) n.expression->accept(*this); }
    void visit(BinaryOperation& n) override { if (n.left) n.left->accept(*this); if (n.right) n.right->accept(*this); }
    void visit(UnaryOperation& n) override { if (n.operand) n.operand->accept(*this); }
    void visit(ConditionalExpression& n) override { if (n.condition) n.condition->accept(*this); if (n.thenExpr) n.thenExpr->accept(*this); if (n.elseExpr) n.elseExpr->accept(*this); }
    void visit(GenericSelection& n) override { if (n.control) n.control->accept(*this); for (auto& a : n.associations) if (a.result) a.result->accept(*this); }
    void visit(InitializerList& n) override { for (auto& e : n.elements) if (e) e->accept(*this); }
    void visit(ArrayAccess& n) override { if (n.arrayExpr) n.arrayExpr->accept(*this); if (n.indexExpr) n.indexExpr->accept(*this); }
    void visit(MemberAccess& n) override { if (n.structExpr) n.structExpr->accept(*this); }
    void visit(CastExpression& n) override { if (n.expression) n.expression->accept(*this); }
    void visit(CompoundLiteral& n) override { if (n.initializer) for (auto& e : n.initializer->elements) if (e) e->accept(*this); }
    void visit(AlignofExpression&) override {}
    void visit(SizeofExpression& n) override { if (!n.isType && n.expression) n.expression->accept(*this); }
    void visit(VariableDeclaration& n) override { if (n.initializer) n.initializer->accept(*this); }
    void visit(ReturnStatement& n) override { if (n.expression) n.expression->accept(*this); }
    void visit(BreakStatement&) override {}
    void visit(ContinueStatement&) override {}
    void visit(SwitchContinueStatement& n) override { if (n.target) n.target->accept(*this); }
    void visit(GotoStatement& n) override { if (n.target) n.target->accept(*this); }
    void visit(LabelledStatement& n) override { if (n.statement) n.statement->accept(*this); }
    void visit(ExpressionStatement& n) override { if (n.expression) n.expression->accept(*this); }
    void visit(IfStatement& n) override { if (n.condition) n.condition->accept(*this); if (n.thenBranch) n.thenBranch->accept(*this); if (n.elseBranch) n.elseBranch->accept(*this); }
    void visit(WhileStatement& n) override { if (n.condition) n.condition->accept(*this); if (n.body) n.body->accept(*this); }
    void visit(DoWhileStatement& n) override { if (n.body) n.body->accept(*this); if (n.condition) n.condition->accept(*this); }
    void visit(ForStatement& n) override { if (n.initializer) n.initializer->accept(*this); if (n.condition) n.condition->accept(*this); if (n.increment) n.increment->accept(*this); if (n.body) n.body->accept(*this); }
    void visit(RepeatStatement& n) override { if (n.body) n.body->accept(*this); }
    void visit(SwitchStatement& n) override { if (n.expression) n.expression->accept(*this); if (n.body) n.body->accept(*this); }
    void visit(CaseStatement& n) override { if (n.value) n.value->accept(*this); if (n.rangeEnd) n.rangeEnd->accept(*this); }
    void visit(DefaultStatement&) override {}
    void visit(AsmStatement&) override {}
    void visit(StaticAssert&) override {}
    void visit(EnumDefinition&) override {}
    void visit(StructDefinition&) override {}
    void visit(CompoundStatement& n) override { for (auto& s : n.statements) if (s) s->accept(*this); }
    void visit(FunctionDeclaration& n) override { if (n.body) n.body->accept(*this); }
    void visit(TranslationUnit& n) override { for (auto& d : n.topLevelDecls) if (d) d->accept(*this); }
};

// Collect parameter names whose address is taken (&param) in a function body.
// Used in zpCall mode to identify params that must be spilled from ZP to the stack frame.
class AddressOfParamCollector : public ASTVisitor {
public:
    std::set<std::string> addressTakenParams; // _p_name entries
    const std::set<std::string>& paramNames_;  // set of _p_name for this function

    AddressOfParamCollector(const std::set<std::string>& paramNames) : paramNames_(paramNames) {}

    void visit(UnaryOperation& n) override {
        if (n.op == "&") {
            if (auto* ref = dynamic_cast<VariableReference*>(n.operand.get())) {
                std::string pName = "_p_" + ref->name;
                if (paramNames_.count(pName)) addressTakenParams.insert(pName);
            }
        }
        if (n.operand) n.operand->accept(*this);
    }
    void visit(IntegerLiteral&) override {}
    void visit(FloatLiteral&) override {}
    void visit(StringLiteral&) override {}
    void visit(VariableReference&) override {}
    void visit(Assignment& n) override { if (n.target) n.target->accept(*this); if (n.expression) n.expression->accept(*this); }
    void visit(BinaryOperation& n) override { if (n.left) n.left->accept(*this); if (n.right) n.right->accept(*this); }
    void visit(ConditionalExpression& n) override { if (n.condition) n.condition->accept(*this); if (n.thenExpr) n.thenExpr->accept(*this); if (n.elseExpr) n.elseExpr->accept(*this); }
    void visit(GenericSelection& n) override { if (n.control) n.control->accept(*this); for (auto& a : n.associations) if (a.result) a.result->accept(*this); }
    void visit(InitializerList& n) override { for (auto& e : n.elements) if (e) e->accept(*this); }
    void visit(ArrayAccess& n) override { if (n.arrayExpr) n.arrayExpr->accept(*this); if (n.indexExpr) n.indexExpr->accept(*this); }
    void visit(FunctionCall& n) override { for (auto& arg : n.arguments) if (arg) arg->accept(*this); }
    void visit(MemberAccess& n) override { if (n.structExpr) n.structExpr->accept(*this); }
    void visit(CastExpression& n) override { if (n.expression) n.expression->accept(*this); }
    void visit(CompoundLiteral& n) override { if (n.initializer) for (auto& e : n.initializer->elements) if (e) e->accept(*this); }
    void visit(AlignofExpression&) override {}
    void visit(SizeofExpression& n) override { if (!n.isType && n.expression) n.expression->accept(*this); }
    void visit(BuiltinVaStart& n) override { if (n.ap) n.ap->accept(*this); }
    void visit(BuiltinVaArg& n) override { if (n.ap) n.ap->accept(*this); }
    void visit(CpuRegisterAccess&) override {}
    void visit(CpuFlagAccess&) override {}
    void visit(LabelAddressExpression&) override {}
    void visit(VariableDeclaration& n) override { if (n.initializer) n.initializer->accept(*this); }
    void visit(ReturnStatement& n) override { if (n.expression) n.expression->accept(*this); }
    void visit(BreakStatement&) override {}
    void visit(ContinueStatement&) override {}
    void visit(SwitchContinueStatement& n) override { if (n.target) n.target->accept(*this); }
    void visit(GotoStatement& n) override { if (n.target) n.target->accept(*this); }
    void visit(LabelledStatement& n) override { if (n.statement) n.statement->accept(*this); }
    void visit(ExpressionStatement& n) override { if (n.expression) n.expression->accept(*this); }
    void visit(IfStatement& n) override { if (n.condition) n.condition->accept(*this); if (n.thenBranch) n.thenBranch->accept(*this); if (n.elseBranch) n.elseBranch->accept(*this); }
    void visit(WhileStatement& n) override { if (n.condition) n.condition->accept(*this); if (n.body) n.body->accept(*this); }
    void visit(DoWhileStatement& n) override { if (n.body) n.body->accept(*this); if (n.condition) n.condition->accept(*this); }
    void visit(ForStatement& n) override { if (n.initializer) n.initializer->accept(*this); if (n.condition) n.condition->accept(*this); if (n.increment) n.increment->accept(*this); if (n.body) n.body->accept(*this); }
    void visit(RepeatStatement& n) override { if (n.body) n.body->accept(*this); }
    void visit(SwitchStatement& n) override { if (n.expression) n.expression->accept(*this); if (n.body) n.body->accept(*this); }
    void visit(CaseStatement& n) override { if (n.value) n.value->accept(*this); if (n.rangeEnd) n.rangeEnd->accept(*this); }
    void visit(DefaultStatement&) override {}
    void visit(AsmStatement&) override {}
    void visit(StaticAssert&) override {}
    void visit(EnumDefinition&) override {}
    void visit(StructDefinition&) override {}
    void visit(CompoundStatement& n) override { for (auto& s : n.statements) if (s) s->accept(*this); }
    void visit(FunctionDeclaration& n) override { if (n.body) n.body->accept(*this); }
    void visit(TranslationUnit& n) override { for (auto& d : n.topLevelDecls) if (d) d->accept(*this); }
};

// Try to extract a compile-time constant integer from an expression tree.
// Handles IntegerLiteral, CastExpression, unary -/~, and binary ops on constants.
static bool tryEvalConstInt(Expression* expr, int& result) {
    if (auto* lit = dynamic_cast<IntegerLiteral*>(expr)) {
        result = lit->value;
        return true;
    }
    if (auto* cast = dynamic_cast<CastExpression*>(expr)) {
        return tryEvalConstInt(cast->expression.get(), result);
    }
    if (auto* unary = dynamic_cast<UnaryOperation*>(expr)) {
        int val;
        if (!tryEvalConstInt(unary->operand.get(), val)) return false;
        if (unary->op == "-") { result = -val; return true; }
        if (unary->op == "~") { result = ~val; return true; }
        return false;
    }
    if (auto* bin = dynamic_cast<BinaryOperation*>(expr)) {
        int l, r;
        if (!tryEvalConstInt(bin->left.get(), l) || !tryEvalConstInt(bin->right.get(), r)) return false;
        if (bin->op == "+") { result = l + r; return true; }
        if (bin->op == "-") { result = l - r; return true; }
        if (bin->op == "*") { result = l * r; return true; }
        if (bin->op == "/" && r != 0) { result = l / r; return true; }
        if (bin->op == "%" && r != 0) { result = l % r; return true; }
        if (bin->op == "&") { result = l & r; return true; }
        if (bin->op == "|") { result = l | r; return true; }
        if (bin->op == "^") { result = l ^ r; return true; }
        if (bin->op == "<<") { result = l << r; return true; }
        if (bin->op == ">>") { result = l >> r; return true; }
        return false;
    }
    return false;
}

// Resolves _Alignas(expr) or _Alignas(type) to an integer alignment value.
// Returns 1 (no alignment) if expr is null, zero, or cannot be evaluated.
static int resolveAlignmentExpr(
    Expression* expr,
    const std::map<std::string, std::shared_ptr<CodeGenerator::StructInfo>>& structs)
{
    if (!expr) return 1;
    // _Alignas(type) → AlignofExpression
    if (auto* ae = dynamic_cast<AlignofExpression*>(expr)) {
        if (ae->pointerLevel > 0 || ae->typeName == "int") return 2;
        if (ae->typeName == "char" || ae->typeName == "_Bool") return 1;
        if (ae->typeName == "long") return 4;
        // Strip struct/union prefix to get aggregate name
        std::string sName = ae->typeName;
        if (sName.length() >= 7 && sName.substr(0, 7) == "struct ") sName = sName.substr(7);
        else if (sName.length() >= 6 && sName.substr(0, 6) == "union ") sName = sName.substr(6);
        if (structs.count(sName)) return structs.at(sName)->alignment;
        return 1;
    }
    // _Alignas(16) or _Alignas(1<<4) → constant expression
    int val = 1;
    if (tryEvalConstInt(expr, val) && val > 0 && (val & (val - 1)) == 0)
        return val;
    return 1;  // non-power-of-two or unevaluable → no alignment
}

// Pre-scan a function body to compute frame layout.
// Assigns a fixed frame offset to every local variable, with scope-aware slot reuse
// for variables in non-overlapping scopes.
class FrameScanner : public ASTVisitor {
public:
    struct LocalInfo {
        std::string name;   // _l_name
        int size;
        int frameOffset;
    };
    std::vector<LocalInfo> locals;
    int maxFrameSize = 0;

    FrameScanner(const std::map<std::string, std::shared_ptr<CodeGenerator::StructInfo>>& s) : structs_(s) {}

    void scan(CompoundStatement& body) {
        scopeBase_ = 0;
        currentOffset_ = 0;
        // Walk the body's statements directly (not through visit(CompoundStatement))
        // to avoid the scope reset for the function's top-level block
        for (auto& s : body.statements) if (s) s->accept(*this);
    }

    void visit(VariableDeclaration& node) override {
        if (node.isGlobal || node.isExtern || node.isStatic) return;
        // Register vars handled separately by CodeGenerator
        if (node.isRegister && node.arraySize() < 0 &&
            node.type != "struct " && node.type.substr(0, 7) != "struct " &&
            node.type.substr(0, 6) != "union ") return;

        std::string lName = "_l_" + node.name;
        int size = computeSize(node);

        // Round currentOffset_ up to satisfy _Alignas requirement
        int align = resolveAlignmentExpr(node.alignmentExpr.get(), structs_);
        if (align > 1 && currentOffset_ % align != 0)
            currentOffset_ += align - (currentOffset_ % align);

        locals.push_back({lName, size, currentOffset_});
        currentOffset_ += size;
        if (currentOffset_ > maxFrameSize) maxFrameSize = currentOffset_;

        // Walk initializer to find compound literals that need frame space
        if (node.initializer) node.initializer->accept(*this);
    }

    // Scope tracking: CompoundStatement that is NOT the function body
    // gets scope enter/exit for slot reuse
    void visit(CompoundStatement& node) override {
        int savedOffset = currentOffset_;
        int savedBase = scopeBase_;
        scopeBase_ = currentOffset_;
        for (auto& s : node.statements) s->accept(*this);
        // After leaving the scope, release the slots (allow reuse)
        currentOffset_ = savedOffset;
        scopeBase_ = savedBase;
        // But maxFrameSize retains the high-water mark
    }

    // Walk all other node types
    void visit(IntegerLiteral&) override {}
    void visit(FloatLiteral&) override {}
    void visit(StringLiteral&) override {}
    void visit(VariableReference&) override {}
    void visit(Assignment& n) override { n.target->accept(*this); n.expression->accept(*this); }
    void visit(BinaryOperation& n) override { if (n.left) n.left->accept(*this); if (n.right) n.right->accept(*this); }
    void visit(UnaryOperation& n) override { if (n.operand) n.operand->accept(*this); }
    void visit(ConditionalExpression& n) override { if (n.condition) n.condition->accept(*this); if (n.thenExpr) n.thenExpr->accept(*this); if (n.elseExpr) n.elseExpr->accept(*this); }
    void visit(GenericSelection& n) override { if (n.control) n.control->accept(*this); for (auto& a : n.associations) if (a.result) a.result->accept(*this); }
    void visit(InitializerList& n) override { for (auto& e : n.elements) if (e) e->accept(*this); }
    void visit(ArrayAccess& n) override { if (n.arrayExpr) n.arrayExpr->accept(*this); if (n.indexExpr) n.indexExpr->accept(*this); }
    void visit(FunctionCall& n) override { for (auto& a : n.arguments) if (a) a->accept(*this); }
    void visit(MemberAccess& n) override { if (n.structExpr) n.structExpr->accept(*this); }
    void visit(CastExpression& n) override { if (n.expression) n.expression->accept(*this); }
    void visit(CompoundLiteral& n) override {
        // Scalar compound literals don't need frame space — they produce values directly
        bool isScalar = n.arrayDims.empty() && n.pointerLevel == 0 &&
                        n.targetType.substr(0, 7) != "struct " &&
                        n.targetType.substr(0, 6) != "union ";
        if (!isScalar) {
            // Allocate frame space for compound literal temporary
            int size = computeCompoundLiteralSize(n);
            std::string lName = "_cl_" + std::to_string(nextTempId_);
            n.tempId = nextTempId_++;
            locals.push_back({lName, size, currentOffset_});
            currentOffset_ += size;
            if (currentOffset_ > maxFrameSize) maxFrameSize = currentOffset_;
        }
        // Walk initializer elements (they may contain nested compound literals)
        if (n.initializer) for (auto& e : n.initializer->elements) if (e) e->accept(*this);
    }
    void visit(AlignofExpression&) override {}
    void visit(SizeofExpression& n) override { if (!n.isType && n.expression) n.expression->accept(*this); }
    void visit(ReturnStatement& n) override { if (n.expression) n.expression->accept(*this); }
    void visit(BreakStatement&) override {}
    void visit(ContinueStatement&) override {}
    void visit(SwitchContinueStatement& n) override { if (n.target) n.target->accept(*this); }
    void visit(GotoStatement& n) override { if (n.target) n.target->accept(*this); }
    void visit(LabelledStatement& n) override { if (n.statement) n.statement->accept(*this); }
    void visit(ExpressionStatement& n) override { if (n.expression) n.expression->accept(*this); }
    void visit(IfStatement& n) override {
        if (n.condition) n.condition->accept(*this);
        // Then and else are separate scopes (mutually exclusive)
        int savedOffset = currentOffset_;
        if (n.thenBranch) n.thenBranch->accept(*this);
        int thenMax = currentOffset_;
        currentOffset_ = savedOffset;
        if (n.elseBranch) n.elseBranch->accept(*this);
        // Keep the higher water mark
        if (thenMax > currentOffset_) currentOffset_ = thenMax;
    }
    void visit(WhileStatement& n) override { if (n.condition) n.condition->accept(*this); if (n.body) n.body->accept(*this); }
    void visit(DoWhileStatement& n) override { if (n.body) n.body->accept(*this); if (n.condition) n.condition->accept(*this); }
    void visit(ForStatement& n) override {
        if (n.initializer) n.initializer->accept(*this);
        if (n.condition) n.condition->accept(*this);
        if (n.increment) n.increment->accept(*this);
        if (n.body) n.body->accept(*this);
    }
    void visit(RepeatStatement& n) override { if (n.body) n.body->accept(*this); }
    void visit(SwitchStatement& n) override { if (n.expression) n.expression->accept(*this); if (n.body) n.body->accept(*this); }
    void visit(CaseStatement& n) override { if (n.value) n.value->accept(*this); if (n.rangeEnd) n.rangeEnd->accept(*this); }
    void visit(DefaultStatement&) override {}
    void visit(AsmStatement&) override {}
    void visit(StaticAssert&) override {}
    void visit(EnumDefinition&) override {}
    void visit(StructDefinition&) override {}
    void visit(FunctionDeclaration& n) override { if (n.body) n.body->accept(*this); }
    void visit(BuiltinVaStart& n) override { if (n.ap) n.ap->accept(*this); }
    void visit(BuiltinVaArg& n) override { if (n.ap) n.ap->accept(*this); }
    void visit(CpuRegisterAccess&) override {}
    void visit(CpuFlagAccess&) override {}
    void visit(LabelAddressExpression&) override {}
    void visit(TranslationUnit& n) override { for (auto& d : n.topLevelDecls) if (d) d->accept(*this); }

private:
    const std::map<std::string, std::shared_ptr<CodeGenerator::StructInfo>>& structs_;
    int currentOffset_ = 0;
    int scopeBase_ = 0;
    int nextTempId_ = 0;

    int computeCompoundLiteralSize(CompoundLiteral& node) {
        if (node.targetType.length() >= 7 && node.targetType.substr(0, 7) == "struct ") {
            std::string sName = node.targetType.substr(7);
            if (structs_.count(sName)) return structs_.at(sName)->totalSize;
        }
        if (node.targetType.length() >= 6 && node.targetType.substr(0, 6) == "union ") {
            std::string sName = node.targetType.substr(6);
            if (structs_.count(sName)) return structs_.at(sName)->totalSize;
        }
        int elemSize = (node.targetType == "long") ? 4 :
                       (node.pointerLevel > 0 || node.targetType == "int") ? 2 :
                       (node.targetType == "char" || node.targetType == "_Bool") ? 1 : 2;
        if (!node.arrayDims.empty()) {
            int total = 1;
            for (int d : node.arrayDims) total *= d;
            return elemSize * total;
        }
        return elemSize;
    }

    int computeSize(VariableDeclaration& node) {
        int size = 0;
        if (node.pointerLevel > 0) size = 2;
        else if (node.type == "char" || node.type == "_Bool") size = 1;
        else if (node.type == "long") size = 4;
        else if (node.type == "int") size = 2;
        else if (node.type.length() >= 7 && node.type.substr(0, 7) == "struct ") {
            std::string sName = node.type.substr(7);
            if (structs_.count(sName)) size = structs_.at(sName)->totalSize;
            else size = 2; // fallback
        } else if (node.type.length() >= 6 && node.type.substr(0, 6) == "union ") {
            std::string sName = node.type.substr(6);
            if (structs_.count(sName)) size = structs_.at(sName)->totalSize;
            else size = 2;
        } else {
            size = 2; // default (int, enum, etc.)
        }
        if (node.arraySize() >= 0) size *= node.arraySize();
        return size;
    }
};
}

void CodeGenerator::visit(CpuRegisterAccess& node) {
    std::string reg = node.regName;
    std::transform(reg.begin(), reg.end(), reg.begin(), ::toupper);
    if (reg == "A") ;
    else if (reg == "X") { if (resultNeeded) emit("txa"); }
    else if (reg == "Y") { if (resultNeeded) emit("tya"); }
    else if (reg == "Z") { if (resultNeeded) emit("tza"); }
    else if (reg == "AX") ;
    else if (reg == "AY") { if (resultNeeded) { emit("phy"); emit("plx"); } }
    else if (reg == "AZ") { if (resultNeeded) { emit("phz"); emit("plx"); } }
    else if (reg == "XY") { if (resultNeeded) { emit("txa"); emit("phy"); emit("plx"); emit("pla"); } }
    else if (reg == "SP") { if (resultNeeded) { emit("tsy"); emit("phy"); emit("plx"); } }
    else if (reg == "Q" || reg == "AXYZ") ;
    invalidateRegs();
}

void CodeGenerator::visit(CpuFlagAccess& node) {
    std::string flag = node.flagName;
    std::transform(flag.begin(), flag.end(), flag.begin(), ::toupper);
    if (resultNeeded) {
        emit("lda #0");
        if (flag == "CARRY") emit("adc #0");
        else if (flag == "ZERO") { emit("bne *+3"); emit("lda #1"); }
        else if (flag == "NEGATIVE") { emit("bpl *+3"); emit("lda #1"); }
        else if (flag == "OVERFLOW") { emit("bvc *+3"); emit("lda #1"); }
    }
    invalidateRegs();
}

void CodeGenerator::visit(TranslationUnit& node) {
    out << "; Generated by cc45" << std::endl;
    out << ".segmentOrder code, data, bss" << std::endl;
    out << ".code" << std::endl;

    // Collect all known function names (definitions + prototypes) for call validation
    knownFunctions.clear();
    // DMA intrinsics are always available
    knownFunctions.insert("__dma_copy");
    knownFunctions.insert("__dma_fill");
    variadicFunctions.clear();
    structReturningFunctions.clear();
    functionReturnTypes.clear();
    for (auto& decl : node.topLevelDecls) {
        if (auto* fn = dynamic_cast<FunctionDeclaration*>(decl.get())) {
            knownFunctions.insert(fn->name);
            if (fn->isVariadic) variadicFunctions.insert(fn->name);
            if (fn->isFastcall) fastcallFunctions.insert(fn->name);
            functionReturnTypes[fn->name] = {fn->returnType, fn->returnPointerLevel, fn->isSigned};
            if (fn->returnPointerLevel == 0) {
                bool isStructRet = isStruct(fn->returnType);
                bool isLongRet = is32BitType(fn->returnType);
                // In zpCall/fastcall mode, long returns use AXYZ directly — no hidden pointer
                bool fnUsesZpCall = zpCallMode || fn->isFastcall;
                if (isStructRet || (isLongRet && !fnUsesZpCall)) {
                    structReturningFunctions.insert(fn->name);
                }
            }
            std::vector<VarInfo> paramTypes;
            for (auto& p : fn->parameters) {
                paramTypes.push_back({p.type, p.pointerLevel, p.isSigned, p.isVolatile, p.isConst, p.isPointerConst});
            }
            functionParamTypes[fn->name] = std::move(paramTypes);
        }
    }

    // 1. Scan for CRT flags and main function
    bool hasMain = false;
    std::set<std::string> definedFunctions;
    std::set<std::string> weakFunctions;
    bool nextIsWeak = false;
    for (auto& decl : node.topLevelDecls) {
        if (auto* fn = dynamic_cast<FunctionDeclaration*>(decl.get())) {
            if (fn->name == "main") hasMain = true;
            if (!fn->isPrototype) {
                definedFunctions.insert(fn->name);
                if (nextIsWeak) weakFunctions.insert(fn->name);
                if (fn->isStatic) staticFunctions.insert(fn->name);
            }
        } else if (auto* as = dynamic_cast<AsmStatement*>(decl.get())) {
            if (as->code == ".weak_next") { nextIsWeak = true; continue; }
            if (as->code == ".crt_no_0100_stack") crtNoPageOneStack = true;
            if (as->code == ".crt_exit_halt") crtExit = CrtExit::HALT;
            if (as->code == ".crt_exit_rts") crtExit = CrtExit::RTS;
            if (as->code == ".crt_exit_brk") crtExit = CrtExit::BRK;
            if (as->code == ".crt_no_bssinit") crtNoBssInit = true;
            if (as->code == ".crt_heap") crtHeap = true;
            if (as->code == ".crt_stdio") crtStdio = true;
            if (as->code == ".encoding ascii") currentStringEncoding_ = StringEncoding::ASCII;
            if (as->code == ".encoding petscii") currentStringEncoding_ = StringEncoding::PETSCII;
            if (as->code == ".encoding screencode") currentStringEncoding_ = StringEncoding::SCREENCODE;
        }
        nextIsWeak = false;
    }

    if (relocMode) {
        CallCollector cc;
        node.accept(cc);

        // Emit .extern for called-but-not-defined functions
        for (const auto& name : cc.calledFunctions) {
            if (!definedFunctions.count(name)) {
                out << ".extern _" << name << std::endl;
            }
        }

        // Emit .global/.weak for defined functions (skip static)
        for (const auto& name : definedFunctions) {
            if (staticFunctions.count(name)) continue;
            if (weakFunctions.count(name)) {
                out << ".weak _" << name << std::endl;
            } else {
                out << ".global _" << name << std::endl;
            }
        }

        // Emit .extern __sp_base for relocatable code (linker provides from CRT)
        out << ".extern __sp_base" << std::endl;

        // Define __zp_scratch for simulated ops (always at $02 for cc45)
        {
            out << ".global __zp_scratch" << std::endl;
            std::stringstream ss;
            ss << "__zp_scratch = $" << std::hex << std::uppercase
               << std::setfill('0') << std::setw(2) << (int)emitter->scratchZP();
            out << ss.str() << std::endl;
        }

        out << std::endl;
    }
    // 2. Emit startup stub if main is present (not in relocatable mode — CRT provides it)
    if (hasMain && !relocMode) {
        if (!relocMode) out << ".org $2000" << std::endl;
        
        if (crtExit == CrtExit::RTS) {
            out << "    tsx" << std::endl;
            out << "    stx __saved_spl + 1" << std::endl;
            out << "    tsy" << std::endl;
            out << "    sty __saved_sph + 1" << std::endl;
        }
        out << "    jsr __init" << std::endl;
        out << "__exit:" << std::endl;
        switch (crtExit) {
            case CrtExit::RTS:
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
        if (relocMode) out << ".global __init" << std::endl;
        out << "__init:" << std::endl;
        out << "__sp_base = $0101" << std::endl;
        if (relocMode) out << ".global __sp_base" << std::endl;
        {
            std::stringstream ss;
            ss << "__zp_scratch = $" << std::hex << std::uppercase
               << std::setfill('0') << std::setw(2) << (int)emitter->scratchZP();
            out << ss.str() << std::endl;
        }
        if (relocMode) out << ".extern _init_bss" << std::endl;
        out << "    jsr _init_bss" << std::endl;
        if (crtHeap) {
            out << "    .extern _init_heap_crt" << std::endl;
            out << "    jsr _init_heap_crt" << std::endl;
        }
        if (crtStdio) {
            out << "    .extern _init_stdio_crt" << std::endl;
            out << "    jsr _init_stdio_crt" << std::endl;
        }
        out << "    jsr _init_features" << std::endl;
        out << "    jmp _main" << std::endl;
        if (relocMode) out << ".global _init_features" << std::endl;
        out << "_init_features:" << std::endl;

        out << "    rts" << std::endl;

        // Emit _init_bss — only in flat mode. In reloc mode, _init_bss
        // is provided by crt_bssinit.o45 in c45.lib, using linker-defined
        // __bss_start/__bss_end symbols.
        if (!relocMode) {
            bool hasBssVars = false;
            if (!crtNoBssInit) {
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
            out << "    rts" << std::endl << std::endl;
        }
    }

    for (auto& decl : node.topLevelDecls) if (decl) decl->accept(*this);
}

void CodeGenerator::visit(FunctionDeclaration& node) {
    if (node.isPrototype) return; // Forward declaration — no code emitted

    // __naked: emit only proc label + body + endproc, no prologue/epilogue
    if (node.isNaked) {
        out << ".code" << std::endl;
        out << "proc _" + node.name << std::endl;
        currentFunction = &node;
        node.body->accept(*this);
        emit("endproc");
        out << std::endl;
        currentFunction = nullptr;
        return;
    }

    out << ".code" << std::endl;
    variableTypes.clear();
    currentVars.clear();

    currentFunction = &node;
    currentParamByteSize = 0;
    currentLocalByteSize = 0;
    useZpCall_ = (zpCallMode || node.isFastcall) && !node.isVariadic;

    // Initialize per-function clobber tracking
    auto& ci = funcClobbers_[node.name];
    ci = FuncClobberInfo{};  // reset
    currentClobbers_ = &ci;

    // --- Pre-scan: compute frame layout ---
    FrameScanner scanner(structs);
    scanner.scan(*node.body);
    int frameSize = scanner.maxFrameSize;

    // Store frame layout for use by visit(VariableDeclaration)
    frameLocals_.clear();
    for (auto& loc : scanner.locals) {
        frameLocals_[loc.name] = loc.frameOffset;
    }
    frameSize_ = frameSize;

    // Collect param info
    struct ParamInfo { std::string pName; int size; };
    std::vector<ParamInfo> paramInfos;

    // In zpCall mode, long returns use AXYZ directly — no hidden pointer needed.
    // Only actual struct returns need the hidden pointer.
    bool isStructReturn = structReturningFunctions.count(node.name) > 0;
    bool isLongReturn = is32BitType(node.returnType) && node.returnPointerLevel == 0;
    bool needsHiddenPtr = isStructReturn && !isLongReturn;
    // In non-zpCall mode, long returns still use the hidden pointer
    if (!useZpCall_) needsHiddenPtr = isStructReturn;

    if (needsHiddenPtr) {
        std::string rpName = "_p___ret_ptr";
        variableTypes[rpName] = {node.returnType, 1, false, false, false, false, false, {}};
        currentParamByteSize += 2;
        paramInfos.push_back({rpName, 2});
    }

    for (auto& param : node.parameters) {
        std::string pName = "_p_" + param.name;
        variableTypes[pName] = {param.type, param.pointerLevel, param.isSigned, param.isVolatile, param.isConst, param.isPointerConst, false, {}, param.isFunctionPointer, param.funcPtrSig};
        int pSize;
        if (param.pointerLevel > 0) pSize = 2;
        else if (is8BitType(param.type)) pSize = 1;
        else if (is32BitType(param.type)) pSize = 4;
        else pSize = 2;
        currentParamByteSize += pSize;
        paramInfos.push_back({pName, pSize});
    }

    if (useZpCall_) {
        // --- ZP Calling Convention ---
        // Variadic functions use stack convention (va_start needs stack-relative addresses)
        zpParams_.clear();
        zpParamTotalBytes_ = 0;
        zpSpilledParams_.clear();

        // Compute ZP param layout: left-to-right packing from zeroPageStart+1
        uint8_t zpBase = (uint8_t)(zeroPageStart + 8);
        int zpOff = 0;
        for (auto& pi : paramInfos) {
            if (zpOff + pi.size > (int)(zeroPageAvail - 1)) {
                throw std::runtime_error("Too many parameter bytes for ZP calling convention (" +
                    std::to_string(zpOff + pi.size) + " > " + std::to_string(zeroPageAvail - 1) +
                    "); increase zeroPageAvail");
            }
            zpParams_[pi.pName] = {(uint8_t)(zpBase + zpOff), pi.size};
            zpOff += pi.size;
        }
        zpParamTotalBytes_ = zpOff;

        // Detect params whose address is taken — must spill to frame
        std::set<std::string> paramNameSet;
        for (auto& pi : paramInfos) paramNameSet.insert(pi.pName);
        AddressOfParamCollector addrCollector(paramNameSet);
        node.body->accept(addrCollector);

        // Allocate frame slots for address-taken params (after scanner's locals)
        int spillOffset = frameSize; // append after existing locals
        for (auto& pName : addrCollector.addressTakenParams) {
            auto& zpi = zpParams_[pName];
            zpSpilledParams_[pName] = {spillOffset, zpi.size};
            spillOffset += zpi.size;
        }
        int spillSize = spillOffset - frameSize;
        frameSize = spillOffset; // expand frame to include spill slots
        frameSize_ = frameSize;  // update member so caller-save offsets are correct

        // Reserve ZP param slots so allocateZP won't hand them out as scratch
        // zpRegs[0] is __zp_scratch, zpRegs[1..N] map to param slots
        for (int i = 0; i < zpOff; i++) {
            int slot = 1 + i; // slot 0 is scratch, slot 1 is first param byte
            while (slot >= (int)zpRegs.size()) zpRegs.push_back({false});
            zpRegs[slot].inUse = true;
        }

        // proc line — no B#/W#/D# annotations in zpCall mode
        out << "proc _" + node.name << std::endl;

        // __interrupt: save all registers at entry
        if (node.isInterrupt) {
            emit("pha"); emit("phx"); emit("phy"); emit("phz");
        }

        // Frame prologue
        emit(".var _fp = 0");
        currentVars.push_back("_fp");

        // Detect leaf functions — if no calls, no need for caller-save
        CallCollector callChecker;
        node.body->accept(callChecker);
        bool isLeaf = callChecker.calledFunctions.empty();

        // Compute caller-save area: save our ZP params when making calls
        // Leaf functions never need caller-save (no calls = no clobber risk)
        // Only save non-spilled params (spilled ones live in frame already)
        int zpSavableBytes = 0;
        for (auto& kv : zpParams_) {
            if (!zpSpilledParams_.count(kv.first)) zpSavableBytes += kv.second.size;
        }
        zpCallerSaveSize_ = isLeaf ? 0 : zpSavableBytes;
        int totalFrame = frameSize + zpCallerSaveSize_;
        if (totalFrame > 0) {
            emit("; frame: " + std::to_string(frameSize) + " bytes locals" +
                 (spillSize > 0 ? " (incl " + std::to_string(spillSize) + " spill)" : "") +
                 " + " + std::to_string(zpCallerSaveSize_) + " bytes caller-save");
            for (int i = 0; i < totalFrame / 2; ++i) emitter->phw_imm(0);
            if (totalFrame % 2) { emitter->lda_imm(0); emitter->pha(); }
            currentLocalByteSize = totalFrame;
        }

        // Emit .local for pre-scanned locals
        for (auto& loc : scanner.locals) {
            emit(".local " + loc.name + " = " + std::to_string(loc.frameOffset));
        }

        // Emit .local for spilled params and copy ZP → frame at entry
        for (auto& sp : zpSpilledParams_) {
            emit(".local " + sp.first + " = " + std::to_string(sp.second.frameOffset));
            auto& zpi = zpParams_[sp.first];
            emit("; spill " + sp.first + " from " + zpHex(zpi.zpAddr) + " to frame offset " + std::to_string(sp.second.frameOffset));
            for (int b = 0; b < sp.second.size; b++) {
                emit("lda " + zpHex(zpi.zpAddr + b));
                emit("sta.fp " + getLocalOffsetSymbol(sp.second.frameOffset + b));
            }
            // Remove from zpParams_ so all subsequent access uses frame
            zpParams_.erase(sp.first);
        }

        // Emit comment showing ZP param layout
        for (auto& pi : paramInfos) {
            if (zpSpilledParams_.count(pi.pName)) {
                auto& sp = zpSpilledParams_[pi.pName];
                emit("; " + pi.pName + " = frame offset " + std::to_string(sp.frameOffset) + " (spilled, " + std::to_string(sp.size) + " bytes)");
            } else {
                auto& zpi = zpParams_[pi.pName];
                emit("; " + pi.pName + " = " + zpHex(zpi.zpAddr) + " (" + std::to_string(zpi.size) + " bytes)");
            }
        }

        node.body->accept(*this);
        if (!node.isNoreturn) {
            bool lastWasReturn = false;
            if (!node.body->statements.empty()) {
                if (dynamic_cast<ReturnStatement*>(node.body->statements.back().get())) {
                    lastWasReturn = true;
                }
            }
            if (!lastWasReturn) {
                if (totalFrame > 0) {
                    emitter->taz();
                    for (int i = 0; i < totalFrame; ++i) emitter->pla();
                    emitter->tza();
                }
                if (node.isInterrupt) {
                    emit("plz"); emit("ply"); emit("plx"); emit("pla");
                    emit("rti");
                } else {
                    emit("rtn #0");
                }
            }
        }

        // Emit function attribute directives for assembler
        // Use paramInfos + zpBase to compute ZP addresses (zpParams_ may have spilled entries removed)
        if (zpParamTotalBytes_ > 0) {
            int attrOff = 0;
            emit(".zp_uses " + [&]() {
                std::string s;
                int off = 0;
                for (auto& pi : paramInfos) {
                    if (!s.empty()) s += ", ";
                    s += zpHex(zpBase + off);
                    off += pi.size;
                }
                return s;
            }());
            emit(".zp_clobbers " + [&]() {
                std::string s;
                int off = 0;
                for (auto& pi : paramInfos) {
                    for (int b = 0; b < pi.size; b++) {
                        if (!s.empty()) s += ", ";
                        s += zpHex(zpBase + off + b);
                    }
                    off += pi.size;
                }
                return s;
            }());
        }

        // Emit function flags
        emit(isLeaf ? ".func_flags zp_call, leaf" : ".func_flags zp_call");

        // Emit actual reg/flag clobbers from tracking (always, not just when ZP params present)
        {
            auto& ci = funcClobbers_[node.name];
            std::string regs;
            if (ci.regMask & CLOBBER_A) regs += "A, ";
            if (ci.regMask & CLOBBER_X) regs += "X, ";
            if (ci.regMask & CLOBBER_Y) regs += "Y, ";
            if (ci.regMask & CLOBBER_Z) regs += "Z, ";
            if (!regs.empty()) { regs.pop_back(); regs.pop_back(); emit(".reg_clobbers " + regs); }
            std::string fl;
            if (ci.flagMask & CLOBBER_C) fl += "C, ";
            if (ci.flagMask & CLOBBER_N) fl += "N, ";
            if (ci.flagMask & CLOBBER_ZF) fl += "Z, ";
            if (ci.flagMask & CLOBBER_V) fl += "V, ";
            if (!fl.empty()) { fl.pop_back(); fl.pop_back(); emit(".flag_clobbers " + fl); }
        }

        // Finalize clobber info
        funcClobbers_[node.name].complete = true;
        currentClobbers_ = nullptr;

        emit("endproc");
        out << std::endl;

        freeRegisterVars();
        // Free ZP param slots
        for (int i = 0; i < zpParamTotalBytes_; i++) {
            int slot = 1 + i;
            if (slot < (int)zpRegs.size()) zpRegs[slot].inUse = false;
        }
        frameLocals_.clear();
        frameSize_ = 0;
        zpParams_.clear();
        zpSpilledParams_.clear();
        zpParamTotalBytes_ = 0;
        zpCallerSaveSize_ = 0;
        currentFunction = nullptr;
        return;
    }

    // --- Stack-based calling convention (original) ---
    std::string procLine = "proc _" + node.name;

    if (needsHiddenPtr) {
        procLine += ", W#_p___ret_ptr";
    }

    for (auto& pi : paramInfos) {
        if (pi.pName == "_p___ret_ptr") continue; // already added
        VarInfo& vi = variableTypes.at(pi.pName);
        if (vi.pointerLevel > 0) procLine += ", W#" + pi.pName;
        else if (is8BitType(vi.type)) procLine += ", B#" + pi.pName;
        else if (is32BitType(vi.type)) procLine += ", D#" + pi.pName;
        else procLine += ", W#" + pi.pName;
    }
    out << procLine << std::endl;

    // __interrupt: save all registers at entry
    if (node.isInterrupt) {
        emit("pha"); emit("phx"); emit("phy"); emit("phz");
    }

    // --- Frame prologue ---
    emit(".var _fp = 0");
    currentVars.push_back("_fp");

    if (frameSize > 0) {
        emit("; frame: " + std::to_string(frameSize) + " bytes");
        for (int i = 0; i < frameSize / 2; ++i) emitter->phw_imm(0);
        if (frameSize % 2) { emitter->lda_imm(0); emitter->pha(); }
        currentLocalByteSize = frameSize;
    }

    // Set up ZP frame pointer ($FD/$FE) for stack-relative access.
    // FP points to the first frame byte (SP+1 after frame allocation).
    // This eliminates per-access __sp_base relocations.
    if (!useZpCall_) {
        emit("; setup frame pointer");
        emitter->setFramePointerZP(0xFD);
        emitter->setupFramePointer();
    }

    for (auto& loc : scanner.locals) {
        emit(".local " + loc.name + " = " + std::to_string(loc.frameOffset));
    }

    // Params: stack-relative, past frame + return address.
    {
        int pOff = frameSize + 2;
        if (node.isVariadic) {
            for (int i = 0; i < (int)paramInfos.size(); ++i) {
                emit(".var " + paramInfos[i].pName + " = " + std::to_string(pOff));
                currentVars.push_back(paramInfos[i].pName);
                pOff += paramInfos[i].size;
            }
        } else {
            for (int i = (int)paramInfos.size() - 1; i >= 0; --i) {
                emit(".var " + paramInfos[i].pName + " = " + std::to_string(pOff));
                currentVars.push_back(paramInfos[i].pName);
                pOff += paramInfos[i].size;
            }
        }
    }

    node.body->accept(*this);
    if (!node.isNoreturn) {
        bool lastWasReturn = false;
        if (!node.body->statements.empty()) {
            if (dynamic_cast<ReturnStatement*>(node.body->statements.back().get())) {
                lastWasReturn = true;
            }
        }
        if (!lastWasReturn) {
            if (frameSize > 0) {
                emitter->taz();
                for (int i = 0; i < frameSize; ++i) emitter->pla();
                emitter->tza();
            }
            if (node.isInterrupt) {
                emit("plz"); emit("ply"); emit("plx"); emit("pla");
                emit("rti");
            } else {
                emit("rtn #0");
            }
        }
    }
    // Detect leaf functions for stack path (ZP path did this at line 1169)
    {
        CallCollector stackCallChecker;
        node.body->accept(stackCallChecker);
        bool isLeaf = stackCallChecker.calledFunctions.empty();
        emit(isLeaf ? ".func_flags stack_call, leaf" : ".func_flags stack_call");
    }
    // Emit reg/flag clobbers from tracking
    {
        auto& ci = funcClobbers_[node.name];
        std::string regs;
        if (ci.regMask & CLOBBER_A) regs += "A, ";
        if (ci.regMask & CLOBBER_X) regs += "X, ";
        if (ci.regMask & CLOBBER_Y) regs += "Y, ";
        if (ci.regMask & CLOBBER_Z) regs += "Z, ";
        if (!regs.empty()) { regs.pop_back(); regs.pop_back(); emit(".reg_clobbers " + regs); }
        std::string fl;
        if (ci.flagMask & CLOBBER_C) fl += "C, ";
        if (ci.flagMask & CLOBBER_N) fl += "N, ";
        if (ci.flagMask & CLOBBER_ZF) fl += "Z, ";
        if (ci.flagMask & CLOBBER_V) fl += "V, ";
        if (!fl.empty()) { fl.pop_back(); fl.pop_back(); emit(".flag_clobbers " + fl); }
    }

    // Finalize clobber info
    funcClobbers_[node.name].complete = true;
    currentClobbers_ = nullptr;

    emit("endproc");
    out << std::endl;

    freeRegisterVars();
    frameLocals_.clear();
    frameSize_ = 0;
    emitter->setFramePointerZP(0); // disable FP for next function
    currentFunction = nullptr;
}

void CodeGenerator::visit(BuiltinVaStart& node) {
    // va_start(ap, last_param): compute the stack address of the first variadic arg
    // and store it into the ap variable.
    // For variadic functions with right-to-left push, first named param is at offset 2,
    // and variadic args follow at higher offsets.
    // first_va_offset = _p_<last_param> + sizeof(last_param)
    // actual_address = __sp_base + first_va_offset + SPL
    std::string lastPName = "_p_" + node.lastParamName;
    if (!variableTypes.count(lastPName)) {
        throw std::runtime_error("va_start: '" + node.lastParamName + "' is not a parameter of the current function");
    }
    VarInfo& vi = variableTypes.at(lastPName);
    int paramSize = (vi.pointerLevel > 0 || vi.type == "int") ? 2 : 1;
    // For default argument promotions, variadic args are always 2 bytes
    // but the last named param keeps its original size

    // Compute address of first variadic arg on the stack.
    // Use ptrstack which correctly handles __sp_base relocation in .o45 mode.
    // The first variadic arg is at _p_last + paramSize (just past the last named param).
    emit("ptrstack " + lastPName + " + " + std::to_string(paramSize));
    // Now AX = address of first variadic arg
    // Store into ap variable
    auto* apRef = dynamic_cast<VariableReference*>(node.ap.get());
    if (!apRef) {
        throw std::runtime_error("va_start: first argument must be a variable");
    }
    std::string apName = resolveVarName(apRef->name);
    if (registerVars.count(apName)) {
        int zpIdx = registerVars[apName].zpIndex;
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpIdx);
        emit("sta $" + ss.str());
        ss.str(""); ss.clear();
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpIdx + 1);
        emit("stx $" + ss.str());
    } else {
        emit("stax " + apName + ", s");
    }
    invalidateRegs();
}

void CodeGenerator::visit(BuiltinVaArg& node) {
    // va_arg(ap, type): load value from *ap, then advance ap by 2
    // All variadic args are 2 bytes (default argument promotions).
    auto* apRef = dynamic_cast<VariableReference*>(node.ap.get());
    if (!apRef) {
        throw std::runtime_error("va_arg: first argument must be a variable");
    }
    std::string apName = resolveVarName(apRef->name);

    bool is8Bit = (node.pointerLevel == 0 && is8BitType(node.typeName));
    bool is32Bit = (node.pointerLevel == 0 && is32BitType(node.typeName));
    int advanceSize = is32Bit ? 4 : 2; // long args are 4 bytes; default promotions give 2

    // Allocate ZP: pointer indirection pair + result storage
    int resSize = is32Bit ? 4 : 2;
    int zpInd = allocateZP(2);
    int zpRes = allocateZP(resSize);
    auto zpHex = [&](int idx) {
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(idx);
        return "$" + ss.str();
    };
    std::string zpStr = zpHex(zpInd);
    std::string zpStrHi = zpHex(zpInd + 1);
    std::string zpResStr = zpHex(zpRes);
    std::string zpResHi = zpHex(zpRes + 1);

    // Load ap into ZP pair for indirection
    if (registerVars.count(apName)) {
        int zpIdx = registerVars[apName].zpIndex;
        emit("lda " + zpHex(zpIdx));
        emit("sta " + zpStr);
        emit("lda " + zpHex(zpIdx + 1));
        emit("sta " + zpStrHi);
    } else {
        emit("ldax " + apName + ", s");
        emit("stax " + zpStr);
    }

    // Read value from *ap via (ZP),Y and save to zpRes
    emit("ldy #0");
    emit("lda (" + zpStr + "),y");
    emit("sta " + zpResStr);
    if (!is8Bit) {
        emit("iny");
        emit("lda (" + zpStr + "),y");
        emit("sta " + zpResHi);
    }
    if (is32Bit) {
        emit("iny");
        emit("lda (" + zpStr + "),y");
        emit("sta " + zpHex(zpRes + 2));
        emit("iny");
        emit("lda (" + zpStr + "),y");
        emit("sta " + zpHex(zpRes + 3));
    }

    // Compute new ap = old ap + advanceSize
    emit("clc");
    emit("lda " + zpStr);
    emit("adc #" + std::to_string(advanceSize));
    emit("sta " + zpStr);
    emit("lda " + zpStrHi);
    emit("adc #0");
    emit("sta " + zpStrHi);

    // Write back updated ap (no stack displacement — safe)
    if (registerVars.count(apName)) {
        int zpIdx = registerVars[apName].zpIndex;
        emit("lda " + zpStr);
        emit("sta " + zpHex(zpIdx));
        emit("lda " + zpStrHi);
        emit("sta " + zpHex(zpIdx + 1));
    } else {
        emit("lda " + zpStr);
        emit("ldx " + zpStrHi);
        emit("stax " + apName + ", s");
    }

    // Load result into registers
    emit("lda " + zpResStr);
    if (is32Bit) {
        emit("ldx " + zpResHi);
        // LDY/LDZ don't support base-page addressing — use LDA+transfer
        emit("pha");
        emit("lda " + zpHex(zpRes + 3)); emit("taz");
        emit("lda " + zpHex(zpRes + 2)); emit("tay");
        emit("pla");
    } else if (!is8Bit) {
        emit("ldx " + zpResHi);
    }

    freeZP(zpRes, resSize);
    freeZP(zpInd, 2);
    invalidateRegs();
}

static bool isTerminalStatement(Statement* stmt) {
    if (dynamic_cast<ReturnStatement*>(stmt)) return true;
    if (dynamic_cast<BreakStatement*>(stmt)) return true;
    if (dynamic_cast<ContinueStatement*>(stmt)) return true;
    if (dynamic_cast<GotoStatement*>(stmt)) return true;
    if (auto* ws = dynamic_cast<WhileStatement*>(stmt)) {
        if (auto* lit = dynamic_cast<IntegerLiteral*>(ws->condition.get()))
            return lit->value != 0;
    }
    if (auto* fs = dynamic_cast<ForStatement*>(stmt)) {
        return fs->condition == nullptr;
    }
    return false;
}

void CodeGenerator::visit(CompoundStatement& node) {
    bool reachable = true;
    for (auto& stmt : node.statements) {
        // Case and default labels in a switch can always be jumped to, so they reset reachability
        if (dynamic_cast<CaseStatement*>(stmt.get()) || dynamic_cast<DefaultStatement*>(stmt.get())) {
            reachable = true;
        }

        if (!reachable) {
            std::string loc = sourceFilename.empty() ? "" : sourceFilename + ":";
            if (stmt->line > 0)
                loc += std::to_string(stmt->line) + ":" + std::to_string(stmt->column) + ": ";
            std::string msg = loc + "warning: unreachable code";
            warnings.push_back(msg);
            std::cerr << msg << std::endl;
            break;
        }
        stmt->accept(*this);
        if (isTerminalStatement(stmt.get()))
            reachable = false;
    }
}

void CodeGenerator::visit(VariableDeclaration& node) {
    embedSource(node);

    if (node.isGlobal || currentFunction == nullptr) {
        std::string gName = "_" + node.name;
        globalVariableTypes[gName] = {node.type, node.pointerLevel, node.isSigned, node.isVolatile, node.isConst, node.isPointerConst, false, node.arrayDims, node.isFunctionPointer, node.funcPtrSig};
        if (node.isExtern) {
            // extern declaration — type is known but no storage emitted
            return;
        }
        if (node.isGlobal) {
            globalVars.push_back(&node);
            if (weakNext) { weakGlobals.insert(node.name); weakNext = false; }
            if (node.isStatic) staticGlobals.insert(node.name);
            return;
        }
    }

    // Static local: allocate in data/bss segment, not on the stack
    if (node.isStatic && currentFunction != nullptr) {
        std::string sName = "__sl_" + currentFunction->name + "_" + node.name;
        globalVariableTypes[sName] = {node.type, node.pointerLevel, node.isSigned, node.isVolatile, node.isConst, node.isPointerConst, false, node.arrayDims};
        // Create a synthetic global VariableDeclaration for emitData
        auto* synth = new VariableDeclaration(node.type, currentFunction->name + "__" + node.name, node.pointerLevel);
        synth->isSigned = node.isSigned;
        synth->isVolatile = node.isVolatile;
        synth->isConst = node.isConst;
        synth->isPointerConst = node.isPointerConst;
        synth->arrayDims = node.arrayDims;
        synth->isGlobal = true;
        synth->isStatic = true;
        if (node.initializer) {
            if (auto* lit = dynamic_cast<IntegerLiteral*>(node.initializer.get())) {
                synth->initializer = std::make_unique<IntegerLiteral>(lit->value);
            } else if (auto* initList = dynamic_cast<InitializerList*>(node.initializer.get())) {
                auto copy = std::make_unique<InitializerList>();
                for (auto& elem : initList->elements) {
                    if (auto* eLit = dynamic_cast<IntegerLiteral*>(elem.get())) {
                        copy->elements.push_back(std::make_unique<IntegerLiteral>(eLit->value));
                    }
                }
                copy->designators = initList->designators;
                synth->initializer = std::move(copy);
            }
            // Non-literal initializers: leave as nullptr, will be zero-initialized in BSS
        }
        globalVars.push_back(synth);
        staticGlobals.insert(synth->name);
        // Alias the local name to the global static label
        variableTypes.erase("_l_" + node.name);
        std::string gName = "_" + synth->name;
        globalVariableTypes[gName] = {node.type, node.pointerLevel, node.isSigned, node.isVolatile, node.isConst, node.isPointerConst, false, node.arrayDims};
        return;
    }

    std::string lName = "_l_" + node.name;
    variableTypes[lName] = {node.type, node.pointerLevel, node.isSigned, node.isVolatile, node.isConst, node.isPointerConst, false, node.arrayDims, node.isFunctionPointer, node.funcPtrSig};

    // Register variable: allocate in zero page instead of stack
    if (node.isRegister && node.arraySize() < 0 && !isStruct(node.type)) {
        int regSize = (node.pointerLevel > 0 || node.type == "int") ? 2 : 1;
        int zpIdx = allocateZP(regSize);
        if ((int)(zeroPageStart + zpIdx + regSize) <= 0x100) {
            variableTypes[lName].isRegister = true;
            registerVars[lName] = {zpIdx, regSize};
            std::stringstream ss;
            ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpIdx);
            // Emit the ZP address as a .var so source comments are meaningful
            emit("; register " + lName + " = " + ss.str());
            if (node.initializer) {
                if (auto* lit = dynamic_cast<IntegerLiteral*>(node.initializer.get())) {
                    if (regSize == 2) {
                        emitter->lda_imm(lit->value & 0xFF);
                        emit("sta " + ss.str());
                        emitter->lda_imm((lit->value >> 8) & 0xFF);
                        emit("sta " + ss.str() + "+1");
                    } else {
                        uint8_t val = lit->value & 0xFF;
                        if (node.type == "_Bool") val = (val != 0) ? 1 : 0;
                        emitter->lda_imm(val);
                        emit("sta " + ss.str());
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
                    emit("sta " + ss.str());
                    if (regSize == 2) emit("stx " + ss.str() + "+1");
                }
            } else {
                emitter->lda_imm(0);
                emit("sta " + ss.str());
                if (regSize == 2) emit("sta " + ss.str() + "+1");
            }
            invalidateRegs();
            return;
        } else {
            // ZP space exhausted — fall back to normal stack allocation
            freeZP(zpIdx, regSize);
        }
    }

    // Unwrap compound literal initializer: (type){1,2} → {1,2}
    // Struct/array compound literals become InitializerLists; scalar compound literals
    // unwrap to the single element expression directly.
    if (node.initializer) {
        if (auto* cl = dynamic_cast<CompoundLiteral*>(node.initializer.get())) {
            if (cl->arrayDims.empty() && !isStruct(cl->targetType) && cl->pointerLevel == 0) {
                // Scalar: (int){42} → 42
                if (cl->initializer && !cl->initializer->elements.empty()) {
                    node.initializer = std::move(cl->initializer->elements[0]);
                }
            } else {
                // Struct/array: (struct Point){1,2} → {1,2}
                node.initializer = std::move(cl->initializer);
            }
        }
    }

    // Frame-local: already pre-allocated in the frame prologue.
    // Just emit initialization code targeting the frame slot, no stack push needed.
    if (frameLocals_.count(lName)) {
        int frameOff = frameLocals_[lName];
        int size = 0;
        if (node.pointerLevel > 0) size = 2;
        else if (node.type == "char" || node.type == "_Bool") size = 1;
        else if (node.type == "long") size = 4;
        else if (node.type == "int") size = 2;
        else if (isStruct(node.type)) {
            std::string sName = getAggregateName(node.type);
            if (structs.count(sName)) size = structs[sName]->totalSize;
        }
        if (node.arraySize() >= 0) size *= node.arraySize();

        // Narrowing warnings
        if (node.initializer && !dynamic_cast<CastExpression*>(node.initializer.get()) && !dynamic_cast<InitializerList*>(node.initializer.get())) {
            if (auto* lit = dynamic_cast<IntegerLiteral*>(node.initializer.get())) {
                if (size == 1 && (lit->value < 0 || lit->value > 255)) {
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

        // Dead-code elimination for unused integer-initialized locals
        if (node.initializer && dynamic_cast<IntegerLiteral*>(node.initializer.get())) {
            if (!isVariableUsed(node.name, *currentFunction)) {
                return;
            }
        }

        // Initialize the frame slot
        if (auto* initList = dynamic_cast<InitializerList*>(node.initializer.get())) {
            if (isStruct(node.type) && node.pointerLevel == 0) {
                // Struct initializer: {val0, val1, ...} or {.x=val, ...} matched to members by offset order
                std::string sName = getAggregateName(node.type);
                if (!structs.count(sName))
                    throw std::runtime_error("Unknown struct type: " + sName);
                auto& sInfo = *structs[sName];
                std::vector<std::pair<std::string, MemberInfo*>> orderedMembers;
                for (auto& [mname, minfo] : sInfo.members) orderedMembers.push_back({mname, &minfo});
                std::sort(orderedMembers.begin(), orderedMembers.end(),
                          [](auto& a, auto& b) { return a.second->offset < b.second->offset; });
                // Resolve designators to member-ordered positions
                std::vector<Expression*> resolved(orderedMembers.size(), nullptr);
                int nextPos = 0;
                for (size_t ei = 0; ei < initList->elements.size(); ei++) {
                    auto desig = initList->getDesignator(ei);
                    if (!desig.memberName.empty()) {
                        for (int j = 0; j < (int)orderedMembers.size(); j++) {
                            if (orderedMembers[j].first == desig.memberName) { nextPos = j; break; }
                        }
                    }
                    if (nextPos < (int)resolved.size()) {
                        resolved[nextPos] = initList->elements[ei].get();
                        nextPos++;
                    }
                }
                for (int i = 0; i < (int)orderedMembers.size(); i++) {
                    if (!resolved[i]) continue;
                    auto& minfo = *orderedMembers[i].second;
                    int memberOff = frameOff + minfo.offset;
                    int memberSize = (minfo.pointerLevel > 0 || minfo.type == "int") ? 2 :
                                     (minfo.type == "char" || minfo.type == "_Bool") ? 1 : 2;
                    if (auto* lit = dynamic_cast<IntegerLiteral*>(resolved[i])) {
                        if (memberSize == 2) {
                            emitter->lda_imm(lit->value & 0xFF);
                            emit("sta.fp " + getLocalOffsetSymbol(memberOff));
                            emitter->lda_imm((lit->value >> 8) & 0xFF);
                            emit("sta.fp " + getLocalOffsetSymbol(memberOff + 1));
                        } else {
                            emitter->lda_imm(lit->value & 0xFF);
                            emit("sta.fp " + getLocalOffsetSymbol(memberOff));
                        }
                    } else {
                        bool oldNeeded = resultNeeded;
                        resultNeeded = true;
                        resolved[i]->accept(*this);
                        resultNeeded = oldNeeded;
                        if (memberSize == 2) emit("stax.fp " + getLocalOffsetSymbol(memberOff));
                        else emit("sta.fp " + getLocalOffsetSymbol(memberOff));
                    }
                }
            } else {
                // Array/scalar initializer (supports [N]=val designators)
                int elementSize = 0;
                if (node.pointerLevel > 0) elementSize = 2;
                else if (is8BitType(node.type)) elementSize = 1;
                else if (node.type == "int") elementSize = 2;
                int totalElements = node.arraySize() >= 0 ? node.arraySize() : 1;
                // Resolve array designators to position map
                std::vector<Expression*> resolved(totalElements, nullptr);
                int nextIdx = 0;
                for (size_t ei = 0; ei < initList->elements.size(); ei++) {
                    auto desig = initList->getDesignator(ei);
                    if (desig.arrayIndex >= 0) nextIdx = desig.arrayIndex;
                    if (nextIdx < totalElements) {
                        resolved[nextIdx] = initList->elements[ei].get();
                        nextIdx++;
                    }
                }
                for (int i = 0; i < totalElements; i++) {
                    int val = 0;
                    if (resolved[i]) {
                        if (auto* lit = dynamic_cast<IntegerLiteral*>(resolved[i])) {
                            val = lit->value;
                        }
                    }
                    int elemOff = frameOff + i * elementSize;
                    if (elementSize == 2) {
                        emitter->lda_imm(val & 0xFF);
                        emit("sta.fp " + getLocalOffsetSymbol(elemOff));
                        emitter->lda_imm((val >> 8) & 0xFF);
                        emit("sta.fp " + getLocalOffsetSymbol(elemOff + 1));
                    } else {
                        emitter->lda_imm(val & 0xFF);
                        emit("sta.fp " + getLocalOffsetSymbol(elemOff));
                    }
                }
            }
        } else if (node.initializer) {
            if (auto* lit = dynamic_cast<IntegerLiteral*>(node.initializer.get())) {
                if (size == 4) {
                    uint32_t val = (uint32_t)lit->value;
                    emitter->lda_imm(val & 0xFF);
                    emit("sta.fp " + getLocalOffsetSymbol(frameOff));
                    emitter->lda_imm((val >> 8) & 0xFF);
                    emit("sta.fp " + getLocalOffsetSymbol(frameOff + 1));
                    emitter->lda_imm((val >> 16) & 0xFF);
                    emit("sta.fp " + getLocalOffsetSymbol(frameOff + 2));
                    emitter->lda_imm((val >> 24) & 0xFF);
                    emit("sta.fp " + getLocalOffsetSymbol(frameOff + 3));
                } else if (size == 2) {
                    emitter->lda_imm(lit->value & 0xFF);
                    emit("sta.fp " + getLocalOffsetSymbol(frameOff));
                    emitter->lda_imm((lit->value >> 8) & 0xFF);
                    emit("sta.fp " + getLocalOffsetSymbol(frameOff + 1));
                } else {
                    uint8_t val = lit->value & 0xFF;
                    if (node.type == "_Bool") val = (val != 0) ? 1 : 0;
                    emitter->lda_imm(val);
                    emit("sta.fp " + getLocalOffsetSymbol(frameOff));
                }
            } else {
                // Check if this is a struct-returning function call
                bool isStructRetInit = false;
                if (auto* fc = dynamic_cast<FunctionCall*>(node.initializer.get())) {
                    if (structReturningFunctions.count(fc->name)) isStructRetInit = true;
                }

                if (isStructRetInit && (isStruct(node.type) || is32BitType(node.type)) && node.pointerLevel == 0) {
                    // Set the destination frame offset so the FunctionCall visitor
                    // passes our frame slot address as the hidden _ret_ptr.
                    // The callee writes directly to our frame slot — no copy needed.
                    structRetDest_ = frameOff;
                    bool oldNeeded = resultNeeded;
                    resultNeeded = true;
                    node.initializer->accept(*this);
                    resultNeeded = oldNeeded;
                    structRetDest_ = -1;
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
                    if (size == 4) {
                        // Store AXYZ to frame slot byte-by-byte
                        // Note: sta.fp does TSX which clobbers X, so save X first
                        emit("stx __zp_scratch");
                        emit("sta.fp " + getLocalOffsetSymbol(frameOff));
                        emit("lda __zp_scratch"); emit("sta.fp " + getLocalOffsetSymbol(frameOff + 1));
                        emit("tya"); emit("sta.fp " + getLocalOffsetSymbol(frameOff + 2));
                        emit("tza"); emit("sta.fp " + getLocalOffsetSymbol(frameOff + 3));
                    } else if (size == 2) {
                        emit("stax.fp " + getLocalOffsetSymbol(frameOff));
                    } else {
                        emit("sta.fp " + getLocalOffsetSymbol(frameOff));
                    }
                }
            }
        }
        // Frame slot was already zeroed by prologue, no init needed for uninitialized vars
        invalidateRegs();
        return;
    }

    if (node.initializer && !dynamic_cast<CastExpression*>(node.initializer.get()) && !dynamic_cast<InitializerList*>(node.initializer.get())) {
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
        else return; // unknown struct — handled by IR path
    }
    if (node.arraySize() >= 0) size *= node.arraySize();

    if (node.isVolatile) {
        if (auto* initList = dynamic_cast<InitializerList*>(node.initializer.get())) {
            int elementSize = 0;
            if (node.pointerLevel > 0) elementSize = 2;
            else if (is8BitType(node.type)) elementSize = 1;
            else if (node.type == "int") elementSize = 2;
            int totalElements = node.arraySize() >= 0 ? node.arraySize() : 1;
            // Resolve array designators
            std::vector<Expression*> resolved(totalElements, nullptr);
            int nextIdx = 0;
            for (size_t ei = 0; ei < initList->elements.size(); ei++) {
                auto desig = initList->getDesignator(ei);
                if (desig.arrayIndex >= 0) nextIdx = desig.arrayIndex;
                if (nextIdx < totalElements) {
                    resolved[nextIdx] = initList->elements[ei].get();
                    nextIdx++;
                }
            }
            for (int i = totalElements - 1; i >= 0; i--) {
                int val = 0;
                if (resolved[i]) {
                    if (auto* lit = dynamic_cast<IntegerLiteral*>(resolved[i])) {
                        val = lit->value;
                    }
                }
                if (elementSize == 2) {
                    emitter->phw_imm((uint16_t)(int16_t)val);
                } else {
                    emitter->lda_imm(val & 0xFF);
                    emitter->pha();
                }
            }
        } else if (node.initializer) {
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

        if (auto* initList = dynamic_cast<InitializerList*>(node.initializer.get())) {
            // Local array with initializer list — push elements in reverse order
            int elementSize = 0;
            if (node.pointerLevel > 0) elementSize = 2;
            else if (is8BitType(node.type)) elementSize = 1;
            else if (node.type == "int") elementSize = 2;
            int totalElements = node.arraySize() >= 0 ? node.arraySize() : 1;
            // Resolve array designators
            std::vector<Expression*> resolved(totalElements, nullptr);
            int nextIdx = 0;
            for (size_t ei = 0; ei < initList->elements.size(); ei++) {
                auto desig = initList->getDesignator(ei);
                if (desig.arrayIndex >= 0) nextIdx = desig.arrayIndex;
                if (nextIdx < totalElements) {
                    resolved[nextIdx] = initList->elements[ei].get();
                    nextIdx++;
                }
            }
            // Push in reverse: last element first (stack grows down)
            for (int i = totalElements - 1; i >= 0; i--) {
                int val = 0;
                if (resolved[i]) {
                    if (auto* lit = dynamic_cast<IntegerLiteral*>(resolved[i])) {
                        val = lit->value;
                    }
                }
                if (elementSize == 2) {
                    emitter->phw_imm((uint16_t)(int16_t)val);
                } else {
                    emitter->lda_imm(val & 0xFF);
                    emitter->pha();
                }
            }
        } else if (node.initializer) {
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
        // Dereference or member access — IR path handles const checks
    }

    if (node.op == "=") {
        if (auto* cra = dynamic_cast<CpuRegisterAccess*>(node.target.get())) {
            std::string reg = cra->regName;
            std::transform(reg.begin(), reg.end(), reg.begin(), ::toupper);

            // Optimization: Direct load for immediate values
            if (auto* lit = dynamic_cast<IntegerLiteral*>(node.expression.get())) {
                uint32_t val = (uint32_t)lit->value;
                if (reg == "A") emit("lda #$" + hex8((uint8_t)val));
                else if (reg == "X") emit("ldx #$" + hex8((uint8_t)val));
                else if (reg == "Y") emit("ldy #$" + hex8((uint8_t)val));
                else if (reg == "Z") emit("ldz #$" + hex8((uint8_t)val));
                else if (reg == "AX") emit("ldax #$" + hex16((uint16_t)val));
                else if (reg == "AY") emit("lday #$" + hex16((uint16_t)val));
                else if (reg == "AZ") emit("ldaz #$" + hex16((uint16_t)val));
                else if (reg == "XY") emit("ldxy #$" + hex16((uint16_t)val));
                else if (reg == "Q" || reg == "AXYZ") emit("ldq #$" + hex32(val));
                else if (reg == "SP") { emit("ldy #$" + hex8((uint8_t)val)); emit("ldx #$" + hex8((uint8_t)(val >> 8))); emit("tys"); }
                invalidateRegs();
                return;
            }

            node.expression->accept(*this);
            if (reg == "A") ;
            else if (reg == "X") emit("tax");
            else if (reg == "Y") emit("tay");
            else if (reg == "Z") emit("taz");
            else if (reg == "AX") ;
            else if (reg == "AY") { emit("phx"); emit("ply"); }
            else if (reg == "AZ") { emit("phx"); emit("plz"); }
            else if (reg == "XY") { emit("tax"); emit("phx"); emit("ply"); emit("plx"); }
            else if (reg == "SP") { emit("phx"); emit("ply"); emit("tys"); }
            invalidateRegs();
            return;
        }
        if (auto* cfa = dynamic_cast<CpuFlagAccess*>(node.target.get())) {
            std::string flag = cfa->flagName;
            std::transform(flag.begin(), flag.end(), flag.begin(), ::toupper);

            // Optimization: Direct SEC/CLC for immediate values
            if (auto* lit = dynamic_cast<IntegerLiteral*>(node.expression.get())) {
                int val = lit->value;
                if (flag == "CARRY") { emit(val ? "sec" : "clc"); invalidateRegs(); return; }
                else if (flag == "DECIMAL") { emit(val ? "sed" : "cld"); invalidateRegs(); return; }
                else if (flag == "INTERRUPT") { emit(val ? "sei" : "cli"); invalidateRegs(); return; }
                else if (flag == "OVERFLOW" && val == 0) { emit("clv"); invalidateRegs(); return; }
            }

            node.expression->accept(*this);
            if (flag == "CARRY") { emit("lsr a"); }
            else if (flag == "ZERO") { emit("cmp #0"); }
            invalidateRegs();
            return;
        }
    }

    if (node.op == "=" && !dynamic_cast<CastExpression*>(node.expression.get())) {
        if (auto* lit = dynamic_cast<IntegerLiteral*>(node.expression.get())) {
            ExpressionType dstType = getExprType(node.target.get());
            int dstSize = is32BitType(dstType.type) ? 4 : (dstType.pointerLevel > 0 || dstType.type == "int") ? 2 : 1;
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
        // ZP calling convention: compound assignment on ZP params
        if (auto* ref = dynamic_cast<VariableReference*>(node.target.get())) {
            std::string rName = resolveVarName(ref->name);
            if (isZpParam(rName)) {
                auto& zpi = zpParams_[rName];
                std::string zpAddr = zpHex(zpi.zpAddr);
                ExpressionType targetType = getExprType(node.target.get());
                bool is16 = (targetType.pointerLevel > 0 || targetType.type == "int");
                std::string actualOp = node.op.substr(0, node.op.length() - 1);

                // Evaluate RHS
                bool oldNeeded = resultNeeded;
                resultNeeded = true;
                node.expression->accept(*this);
                resultNeeded = oldNeeded;
                emitter->push_ax(); // save RHS on stack

                // Load current value from ZP
                if (is16) {
                    emit("ldax " + zpAddr);
                } else {
                    emit("lda " + zpAddr);
                    emitter->ldx_imm(0);
                }
                int zpLeft = allocateZP(2);
                std::stringstream ssLeft;
                ssLeft << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpLeft);
                emit("stax $" + ssLeft.str());

                emitter->pop_ax();
                emitOperation(actualOp, zpLeft, targetType, getExprType(node.expression.get()));

                // Store result back to ZP
                emit("sta " + zpAddr);
                if (is16) emit("stx " + zpAddr + "+1");

                freeZP(zpLeft, 2);
                invalidateRegs();
                return;
            }
        }

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
        ASTNode* refNode = ref;  // Use ref for line info (more accurate than Assignment node)

        // ZP calling convention: spilled params (address-taken) live in frame
        if (isZpSpilledParam(rName)) {
            VarInfo vi = variableTypes.at(rName);
            auto& sp = zpSpilledParams_[rName];
            std::string off = std::to_string(sp.frameOffset);
            bool is32 = is32BitType(vi.type) && vi.pointerLevel == 0;
            bool is16 = !is32 && (vi.pointerLevel > 0 || vi.type == "int");

            // Literal assignment
            if (auto* lit = dynamic_cast<IntegerLiteral*>(node.expression.get())) {
                uint32_t uval = (uint32_t)lit->value;
                if (is32) {
                    emitter->lda_imm(uval & 0xFF); emit("sta.fp " + off);
                    emitter->lda_imm((uval >> 8) & 0xFF); emit("sta.fp " + getLocalOffsetSymbol(sp.frameOffset + 1));
                    emitter->lda_imm((uval >> 16) & 0xFF); emit("sta.fp " + getLocalOffsetSymbol(sp.frameOffset + 2));
                    emitter->lda_imm((uval >> 24) & 0xFF); emit("sta.fp " + getLocalOffsetSymbol(sp.frameOffset + 3));
                } else if (is16) {
                    emitter->lda_imm(uval & 0xFF); emit("sta.fp " + off);
                    emitter->lda_imm((uval >> 8) & 0xFF); emit("sta.fp " + getLocalOffsetSymbol(sp.frameOffset + 1));
                } else {
                    uint8_t val = uval & 0xFF;
                    if (vi.type == "_Bool") val = (val != 0) ? 1 : 0;
                    emitter->lda_imm(val); emit("sta.fp " + off);
                }
                invalidateRegs();
                return;
            }

            // General expression assignment
            bool oldNeeded = resultNeeded;
            resultNeeded = true;
            node.expression->accept(*this);
            resultNeeded = oldNeeded;

            if (vi.type == "_Bool" && vi.pointerLevel == 0) {
                ExpressionType srcType = getExprType(node.expression.get());
                int srcSize = (srcType.pointerLevel > 0 || srcType.type == "int") ? 2 : 1;
                emitBoolNormalize(srcSize);
            }

            if (is32) {
                // AXYZ → frame: stax.fp stores A,X; then Y,Z via save/restore
                emit("stax.fp " + off);
                emit("pha"); emit("tya"); emit("sta.fp " + getLocalOffsetSymbol(sp.frameOffset + 2));
                emit("tza"); emit("sta.fp " + getLocalOffsetSymbol(sp.frameOffset + 3)); emit("pla");
            } else if (is16) {
                emit("stax.fp " + off);
            } else {
                emit("sta.fp " + off);
            }
            invalidateRegs();
            return;
        }

        // ZP calling convention: params live in zero page
        if (isZpParam(rName)) {
            VarInfo vi = variableTypes.at(rName);
            auto& zpi = zpParams_[rName];
            std::string zpAddr = zpHex(zpi.zpAddr);
            bool is32 = is32BitType(vi.type) && vi.pointerLevel == 0;
            bool is16 = !is32 && (vi.pointerLevel > 0 || vi.type == "int");

            // Optimization: x = x;
            if (auto* rhsRef = dynamic_cast<VariableReference*>(node.expression.get())) {
                if (resolveVarName(rhsRef->name) == rName && !vi.isVolatile) {
                    if (resultNeeded) node.target->accept(*this);
                    return;
                }
            }

            // inc/dec optimization for x = x + 1 / x = x - 1
            if (!is32) {
                if (auto* bin = dynamic_cast<BinaryOperation*>(node.expression.get())) {
                    if (bin->op == "+" || bin->op == "-") {
                        if (auto* leftRef = dynamic_cast<VariableReference*>(bin->left.get())) {
                            if (auto* rightLit = dynamic_cast<IntegerLiteral*>(bin->right.get())) {
                                if (resolveVarName(leftRef->name) == rName && rightLit->value == 1) {
                                    if (bin->op == "+") {
                                        if (is16) emit("inw " + zpAddr);
                                        else emit("inc " + zpAddr);
                                    } else {
                                        if (is16) emit("dew " + zpAddr);
                                        else emit("dec " + zpAddr);
                                    }
                                    invalidateRegs();
                                    if (resultNeeded) ref->accept(*this);
                                    return;
                                }
                            }
                        }
                    }
                }
            }

            // Literal assignment
            if (auto* lit = dynamic_cast<IntegerLiteral*>(node.expression.get())) {
                uint32_t uval = (uint32_t)lit->value;
                if (is32) {
                    emitter->lda_imm(uval & 0xFF); emit("sta " + zpAddr);
                    emitter->lda_imm((uval >> 8) & 0xFF); emit("sta " + zpAddr + "+1");
                    emitter->lda_imm((uval >> 16) & 0xFF); emit("sta " + zpAddr + "+2");
                    emitter->lda_imm((uval >> 24) & 0xFF); emit("sta " + zpAddr + "+3");
                } else if (is16) {
                    emitter->lda_imm(uval & 0xFF); emit("sta " + zpAddr);
                    emitter->lda_imm((uval >> 8) & 0xFF); emit("sta " + zpAddr + "+1");
                } else {
                    uint8_t val = uval & 0xFF;
                    if (vi.type == "_Bool") val = (val != 0) ? 1 : 0;
                    emitter->lda_imm(val); emit("sta " + zpAddr);
                }
                invalidateRegs();
                return;
            }

            // General expression assignment
            bool oldNeeded = resultNeeded;
            resultNeeded = true;
            node.expression->accept(*this);
            resultNeeded = oldNeeded;

            if (vi.type == "_Bool" && vi.pointerLevel == 0) {
                ExpressionType srcType = getExprType(node.expression.get());
                int srcSize = (srcType.pointerLevel > 0 || srcType.type == "int") ? 2 : 1;
                emitBoolNormalize(srcSize);
            }

            if (is32) {
                emit("sta " + zpAddr); emit("stx " + zpAddr + "+1");
                emit("pha"); emit("tya"); emit("sta " + zpAddr + "+2");
                emit("tza"); emit("sta " + zpAddr + "+3"); emit("pla");
            } else {
                emit("sta " + zpAddr);
                if (is16) emit("stx " + zpAddr + "+1");
            }
            invalidateRegs();
            return;
        }

        // Register variable: use direct ZP addressing
        if (registerVars.count(rName)) {
            VarInfo vi = variableTypes.at(rName);
            auto& rv = registerVars[rName];
            std::stringstream ssZP;
            ssZP << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(rv.zpIndex);
            std::string zpAddr = ssZP.str();
            bool is16 = (vi.pointerLevel > 0 || vi.type == "int");

            // Optimization: x = x;
            if (auto* rhsRef = dynamic_cast<VariableReference*>(node.expression.get())) {
                if (resolveVarName(rhsRef->name) == rName && !vi.isVolatile) {
                    if (resultNeeded) node.target->accept(*this);
                    return;
                }
            }

            // inc/dec optimization for x = x + 1 / x = x - 1
            if (auto* bin = dynamic_cast<BinaryOperation*>(node.expression.get())) {
                if (bin->op == "+" || bin->op == "-") {
                    if (auto* leftRef = dynamic_cast<VariableReference*>(bin->left.get())) {
                        if (auto* rightLit = dynamic_cast<IntegerLiteral*>(bin->right.get())) {
                            if (resolveVarName(leftRef->name) == rName && rightLit->value == 1) {
                                // inw/dew work for base-page addresses
                                if (bin->op == "+") {
                                    if (is16) emit("inw " + zpAddr);
                                    else emit("inc " + zpAddr);
                                } else {
                                    if (is16) emit("dew " + zpAddr);
                                    else emit("dec " + zpAddr);
                                }
                                invalidateRegs();
                                if (resultNeeded) ref->accept(*this);
                                return;
                            }
                        }
                    }
                }
            }

            // Literal assignment
            if (auto* lit = dynamic_cast<IntegerLiteral*>(node.expression.get())) {
                if (is16) {
                    emitter->lda_imm(lit->value & 0xFF);
                    emit("sta " + zpAddr);
                    emitter->lda_imm((lit->value >> 8) & 0xFF);
                    emit("sta " + zpAddr + "+1");
                } else {
                    uint8_t val = lit->value & 0xFF;
                    if (vi.type == "_Bool") val = (val != 0) ? 1 : 0;
                    emitter->lda_imm(val);
                    emit("sta " + zpAddr);
                }
                invalidateRegs();
                return;
            }

            // General expression assignment
            bool oldNeeded = resultNeeded;
            resultNeeded = true;
            node.expression->accept(*this);
            resultNeeded = oldNeeded;

            if (vi.type == "_Bool" && vi.pointerLevel == 0) {
                ExpressionType srcType = getExprType(node.expression.get());
                int srcSize = (srcType.pointerLevel > 0 || srcType.type == "int") ? 2 : 1;
                emitBoolNormalize(srcSize);
            }

            emit("sta " + zpAddr);
            if (is16) emit("stx " + zpAddr + "+1");
            invalidateRegs();
            return;
        }

        bool isGlobal = globalVariableTypes.count(rName);
        std::string suffix = isGlobal ? "" : ", s";

        // Optimization: x = x;
        if (auto* rhsRef = dynamic_cast<VariableReference*>(node.expression.get())) {
            if (resolveVarName(rhsRef->name) == rName) {
                VarInfo vi = lookupVar(rName, refNode);
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
                            VarInfo vi = lookupVar(rName, refNode);
                            bool is16 = (vi.pointerLevel > 0 || vi.type == "int");
                            if (isStruct(vi.type)) {
                                std::string sName = getAggregateName(vi.type);
                                if (structs.count(sName) && structs[sName]->totalSize > 1) is16 = true;
                            }
                            if (!isGlobal) {
                                // inw/dew work for base-page and stack-relative
                                if (bin->op == "+") {
                                    if (is16) emit("inw " + rName + suffix);
                                    else emit("inc " + rName + suffix);
                                } else {
                                    if (is16) emit("dew " + rName + suffix);
                                    else emit("dec " + rName + suffix);
                                }
                            } else if (!is16) {
                                // 8-bit: inc/dec support absolute addressing
                                if (bin->op == "+") emit("inc " + rName);
                                else emit("dec " + rName);
                            } else {
                                // 16-bit global: inw/dew only support base-page; use inc/dec absolute
                                if (bin->op == "+") {
                                    emit("inc " + rName);
                                    emit("bne *+5");
                                    emit("inc " + rName + "+1");
                                } else {
                                    emit("lda " + rName);
                                    emit("bne *+5");
                                    emit("dec " + rName + "+1");
                                    emit("dec " + rName);
                                }
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
            VarInfo vi = lookupVar(rName, refNode);
            bool is32 = is32BitType(vi.type) && vi.pointerLevel == 0;
            bool is16 = !is32 && (vi.pointerLevel > 0 || vi.type == "int");
            if (!is32 && isStruct(vi.type)) {
                std::string sName = getAggregateName(vi.type);
                if (structs.count(sName) && structs[sName]->totalSize > 1) is16 = true;
            }
            if (is32) {
                // Store 32-bit literal
                uint32_t val = (uint32_t)lit->value;
                if (isGlobal) {
                    std::stringstream ssLo, ssHi;
                    ssLo << "#$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (val & 0xFFFF);
                    ssHi << "#$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << ((val >> 16) & 0xFFFF);
                    emit("stw " + ssLo.str() + ", " + rName);
                    emit("stw " + ssHi.str() + ", " + rName + "+2");
                } else {
                    std::stringstream ssLo, ssHi;
                    ssLo << "#$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (val & 0xFFFF);
                    ssHi << "#$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << ((val >> 16) & 0xFFFF);
                    emit("stw.sp " + ssLo.str() + ", " + rName);
                    emit("stw.sp " + ssHi.str() + ", " + rName + "+2");
                }
                invalidateRegs();
            } else if (is16) {
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

        VarInfo vi = lookupVar(rName, refNode);
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

        bool is32 = is32BitType(vi.type) && vi.pointerLevel == 0;
        bool is16 = !is32 && (vi.pointerLevel > 0 || vi.type == "int");
        if (!is32 && isStruct(vi.type)) {
            std::string sName = getAggregateName(vi.type);
            if (structs.count(sName) && structs[sName]->totalSize > 1) is16 = true;
        }
        if (is32) {
            // Store AXYZ to 32-bit variable
            if (isGlobal) {
                emit("stq " + rName);
            } else {
                emit("sta.sp " + rName);
                emit("pha"); emit("txa"); emit("sta.sp " + rName + "+1"); emit("pla");
                emit("pha"); emit("tya"); emit("sta.sp " + rName + "+2"); emit("pla");
                emit("pha"); emit("tza"); emit("sta.sp " + rName + "+3"); emit("pla");
            }
            invalidateRegs();
        } else if (is16) {
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
            if (vi.isVolatile || !isGlobal) invalidateRegs();
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
            if (vi.isVolatile || !isGlobal) invalidateRegs();
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
                        std::string memberAddr = rName + (mInfo.offset ? "+" + std::to_string(mInfo.offset) : "");
                        if (mInfo.bitWidth > 0) {
                            // Bitfield insert: A has new value, do RMW on storage unit
                            std::string bfOp = is16 ? "bfins16" : "bfins";
                            if (!isGlobal) bfOp += ".sp";
                            emit(bfOp + " " + memberAddr + ", #" + std::to_string(mInfo.bitOffset) + ", #" + std::to_string(mInfo.bitWidth));
                        } else if (is16) {
                            emit("stax " + memberAddr + suffix);
                            updateRegAVar(rName, mInfo.offset); updateRegXVar(rName, mInfo.offset + 1);
                        } else {
                            if (isGlobal) emit("sta " + memberAddr);
                            else emit("sta.sp " + memberAddr);
                            updateRegAVar(rName, mInfo.offset);
                        }
                        invalidateRegs();
                        return;
                    }
                }
            }
        }
    }

    // Check if target is a bitfield member (for arrow/indirect access)
    int bfWidth = 0, bfOffset = 0;
    bool bfIs16 = false;
    if (auto* ma = dynamic_cast<MemberAccess*>(node.target.get())) {
        ExpressionType bType = getExprType(ma->structExpr.get());
        if (!isStruct(bType.type)) {
            if (auto* ref = dynamic_cast<VariableReference*>(ma->structExpr.get())) {
                if (globalVariableTypes.count("_" + ref->name))
                    bType = {globalVariableTypes.at("_" + ref->name).type, globalVariableTypes.at("_" + ref->name).pointerLevel};
            }
        }
        if (isStruct(bType.type)) {
            std::string sn = getAggregateName(bType.type);
            if (structs.count(sn) && structs[sn]->members.count(ma->memberName)) {
                auto& mi = structs[sn]->members[ma->memberName];
                if (mi.bitWidth > 0) {
                    bfWidth = mi.bitWidth;
                    bfOffset = mi.bitOffset;
                    bfIs16 = (mi.pointerLevel > 0 || mi.type == "int");
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

    if (bfWidth > 0) {
        // Bitfield insert via indirect ZP pointer
        std::stringstream ssZ;
        ssZ << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpIdx);
        std::string bfOp = bfIs16 ? "bfins16.ind" : "bfins.ind";
        emit(bfOp + " " + ssZ.str() + ", #" + std::to_string(bfOffset) + ", #" + std::to_string(bfWidth));
    } else {
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
    bool anyIs32 = (is32BitType(lhsType.type) && lhsType.pointerLevel == 0) ||
                   (is32BitType(rhsType.type) && rhsType.pointerLevel == 0);
    bool isLiteralOne = false;
    if (scale == 1 && !anyIs32) {
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
        regA.known = false; regX.known = false;
        clobberReg(CLOBBER_A | CLOBBER_X);
        invalidateFlags();
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

    bool is32BitOp_early = (is32BitType(lhsType.type) && lhsType.pointerLevel == 0) ||
                           (is32BitType(rhsType.type) && rhsType.pointerLevel == 0);
    bool isMultiplicativeLiteral = false;
    if (!is32BitOp_early && dynamic_cast<IntegerLiteral*>(node.right.get())) {
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
                bool is32 = is32BitType(lhsType.type) && lhsType.pointerLevel == 0;
                for (int i = 0; i < shifts; i++) {
                    emit(is32 ? "aslq" : "lsl.16 .ax");
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
            bool is32 = is32BitType(lhsType.type) && lhsType.pointerLevel == 0;
            for (int i = 0; i < val; i++) {
                if (is32) {
                    if (node.op == "<<") emit("aslq");
                    else emit(isSigned ? "asr.32 .AXYZ" : "lsrq");
                } else {
                    if (node.op == "<<") emit("lsl.16 .ax");
                    else emit(isSigned ? "asr.16 .ax" : "lsr.16 .ax");
                }
            }
            invalidateFlags(); resultNeeded = oldNeeded; return;
        }
    }

    // Determine if this is a 32-bit operation
    bool is32BitOp = (is32BitType(lhsType.type) && lhsType.pointerLevel == 0) ||
                     (is32BitType(rhsType.type) && rhsType.pointerLevel == 0);

    if (is32BitOp) {
        // --- 32-bit code path using native Q register operations ---
        // Widen left operand to 32-bit if needed
        if (!is32BitType(lhsType.type) || lhsType.pointerLevel > 0) {
            if (lhsType.isSigned) emit("sxt.16"); // sign-extend AX → AXYZ
            else { emit("ldy #$00"); emit("ldz #$00"); } // zero-extend
        }

        // Check if right side has a call (zpCall mode ZP clobber protection)
        bool rightHasCall32 = false;
        if (useZpCall_) {
            CallCollector cc;
            node.right->accept(cc);
            rightHasCall32 = !cc.calledFunctions.empty();
        }

        int zpIdx = allocateZP(4);
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpIdx);

        if (rightHasCall32 && (node.op == "+" || node.op == "&" || node.op == "|" || node.op == "^")) {
            // Commutative 32-bit op with call on right: save left to stack,
            // evaluate right, store right to ZP, restore left to AXYZ, then op.
            emit("phz"); emit("phy"); emit("phx"); emit("pha");
            for (const auto& varName : currentVars) {
                emit(".var " + varName + " = " + varName + " + 4");
            }
            node.right->accept(*this);
            resultNeeded = oldNeeded;
            if (!is32BitType(rhsType.type) || rhsType.pointerLevel > 0) {
                if (rhsType.isSigned) emit("sxt.16");
                else { emit("ldy #$00"); emit("ldz #$00"); }
            }
            // AXYZ = right. Store to ZP (call done, ZP stable).
            emit("stq $" + ss.str());
            // Pop left from stack
            emit("pla"); emit("plx"); emit("ply"); emit("plz");
            for (const auto& varName : currentVars) {
                emit(".var " + varName + " = " + varName + " - 4");
            }
            // AXYZ = left, zpIdx = right. Commutative op:
            if (node.op == "+") { emit("clc"); emit("adcq $" + ss.str()); }
            else if (node.op == "&") emit("andq $" + ss.str());
            else if (node.op == "|") emit("oraq $" + ss.str());
            else if (node.op == "^") emit("eorq $" + ss.str());
        } else {
            emit("stq $" + ss.str());  // save left operand to ZP
            node.right->accept(*this);
            resultNeeded = oldNeeded;
            // Widen right operand to 32-bit if needed
            if (!is32BitType(rhsType.type) || rhsType.pointerLevel > 0) {
                if (rhsType.isSigned) emit("sxt.16");
                else { emit("ldy #$00"); emit("ldz #$00"); }
            }

            if (node.op == "+" || node.op == "&" || node.op == "|" || node.op == "^") {
                // Commutative ops: right is in AXYZ, left in ZP — use native Q ops
                if (node.op == "+") { emit("clc"); emit("adcq $" + ss.str()); }
                else if (node.op == "&") emit("andq $" + ss.str());
                else if (node.op == "|") emit("oraq $" + ss.str());
                else if (node.op == "^") emit("eorq $" + ss.str());
        } else {
            // Non-commutative: need left in AXYZ, right in ZP
            int zpRight = allocateZP(4);
            std::stringstream ssRight; ssRight << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpRight);
            emit("stq $" + ssRight.str());  // save right
            emit("ldq $" + ss.str());       // reload left into AXYZ

            bool isSigned = (lhsType.isSigned || rhsType.isSigned) && lhsType.pointerLevel == 0;
            if (node.op == "-") {
                emit("sec"); emit("sbcq $" + ssRight.str());
            } else if (node.op == "*") {
                emit((isSigned ? "mul.s16" : "mul.32") + std::string(" .AXYZ, $") + ssRight.str());
            } else if (node.op == "/" || node.op == "%") {
                if (node.op == "/") emit("div.32 .AXYZ, $" + ssRight.str());
                else emit("mod.32 .AXYZ, $" + ssRight.str());
            } else if (node.op == "<<") {
                std::string labelStart = newDontCareLabel(); std::string labelEnd = newDontCareLabel();
                int shiftZp = allocateZP(1); std::stringstream ssSh; ssSh << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(shiftZp);
                emit("lda $" + ssRight.str()); emit("sta $" + ssSh.str());
                emit("ldq $" + ss.str()); // reload left
                out << labelStart << ":" << std::endl;
                emit("lda $" + ssSh.str()); emit("beq " + labelEnd); emit("dec $" + ssSh.str());
                emit("ldq $" + ss.str()); emit("aslq"); emit("stq $" + ss.str());
                emit("bra " + labelStart); out << labelEnd << ":" << std::endl;
                emit("ldq $" + ss.str());
                freeZP(shiftZp, 1);
            } else if (node.op == ">>") {
                bool isSigned = lhsType.isSigned && lhsType.pointerLevel == 0;
                std::string labelStart = newDontCareLabel(); std::string labelEnd = newDontCareLabel();
                int shiftZp = allocateZP(1); std::stringstream ssSh; ssSh << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(shiftZp);
                emit("lda $" + ssRight.str()); emit("sta $" + ssSh.str());
                emit("ldq $" + ss.str()); // reload left
                out << labelStart << ":" << std::endl;
                emit("lda $" + ssSh.str()); emit("beq " + labelEnd); emit("dec $" + ssSh.str());
                emit("ldq $" + ss.str());
                if (isSigned) emit("asr.32 .AXYZ"); else emit("lsrq");
                emit("stq $" + ss.str());
                emit("bra " + labelStart); out << labelEnd << ":" << std::endl;
                emit("ldq $" + ss.str());
                freeZP(shiftZp, 1);
            } else {
                // Relational operators
                bool eitherSigned = lhsType.isSigned || rhsType.isSigned;
                if (eitherSigned) emit("cmp.s32 .AXYZ, $" + ssRight.str());
                else emit("cmpq $" + ssRight.str());

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
            freeZP(zpRight, 4);
        }
        invalidateFlags();
        freeZP(zpIdx, 4);
        }
    } else {

    // --- 16-bit code path (original) ---
    // In zpCall mode, check if the right side contains a function call.
    // If so, the left operand must be saved to the stack (not ZP) because
    // the callee will clobber the same ZP slots during recursion/nested calls.
    bool rightHasCall = false;
    if (useZpCall_) {
        CallCollector cc;
        node.right->accept(cc);
        rightHasCall = !cc.calledFunctions.empty();
    }

    int zpIdx = allocateZP(2);
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpIdx);

    if (rightHasCall) {
        // In zpCall mode, the right side contains a function call that will
        // clobber ZP temps. Save left to stack (safe across calls), evaluate
        // right, then restore left to ZP (call is done, ZP is stable now).
        emitter->push_ax();  // left → stack (safe across call)
        for (const auto& varName : currentVars) {
            emit(".var " + varName + " = " + varName + " + 2");
        }
        node.right->accept(*this);
        resultNeeded = oldNeeded;
        // AX = right result. Call is done — ZP is now stable.
        // Save right to zpIdx temporarily, pop left, then swap.
        std::string zpLo = zpHex(emitter->getZP(zpIdx));
        std::string zpHi = zpHex(emitter->getZP(zpIdx) + 1);
        emit("stax " + zpLo);          // zpIdx = right (temporarily)
        emitter->pop_ax();              // AX = left (from stack)
        for (const auto& varName : currentVars) {
            emit(".var " + varName + " = " + varName + " - 2");
        }
        // Now AX = left, zpIdx = right. Swap: zpIdx ← left, AX ← right.
        emit("ldy " + zpLo);           // Y = right_lo
        emit("sta " + zpLo);           // zpIdx_lo = left_lo
        emit("lda " + zpHi);           // A = right_hi
        emit("stx " + zpHi);           // zpIdx_hi = left_hi
        emit("tax");                    // X = right_hi
        emit("tya");                    // A = right_lo
    } else {
        emit("stax $" + ss.str());
        node.right->accept(*this);
        resultNeeded = oldNeeded;
    }
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
    } // end 16-bit else
    invalidateRegs();
}

void CodeGenerator::visit(ConditionalExpression& node) {
    embedSource(node);
    std::string labelElse = newLabel();
    std::string labelEnd = newLabel();
    emitJumpIfFalse(node.condition.get(), labelElse);
    if (node.thenExpr) {
        node.thenExpr->accept(*this);
    } else {
        // Elvis operator: re-evaluate condition as then-value
        node.condition->accept(*this);
    }
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

void CodeGenerator::visit(InitializerList& node) {
    // No-op in legacy validator: compound literals and nested initializers
    // are handled correctly by the IR pipeline.
    (void)node;
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
        if (auto* ref = dynamic_cast<VariableReference*>(node.operand.get())) {
            std::string rName = resolveVarName(ref->name);
            if (registerVars.count(rName))
                throw std::runtime_error("Compile Error: Cannot take address of register variable '" + ref->name + "'");
        }
        if (auto* ma = dynamic_cast<MemberAccess*>(node.operand.get())) {
            ExpressionType bType = getExprType(ma->structExpr.get());
            if (isStruct(bType.type)) {
                std::string sn = getAggregateName(bType.type);
                if (structs.count(sn) && structs[sn]->members.count(ma->memberName)) {
                    if (structs[sn]->members[ma->memberName].bitWidth > 0)
                        throw std::runtime_error("Cannot take address of bitfield member '" + ma->memberName + "'");
                }
            }
        }
        emitAddress(node.operand.get());
    } else if (node.op == "++" || node.op == "--" || node.op == "++_POST" || node.op == "--_POST") {
        bool isInc = (node.op.substr(0, 2) == "++");
        bool isPost = (node.op.find("_POST") != std::string::npos);
        if (auto* ref = dynamic_cast<VariableReference*>(node.operand.get())) {
            std::string rName = resolveVarName(ref->name);
            VarInfo vi = lookupVar(rName, &node);
            if ((vi.isConst && vi.pointerLevel == 0) || vi.isPointerConst) throw std::runtime_error("Compile Error: Increment/decrement of read-only variable '" + ref->name + "'");
            bool is32Bit = is32BitType(vi.type) && vi.pointerLevel == 0;
            bool is16Bit = !is32Bit && (vi.pointerLevel > 0 || vi.type == "int");
            if (!is32Bit && isStruct(vi.type)) {
                std::string sName = getAggregateName(vi.type);
                if (structs.count(sName) && structs[sName]->totalSize > 1) is16Bit = true;
            }
            // Spilled params: use indirect inc/dec through frame address
            if (isZpSpilledParam(rName)) {
                emitIndirectIncDec(node, isInc, isPost);
                return;
            }
            if (isPost && resultNeeded) ref->accept(*this);
            invalidateRegs();
            bool isGlobal = globalVariableTypes.count(rName);
            std::string suffix = isGlobal ? "" : ", s";
            if (is32Bit && isGlobal) {
                // Native 32-bit inc/dec on absolute address
                if (isInc) emit("incq " + rName);
                else emit("decq " + rName);
            } else if (is32Bit) {
                // Stack-relative 32-bit inc/dec via ZP temp + native incq/decq
                int zpTmp = allocateZP(4);
                std::stringstream ss;
                ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpTmp);
                std::string zpAddr = ss.str();
                // Load 4 bytes from stack to ZP temp
                for (int i = 0; i < 4; i++) {
                    emit("lda.sp " + rName + (i ? "+" + std::to_string(i) : ""));
                    emit("sta " + zpAddr + (i ? "+" + std::to_string(i) : ""));
                }
                // Native 32-bit inc/dec on ZP
                if (isInc) emit("incq " + zpAddr);
                else emit("decq " + zpAddr);
                // Store back to stack
                for (int i = 0; i < 4; i++) {
                    emit("lda " + zpAddr + (i ? "+" + std::to_string(i) : ""));
                    emit("sta.sp " + rName + (i ? "+" + std::to_string(i) : ""));
                }
                freeZP(zpTmp, 4);
            } else if (is16Bit && !isGlobal) {
                if (isInc) emit("inw " + rName + suffix);
                else emit("dew " + rName + suffix);
            } else if (is16Bit) {
                if (isInc) {
                    emit("inc " + rName);
                    emit("bne *+5");
                    emit("inc " + rName + "+1");
                } else {
                    emit("lda " + rName);
                    emit("bne *+5");
                    emit("dec " + rName + "+1");
                    emit("dec " + rName);
                }
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
                            if (mInfo.bitWidth > 0) {
                                // Bitfield ++/--: read, extract, inc/dec, insert
                                bool isGlobal = globalVariableTypes.count(rName);
                                std::string suffix = isGlobal ? "" : ", s";
                                std::string memberAddr = rName + (mInfo.offset ? "+" + std::to_string(mInfo.offset) : "");
                                // Load storage unit
                                if (is16Bit) emit("ldax " + memberAddr + suffix);
                                else { if (isGlobal) emit("lda " + memberAddr); else emit("lda.sp " + memberAddr); }
                                // Extract bitfield
                                std::string bfExtOp = is16Bit ? "bfext16" : "bfext";
                                emit(bfExtOp + " #" + std::to_string(mInfo.bitOffset) + ", #" + std::to_string(mInfo.bitWidth));
                                if (isPost && resultNeeded) { emitter->push("a"); }
                                // Inc/dec
                                if (isInc) { emitter->clc(); emitter->adc_imm(1); }
                                else { emitter->sec(); emitter->sbc_imm(1); }
                                // Insert back
                                std::string bfInsOp = is16Bit ? "bfins16" : "bfins";
                                if (!isGlobal) bfInsOp += ".sp";
                                emit(bfInsOp + " " + memberAddr + ", #" + std::to_string(mInfo.bitOffset) + ", #" + std::to_string(mInfo.bitWidth));
                                invalidateRegs();
                                if (isPost && resultNeeded) { emitter->pop("a"); emitter->ldx_imm(0); }
                                else if (!isPost && resultNeeded) node.operand->accept(*this);
                            } else {
                            if (isPost && resultNeeded) node.operand->accept(*this);
                            invalidateRegs();
                            bool isGlobal = globalVariableTypes.count(rName);
                            std::string suffix = isGlobal ? "" : ", s";
                            std::string memberAddr = rName + (mInfo.offset ? "+" + std::to_string(mInfo.offset) : "");
                            if (is16Bit && !isGlobal) {
                                if (isInc) emit("inw " + memberAddr + suffix);
                                else emit("dew " + memberAddr + suffix);
                            } else if (is16Bit) {
                                // inw/dew only support base-page; use inc/dec absolute
                                if (isInc) {
                                    emit("inc " + memberAddr);
                                    emit("bne *+5");
                                    emit("inc " + memberAddr + "+1");
                                } else {
                                    emit("lda " + memberAddr);
                                    emit("bne *+5");
                                    emit("dec " + memberAddr + "+1");
                                    emit("dec " + memberAddr);
                                }
                            } else {
                                if (isInc) emit("inc " + memberAddr + suffix);
                                else emit("dec " + memberAddr + suffix);
                            }
                            if (!isPost && resultNeeded) node.operand->accept(*this);
                            }
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
        } else if (node.op == "~") {
            ExpressionType subType = getExprType(node.operand.get());
            if (is32BitType(subType.type) && subType.pointerLevel == 0) emitter->not_32();
            else emitter->not_16();
        } else if (node.op == "-") {
            ExpressionType subType = getExprType(node.operand.get());
            if (is32BitType(subType.type) && subType.pointerLevel == 0) emitter->neg_32();
            else emitter->neg_16();
        }
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
    if (node.code == ".crt_heap") {
        crtHeap = true;
        return;
    }
    if (node.code == ".crt_stdio") {
        crtStdio = true;
        return;
    }
    if (node.code == ".encoding ascii") {
        currentStringEncoding_ = StringEncoding::ASCII;
        return;
    }
    if (node.code == ".encoding petscii") {
        currentStringEncoding_ = StringEncoding::PETSCII;
        return;
    }
    if (node.code == ".encoding screencode") {
        currentStringEncoding_ = StringEncoding::SCREENCODE;
        return;
    }
    embedSource(node);
    emit(node.code);
    invalidateRegs();
}

void CodeGenerator::visit(StaticAssert& node) {}

void CodeGenerator::visit(ReturnStatement& node) {
    embedSource(node);

    // Check if this is a struct-returning function
    bool isStructReturn = currentFunction && structReturningFunctions.count(currentFunction->name) > 0;

    if (isStructReturn && node.expression) {
        bool isLongReturn = is32BitType(currentFunction->returnType) && currentFunction->returnPointerLevel == 0;

        if (isLongReturn && useZpCall_) {
            // zpCall: long return in AXYZ directly — no hidden pointer needed
            bool oldNeeded = resultNeeded;
            resultNeeded = true;
            node.expression->accept(*this);
            resultNeeded = oldNeeded;
            // Result is already in AXYZ from the expression evaluation
        } else if (isLongReturn) {
            // Stack convention: long return via hidden pointer
            bool oldNeeded = resultNeeded;
            resultNeeded = true;
            node.expression->accept(*this);
            resultNeeded = oldNeeded;

            // Save AXYZ to ZP byte-by-byte (avoiding stq for ZP compatibility)
            int zpVal = allocateZP(4);
            int zpDest = allocateZP(2);
            std::stringstream ssVal, ssDest;
            ssVal << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpVal);
            ssDest << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpDest);
            emit("sta " + ssVal.str());
            emit("stx " + ssVal.str() + "+1");
            emit("pha"); emit("tya"); emit("sta " + ssVal.str() + "+2");
            emit("tza"); emit("sta " + ssVal.str() + "+3"); emit("pla");
            // Load destination pointer
            emit("ldax _p___ret_ptr, s");
            emit("stax " + ssDest.str());
            // Store 4 bytes through pointer
            for (int i = 0; i < 4; i++) {
                emitter->ldy_imm(i);
                emit("lda " + ssVal.str() + "+" + std::to_string(i));
                emit("sta (" + ssDest.str() + "),y");
            }
            freeZP(zpDest, 2);
            freeZP(zpVal, 4);
        } else {
            // Struct return: copy via source address
            std::string sName = getAggregateName(currentFunction->returnType);
            int structSize = structs.count(sName) ? structs[sName]->totalSize : 2;

            // Get source address (the expression result — should be a struct lvalue)
            emitAddress(node.expression.get());
            // AX now holds the source address; save to ZP
            int zpSrc = allocateZP(2);
            std::stringstream ssSrc;
            ssSrc << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpSrc);
            emit("stax $" + ssSrc.str());

            // Load destination address from _ret_ptr parameter
            if (isZpParam("_p___ret_ptr")) {
                auto& zpi = zpParams_["_p___ret_ptr"];
                emit("ldax " + zpHex(zpi.zpAddr));
            } else {
                emit("ldax _p___ret_ptr, s");
            }
            int zpDest = allocateZP(2);
            std::stringstream ssDest;
            ssDest << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpDest);
            emit("stax $" + ssDest.str());

            // Byte-copy loop: copy structSize bytes from (zpSrc) to (zpDest)
            if (structSize <= 8) {
                for (int i = 0; i < structSize; i++) {
                    emitter->ldy_imm(i);
                    emit("lda ($" + ssSrc.str() + "),y");
                    emit("sta ($" + ssDest.str() + "),y");
                }
            } else {
                std::stringstream ssLen;
                ssLen << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (structSize & 0xFF);
                emit("ldx #$" + ssLen.str());
                emit("ldy #$00");
                emit("MOVE ($" + ssSrc.str() + "), ($" + ssDest.str() + ")");
            }

            freeZP(zpDest, 2);
            freeZP(zpSrc, 2);
        }
    } else if (node.expression) {
        bool oldNeeded = resultNeeded;
        resultNeeded = true;
        node.expression->accept(*this);
        resultNeeded = oldNeeded;
    }

    // Clean up the pre-allocated frame from stack before returning.
    // Use frameSize_ (not currentLocalByteSize, which accumulates transient arg pushes).
    // For ZP calling convention, also include the caller-save area.
    // AX holds the return value, so use Z register to preserve A.
    int cleanupSize = frameSize_ + zpCallerSaveSize_;
    if (cleanupSize > 0) {
        emitter->taz();
        for (int i = 0; i < cleanupSize; ++i) emitter->pla();
        emitter->tza();
    }
    if (currentFunction && currentFunction->isInterrupt) {
        emit("plz"); emit("ply"); emit("plx"); emit("pla");
        emit("rti");
    } else {
        emit("rtn #0");
    }
    auto* saved = currentClobbers_;
    currentClobbers_ = nullptr;  // don't record post-return invalidation
    invalidateRegs();
    currentClobbers_ = saved;
}

void CodeGenerator::visit(GotoStatement& node) {
    embedSource(node);
    if (node.target) {
        // Computed goto: goto *expr — emit indirect jump
        node.target->accept(*this);
        emit("sta $0008");
        emit("stx $0009");
        emit("jmp ($0008)");
    } else {
        emit("bra " + node.label);
    }
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
        size = getTypeSizeWithStructs(node.typeName, node.pointerLevel, -1);
        if (node.pointerLevel == 0) {
            for (int d : node.arrayDims) {
                if (d > 0) size *= d;
            }
        }
    } else {
        ExpressionType et = getExprType(node.expression.get());
        // For sizeof on a variable, use the raw type (without array decay)
        // so struct arrays compute element_size * count, not pointer_size * count
        int arrSize = -1;
        int ptrLevel = et.pointerLevel;
        if (auto* ref = dynamic_cast<VariableReference*>(node.expression.get())) {
            std::string rName = resolveVarName(ref->name);
            VarInfo* vi = nullptr;
            if (variableTypes.count(rName)) vi = &variableTypes.at(rName);
            else if (globalVariableTypes.count("_" + ref->name)) vi = &globalVariableTypes.at("_" + ref->name);
            if (vi) {
                arrSize = vi->arraySize();
                ptrLevel = vi->pointerLevel; // raw, without arrayDims adjustment
            }
        }
        size = getTypeSizeWithStructs(et.type, ptrLevel, arrSize);
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

void CodeGenerator::visit(RepeatStatement& node) {
    // Validator: declare the loop variable if present, then validate body
    if (!node.varName.empty()) {
        std::string lName = "_l_" + node.varName;
        variableTypes[lName] = {node.varType, 0, node.varSigned, false, false, false, {}};
    }
    for (int i = 0; i < node.count; i++) {
        node.body->accept(*this);
    }
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
    void visit(FloatLiteral&) override {}
        void visit(StringLiteral&) override {}
        void visit(VariableReference&) override {}
        void visit(Assignment& node) override { node.expression->accept(*this); }
        void visit(BinaryOperation& node) override { node.left->accept(*this); node.right->accept(*this); }
        void visit(UnaryOperation& node) override { node.operand->accept(*this); }
        void visit(FunctionCall& node) override { for (auto& arg : node.arguments) arg->accept(*this); }
        void visit(BuiltinVaStart& node) override { node.ap->accept(*this); }
        void visit(BuiltinVaArg& node) override { node.ap->accept(*this); }
        void visit(CpuRegisterAccess&) override {}
        void visit(CpuFlagAccess&) override {}
        void visit(MemberAccess& node) override { node.structExpr->accept(*this); }
        void visit(ConditionalExpression& node) override { node.condition->accept(*this); if (node.thenExpr) node.thenExpr->accept(*this); node.elseExpr->accept(*this); }
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
        void visit(InitializerList& node) override { for (auto& elem : node.elements) elem->accept(*this); }
        void visit(ArrayAccess& node) override { node.arrayExpr->accept(*this); node.indexExpr->accept(*this); }
        void visit(CastExpression& node) override { node.expression->accept(*this); }
        void visit(CompoundLiteral& node) override { for (auto& e : node.initializer->elements) e->accept(*this); }
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
        void visit(RepeatStatement& node) override { node.body->accept(*this); }
        void visit(SwitchStatement&) override {}
        void visit(CaseStatement& node) override {
            uint32_t val = 0; if (auto* lit = dynamic_cast<IntegerLiteral*>(node.value.get())) val = lit->value;
            uint32_t endVal = val;
            bool isRange = false;
            if (node.rangeEnd) {
                if (auto* lit2 = dynamic_cast<IntegerLiteral*>(node.rangeEnd.get())) endVal = lit2->value;
                isRange = true;
            }
            std::string l = gen.newLabel(); info.cases.push_back({val, endVal, isRange, l}); node.label = l;
        }
        void visit(DefaultStatement& node) override {
            std::string l = gen.newLabel(); info.defaultLabel = l; info.hasDefault = true; node.label = l;
        }
        void visit(AsmStatement&) override {}
        void visit(StaticAssert&) override {}
        void visit(EnumDefinition&) override {}
        void visit(StructDefinition&) override {}
        void visit(CompoundStatement& node) override { for(auto& s : node.statements) if (s) s->accept(*this); }
        void visit(FunctionDeclaration& node) override { if (node.body) node.body->accept(*this); }
        void visit(LabelAddressExpression&) override {}
        void visit(TranslationUnit& node) override { for (auto& d : node.topLevelDecls) if (d) d->accept(*this); }
    };
    CaseCollector collector(info, *this);
    node.body->accept(collector);
    for (auto& c : info.cases) {
        emit("ldax $" + ss.str());
        if (c.isRange) {
            // Range case: check value >= min && value <= max
            std::string skipLabel = newLabel();
            std::stringstream ssMin;
            ssMin << "#$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << c.value;
            emit("cmp.16 .ax, " + ssMin.str());
            emit("bcc " + skipLabel);  // value < min → skip
            std::stringstream ssMax;
            ssMax << "#$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (c.rangeEndValue + 1);
            emit("cmp.16 .ax, " + ssMax.str());
            emit("bcc " + c.label);    // value < max+1 (i.e., value <= max) �� match
            out << skipLabel << ":" << std::endl;
        } else {
            std::stringstream ssVal;
            ssVal << "#$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << c.value;
            emit("cmp.16 .ax, " + ssVal.str()); emit("beq " + c.label);
        }
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

    // Bitfield packing state
    std::string bfUnitType;   // type of current bitfield storage unit ("" = none)
    int bfUnitBits = 0;       // bits used in current unit
    int bfUnitOffset = 0;     // byte offset of current unit
    int bfUnitSize = 0;       // byte size of current unit (1 or 2)

    auto closeBitfieldUnit = [&]() {
        if (!bfUnitType.empty()) {
            if (!node.isUnion) currentOffset = bfUnitOffset + bfUnitSize;
            bfUnitType.clear();
            bfUnitBits = 0;
        }
    };

    // Validate flexible array members (C99): must be last, not in union, struct needs >=1 other member
    for (size_t i = 0; i < node.members.size(); i++) {
        auto& m = node.members[i];
        bool isFAM = (!m.arrayDims.empty() && m.arrayDims[0] == 0 && m.arrayDims.size() == 1);
        if (isFAM) {
            if (node.isUnion)
                throw std::runtime_error("Flexible array member '" + m.name + "' not allowed in union");
            if (i != node.members.size() - 1)
                throw std::runtime_error("Flexible array member '" + m.name + "' must be the last member of struct '" + node.name + "'");
            // Relaxed: allow flexible array as sole member (GCC extension)
            // if (node.members.size() < 2) — warning only
        }
    }

    for (auto& member : node.members) {
        int mAlign = 1;
        int mSize = 0;

        if (member.pointerLevel > 0 || member.type == "int") {
            mSize = 2;
            mAlign = node.isUnpacked ? 2 : 1;  // packed by default
        } else if (is8BitType(member.type)) {
            mAlign = 1;
            mSize = 1;
        } else if (isStruct(member.type)) {
            std::string sName = getAggregateName(member.type);
            if (structs.count(sName)) {
                mAlign = node.isUnpacked ? structs[sName]->alignment : 1;
                mSize = structs[sName]->totalSize;
            } else {
                return; // unknown nested struct — handled by IR path
            }
        }

        // Evaluate _Alignas on struct member if present (always honored)
        int memberAlignOverride = resolveAlignmentExpr(member.alignmentExpr.get(), structs);
        if (memberAlignOverride > mAlign) mAlign = memberAlignOverride;
        if (member.alignment > mAlign) mAlign = member.alignment;
        if (mAlign > maxAlign) maxAlign = mAlign;

        if (member.arraySize() >= 0) mSize *= member.arraySize();

        // Bitfield member
        if (member.bitWidth > 0) {
            int unitMaxBits = mSize * 8;
            // Start new unit if: different type, or doesn't fit in current unit
            if (bfUnitType.empty() || member.type != bfUnitType ||
                bfUnitBits + member.bitWidth > unitMaxBits) {
                closeBitfieldUnit();
                // Align for new unit
                if (!node.isUnion) {
                    if (currentOffset % mAlign != 0) currentOffset += mAlign - (currentOffset % mAlign);
                }
                bfUnitType = member.type;
                bfUnitBits = 0;
                bfUnitOffset = node.isUnion ? 0 : currentOffset;
                bfUnitSize = mSize;
            }

            MemberInfo mInfo;
            mInfo.type = member.type;
            mInfo.pointerLevel = member.pointerLevel;
            mInfo.isSigned = member.isSigned;
            mInfo.isConst = member.isConst;
            mInfo.offset = bfUnitOffset;
            mInfo.alignment = mAlign;
            mInfo.bitWidth = member.bitWidth;
            mInfo.bitOffset = bfUnitBits;
            if (!member.name.empty()) info->members[member.name] = mInfo;
            bfUnitBits += member.bitWidth;

            if (node.isUnion) {
                if (mSize > maxSize) maxSize = mSize;
            }
            continue;
        }

        // Non-bitfield member: close any open bitfield unit
        closeBitfieldUnit();

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
    closeBitfieldUnit();
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
    if (!isStruct(baseType.type)) return; // non-struct dot/arrow — handled by IR path
    std::string sName = getAggregateName(baseType.type);
    if (!structs.count(sName)) return; // unknown struct — handled by IR path
    auto& sInfo = *structs[sName];
    if (!sInfo.members.count(node.memberName)) return; // method call or unknown member — handled by IR path
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
            std::string memberAddr = rName + (mInfo.offset ? "+" + std::to_string(mInfo.offset) : "");
            if (is16) {
                emit("ldax " + memberAddr + suffix);
                updateRegAVar(rName, mInfo.offset); updateRegXVar(rName, mInfo.offset + 1);
                updateZNFlags(FlagSource::A);
            } else {
                if (isGlobal) emit("lda " + memberAddr);
                else emit("lda.sp " + memberAddr);
                updateRegAVar(rName, mInfo.offset);
                emitter->ldx_imm(0); updateRegX(0);
            }
            // Bitfield extract
            if (mInfo.bitWidth > 0) {
                std::string bfOp = is16 ? "bfext16" : "bfext";
                emit(bfOp + " #" + std::to_string(mInfo.bitOffset) + ", #" + std::to_string(mInfo.bitWidth));
                invalidateRegs();
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
    // Bitfield extract for indirect/arrow access
    if (mInfo.bitWidth > 0) {
        std::string bfOp = is16 ? "bfext16" : "bfext";
        emit(bfOp + " #" + std::to_string(mInfo.bitOffset) + ", #" + std::to_string(mInfo.bitWidth));
    }
    freeZP(zpIdx, 2);
    invalidateRegs();
}

void CodeGenerator::visit(IntegerLiteral& node) {
    if (!resultNeeded) return;
    uint32_t uval = (uint32_t)node.value;
    bool needs32 = uval > 0xFFFF || (node.value < 0 && node.value < -32768)
                   || is32BitType(node.castType);
    if (node.value == 0) {
        emit("zero a, x"); updateRegA(0); updateRegX(0); updateZNFlags(FlagSource::A);
        if (needs32) { emitter->ldy_imm(0); emitter->ldz_imm(0); }
        return;
    }
    if (needs32) {
        // Load all 4 bytes into AXYZ
        std::stringstream ss;
        ss << "#$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (uval & 0xFFFF);
        emit("ldax " + ss.str());
        emitter->ldy_imm((uval >> 16) & 0xFF);
        emitter->ldz_imm((uval >> 24) & 0xFF);
    } else {
        std::stringstream ss;
        ss << "#$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (uval & 0xFFFF);
        emit("ldax " + ss.str());
    }
    updateRegA(node.value & 0xFF); updateRegX((node.value >> 8) & 0xFF); updateZNFlags(FlagSource::A);
}

void CodeGenerator::visit(FloatLiteral&) {
    // Float codegen handled by IRBuilder/IRCodeGen path
}

void CodeGenerator::visit(StringLiteral& node) {
    if (!resultNeeded) return;
    std::string poolKey;
    if (node.isAscii) {
        poolKey = std::string("\x01") + node.value;  // explicit @"..." always ASCII
    } else if (currentStringEncoding_ == StringEncoding::SCREENCODE) {
        poolKey = std::string("\x02") + node.value;
    } else if (currentStringEncoding_ == StringEncoding::ASCII) {
        poolKey = std::string("\x01") + node.value;
    } else {
        poolKey = node.value;  // PETSCII default
    }
    if (stringPool.find(poolKey) == stringPool.end()) {
        stringPool[poolKey] = "STR" + std::to_string(stringCount++);
        if (!poolKey.empty() && poolKey[0] == '\x01') asciiStrings.insert(poolKey);
        if (!poolKey.empty() && poolKey[0] == '\x02') screencodeStrings.insert(poolKey);
    }
    std::string label = stringPool[poolKey];
    emit("ldax #" + label); invalidateRegs();
}

void CodeGenerator::visit(VariableReference& node) {
    if (!resultNeeded) return;
    // Check if this is a function name used as a value (function pointer)
    if (knownFunctions.count(node.name) && !variableTypes.count("_l_" + node.name) &&
        !variableTypes.count("_p_" + node.name) && !globalVariableTypes.count("_" + node.name)) {
        emit("lda #<_" + node.name);
        emit("ldx #>_" + node.name);
        invalidateRegs();
        return;
    }
    std::string rName = resolveVarName(node.name);

    // ZP calling convention: spilled params (address-taken) live in frame
    if (isZpSpilledParam(rName)) {
        VarInfo vi = variableTypes.at(rName);
        auto& sp = zpSpilledParams_[rName];
        bool is32 = is32BitType(vi.type) && vi.pointerLevel == 0;
        bool is16 = !is32 && (vi.pointerLevel > 0 || vi.type == "int");
        if (is32) {
            emit("lda.fp " + getLocalOffsetSymbol(sp.frameOffset + 3)); emit("taz");
            emit("lda.fp " + getLocalOffsetSymbol(sp.frameOffset + 2)); emit("tay");
            emit("lda.fp " + getLocalOffsetSymbol(sp.frameOffset + 1)); emit("tax");
            emit("lda.fp " + getLocalOffsetSymbol(sp.frameOffset));
            invalidateRegs();
        } else if (is16) {
            emit("ldax.fp " + getLocalOffsetSymbol(sp.frameOffset));
            invalidateRegs();
        } else {
            emit("lda.fp " + getLocalOffsetSymbol(sp.frameOffset));
            invalidateRegs();
            if (!regX.known || regX.isVariable || regX.value != 0) { emitter->ldx_imm(0); updateRegX(0); }
        }
        return;
    }

    // ZP calling convention: params live in zero page
    if (isZpParam(rName)) {
        VarInfo vi = variableTypes.at(rName);
        auto& zpi = zpParams_[rName];
        std::string zpAddr = zpHex(zpi.zpAddr);
        bool is32 = is32BitType(vi.type) && vi.pointerLevel == 0;
        bool is16 = !is32 && (vi.pointerLevel > 0 || vi.type == "int");
        if (is32) {
            // Load 32-bit from ZP into AXYZ
            emit("lda " + zpAddr + "+3"); emit("taz");
            emit("lda " + zpAddr + "+2"); emit("tay");
            emit("ldx " + zpAddr + "+1");
            emit("lda " + zpAddr);
            invalidateRegs();
        } else if (is16) {
            emit("ldax " + zpAddr);
            updateRegAVar(rName, 0); updateRegXVar(rName, 1); updateZNFlags(FlagSource::A);
        } else {
            emit("lda " + zpAddr); updateRegAVar(rName, 0);
            if (!regX.known || regX.isVariable || regX.value != 0) { emitter->ldx_imm(0); updateRegX(0); }
        }
        return;
    }

    // Register variable: use direct ZP addressing
    if (registerVars.count(rName)) {
        VarInfo vi = variableTypes.at(rName);
        auto& rv = registerVars[rName];
        std::stringstream ss;
        ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(rv.zpIndex);
        std::string zpAddr = ss.str();
        bool is16 = (vi.pointerLevel > 0 || vi.type == "int");

        bool lowCorrect = (regA.known && regA.isVariable && regA.varName == rName && regA.varOffset == 0);
        bool highCorrect = (regX.known && regX.isVariable && regX.varName == rName && regX.varOffset == 1);
        if (vi.isVolatile) { lowCorrect = false; highCorrect = false; }

        if (is16) {
            if (lowCorrect && highCorrect) {}
            else if (lowCorrect) { emit("ldx " + zpAddr + "+1"); updateRegXVar(rName, 1); }
            else if (highCorrect) { emit("lda " + zpAddr); updateRegAVar(rName, 0); }
            else { emit("ldax " + zpAddr); updateRegAVar(rName, 0); updateRegXVar(rName, 1); updateZNFlags(FlagSource::A); }
        } else {
            if (lowCorrect) {}
            else { emit("lda " + zpAddr); updateRegAVar(rName, 0); }
            if (!regX.known || regX.isVariable || regX.value != 0) { emitter->ldx_imm(0); updateRegX(0); }
        }
        return;
    }

    bool isGlobal = globalVariableTypes.count(rName);
    std::string suffix = isGlobal ? "" : ", s";
    VarInfo vi = lookupVar(rName, &node);

    if (vi.arraySize() >= 0) {
        emitAddress(&node); // This will put address in AX
        return;
    }

    bool is32Bit = is32BitType(vi.type) && vi.pointerLevel == 0;
    bool is16Bit = !is32Bit && (vi.pointerLevel > 0 || vi.type == "int");
    if (!is32Bit && isStruct(vi.type)) {
        std::string sName = getAggregateName(vi.type);
        if (structs.count(sName) && structs[sName]->totalSize > 1) is16Bit = true;
    }
    if (is32Bit) {
        // Load 32-bit value into AXYZ via native LDQ
        if (isGlobal) {
            emit("ldq " + rName);
        } else {
            // Stack-relative: byte-by-byte load into AXYZ
            // Load hi bytes via lda.sp+offset (each lda.sp does its own TSX).
            // Load byte 1→X last (just before final lda.sp for byte 0)
            // because lda.sp clobbers X via TSX.
            emit("lda.sp " + rName + "+3"); emit("taz");
            emit("lda.sp " + rName + "+2"); emit("tay");
            emit("lda.sp " + rName + "+1"); emit("sta __zp_scratch");
            emit("lda.sp " + rName);
            emit("ldx __zp_scratch");
        }
        invalidateRegs();
    } else if (is16Bit) {
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
    // Determine if this is an indirect call (function pointer)
    bool isIndirect = false;
    if (node.callExpr) {
        isIndirect = true;
    } else if (!node.name.empty() && !knownFunctions.count(node.name)) {
        // Name might be a function pointer variable
        std::string rName = resolveVarName(node.name);
        if (variableTypes.count(rName) || globalVariableTypes.count(rName)) {
            VarInfo vi = lookupVar(rName, &node);
            if (vi.isFunctionPointer || vi.pointerLevel > 0) {
                isIndirect = true;
            }
        }
    }

    if (isIndirect) {
        bool oldNeeded = resultNeeded; resultNeeded = true;
        invalidateRegs(); // Ensure fresh load of function pointer value
        // Evaluate the function pointer expression
        if (node.callExpr) {
            // For (*fp)(args) — the callExpr is a UnaryOperation("*", VariableReference("fp"))
            // We need the address value, not the dereferenced content.
            // If it's *expr, we want the value of expr (the pointer itself).
            if (auto* un = dynamic_cast<UnaryOperation*>(node.callExpr.get())) {
                if (un->op == "*") {
                    un->operand->accept(*this);
                } else {
                    node.callExpr->accept(*this);
                }
            } else {
                node.callExpr->accept(*this);
            }
        } else {
            // fp(args) — load the variable value
            auto ref = VariableReference(node.name);
            ref.line = node.line;
            ref.column = node.column;
            ref.accept(*this);
        }
        // Store function address in ZP
        int zpCall = allocateZP(2);
        std::stringstream ssCall;
        ssCall << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)emitter->getZP(zpCall);
        emit("stax $" + ssCall.str());
        // Push arguments (same logic as direct call to handle stack-relative offsets)
        int pushedBytesI = 0;
        for (auto& arg : node.arguments) {
            int pushSize = 2;
            if (auto* ref = dynamic_cast<VariableReference*>(arg.get())) {
                std::string rName = resolveVarName(ref->name);
                if (!variableTypes.count(rName) && !globalVariableTypes.count(rName)) {
                    arg->accept(*this); emitter->push_ax();
                } else {
                    VarInfo vi = lookupVar(rName, &node);
                    bool is16Bit = (vi.pointerLevel > 0 || vi.type == "int");
                    if (isStruct(vi.type)) {
                        std::string sName = getAggregateName(vi.type);
                        if (structs.count(sName) && structs[sName]->totalSize > 1) is16Bit = true;
                    }
                    if (registerVars.count(rName)) {
                        arg->accept(*this);
                        if (is16Bit) emitter->push_ax();
                        else { emitter->push("a"); pushSize = 1; }
                    } else if (is16Bit) {
                        emit("phw.sp " + rName + ", s");
                    } else {
                        arg->accept(*this); emitter->push("a"); pushSize = 1;
                    }
                }
            } else { arg->accept(*this); emitter->push_ax(); }
            pushedBytesI += pushSize;
            for (const auto& varName : currentVars) {
                emit(".var " + varName + " = " + varName + " + " + std::to_string(pushSize));
            }
        }
        int argBytes = (int)node.arguments.size() * 2;
        resultNeeded = oldNeeded;
        // Indirect call via JSR ($ZP)
        emit("jsr ($" + ssCall.str() + ")");
        freeZP(zpCall, 2);
        // Caller cleanup
        if (argBytes > 0) {
            emitter->taz();
            for (int i = 0; i < argBytes; ++i) emitter->pla();
            emitter->tza();
        }
        if (pushedBytesI > 0) {
            for (const auto& varName : currentVars) {
                emit(".var " + varName + " = " + varName + " - " + std::to_string(pushedBytesI));
            }
        }
        invalidateRegs();
        return;
    }

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
        // C89 implicit declaration: warn and treat as int func()
        std::cerr << loc << "warning: implicit declaration of function '" << node.name << "'\n";
        knownFunctions.insert(node.name);
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

    bool isVariadicCall = variadicFunctions.count(node.name) > 0;

    bool calleeUsesZpCall = (zpCallMode || fastcallFunctions.count(node.name) > 0) && !isVariadicCall;

    if (calleeUsesZpCall) {
        // --- ZP Calling Convention ---
        // Variadic calls use stack convention (callee uses va_start with stack addresses)
        // Compute callee's ZP param layout
        bool hasParamTypes = functionParamTypes.count(node.name) > 0;
        auto& calleePTypes = hasParamTypes ? functionParamTypes[node.name] : functionParamTypes[""];

        struct CalleeParam { uint8_t zpAddr; int size; };
        std::vector<CalleeParam> calleeParams;
        uint8_t zpBase = (uint8_t)(zeroPageStart + 8);
        int zpOff = 0;

        // For struct-returning functions (not long), add hidden pointer as first param
        bool isStructRetCall = structReturningFunctions.count(node.name) > 0;
        bool isLongRetCall = false;
        if (functionReturnTypes.count(node.name)) {
            auto& ri = functionReturnTypes[node.name];
            isLongRetCall = is32BitType(ri.type) && ri.pointerLevel == 0;
        }
        bool needsHiddenPtrCall = isStructRetCall && !isLongRetCall;

        if (needsHiddenPtrCall && structRetDest_ >= 0) {
            calleeParams.push_back({(uint8_t)(zpBase + zpOff), 2});
            zpOff += 2;
        }

        for (size_t ai = 0; ai < node.arguments.size(); ++ai) {
            int pSize = 2; // default
            if (hasParamTypes && ai < calleePTypes.size()) {
                if (is8BitType(calleePTypes[ai].type) && calleePTypes[ai].pointerLevel == 0) pSize = 1;
                else if (is32BitType(calleePTypes[ai].type) && calleePTypes[ai].pointerLevel == 0) pSize = 4;
            }
            // Check argument expression type too
            ExpressionType argType = getExprType(node.arguments[ai].get());
            if (is32BitType(argType.type) && argType.pointerLevel == 0) pSize = 4;
            if (auto* lit = dynamic_cast<IntegerLiteral*>(node.arguments[ai].get())) {
                uint32_t uv = (uint32_t)lit->value;
                if (uv > 0xFFFF || (lit->value < 0 && lit->value < -32768)) pSize = 4;
            }
            if (auto* ref = dynamic_cast<VariableReference*>(node.arguments[ai].get())) {
                std::string rn = resolveVarName(ref->name);
                VarInfo* vi = nullptr;
                if (variableTypes.count(rn)) vi = &variableTypes.at(rn);
                else if (globalVariableTypes.count(rn)) vi = &globalVariableTypes.at(rn);
                if (vi && is32BitType(vi->type) && vi->pointerLevel == 0) pSize = 4;
            }
            calleeParams.push_back({(uint8_t)(zpBase + zpOff), pSize});
            zpOff += pSize;
        }

        // Caller-save: save our active (non-spilled) ZP params to frame before clobbering them
        bool hasActiveZpParams = !zpParams_.empty();
        if (hasActiveZpParams) {
            int saveOff = frameSize_; // caller-save area starts after locals (incl spill)
            emit("; caller-save ZP params to frame");
            for (auto& kv : zpParams_) {
                auto& zpi = kv.second;
                for (int b = 0; b < zpi.size; b++) {
                    emit("lda " + zpHex(zpi.zpAddr + b));
                    emit("sta.fp " + getLocalOffsetSymbol(saveOff++));
                }
            }
        }

        // If caller has no active ZP params, we can store args directly.
        // Otherwise, evaluate to stack first to avoid clobbering our own params.
        bool directStore = !hasActiveZpParams;
        int argIdx = 0;
        int totalPushedForArgs = 0;
        if (needsHiddenPtrCall && structRetDest_ >= 0) {
            if (directStore) {
                emit("leax.fp " + getLocalOffsetSymbol(structRetDest_));
                emit("sta " + zpHex(calleeParams[argIdx].zpAddr));
                emit("stx " + zpHex(calleeParams[argIdx].zpAddr + 1));
            } else {
                emit("leax.fp " + getLocalOffsetSymbol(structRetDest_));
                emitter->push_ax();
                totalPushedForArgs += 2;
                for (const auto& varName : currentVars) {
                    emit(".var " + varName + " = " + varName + " + 2");
                }
            }
            argIdx++;
        }

        for (size_t ai = 0; ai < node.arguments.size(); ++ai) {
            auto& arg = node.arguments[ai];
            auto& cp = calleeParams[argIdx + ai];
            arg->accept(*this);

            if (cp.size == 4) {
                ExpressionType aType = getExprType(arg.get());
                bool alreadyWide = is32BitType(aType.type) && aType.pointerLevel == 0;
                if (auto* lit = dynamic_cast<IntegerLiteral*>(arg.get())) {
                    uint32_t uv = (uint32_t)lit->value;
                    if (uv > 0xFFFF || (lit->value < 0 && lit->value < -32768)) alreadyWide = true;
                }
                if (!alreadyWide) {
                    if (aType.isSigned) emit("sxt.16");
                    else { emit("ldy #$00"); emit("ldz #$00"); }
                }
                if (directStore) {
                    emit("sta " + zpHex(cp.zpAddr));
                    emit("stx " + zpHex(cp.zpAddr + 1));
                    emit("pha"); emit("tya"); emit("sta " + zpHex(cp.zpAddr + 2));
                    emit("tza"); emit("sta " + zpHex(cp.zpAddr + 3)); emit("pla");
                } else {
                    emit("phz"); emit("phy"); emit("phx"); emit("pha");
                    totalPushedForArgs += 4;
                    for (const auto& varName : currentVars) {
                        emit(".var " + varName + " = " + varName + " + 4");
                    }
                }
            } else if (cp.size == 1) {
                if (directStore) {
                    emit("sta " + zpHex(cp.zpAddr));
                } else {
                    emitter->push("a");
                    totalPushedForArgs += 1;
                    for (const auto& varName : currentVars) {
                        emit(".var " + varName + " = " + varName + " + 1");
                    }
                }
            } else {
                if (directStore) {
                    emit("sta " + zpHex(cp.zpAddr));
                    emit("stx " + zpHex(cp.zpAddr + 1));
                } else {
                    emitter->push_ax();
                    totalPushedForArgs += 2;
                    for (const auto& varName : currentVars) {
                        emit(".var " + varName + " = " + varName + " + 2");
                    }
                }
            }
        }

        // For indirect path: pop from stack into callee's ZP slots (reverse order)
        if (!directStore && totalPushedForArgs > 0) {
            for (int ai = (int)node.arguments.size() - 1 + argIdx; ai >= 0; --ai) {
                auto& cp = calleeParams[ai];
                if (cp.size == 4) {
                    emit("pla"); emit("sta " + zpHex(cp.zpAddr));
                    emit("pla"); emit("sta " + zpHex(cp.zpAddr + 1));
                    emit("pla"); emit("sta " + zpHex(cp.zpAddr + 2));
                    emit("pla"); emit("sta " + zpHex(cp.zpAddr + 3));
                } else if (cp.size == 1) {
                    emitter->pop("a");
                    emit("sta " + zpHex(cp.zpAddr));
                } else {
                    emitter->pop_ax();
                    emit("sta " + zpHex(cp.zpAddr));
                    emit("stx " + zpHex(cp.zpAddr + 1));
                }
            }
            for (const auto& varName : currentVars) {
                emit(".var " + varName + " = " + varName + " - " + std::to_string(totalPushedForArgs));
            }
        }

        resultNeeded = oldNeeded;
        emit("jsr _" + node.name);

        // Caller-restore: restore our active ZP params from frame
        if (hasActiveZpParams) {
            int saveOff = frameSize_;
            emit("; caller-restore ZP params from frame");
            // Save return value (A/AX/AXYZ) before restoring
            if (resultNeeded) {
                emitter->push_ax(); // save return value — shifts SP by 2
                for (const auto& varName : currentVars) {
                    emit(".var " + varName + " = " + varName + " + 2");
                }
            }
            for (auto& kv : zpParams_) {
                auto& zpi = kv.second;
                for (int b = 0; b < zpi.size; b++) {
                    emit("lda.fp " + getLocalOffsetSymbol(saveOff++));
                    emit("sta " + zpHex(zpi.zpAddr + b));
                }
            }
            if (resultNeeded) {
                for (const auto& varName : currentVars) {
                    emit(".var " + varName + " = " + varName + " - 2");
                }
                emitter->pop_ax(); // restore return value
            }
        }

        invalidateRegs();
        return;
    }

    // --- Stack-based calling convention (original) ---
    // When in zpCallMode and falling through here (e.g., variadic call),
    // save caller's ZP params to frame before the call clobbers them.
    bool zpCallerSaveForStackCall = useZpCall_ && !zpParams_.empty();
    if (zpCallerSaveForStackCall) {
        int saveOff = frameSize_;
        emit("; caller-save ZP params before stack-convention call");
        for (auto& kv : zpParams_) {
            auto& zpi = kv.second;
            for (int b = 0; b < zpi.size; b++) {
                emit("lda " + zpHex(zpi.zpAddr + b));
                emit("sta.fp " + getLocalOffsetSymbol(saveOff++));
            }
        }
    }

    int pushedBytes = 0;

    // For struct-returning functions, push hidden return pointer FIRST
    // (before regular args) so leax.fp uses the un-bumped _fp value.
    bool isStructRetCall = structReturningFunctions.count(node.name) > 0;
    if (isStructRetCall && structRetDest_ >= 0) {
        emit("leax.fp " + getLocalOffsetSymbol(structRetDest_));
        emitter->push_ax();
        int pushSize = 2;
        pushedBytes += pushSize;
        for (const auto& varName : currentVars) {
            emit(".var " + varName + " = " + varName + " + " + std::to_string(pushSize));
        }
    }

    // For variadic functions, push arguments right-to-left so named params
    // end up at fixed offsets from SP regardless of extra args.
    // (isVariadicCall already computed above)
    int argStart = isVariadicCall ? (int)node.arguments.size() - 1 : 0;
    int argEnd   = isVariadicCall ? -1 : (int)node.arguments.size();
    int argStep  = isVariadicCall ? -1 : 1;
    // Look up callee's parameter types for correct push sizes
    bool hasParamTypes = functionParamTypes.count(node.name) > 0;
    auto& calleePTypes = hasParamTypes ? functionParamTypes[node.name] : functionParamTypes[""];

    for (int ai = argStart; ai != argEnd; ai += argStep) {
        auto& arg = node.arguments[ai];
        // Determine the callee's expected param size for this arg position
        bool paramIs8Bit = false;
        bool paramIs32Bit = false;
        if (hasParamTypes && ai < (int)calleePTypes.size()) {
            paramIs8Bit = is8BitType(calleePTypes[ai].type) && calleePTypes[ai].pointerLevel == 0;
            paramIs32Bit = is32BitType(calleePTypes[ai].type) && calleePTypes[ai].pointerLevel == 0;
        }
        // Also check the argument expression type for long
        ExpressionType argType = getExprType(arg.get());
        if (is32BitType(argType.type) && argType.pointerLevel == 0) paramIs32Bit = true;
        // IntegerLiterals exceeding 16-bit range must be pushed as 32-bit
        if (auto* lit = dynamic_cast<IntegerLiteral*>(arg.get())) {
            uint32_t uv = (uint32_t)lit->value;
            if (uv > 0xFFFF || (lit->value < 0 && lit->value < -32768)) paramIs32Bit = true;
        }
        // VariableReference to a long variable: the constant folder may have
        // folded the expression to a small IntegerLiteral losing the long type,
        // but the variable's declared type is still long.
        if (auto* ref = dynamic_cast<VariableReference*>(arg.get())) {
            std::string rn = resolveVarName(ref->name);
            VarInfo* vi = nullptr;
            if (variableTypes.count(rn)) vi = &variableTypes.at(rn);
            else if (globalVariableTypes.count(rn)) vi = &globalVariableTypes.at(rn);
            if (vi && is32BitType(vi->type) && vi->pointerLevel == 0) paramIs32Bit = true;
        }
        // Variadic args always promoted to at least 16-bit
        if (isVariadicCall && ai >= (int)(hasParamTypes ? calleePTypes.size() : 0)) paramIs8Bit = false;
        if (isVariadicCall) paramIs8Bit = false; // default argument promotion

        int pushSize = paramIs32Bit ? 4 : paramIs8Bit ? 1 : 2;
        arg->accept(*this);
        if (paramIs32Bit) {
            // Widen to 32-bit if the argument expression is narrower
            ExpressionType aType = getExprType(arg.get());
            bool alreadyWide = is32BitType(aType.type) && aType.pointerLevel == 0;
            // IntegerLiterals > 16-bit already loaded Y/Z directly
            if (auto* lit = dynamic_cast<IntegerLiteral*>(arg.get())) {
                uint32_t uv = (uint32_t)lit->value;
                if (uv > 0xFFFF || (lit->value < 0 && lit->value < -32768)) alreadyWide = true;
            }
            if (!alreadyWide) {
                if (aType.isSigned) emit("sxt.16");
                else { emit("ldy #$00"); emit("ldz #$00"); }
            }
            // Push AXYZ: Z first (hi), then Y, X, A (lo) for little-endian on stack
            emit("phz"); emit("phy"); emit("phx"); emit("pha");
        } else if (paramIs8Bit) {
            emitter->push("a");
        } else {
            emitter->push_ax();
        }
        // Keep assembler's frame tracking in sync so subsequent stack-relative reads use correct offsets
        pushedBytes += pushSize;
        for (const auto& varName : currentVars) {
            emit(".var " + varName + " = " + varName + " + " + std::to_string(pushSize));
        }
    }
    int argBytes = pushedBytes;
    resultNeeded = oldNeeded; emit("jsr _" + node.name);
    // Caller cleans up pushed arguments
    if (argBytes > 0) {
        emitter->taz();  // save A to Z
        for (int i = 0; i < argBytes; ++i) emitter->pla();
        // Recalculate frame pointer after arg cleanup (SP back at frame level).
        // A is in Z, save X to scratch ZP.
        if (emitter->hasFramePointer()) {
            emitter->stx_scratch();
            emitter->setupFramePointer();
            emitter->ldx_scratch();
        }
        emitter->tza();  // restore A
    } else if (emitter->hasFramePointer()) {
        // No args but still need FP recalc
        emitter->taz();
        emitter->stx_scratch();
        emitter->setupFramePointer();
        emitter->ldx_scratch();
        emitter->tza();
    }
    // Restore assembler frame tracking
    if (pushedBytes > 0) {
        for (const auto& varName : currentVars) {
            emit(".var " + varName + " = " + varName + " - " + std::to_string(pushedBytes));
        }
    }
    // Restore caller's ZP params after stack-convention call
    if (zpCallerSaveForStackCall) {
        int saveOff = frameSize_;
        emit("; caller-restore ZP params after stack-convention call");
        if (resultNeeded) {
            emitter->push_ax();
            for (const auto& varName : currentVars) {
                emit(".var " + varName + " = " + varName + " + 2");
            }
        }
        for (auto& kv : zpParams_) {
            auto& zpi = kv.second;
            for (int b = 0; b < zpi.size; b++) {
                emit("lda.fp " + getLocalOffsetSymbol(saveOff++));
                emit("sta " + zpHex(zpi.zpAddr + b));
            }
        }
        if (resultNeeded) {
            for (const auto& varName : currentVars) {
                emit(".var " + varName + " = " + varName + " - 2");
            }
            emitter->pop_ax();
        }
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
    int srcSize = is32BitType(srcType.type) ? 4 : (srcType.pointerLevel > 0 || srcType.type == "int") ? 2 : 1;
    int dstSize = is32BitType(node.targetType) ? 4 : (node.pointerLevel > 0 || node.targetType == "int" || node.targetType == "void") ? 2 : 1;

    if (node.targetType == "_Bool") {
        emitBoolNormalize(srcSize);
        emitter->ldx_imm(0);
        updateRegX(0);
    } else if (dstSize == 4 && srcSize <= 2) {
        // Widening to 32-bit: extend AX to AXYZ
        if (srcSize == 1) { emitter->ldx_imm(0); updateRegX(0); }
        if (node.isSigned || srcType.isSigned) emit("sxt.16"); // sign-extend AX → AXYZ
        else { emit("ldy #$00"); emit("ldz #$00"); } // zero-extend
    } else if (srcSize == 4 && dstSize <= 2) {
        // Narrowing from 32-bit: keep A (and X for 16-bit), discard Y/Z
        if (dstSize == 1) { emitter->ldx_imm(0); updateRegX(0); }
    } else if (srcSize == 2 && dstSize == 1) {
        emitter->ldx_imm(0);
        updateRegX(0);
    } else if (srcSize == 1 && dstSize == 2) {
        emitter->ldx_imm(0);
        updateRegX(0);
    }
    // Same size: no-op
}

void CodeGenerator::visit(CompoundLiteral& node) {
    embedSource(node);
    if (!resultNeeded) return;

    auto& initList = *node.initializer;

    // Scalar compound literal: (int){42} — just produce the value, no temp needed
    if (node.arrayDims.empty() && !isStruct(node.targetType) && node.pointerLevel == 0) {
        if (initList.elements.size() >= 1) {
            bool oldNeeded = resultNeeded;
            resultNeeded = true;
            initList.elements[0]->accept(*this);
            resultNeeded = oldNeeded;
        }
        return;
    }

    // Struct/array compound literal: allocate frame temp, init, return address
    std::string lName = "_cl_" + std::to_string(node.tempId);

    if (frameLocals_.count(lName) == 0) {
        throw std::runtime_error("Compound literal temporary '" + lName + "' not found in frame layout");
    }
    int frameOff = frameLocals_[lName];

    if (isStruct(node.targetType) && node.pointerLevel == 0) {
        // Struct compound literal: (struct Point){1, 2}
        std::string sName = getAggregateName(node.targetType);
        if (!structs.count(sName))
            return; // unknown struct in compound literal — handled by IR path
        auto& sInfo = *structs[sName];

        // Build ordered member list (by offset)
        std::vector<std::pair<std::string, MemberInfo*>> orderedMembers;
        for (auto& [name, minfo] : sInfo.members) {
            orderedMembers.push_back({name, &minfo});
        }
        std::sort(orderedMembers.begin(), orderedMembers.end(),
                  [](auto& a, auto& b) { return a.second->offset < b.second->offset; });

        for (int i = 0; i < (int)orderedMembers.size() && i < (int)initList.elements.size(); i++) {
            auto& minfo = *orderedMembers[i].second;
            int memberOff = frameOff + minfo.offset;
            int memberSize = (minfo.pointerLevel > 0 || minfo.type == "int") ? 2 :
                             (minfo.type == "char" || minfo.type == "_Bool") ? 1 : 2;
            if (minfo.arraySize() >= 0) memberSize *= minfo.arraySize();

            if (auto* lit = dynamic_cast<IntegerLiteral*>(initList.elements[i].get())) {
                if (memberSize == 2) {
                    emitter->lda_imm(lit->value & 0xFF);
                    emit("sta.fp " + getLocalOffsetSymbol(memberOff));
                    emitter->lda_imm((lit->value >> 8) & 0xFF);
                    emit("sta.fp " + getLocalOffsetSymbol(memberOff + 1));
                } else {
                    emitter->lda_imm(lit->value & 0xFF);
                    emit("sta.fp " + getLocalOffsetSymbol(memberOff));
                }
            } else {
                bool oldNeeded = resultNeeded;
                resultNeeded = true;
                initList.elements[i]->accept(*this);
                resultNeeded = oldNeeded;
                if (memberSize == 2) {
                    emit("stax.fp " + getLocalOffsetSymbol(memberOff));
                } else {
                    emit("sta.fp " + getLocalOffsetSymbol(memberOff));
                }
            }
        }
    } else if (!node.arrayDims.empty()) {
        // Array compound literal: (int[]){1, 2, 3}
        int elementSize = (node.pointerLevel > 0 || node.targetType == "int") ? 2 :
                          (node.targetType == "char" || node.targetType == "_Bool") ? 1 : 2;
        int totalElements = 1;
        for (int d : node.arrayDims) totalElements *= d;

        for (int i = 0; i < totalElements; i++) {
            int elemOff = frameOff + i * elementSize;
            if (i < (int)initList.elements.size()) {
                if (auto* lit = dynamic_cast<IntegerLiteral*>(initList.elements[i].get())) {
                    if (elementSize == 2) {
                        emitter->lda_imm(lit->value & 0xFF);
                        emit("sta.fp " + getLocalOffsetSymbol(elemOff));
                        emitter->lda_imm((lit->value >> 8) & 0xFF);
                        emit("sta.fp " + getLocalOffsetSymbol(elemOff + 1));
                    } else {
                        emitter->lda_imm(lit->value & 0xFF);
                        emit("sta.fp " + getLocalOffsetSymbol(elemOff));
                    }
                } else {
                    bool oldNeeded = resultNeeded;
                    resultNeeded = true;
                    initList.elements[i]->accept(*this);
                    resultNeeded = oldNeeded;
                    if (elementSize == 2) {
                        emit("stax.fp " + getLocalOffsetSymbol(elemOff));
                    } else {
                        emit("sta.fp " + getLocalOffsetSymbol(elemOff));
                    }
                }
            } else {
                // Zero-fill remaining elements
                emitter->lda_imm(0);
                emit("sta.fp " + getLocalOffsetSymbol(elemOff));
                if (elementSize == 2) {
                    emit("sta.fp " + getLocalOffsetSymbol(elemOff + 1));
                }
            }
        }
    }

    // Return address of the temporary in AX
    emit("leax.fp " + getLocalOffsetSymbol(frameOff));
    invalidateRegs();
}

void CodeGenerator::visit(AlignofExpression& node) {
    int alignment = 1;
    if (node.pointerLevel > 0 || node.typeName == "int") alignment = 2;
    else if (is8BitType(node.typeName)) alignment = 1;
    else { std::string sName = getAggregateName(node.typeName); if (structs.count(sName)) alignment = structs[sName]->alignment; }
    emitter->lda_imm(alignment & 0xFF); updateRegA(alignment & 0xFF); invalidateFlags();
}

// Try to extract a compile-time constant integer from an expression tree.
// Handles IntegerLiteral, CastExpression, unary -/~, and binary ops on constants.
void CodeGenerator::emitData() {
    // Emit .global/.weak for all global variables in relocatable mode (skip static)
    if (relocMode) {
        for (auto* gVar : globalVars) {
            if (staticGlobals.count(gVar->name)) continue;
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
        // Resolve _Alignas if not already set
        if (gVar->alignment == 0 && gVar->alignmentExpr)
            gVar->alignment = resolveAlignmentExpr(gVar->alignmentExpr.get(), structs);
        if (gVar->alignment > 1) out << "    .align " << std::to_string(gVar->alignment) << std::endl;
        out << "_" << gVar->name << ":" << std::endl;
        int size = 0;
        if (gVar->pointerLevel > 0) size = 2;
        else if (is8BitType(gVar->type)) size = 1;
        else if (is32BitType(gVar->type)) size = 4;
        else if (gVar->type == "int") size = 2;
        else if (isStruct(gVar->type)) { std::string sName = getAggregateName(gVar->type); if (structs.count(sName)) size = structs[sName]->totalSize; }

        if (gVar->arraySize() >= 0) size *= gVar->arraySize();

        if (auto* initList = dynamic_cast<InitializerList*>(gVar->initializer.get())) {
            int elementSize = 0;
            if (gVar->pointerLevel > 0) elementSize = 2;
            else if (is8BitType(gVar->type)) elementSize = 1;
            else if (is32BitType(gVar->type)) elementSize = 4;
            else if (gVar->type == "int") elementSize = 2;
            else if (isStruct(gVar->type)) { std::string sName = getAggregateName(gVar->type); if (structs.count(sName)) elementSize = structs[sName]->totalSize; }
            int totalElements = gVar->arraySize() >= 0 ? gVar->arraySize() : 1;
            // Resolve designators (array [N]=val or struct .member=val for global struct init)
            std::vector<Expression*> resolved(totalElements, nullptr);
            if (isStruct(gVar->type) && gVar->pointerLevel == 0 && gVar->arraySize() < 0) {
                // Global struct: resolve member designators to per-member values
                std::string sName = getAggregateName(gVar->type);
                if (structs.count(sName)) {
                    auto& sInfo = *structs[sName];
                    std::vector<std::pair<std::string, MemberInfo*>> orderedMembers;
                    for (auto& [mname, minfo] : sInfo.members) orderedMembers.push_back({mname, &minfo});
                    std::sort(orderedMembers.begin(), orderedMembers.end(),
                              [](auto& a, auto& b) { return a.second->offset < b.second->offset; });
                    resolved.resize(orderedMembers.size(), nullptr);
                    int nextPos = 0;
                    for (size_t ei = 0; ei < initList->elements.size(); ei++) {
                        auto desig = initList->getDesignator(ei);
                        if (!desig.memberName.empty()) {
                            for (int j = 0; j < (int)orderedMembers.size(); j++) {
                                if (orderedMembers[j].first == desig.memberName) { nextPos = j; break; }
                            }
                        }
                        if (nextPos < (int)resolved.size()) {
                            resolved[nextPos] = initList->elements[ei].get();
                            nextPos++;
                        }
                    }
                    // Emit each member in offset order
                    for (size_t mi = 0; mi < orderedMembers.size(); mi++) {
                        auto& minfo = *orderedMembers[mi].second;
                        int mSize = (minfo.pointerLevel > 0 || minfo.type == "int") ? 2 :
                                    is8BitType(minfo.type) ? 1 : is32BitType(minfo.type) ? 4 : 2;
                        int constVal = 0;
                        if (resolved[mi]) tryEvalConstInt(resolved[mi], constVal);
                        if (mSize == 1) out << "    .byte $" << std::right << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (constVal & 0xFF) << std::dec << std::endl;
                        else if (mSize == 2) out << "    .word $" << std::right << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (constVal & 0xFFFF) << std::dec << std::endl;
                        else if (mSize == 4) out << "    .dword $" << std::right << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << (constVal & 0xFFFFFFFF) << std::dec << std::endl;
                        else out << "    .res " << std::to_string(mSize) << ", 0" << std::endl;
                    }
                }
            } else {
                // Array or scalar: resolve array designators
                int nextIdx = 0;
                for (size_t ei = 0; ei < initList->elements.size(); ei++) {
                    auto desig = initList->getDesignator(ei);
                    if (desig.arrayIndex >= 0) nextIdx = desig.arrayIndex;
                    if (nextIdx < totalElements) {
                        resolved[nextIdx] = initList->elements[ei].get();
                        nextIdx++;
                    }
                }
                for (int i = 0; i < totalElements; i++) {
                    int constVal = 0;
                    if (resolved[i]) tryEvalConstInt(resolved[i], constVal);
                    if (elementSize == 1) out << "    .byte $" << std::right << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (constVal & 0xFF) << std::dec << std::endl;
                    else if (elementSize == 2) out << "    .word $" << std::right << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (constVal & 0xFFFF) << std::dec << std::endl;
                    else if (elementSize == 4) out << "    .dword $" << std::right << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << (constVal & 0xFFFFFFFF) << std::dec << std::endl;
                    else out << "    .res " << std::to_string(elementSize) << ", 0" << std::endl;
                }
            }
        } else {
            int constVal;
            if (tryEvalConstInt(gVar->initializer.get(), constVal)) {
                if (size == 1) out << "    .byte $" << std::right << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (constVal & 0xFF) << std::dec << std::endl;
                else if (size == 2) out << "    .word $" << std::right << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (constVal & 0xFFFF) << std::dec << std::endl;
                else if (size == 4) out << "    .dword $" << std::right << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << (constVal & 0xFFFFFFFF) << std::dec << std::endl;
                else out << "    .res " << std::to_string(size) << ", 0" << std::endl;
            } else {
                out << "    .res " << std::to_string(size) << ", 0" << std::endl;
            }
        }
    }

    // BSS section — uninitialized globals
    if (!uninitializedVars.empty() || !relocMode) {
        out << std::endl << ".bss" << std::endl;
        out << "; BSS Section" << std::endl;
        // In flat mode: emit __bss_start/__bss_end labels (no linker to provide them).
        // In reloc mode: the linker provides __bss_start/__bss_end at merged BSS boundaries.
        if (!relocMode) out << "__bss_start:" << std::endl;
        for (auto* gVar : uninitializedVars) {
            // Resolve _Alignas if not already set
            if (gVar->alignment == 0 && gVar->alignmentExpr)
                gVar->alignment = resolveAlignmentExpr(gVar->alignmentExpr.get(), structs);
            if (gVar->alignment > 1) out << "    .align " << std::to_string(gVar->alignment) << std::endl;
            std::string gName = "_" + gVar->name;
            if (relocMode && !staticGlobals.count(gVar->name)) out << ".global " << gName << std::endl;
            out << gName << ":" << std::endl;
            int size = 0;
            if (gVar->pointerLevel > 0) size = 2;
            else if (is8BitType(gVar->type)) size = 1;
            else if (is32BitType(gVar->type)) size = 4;
            else if (gVar->type == "int") size = 2;
            else if (isStruct(gVar->type)) { std::string sName = getAggregateName(gVar->type); if (structs.count(sName)) size = structs[sName]->totalSize; }
            if (gVar->arraySize() >= 0) size *= gVar->arraySize();
            out << "    .res " << std::to_string(size) << std::endl;
        }
        if (!relocMode) out << "__bss_end:" << std::endl;
    }

    out << std::endl << ".data" << std::endl;
    for (const auto& entry : stringPool) {
        out << entry.second << ":" << std::endl;
        if (screencodeStrings.count(entry.first)) {
            std::string content = entry.first.substr(1);
            out << "    .screencode \"" << content << "\"" << std::endl;
        } else if (asciiStrings.count(entry.first)) {
            std::string content = entry.first.substr(1);
            out << "    .ascii \"" << content << "\"" << std::endl;
        } else {
            out << "    .text \"" << entry.first << "\"" << std::endl;
        }
        out << "    .byte 0" << std::endl;
    }
}

void CodeGenerator::clobberReg(uint8_t mask) {
    if (currentClobbers_) currentClobbers_->regMask |= mask;
}

void CodeGenerator::clobberFlag(uint8_t mask) {
    if (currentClobbers_) currentClobbers_->flagMask |= mask;
}

void CodeGenerator::invalidateFromClobbers(uint8_t regMask, uint8_t flagMask) {
    if (regMask & CLOBBER_A) { regA.known = false; regA.isVariable = false; regA.varName = ""; regA.varOffset = 0; regA.value = 0; }
    if (regMask & CLOBBER_X) { regX.known = false; regX.isVariable = false; regX.varName = ""; regX.varOffset = 0; regX.value = 0; }
    if (regMask & CLOBBER_Y) { regY.known = false; regY.isVariable = false; regY.varName = ""; regY.varOffset = 0; regY.value = 0; }
    if (regMask & CLOBBER_Z) { regZ.known = false; regZ.isVariable = false; regZ.varName = ""; regZ.varOffset = 0; regZ.value = 0; }
    if (flagMask & CLOBBER_C) flags.carry = TriState::UNKNOWN;
    if (flagMask & CLOBBER_N) flags.negative = TriState::UNKNOWN;
    if (flagMask & CLOBBER_ZF) flags.zero = TriState::UNKNOWN;
    if (flagMask & CLOBBER_V) flags.overflow = TriState::UNKNOWN;
    if (flagMask & (CLOBBER_N | CLOBBER_ZF)) flags.znSource = FlagSource::NONE;
}

void CodeGenerator::invalidateRegs() {
    regA.known = false; regA.isVariable = false; regA.varName = ""; regA.varOffset = 0; regA.value = 0;
    regX.known = false; regX.isVariable = false; regX.varName = ""; regX.varOffset = 0; regX.value = 0;
    regY.known = false; regY.isVariable = false; regY.varName = ""; regY.varOffset = 0; regY.value = 0;
    regZ.known = false; regZ.isVariable = false; regZ.varName = ""; regZ.varOffset = 0; regZ.value = 0;
    clobberReg(CLOBBER_A | CLOBBER_X | CLOBBER_Y | CLOBBER_Z);
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
    if (s && d) {
        *d = *s;
        if (dest == FlagSource::A) clobberReg(CLOBBER_A);
        else if (dest == FlagSource::X) clobberReg(CLOBBER_X);
        else if (dest == FlagSource::Y) clobberReg(CLOBBER_Y);
        else if (dest == FlagSource::Z) clobberReg(CLOBBER_Z);
        updateZNFlags(dest, flags.zero, flags.negative);
    }
}

void CodeGenerator::updateFlags(TriState c, TriState z, TriState n, TriState v) {
    if (c != TriState::UNKNOWN) { flags.carry = c; clobberFlag(CLOBBER_C); }
    if (z != TriState::UNKNOWN) { flags.zero = z; clobberFlag(CLOBBER_ZF); }
    if (n != TriState::UNKNOWN) { flags.negative = n; clobberFlag(CLOBBER_N); }
    if (v != TriState::UNKNOWN) { flags.overflow = v; clobberFlag(CLOBBER_V); }
    if (z != TriState::UNKNOWN || n != TriState::UNKNOWN) flags.znSource = FlagSource::NONE;
}

void CodeGenerator::updateZNFlags(FlagSource source, TriState z, TriState n) {
    flags.znSource = source; flags.zero = z; flags.negative = n;
    clobberFlag(CLOBBER_ZF | CLOBBER_N);
}

void CodeGenerator::invalidateFlags() {
    flags.carry = TriState::UNKNOWN; flags.zero = TriState::UNKNOWN; flags.negative = TriState::UNKNOWN; flags.overflow = TriState::UNKNOWN; flags.znSource = FlagSource::NONE;
    clobberFlag(CLOBBER_C | CLOBBER_N | CLOBBER_ZF | CLOBBER_V);
}

void CodeGenerator::updateRegA(uint8_t val) {
    regA.known = true; regA.isVariable = false; regA.value = val;
    clobberReg(CLOBBER_A);
    updateZNFlags(FlagSource::A, val == 0 ? TriState::SET : TriState::CLEAR, (val & 0x80) ? TriState::SET : TriState::CLEAR);
}
void CodeGenerator::updateRegX(uint8_t val) {
    regX.known = true; regX.isVariable = false; regX.value = val;
    clobberReg(CLOBBER_X);
    updateZNFlags(FlagSource::X, val == 0 ? TriState::SET : TriState::CLEAR, (val & 0x80) ? TriState::SET : TriState::CLEAR);
}
void CodeGenerator::updateRegY(uint8_t val) {
    regY.known = true; regY.isVariable = false; regY.value = val;
    clobberReg(CLOBBER_Y);
    updateZNFlags(FlagSource::Y, val == 0 ? TriState::SET : TriState::CLEAR, (val & 0x80) ? TriState::SET : TriState::CLEAR);
}
void CodeGenerator::updateRegZ(uint8_t val) {
    regZ.known = true; regZ.isVariable = false; regZ.value = val;
    clobberReg(CLOBBER_Z);
    updateZNFlags(FlagSource::Z, val == 0 ? TriState::SET : TriState::CLEAR, (val & 0x80) ? TriState::SET : TriState::CLEAR);
}
void CodeGenerator::updateRegAVar(const std::string& name, int offset) { regA.known = true; regA.isVariable = true; regA.varName = name; regA.varOffset = offset; clobberReg(CLOBBER_A); updateZNFlags(FlagSource::A); }
void CodeGenerator::updateRegXVar(const std::string& name, int offset) { regX.known = true; regX.isVariable = true; regX.varName = name; regX.varOffset = offset; clobberReg(CLOBBER_X); updateZNFlags(FlagSource::X); }
void CodeGenerator::updateRegYVar(const std::string& name, int offset) { regY.known = true; regY.isVariable = true; regY.varName = name; regY.varOffset = offset; clobberReg(CLOBBER_Y); updateZNFlags(FlagSource::Y); }
void CodeGenerator::updateRegZVar(const std::string& name, int offset) { regZ.known = true; regZ.isVariable = true; regZ.varName = name; regZ.varOffset = offset; clobberReg(CLOBBER_Z); updateZNFlags(FlagSource::Z); }

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

void CodeGenerator::freeRegisterVars() {
    for (auto& [name, rv] : registerVars) {
        freeZP(rv.zpIndex, rv.size);
    }
    registerVars.clear();
}

class VariableUseChecker : public ASTVisitor {
public:
    std::string targetVarName; bool used = false; std::string currentDeclVarName;
    CodeGenerator& gen;
    VariableUseChecker(const std::string& name, const std::string& currentDecl, CodeGenerator& g) : targetVarName(name), currentDeclVarName(currentDecl), gen(g) {}
    void visit(IntegerLiteral&) override {}
    void visit(FloatLiteral&) override {}
    void visit(StringLiteral&) override {}
    void visit(VariableReference& node) override { if (node.name == targetVarName) used = true; }
    void visit(Assignment& node) override { if (auto* ref = dynamic_cast<VariableReference*>(node.target.get())) if (ref->name == targetVarName) used = true; node.expression->accept(*this); if (node.target) node.target->accept(*this); }
    void visit(BinaryOperation& node) override { node.left->accept(*this); node.right->accept(*this); }
    void visit(UnaryOperation& node) override { node.operand->accept(*this); }
    void visit(FunctionCall& node) override { for (auto& arg : node.arguments) arg->accept(*this); }
    void visit(BuiltinVaStart& node) override { node.ap->accept(*this); }
    void visit(BuiltinVaArg& node) override { node.ap->accept(*this); }
    void visit(CpuRegisterAccess&) override {}
    void visit(CpuFlagAccess&) override {}
    void visit(MemberAccess& node) override { node.structExpr->accept(*this); }
    void visit(ConditionalExpression& node) override { node.condition->accept(*this); if (node.thenExpr) node.thenExpr->accept(*this); node.elseExpr->accept(*this); }
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
    void visit(InitializerList& node) override { for (auto& elem : node.elements) if (elem) elem->accept(*this); }
    void visit(ArrayAccess& node) override { if (node.arrayExpr) node.arrayExpr->accept(*this); if (node.indexExpr) node.indexExpr->accept(*this); }
    void visit(CastExpression& node) override { if (node.expression) node.expression->accept(*this); }
    void visit(CompoundLiteral& node) override { if (node.initializer) for (auto& e : node.initializer->elements) if (e) e->accept(*this); }
    void visit(AlignofExpression&) override {}
    void visit(SizeofExpression& node) override { if (!node.isType && node.expression) node.expression->accept(*this); }
    void visit(VariableDeclaration& node) override { if (node.initializer) node.initializer->accept(*this); }
    void visit(ReturnStatement& node) override { if(node.expression) node.expression->accept(*this); }
    void visit(BreakStatement&) override {}
    void visit(ContinueStatement&) override {}
    void visit(SwitchContinueStatement& node) override { if (node.target) node.target->accept(*this); }
    void visit(GotoStatement& node) override { if (node.target) node.target->accept(*this); }
    void visit(LabelledStatement& node) override { if (node.statement) node.statement->accept(*this); }
    void visit(ExpressionStatement& node) override { if (node.expression) node.expression->accept(*this); }
    void visit(IfStatement& node) override { if (node.condition) node.condition->accept(*this); if (node.thenBranch) node.thenBranch->accept(*this); if(node.elseBranch) node.elseBranch->accept(*this); }
    void visit(WhileStatement& node) override { if (node.condition) node.condition->accept(*this); if (node.body) node.body->accept(*this); }
    void visit(DoWhileStatement& node) override { if (node.body) node.body->accept(*this); if (node.condition) node.condition->accept(*this); }
    void visit(ForStatement& node) override {
        if (node.initializer) node.initializer->accept(*this);
        if (node.condition) node.condition->accept(*this);
        if (node.increment) node.increment->accept(*this);
        if (node.body) node.body->accept(*this);
    }
    void visit(RepeatStatement& node) override { if (node.body) node.body->accept(*this); }
    void visit(SwitchStatement& node) override { if (node.expression) node.expression->accept(*this); if (node.body) node.body->accept(*this); }
    void visit(CaseStatement& node) override { if (node.value) node.value->accept(*this); if (node.rangeEnd) node.rangeEnd->accept(*this); }
    void visit(DefaultStatement&) override {}
    void visit(AsmStatement&) override {}
    void visit(StaticAssert&) override {}
    void visit(EnumDefinition&) override {}
    void visit(StructDefinition&) override {}
    void visit(CompoundStatement& node) override { for(auto& s : node.statements) if (s) s->accept(*this); }
    void visit(FunctionDeclaration& node) override { if (node.body) node.body->accept(*this); }
    void visit(LabelAddressExpression&) override {}
    void visit(TranslationUnit& node) override { for(auto& decl : node.topLevelDecls) if (decl) decl->accept(*this); }
};

bool CodeGenerator::isVariableUsed(const std::string& varName, FunctionDeclaration& func) {
    VariableUseChecker checker(varName, varName, *this);
    func.body->accept(checker);
    return checker.used;
}

void CodeGenerator::visit(LabelAddressExpression& node) {
    if (!resultNeeded) return;
    emit("ldax #@" + node.label);
    invalidateRegs();
}
