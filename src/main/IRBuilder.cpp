#include "IRBuilder.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>

IRBuilder::IRBuilder() {}

void IRBuilder::setSourceInfo(const std::string& filename) {
    module_.sourceFile = filename;
}

void IRBuilder::generate(TranslationUnit& unit) {
    unit.accept(*this);
    // Compute extern symbols: called but not defined in this module
    for (const auto& name : calledFunctions_) {
        if (!definedFunctions_.count(name)) {
            module_.externs.push_back(name);
        }
    }
}


std::string IRBuilder::newLabel(const std::string& prefix) {
    return prefix + std::to_string(nextLabel_++);
}

void IRBuilder::emit(ir::Inst inst) {
    if (currentBlock_) currentBlock_->insts.push_back(std::move(inst));
}

ir::Block& IRBuilder::startBlock(const std::string& label) {
    if (!currentFunc_) {
        // Safety: create a dummy function if none exists
        module_.functions.push_back({});
        currentFunc_ = &module_.functions.back();
        currentFunc_->name = "__orphan";
    }
    currentFunc_->blocks.push_back({label, {}});
    currentBlock_ = &currentFunc_->blocks.back();
    return *currentBlock_;
}

ir::Operand IRBuilder::allocVreg(ir::Type t) {
    if (!currentFunc_) return ir::Operand::none();
    uint32_t id = currentFunc_->allocVreg();
    currentFunc_->vregTypes[id] = t;
    return ir::Operand::vreg(id, t);
}

ir::Type IRBuilder::mapType(const std::string& typeName, int ptrLevel) {
    if (ptrLevel > 0) return ir::Type::PTR;
    if (typeName == "char" || typeName == "unsigned char") return ir::Type::I8;
    if (typeName == "long" || typeName == "unsigned long") return ir::Type::I32;
    if (typeName == "void") return ir::Type::VOID;
    
    // Check for 4-byte structs/unions
    if (getTypeSize(typeName, 0) == 4) return ir::Type::I32;

    return ir::Type::I16;
}

int IRBuilder::getTypeSize(const std::string& typeName, int ptrLevel) {
    if (ptrLevel > 0) return 2;
    if (typeName == "char" || typeName == "unsigned char") return 1;
    if (typeName == "int" || typeName == "unsigned int" ||
        typeName == "short" || typeName == "unsigned short" ||
        typeName == "unsigned") return 2;
    if (typeName == "long" || typeName == "unsigned long") return 4;
    
    std::string sName = getAggregateName(typeName);
    auto it = structs_.find(sName);
    if (it != structs_.end()) return it->second.totalSize;
    return 2; // default for unknown types
}

IRBuilder::IRTypeInfo IRBuilder::getExprTypeInfo(Expression* expr) {
    if (!expr) return {};
    
    if (auto* lit = dynamic_cast<IntegerLiteral*>(expr)) {
        ir::Type t = ir::Type::I16;
        std::string tn = "int";
        bool isSigned = true;
        if (!lit->castType.empty()) {
            tn = lit->castType;
            t = mapType(lit->castType, lit->castPointerLevel);
            isSigned = lit->castIsSigned;
        }
        return {t, tn, isSigned, (t == ir::Type::PTR) ? ir::Type::I16 : ir::Type::VOID, ir::typeSize(t)};
    }
    
    if (auto* cast = dynamic_cast<CastExpression*>(expr)) {
        ir::Type t = mapType(cast->targetType, cast->pointerLevel);
        return {t, cast->targetType, cast->isSigned, (t == ir::Type::PTR) ? mapType(cast->targetType, 0) : ir::Type::VOID, ir::typeSize(t)};
    }
    
    if (auto* ref = dynamic_cast<VariableReference*>(expr)) {
        // Check for arrays first
        auto ait = localArrayDims_.find(ref->name);
        if (ait != localArrayDims_.end() && !ait->second.empty()) {
            std::string tn = localTypeNames_[ref->name];
            int totalSize = getTypeSize(tn, 0);
            for (int d : ait->second) totalSize *= d;
            return {ir::Type::PTR, tn, false, localTypes_[ref->name], totalSize};
        }

        auto gait = globalArrayDims_.find(ref->name);
        if (gait != globalArrayDims_.end() && !gait->second.empty()) {
            std::string tn = globalTypeNames_[ref->name];
            int totalSize = getTypeSize(tn, 0);
            for (int d : gait->second) totalSize *= d;
            return {ir::Type::PTR, tn, false, globalTypes_[ref->name], totalSize};
        }

        auto it = localTypes_.find(ref->name);
        if (it != localTypes_.end()) {
            IRTypeInfo info;
            info.type = it->second;
            info.typeName = localTypeNames_[ref->name];
            info.isSigned = localSigned_[ref->name];
            info.totalSize = ir::typeSize(info.type);
            if (info.type == ir::Type::PTR) {
                auto pit = localPointedToType_.find(ref->name);
                if (pit != localPointedToType_.end()) info.pointedToType = pit->second;
            }
            return info;
        }
        auto git = globalTypes_.find(ref->name);
        if (git != globalTypes_.end()) {
            IRTypeInfo info;
            info.type = git->second;
            info.typeName = globalTypeNames_[ref->name];
            info.isSigned = true; 
            info.totalSize = ir::typeSize(info.type);
            if (info.type == ir::Type::PTR) {
                auto gpit = globalPointedToType_.find(ref->name);
                if (gpit != globalPointedToType_.end()) info.pointedToType = gpit->second;
            }
            return info;
        }
    }
    
    if (auto* ma = dynamic_cast<MemberAccess*>(expr)) {
        for (const auto& [sname, sinfo] : structs_) {
            auto mit = sinfo.members.find(ma->memberName);
            if (mit != sinfo.members.end()) {
                IRTypeInfo info;
                info.type = mapType(mit->second.type, mit->second.pointerLevel);
                info.typeName = mit->second.type;
                info.isSigned = mit->second.isSigned;
                info.totalSize = ir::typeSize(info.type);
                if (info.type == ir::Type::PTR) {
                    info.pointedToType = mapType(mit->second.type, 0);
                }
                return info;
            }
        }
    }
    
    if (auto* aa = dynamic_cast<ArrayAccess*>(expr)) {
        Expression* root = aa->arrayExpr.get();
        int depth = 1;
        while (auto* inner = dynamic_cast<ArrayAccess*>(root)) {
            root = inner->arrayExpr.get();
            depth++;
        }
        
        if (auto* ref = dynamic_cast<VariableReference*>(root)) {
            ir::Type baseType = ir::Type::I16;
            std::string tn = "int";
            bool isSigned = true;
            std::vector<int> dims;
            
            auto pit = localPointedToType_.find(ref->name);
            if (pit != localPointedToType_.end()) {
                baseType = pit->second;
                isSigned = localSigned_[ref->name];
                // TODO: pointed-to type name
            } else {
                auto gpit = globalPointedToType_.find(ref->name);
                if (gpit != globalPointedToType_.end()) {
                    baseType = gpit->second;
                } else {
                    auto tit = localTypes_.find(ref->name);
                    if (tit != localTypes_.end()) {
                        baseType = tit->second;
                        isSigned = localSigned_[ref->name];
                    } else {
                        auto gtit = globalTypes_.find(ref->name);
                        if (gtit != globalTypes_.end()) baseType = gtit->second;
                    }
                }
            }

            auto ait = localArrayDims_.find(ref->name);
            if (ait != localArrayDims_.end()) dims = ait->second;
            else {
                auto gait = globalArrayDims_.find(ref->name);
                if (gait != globalArrayDims_.end()) dims = gait->second;
            }

            if (depth < (int)dims.size()) {
                // Not the final dimension -> it's still an array/pointer
                int totalSize = getTypeSize(tn, 0);
                for (size_t i = depth; i < dims.size(); i++) totalSize *= dims[i];
                return {ir::Type::PTR, tn, false, baseType, totalSize};
            }
            // Final dimension (or pointer subscript) -> it's the element type
            return {baseType, tn, isSigned, ir::Type::VOID, getTypeSize(tn, 0)};
        }
    }
    
    if (auto* deref = dynamic_cast<UnaryOperation*>(expr)) {
        if (deref->op == "*") {
            if (auto* ref = dynamic_cast<VariableReference*>(deref->operand.get())) {
                auto pit = localPointedToType_.find(ref->name);
                if (pit != localPointedToType_.end()) return {pit->second, "TODO", localSigned_[ref->name], ir::Type::VOID, ir::typeSize(pit->second)};
                auto gpit = globalPointedToType_.find(ref->name);
                if (gpit != globalPointedToType_.end()) return {gpit->second, "TODO", true, ir::Type::VOID, ir::typeSize(gpit->second)};
            }
        }
        if (deref->op == "&") {
            IRTypeInfo sub = getExprTypeInfo(deref->operand.get());
            return {ir::Type::PTR, sub.typeName + "*", false, sub.type, 2};
        }
    }
    
    return {ir::Type::I16, "int", true, ir::Type::VOID, 2};
}

ir::Operand IRBuilder::emitCast(ir::Operand src, ir::Type targetType, bool isSigned) {
    if (src.type == targetType) return src;

    // Determine conversion type
    ir::Op op = ir::Op::NOP;
    int srcSize = ir::typeSize(src.type);
    int targetSize = ir::typeSize(targetType);

    if (srcSize < targetSize) {
        op = isSigned ? ir::Op::SEXT : ir::Op::ZEXT;
    } else if (srcSize > targetSize) {
        op = ir::Op::TRUNC;
    } else {
        // Same size, different types (e.g. PTR vs I16) — no conversion needed
        return src;
    }

    // Optimize: immediate values can be cast at compile time without emitting instructions
    if (src.isImm()) {
        int64_t val = src.immVal;
        if (op == ir::Op::TRUNC) {
            if (targetType == ir::Type::I8) val &= 0xFF;
            else if (targetType == ir::Type::I16) val &= 0xFFFF;
        } else if (op == ir::Op::SEXT) {
            if (src.type == ir::Type::I8 && targetType == ir::Type::I16) {
                val = (int64_t)(int8_t)(val & 0xFF);
            } else if (src.type == ir::Type::I16 && targetType == ir::Type::I32) {
                val = (int64_t)(int16_t)(val & 0xFFFF);
            }
        }
        // ZEXT: high bits already zero for unsigned values
        return ir::Operand::imm(val, targetType);
    }

    auto dest = allocVreg(targetType);
    ir::Inst inst;
    inst.op = op;
    inst.dest = dest;
    inst.resultType = targetType;
    inst.src1 = src;
    emit(inst);
    return dest;
}

