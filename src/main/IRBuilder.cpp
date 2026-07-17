#include "IRBuilder.hpp"
#include "Diagnostic.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>

IRBuilder::IRBuilder() {}

void IRBuilder::setSourceInfo(const std::string& filename) {
    module_.sourceFile = filename;
}

void IRBuilder::generate(TranslationUnit& unit) {
    unit.accept(*this);
    // Eliminate unused static functions (iterative — handles call chains)
    {
        bool changed = true;
        while (changed) {
            // Rebuild called set from surviving functions only
            std::set<std::string> usedFuncs;
            for (const auto& fn : module_.functions) {
                for (const auto& blk : fn.blocks) {
                    for (const auto& inst : blk.insts) {
                        if ((inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID) &&
                            inst.src1.kind == ir::OperandKind::GLOBAL) {
                            usedFuncs.insert(inst.src1.name);
                        }
                        // Function address taken (function pointer) via ADDR_GLOBAL
                        if (inst.op == ir::Op::ADDR_GLOBAL &&
                            inst.src1.kind == ir::OperandKind::GLOBAL) {
                            usedFuncs.insert(inst.src1.name);
                        }
                        // Function name used as value (global operand in args, src1, src2)
                        if (inst.src1.kind == ir::OperandKind::GLOBAL) usedFuncs.insert(inst.src1.name);
                        if (inst.src2.kind == ir::OperandKind::GLOBAL) usedFuncs.insert(inst.src2.name);
                        for (const auto& arg : inst.args) {
                            if (arg.kind == ir::OperandKind::GLOBAL) usedFuncs.insert(arg.name);
                        }
                    }
                }
            }
            // Phase 3: vtable-referenced functions are always used
            for (const auto& g : module_.globals) {
                for (const auto& methodName : g.vtableMethodNames) {
                    if (!methodName.empty()) usedFuncs.insert(methodName);
                }
            }
            auto it = std::remove_if(module_.functions.begin(), module_.functions.end(),
                [&usedFuncs](const ir::Function& fn) {
                    return fn.isStatic && fn.name != "_main" &&
                           !usedFuncs.count(fn.name);
                });
            changed = (it != module_.functions.end());
            module_.functions.erase(it, module_.functions.end());
        }
    }
    // Phase 4: Compiler-level devirtualization
    // For each vtable slot, if only one implementation exists across all vtables,
    // replace virtual CALL_INDIRECT with direct CALL.
    {
        // Build slot → set of implementations
        std::map<int, std::set<std::string>> slotImpls;
        for (const auto& g : module_.globals) {
            for (int i = 0; i < (int)g.vtableMethodNames.size(); i++) {
                if (!g.vtableMethodNames[i].empty()) {
                    slotImpls[i].insert(g.vtableMethodNames[i]);
                }
            }
        }

        // Find slots with exactly one implementation
        std::map<int, std::string> devirtTargets;
        for (const auto& [slot, impls] : slotImpls) {
            if (impls.size() == 1) {
                devirtTargets[slot] = *impls.begin();
            }
        }

        // Scan all functions for virtual call pattern:
        // LOAD (vtable ptr) → ADD (slot offset) → LOAD (func ptr) → CALL_INDIRECT
        // Replace with direct CALL if slot is devirtualizable
        if (!devirtTargets.empty()) {
            int devirtCount = 0;
            for (auto& fn : module_.functions) {
                for (auto& blk : fn.blocks) {
                    for (size_t i = 0; i < blk.insts.size(); i++) {
                        auto& inst = blk.insts[i];
                        if (inst.op == ir::Op::CALL_INDIRECT && i >= 3) {
                            // Check if preceded by vtable dispatch pattern
                            auto& loadFp = blk.insts[i-1]; // LOAD func ptr
                            auto& addSlot = blk.insts[i-2]; // ADD slot offset
                            if (loadFp.op == ir::Op::LOAD && addSlot.op == ir::Op::ADD &&
                                addSlot.src2.kind == ir::OperandKind::IMM) {
                                int slotOffset = (int)addSlot.src2.immVal;
                                int slot = slotOffset / 2;
                                auto dit = devirtTargets.find(slot);
                                if (dit != devirtTargets.end()) {
                                    // Devirtualize: replace CALL_INDIRECT with direct CALL
                                    inst.op = ir::Op::CALL;
                                    inst.src1 = ir::Operand::global(dit->second);
                                    devirtCount++;
                                }
                            }
                        }
                    }
                }
            }
            if (devirtCount > 0) {
                warnings_.push_back(formatDiagnostic(module_.sourceFile, 0, 0, Severity::Note,
                    "devirtualized " + std::to_string(devirtCount) + " virtual call(s)"));
            }
        }
    }

    // Parameter narrowing analysis: detect static functions where all call sites
    // pass values fitting in char for int parameters. Emit advisory warning.
    {
        // Build map of function name → param types
        std::map<std::string, std::vector<ir::Type>> funcParams;
        std::map<std::string, int> funcDeclLines;
        for (const auto& fn : module_.functions) {
            if (fn.isStatic && fn.name != "_main") {
                funcParams[fn.name] = fn.paramTypes;
                funcDeclLines[fn.name] = fn.declLine;
            }
        }
        // Build CONST vreg map for each function (to resolve CONST vreg args)
        std::map<std::string, std::vector<int64_t>> paramMaxVals;
        std::map<std::string, std::vector<bool>> paramAllConst;   // all values 0-255
        std::map<std::string, std::vector<bool>> paramAllNonNeg;  // all values >= 0
        for (const auto& fn : module_.functions) {
            // Collect CONST definitions in this function
            std::map<uint32_t, int64_t> constDefs;
            for (const auto& blk : fn.blocks) {
                for (const auto& inst : blk.insts) {
                    if (inst.op == ir::Op::CONST && inst.dest.isVreg())
                        constDefs[inst.dest.vregId] = inst.src1.immVal;
                }
            }
            // Check call sites
            for (const auto& blk : fn.blocks) {
                for (const auto& inst : blk.insts) {
                    if ((inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID) &&
                        inst.src1.kind == ir::OperandKind::GLOBAL &&
                        funcParams.count(inst.src1.name)) {
                        auto& maxVals = paramMaxVals[inst.src1.name];
                        auto& allConst = paramAllConst[inst.src1.name];
                        auto& allNonNeg = paramAllNonNeg[inst.src1.name];
                        if (maxVals.empty()) {
                            maxVals.resize(inst.args.size(), 0);
                            allConst.resize(inst.args.size(), true);
                            allNonNeg.resize(inst.args.size(), true);
                        }
                        for (size_t i = 0; i < inst.args.size() && i < maxVals.size(); i++) {
                            int64_t v = -1;
                            bool known = false;
                            if (inst.args[i].isImm()) {
                                v = inst.args[i].immVal;
                                known = true;
                            } else if (inst.args[i].isVreg()) {
                                auto cit = constDefs.find(inst.args[i].vregId);
                                if (cit != constDefs.end()) { v = cit->second; known = true; }
                            }
                            if (known && v >= 0 && v <= 255) {
                                if (v > maxVals[i]) maxVals[i] = v;
                            } else if (known && v >= 0) {
                                allConst[i] = false;
                                if (v > maxVals[i]) maxVals[i] = v;
                            } else {
                                allConst[i] = false;
                                allNonNeg[i] = false;
                            }
                        }
                    }
                }
            }
        }
        // Emit warnings for narrowable parameters
        for (const auto& [name, params] : funcParams) {
            auto acIt = paramAllConst.find(name);
            if (acIt == paramAllConst.end()) continue;
            const auto& allConst = acIt->second;
            auto nnIt = paramAllNonNeg.find(name);
            const auto* allNonNegPtr = (nnIt != paramAllNonNeg.end()) ? &nnIt->second : nullptr;
            for (size_t i = 0; i < params.size() && i < allConst.size(); i++) {
                std::string cleanName = (name.size() > 1 && name[0] == '_') ? name.substr(1) : name;
                if (params[i] == ir::Type::I16 && allConst[i]) {
                    // All values fit in unsigned char
                    std::stringstream ss;
                    ss << "parameter " << (i + 1) << " of '" << cleanName
                       << "' could be 'unsigned char' (all call sites pass 0-255)";
                    warnings_.push_back(formatDiagnostic(module_.sourceFile, funcDeclLines[name], 0,
                        Severity::Note, ss.str()));
                } else if (params[i] == ir::Type::I16 && !allConst[i] &&
                           allNonNegPtr && i < allNonNegPtr->size() && (*allNonNegPtr)[i]) {
                    // All values non-negative but some > 255 — suggest unsigned int
                    std::stringstream ss;
                    ss << "parameter " << (i + 1) << " of '" << cleanName
                       << "' could be 'unsigned int' (all call sites pass non-negative values)";
                    warnings_.push_back(formatDiagnostic(module_.sourceFile, funcDeclLines[name], 0,
                        Severity::Note, ss.str()));
                }
            }
        }
    }

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
    if (typeName == "float" || typeName == "double") return ir::Type::F32;

    // __int(N) / __uint(N) — map to nearest supported type
    if (typeName.substr(0, 5) == "__int" || typeName.substr(0, 6) == "__uint") {
        int sz = getTypeSize(typeName, 0);
        if (sz <= 1) return ir::Type::I8;
        if (sz <= 2) return ir::Type::I16;
        if (sz <= 4) return ir::Type::I32;
        return ir::Type::I_N;
    }

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
    if (typeName == "float" || typeName == "double") return 5;
    // __int(N) / __uint(N) — extract bit width from name
    if (typeName.substr(0, 5) == "__int" || typeName.substr(0, 6) == "__uint") {
        size_t numStart = typeName.find_first_of("0123456789");
        if (numStart != std::string::npos) {
            int bits = std::stoi(typeName.substr(numStart));
            return (bits + 7) / 8; // round up to bytes
        }
        return 4; // default
    }

    std::string sName = getAggregateName(typeName);
    bool isStructOrUnion = (typeName.rfind("struct ", 0) == 0 || typeName.rfind("union ", 0) == 0);
    auto it = structs_.find(sName);
    if (it != structs_.end()) return it->second.totalSize;
    if (isStructOrUnion) {
        throw std::runtime_error("Unknown struct/union type: " + sName);
    }
    return 2; // default for unknown types
}