std::string IRBuilder::getAggregateName(const std::string& type) {
    if (type.length() >= 7 && type.substr(0, 7) == "struct ") return type.substr(7);
    if (type.length() >= 6 && type.substr(0, 6) == "union ") return type.substr(6);
    return type;
}

ir::SourceLoc IRBuilder::loc(ASTNode& node) {
    ir::SourceLoc sl;
    sl.file = module_.sourceFile;
    sl.line = node.line;
    return sl;
}

// ============================================================================
// Top-level
// ============================================================================

void IRBuilder::visit(TranslationUnit& node) {
    for (auto& decl : node.topLevelDecls) {
        decl->accept(*this);
    }
}

// ============================================================================
// Functions
// ============================================================================

void IRBuilder::visit(FunctionDeclaration& node) {
    // Record parameter info and return type for all functions (including prototypes)
    {
        std::vector<ParamInfo> pinfo;
        std::vector<ir::Type> ptypes;
        std::vector<bool> psigned;
        for (const auto& p : node.parameters) {
            pinfo.push_back({p.isConst, p.pointerLevel});
            ptypes.push_back(mapType(p.type, p.pointerLevel));
            psigned.push_back(p.isSigned);
        }
        funcParamInfo_[node.name] = pinfo;
        functionParamTypes_[node.name] = ptypes;
        functionParamSigned_[node.name] = psigned;
        functionReturnTypes_[node.name] = mapType(node.returnType, node.returnPointerLevel);
        if (node.isVariadic) variadicFunctions_.insert(node.name);
    }
    if (node.isPrototype || !node.body) return;

    ir::Function fn;
    fn.name = "_" + node.name;
    definedFunctions_.insert(fn.name);
    fn.returnType = mapType(node.returnType, node.returnPointerLevel);
    fn.conv = (zpCallMode || node.isFastcall) ? ir::CallConv::ZP : ir::CallConv::STACK;
    fn.isVariadic = node.isVariadic;
    fn.isStatic = node.isStatic;
    fn.isInterrupt = node.isInterrupt;
    fn.isNaked = node.isNaked;

    for (const auto& p : node.parameters) {
        fn.paramTypes.push_back(mapType(p.type, p.pointerLevel));
        fn.paramNames.push_back(p.name);
    }

    module_.functions.push_back(std::move(fn));
    currentFunc_ = &module_.functions.back();
    locals_.clear();
    localTypes_.clear();
    localSigned_.clear();
    localConst_.clear();
    usedVregs_.clear();
    localDeclLocs_.clear();

    startBlock("entry");

    // Create vRegs for parameters
    for (const auto& p : node.parameters) {
        ir::Type pt = mapType(p.type, p.pointerLevel);
        auto vreg = allocVreg(pt);
        locals_[p.name] = vreg;
        localDeclLocs_[p.name] = loc(node);
        if (currentFunc_) currentFunc_->localNames[p.name] = vreg.vregId;
        localTypes_[p.name] = pt;
        localTypeNames_[p.name] = p.type;
        localSigned_[p.name] = p.isSigned;
        // For non-pointers: const int x → isConst=true, variable is read-only
        // For pointers: const int *p → isConst=true, pointed-to is read-only, pointer itself is writable
        //               int * const p → isPointerConst=true, pointer is read-only
        localConst_[p.name] = (p.pointerLevel == 0 && p.isConst) || p.isPointerConst;
        localPointsToConst_[p.name] = p.isConst && p.pointerLevel > 0;
        if (p.pointerLevel > 0) localPointedToType_[p.name] = mapType(p.type, 0);
        if (p.isVolatile) currentFunc_->memoryVregs.insert(vreg.vregId);
        currentFunc_->localSlotVregs.insert(vreg.vregId);
    }

    // Visit body
    node.body->accept(*this);

    // Check for unused local variables and parameters
    for (const auto& [name, vregOp] : locals_) {
        if (usedVregs_.find(vregOp.vregId) == usedVregs_.end() &&
            externalUsedVars_.find(name) == externalUsedVars_.end()) {
            std::stringstream ss;
            ss << "warning: unused variable '" << name << "'";
            ir::SourceLoc sl = localDeclLocs_[name];
            if (sl.valid()) {
                ss << " at " << sl.file << ":" << sl.line;
            }
            warnings_.push_back(ss.str());
        }
    }

    // Ensure function ends with a return
    if (currentBlock_ && (currentBlock_->insts.empty() ||
        (currentBlock_->insts.back().op != ir::Op::RET &&
         currentBlock_->insts.back().op != ir::Op::RET_VOID &&
         currentBlock_->insts.back().op != ir::Op::BR))) {
        ir::Inst ret;
        ret.op = ir::Op::RET_VOID;
        emit(ret);
    }

    currentFunc_ = nullptr;
    currentBlock_ = nullptr;
}

void IRBuilder::visit(CompoundStatement& node) {
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
}

// ============================================================================
// Variables
// ============================================================================

void IRBuilder::visit(VariableDeclaration& node) {
    ir::Type t = mapType(node.type, node.pointerLevel);

    if (!currentFunc_) {
        // Global variable
        ir::Module::GlobalVar gv;
        gv.name = "_" + node.name;
        gv.type = t;
        gv.size = getTypeSize(node.type, node.pointerLevel);
        if (node.arraySize() > 0) gv.size *= node.arraySize();
        gv.isConst = node.isConst;
        gv.isStatic = node.isStatic;
        if (node.initializer) {
            if (auto* lit = dynamic_cast<IntegerLiteral*>(node.initializer.get())) {
                gv.hasInitValue = true;
                gv.initValue = lit->value;
            } else if (auto* cast = dynamic_cast<CastExpression*>(node.initializer.get())) {
                if (auto* clit = dynamic_cast<IntegerLiteral*>(cast->expression.get())) {
                    gv.hasInitValue = true;
                    gv.initValue = clit->value;
                }
            } else if (auto* il = dynamic_cast<InitializerList*>(node.initializer.get())) {
                gv.hasInitValue = true;
                for (auto const& elem : il->elements) {
                    if (auto* elit = dynamic_cast<IntegerLiteral*>(elem.get())) {
                        gv.initList.push_back(elit->value);
                    } else {
                        gv.initList.push_back(0); // TODO: handle non-constant global initializers?
                    }
                }
            }
        }
        if (node.pointerLevel > 0) globalPointedToType_[node.name] = mapType(node.type, 0);
        if (!node.arrayDims.empty()) globalArrayDims_[node.name] = node.arrayDims;
        globalTypes_[node.name] = t;
        globalTypeNames_[node.name] = node.type;
        module_.globals.push_back(gv);
        return;
    }

    // Local variable — allocate a vReg
    auto vreg = allocVreg(t);
    locals_[node.name] = vreg;
    localDeclLocs_[node.name] = loc(node);
    if (currentFunc_) currentFunc_->localNames[node.name] = vreg.vregId;
    localTypes_[node.name] = t;
    localTypeNames_[node.name] = node.type;
    localSigned_[node.name] = node.isSigned;
    localConst_[node.name] = node.isConst && node.pointerLevel == 0;
    localPointsToConst_[node.name] = node.isConst && node.pointerLevel > 0;
    if (node.pointerLevel > 0) localPointedToType_[node.name] = mapType(node.type, 0);
    if (node.isVolatile && currentFunc_) currentFunc_->memoryVregs.insert(vreg.vregId);
    if (currentFunc_) currentFunc_->localSlotVregs.insert(vreg.vregId);
    if (!node.arrayDims.empty()) {
        localArrayDims_[node.name] = node.arrayDims;
        // Record total byte size for array vRegs
        int totalSize = getTypeSize(node.type, node.pointerLevel);
        for (int d : node.arrayDims) totalSize *= d;
        currentFunc_->vregSizes[vreg.vregId] = totalSize;
        currentFunc_->memoryVregs.insert(vreg.vregId);
    }
    
    if (getTypeSize(node.type, 0) > 2 && node.pointerLevel == 0) {
        // Large aggregates must go to frame with correct byte size
        currentFunc_->memoryVregs.insert(vreg.vregId);
        currentFunc_->vregSizes[vreg.vregId] = getTypeSize(node.type, 0);
    }

    // Emit initializer
    if (node.initializer) {
        if (auto* initList = dynamic_cast<InitializerList*>(node.initializer.get())) {
            if (!node.arrayDims.empty()) {
                // Array initializer: store each element at base + i*elemSize
                // First, get the address of the array
                auto baseAddr = allocVreg(ir::Type::PTR);
                ir::Inst addrInst;
                addrInst.op = ir::Op::ADDR_LOCAL;
                addrInst.dest = baseAddr;
                addrInst.resultType = ir::Type::PTR;
                addrInst.src1 = ir::Operand::vreg(vreg.vregId, ir::Type::PTR);
                addrInst.loc = loc(node);
                emit(addrInst);

                int elemSize = ir::typeSize(t);
                for (size_t i = 0; i < initList->elements.size(); i++) {
                    initList->elements[i]->accept(*this);
                    auto val = lastValue_;
                    // Compute element address: base + i*elemSize
                    auto elemAddr = allocVreg(ir::Type::PTR);
                    ir::Inst elem;
                    elem.op = ir::Op::ADDR_ELEM;
                    elem.dest = elemAddr;
                    elem.resultType = ir::Type::PTR;
                    elem.src1 = baseAddr;
                    elem.src2 = ir::Operand::imm((int)i, ir::Type::I16);
                    elem.args.push_back(ir::Operand::imm(elemSize, ir::Type::I16));
                    elem.loc = loc(node);
                    emit(elem);
                    // Store element
                    ir::Inst store;
                    store.op = ir::Op::STORE;
                    store.resultType = t;
                    store.src1 = val;
                    store.src2 = elemAddr;
                    store.loc = loc(node);
                    emit(store);
                }
            } else if (structs_.count(getAggregateName(node.type))) {
                // Struct initializer list — store each member at its offset
                auto& si = structs_[getAggregateName(node.type)];
                auto baseAddr = allocVreg(ir::Type::PTR);
                ir::Inst addrInst;
                addrInst.op = ir::Op::ADDR_LOCAL;
                addrInst.dest = baseAddr;
                addrInst.resultType = ir::Type::PTR;
                addrInst.src1 = ir::Operand::vreg(vreg.vregId, ir::Type::PTR);
                addrInst.loc = loc(node);
                emit(addrInst);
                for (size_t i = 0; i < initList->elements.size() && i < si.memberOrder.size(); i++) {
                    initList->elements[i]->accept(*this);
                    auto val = lastValue_;
                    auto& mi = si.members[si.memberOrder[i]];
                    ir::Type mt = mapType(mi.type, mi.pointerLevel);
                    auto elemAddr = allocVreg(ir::Type::PTR);
                    ir::Inst add;
                    add.op = ir::Op::ADD;
                    add.dest = elemAddr;
                    add.resultType = ir::Type::PTR;
                    add.src1 = baseAddr;
                    add.src2 = ir::Operand::imm(mi.offset, ir::Type::I16);
                    add.loc = loc(node);
                    emit(add);
                    ir::Inst store;
                    store.op = ir::Op::STORE;
                    store.resultType = mt;
                    store.src1 = val;
                    store.src2 = elemAddr;
                    store.loc = loc(node);
                    emit(store);
                }
                currentFunc_->memoryVregs.insert(vreg.vregId);
            } else {
                // Non-struct initializer list — evaluate and store last value
                node.initializer->accept(*this);
                ir::Inst store;
                store.op = ir::Op::STORE;
                store.resultType = t;
                store.src1 = lastValue_;
                store.src2 = vreg;
                store.loc = loc(node);
                emit(store);
            }
        } else if (auto* cl = dynamic_cast<CompoundLiteral*>(node.initializer.get());
                   cl && cl->initializer && structs_.count(getAggregateName(node.type))) {
            // Struct compound literal — store members directly into variable
            auto& si = structs_[getAggregateName(node.type)];
            auto baseAddr = allocVreg(ir::Type::PTR);
            ir::Inst addrInst;
            addrInst.op = ir::Op::ADDR_LOCAL;
            addrInst.dest = baseAddr;
            addrInst.resultType = ir::Type::PTR;
            addrInst.src1 = ir::Operand::vreg(vreg.vregId, ir::Type::PTR);
            addrInst.loc = loc(node);
            emit(addrInst);
            for (size_t i = 0; i < cl->initializer->elements.size() && i < si.memberOrder.size(); i++) {
                cl->initializer->elements[i]->accept(*this);
                auto val = lastValue_;
                auto& mi = si.members[si.memberOrder[i]];
                ir::Type mt = mapType(mi.type, mi.pointerLevel);
                auto elemAddr = allocVreg(ir::Type::PTR);
                ir::Inst add;
                add.op = ir::Op::ADD;
                add.dest = elemAddr;
                add.resultType = ir::Type::PTR;
                add.src1 = baseAddr;
                add.src2 = ir::Operand::imm(mi.offset, ir::Type::I16);
                add.loc = loc(node);
                emit(add);
                ir::Inst store;
                store.op = ir::Op::STORE;
                store.resultType = mt;
                store.src1 = val;
                store.src2 = elemAddr;
                store.loc = loc(node);
                emit(store);
            }
            currentFunc_->memoryVregs.insert(vreg.vregId);
        } else {
            // Simple scalar initializer
            node.initializer->accept(*this);
            ir::Inst store;
            store.op = ir::Op::STORE;
            store.resultType = t;
            store.src1 = lastValue_;
            store.src2 = vreg;
            store.loc = loc(node);
            emit(store);
        }
    }
}

// ============================================================================
// Expressions
// ============================================================================

void IRBuilder::visit(IntegerLiteral& node) {
    ir::Type t = ir::Type::I16;
    if (!node.castType.empty()) t = mapType(node.castType, node.castPointerLevel);
    auto dest = allocVreg(t);
    ir::Inst inst;
    inst.op = ir::Op::CONST;
    inst.dest = dest;
    inst.resultType = t;
    inst.src1 = ir::Operand::imm(node.value, t);
    inst.loc = loc(node);
    emit(inst);
    lastValue_ = dest;
    lastValueSigned_ = node.castIsSigned;
}

void IRBuilder::visit(StringLiteral& node) {
    std::string name = "__str_" + std::to_string(nextLabel_++);
    // Record the string literal in the module for data section emission
    module_.strings.push_back({name, node.value, node.isAscii});
    auto dest = allocVreg(ir::Type::PTR);
    ir::Inst inst;
    inst.op = ir::Op::ADDR_GLOBAL;
    inst.dest = dest;
    inst.resultType = ir::Type::PTR;
    inst.src1 = ir::Operand::global(name);
    inst.loc = loc(node);
    emit(inst);
    lastValue_ = dest;
}

void IRBuilder::visit(VariableReference& node) {
    auto it = locals_.find(node.name);
    if (it != locals_.end()) {
        if (!computeAddressOnly_) {
            usedVregs_.insert(it->second.vregId);
        }
        if (computeAddressOnly_) {
            // Return the address of the local variable
            auto addr = allocVreg(ir::Type::PTR);
            ir::Inst addrInst;
            addrInst.op = ir::Op::ADDR_LOCAL;
            addrInst.dest = addr;
            addrInst.resultType = ir::Type::PTR;
            addrInst.src1 = it->second; // the vReg slot
            addrInst.loc = loc(node);
            emit(addrInst);
            lastValue_ = addr;
        } else {
            lastValue_ = it->second;
        }
        auto sit = localSigned_.find(node.name);
        lastValueSigned_ = (sit != localSigned_.end()) ? sit->second : false;
    } else {
        // Global variable
        if (computeAddressOnly_) {
            lastValue_ = ir::Operand::global("_" + node.name);
            return;
        }

        auto git = globalArrayDims_.find(node.name);
        if (git != globalArrayDims_.end()) {
            // It's a global array — value is its address
            lastValue_ = ir::Operand::global("_" + node.name);
            return;
        }

        IRTypeInfo info = getExprTypeInfo(&node);
        ir::Type loadType = info.type;

        auto dest = allocVreg(loadType);
        ir::Inst inst;
        inst.op = ir::Op::LOAD;
        inst.dest = dest;
        inst.resultType = loadType;
        inst.src1 = ir::Operand::global("_" + node.name);
        inst.loc = loc(node);
        emit(inst);
        lastValue_ = dest;
    }
}

void IRBuilder::visit(Assignment& node) {
    // 1. Semantic checks for constness
    if (auto* ma = dynamic_cast<MemberAccess*>(node.target.get())) {
        for (const auto& [sname, sinfo] : structs_) {
            auto mit = sinfo.members.find(ma->memberName);
            if (mit != sinfo.members.end() && mit->second.isConst) {
                errors_.push_back("Compile Error: Assignment to read-only location");
                return;
            }
        }
    }
    if (auto* vr = dynamic_cast<VariableReference*>(node.target.get())) {
        auto cit = localConst_.find(vr->name);
        if (cit != localConst_.end() && cit->second) {
            errors_.push_back("Compile Error: Assignment to read-only location");
            return;
        }
    }
    if (auto* deref = dynamic_cast<UnaryOperation*>(node.target.get())) {
        if (deref->op == "*") {
            if (auto* vr = dynamic_cast<VariableReference*>(deref->operand.get())) {
                auto pit = localPointsToConst_.find(vr->name);
                if (pit != localPointsToConst_.end() && pit->second) {
                    errors_.push_back("Compile Error: Assignment to read-only location");
                    return;
                }
            }
        }
    }

    // 2. Evaluate RHS value
    // If RHS is a simple integer literal and LHS is narrower, emit CONST at target width
    // to avoid unnecessary widening + truncation.
    IRTypeInfo lhsTypeHint = getExprTypeInfo(node.target.get());
    if (auto* lit = dynamic_cast<IntegerLiteral*>(node.expression.get())) {
        if (lit->castType.empty() && lhsTypeHint.type == ir::Type::I8) {
            auto dest = allocVreg(ir::Type::I8);
            ir::Inst inst;
            inst.op = ir::Op::CONST;
            inst.dest = dest;
            inst.resultType = ir::Type::I8;
            inst.src1 = ir::Operand::imm(lit->value & 0xFF, ir::Type::I8);
            inst.loc = loc(node);
            emit(inst);
            lastValue_ = dest;
            lastValueSigned_ = false;
        } else {
            node.expression->accept(*this);
        }
    } else {
        node.expression->accept(*this);
    }
    auto rhs = lastValue_;

    // 2.5 Handle CPU register/flag writes
    if (auto* cra = dynamic_cast<CpuRegisterAccess*>(node.target.get())) {
        ir::Inst inst;
        inst.op = ir::Op::CPU_REG_WRITE;
        inst.asmText = cra->regName;
        inst.src1 = rhs;
        inst.loc = loc(node);
        emit(inst);
        lastValue_ = rhs;
        return;
    }
    if (auto* cfa = dynamic_cast<CpuFlagAccess*>(node.target.get())) {
        ir::Inst inst;
        inst.op = ir::Op::CPU_FLAG_WRITE;
        inst.asmText = cfa->flagName;
        inst.src1 = rhs;
        inst.loc = loc(node);
        emit(inst);
        lastValue_ = rhs;
        return;
    }

    // 3. Handle bitfield assignment (special case as it needs RMW)
    if (auto* ma = dynamic_cast<MemberAccess*>(node.target.get())) {
        int bitWidth = 0, bitOffset = 0, memberOffset = 0;
        ir::Type memberType = ir::Type::I16;
        for (const auto& [sname, sinfo] : structs_) {
            auto mit = sinfo.members.find(ma->memberName);
            if (mit != sinfo.members.end() && mit->second.bitWidth > 0) {
                bitWidth = mit->second.bitWidth;
                bitOffset = mit->second.bitOffset;
                memberOffset = mit->second.offset;
                memberType = mapType(mit->second.type, mit->second.pointerLevel);
                break;
            }
        }
        if (bitWidth > 0) {
            ma->structExpr->accept(*this);
            auto base = lastValue_;
            auto addr = allocVreg(ir::Type::PTR);
            ir::Inst add;
            add.op = ir::Op::ADD;
            add.dest = addr;
            add.resultType = ir::Type::PTR;
            add.src1 = base;
            add.src2 = ir::Operand::imm(memberOffset, ir::Type::I16);
            add.loc = loc(node);
            emit(add);

            ir::Inst bfins;
            bfins.op = ir::Op::BFINS;
            bfins.resultType = memberType;
            bfins.src1 = rhs;
            bfins.src2 = addr;
            bfins.args.push_back(ir::Operand::imm(bitOffset, ir::Type::I8));
            bfins.args.push_back(ir::Operand::imm(bitWidth, ir::Type::I8));
            bfins.loc = loc(node);
            emit(bfins);
            lastValue_ = rhs;
            return;
        }
    }

    // 4. Standard assignment
    IRTypeInfo lhsInfo = getExprTypeInfo(node.target.get());
    ir::Type storeType = lhsInfo.type;
    auto rhsVal = emitCast(rhs, storeType, lhsInfo.isSigned);

    // Fast path: simple local or global variable → direct vreg/global store
    if (auto* ref = dynamic_cast<VariableReference*>(node.target.get())) {
        auto lit = locals_.find(ref->name);
        if (lit != locals_.end()) {
            // Direct store to local vreg
            ir::Inst store;
            store.op = ir::Op::STORE;
            store.resultType = storeType;
            store.src1 = rhsVal;
            store.src2 = lit->second;  // vreg directly, not ADDR_LOCAL
            store.loc = loc(node);
            emit(store);
            lastValue_ = rhsVal;
            return;
        }
        // Check global
        auto git = globalTypes_.find(ref->name);
        if (git != globalTypes_.end()) {
            ir::Inst store;
            store.op = ir::Op::STORE;
            store.resultType = storeType;
            store.src1 = rhsVal;
            store.src2 = ir::Operand::global("_" + ref->name);
            store.loc = loc(node);
            emit(store);
            lastValue_ = rhsVal;
            return;
        }
    }

    // General path: evaluate LHS as an address and store through it
    computeAddressOnly_ = true;
    node.target->accept(*this);
    auto lhsAddr = lastValue_;
    computeAddressOnly_ = false;

    ir::Inst store;
    store.op = ir::Op::STORE;
    store.resultType = storeType;
    store.src1 = rhsVal;
    store.src2 = lhsAddr;
    store.loc = loc(node);
    emit(store);

    lastValue_ = rhsVal;
}