IRBuilder::IRTypeInfo IRBuilder::getExprTypeInfo(Expression* expr) {
    if (!expr) return {};
    
    if (auto* gs = dynamic_cast<GenericSelection*>(expr)) {
        IRTypeInfo controlInfo = getExprTypeInfo(gs->control.get());
        int controlPtrLevel = (controlInfo.type == ir::Type::PTR) ? ((controlInfo.pointedToType == ir::Type::PTR) ? 2 : 1) : 0;
        const GenericAssociation* selected = nullptr;
        for (const auto& assoc : gs->associations) {
            if (!assoc.isDefault) {
                std::string atn = assoc.typeName;
                std::string ctn = controlInfo.typeName;
                if (atn == "unsigned") atn = "unsigned int";
                if (ctn == "unsigned") ctn = "unsigned int";
                if (atn == "signed") atn = "int";
                if (ctn == "signed") ctn = "int";
                if (atn == ctn && assoc.pointerLevel == controlPtrLevel) {
                    selected = &assoc;
                    break;
                }
            }
        }
        if (!selected) {
            for (const auto& assoc : gs->associations) {
                if (assoc.isDefault) {
                    selected = &assoc;
                    break;
                }
            }
        }
        if (selected && selected->result) {
            return getExprTypeInfo(selected->result.get());
        }
        return {ir::Type::I16, "int", false, ir::Type::VOID, 2};
    }
    
    if (auto* lit = dynamic_cast<IntegerLiteral*>(expr)) {
        ir::Type t = ir::Type::I16;
        std::string tn = "int";
        bool isSigned = false;  // cc45 defaults to unsigned
        if (!lit->castType.empty()) {
            tn = lit->castType;
            t = mapType(lit->castType, lit->castPointerLevel);
            isSigned = lit->castIsSigned;
        } else if (lit->value > 65535 || lit->value < -32768) {
            t = ir::Type::I32;
            tn = "long";
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
        IRTypeInfo baseInfo = getExprTypeInfo(ma->structExpr.get());
        std::string sName = getAggregateName(baseInfo.typeName);
        int accOffset = 0;
        auto* mit = findStructMember(sName, ma->memberName, accOffset);
        if (mit) {
            IRTypeInfo info;
            info.type = mapType(mit->type, mit->pointerLevel);
            info.typeName = mit->type;
            info.isSigned = mit->isSigned;
            info.totalSize = ir::typeSize(info.type);
            if (info.type == ir::Type::PTR) {
                info.pointedToType = mapType(mit->type, 0);
            }
            return info;
        }
        // Fallback: search all structs
        for (const auto& [sname, sinfo] : structs_) {
            auto it = sinfo.members.find(ma->memberName);
            if (it != sinfo.members.end()) {
                IRTypeInfo info;
                info.type = mapType(it->second.type, it->second.pointerLevel);
                info.typeName = it->second.type;
                info.isSigned = it->second.isSigned;
                info.totalSize = ir::typeSize(info.type);
                if (info.type == ir::Type::PTR) {
                    info.pointedToType = mapType(it->second.type, 0);
                }
                return info;
            }
        }
    }
    
    if (auto* aa = dynamic_cast<ArrayAccess*>(expr)) {
        std::string tn = getExprTypeInfo(aa->arrayExpr.get()).typeName;
        if (tn.empty()) tn = "int";
        Expression* root = aa->arrayExpr.get();
        int depth = 1;
        while (auto* inner = dynamic_cast<ArrayAccess*>(root)) {
            root = inner->arrayExpr.get();
            depth++;
        }
        
        if (auto* ref = dynamic_cast<VariableReference*>(root)) {
            ir::Type baseType = ir::Type::I16;
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
            // Final dimension (or pointer subscript) -> it's the element type.
            // For pointer arrays (e.g. char *arr[7]), the element is a pointer,
            // not the pointed-to type.
            {
                auto tit = localTypes_.find(ref->name);
                ir::Type varType = (tit != localTypes_.end()) ? tit->second : ir::Type::I16;
                if (tit == localTypes_.end()) {
                    auto gtit = globalTypes_.find(ref->name);
                    if (gtit != globalTypes_.end()) varType = gtit->second;
                }
                if (varType == ir::Type::PTR && !dims.empty()) {
                    // Array of pointers: element type is PTR, pointed-to is baseType
                    return {ir::Type::PTR, tn, isSigned, baseType, 2};
                }
            }
            return {baseType, tn, isSigned, ir::Type::VOID, getTypeSize(tn, 0)};
        } else {
            // General fallback for non-variable roots (e.g. member arrays, function returns, derefs)
            IRTypeInfo arrayTypeInfo = getExprTypeInfo(aa->arrayExpr.get());
            ir::Type baseType = arrayTypeInfo.pointedToType;
            if (baseType == ir::Type::VOID) {
                baseType = mapType(tn, 0);
            }
            return {baseType, tn, arrayTypeInfo.isSigned, ir::Type::VOID, getTypeSize(tn, 0)};
        }
    }
    
    if (auto* deref = dynamic_cast<UnaryOperation*>(expr)) {
        if (deref->op == "*") {
            if (auto* ref = dynamic_cast<VariableReference*>(deref->operand.get())) {
                auto pit = localPointedToType_.find(ref->name);
                if (pit != localPointedToType_.end()) {
                    std::string tn = localTypeNames_.count(ref->name) ? localTypeNames_[ref->name] : "";
                    int sz = tn.empty() ? ir::typeSize(pit->second) : getTypeSize(tn, 0);
                    return {pit->second, tn, localSigned_[ref->name], ir::Type::VOID, sz};
                }
                auto gpit = globalPointedToType_.find(ref->name);
                if (gpit != globalPointedToType_.end()) {
                    std::string tn = globalTypeNames_.count(ref->name) ? globalTypeNames_[ref->name] : "";
                    int sz = tn.empty() ? ir::typeSize(gpit->second) : getTypeSize(tn, 0);
                    return {gpit->second, tn, true, ir::Type::VOID, sz};
                }
            }
            // General case: derive pointee type from operand expression
            // Handles *(cast_expr), *(ptr + offset), etc.
            IRTypeInfo operandInfo = getExprTypeInfo(deref->operand.get());
            if (operandInfo.type == ir::Type::PTR && operandInfo.pointedToType != ir::Type::VOID) {
                int sz = operandInfo.typeName.empty() ? ir::typeSize(operandInfo.pointedToType) : getTypeSize(operandInfo.typeName, 0);
                return {operandInfo.pointedToType, operandInfo.typeName, operandInfo.isSigned, ir::Type::VOID, sz};
            }
        }
        if (deref->op == "&") {
            IRTypeInfo sub = getExprTypeInfo(deref->operand.get());
            return {ir::Type::PTR, sub.typeName + "*", false, sub.type, 2};
        }
        // ++, --, ++_POST, --_POST: same type as operand
        if (deref->op == "++" || deref->op == "--" ||
            deref->op == "++_POST" || deref->op == "--_POST") {
            return getExprTypeInfo(deref->operand.get());
        }
    }

    if (auto* call = dynamic_cast<FunctionCall*>(expr)) {
        auto it = functionReturnTypes_.find(call->name);
        if (it != functionReturnTypes_.end()) {
            bool sig = false; // unsigned by default
            auto sit = functionParamSigned_.find(call->name);
            if (sit != functionParamSigned_.end() && !sit->second.empty()) {
                sig = sit->second[0]; // TODO: use a proper convention for return type signedness
            }
            return {it->second, "", sig, ir::Type::VOID, ir::typeSize(it->second)};
        }
    }

    // Binary operations: propagate type and signedness
    if (auto* bin = dynamic_cast<BinaryOperation*>(expr)) {
        if (bin->op == ",") {
            return getExprTypeInfo(bin->right.get());
        }
        // For arithmetic/bitwise ops, result type comes from operands
        auto lhs = getExprTypeInfo(bin->left.get());
        auto rhs = getExprTypeInfo(bin->right.get());
        // Signed if both operands are signed, or one is signed and the other
        // is an unsigned literal that fits in signed range
        bool sig = lhs.isSigned && rhs.isSigned;
        if (!sig && (lhs.isSigned || rhs.isSigned)) {
            auto* lhsLit = dynamic_cast<IntegerLiteral*>(bin->left.get());
            auto* rhsLit = dynamic_cast<IntegerLiteral*>(bin->right.get());
            if (lhs.isSigned && !rhs.isSigned && rhsLit && rhsLit->castType.empty() && rhsLit->value >= 0 && rhsLit->value <= 32767)
                sig = true;
            else if (rhs.isSigned && !lhs.isSigned && lhsLit && lhsLit->castType.empty() && lhsLit->value >= 0 && lhsLit->value <= 32767)
                sig = true;
        }
        ir::Type t = (ir::typeSize(lhs.type) >= ir::typeSize(rhs.type)) ? lhs.type : rhs.type;
        return {t, lhs.typeName, sig, ir::Type::VOID, ir::typeSize(t)};
    }

    return {ir::Type::I16, "int", false, ir::Type::VOID, 2};
}

ir::Operand IRBuilder::emitCast(ir::Operand src, ir::Type targetType, bool isSigned) {
    if (src.type == targetType) return src;

    // Float conversions
    if (src.type == ir::Type::F32 && targetType != ir::Type::F32) {
        auto dest = allocVreg(targetType);
        ir::Inst inst; inst.op = ir::Op::FTOI; inst.dest = dest;
        inst.resultType = targetType; inst.src1 = src; emit(inst);
        return dest;
    }
    if (targetType == ir::Type::F32 && src.type != ir::Type::F32) {
        auto dest = allocVreg(ir::Type::F32);
        ir::Inst inst; inst.op = ir::Op::ITOF; inst.dest = dest;
        inst.resultType = ir::Type::F32; inst.src1 = src; emit(inst);
        return dest;
    }

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

IRBuilder::IRMemberInfo* IRBuilder::findStructMember(const std::string& sName, const std::string& memberName, int& accumulatedOffset) {
    auto sit = structs_.find(sName);
    if (sit == structs_.end()) return nullptr;

    auto mit = sit->second.members.find(memberName);
    if (mit != sit->second.members.end()) {
        return &mit->second;
    }

    for (const auto& [name, mi] : sit->second.members) {
        if (name.rfind("__anon_member_", 0) == 0 || name.empty()) {
            std::string subStructName = getAggregateName(mi.type);
            int subOffset = 0;
            auto* found = findStructMember(subStructName, memberName, subOffset);
            if (found) {
                accumulatedOffset += mi.offset + subOffset;
                return found;
            }
        }
    }
    return nullptr;
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
    // Pre-scan for encoding pragma directives to set state before visiting strings
    for (auto& decl : node.topLevelDecls) {
        if (auto* as = dynamic_cast<AsmStatement*>(decl.get())) {
            if (as->code == ".encoding ascii") currentStringEncoding_ = StringEncoding::ASCII;
            if (as->code == ".encoding petscii") currentStringEncoding_ = StringEncoding::PETSCII;
            if (as->code == ".encoding screencode") currentStringEncoding_ = StringEncoding::SCREENCODE;
        }
    }
    // Now visit all declarations
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
        if (node.isRegparm) regparmFunctions_.insert(node.name);
        allFunctions_[node.name] = &node;
    }
    if (node.isPrototype || !node.body) return;

    ir::Function fn;
    fn.name = "_" + node.name;
    definedFunctions_.insert(fn.name);
    fn.returnType = mapType(node.returnType, node.returnPointerLevel);
    fn.conv = (zpCallMode || node.isFastcall) ? ir::CallConv::ZP : ir::CallConv::STACK;
    fn.isVariadic = node.isVariadic;
    fn.isStatic = node.isStatic || node.isInline;
    fn.isWeak = weakNextFunction_;
    weakNextFunction_ = false;
    fn.isInterrupt = node.isInterrupt;
    fn.declLine = node.line;
    fn.isNaked = node.isNaked;
    fn.isRegparm = node.isRegparm;
    fn.isNested = node.isNested;

    if (node.isNested) {
        ir::Type pt = ir::Type::PTR;
        auto vreg = allocVreg(pt);
        fn.staticLinkVreg = vreg.vregId;
    }

    // Register inline candidate if marked inline, -finline-functions, or trivial method
    if (!node.isVariadic && !node.isInterrupt && !node.isNaked) {
        int stmtCount = node.body ? (int)node.body->statements.size() : 0;
        bool shouldInline = node.isInline || inlineFunctions;
        // Auto-inline trivial methods (≤3 statements — getters, setters, simple computations)
        if (!shouldInline && node.isMethod && stmtCount <= 3) {
            shouldInline = true;
        }
        if (shouldInline && stmtCount <= INLINE_MAX_STMTS) {
            inlineCandidates_[node.name] = &node;
        }
    }

    for (const auto& p : node.parameters) {
        fn.paramTypes.push_back(mapType(p.type, p.pointerLevel));
        fn.paramNames.push_back(p.name);
    }

    module_.functions.push_back(std::move(fn));
    ir::Function* fnPtr = &module_.functions.back();

    // Save current scope if we are nesting
    if (currentFunc_) {
        FunctionScope scope;
        scope.func = currentFunc_;
        scope.block = currentBlock_;
        scope.locals = std::move(locals_);
        scope.localTypes = std::move(localTypes_);
        scope.localTypeNames = std::move(localTypeNames_);
        scope.localSigned = std::move(localSigned_);
        scope.localConst = std::move(localConst_);
        scope.localPointsToConst = std::move(localPointsToConst_);
        scope.localRegister = std::move(localRegister_);
        scope.localPointedToType = std::move(localPointedToType_);
        scope.localArrayDims = std::move(localArrayDims_);
        scope.usedVregs = std::move(usedVregs_);
        scope.localDeclLocs = std::move(localDeclLocs_);
        functionStack_.push_back(std::move(scope));

        // Reset state for new function
        locals_.clear();
        localTypes_.clear();
        localTypeNames_.clear();
        localSigned_.clear();
        localConst_.clear();
        localPointsToConst_.clear();
        localRegister_.clear();
        localPointedToType_.clear();
        localArrayDims_.clear();
        usedVregs_.clear();
        localDeclLocs_.clear();
    }

    currentFunc_ = fnPtr;
    startBlock("entry");

    // Load static link from ZP if nested
    if (fnPtr->isNested) {
        ir::Type pt = ir::Type::PTR;
        auto vreg = ir::Operand::vreg(fnPtr->staticLinkVreg, pt);
        locals_["__static_link"] = vreg;
        localTypes_["__static_link"] = pt;
        localTypeNames_["__static_link"] = "void*";
        localSigned_["__static_link"] = false;
        localConst_["__static_link"] = true;
        currentFunc_->localNames["__static_link"] = vreg.vregId;
        currentFunc_->localSlotVregs.insert(vreg.vregId);

        ir::Inst loadSl;
        loadSl.op = ir::Op::LOAD_ZP;
        loadSl.dest = vreg;
        loadSl.resultType = pt;
        loadSl.src1 = ir::Operand::global("__static_chain");
        loadSl.loc = loc(node);
        emit(loadSl);
    }
    currentFuncParams_.clear();
    for (const auto& p : node.parameters) {
        currentFuncParams_.insert(p.name);
    }
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
            ir::SourceLoc sl = localDeclLocs_[name];
            int line = sl.valid() ? sl.line : node.line;
            std::string file = sl.valid() ? sl.file : node.sourceFile;
            int col = sl.valid() ? sl.column : node.column;
            warnings_.push_back(formatDiagnostic(file, line, col, Severity::Warning,
                "unused variable '" + name + "'"));
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

    // Dead block elimination: remove trailing blocks after while(1) infinite loops.
    // Conservative approach: only remove blocks that follow a block ending with an
    // unconditional back-branch (BR to an earlier block) where no other block
    // branches to the trailing blocks.
    if (currentFunc_ && currentFunc_->blocks.size() > 1) {
        // Find all referenced labels
        std::set<std::string> referenced;
        for (auto& blk : currentFunc_->blocks) {
            for (auto& inst : blk.insts) {
                if (inst.src1.kind == ir::OperandKind::LABEL) referenced.insert(inst.src1.name);
                if (inst.src2.kind == ir::OperandKind::LABEL) referenced.insert(inst.src2.name);
                if (inst.dest.kind == ir::OperandKind::LABEL) referenced.insert(inst.dest.name);
                for (auto& sc : inst.switchCases) referenced.insert(sc.second);
            }
        }
        // Remove trailing blocks that are never referenced by any branch
        while (currentFunc_->blocks.size() > 1) {
            auto& last = currentFunc_->blocks.back();
            if (referenced.find(last.label) == referenced.end()) {
                currentFunc_->blocks.pop_back();
            } else {
                break;
            }
        }
    }

    // Dead vreg elimination: remove instructions that write to vregs that are
    // never read. Also removes STORE to local slots that are never loaded.
    // Iterates until stable (handles transitive dead chains).
    if (currentFunc_) {
        bool changed = true;
        while (changed) {
            changed = false;

            // Pass 1: find local slot vregs that are only written, never read as values.
            // A STORE to a local slot (src2 = local vreg) is a write.
            // A LOAD/use of the local vreg (in src1/src2 of non-STORE, or in args) is a read.
            std::set<uint32_t> readVregs;
            std::set<uint32_t> writtenLocalSlots;
            for (auto& blk : currentFunc_->blocks) {
                for (auto& inst : blk.insts) {
                    if (inst.op == ir::Op::NOP) continue;
                    // Track reads
                    if (inst.src1.isVreg()) readVregs.insert(inst.src1.vregId);
                    // For STORE: src2 is the target. If it's a local slot, it's a write not a read.
                    if (inst.op == ir::Op::STORE && inst.src2.isVreg() &&
                        currentFunc_->localSlotVregs.count(inst.src2.vregId)) {
                        writtenLocalSlots.insert(inst.src2.vregId);
                    } else if (inst.src2.isVreg()) {
                        readVregs.insert(inst.src2.vregId);
                    }
                    for (auto& a : inst.args)
                        if (a.isVreg()) readVregs.insert(a.vregId);
                }
            }

            // Pass 2: eliminate dead local slot stores and dead vreg writes
            for (auto& blk : currentFunc_->blocks) {
                for (auto& inst : blk.insts) {
                    if (inst.op == ir::Op::NOP) continue;

                    // Eliminate STORE to local slot that is never read
                    if (inst.op == ir::Op::STORE && inst.src2.isVreg() &&
                        currentFunc_->localSlotVregs.count(inst.src2.vregId) &&
                        !currentFunc_->memoryVregs.count(inst.src2.vregId) &&
                        readVregs.find(inst.src2.vregId) == readVregs.end()) {
                        inst.op = ir::Op::NOP;
                        changed = true;
                        continue;
                    }

                    // Eliminate instructions that write to dead temp vregs
                    if (!inst.dest.isVreg()) continue;
                    if (inst.op == ir::Op::STORE || inst.op == ir::Op::STORE_ZP) continue;
                    if (inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID || inst.op == ir::Op::CALL_INDIRECT) continue;
                    if (inst.op == ir::Op::BR || inst.op == ir::Op::BR_COND) continue;
                    if (inst.op == ir::Op::RET || inst.op == ir::Op::RET_VOID) continue;
                    if (inst.op == ir::Op::ASM_INLINE) continue;
                    if (inst.op == ir::Op::CPU_REG_WRITE || inst.op == ir::Op::CPU_FLAG_WRITE) continue;
                    if (currentFunc_->memoryVregs.count(inst.dest.vregId)) continue;
                    if (readVregs.find(inst.dest.vregId) == readVregs.end()) {
                        inst.op = ir::Op::NOP;
                        changed = true;
                    }
                }
            }
        }
    }

    // Restore previous scope if we nested
    if (!functionStack_.empty()) {
        FunctionScope& scope = functionStack_.back();
        currentFunc_ = scope.func;
        currentBlock_ = scope.block;
        locals_ = std::move(scope.locals);
        localTypes_ = std::move(scope.localTypes);
        localTypeNames_ = std::move(scope.localTypeNames);
        localSigned_ = std::move(scope.localSigned);
        localConst_ = std::move(scope.localConst);
        localPointsToConst_ = std::move(scope.localPointsToConst);
        localRegister_ = std::move(scope.localRegister);
        localPointedToType_ = std::move(scope.localPointedToType);
        localArrayDims_ = std::move(scope.localArrayDims);
        usedVregs_ = std::move(scope.usedVregs);
        localDeclLocs_ = std::move(scope.localDeclLocs);
        functionStack_.pop_back();
    } else {
        currentFunc_ = nullptr;
        currentBlock_ = nullptr;
    }
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
    bool isStructOrUnion = (node.type.rfind("struct ", 0) == 0 || node.type.rfind("union ", 0) == 0);
    if (isStructOrUnion && node.pointerLevel == 0) {
        std::string sName = getAggregateName(node.type);
        if (!structs_.count(sName)) {
            throw std::runtime_error("Error at line " + std::to_string(node.line) + ": Unknown struct/union type: Unknown struct type: " + node.type);
        }
    }

    ir::Type t = mapType(node.type, node.pointerLevel);

    if (!currentFunc_) {
        // Extern declaration: add to externs list, no BSS allocation
        if (node.isExtern) {
            module_.externs.push_back("_" + node.name);
            return;
        }
        // Global variable
        ir::Module::GlobalVar gv;
        gv.name = "_" + node.name;
        gv.type = t;
        gv.size = getTypeSize(node.type, node.pointerLevel);
        if (node.arraySize() > 0) gv.size *= node.arraySize();
        gv.isConst = node.isConst;
        gv.isStatic = node.isStatic;
        if (node.initializer) {
            if (auto* flit = dynamic_cast<FloatLiteral*>(node.initializer.get())) {
                gv.hasInitValue = true;
                // Store double bits in initValue for CBM conversion in IRCodeGen
                std::memcpy(&gv.initValue, &flit->value, sizeof(double));
            } else if (auto* lit = dynamic_cast<IntegerLiteral*>(node.initializer.get())) {
                gv.hasInitValue = true;
                gv.initValue = lit->value;
            } else if (auto* slit = dynamic_cast<StringLiteral*>(node.initializer.get())) {
                // Scalar string literal init: char *p = "hello"
                std::string label = "__str_" + std::to_string(nextLabel_++);
                ir::Module::StringLiteral sl;
                sl.label = label;
                sl.value = slit->value;
                sl.isAscii = slit->isAscii || (currentStringEncoding_ == StringEncoding::ASCII);
                sl.encoding = slit->isAscii ? 1 : (currentStringEncoding_ == StringEncoding::SCREENCODE ? 2 : 0);
                module_.strings.push_back(sl);
                gv.hasInitValue = true;
                gv.initLabels.push_back(label); // symbolic reference
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
                        gv.initLabels.push_back("");
                    } else if (auto* slit = dynamic_cast<StringLiteral*>(elem.get())) {
                        // String literal in pointer array: create string constant and store label
                        std::string label = "__str_" + std::to_string(nextLabel_++);
                        ir::Module::StringLiteral sl;
                        sl.label = label;
                        sl.value = slit->value;
                        sl.isAscii = slit->isAscii || (currentStringEncoding_ == StringEncoding::ASCII);
                        sl.encoding = slit->isAscii ? 1 : (currentStringEncoding_ == StringEncoding::SCREENCODE ? 2 : 0);
                        module_.strings.push_back(sl);
                        gv.initList.push_back(0);
                        gv.initLabels.push_back(label);
                    } else {
                        gv.initList.push_back(0);
                        gv.initLabels.push_back("");
                    }
                }
            }
        }
        if (node.pointerLevel > 0) globalPointedToType_[node.name] = mapType(node.type, 0);
        if (!node.arrayDims.empty()) globalArrayDims_[node.name] = node.arrayDims;
        globalTypes_[node.name] = t;
        globalTypeNames_[node.name] = node.type;
        globalRegister_[node.name] = node.isRegister;
        module_.globals.push_back(gv);
        return;
    }

    // Local variable — allocate a vReg
    auto vreg = allocVreg(t);
    locals_[node.name] = vreg;
    localRegister_[node.name] = node.isRegister;
    localDeclLocs_[node.name] = loc(node);
    if (currentFunc_) {
        currentFunc_->localNames[node.name] = vreg.vregId;
        if (node.isRegister) {
            currentFunc_->registerVregs.insert(vreg.vregId);
        }
    }
    localTypes_[node.name] = t;
    localTypeNames_[node.name] = node.type;
    localSigned_[node.name] = node.isSigned;
    localConst_[node.name] = node.isConst && node.pointerLevel == 0;
    localPointsToConst_[node.name] = node.isConst && node.pointerLevel > 0;
    if (node.pointerLevel > 0) {
        localPointedToType_[node.name] = mapType(node.type, node.pointerLevel - 1);
        // Track constant pointer initializer for propagation
        if (node.initializer) {
            if (auto* intLit = dynamic_cast<IntegerLiteral*>(node.initializer.get())) {
                localConstPtrValue_[node.name] = intLit->value;
            } else if (auto* castExpr = dynamic_cast<CastExpression*>(node.initializer.get())) {
                if (auto* intLit2 = dynamic_cast<IntegerLiteral*>(castExpr->expression.get())) {
                    localConstPtrValue_[node.name] = intLit2->value;
                }
            }
        }
    }
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
    
    if (getTypeSize(node.type, 0) > 2 && node.pointerLevel == 0 && node.arrayDims.empty()) {
        // Large aggregates (non-array) must go to frame with correct byte size
        currentFunc_->memoryVregs.insert(vreg.vregId);
        currentFunc_->vregSizes[vreg.vregId] = getTypeSize(node.type, 0);
    }

    // Phase 3: Auto-initialize __vt pointer for structs with virtual methods
    if (node.pointerLevel == 0 && currentFunc_) {
        std::string sName = getAggregateName(node.type);
        auto sit = structs_.find(sName);
        if (sit != structs_.end() && sit->second.members.count("__vt")) {
            // Find vtable global name
            std::string vtableName = sName + "_vtable"; // ADDR_GLOBAL adds _ prefix
            // Store vtable address to __vt member (offset 0)
            auto baseAddr = allocVreg(ir::Type::PTR);
            ir::Inst addr;
            addr.op = ir::Op::ADDR_LOCAL;
            addr.dest = baseAddr;
            addr.resultType = ir::Type::PTR;
            addr.src1 = vreg;
            addr.loc = loc(node);
            emit(addr);

            auto vtAddr = allocVreg(ir::Type::PTR);
            ir::Inst loadVt;
            loadVt.op = ir::Op::ADDR_GLOBAL;
            loadVt.dest = vtAddr;
            loadVt.resultType = ir::Type::PTR;
            loadVt.src1 = ir::Operand::global(vtableName);
            loadVt.loc = loc(node);
            emit(loadVt);

            ir::Inst storeVt;
            storeVt.op = ir::Op::STORE;
            storeVt.resultType = ir::Type::I16;
            storeVt.src1 = vtAddr;
            storeVt.src2 = baseAddr;
            storeVt.loc = loc(node);
            emit(storeVt);
        }
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
    // Auto-promote to long if value exceeds 16-bit range
    if (t == ir::Type::I16 && node.castType.empty() && (node.value > 65535 || node.value < -32768)) {
        t = ir::Type::I32;
        std::stringstream ss;
        ss << "integer literal " << node.value << " exceeds 16-bit range, promoted to long";
        warnings_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column,
            Severity::Warning, ss.str()));
    }
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