void IRBuilder::visit(BinaryOperation& node) {
    IRTypeInfo lhsInfo = getExprTypeInfo(node.left.get());
    IRTypeInfo rhsInfo = getExprTypeInfo(node.right.get());

    // Pre-determine if we can keep this operation at I8 (char width).
    // Check BEFORE visiting operands so literals can be emitted at the right width.
    bool forceI8 = false;
    if (lhsInfo.type == ir::Type::I8) {
        if (auto* rlit = dynamic_cast<IntegerLiteral*>(node.right.get())) {
            if (rlit->castType.empty() && rlit->value >= 0 && rlit->value <= 255) forceI8 = true;
        } else if (rhsInfo.type == ir::Type::I8) {
            forceI8 = true;
        }
    } else if (rhsInfo.type == ir::Type::I8) {
        if (auto* llit = dynamic_cast<IntegerLiteral*>(node.left.get())) {
            if (llit->castType.empty() && llit->value >= 0 && llit->value <= 255) forceI8 = true;
        }
    }

    // Visit LHS
    node.left->accept(*this);
    auto lhs = lastValue_;

    // Visit RHS — emit literal at I8 width if forceI8
    if (forceI8 && dynamic_cast<IntegerLiteral*>(node.right.get())) {
        auto* rlit = dynamic_cast<IntegerLiteral*>(node.right.get());
        auto dest = allocVreg(ir::Type::I8);
        ir::Inst inst;
        inst.op = ir::Op::CONST;
        inst.dest = dest;
        inst.resultType = ir::Type::I8;
        inst.src1 = ir::Operand::imm(rlit->value & 0xFF, ir::Type::I8);
        inst.loc = loc(node);
        emit(inst);
        lastValue_ = dest;
    } else {
        node.right->accept(*this);
    }
    auto rhs = lastValue_;

    // Determine result type
    ir::Type resultType = forceI8 ? ir::Type::I8 : lhs.type;
    if (!forceI8 && ir::typeSize(rhs.type) > ir::typeSize(resultType)) resultType = rhs.type;

    // Cast operands to result type
    auto lhsVal = emitCast(lhs, resultType, lhsInfo.isSigned);
    auto rhsVal = emitCast(rhs, resultType, rhsInfo.isSigned);

    // For comparison operators: use signed ops only if BOTH operands are signed.
    bool bothSigned = lhsInfo.isSigned && rhsInfo.isSigned;

    ir::Op op = ir::Op::NOP;
    ir::Type finalResultType = resultType;

    if (node.op == "+") op = ir::Op::ADD;
    else if (node.op == "-") op = ir::Op::SUB;
    else if (node.op == "*") op = ir::Op::MUL;
    else if (node.op == "/") op = ir::Op::DIV;
    else if (node.op == "%") op = ir::Op::MOD;
    else if (node.op == "&") op = ir::Op::AND;
    else if (node.op == "|") op = ir::Op::OR;
    else if (node.op == "^") op = ir::Op::XOR;
    else if (node.op == "<<") op = ir::Op::SHL;
    else if (node.op == ">>") op = ir::Op::SHR;
    else if (node.op == "==") { op = ir::Op::CMP_EQ; finalResultType = ir::Type::I8; }
    else if (node.op == "!=") { op = ir::Op::CMP_NE; finalResultType = ir::Type::I8; }
    else if (node.op == "<") { op = bothSigned ? ir::Op::CMP_LT : ir::Op::CMP_LTU; finalResultType = ir::Type::I8; }
    else if (node.op == "<=") { op = bothSigned ? ir::Op::CMP_LE : ir::Op::CMP_LEU; finalResultType = ir::Type::I8; }
    else if (node.op == ">") { op = bothSigned ? ir::Op::CMP_GT : ir::Op::CMP_GTU; finalResultType = ir::Type::I8; }
    else if (node.op == ">=") { op = bothSigned ? ir::Op::CMP_GE : ir::Op::CMP_GEU; finalResultType = ir::Type::I8; }
    else if (node.op == "&&") { op = ir::Op::AND; finalResultType = ir::Type::I8; }
    else if (node.op == "||") { op = ir::Op::OR; finalResultType = ir::Type::I8; }

    auto dest = allocVreg(finalResultType);
    ir::Inst inst;
    inst.op = op;
    inst.dest = dest;
    inst.resultType = finalResultType;
    inst.src1 = lhsVal;
    inst.src2 = rhsVal;
    inst.loc = loc(node);
    emit(inst);
    lastValue_ = dest;
}

void IRBuilder::visit(UnaryOperation& node) {
    node.operand->accept(*this);
    auto src = lastValue_;

    if (node.op == "-") {
        auto dest = allocVreg(src.type);
        ir::Inst inst;
        inst.op = ir::Op::NEG;
        inst.dest = dest;
        inst.resultType = src.type;
        inst.src1 = src;
        inst.loc = loc(node);
        emit(inst);
        lastValue_ = dest;
    } else if (node.op == "~") {
        auto dest = allocVreg(src.type);
        ir::Inst inst;
        inst.op = ir::Op::NOT;
        inst.dest = dest;
        inst.resultType = src.type;
        inst.src1 = src;
        inst.loc = loc(node);
        emit(inst);
        lastValue_ = dest;
    } else if (node.op == "!" ) {
        auto dest = allocVreg(ir::Type::I8);
        ir::Inst inst;
        inst.op = ir::Op::CMP_EQ;
        inst.dest = dest;
        inst.resultType = ir::Type::I8;
        inst.src1 = src;
        inst.src2 = ir::Operand::imm(0, src.type);
        inst.loc = loc(node);
        emit(inst);
        lastValue_ = dest;
    } else if (node.op == "&") {
        // Address-of: compute the runtime address of the operand
        if (auto* vr = dynamic_cast<VariableReference*>(node.operand.get())) {
            auto it = locals_.find(vr->name);
            if (it != locals_.end()) {
                // Force to frame because address is taken
                if (currentFunc_) currentFunc_->memoryVregs.insert(it->second.vregId);

                // Local variable: emit ADDR_LOCAL with the vReg ID
                auto dest = allocVreg(ir::Type::PTR);
                ir::Inst inst;
                inst.op = ir::Op::ADDR_LOCAL;
                inst.dest = dest;
                inst.resultType = ir::Type::PTR;
                inst.src1 = ir::Operand::vreg(it->second.vregId, ir::Type::PTR);
                inst.loc = loc(node);
                emit(inst);
                lastValue_ = dest;
            } else {
                // Global variable
                auto dest = allocVreg(ir::Type::PTR);
                ir::Inst inst;
                inst.op = ir::Op::ADDR_GLOBAL;
                inst.dest = dest;
                inst.resultType = ir::Type::PTR;
                inst.src1 = ir::Operand::global("_" + vr->name);
                inst.loc = loc(node);
                emit(inst);
                lastValue_ = dest;
            }
        } else {
            lastValue_ = src;
        }
    } else if (node.op == "*") {
        // Dereference
        bool oldAddrMode = computeAddressOnly_;
        computeAddressOnly_ = false;
        node.operand->accept(*this);
        auto src = lastValue_;
        computeAddressOnly_ = oldAddrMode;

        if (computeAddressOnly_) {
            // We want the address this pointer points to, which IS its value
            lastValue_ = src;
            return;
        }

        IRTypeInfo info = getExprTypeInfo(&node);
        ir::Type loadType = info.type;

        auto dest = allocVreg(loadType);
        ir::Inst inst;
        inst.op = ir::Op::LOAD;
        inst.dest = dest;
        inst.resultType = loadType;
        inst.src1 = src;
        inst.loc = loc(node);
        emit(inst);
        lastValue_ = dest;
    } else if (node.op == "++" || node.op == "--" || node.op == "++_POST" || node.op == "--_POST") {
        bool isPost = (node.op.find("_POST") != std::string::npos);
        std::string baseOp = isPost ? node.op.substr(0, 2) : node.op;

        auto dest = allocVreg(src.type);
        ir::Inst inst;
        inst.op = (baseOp == "++") ? ir::Op::ADD : ir::Op::SUB;
        inst.dest = dest;
        inst.resultType = src.type;
        inst.src1 = src;
        inst.src2 = ir::Operand::imm(1, src.type);
        inst.loc = loc(node);
        emit(inst);

        // Store back to the variable
        ir::Operand addr;
        if (auto* vr = dynamic_cast<VariableReference*>(node.operand.get())) {
            auto lit = locals_.find(vr->name);
            if (lit != locals_.end()) {
                addr = lit->second; // local vreg
            } else {
                addr = ir::Operand::global("_" + vr->name); // global
            }
        } else {
            bool oldAddrMode = computeAddressOnly_;
            computeAddressOnly_ = true;
            node.operand->accept(*this);
            addr = lastValue_;
            computeAddressOnly_ = oldAddrMode;
        }

        ir::Inst store;
        store.op = ir::Op::STORE;
        store.src1 = dest;
        store.src2 = addr;
        store.resultType = src.type;
        emit(store);

        lastValue_ = isPost ? src : dest;
    }
}

void IRBuilder::visit(CastExpression& node) {
    node.expression->accept(*this);
    auto src = lastValue_;
    ir::Type destType = mapType(node.targetType, node.pointerLevel);

    if (src.type == destType) {
        lastValue_ = src;
        return;
    }

    auto dest = allocVreg(destType);
    ir::Inst inst;
    if (ir::typeSize(destType) > ir::typeSize(src.type)) {
        inst.op = node.isSigned ? ir::Op::SEXT : ir::Op::ZEXT;
    } else {
        inst.op = ir::Op::TRUNC;
    }
    inst.dest = dest;
    inst.resultType = destType;
    inst.src1 = src;
    inst.loc = loc(node);
    emit(inst);
    lastValue_ = dest;
}

void IRBuilder::visit(FunctionCall& node) {
    // Check for passing &const_var to non-const pointer parameter
    auto pit = funcParamInfo_.find(node.name);
    if (pit != funcParamInfo_.end()) {
        for (size_t i = 0; i < node.arguments.size() && i < pit->second.size(); i++) {
            const auto& pinfo = pit->second[i];
            // If param is a non-const pointer and argument is &const_var
            if (pinfo.pointerLevel > 0 && !pinfo.isConst) {
                if (auto* addr = dynamic_cast<UnaryOperation*>(node.arguments[i].get())) {
                    if (addr->op == "&") {
                        if (auto* vr = dynamic_cast<VariableReference*>(addr->operand.get())) {
                            auto cit = localConst_.find(vr->name);
                            bool varIsConst = (cit != localConst_.end() && cit->second);
                            auto pcit = localPointsToConst_.find(vr->name);
                            if (!varIsConst && pcit != localPointsToConst_.end()) varIsConst = false;
                            // Check if the variable itself is const (not pointer-to-const)
                            // For `const int x`, localConst_["x"] = true
                            if (cit != localConst_.end() && cit->second) {
                                warnings_.push_back(
                                    "warning: passing argument discards 'const' qualifier");
                            }
                        }
                    }
                }
            }
        }
    }

    // Evaluate arguments
    std::vector<ir::Operand> args;
    for (auto& arg : node.arguments) {
        arg->accept(*this);
        args.push_back(lastValue_);
    }

    ir::Inst inst;
    inst.args = args;
    inst.loc = loc(node);
    
    // Determine calling convention
    bool isVariadic = variadicFunctions_.count(node.name) > 0;
    inst.callConv = (zpCallMode && !isVariadic) ? ir::CallConv::ZP : ir::CallConv::STACK;

    std::vector<ir::Operand> castArgs;
    auto const& pTypes = functionParamTypes_[node.name];
    auto const& pSigned = functionParamSigned_[node.name];

    for (size_t i = 0; i < args.size(); i++) {
        if (i < pTypes.size()) {
            castArgs.push_back(emitCast(args[i], pTypes[i], pSigned[i]));
        } else {
            // Variadic or extra arguments: use default promotions?
            // For now just pass as is.
            castArgs.push_back(args[i]);
        }
    }
    inst.args = castArgs;

    if (node.callExpr) {
        // Indirect call via function pointer
        node.callExpr->accept(*this);
        inst.src1 = lastValue_;
        auto dest = allocVreg(ir::Type::I16);
        inst.op = ir::Op::CALL_INDIRECT;
        inst.callConv = ir::CallConv::STACK; // TODO: support ZP indirect calls
        inst.dest = dest;
        inst.resultType = ir::Type::I16;
        emit(inst);
        lastValue_ = dest;
    } else {
        std::string mangledName = "_" + node.name;
        calledFunctions_.insert(mangledName);
        inst.src1 = ir::Operand::global(mangledName);
        
        ir::Type retType = ir::Type::I16;
        auto it = functionReturnTypes_.find(node.name);
        if (it != functionReturnTypes_.end()) retType = it->second;

        if (retType == ir::Type::VOID) {
            inst.op = ir::Op::CALL_VOID;
            inst.resultType = ir::Type::VOID;
            emit(inst);
        } else {
            auto dest = allocVreg(retType);
            inst.op = ir::Op::CALL;
            inst.dest = dest;
            inst.resultType = retType;
            emit(inst);
            lastValue_ = dest;
        }
    }
}

void IRBuilder::visit(ExpressionStatement& node) {
    if (node.expression) node.expression->accept(*this);
}

// ============================================================================
// Control flow
// ============================================================================

void IRBuilder::visit(ReturnStatement& node) {
    if (node.expression) {
        ir::Type retType = ir::Type::I16;
        if (currentFunc_) retType = currentFunc_->returnType;

        // Optimize: return <integer_literal> → pass immediate directly, no vreg
        if (auto* lit = dynamic_cast<IntegerLiteral*>(node.expression.get())) {
            if (lit->castType.empty()) {
                ir::Inst ret;
                ret.op = ir::Op::RET;
                ret.src1 = ir::Operand::imm(lit->value, retType);
                ret.resultType = retType;
                ret.loc = loc(node);
                emit(ret);
                return;
            }
        }

        node.expression->accept(*this);
        auto val = lastValue_;

        IRTypeInfo info = getExprTypeInfo(node.expression.get());
        auto castVal = emitCast(val, retType, info.isSigned);

        ir::Inst ret;
        ret.op = ir::Op::RET;
        ret.src1 = castVal;
        ret.resultType = retType;
        ret.loc = loc(node);
        emit(ret);
    } else {
        ir::Inst ret;
        ret.op = ir::Op::RET_VOID;
        ret.loc = loc(node);
        emit(ret);
    }
}

void IRBuilder::visit(IfStatement& node) {
    node.condition->accept(*this);
    auto cond = lastValue_;

    std::string thenLabel = newLabel("if_then");
    std::string elseLabel = newLabel("if_else");
    std::string endLabel = newLabel("if_end");

    ir::Inst br;
    br.op = ir::Op::BR_COND;
    br.src1 = cond;
    br.dest = ir::Operand::label(thenLabel);
    br.src2 = ir::Operand::label(node.elseBranch ? elseLabel : endLabel);
    br.loc = loc(node);
    emit(br);

    startBlock(thenLabel);
    node.thenBranch->accept(*this);
    ir::Inst brEnd;
    brEnd.op = ir::Op::BR;
    brEnd.src1 = ir::Operand::label(endLabel);
    emit(brEnd);

    if (node.elseBranch) {
        startBlock(elseLabel);
        node.elseBranch->accept(*this);
        ir::Inst brEnd2;
        brEnd2.op = ir::Op::BR;
        brEnd2.src1 = ir::Operand::label(endLabel);
        emit(brEnd2);
    }

    startBlock(endLabel);
}

void IRBuilder::visit(WhileStatement& node) {
    // Detect while(1) / while(true) — constant true condition
    if (auto* lit = dynamic_cast<IntegerLiteral*>(node.condition.get())) {
        if (lit->value != 0) {
            // Infinite loop: just emit body + unconditional back-branch
            std::string bodyLabel = newLabel("while_body");
            std::string endLabel = newLabel("while_end");
            loopStack_.push_back({endLabel, bodyLabel});

            startBlock(bodyLabel);
            node.body->accept(*this);
            ir::Inst brBack;
            brBack.op = ir::Op::BR;
            brBack.src1 = ir::Operand::label(bodyLabel);
            emit(brBack);

            loopStack_.pop_back();
            startBlock(endLabel);
            return;
        }
    }

    std::string condLabel = newLabel("while_cond");
    std::string bodyLabel = newLabel("while_body");
    std::string endLabel = newLabel("while_end");

    loopStack_.push_back({endLabel, condLabel});

    ir::Inst brCond;
    brCond.op = ir::Op::BR;
    brCond.src1 = ir::Operand::label(condLabel);
    emit(brCond);

    startBlock(condLabel);
    node.condition->accept(*this);
    ir::Inst br;
    br.op = ir::Op::BR_COND;
    br.src1 = lastValue_;
    br.dest = ir::Operand::label(bodyLabel);
    br.src2 = ir::Operand::label(endLabel);
    emit(br);

    startBlock(bodyLabel);
    node.body->accept(*this);
    ir::Inst brBack;
    brBack.op = ir::Op::BR;
    brBack.src1 = ir::Operand::label(condLabel);
    emit(brBack);

    loopStack_.pop_back();
    startBlock(endLabel);
}

void IRBuilder::visit(DoWhileStatement& node) {
    std::string bodyLabel = newLabel("do_body");
    std::string condLabel = newLabel("do_cond");
    std::string endLabel = newLabel("do_end");

    loopStack_.push_back({endLabel, condLabel});

    ir::Inst brBody;
    brBody.op = ir::Op::BR;
    brBody.src1 = ir::Operand::label(bodyLabel);
    emit(brBody);

    startBlock(bodyLabel);
    node.body->accept(*this);

    startBlock(condLabel);
    node.condition->accept(*this);
    ir::Inst br;
    br.op = ir::Op::BR_COND;
    br.src1 = lastValue_;
    br.dest = ir::Operand::label(bodyLabel);
    br.src2 = ir::Operand::label(endLabel);
    emit(br);

    loopStack_.pop_back();
    startBlock(endLabel);
}

void IRBuilder::visit(ForStatement& node) {
    std::string condLabel = newLabel("for_cond");
    std::string bodyLabel = newLabel("for_body");
    std::string incLabel = newLabel("for_inc");
    std::string endLabel = newLabel("for_end");

    loopStack_.push_back({endLabel, incLabel});

    if (node.initializer) node.initializer->accept(*this);

    ir::Inst brCond;
    brCond.op = ir::Op::BR;
    brCond.src1 = ir::Operand::label(condLabel);
    emit(brCond);

    startBlock(condLabel);
    if (node.condition) {
        node.condition->accept(*this);
        ir::Inst br;
        br.op = ir::Op::BR_COND;
        br.src1 = lastValue_;
        br.dest = ir::Operand::label(bodyLabel);
        br.src2 = ir::Operand::label(endLabel);
        emit(br);
    } else {
        ir::Inst br;
        br.op = ir::Op::BR;
        br.src1 = ir::Operand::label(bodyLabel);
        emit(br);
    }

    startBlock(bodyLabel);
    node.body->accept(*this);

    startBlock(incLabel);
    if (node.increment) node.increment->accept(*this);
    ir::Inst brBack;
    brBack.op = ir::Op::BR;
    brBack.src1 = ir::Operand::label(condLabel);
    emit(brBack);

    loopStack_.pop_back();
    startBlock(endLabel);
}