void IRBuilder::visit(FloatLiteral& node) {
    auto dest = allocVreg(ir::Type::F32);
    ir::Inst inst;
    inst.op = ir::Op::FCONST;
    inst.dest = dest;
    inst.resultType = ir::Type::F32;
    int64_t bits = 0;
    static_assert(sizeof(double) == sizeof(int64_t), "double must be 64-bit");
    std::memcpy(&bits, &node.value, sizeof(double));
    inst.src1 = ir::Operand::imm(bits, ir::Type::F32);
    inst.loc = loc(node);
    emit(inst);
    lastValue_ = dest;
    lastValueSigned_ = false;
}

void IRBuilder::visit(StringLiteral& node) {
    std::string name = "__str_" + std::to_string(nextLabel_++);
    // Determine encoding: explicit @"..." forces ASCII, otherwise use pragma state
    int encoding = 0;  // PETSCII default
    if (node.isAscii) {
        encoding = 1;  // ASCII
    } else if (currentStringEncoding_ == StringEncoding::ASCII) {
        encoding = 1;  // ASCII via pragma
    } else if (currentStringEncoding_ == StringEncoding::SCREENCODE) {
        encoding = 2;  // SCREENCODE via pragma
    }
    // Record the string literal in the module for data section emission
    ir::Module::StringLiteral sl;
    sl.label = name;
    sl.value = node.value;
    sl.isAscii = (encoding == 1);
    sl.encoding = encoding;
    module_.strings.push_back(sl);
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
    // Check if this is a function name used as a value (function pointer)
    auto fit = allFunctions_.find(node.name);
    if (fit != allFunctions_.end()) {
        auto* fn = fit->second;
        if (fn->isNested) {
            // WE NEED A TRAMPOLINE!
            // 1. Allocate an 11-byte buffer on the frame for the trampoline
            auto bufVreg = allocVreg(ir::Type::PTR);
            currentFunc_->vregSizes[bufVreg.vregId] = 11;
            currentFunc_->memoryVregs.insert(bufVreg.vregId);
            
            // 2. Get the static link to pass to the nested function.
            // (Same logic as direct FunctionCall: if callee is our direct child, SL=our FP).
            ir::Operand sl;
            FunctionDeclaration* calleeParent = fn->parentFunc;
            if (currentFunc_ && "_" + calleeParent->name == currentFunc_->name) {
                sl = allocVreg(ir::Type::PTR);
                ir::Inst getFp;
                getFp.op = ir::Op::GET_FP;
                getFp.dest = sl;
                getFp.resultType = ir::Type::PTR;
                getFp.loc = loc(node);
                emit(getFp);
            } else {
                // ... follow static chain ... (simplified for now: assume direct parent)
                if (currentFunc_ && currentFunc_->isNested) {
                     sl = ir::Operand::vreg(currentFunc_->staticLinkVreg, ir::Type::PTR);
                } else {
                     sl = ir::Operand::imm(0, ir::Type::PTR);
                }
            }

            // 3. Emit TRAMPOLINE opcode to initialize the buffer
            // dest = result (address of trampoline), src1 = target func, src2 = link, args[0] = buffer vreg
            auto result = allocVreg(ir::Type::PTR);
            ir::Inst tramp;
            tramp.op = ir::Op::TRAMPOLINE;
            tramp.dest = result;
            tramp.resultType = ir::Type::PTR;
            tramp.src1 = ir::Operand::global("_" + node.name);
            tramp.src2 = sl;
            tramp.args = { bufVreg };
            tramp.loc = loc(node);
            emit(tramp);
            
            lastValue_ = result;
            lastValueSigned_ = false;
            return;
        } else {
            // Global function pointer — track reference for extern emission
            calledFunctions_.insert("_" + node.name);
            lastValue_ = ir::Operand::global("_" + node.name);
            lastValueSigned_ = false;
            return;
        }
    }

    auto it = locals_.find(node.name);
    if (it != locals_.end()) {
        // Mark as used whether reading value or taking address (&var).
        // Address-of still "uses" the variable (e.g. passing struct by pointer).
        usedVregs_.insert(it->second.vregId);

        // Array-to-pointer decay: when an array name is used as a value
        // (not in address-only mode), it decays to a pointer to its first element.
        bool isArray = localArrayDims_.count(node.name) > 0;

        if (computeAddressOnly_ || isArray) {
            // Return the address of the local variable (or array decay)
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
        return;
    }

    // Search parent scopes
    for (int i = (int)functionStack_.size() - 1; i >= 0; i--) {
        auto& scope = functionStack_[i];
        auto pit = scope.locals.find(node.name);
        if (pit != scope.locals.end()) {
            // Found in parent frame!
            // Mark as escaped/captured in the parent function
            scope.func->memoryVregs.insert(pit->second.vregId);
            scope.usedVregs.insert(pit->second.vregId);

            // Use ADDR_UPLEVEL to get its address
            int levels = (int)functionStack_.size() - i;
            auto addr = allocVreg(ir::Type::PTR);
            ir::Inst addrInst;
            addrInst.op = ir::Op::ADDR_UPLEVEL;
            addrInst.dest = addr;
            addrInst.resultType = ir::Type::PTR;
            addrInst.src1 = ir::Operand::vreg(currentFunc_->staticLinkVreg, ir::Type::PTR);
            // Pack metadata into args: [0]=parent_vreg, [1]=imm(levels)
            ir::Operand parentVreg = pit->second;
            parentVreg.name = scope.func->name;
            addrInst.args = { parentVreg, ir::Operand::imm(levels, ir::Type::I16) };
            addrInst.loc = loc(node);
            emit(addrInst);

            if (computeAddressOnly_) {
                lastValue_ = addr;
            } else {
                // Load the value from the captured address
                ir::Type t = scope.localTypes.at(node.name);
                auto val = allocVreg(t);
                ir::Inst loadInst;
                loadInst.op = ir::Op::LOAD;
                loadInst.dest = val;
                loadInst.resultType = t;
                loadInst.src1 = addr;
                loadInst.loc = loc(node);
                emit(loadInst);
                lastValue_ = val;
            }
            lastValueSigned_ = scope.localSigned.at(node.name);
            return;
        }
    }

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

void IRBuilder::visit(Assignment& node) {
    // 1. Semantic checks for constness
    if (auto* ma = dynamic_cast<MemberAccess*>(node.target.get())) {
        for (const auto& [sname, sinfo] : structs_) {
            auto mit = sinfo.members.find(ma->memberName);
            if (mit != sinfo.members.end() && mit->second.isConst) {
                errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column,
                    Severity::Error, "Compile Error: Assignment to read-only location"));
                return;
            }
        }
    }
    if (auto* vr = dynamic_cast<VariableReference*>(node.target.get())) {
        auto cit = localConst_.find(vr->name);
        if (cit != localConst_.end() && cit->second) {
            errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column,
                Severity::Error, "Compile Error: Assignment to read-only location"));
            return;
        }
    }
    if (auto* deref = dynamic_cast<UnaryOperation*>(node.target.get())) {
        if (deref->op == "*") {
            if (auto* vr = dynamic_cast<VariableReference*>(deref->operand.get())) {
                auto pit = localPointsToConst_.find(vr->name);
                if (pit != localPointsToConst_.end() && pit->second) {
                    // Check if this is a multi-level pointer (const T **pp)
                    // For const T *p: *p gives const T → read-only
                    // For const T **pp: *pp gives const T* → writable (it's a pointer)
                    auto ptit = localPointedToType_.find(vr->name);
                    bool isMultiPtr = (ptit != localPointedToType_.end() && ptit->second == ir::Type::PTR);
                    if (!isMultiPtr) {
                        errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column,
                            Severity::Error, "Compile Error: Assignment to read-only location"));
                        return;
                    }
                }
            }
        }
    }

    // 1.5 Fast path: *const_ptr = literal → emit direct store with immediate, no vreg
    if (auto* deref = dynamic_cast<UnaryOperation*>(node.target.get())) {
        if (deref->op == "*") {
            if (auto* ref = dynamic_cast<VariableReference*>(deref->operand.get())) {
                auto cit = localConstPtrValue_.find(ref->name);
                if (cit != localConstPtrValue_.end()) {
                    if (auto* litExpr = dynamic_cast<IntegerLiteral*>(node.expression.get())) {
                        IRTypeInfo derefInfo = getExprTypeInfo(node.target.get());
                        std::string addrName = "$" + [&]() {
                            std::stringstream ss;
                            ss << std::hex << std::uppercase << std::setfill('0')
                               << std::setw(4) << (uint16_t)cit->second;
                            return ss.str();
                        }();
                        ir::Inst store;
                        store.op = ir::Op::STORE;
                        store.resultType = derefInfo.type;
                        store.src1 = ir::Operand::imm(litExpr->value, derefInfo.type);
                        store.src2 = ir::Operand::global(addrName);
                        store.loc = loc(node);
                        emit(store);
                        lastValue_ = store.src1;
                        return;
                    }
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

    // 2.5 Handle compound assignment (+=, -=, &=, |=, ^=, <<=, >>=, *=, /=, %=)
    if (node.op != "=") {
        // Load current LHS value
        node.target->accept(*this);
        auto lhsVal = lastValue_;
        bool lhsSigned = lastValueSigned_;
        auto rhsInfo = getExprTypeInfo(node.expression.get());

        // Determine the operation
        ir::Op binOp = ir::Op::NOP;
        bool bothSigned = lhsSigned && rhsInfo.isSigned;
        if (!bothSigned && (lhsSigned || rhsInfo.isSigned)) {
            auto* rhsLit = dynamic_cast<IntegerLiteral*>(node.expression.get());
            if (lhsSigned && !rhsInfo.isSigned && rhsLit && rhsLit->castType.empty() && rhsLit->value >= 0 && rhsLit->value <= 32767)
                bothSigned = true;
        }
        if (node.op == "+=") binOp = ir::Op::ADD;
        else if (node.op == "-=") binOp = ir::Op::SUB;
        else if (node.op == "*=") binOp = bothSigned ? ir::Op::MUL : ir::Op::MUL_U;
        else if (node.op == "/=") binOp = bothSigned ? ir::Op::DIV : ir::Op::DIV_U;
        else if (node.op == "%=") binOp = bothSigned ? ir::Op::MOD : ir::Op::MOD_U;
        else if (node.op == "&=") binOp = ir::Op::AND;
        else if (node.op == "|=") binOp = ir::Op::OR;
        else if (node.op == "^=") binOp = ir::Op::XOR;
        else if (node.op == "<<=") binOp = ir::Op::SHL;
        else if (node.op == ">>=") binOp = bothSigned ? ir::Op::ASR : ir::Op::SHR;

        if (binOp != ir::Op::NOP) {
            ir::Type opType = lhsVal.type;
            auto castRhs = emitCast(rhs, opType, false);
            auto dest = allocVreg(opType);
            ir::Inst inst;
            inst.op = binOp;
            inst.dest = dest;
            inst.resultType = opType;
            inst.src1 = lhsVal;
            inst.src2 = castRhs;
            inst.loc = loc(node);
            emit(inst);
            rhs = dest;
        }
    }

    // 2.6 Handle CPU register/flag writes
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
            auto bfinsResult = allocVreg(memberType);
            bfins.dest = bfinsResult;  // Allocate vreg to hold modified value
            bfins.src1 = rhs;
            bfins.src2 = addr;
            bfins.args.push_back(ir::Operand::imm(bitOffset, ir::Type::I8));
            bfins.args.push_back(ir::Operand::imm(bitWidth, ir::Type::I8));
            bfins.loc = loc(node);
            emit(bfins);

            // Store the modified value back to the original storage location
            ir::Inst store;
            store.op = ir::Op::STORE;
            store.src1 = bfinsResult;
            store.src2 = addr;
            store.resultType = ir::Type::VOID;
            store.loc = loc(node);
            emit(store);

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
            // Track constant pointer assignments for propagation
            if (storeType == ir::Type::PTR || localPointedToType_.count(ref->name)) {
                bool tracked = false;
                if (auto* intLit = dynamic_cast<IntegerLiteral*>(node.expression.get())) {
                    localConstPtrValue_[ref->name] = intLit->value;
                    tracked = true;
                } else if (auto* castExpr = dynamic_cast<CastExpression*>(node.expression.get())) {
                    if (auto* intLit2 = dynamic_cast<IntegerLiteral*>(castExpr->expression.get())) {
                        localConstPtrValue_[ref->name] = intLit2->value;
                        tracked = true;
                    }
                }
                if (!tracked) localConstPtrValue_.erase(ref->name);
            }
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

    // Pointer constant propagation: *ptr = val where ptr holds a known constant address
    // Emit direct absolute store instead of loading the pointer and doing indirect store.
    if (auto* deref = dynamic_cast<UnaryOperation*>(node.target.get())) {
        if (deref->op == "*") {
            if (auto* ref = dynamic_cast<VariableReference*>(deref->operand.get())) {
                auto cit = localConstPtrValue_.find(ref->name);
                if (cit != localConstPtrValue_.end()) {
                    IRTypeInfo derefInfo = getExprTypeInfo(node.target.get());

                    // Use immediate operand if RHS is a literal (avoids CONST vreg entirely)
                    ir::Operand val;
                    if (auto* litExpr = dynamic_cast<IntegerLiteral*>(node.expression.get())) {
                        val = ir::Operand::imm(litExpr->value, derefInfo.type);
                    } else {
                        val = emitCast(rhsVal, derefInfo.type, derefInfo.isSigned);
                    }

                    // Synthesize a global name for the absolute address
                    std::string addrName = "$" + [&]() {
                        std::stringstream ss;
                        ss << std::hex << std::uppercase << std::setfill('0')
                           << std::setw(4) << (uint16_t)cit->second;
                        return ss.str();
                    }();

                    ir::Inst store;
                    store.op = ir::Op::STORE;
                    store.resultType = derefInfo.type;
                    store.src1 = val;
                    store.src2 = ir::Operand::global(addrName);
                    store.loc = loc(node);
                    emit(store);
                    lastValue_ = val;
                    return;
                }
            }
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
    // Comma operator: evaluate left for side effects, discard, return right
    if (node.op == ",") {
        node.left->accept(*this);
        // Discard left result
        node.right->accept(*this);
        // lastValue_ is now the right-hand result
        return;
    }

    // Short-circuit evaluation for && and ||
    // C guarantees left-to-right evaluation with short-circuit.
    // Emit: evaluate LHS → branch on result → evaluate RHS → merge
    if (node.op == "&&" || node.op == "||") {
        std::string mergeLabel = newLabel("sc_merge");
        std::string shortLabel = newLabel("sc_short");

        // Evaluate LHS
        node.left->accept(*this);
        auto lhs = lastValue_;

        // Branch: for &&, skip RHS if LHS is false; for ||, skip if true
        // BR_COND: src1=condition, dest=true label, src2=false label
        {
            ir::Inst br;
            br.op = ir::Op::BR_COND;
            br.src1 = lhs;
            if (node.op == "&&") {
                // LHS true → continue to RHS (merge), LHS false → short-circuit
                br.dest = ir::Operand::label(mergeLabel);
                br.src2 = ir::Operand::label(shortLabel);
            } else {
                // LHS true → short-circuit, LHS false → continue to RHS (merge)
                br.dest = ir::Operand::label(shortLabel);
                br.src2 = ir::Operand::label(mergeLabel);
            }
            br.loc = loc(node);
            emit(br);
        }

        // RHS evaluation block (merge)
        startBlock(mergeLabel);
        node.right->accept(*this);
        auto rhs = lastValue_;

        // Jump to done
        std::string doneLabel = newLabel("sc_done");
        {
            ir::Inst br;
            br.op = ir::Op::BR;
            br.src1 = ir::Operand::label(doneLabel);
            br.loc = loc(node);
            emit(br);
        }

        // Short-circuit block: result is 0 for &&, 1 for ||
        startBlock(shortLabel);
        auto shortVal = allocVreg(ir::Type::I8);
        {
            ir::Inst c;
            c.op = ir::Op::CONST;
            c.dest = shortVal;
            c.resultType = ir::Type::I8;
            c.src1 = ir::Operand::imm(node.op == "&&" ? 0 : 1, ir::Type::I8);
            c.loc = loc(node);
            emit(c);
        }
        {
            ir::Inst br;
            br.op = ir::Op::BR;
            br.src1 = ir::Operand::label(doneLabel);
            br.loc = loc(node);
            emit(br);
        }

        // Done block — use RHS result directly (no extra CMP_NE wrapper).
        // For if-conditions, the BR_COND fusion will use the RHS comparison
        // result's flags directly.
        startBlock(doneLabel);
        lastValue_ = rhs;
        return;
    }

    IRTypeInfo lhsInfo = getExprTypeInfo(node.left.get());
    IRTypeInfo rhsInfo = getExprTypeInfo(node.right.get());

    // Operator overloading: if LHS is a struct with an operator method, dispatch to it
    if (!lhsInfo.typeName.empty() && lhsInfo.type != ir::Type::PTR) {
        std::string sName = getAggregateName(lhsInfo.typeName);
        if (structs_.count(sName)) {
            // Map operator to method name suffix
            static const std::map<std::string, std::string> opMethodMap = {
                {"+", "add"}, {"-", "sub"}, {"*", "mul"}, {"/", "div"}, {"%", "mod"},
                {"==", "eq"}, {"!=", "ne"}, {"<", "lt"}, {">", "gt"}, {"<=", "le"}, {">=", "ge"},
                {"<<", "shl"}, {">>", "shr"}, {"&", "band"}, {"|", "bor"}, {"^", "bxor"},
                {"+=", "add_assign"}, {"-=", "sub_assign"}, {"*=", "mul_assign"},
                {"/=", "div_assign"}, {"%=", "mod_assign"}, {"&=", "band_assign"},
                {"|=", "bor_assign"}, {"^=", "bxor_assign"}, {"<<=", "shl_assign"}, {">>=", "shr_assign"},
            };
            auto opIt = opMethodMap.find(node.op);
            if (opIt != opMethodMap.end()) {
                std::string mangledName = sName + "__operator_" + opIt->second;
                if (functionReturnTypes_.count(mangledName)) {
                    // Compute 'this' pointer (address of LHS)
                    computeAddressOnly_ = true;
                    node.left->accept(*this);
                    computeAddressOnly_ = false;
                    auto thisPtr = lastValue_;

                    // Evaluate RHS
                    node.right->accept(*this);
                    auto rhsVal = lastValue_;

                    // Call StructName__operator_OP(&lhs, rhs)
                    auto retType = functionReturnTypes_[mangledName];
                    auto dest = allocVreg(retType);
                    ir::Inst call;
                    call.op = (retType == ir::Type::VOID) ? ir::Op::CALL_VOID : ir::Op::CALL;
                    call.dest = dest;
                    call.resultType = retType;
                    call.src1 = ir::Operand::global("_" + mangledName);
                    call.args = {thisPtr, rhsVal};
                    call.callConv = ir::CallConv::STACK;
                    call.loc = loc(node);
                    emit(call);
                    lastValue_ = dest;
                    return;
                }
            }
        }
    }

    // Pre-determine if we can keep this operation at I8 (char width).
    // Only when BOTH operands are I8, or one is I8 and the other is a small
    // literal with no explicit wider type. Never narrow a wider operand to I8.
    bool forceI8 = false;
    if (lhsInfo.type == ir::Type::I8 && rhsInfo.type == ir::Type::I8) {
        forceI8 = true;
    } else if (lhsInfo.type == ir::Type::I8 && ir::typeSize(rhsInfo.type) <= 2) {
        // I8 op with I16/PTR literal — keep I8 if literal fits in 8 bits
        if (auto* rlit = dynamic_cast<IntegerLiteral*>(node.right.get())) {
            if (rlit->castType.empty() && rlit->value >= 0 && rlit->value <= 255) forceI8 = true;
        }
    } else if (rhsInfo.type == ir::Type::I8 && ir::typeSize(lhsInfo.type) <= 2) {
        if (auto* llit = dynamic_cast<IntegerLiteral*>(node.left.get())) {
            if (llit->castType.empty() && llit->value >= 0 && llit->value <= 255) forceI8 = true;
        }
    }

    // Visit LHS
    node.left->accept(*this);
    auto lhs = lastValue_;

    // For shift operations with literal amount, pass as immediate (enables byte-shuffle optimizations)
    bool shiftImm = false;
    if ((node.op == "<<" || node.op == ">>") && dynamic_cast<IntegerLiteral*>(node.right.get())) {
        shiftImm = true;
    }

    // Visit RHS — emit literal at I8 width if forceI8
    if (shiftImm) {
        // Don't visit RHS — will use immediate directly in the shift instruction
    } else if (forceI8 && dynamic_cast<IntegerLiteral*>(node.right.get())) {
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
    auto rhs = shiftImm ? ir::Operand::imm(
        dynamic_cast<IntegerLiteral*>(node.right.get())->value, ir::Type::I8)
        : lastValue_;

    // Determine result type
    ir::Type resultType = forceI8 ? ir::Type::I8 : lhs.type;
    if (!forceI8 && !shiftImm && ir::typeSize(rhs.type) > ir::typeSize(resultType)) resultType = rhs.type;

    // Cast operands to result type
    auto lhsVal = emitCast(lhs, resultType, lhsInfo.isSigned);
    auto rhsVal = shiftImm ? rhs : emitCast(rhs, resultType, rhsInfo.isSigned);

    // Pointer arithmetic: scale integer operand by pointed-to element size
    if ((node.op == "+" || node.op == "-") && resultType == ir::Type::PTR) {
        int elemSize = 1;
        IRTypeInfo ptrInfo = (lhsInfo.type == ir::Type::PTR) ? lhsInfo : rhsInfo;
        if (ptrInfo.pointedToType != ir::Type::VOID)
            elemSize = ir::typeSize(ptrInfo.pointedToType);
        // Check for struct/union with larger sizes
        if (!ptrInfo.typeName.empty()) {
            std::string sName = getAggregateName(ptrInfo.typeName);
            auto sit = structs_.find(sName);
            if (sit != structs_.end()) elemSize = sit->second.totalSize;
        }
        if (elemSize > 1) {
            // Scale the non-pointer operand
            auto& intOp = (lhsInfo.type != ir::Type::PTR) ? lhsVal : rhsVal;
            auto scaled = allocVreg(ir::Type::PTR);
            ir::Inst mul;
            mul.op = ir::Op::MUL_U;
            mul.dest = scaled;
            mul.resultType = ir::Type::PTR;
            mul.src1 = intOp;
            mul.src2 = ir::Operand::imm(elemSize, ir::Type::PTR);
            mul.loc = loc(node);
            emit(mul);
            intOp = scaled;
        }
    }

    // For comparison operators: use signed ops only if BOTH operands are signed.
    // Per C semantics: if one operand is signed and the other is an unsigned
    // literal that fits in the signed range (0-32767 for 16-bit), treat as signed.
    bool bothSigned = lhsInfo.isSigned && rhsInfo.isSigned;
    if (!bothSigned && (lhsInfo.isSigned || rhsInfo.isSigned)) {
        auto* lhsLit = dynamic_cast<IntegerLiteral*>(node.left.get());
        auto* rhsLit = dynamic_cast<IntegerLiteral*>(node.right.get());
        if (lhsInfo.isSigned && !rhsInfo.isSigned && rhsLit && rhsLit->castType.empty() && rhsLit->value >= 0 && rhsLit->value <= 32767)
            bothSigned = true;
        else if (rhsInfo.isSigned && !lhsInfo.isSigned && lhsLit && lhsLit->castType.empty() && lhsLit->value >= 0 && lhsLit->value <= 32767)
            bothSigned = true;
    }

    ir::Op op = ir::Op::NOP;
    ir::Type finalResultType = resultType;

    // C integer promotion: arithmetic on char (I8) promotes to int (I16)
    // This ensures signed subtraction and other arithmetic produce correct results.
    if (finalResultType == ir::Type::I8) {
        finalResultType = ir::Type::I16;
        lhsVal = emitCast(lhsVal, ir::Type::I16, lhsInfo.isSigned);
        rhsVal = emitCast(rhsVal, ir::Type::I16, rhsInfo.isSigned);
    }

    // Float binary operations
    if (resultType == ir::Type::F32) {
        // Float comparisons: emit FCMP (returns -1/0/1), then unsigned test
        if (node.op == "==" || node.op == "!=" ||
            node.op == "<" || node.op == "<=" ||
            node.op == ">" || node.op == ">=") {
            // FCMP → I8 result: -1/$FF (a<b), 0 (a==b), 1 (a>b)
            auto cmpResult = allocVreg(ir::Type::I8);
            ir::Inst cmp; cmp.op = ir::Op::FCMP; cmp.dest = cmpResult;
            cmp.resultType = ir::Type::I8;
            cmp.src1 = lhsVal; cmp.src2 = rhsVal; cmp.loc = loc(node); emit(cmp);
            // Map float comparison to unsigned equality check on FCMP result:
            //   ==  →  result == 0     !=  →  result != 0
            //   <   →  result == $FF   <=  →  result != 1
            //   >   →  result == 1     >=  →  result != $FF
            ir::Op intCmp; int testVal;
            if (node.op == "==")      { intCmp = ir::Op::CMP_EQ;  testVal = 0; }
            else if (node.op == "!=") { intCmp = ir::Op::CMP_NE;  testVal = 0; }
            else if (node.op == "<")  { intCmp = ir::Op::CMP_EQ;  testVal = 0xFF; }
            else if (node.op == "<=") { intCmp = ir::Op::CMP_NE;  testVal = 1; }
            else if (node.op == ">")  { intCmp = ir::Op::CMP_EQ;  testVal = 1; }
            else                      { intCmp = ir::Op::CMP_NE;  testVal = 0xFF; }
            auto boolResult = allocVreg(ir::Type::I8);
            ir::Inst test; test.op = intCmp; test.dest = boolResult;
            test.resultType = ir::Type::I8;
            test.src1 = cmpResult;
            test.src2 = ir::Operand::imm(testVal, ir::Type::I8);
            test.loc = loc(node); emit(test);
            lastValue_ = boolResult; return;
        }
        if (node.op == "+") op = ir::Op::FADD;
        else if (node.op == "-") op = ir::Op::FSUB;
        else if (node.op == "*") op = ir::Op::FMUL;
        else if (node.op == "/") op = ir::Op::FDIV;
        else throw std::runtime_error("Unsupported operator '" + node.op + "' for float type");
        auto dest = allocVreg(finalResultType);
        ir::Inst inst; inst.op = op; inst.dest = dest; inst.resultType = finalResultType;
        inst.src1 = lhsVal; inst.src2 = rhsVal; inst.loc = loc(node); emit(inst);
        lastValue_ = dest; return;
    }

    if (node.op == "+") op = ir::Op::ADD;
    else if (node.op == "-") op = ir::Op::SUB;
    else if (node.op == "*") op = bothSigned ? ir::Op::MUL : ir::Op::MUL_U;
    else if (node.op == "/") op = bothSigned ? ir::Op::DIV : ir::Op::DIV_U;
    else if (node.op == "%") op = bothSigned ? ir::Op::MOD : ir::Op::MOD_U;
    else if (node.op == "&") op = ir::Op::AND;
    else if (node.op == "|") op = ir::Op::OR;
    else if (node.op == "^") op = ir::Op::XOR;
    else if (node.op == "<<") op = ir::Op::SHL;
    else if (node.op == ">>") op = bothSigned ? ir::Op::ASR : ir::Op::SHR;
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
    // Skip pre-visit for dereference (*) and address-of (&) — they handle
    // their own operand evaluation with different computeAddressOnly_ state.
    // Pre-visiting would double-evaluate operands with side effects (e.g. *p++
    // would increment p twice and use the wrong address).
    if (node.op != "*" && node.op != "&") {
        node.operand->accept(*this);
    }
    auto src = lastValue_;

    // Unary operator overloading for struct types
    if (node.op == "~" || node.op == "-" || node.op == "!" || node.op == "++" || node.op == "--") {
        IRTypeInfo srcInfo = getExprTypeInfo(node.operand.get());
        if (!srcInfo.typeName.empty() && srcInfo.type != ir::Type::PTR) {
            std::string sName = getAggregateName(srcInfo.typeName);
            if (structs_.count(sName)) {
                static const std::map<std::string, std::string> unaryOpMap = {
                    {"~", "bnot"}, {"-", "neg"}, {"!", "lnot"}, {"++", "inc"}, {"--", "dec"},
                };
                auto opIt = unaryOpMap.find(node.op);
                if (opIt != unaryOpMap.end()) {
                    std::string mangledName = sName + "__operator_" + opIt->second;
                    if (functionReturnTypes_.count(mangledName)) {
                        computeAddressOnly_ = true;
                        node.operand->accept(*this);
                        computeAddressOnly_ = false;
                        auto thisPtr = lastValue_;
                        auto retType = functionReturnTypes_[mangledName];
                        auto dest = allocVreg(retType);
                        ir::Inst call;
                        call.op = (retType == ir::Type::VOID) ? ir::Op::CALL_VOID : ir::Op::CALL;
                        call.dest = dest;
                        call.resultType = retType;
                        call.src1 = ir::Operand::global("_" + mangledName);
                        call.args = {thisPtr};
                        call.callConv = ir::CallConv::STACK;
                        call.loc = loc(node);
                        emit(call);
                        lastValue_ = dest;
                        return;
                    }
                }
            }
        }
    }

    if (node.op == "-") {
        auto dest = allocVreg(src.type);
        ir::Inst inst;
        inst.op = (src.type == ir::Type::F32) ? ir::Op::FNEG : ir::Op::NEG;
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
            auto rit = localRegister_.find(vr->name);
            if (rit != localRegister_.end() && rit->second) {
                errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column,
                    Severity::Error, "Compile Error: Cannot take address of register variable"));
                return;
            }
            auto grit = globalRegister_.find(vr->name);
            if (grit != globalRegister_.end() && grit->second) {
                errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column,
                    Severity::Error, "Compile Error: Cannot take address of register variable"));
                return;
            }
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
        } else if (auto* ma = dynamic_cast<MemberAccess*>(node.operand.get())) {
            IRTypeInfo baseInfo = getExprTypeInfo(ma->structExpr.get());
            std::string sName = getAggregateName(baseInfo.typeName);
            int accOffset = 0;
            auto* mit = findStructMember(sName, ma->memberName, accOffset);
            if (mit && mit->bitWidth > 0) {
                errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column,
                    Severity::Error, "Compile Error: Cannot take address of bitfield member"));
                return;
            }
            bool oldAddrMode = computeAddressOnly_;
            computeAddressOnly_ = true;
            node.operand->accept(*this);
            computeAddressOnly_ = oldAddrMode;
        } else {
            // General case (e.g., &arr[i]):
            // re-visit with computeAddressOnly_ to get the address
            bool oldAddrMode = computeAddressOnly_;
            computeAddressOnly_ = true;
            node.operand->accept(*this);
            computeAddressOnly_ = oldAddrMode;
        }
    } else if (node.op == "*") {
        // Dereference
        bool oldAddrMode = computeAddressOnly_;
        computeAddressOnly_ = false;
        node.operand->accept(*this);
        auto src = lastValue_;
        computeAddressOnly_ = oldAddrMode;

        if (computeAddressOnly_) {
            // We want the address this pointer points to for an indirect store.
            // If src is a mutable local slot vreg, emit DEREF to read its value
            // into a fresh non-slot vreg. The STORE handler then correctly uses
            // indirect addressing. DEREF is not eliminable by COPY chain optimization.
            // If src is already a computed value (e.g. from p++ snapshot), pass through.
            if (src.isVreg() && currentFunc_ &&
                currentFunc_->localSlotVregs.count(src.vregId)) {
                auto addr = allocVreg(ir::Type::PTR);
                ir::Inst deref;
                deref.op = ir::Op::DEREF;
                deref.dest = addr;
                deref.resultType = ir::Type::PTR;
                deref.src1 = src;
                deref.loc = loc(node);
                emit(deref);
                lastValue_ = addr;
            } else {
                lastValue_ = src;
            }
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

        if (auto* vr = dynamic_cast<VariableReference*>(node.operand.get())) {
            auto cit = localConst_.find(vr->name);
            if (cit != localConst_.end() && cit->second) {
                errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column,
                    Severity::Error, "Compile Error: Assignment to read-only location"));
            }
        }
        if (auto* deref = dynamic_cast<UnaryOperation*>(node.operand.get())) {
            if (deref->op == "*") {
                if (auto* vr = dynamic_cast<VariableReference*>(deref->operand.get())) {
                    auto pit = localPointsToConst_.find(vr->name);
                    if (pit != localPointsToConst_.end() && pit->second) {
                        auto ptit = localPointedToType_.find(vr->name);
                        bool isMultiPtr = (ptit != localPointedToType_.end() && ptit->second == ir::Type::PTR);
                        if (!isMultiPtr) {
                            errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column,
                                Severity::Error, "Compile Error: Increment/decrement of read-only location"));
                        }
                    }
                }
            }
        }
        if (auto* ma = dynamic_cast<MemberAccess*>(node.operand.get())) {
            IRTypeInfo baseInfo = getExprTypeInfo(ma->structExpr.get());
            std::string sName = getAggregateName(baseInfo.typeName);
            int accOffset = 0;
            auto* mit = findStructMember(sName, ma->memberName, accOffset);
            if (mit && mit->isConst) {
                errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column,
                    Severity::Error, "Compile Error: Increment/decrement of read-only member"));
            }
        }

        // For pointer increment/decrement, scale by pointed-to element size
        int incVal = 1;
        if (src.type == ir::Type::PTR) {
            auto info = getExprTypeInfo(node.operand.get());
            if (info.pointedToType != ir::Type::VOID) {
                incVal = ir::typeSize(info.pointedToType);
            }
            // Check for struct/union pointed-to types with larger sizes
            if (!info.typeName.empty()) {
                std::string ptName = info.typeName;
                // Strip pointer suffix if present
                std::string sName = getAggregateName(ptName);
                auto sit = structs_.find(sName);
                if (sit != structs_.end()) {
                    incVal = sit->second.totalSize;
                }
            }
        }

        // For post-increment: snapshot the old value BEFORE computing the
        // incremented value using DEREF. Then ADD from the snapshot so the
        // optimizer can't CSE two increments of the same mutable slot.
        ir::Operand addSrc = src;
        ir::Operand oldVal;
        if (isPost && src.isVreg()) {
            oldVal = allocVreg(src.type);
            ir::Inst deref;
            deref.op = ir::Op::DEREF;
            deref.dest = oldVal;
            deref.resultType = src.type;
            deref.src1 = src;
            deref.loc = loc(node);
            emit(deref);
            addSrc = oldVal; // ADD from snapshot, not from mutable slot
        }

        auto dest = allocVreg(src.type);
        ir::Inst inst;
        inst.op = (baseOp == "++") ? ir::Op::ADD : ir::Op::SUB;
        inst.dest = dest;
        inst.resultType = src.type;
        inst.src1 = addSrc;
        inst.src2 = ir::Operand::imm(incVal, src.type);
        inst.loc = loc(node);
        emit(inst);

        // Store back to the variable (handle bitfield specially)
        if (auto* ma = dynamic_cast<MemberAccess*>(node.operand.get())) {
            // Check if this is a bitfield member
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
                // Bitfield store: use BFINS
                ma->structExpr->accept(*this);
                auto base = lastValue_;
                auto addrVreg = allocVreg(ir::Type::PTR);
                ir::Inst add;
                add.op = ir::Op::ADD;
                add.dest = addrVreg;
                add.resultType = ir::Type::PTR;
                add.src1 = base;
                add.src2 = ir::Operand::imm(memberOffset, ir::Type::I16);
                add.loc = loc(node);
                emit(add);

                ir::Inst bfins;
                bfins.op = ir::Op::BFINS;
                bfins.resultType = memberType;
                auto bfinsResult = allocVreg(memberType);
                bfins.dest = bfinsResult;
                bfins.src1 = dest;  // The incremented value
                bfins.src2 = addrVreg;
                bfins.args.push_back(ir::Operand::imm(bitOffset, ir::Type::I8));
                bfins.args.push_back(ir::Operand::imm(bitWidth, ir::Type::I8));
                bfins.loc = loc(node);
                emit(bfins);

                // Store the modified value back
                ir::Inst store;
                store.op = ir::Op::STORE;
                store.src1 = bfinsResult;
                store.src2 = addrVreg;
                store.resultType = ir::Type::VOID;
                store.loc = loc(node);
                emit(store);

                lastValue_ = isPost ? oldVal : dest;
                return;
            }
        }

        // Regular (non-bitfield) store
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

        lastValue_ = isPost ? oldVal : dest;
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
    // Struct method call: p.method(args) or ptr->method(args)
    // callExpr is MemberAccess, check if member is a known method
    if (node.callExpr) {
        if (auto* ma = dynamic_cast<MemberAccess*>(node.callExpr.get())) {
            // Look up struct type of the LHS
            IRTypeInfo baseInfo = getExprTypeInfo(ma->structExpr.get());
            std::string sName = getAggregateName(baseInfo.typeName);
            // Check if this member is a method (mangled name exists as a function)
            std::string mangledName = sName + "__" + ma->memberName;
            std::string irName = "_" + mangledName;
            if (functionReturnTypes_.count(mangledName)) {
                // Compute 'this' pointer
                bool oldAddrMode = computeAddressOnly_;
                if (!ma->isArrow) {
                    computeAddressOnly_ = true;
                }
                ma->structExpr->accept(*this);
                computeAddressOnly_ = oldAddrMode;
                auto thisPtr = lastValue_;

                // Build argument list with 'this' prepended
                std::vector<ir::Operand> args;
                args.push_back(thisPtr);
                for (auto& arg : node.arguments) {
                    arg->accept(*this);
                    args.push_back(lastValue_);
                }

                auto retType = functionReturnTypes_[mangledName];

                // Check if this is a virtual method
                // Direct call if: method is final, struct is final, or hierarchy sealed
                bool isVirtualCall = false;
                int vtSlot = -1;
                auto fit = allFunctions_.find(mangledName);
                if (fit != allFunctions_.end() && fit->second->isVirtual) {
                    bool canDevirt = fit->second->isFinal;
                    // #6: Final struct propagation
                    if (!canDevirt && fit->second->isMethod &&
                        parsedStructIsFinal_.count(fit->second->methodStructName) &&
                        parsedStructIsFinal_[fit->second->methodStructName]) {
                        canDevirt = true;
                    }
                    if (!canDevirt) {
                        isVirtualCall = true;
                        vtSlot = fit->second->vtableSlot;
                    }
                }

                ir::Inst inst;
                inst.args = args;
                inst.loc = loc(node);
                inst.callConv = ir::CallConv::STACK;

                if (isVirtualCall && vtSlot >= 0) {
                    // Virtual dispatch: load vtable ptr from this->__vt, index by slot
                    // this->__vt is at offset 0 of the struct
                    auto vtAddr = allocVreg(ir::Type::PTR);
                    ir::Inst loadVt;
                    loadVt.op = ir::Op::LOAD;
                    loadVt.dest = vtAddr;
                    loadVt.resultType = ir::Type::PTR;
                    loadVt.src1 = thisPtr; // __vt is at offset 0
                    loadVt.loc = loc(node);
                    emit(loadVt);

                    // Index into vtable: vtable + slot * 2
                    auto slotAddr = allocVreg(ir::Type::PTR);
                    ir::Inst addSlot;
                    addSlot.op = ir::Op::ADD;
                    addSlot.dest = slotAddr;
                    addSlot.resultType = ir::Type::PTR;
                    addSlot.src1 = vtAddr;
                    addSlot.src2 = ir::Operand::imm(vtSlot * 2, ir::Type::I16);
                    addSlot.loc = loc(node);
                    emit(addSlot);

                    // Load function pointer from vtable slot
                    auto funcPtr = allocVreg(ir::Type::PTR);
                    ir::Inst loadFp;
                    loadFp.op = ir::Op::LOAD;
                    loadFp.dest = funcPtr;
                    loadFp.resultType = ir::Type::PTR;
                    loadFp.src1 = slotAddr;
                    loadFp.loc = loc(node);
                    emit(loadFp);

                    // Indirect call through vtable entry
                    inst.op = ir::Op::CALL_INDIRECT;
                    inst.src1 = funcPtr;
                } else {
                    // Direct call — check for inline expansion of trivial methods
                    auto inlineIt = inlineCandidates_.find(mangledName);
                    if (inlineIt != inlineCandidates_.end() &&
                        inlineExpansionStack_.find(mangledName) == inlineExpansionStack_.end()) {
                        // Inline expand: temporarily bind params to args
                        FunctionDeclaration* inlineFunc = inlineIt->second;
                        inlineExpansionStack_.insert(mangledName);
                        auto savedLocals = locals_;
                        auto savedLocalTypes = localTypes_;
                        auto savedLocalTypeNames = localTypeNames_;
                        auto savedLocalSigned = localSigned_;
                        auto savedLocalConst = localConst_;

                        // Bind this + args to param names
                        for (size_t pi = 0; pi < inlineFunc->parameters.size() && pi < args.size(); pi++) {
                            locals_[inlineFunc->parameters[pi].name] = args[pi];
                            localTypes_[inlineFunc->parameters[pi].name] = args[pi].type;
                            localTypeNames_[inlineFunc->parameters[pi].name] = inlineFunc->parameters[pi].type;
                            localSigned_[inlineFunc->parameters[pi].name] = inlineFunc->parameters[pi].isSigned;
                        }

                        auto inlineResult = allocVreg(retType);
                        inlineReturnTarget_ = inlineResult;
                        inlineReturnLabel_ = newLabel("inline_ret");

                        if (inlineFunc->body) inlineFunc->body->accept(*this);

                        startBlock(inlineReturnLabel_);
                        lastValue_ = inlineResult;

                        locals_ = savedLocals;
                        localTypes_ = savedLocalTypes;
                        localTypeNames_ = savedLocalTypeNames;
                        localSigned_ = savedLocalSigned;
                        localConst_ = savedLocalConst;
                        inlineExpansionStack_.erase(mangledName);
                        return;
                    }
                    inst.op = ir::Op::CALL;
                    inst.src1 = ir::Operand::global(irName);
                }

                auto dest = allocVreg(retType);
                inst.dest = dest;
                inst.resultType = retType;
                emit(inst);
                lastValue_ = dest;
                return;
            }
        }
    }

    // DMA intrinsics: __dma_copy(dst, src, len), __dma_fill(dst, len, val)
    if (node.name == "__dma_copy" && node.arguments.size() == 3) {
        // Evaluate args: dst (ptr), src (ptr), len (int)
        node.arguments[0]->accept(*this); auto dst = lastValue_;
        node.arguments[1]->accept(*this); auto src = lastValue_;
        node.arguments[2]->accept(*this); auto len = lastValue_;

        // Build DMA copy job inline via assembly:
        // Store src/dst/len to ZP scratch area, then build 12-byte job on stack
        auto emitAsm = [&](const std::string& line) {
            ir::Inst a; a.op = ir::Op::ASM_INLINE; a.asmText = line; a.loc = loc(node); emit(a);
        };

        // Save args to ZP $02-$07 (scratch area, safe for DMA setup)
        ir::Inst storeSrc; storeSrc.op = ir::Op::STORE; storeSrc.resultType = ir::Type::I16;
        storeSrc.src1 = src; storeSrc.src2 = ir::Operand::global("$02"); storeSrc.loc = loc(node); emit(storeSrc);
        ir::Inst storeDst; storeDst.op = ir::Op::STORE; storeDst.resultType = ir::Type::I16;
        storeDst.src1 = dst; storeDst.src2 = ir::Operand::global("$04"); storeDst.loc = loc(node); emit(storeDst);
        ir::Inst storeLen; storeLen.op = ir::Op::STORE; storeLen.resultType = ir::Type::I16;
        storeLen.src1 = len; storeLen.src2 = ir::Operand::global("$06"); storeLen.loc = loc(node); emit(storeLen);

        // Build 12-byte F018B DMA job on stack and trigger
        emitAsm("pha");                          // save A
        emitAsm("lda #0");    emitAsm("pha");    // modulo MSB
        emitAsm("pha");                          // modulo LSB
        emitAsm("pha");                          // command MSB
        emitAsm("pha");                          // dest bank
        emitAsm("lda $05");   emitAsm("pha");    // dest addr hi
        emitAsm("lda $04");   emitAsm("pha");    // dest addr lo
        emitAsm("lda #0");    emitAsm("pha");    // src bank
        emitAsm("lda $03");   emitAsm("pha");    // src addr hi
        emitAsm("lda $02");   emitAsm("pha");    // src addr lo
        emitAsm("lda $07");   emitAsm("pha");    // length hi
        emitAsm("lda $06");   emitAsm("pha");    // length lo
        emitAsm("lda #$00");  emitAsm("pha");    // command: copy=0x00
        // Trigger: point DMA controller at stack
        emitAsm("tsx");
        emitAsm("txa");
        emitAsm("clc");
        emitAsm("adc #1");                       // SP+1 = job start
        emitAsm("sta $D701");                    // DMA addr lo
        emitAsm("lda #$01");                     // stack page = $01
        emitAsm("sta $D702");                    // DMA addr mi
        emitAsm("stz $D703");                    // DMA addr hi
        emitAsm("stz $D700");                    // trigger DMA
        // Clean up stack (13 bytes pushed)
        emitAsm("tsx");
        emitAsm("txa");
        emitAsm("clc");
        emitAsm("adc #13");
        emitAsm("tax");
        emitAsm("txs");
        emitAsm("pla");                          // restore A
        return;
    }

    if (node.name == "__dma_fill" && node.arguments.size() == 3) {
        // Evaluate args: dst (ptr), len (int), val (char)
        node.arguments[0]->accept(*this); auto dst = lastValue_;
        node.arguments[1]->accept(*this); auto len = lastValue_;
        node.arguments[2]->accept(*this); auto val = lastValue_;

        auto emitAsm = [&](const std::string& line) {
            ir::Inst a; a.op = ir::Op::ASM_INLINE; a.asmText = line; a.loc = loc(node); emit(a);
        };

        // Save args to ZP $02-$06
        ir::Inst storeDst; storeDst.op = ir::Op::STORE; storeDst.resultType = ir::Type::I16;
        storeDst.src1 = dst; storeDst.src2 = ir::Operand::global("$04"); storeDst.loc = loc(node); emit(storeDst);
        ir::Inst storeLen; storeLen.op = ir::Op::STORE; storeLen.resultType = ir::Type::I16;
        storeLen.src1 = len; storeLen.src2 = ir::Operand::global("$06"); storeLen.loc = loc(node); emit(storeLen);
        ir::Inst storeVal; storeVal.op = ir::Op::STORE; storeVal.resultType = ir::Type::I8;
        storeVal.src1 = val; storeVal.src2 = ir::Operand::global("$02"); storeVal.loc = loc(node); emit(storeVal);

        // Build 12-byte F018B DMA fill job on stack and trigger
        emitAsm("pha");                          // save A
        emitAsm("lda #0");    emitAsm("pha");    // modulo MSB
        emitAsm("pha");                          // modulo LSB
        emitAsm("pha");                          // command MSB
        emitAsm("pha");                          // dest bank
        emitAsm("lda $05");   emitAsm("pha");    // dest addr hi
        emitAsm("lda $04");   emitAsm("pha");    // dest addr lo
        emitAsm("lda #0");    emitAsm("pha");    // src bank (ignored for fill)
        emitAsm("lda $02");   emitAsm("pha");    // fill value (in src addr lo)
        emitAsm("lda #0");    emitAsm("pha");    // fill value hi (ignored)
        emitAsm("lda $07");   emitAsm("pha");    // length hi
        emitAsm("lda $06");   emitAsm("pha");    // length lo
        emitAsm("lda #$03");  emitAsm("pha");    // command: fill=0x03
        // Trigger DMA
        emitAsm("tsx");
        emitAsm("txa");
        emitAsm("clc");
        emitAsm("adc #1");
        emitAsm("sta $D701");
        emitAsm("lda #$01");
        emitAsm("sta $D702");
        emitAsm("stz $D703");
        emitAsm("stz $D700");
        // Clean up stack
        emitAsm("tsx");
        emitAsm("txa");
        emitAsm("clc");
        emitAsm("adc #13");
        emitAsm("tax");
        emitAsm("txs");
        emitAsm("pla");                          // restore A
        return;
    }

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
                                warnings_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column,
                                    Severity::Warning, "passing argument discards 'const' qualifier"));
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
    inst.isRegparm = regparmFunctions_.count(node.name) > 0;

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
    // Handle nested function static link
    auto fit = allFunctions_.find(node.name);
    if (fit != allFunctions_.end() && fit->second->isNested) {
        FunctionDeclaration* calleeParent = fit->second->parentFunc;
        ir::Operand sl;

        if (currentFunc_ && "_" + calleeParent->name == currentFunc_->name) {
            // Case 1: Calling a direct child. SL = current FP.
            sl = allocVreg(ir::Type::PTR);
            ir::Inst getFp;
            getFp.op = ir::Op::GET_FP;
            getFp.dest = sl;
            getFp.resultType = ir::Type::PTR;
            getFp.loc = loc(node);
            emit(getFp);
        } else {
            // Case 2: Sibling or ancestor's child. 
            // Follow static chain from current function's SL.
            if (currentFunc_ && currentFunc_->isNested) {
                ir::Operand currentSL;
                currentSL.kind = ir::OperandKind::VREG;
                currentSL.vregId = currentFunc_->staticLinkVreg;
                currentSL.type = ir::Type::PTR;

                // How many levels up to go?
                // We need to go from currentFunc_->parent to calleeParent.
                int levels = 0;
                auto cit = allFunctions_.find(currentFunc_->name.substr(1));
                FunctionDeclaration* curr = (cit != allFunctions_.end()) ? cit->second->parentFunc : nullptr;
                while (curr && curr != calleeParent) {
                    curr = curr->parentFunc;
                    levels++;
                }

                if (levels == 0) {
                    // Sibling: just use our own SL
                    sl = currentSL;
                } else {
                    // Ancestor: go up 'levels' times
                    sl = allocVreg(ir::Type::PTR);
                    ir::Inst addrUp;
                    addrUp.op = ir::Op::ADDR_UPLEVEL;
                    addrUp.dest = sl;
                    addrUp.resultType = ir::Type::PTR;
                    addrUp.src1 = currentSL;
                    // Pack levels. In this case, we just want the frame base, so vregId is -1
                    addrUp.args = { ir::Operand::vreg(0xFFFFFFFF, ir::Type::PTR), ir::Operand::imm(levels, ir::Type::I16) };
                    addrUp.loc = loc(node);
                    emit(addrUp);
                }
            } else {
                // Should not happen for valid C: global calling nested without pointer
                sl = ir::Operand::imm(0, ir::Type::PTR);
            }
        }
        
        // Pass via __static_chain ZP register
        ir::Inst storeSl;
        storeSl.op = ir::Op::STORE_ZP;
        storeSl.src1 = sl;
        storeSl.src2 = ir::Operand::global("__static_chain");
        storeSl.loc = loc(node);
        emit(storeSl);
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
        // Check for inline expansion
        auto inlineIt = inlineCandidates_.find(node.name);
        if (inlineIt != inlineCandidates_.end() &&
            inlineExpansionStack_.find(node.name) == inlineExpansionStack_.end()) {

            FunctionDeclaration* inlineFunc = inlineIt->second;

            // Guard against recursion
            inlineExpansionStack_.insert(node.name);

            // Save caller's local state
            auto savedLocals = locals_;
            auto savedLocalTypes = localTypes_;
            auto savedLocalSigned = localSigned_;
            auto savedLocalConst = localConst_;
            auto savedLocalTypeNames = localTypeNames_;
            auto savedLocalPointsToConst = localPointsToConst_;
            auto savedLocalPointedToType = localPointedToType_;

            // Map parameters to argument vregs
            for (size_t i = 0; i < inlineFunc->parameters.size() && i < args.size(); i++) {
                const auto& p = inlineFunc->parameters[i];
                ir::Type pt = mapType(p.type, p.pointerLevel);
                auto argOp = (i < castArgs.size()) ? castArgs[i] : args[i];

                // Create a local vreg for the parameter and copy the arg into it
                auto paramVreg = allocVreg(pt);
                ir::Inst copy;
                copy.op = ir::Op::COPY;
                copy.dest = paramVreg;
                copy.src1 = argOp;
                copy.resultType = pt;
                copy.loc = loc(node);
                emit(copy);

                locals_[p.name] = paramVreg;
                localTypes_[p.name] = pt;
                localTypeNames_[p.name] = p.type;
                localSigned_[p.name] = p.isSigned;
                localConst_[p.name] = (p.pointerLevel == 0 && p.isConst);
                if (p.pointerLevel > 0) localPointedToType_[p.name] = mapType(p.type, 0);
            }

            // Create a result vreg for the return value
            ir::Type retType = ir::Type::I16;
            auto retIt = functionReturnTypes_.find(node.name);
            if (retIt != functionReturnTypes_.end()) retType = retIt->second;

            ir::Operand resultVreg;
            if (retType != ir::Type::VOID) {
                resultVreg = allocVreg(retType);
            }

            // Create merge label for return statements
            std::string mergeLabel = newLabel("inline_end");

            // Save and set inline return context
            auto savedInlineReturnTarget = inlineReturnTarget_;
            auto savedInlineReturnLabel = inlineReturnLabel_;
            inlineReturnTarget_ = resultVreg;
            inlineReturnLabel_ = mergeLabel;

            // Visit the inlined function body
            inlineFunc->body->accept(*this);

            // Emit merge block
            startBlock(mergeLabel);

            // Restore caller state
            inlineReturnTarget_ = savedInlineReturnTarget;
            inlineReturnLabel_ = savedInlineReturnLabel;
            locals_ = savedLocals;
            localTypes_ = savedLocalTypes;
            localSigned_ = savedLocalSigned;
            localConst_ = savedLocalConst;
            localTypeNames_ = savedLocalTypeNames;
            localPointsToConst_ = savedLocalPointsToConst;
            localPointedToType_ = savedLocalPointedToType;

            inlineExpansionStack_.erase(node.name);

            if (retType != ir::Type::VOID) {
                lastValue_ = resultVreg;
            }
        } else {
            // Check if this name is a variable (function pointer) rather than a function
            bool isVariable = false;
            if (locals_.count(node.name) || globalTypes_.count(node.name)) {
                if (!definedFunctions_.count("_" + node.name) &&
                    !functionReturnTypes_.count(node.name)) {
                    isVariable = true;
                }
            }

            if (isVariable) {
                // Indirect call via function pointer variable
                // Load the pointer value from the variable
                auto varIt = locals_.find(node.name);
                ir::Operand ptrVal;
                if (varIt != locals_.end()) {
                    // Local function pointer variable
                    auto addr = allocVreg(ir::Type::PTR);
                    ir::Inst loadInst;
                    loadInst.op = ir::Op::LOAD;
                    loadInst.dest = addr;
                    loadInst.resultType = ir::Type::PTR;
                    loadInst.src1 = varIt->second;
                    loadInst.loc = loc(node);
                    emit(loadInst);
                    ptrVal = addr;
                } else {
                    // Global function pointer variable — load from global
                    auto addr = allocVreg(ir::Type::PTR);
                    ir::Inst loadInst;
                    loadInst.op = ir::Op::LOAD;
                    loadInst.dest = addr;
                    loadInst.resultType = ir::Type::PTR;
                    loadInst.src1 = ir::Operand::global("_" + node.name);
                    loadInst.loc = loc(node);
                    emit(loadInst);
                    ptrVal = addr;
                }
                inst.src1 = ptrVal;
                auto dest = allocVreg(ir::Type::I16);
                inst.op = ir::Op::CALL_INDIRECT;
                inst.callConv = ir::CallConv::STACK;
                inst.dest = dest;
                inst.resultType = ir::Type::I16;
                emit(inst);
                lastValue_ = dest;
                lastValueSigned_ = true; // function pointer returns signed int by default
            } else {
                // Normal direct call path
                std::string mangledName = "_" + node.name;
                calledFunctions_.insert(mangledName);
                inst.src1 = ir::Operand::global(mangledName);

                ir::Type retType = ir::Type::I16;
                auto it = functionReturnTypes_.find(node.name);
                if (it != functionReturnTypes_.end()) {
                    retType = it->second;
                } else {
                    warnings_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column,
                        Severity::Warning, "implicit declaration of function '" + node.name + "'"));
                }

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
                    // Function returns are signed by default (int, long)
                    // unless the return type is explicitly unsigned
                    lastValueSigned_ = true;
                }
            }
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
    // Inline expansion: return stores to inline result vreg and branches to merge
    if (!inlineReturnLabel_.empty()) {
        if (node.expression) {
            node.expression->accept(*this);
            auto val = lastValue_;

            ir::Type retType = ir::Type::I16;
            if (currentFunc_) retType = currentFunc_->returnType;

            IRTypeInfo info = getExprTypeInfo(node.expression.get());
            auto castVal = emitCast(val, retType, info.isSigned);

            ir::Inst copy;
            copy.op = ir::Op::COPY;
            copy.dest = inlineReturnTarget_;
            copy.src1 = castVal;
            copy.resultType = retType;
            copy.loc = loc(node);
            emit(copy);
        }
        ir::Inst br;
        br.op = ir::Op::BR;
        br.src1 = ir::Operand::label(inlineReturnLabel_);
        br.loc = loc(node);
        emit(br);
        // Start a new block for any code after this return (dead, but keeps IR well-formed)
        startBlock(newLabel("inline_after_ret"));
        return;
    }

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