// ============================================================================
// Stub visitors (TODO: implement in future IR phases)
// ============================================================================

void IRBuilder::visit(ConditionalExpression& node) {
    node.condition->accept(*this);
    auto cond = lastValue_;

    std::string thenLabel = newLabel("tern_then");
    std::string elseLabel = newLabel("tern_else");
    std::string endLabel = newLabel("tern_end");

    // Pre-allocate result vReg
    IRTypeInfo info = getExprTypeInfo(&node);
    auto dest = allocVreg(info.type);

    ir::Inst br;
    br.op = ir::Op::BR_COND;
    br.src1 = cond;
    br.dest = ir::Operand::label(thenLabel);
    br.src2 = ir::Operand::label(elseLabel);
    br.loc = loc(node);
    emit(br);
    
    startBlock(thenLabel);
    node.thenExpr->accept(*this);
    auto thenVal = lastValue_;
    
    ir::Inst copyThen;
    copyThen.op = ir::Op::COPY;
    copyThen.dest = dest;
    copyThen.src1 = thenVal;
    copyThen.resultType = dest.type;
    emit(copyThen);

    ir::Inst brEnd1;
    brEnd1.op = ir::Op::BR;
    brEnd1.src1 = ir::Operand::label(endLabel);
    emit(brEnd1);

    startBlock(elseLabel);
    node.elseExpr->accept(*this);
    auto elseVal = lastValue_;
    
    ir::Inst copyElse;
    copyElse.op = ir::Op::COPY;
    copyElse.dest = dest;
    copyElse.src1 = elseVal;
    copyElse.resultType = dest.type;
    emit(copyElse);

    ir::Inst brEnd2;
    brEnd2.op = ir::Op::BR;
    brEnd2.src1 = ir::Operand::label(endLabel);
    emit(brEnd2);

    startBlock(endLabel);
    lastValue_ = dest;
}

void IRBuilder::visit(GenericSelection& node) {
    // Evaluate the controlling expression (default association used at IR level)
    if (node.control) node.control->accept(*this);
}

void IRBuilder::visit(InitializerList& node) {
    // Emit each element; last value is the last element
    for (auto& elem : node.elements) {
        elem->accept(*this);
    }
}

void IRBuilder::visit(ArrayAccess& node) {
    bool oldAddrMode = computeAddressOnly_;
    computeAddressOnly_ = false;

    // Evaluate base address
    if (auto* ref = dynamic_cast<VariableReference*>(node.arrayExpr.get())) {
        auto ait = localArrayDims_.find(ref->name);
        if (ait != localArrayDims_.end() && !ait->second.empty()) {
            // Local array — compute address via ADDR_LOCAL
            auto it = locals_.find(ref->name);
            if (it != locals_.end()) {
                auto dest = allocVreg(ir::Type::PTR);
                ir::Inst addrInst;
                addrInst.op = ir::Op::ADDR_LOCAL;
                addrInst.dest = dest;
                addrInst.resultType = ir::Type::PTR;
                addrInst.src1 = ir::Operand::vreg(it->second.vregId, ir::Type::PTR);
                addrInst.loc = loc(node);
                emit(addrInst);
                lastValue_ = dest;
            }
        } else {
            // Pointer variable or global array — value IS the address
            node.arrayExpr->accept(*this);
        }
    } else {
        node.arrayExpr->accept(*this);
    }
    auto base = lastValue_;

    // Evaluate index
    node.indexExpr->accept(*this);
    auto index = lastValue_;
    
    computeAddressOnly_ = oldAddrMode;


    // Determine element size from array type (handling multi-dimensional arrays)
    int elemSize = 2; // default
    Expression* root = node.arrayExpr.get();
    int depth = 1;
    while (auto* inner = dynamic_cast<ArrayAccess*>(root)) {
        root = inner->arrayExpr.get();
        depth++;
    }

    if (auto* ref = dynamic_cast<VariableReference*>(root)) {
        std::vector<int> dims;
        ir::Type baseType = ir::Type::I16;
        
        auto pit = localPointedToType_.find(ref->name);
        if (pit != localPointedToType_.end()) {
            baseType = pit->second;
        } else {
            auto gpit = globalPointedToType_.find(ref->name);
            if (gpit != globalPointedToType_.end()) baseType = gpit->second;
            else {
                auto tit = localTypes_.find(ref->name);
                if (tit != localTypes_.end()) baseType = tit->second;
                else {
                    auto gtit = globalTypes_.find(ref->name);
                    if (gtit != globalTypes_.end()) baseType = gtit->second;
                }
            }
        }

        auto ait = localArrayDims_.find(ref->name);
        if (ait != localArrayDims_.end()) dims = ait->second;
        else {
            auto gait = globalArrayDims_.find(ref->name);
            if (gait != globalArrayDims_.end()) dims = gait->second;
        }

        if (!dims.empty() && depth <= (int)dims.size()) {
            std::string tn;
            if (localTypeNames_.count(ref->name)) tn = localTypeNames_[ref->name];
            else tn = globalTypeNames_[ref->name];

            int stride = getTypeSize(tn, 0);
            for (size_t i = depth; i < dims.size(); i++) {
                stride *= dims[i];
            }
            elemSize = stride;
        } else {
            elemSize = ir::typeSize(baseType);
        }
    }

    // addr = base + index * elemSize
    auto addr = allocVreg(ir::Type::PTR);
    ir::Inst inst;
    inst.op = ir::Op::ADDR_ELEM;
    inst.dest = addr;
    inst.resultType = ir::Type::PTR;
    inst.src1 = base;
    inst.src2 = index;
    inst.args.push_back(ir::Operand::imm(elemSize, ir::Type::I16));
    inst.loc = loc(node);
    emit(inst);

    // Determine if we need to LOAD the final value or just use the sub-array address
    bool isFinal = true;
    if (auto* ref = dynamic_cast<VariableReference*>(root)) {
        std::vector<int> dims;
        auto ait = localArrayDims_.find(ref->name);
        if (ait != localArrayDims_.end()) dims = ait->second;
        else {
            auto git = globalArrayDims_.find(ref->name);
            if (git != globalArrayDims_.end()) dims = git->second;
        }
        if (depth < (int)dims.size()) isFinal = false;
    }

    if (isFinal) {
        if (computeAddressOnly_) {
            lastValue_ = addr;
            return;
        }

        // Final element reached: load it
        IRTypeInfo info = getExprTypeInfo(&node);
        ir::Type finalType = info.type;
        
        auto val = allocVreg(finalType);
        ir::Inst load;
        load.op = ir::Op::LOAD;
        load.dest = val;
        load.resultType = finalType;
        load.src1 = addr;
        load.loc = loc(node);
        emit(load);
        lastValue_ = val;
    } else {
        // Intermediate sub-array address: no LOAD needed, address is the value
        lastValue_ = addr;
    }
}

void IRBuilder::visit(MemberAccess& node) {
    // For p->member: load the pointer value (not address-of)
    // For s.member: compute address of the struct variable
    bool oldAddrMode = computeAddressOnly_;
    computeAddressOnly_ = !node.isArrow;
    node.structExpr->accept(*this);
    auto base = lastValue_;
    computeAddressOnly_ = oldAddrMode;

    // Look up member info
    int memberOffset = 0;
    int bitWidth = 0;
    int bitOffset = 0;

    IRTypeInfo baseInfo = getExprTypeInfo(node.structExpr.get());
    std::string sName = getAggregateName(baseInfo.typeName);

    auto sit = structs_.find(sName);
    if (sit != structs_.end()) {
        auto mit = sit->second.members.find(node.memberName);
        if (mit != sit->second.members.end()) {
            memberOffset = mit->second.offset;
            bitWidth = mit->second.bitWidth;
            bitOffset = mit->second.bitOffset;
        }
    } else {
        // Fallback: search all structs (fragile but handles some cases)
        for (const auto& [sn, sinfo] : structs_) {
            auto mit = sinfo.members.find(node.memberName);
            if (mit != sinfo.members.end()) {
                memberOffset = mit->second.offset;
                bitWidth = mit->second.bitWidth;
                bitOffset = mit->second.bitOffset;
                break;
            }
        }
    }

    // Compute address of the storage unit containing the member
    auto addr = allocVreg(ir::Type::PTR);
    ir::Inst add;
    add.op = ir::Op::ADD;
    add.dest = addr;
    add.resultType = ir::Type::PTR;
    add.src1 = base;
    add.src2 = ir::Operand::imm(memberOffset, ir::Type::I16);
    add.loc = loc(node);
    emit(add);

    if (computeAddressOnly_) {
        lastValue_ = addr;
        return;
    }

    // Load the storage unit
    IRTypeInfo info = getExprTypeInfo(&node);
    ir::Type loadType = info.type;

    auto val = allocVreg(loadType);
    ir::Inst load;
    load.op = ir::Op::LOAD;
    load.dest = val;
    load.resultType = loadType;
    load.src1 = addr;
    load.loc = loc(node);
    emit(load);

    if (bitWidth > 0) {
        // Bitfield: extract the field from the loaded storage unit
        auto extracted = allocVreg(loadType);
        ir::Inst bfext;
        bfext.op = ir::Op::BFEXT;
        bfext.dest = extracted;
        bfext.resultType = loadType;
        bfext.src1 = val;
        bfext.args.push_back(ir::Operand::imm(bitOffset, ir::Type::I8));
        bfext.args.push_back(ir::Operand::imm(bitWidth, ir::Type::I8));
        bfext.loc = loc(node);
        emit(bfext);
        lastValue_ = extracted;
    } else {
        lastValue_ = val;
    }
}

void IRBuilder::visit(CompoundLiteral& node) {
    ir::Type t = mapType(node.targetType, node.pointerLevel);
    int size = getTypeSize(node.targetType, node.pointerLevel);
    if (!node.arrayDims.empty()) {
        int total = 1;
        for (int d : node.arrayDims) total *= d;
        size *= total;
    }

    if (node.initializer && node.initializer->elements.size() == 1 && node.arrayDims.empty()) {
        // Scalar compound literal: just evaluate the value
        node.initializer->elements[0]->accept(*this);
    } else {
        // Aggregate: allocate a frame buffer with the actual struct/array size,
        // then a separate PTR vReg for the address (so ADDR_LOCAL doesn't overwrite the buffer)
        auto buf = allocVreg(t);
        if (currentFunc_) {
            currentFunc_->memoryVregs.insert(buf.vregId);
            currentFunc_->vregSizes[buf.vregId] = size;
        }
        auto temp = allocVreg(ir::Type::PTR);
        ir::Inst alloc;
        alloc.op = ir::Op::ADDR_LOCAL;
        alloc.dest = temp;
        alloc.resultType = ir::Type::PTR;
        alloc.src1 = ir::Operand::vreg(buf.vregId, ir::Type::PTR);
        alloc.loc = loc(node);
        emit(alloc);

        if (node.initializer) {
            std::string sName = getAggregateName(node.targetType);
            bool isStruct = structs_.count(sName) > 0;
            if (isStruct) {
                // Store each member at its correct offset using struct layout
                auto& si = structs_[sName];
                for (size_t i = 0; i < node.initializer->elements.size() && i < si.memberOrder.size(); i++) {
                    node.initializer->elements[i]->accept(*this);
                    auto val = lastValue_;
                    auto& mi = si.members[si.memberOrder[i]];
                    ir::Type mt = mapType(mi.type, mi.pointerLevel);
                    auto addr = allocVreg(ir::Type::PTR);
                    ir::Inst add;
                    add.op = ir::Op::ADD;
                    add.dest = addr;
                    add.resultType = ir::Type::PTR;
                    add.src1 = temp;
                    add.src2 = ir::Operand::imm(mi.offset, ir::Type::I16);
                    emit(add);
                    ir::Inst store;
                    store.op = ir::Op::STORE;
                    store.resultType = mt;
                    store.src1 = val;
                    store.src2 = addr;
                    emit(store);
                }
            } else {
                // Array or non-struct aggregate
                int elemOff = 0;
                int elemSize = ir::typeSize(t);
                for (auto& elem : node.initializer->elements) {
                    elem->accept(*this);
                    auto val = lastValue_;
                    auto addr = allocVreg(ir::Type::PTR);
                    ir::Inst add;
                    add.op = ir::Op::ADD;
                    add.dest = addr;
                    add.resultType = ir::Type::PTR;
                    add.src1 = temp;
                    add.src2 = ir::Operand::imm(elemOff, ir::Type::I16);
                    emit(add);
                    ir::Inst store;
                    store.op = ir::Op::STORE;
                    store.resultType = t;
                    store.src1 = val;
                    store.src2 = addr;
                    emit(store);
                    elemOff += elemSize;
                }
            }
        }
        lastValue_ = temp;
    }
}

void IRBuilder::visit(AlignofExpression&) {
    lastValue_ = ir::Operand::imm(1, ir::Type::I16);
}

void IRBuilder::visit(SizeofExpression& node) {
    int sz = 2;
    if (node.isType) {
        sz = getTypeSize(node.typeName, node.pointerLevel);
    } else if (node.expression) {
        // sizeof(expr) — evaluate type of expression
        IRTypeInfo info = getExprTypeInfo(node.expression.get());
        sz = info.totalSize;
    }
    auto dest = allocVreg(ir::Type::I16);
    ir::Inst inst;
    inst.op = ir::Op::CONST;
    inst.dest = dest;
    inst.resultType = ir::Type::I16;
    inst.src1 = ir::Operand::imm(sz, ir::Type::I16);
    inst.loc = loc(node);
    emit(inst);
    lastValue_ = dest;
}

class CaseCollector : public ASTVisitor {
public:
    IRBuilder::SwitchCtx& ctx;
    IRBuilder& builder;
    CaseCollector(IRBuilder::SwitchCtx& c, IRBuilder& b) : ctx(c), builder(b) {}

    void visit(IntegerLiteral&) override {}
    void visit(StringLiteral&) override {}
    void visit(VariableReference&) override {}
    void visit(Assignment& node) override { if (node.expression) node.expression->accept(*this); }
    void visit(BinaryOperation& node) override { if (node.left) node.left->accept(*this); if (node.right) node.right->accept(*this); }
    void visit(UnaryOperation& node) override { if (node.operand) node.operand->accept(*this); }
    void visit(FunctionCall& node) override { for (auto& arg : node.arguments) arg->accept(*this); }
    void visit(BuiltinVaStart& node) override { if (node.ap) node.ap->accept(*this); }
    void visit(BuiltinVaArg& node) override { if (node.ap) node.ap->accept(*this); }
    void visit(CpuRegisterAccess&) override {}
    void visit(CpuFlagAccess&) override {}
    void visit(MemberAccess& node) override { if (node.structExpr) node.structExpr->accept(*this); }
    void visit(ConditionalExpression& node) override {
        if (node.condition) node.condition->accept(*this);
        if (node.thenExpr) node.thenExpr->accept(*this);
        if (node.elseExpr) node.elseExpr->accept(*this);
    }
    void visit(GenericSelection& node) override {
        if (node.control) node.control->accept(*this);
        for (auto& assoc : node.associations) if (assoc.result) assoc.result->accept(*this);
    }
    void visit(InitializerList& node) override { for (auto& elem : node.elements) if (elem) elem->accept(*this); }
    void visit(ArrayAccess& node) override { if (node.arrayExpr) node.arrayExpr->accept(*this); if (node.indexExpr) node.indexExpr->accept(*this); }
    void visit(CastExpression& node) override { if (node.expression) node.expression->accept(*this); }
    void visit(CompoundLiteral& node) override { if (node.initializer) node.initializer->accept(*this); }
    void visit(AlignofExpression&) override {}
    void visit(SizeofExpression& node) override { if (!node.isType && node.expression) node.expression->accept(*this); }
    void visit(VariableDeclaration& node) override { if (node.initializer) node.initializer->accept(*this); }
    void visit(ReturnStatement& node) override { if (node.expression) node.expression->accept(*this); }
    void visit(BreakStatement&) override {}
    void visit(ContinueStatement&) override {}
    void visit(SwitchContinueStatement& node) override { if (node.target) node.target->accept(*this); }
    void visit(GotoStatement&) override {}
    void visit(LabelledStatement& node) override { if (node.statement) node.statement->accept(*this); }
    void visit(ExpressionStatement& node) override { if (node.expression) node.expression->accept(*this); }
    void visit(IfStatement& node) override {
        if (node.condition) node.condition->accept(*this);
        if (node.thenBranch) node.thenBranch->accept(*this);
        if (node.elseBranch) node.elseBranch->accept(*this);
    }
    void visit(WhileStatement& node) override {
        if (node.condition) node.condition->accept(*this);
        if (node.body) node.body->accept(*this);
    }
    void visit(DoWhileStatement& node) override {
        if (node.body) node.body->accept(*this);
        if (node.condition) node.condition->accept(*this);
    }
    void visit(ForStatement& node) override {
        if (node.initializer) node.initializer->accept(*this);
        if (node.condition) node.condition->accept(*this);
        if (node.increment) node.increment->accept(*this);
        if (node.body) node.body->accept(*this);
    }
    void visit(SwitchStatement& node) override {
        if (node.expression) node.expression->accept(*this);
        // Do NOT visit body to avoid collecting cases from nested switches
    }
    void visit(CaseStatement& node) override {
        int64_t val = 0;
        if (auto* lit = dynamic_cast<IntegerLiteral*>(node.value.get())) val = lit->value;
        int64_t endVal = val;
        bool isRange = false;
        if (node.rangeEnd) {
            if (auto* lit2 = dynamic_cast<IntegerLiteral*>(node.rangeEnd.get())) endVal = lit2->value;
            isRange = true;
        }
        node.label = builder.newLabel("case");
        ctx.cases.push_back({val, endVal, isRange, node.label});
    }
    void visit(DefaultStatement& node) override {
        node.label = builder.newLabel("default");
        ctx.defaultLabel = node.label;
        ctx.hasDefault = true;
    }
    void visit(AsmStatement&) override {}
    void visit(StaticAssert&) override {}
    void visit(EnumDefinition&) override {}
    void visit(StructDefinition&) override {}
    void visit(CompoundStatement& node) override { for (auto& s : node.statements) if (s) s->accept(*this); }
    void visit(FunctionDeclaration&) override {}
    void visit(TranslationUnit&) override {}
};

void IRBuilder::visit(SwitchStatement& node) {
    node.expression->accept(*this);
    auto expr = lastValue_;

    std::string breakLabel = newLabel("switch_end");
    std::string defaultLabel = breakLabel; // default falls through to break if no default case

    SwitchCtx ctx;
    ctx.breakLabel = breakLabel;
    ctx.defaultLabel = defaultLabel;
    ctx.expr = expr;
    
    // First pass: collect all cases and default labels
    CaseCollector collector(ctx, *this);
    node.body->accept(collector);

    // Emit comparison dispatch logic
    for (auto const& c : ctx.cases) {
        if (c.isRange) {
            // Range case: expr >= minVal && expr <= maxVal
            std::string nextCmpLabel = newLabel("case_skip");

            // Check expr >= minVal (unsigned)
            auto geDest = allocVreg(ir::Type::I8);
            ir::Inst cmpGe;
            cmpGe.op = ir::Op::CMP_GEU;
            cmpGe.dest = geDest;
            cmpGe.resultType = ir::Type::I8;
            cmpGe.src1 = expr;
            cmpGe.src2 = ir::Operand::imm(c.minVal, expr.type);
            cmpGe.loc = loc(node);
            emit(cmpGe);

            std::string checkUpperLabel = newLabel("range_upper");
            ir::Inst brGe;
            brGe.op = ir::Op::BR_COND;
            brGe.src1 = geDest;
            brGe.dest = ir::Operand::label(checkUpperLabel);
            brGe.src2 = ir::Operand::label(nextCmpLabel);
            emit(brGe);
            startBlock(checkUpperLabel);

            // Check expr <= maxVal (unsigned)
            auto leDest = allocVreg(ir::Type::I8);
            ir::Inst cmpLe;
            cmpLe.op = ir::Op::CMP_LEU;
            cmpLe.dest = leDest;
            cmpLe.resultType = ir::Type::I8;
            cmpLe.src1 = expr;
            cmpLe.src2 = ir::Operand::imm(c.maxVal, expr.type);
            cmpLe.loc = loc(node);
            emit(cmpLe);

            ir::Inst brLe;
            brLe.op = ir::Op::BR_COND;
            brLe.src1 = leDest;
            brLe.dest = ir::Operand::label(c.label);
            brLe.src2 = ir::Operand::label(nextCmpLabel);
            emit(brLe);
            startBlock(nextCmpLabel);
        } else {
            // Single-value case
            auto dest = allocVreg(ir::Type::I8);
            ir::Inst cmp;
            cmp.op = ir::Op::CMP_EQ;
            cmp.dest = dest;
            cmp.resultType = ir::Type::I8;
            cmp.src1 = expr;
            cmp.src2 = ir::Operand::imm(c.minVal, expr.type);
            cmp.loc = loc(node);
            emit(cmp);

            std::string nextCmpLabel = newLabel("case_skip");
            ir::Inst br;
            br.op = ir::Op::BR_COND;
            br.src1 = dest;
            br.dest = ir::Operand::label(c.label);
            br.src2 = ir::Operand::label(nextCmpLabel);
            emit(br);
            startBlock(nextCmpLabel);
        }
    }

    // Branch to default (or break if no default)
    ir::Inst brDef;
    brDef.op = ir::Op::BR;
    brDef.src1 = ir::Operand::label(ctx.defaultLabel);
    emit(brDef);

    // Second pass: visit body to emit case blocks
    switchStack_.push_back(ctx);
    std::string outerContinue = loopStack_.empty() ? "" : loopStack_.back().continueLabel;
    loopStack_.push_back({breakLabel, outerContinue});

    node.body->accept(*this);

    loopStack_.pop_back();
    switchStack_.pop_back();
    
    // Ensure we end at the break label
    if (!currentBlock_ || currentBlock_->insts.empty() || currentBlock_->insts.back().op != ir::Op::BR) {
        ir::Inst brEnd;
        brEnd.op = ir::Op::BR;
        brEnd.dest = ir::Operand::label(breakLabel);
        emit(brEnd);
    }
    startBlock(breakLabel);
}