// Emit condition branches for if/while — chains && and || directly
// without boolean materialization. For simple conditions, evaluates
// and emits BR_COND.
void IRBuilder::emitConditionBranches(Expression* cond, const std::string& trueLabel,
                                      const std::string& falseLabel, ir::SourceLoc sl) {
    // Check for && / ||
    if (auto* bin = dynamic_cast<BinaryOperation*>(cond)) {
        if (bin->op == "&&") {
            // A && B: if A is false → falseLabel; else test B
            std::string rhsLabel = newLabel("and_rhs");
            emitConditionBranches(bin->left.get(), rhsLabel, falseLabel, sl);
            startBlock(rhsLabel);
            emitConditionBranches(bin->right.get(), trueLabel, falseLabel, sl);
            return;
        }
        if (bin->op == "||") {
            // A || B: if A is true → trueLabel; else test B
            std::string rhsLabel = newLabel("or_rhs");
            emitConditionBranches(bin->left.get(), trueLabel, rhsLabel, sl);
            startBlock(rhsLabel);
            emitConditionBranches(bin->right.get(), trueLabel, falseLabel, sl);
            return;
        }
    }
    // Check for !cond — swap targets
    if (auto* unary = dynamic_cast<UnaryOperation*>(cond)) {
        if (unary->op == "!") {
            emitConditionBranches(unary->operand.get(), falseLabel, trueLabel, sl);
            return;
        }
    }

    // Simple condition — evaluate and branch
    cond->accept(*this);
    auto val = lastValue_;
    ir::Inst br;
    br.op = ir::Op::BR_COND;
    br.src1 = val;
    br.dest = ir::Operand::label(trueLabel);
    br.src2 = ir::Operand::label(falseLabel);
    br.loc = sl;
    emit(br);
}