void IRBuilder::visit(CaseStatement& node) {
    if (switchStack_.empty()) return;
    
    // Start the block for this case
    if (node.label.empty()) {
        node.label = newLabel("case"); // should have been set by collector
    }
    startBlock(node.label);
}

void IRBuilder::visit(DefaultStatement& node) {
    if (switchStack_.empty()) return;

    if (node.label.empty()) {
        node.label = newLabel("default"); // should have been set by collector
    }
    startBlock(node.label);
}

void IRBuilder::visit(BreakStatement&) {
    if (loopStack_.empty()) return;
    ir::Inst br;
    br.op = ir::Op::BR;
    br.src1 = ir::Operand::label(loopStack_.back().breakLabel);
    emit(br);
}

void IRBuilder::visit(ContinueStatement&) {
    if (loopStack_.empty() || loopStack_.back().continueLabel.empty()) return;
    ir::Inst br;
    br.op = ir::Op::BR;
    br.src1 = ir::Operand::label(loopStack_.back().continueLabel);
    emit(br);
}

void IRBuilder::visit(SwitchContinueStatement&) {
    // Non-standard extension — skip for now
}

void IRBuilder::visit(GotoStatement& node) {
    ir::Inst br;
    br.op = ir::Op::BR;
    br.src1 = ir::Operand::label(node.label);
    br.loc = loc(node);
    emit(br);
}

void IRBuilder::visit(LabelledStatement& node) {
    startBlock(node.label);
    if (node.statement) node.statement->accept(*this);
}

void IRBuilder::visit(AsmStatement& node) {
    if (node.code == ".no_zp_save") {
        module_.saveZP = false;
        return;
    }
    // Scan inline asm for parameter/local references (@_p_name, @_l_name)
    // to suppress unused-variable warnings
    for (const auto& [name, vregOp] : locals_) {
        std::string pRef = "@_p_" + name;
        std::string lRef = "@_l_" + name;
        if (node.code.find(pRef) != std::string::npos ||
            node.code.find(lRef) != std::string::npos) {
            usedVregs_.insert(vregOp.vregId);
        }
    }
    ir::Inst inst;
    inst.op = ir::Op::ASM_INLINE;
    inst.asmText = node.code;
    inst.loc = loc(node);
    emit(inst);
}

void IRBuilder::visit(StaticAssert&) { /* compile-time only */ }

void IRBuilder::visit(EnumDefinition&) { /* type-level only, values folded by ConstantFolder */ }

void IRBuilder::visit(StructDefinition& node) {
    IRStructInfo info;
    info.name = node.name;
    info.isUnion = node.isUnion;
    int currentOffset = 0;

    // Bitfield packing state
    std::string bfUnitType;
    int bfUnitOffset = 0;
    int bfBitsUsed = 0;
    int bfUnitSize = 0;

    auto closeBitfieldUnit = [&]() {
        if (!bfUnitType.empty()) {
            currentOffset = bfUnitOffset + bfUnitSize;
            bfUnitType.clear();
            bfBitsUsed = 0;
            bfUnitSize = 0;
        }
    };

    for (const auto& m : node.members) {
        IRMemberInfo mi;
        mi.type = m.type;
        mi.pointerLevel = m.pointerLevel;
        mi.isSigned = m.isSigned;
        mi.isConst = m.isConst;
        mi.arrayDims = m.arrayDims;
        mi.bitWidth = m.bitWidth;

        if (m.bitWidth > 0) {
            // Bitfield member
            int unitSize = getTypeSize(m.type, 0);
            int unitBits = unitSize * 8;

            // Start new unit if type changes or bits don't fit
            if (bfUnitType != m.type || bfBitsUsed + m.bitWidth > unitBits) {
                closeBitfieldUnit();
                // Align to unit size (only if __unpacked)
                int align = node.isUnpacked ? std::max(1, unitSize) : 1;
                if (!node.isUnion && align > 1 && currentOffset % align != 0)
                    currentOffset += align - (currentOffset % align);
                bfUnitType = m.type;
                bfUnitOffset = node.isUnion ? 0 : currentOffset;
                bfBitsUsed = 0;
                bfUnitSize = unitSize;
            }

            mi.offset = bfUnitOffset;
            mi.size = unitSize;
            mi.bitOffset = bfBitsUsed;
            bfBitsUsed += m.bitWidth;
        } else {
            // Regular member — close any open bitfield unit first
            closeBitfieldUnit();

            mi.size = getTypeSize(m.type, m.pointerLevel);
            if (m.arraySize() > 0) mi.size *= m.arraySize();

            if (node.isUnion) {
                mi.offset = 0;
            } else {
                // Packed by default — only align if __unpacked
                int align = node.isUnpacked ? std::max(1, getTypeSize(m.type, m.pointerLevel)) : 1;
                if (align > 1 && currentOffset % align != 0)
                    currentOffset += align - (currentOffset % align);
                mi.offset = currentOffset;
                currentOffset += mi.size;
            }
        }

        info.members[m.name] = mi;
        info.memberOrder.push_back(m.name);
    }

    closeBitfieldUnit();

    info.totalSize = node.isUnion ? 0 : currentOffset;
    if (node.isUnion) {
        for (const auto& [n, mi] : info.members)
            if (mi.size > info.totalSize) info.totalSize = mi.size;
    }
    structs_[node.name] = info;
}
void IRBuilder::visit(BuiltinVaStart& node) {
    // va_start(ap, last_param): compute address past last named param
    // At IR level: ap = addr_local(last_param) + sizeof(last_param)
    node.ap->accept(*this);
    auto apAddr = lastValue_;

    // Compute ap = address on stack past the last named parameter.
    // Variadic args follow the named params on the stack. Use inline asm
    // to compute the stack address via leax.fp @_p_<lastParam>, then add sizeof.
    auto dest = allocVreg(ir::Type::PTR);
    {
        ir::Inst inst;
        inst.op = ir::Op::VA_START;
        inst.dest = dest;
        inst.resultType = ir::Type::PTR;
        inst.asmText = node.lastParamName;
        inst.loc = loc(node);
        emit(inst);
    }

    // Store into ap
    ir::Inst store;
    store.op = ir::Op::STORE;
    store.resultType = ir::Type::PTR;
    store.src1 = dest;
    store.src2 = apAddr;
    store.loc = loc(node);
    emit(store);
    lastValue_ = dest;
}

void IRBuilder::visit(BuiltinVaArg& node) {
    // va_arg(ap, type): load value from *ap, then advance ap by sizeof(type)
    node.ap->accept(*this);
    auto apAddr = lastValue_;

    ir::Type argType = mapType(node.typeName, node.pointerLevel);
    int argSize = ir::typeSize(argType);
    if (argSize < 2) argSize = 2; // default argument promotion

    // Load the argument from *ap (ap is a ZP pointer pair — single dereference)
    auto val = allocVreg(argType);
    ir::Inst loadVal;
    loadVal.op = ir::Op::LOAD;
    loadVal.dest = val;
    loadVal.resultType = argType;
    loadVal.src1 = apAddr;
    loadVal.loc = loc(node);
    emit(loadVal);

    // Advance ap: ap += argSize
    auto newAp = allocVreg(ir::Type::PTR);
    ir::Inst advance;
    advance.op = ir::Op::ADD;
    advance.dest = newAp;
    advance.resultType = ir::Type::PTR;
    advance.src1 = apAddr;
    advance.src2 = ir::Operand::imm(argSize, ir::Type::I16);
    advance.loc = loc(node);
    emit(advance);

    // Store back
    ir::Inst storeAp;
    storeAp.op = ir::Op::STORE;
    storeAp.resultType = ir::Type::PTR;
    storeAp.src1 = newAp;
    storeAp.src2 = apAddr;
    emit(storeAp);

    lastValue_ = val;
}

void IRBuilder::visit(CpuRegisterAccess& node) {
    ir::Type t = ir::Type::I8;
    std::string reg = node.regName;
    std::transform(reg.begin(), reg.end(), reg.begin(), ::toupper);
    if (reg == "AX" || reg == "AY" || reg == "AZ" || reg == "XY" || reg == "SP") t = ir::Type::I16;
    else if (reg == "Q" || reg == "AXYZ") t = ir::Type::I32;

    auto dest = allocVreg(t);
    ir::Inst inst;
    inst.op = ir::Op::CPU_REG_READ;
    inst.dest = dest;
    inst.resultType = t;
    inst.asmText = node.regName;
    inst.loc = loc(node);
    emit(inst);
    lastValue_ = dest;
    lastValueSigned_ = false; // Registers are unsigned
}

void IRBuilder::visit(CpuFlagAccess& node) {
    auto dest = allocVreg(ir::Type::I8); // Flags are 1-bit, use i8 boolean
    ir::Inst inst;
    inst.op = ir::Op::CPU_FLAG_READ;
    inst.dest = dest;
    inst.resultType = ir::Type::I8;
    inst.asmText = node.flagName;
    inst.loc = loc(node);
    emit(inst);
    lastValue_ = dest;
    lastValueSigned_ = false;
}