void IRBuilder::visit(IfStatement& node) {
    std::string thenLabel = newLabel("if_then");
    std::string elseLabel = newLabel("if_else");
    std::string endLabel = newLabel("if_end");
    std::string falseTarget = node.elseBranch ? elseLabel : endLabel;

    // Optimization: for && and || conditions, emit chained branches directly
    // to the if's true/false targets — no boolean materialization.
    emitConditionBranches(node.condition.get(), thenLabel, falseTarget, loc(node));

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
    emitConditionBranches(node.condition.get(), bodyLabel, endLabel, loc(node));

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

void IRBuilder::visit(RepeatStatement& node) {
    if (node.count <= 0) return; // repeat(0) = no-op

    if (node.varName.empty()) {
        // Simple repeat(N) { body } — unroll N times
        for (int i = 0; i < node.count; i++) {
            node.body->accept(*this);
        }
    } else {
        // repeat(type var, N) { body } — unroll with var = 1..N
        ir::Type varType = mapType(node.varType, 0);
        auto vreg = allocVreg(varType);
        locals_[node.varName] = vreg;
        localTypes_[node.varName] = varType;
        localTypeNames_[node.varName] = node.varType;
        localSigned_[node.varName] = node.varSigned;
        if (currentFunc_) {
            currentFunc_->localNames[node.varName] = vreg.vregId;
            currentFunc_->localSlotVregs.insert(vreg.vregId);
        }

        for (int i = 1; i <= node.count; i++) {
            // Set var = i as a constant
            ir::Inst constInst;
            constInst.op = ir::Op::CONST;
            constInst.dest = vreg;
            constInst.resultType = varType;
            constInst.src1 = ir::Operand::imm(i, varType);
            constInst.loc = loc(node);
            emit(constInst);

            // Store to the vreg slot
            ir::Inst store;
            store.op = ir::Op::STORE;
            store.resultType = varType;
            store.src1 = ir::Operand::imm(i, varType);
            store.src2 = vreg;
            store.loc = loc(node);
            emit(store);

            node.body->accept(*this);
        }
    }
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
    ir::Operand thenVal;
    if (node.thenExpr) {
        node.thenExpr->accept(*this);
        thenVal = lastValue_;
    } else {
        // Elvis operator (expr ?: default): use condition value as then-result
        thenVal = cond;
    }
    
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
    IRTypeInfo controlInfo = getExprTypeInfo(node.control.get());
    int controlPtrLevel = (controlInfo.type == ir::Type::PTR) ? ((controlInfo.pointedToType == ir::Type::PTR) ? 2 : 1) : 0;
    const GenericAssociation* selected = nullptr;
    for (const auto& assoc : node.associations) {
        if (!assoc.isDefault) {
            std::string atn = assoc.typeName;
            std::string ctn = controlInfo.typeName;
            if (atn == "unsigned") atn = "unsigned int";
            if (ctn == "unsigned") ctn = "unsigned int";
            if (atn == "signed") atn = "int";
            if (ctn == "signed") ctn = "int";
            if (atn == ctn && assoc.pointerLevel == controlPtrLevel) {
                selected = &assoc;
                break;
            }
        }
    }
    if (!selected) {
        for (const auto& assoc : node.associations) {
            if (assoc.isDefault) {
                selected = &assoc;
                break;
            }
        }
    }
    if (!selected) {
        throw std::runtime_error("No matching association in _Generic selection");
    }
    if (selected->result) selected->result->accept(*this);
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

            // For pointer arrays (e.g. char *arr[7]), each element is a pointer (2 bytes),
            // not the pointed-to type size.  Detect via localPointedToType_/globalPointedToType_.
            bool isPointerElement = localPointedToType_.count(ref->name) || globalPointedToType_.count(ref->name);
            int stride = isPointerElement ? 2 : getTypeSize(tn, 0);
            for (size_t i = depth; i < dims.size(); i++) {
                stride *= dims[i];
            }
            elemSize = stride;
        } else {
            elemSize = ir::typeSize(baseType);
        }
    } else if (auto* ma = dynamic_cast<MemberAccess*>(root)) {
        // Array member of a struct (e.g., sid1->voice[1], vic4->sprite[0])
        // Look up the member's element type from struct info
        IRTypeInfo parentInfo = getExprTypeInfo(ma->structExpr.get());
        std::string parentName = getAggregateName(parentInfo.typeName);
        auto psit = structs_.find(parentName);
        if (psit != structs_.end()) {
            auto pmit = psit->second.members.find(ma->memberName);
            if (pmit != psit->second.members.end()) {
                elemSize = getTypeSize(pmit->second.type, pmit->second.pointerLevel);
            }
        }
    } else {
        // General case: derive element size from expression type info
        // Handles cast-pointer subscript: ((unsigned char *)0xD800)[n]
        IRTypeInfo arrInfo = getExprTypeInfo(node.arrayExpr.get());
        if (arrInfo.type == ir::Type::PTR && arrInfo.pointedToType != ir::Type::VOID) {
            elemSize = ir::typeSize(arrInfo.pointedToType);
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

    IRTypeInfo baseInfo = getExprTypeInfo(node.structExpr.get());
    bool isBaseStructOrUnion = (baseInfo.typeName.rfind("struct ", 0) == 0 || baseInfo.typeName.rfind("union ", 0) == 0);
    bool valid = isBaseStructOrUnion && (node.isArrow ? (baseInfo.type == ir::Type::PTR) : (baseInfo.type != ir::Type::PTR));
    if (!valid) {
        throw std::runtime_error("Error at line " + std::to_string(node.line) + ": Dot/Arrow operator on non-struct type");
    }

    std::string sName = getAggregateName(baseInfo.typeName);
    auto sit = structs_.find(sName);
    if (sit == structs_.end()) {
        throw std::runtime_error("Error at line " + std::to_string(node.line) + ": Unknown struct/union type '" + sName + "'");
    }

    int accumulatedOffset = 0;
    auto* mit = findStructMember(sName, node.memberName, accumulatedOffset);
    if (!mit) {
        throw std::runtime_error("Error at line " + std::to_string(node.line) + ": Member '" + node.memberName + "' not found in struct '" + sName + "'");
    }

    int memberOffset = mit->offset + accumulatedOffset;
    int bitWidth = mit->bitWidth;
    int bitOffset = mit->bitOffset;
    bool memberIsArray = !mit->arrayDims.empty();
    bool memberIsStruct = structs_.count(mit->type) > 0;

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

    // Array/struct members decay to their address (no LOAD needed)
    if (computeAddressOnly_ || memberIsArray || memberIsStruct) {
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
    bool isStructOrUnion = (node.targetType.rfind("struct ", 0) == 0 || node.targetType.rfind("union ", 0) == 0);
    if (isStructOrUnion && node.pointerLevel == 0) {
        std::string sName = getAggregateName(node.targetType);
        if (!structs_.count(sName)) {
            throw std::runtime_error("Error at line " + std::to_string(node.line) + ": Unknown struct type: " + node.targetType);
        }
    }

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
        if (node.pointerLevel == 0) {
            for (int d : node.arrayDims) {
                if (d > 0) sz *= d;
            }
        }
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
    void visit(FloatLiteral&) override {}
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
    void visit(RepeatStatement& node) override { if (node.body) node.body->accept(*this); }
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
    void visit(LabelAddressExpression&) override {}
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
    if (node.target) {
        // Computed goto: goto *expr
        node.target->accept(*this);
        ir::Inst inst;
        inst.op = ir::Op::BR_INDIRECT;
        inst.src1 = lastValue_;
        inst.loc = loc(node);
        emit(inst);
        return;
    }
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
    if (node.code == ".weak_next") {
        weakNextFunction_ = true;
        return;
    }
    if (node.code == ".no_zp_save") {
        module_.saveZP = false;
        return;
    }
    if (node.code.rfind(".set_bp ", 0) == 0) {
        module_.setBP = std::stoi(node.code.substr(8));
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

    // Phase 3: Inherit parent struct members at offset 0
    if (!node.parentStruct.empty()) {
        auto pit = structs_.find(node.parentStruct);
        if (pit != structs_.end()) {
            const auto& parentInfo = pit->second;
            for (const auto& pName : parentInfo.memberOrder) {
                info.members[pName] = parentInfo.members.at(pName);
                info.memberOrder.push_back(pName);
            }
            currentOffset = parentInfo.totalSize;
        }
    }

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
    parsedStructIsFinal_[node.name] = node.isFinal;

    // Emit struct methods as top-level functions
    for (auto& method : node.methods) {
        method->accept(*this);
    }

    // Phase 3: Generate vtable for structs with virtual methods
    if (node.hasVirtual) {
        // Collect vtable entries: slot → function name
        int maxSlot = -1;
        std::map<int, std::string> vtableEntries;

        // First: inherit parent's vtable entries
        if (!node.parentStruct.empty() && structs_.count(node.parentStruct)) {
            // Look up parent's vtable entries from module globals
            for (const auto& g : module_.globals) {
                if (g.name == "_" + node.parentStruct + "_vtable") {
                    // Copy parent vtable entries
                    // (stored as function name references in vtableMethodNames)
                    break;
                }
            }
        }

        // Then: fill in this struct's methods (overrides replace parent entries)
        for (auto& method : node.methods) {
            if (method->isVirtual && method->vtableSlot >= 0) {
                vtableEntries[method->vtableSlot] = "_" + method->name;
                if (method->vtableSlot > maxSlot) maxSlot = method->vtableSlot;
            }
        }

        // Also inherit parent vtable entries that weren't overridden
        if (!node.parentStruct.empty()) {
            for (const auto& g : module_.globals) {
                if (g.name == "_" + node.parentStruct + "_vtable" && !g.vtableMethodNames.empty()) {
                    for (int i = 0; i < (int)g.vtableMethodNames.size(); i++) {
                        if (!vtableEntries.count(i)) {
                            vtableEntries[i] = g.vtableMethodNames[i];
                            if (i > maxSlot) maxSlot = i;
                        }
                    }
                }
            }
        }

        if (maxSlot >= 0) {
            ir::Module::GlobalVar vtGlobal;
            vtGlobal.name = "_" + node.name + "_vtable";
            vtGlobal.type = ir::Type::I16;
            vtGlobal.size = (maxSlot + 1) * 2; // 2 bytes per function pointer
            vtGlobal.hasInitValue = true;
            vtGlobal.isConst = true;
            // Store method names for vtable entries (resolved by linker/codegen)
            for (int i = 0; i <= maxSlot; i++) {
                if (vtableEntries.count(i)) {
                    vtGlobal.vtableMethodNames.push_back(vtableEntries[i]);
                } else {
                    vtGlobal.vtableMethodNames.push_back(""); // empty slot
                }
            }
            module_.globals.push_back(vtGlobal);
        }
    }
}
void IRBuilder::visit(BuiltinVaStart& node) {
    if (!dynamic_cast<VariableReference*>(node.ap.get())) {
        errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column,
            Severity::Error, "Compile Error: va_start: first argument must be a variable"));
        return;
    }
    if (currentFuncParams_.find(node.lastParamName) == currentFuncParams_.end()) {
        errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column,
            Severity::Error, "Compile Error: va_start: '" + node.lastParamName + "' is not a parameter"));
        return;
    }
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


void IRBuilder::visit(LabelAddressExpression& node) {
    ir::Inst addr;
    addr.op = ir::Op::ADDR_LABEL;
    addr.dest = allocVreg(ir::Type::I16);
    addr.src1 = ir::Operand::label(node.label);
    addr.loc = loc(node);
    emit(addr);
    lastValue_ = addr.dest;
}
