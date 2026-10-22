#include "CompilerLog.hpp"
#include "StructLayout.hpp"
#include "IRBuilder.hpp"
#include "OptimizationCatalog.hpp"
#include "CBMFloat.hpp"
#include "Diagnostic.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>

static const std::map<std::string, size_t>& printfLikeFunctions();

IRBuilder::IRBuilder() {}

void IRBuilder::setSourceInfo(const std::string& filename) {
    module_.sourceFile = filename;
}

// Helper to output warnings immediately to stderr
void IRBuilder::emitWarning(const std::string& warning) {
    printedWarnings_.insert(warnings_.size());
    warnings_.push_back(warning);
    std::cerr << warning << std::endl;
}

void IRBuilder::generate(TranslationUnit& unit) {
    // Phase 102: Pre-pass to register all struct definitions and typedef mappings
    registerAllStructDefinitions(unit);

    unit.accept(*this);
    // what was declared volatile (by name: a local of the same name only
    // makes CSE keep more loads)
    for (const auto& n : volatileNames_) module_.volatileGlobals.insert("_" + n);
    // Eliminate unused static functions (iterative — handles call chains)
    {
        bool changed = true;
        // Build set of all function names for filtering
        std::set<std::string> allFunctionNames;
        for (const auto& fn : module_.functions) {
            allFunctionNames.insert(fn.name);
        }

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
                            inst.src1.kind == ir::OperandKind::GLOBAL &&
                            allFunctionNames.count(inst.src1.name)) {
                            usedFuncs.insert(inst.src1.name);
                        }
                        // Function name used as value in operands - only add if it's a known function
                        if (inst.src1.kind == ir::OperandKind::GLOBAL && allFunctionNames.count(inst.src1.name)) {
                            usedFuncs.insert(inst.src1.name);
                        }
                        if (inst.src2.kind == ir::OperandKind::GLOBAL && allFunctionNames.count(inst.src2.name)) {
                            usedFuncs.insert(inst.src2.name);
                        }
                        for (const auto& arg : inst.args) {
                            if (arg.kind == ir::OperandKind::GLOBAL && allFunctionNames.count(arg.name)) {
                                usedFuncs.insert(arg.name);
                            }
                        }
                    }
                }
            }
            // Phase 3: vtable-referenced functions are always used
            for (const auto& g : module_.globals) {
                for (const auto& methodName : g.vtableMethodNames) {
                    if (!methodName.empty()) usedFuncs.insert(methodName);
                }
                // Functions whose address initializes a global (int (*f)() = func)
                for (const auto& lbl : g.initLabels) {
                    if (!lbl.empty()) usedFuncs.insert(lbl);
                }
                // and the words of an array or struct laid out byte by byte
                // ({ f } as well as { &f }: #326; "name+n" too)
                for (const auto& [off, w] : g.initWords) {
                    std::string base = w.substr(0, w.find_first_of("+-"));
                    if (allFunctionNames.count(base)) usedFuncs.insert(base);
                }
            }

            // Remove unused functions (static, inlined, or unreachable)
            auto it = std::remove_if(module_.functions.begin(), module_.functions.end(),
                [&usedFuncs](const ir::Function& fn) {
                    // Always keep _main
                    if (fn.name == "_main") return false;
                    // Keep functions that are called within this module
                    if (usedFuncs.count(fn.name)) return false;
                    // Keep non-static (extern/global) functions — they may be called from other modules;
                    // not a struct method nothing here calls: a unit that calls one has the
                    // struct, so its own (weak) copy (#330: <intwide.h>'s 36 operators in
                    // every program that added two long longs)
                    if (!fn.isStatic && !fn.isMethod) return false;
                    // Remove only static functions that are not called
                    return true;
                });
            changed = (it != module_.functions.end());
            module_.functions.erase(it, module_.functions.end());
        }
        // A data word naming a label of a removed function (a static &&label
        // table of that function): the table goes with it
        std::set<std::string> kept;
        for (const auto& fn : module_.functions) kept.insert(fn.name);
        module_.globals.erase(std::remove_if(module_.globals.begin(), module_.globals.end(),
            [&](const ir::Module::GlobalVar& g) {
                for (const auto& [off, w] : g.initWords) {
                    size_t at = w.find("__lbl_");
                    if (at != std::string::npos && !kept.count(w.substr(0, at))) return true;
                }
                return false;
            }), module_.globals.end());
    }
    // Phase 4: Compiler-level devirtualization
    // For each vtable slot, if only one implementation exists across all vtables,
    // replace virtual CALL_INDIRECT with direct CALL.
    if (devirtualize) {
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
            if (devirtCount > 0 && g_cc45Verbosity >= 1) {
                warnings_.push_back(formatDiagnostic(module_.sourceFile, 0, 0, Severity::Note,
                    "devirtualized " + std::to_string(devirtCount) + " virtual call(s)"));
            }
        }
    }

    // Parameter narrowing analysis: detect static functions where all call sites
    // pass values fitting in char for int parameters. Emit advisory warning.
    {
        // parameter i of a function (its name without the leading _) is declared signed
        auto paramSigned = [&](const std::string& fname, size_t i) {
            auto it = functionParamSigned_.find(fname);
            return it != functionParamSigned_.end() && i < it->second.size() && it->second[i];
        };
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
                           allNonNegPtr && i < allNonNegPtr->size() && (*allNonNegPtr)[i] &&
                           paramSigned(name.substr(1), i)) {
                    // All values non-negative but some > 255 — suggest unsigned int
                    // (only for a parameter declared signed)
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
    if (!zpCallMode && (inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID) &&
        inst.callConv == ir::CallConv::STACK && inst.src1.kind == ir::OperandKind::GLOBAL)
        stackCalledFunctions_.insert(inst.src1.name);
    if (currentBlock_) currentBlock_->insts.push_back(std::move(inst));
}

// __noescape on a declaration (Issue #301): only a read-only object (const,
// or for pointers * const) may be kept with the code: a copied overlay is
// copied in again from its store, so a write would be lost. An extern
// declaration or one without an initializer is not placed (resident).
bool IRBuilder::noescapeGlobal(const VariableDeclaration& node) {
    if (!node.isNoescape) return false;
    bool readOnly = node.pointerLevel == 0 ? node.isConst : node.isPointerConst;
    if (!readOnly) {
        errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column, Severity::Error,
            "'" + node.name + "': __noescape needs a read-only object (const, or * const for a pointer)"));
        return false;
    }
    return node.initializer != nullptr;
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
    currentBlock_->cold = coldDepth_ > 0;
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
    // Build aggregate map from our structs_
    std::map<std::string, TypeSystem::AggregateInfo> aggregates;
    for (const auto& [name, structInfo] : structs_) {
        aggregates[name] = {name, structInfo.totalSize};
    }

    // Delegate to unified TypeSystem
    return TypeSystem::getTypeSize(typeName, ptrLevel, aggregates);
}

// A variable of a plain integer type that is unsigned only by cc45's default
bool IRBuilder::plainUnsignedVar(Expression* e, std::string& name) {
    auto* vr = dynamic_cast<VariableReference*>(e);
    if (!vr) return false;
    name = vr->name;
    if (locals_.count(vr->name)) {
        auto it = localPlainUnsigned_.find(vr->name);
        return it != localPlainUnsigned_.end() && it->second;
    }
    auto it = globalPlainUnsigned_.find(vr->name);
    return it != globalPlainUnsigned_.end() && it->second;
}

IRBuilder::IRTypeInfo IRBuilder::getExprTypeInfo(Expression* expr) {
    if (!expr) return {};
    if (!vmVars_.empty()) {
        if (auto* vr = dynamic_cast<VariableReference*>(expr)) {
            auto it = vmVars_.find(vr->name);
            if (it != vmVars_.end() && locals_.count(vr->name)) {
                std::string tn = localTypeNames_.count(vr->name) ? localTypeNames_[vr->name] : "struct " + it->second;
                return {mapType(tn, 0), tn, false, ir::Type::VOID, structs_[it->second].totalSize};
            }
        }
    }
    
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
    
    if (dynamic_cast<FloatLiteral*>(expr)) return {ir::Type::F32, "float", true, ir::Type::VOID, 5};
    if (auto* lit = dynamic_cast<IntegerLiteral*>(expr)) {
        ir::Type t = ir::Type::I16;
        std::string tn = "int";
        bool isSigned = lit->value < 0;  // cc45 defaults to unsigned; a negative constant is signed
        if (!lit->castType.empty()) {
            tn = lit->castType;
            t = mapType(lit->castType, lit->castPointerLevel);
            isSigned = lit->castIsSigned;
        } else if (lit->value > 65535 || lit->value < -32768) {
            t = ir::Type::I32;
            tn = "long";
        }
        return {t, tn, isSigned, (t == ir::Type::PTR) ? mapType(lit->castType, 0) : ir::Type::VOID, ir::typeSize(t)};
    }
    
    if (auto* str = dynamic_cast<StringLiteral*>(expr)) {
        // A string literal is a char array: decays to a pointer to char
        // (L"...": to 16-bit wchar_t)
        if (str->isWide)
            return {ir::Type::PTR, "int", false, ir::Type::I16, 2 * ((int)str->value.size() + 1)};
        return {ir::Type::PTR, "char", false, ir::Type::I8, (int)str->value.size() + 1};
    }

    if (auto* se = dynamic_cast<StatementExpression*>(expr)) {
        if (!se->result) return {ir::Type::I16, "int", false, ir::Type::VOID, 2};
        // The value is often a variable declared in the block itself, which
        // isn't known until the block has been visited
        if (auto* v = dynamic_cast<VariableReference*>(se->result.get())) {
            std::function<VariableDeclaration*(CompoundStatement*)> find = [&](CompoundStatement* cs) -> VariableDeclaration* {
                for (auto& st : cs->statements) {
                    if (auto* d = dynamic_cast<VariableDeclaration*>(st.get()); d && d->name == v->name) return d;
                    if (auto* inner = dynamic_cast<CompoundStatement*>(st.get()))
                        if (auto* d = find(inner)) return d;
                }
                return nullptr;
            };
            if (auto* d = se->body ? find(se->body.get()) : nullptr) {
                if (!d->arrayDims.empty())
                    return {ir::Type::PTR, d->type, false, mapType(d->type, d->pointerLevel), 2};
                ir::Type t = mapType(d->type, d->pointerLevel);
                return {t, d->type, d->isSigned, t == ir::Type::PTR ? mapType(d->type, d->pointerLevel - 1) : ir::Type::VOID,
                        getTypeSize(d->type, d->pointerLevel)};
            }
        }
        return getExprTypeInfo(se->result.get());
    }

    if (auto* cl = dynamic_cast<CompoundLiteral*>(expr)) {
        // (T){...}: an array literal decays to a pointer to its elements
        if (!cl->arrayDims.empty())
            return {ir::Type::PTR, cl->targetType, false, mapType(cl->targetType, cl->pointerLevel), 2};
        ir::Type t = mapType(cl->targetType, cl->pointerLevel);
        return {t, cl->targetType, cl->isSigned, (t == ir::Type::PTR) ? mapType(cl->targetType, 0) : ir::Type::VOID,
                getTypeSize(cl->targetType, cl->pointerLevel)};
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
            // (an array of pointers: its elements are pointers)
            int totalSize = getTypeSize(tn, localPtrLevel_.count(ref->name) ? localPtrLevel_[ref->name] : 0);
            for (int d : ait->second) totalSize *= d;
            IRTypeInfo r{ir::Type::PTR, tn, false, localTypes_[ref->name], totalSize};
            // (a one-dimensional array decays to a pointer to its element:
            // char *a[] is a char **, Issue #303)
            if (ait->second.size() == 1) r.ptrDepth = (localPtrLevel_.count(ref->name) ? localPtrLevel_[ref->name] : 0) + 1;
            return r;
        }

        auto gait = globalArrayDims_.find(ref->name);
        if (gait != globalArrayDims_.end() && !gait->second.empty()) {
            std::string tn = globalTypeNames_[ref->name];
            int totalSize = getTypeSize(tn, globalPtrLevel_.count(ref->name) ? globalPtrLevel_[ref->name] : 0);
            for (int d : gait->second) totalSize *= d;
            IRTypeInfo r{ir::Type::PTR, tn, false, globalTypes_[ref->name], totalSize};
            if (gait->second.size() == 1) r.ptrDepth = (globalPtrLevel_.count(ref->name) ? globalPtrLevel_[ref->name] : 0) + 1;
            return r;
        }

        auto it = localTypes_.find(ref->name);
        if (it != localTypes_.end()) {
            IRTypeInfo info;
            info.type = it->second;
            info.typeName = localTypeNames_[ref->name];
            info.isSigned = localSigned_[ref->name];
            info.totalSize = ir::typeSize(info.type);
            // Struct/union variable (or by-copy parameter): its full size
            if (structs_.count(getAggregateName(info.typeName)) &&
                (info.type != ir::Type::PTR || aggCopyParams_.count(ref->name)))
                info.totalSize = getTypeSize(info.typeName, 0);
            if (info.type == ir::Type::PTR) {
                auto pit = localPointedToType_.find(ref->name);
                if (pit != localPointedToType_.end()) info.pointedToType = pit->second;
                auto lv = localPtrLevel_.find(ref->name);
                if (lv != localPtrLevel_.end() && !paramArrayDims_.count(ref->name)) info.ptrDepth = lv->second;
            }
            return info;
        }
        auto git = globalTypes_.find(ref->name);
        if (git != globalTypes_.end()) {
            IRTypeInfo info;
            info.type = git->second;
            info.typeName = globalTypeNames_[ref->name];
            auto gsit = globalSigned_.find(ref->name);
            info.isSigned = gsit != globalSigned_.end() && gsit->second;
            info.totalSize = ir::typeSize(info.type);
            if (structs_.count(getAggregateName(info.typeName)) && info.type != ir::Type::PTR)
                info.totalSize = getTypeSize(info.typeName, 0);
            if (info.type == ir::Type::PTR) {
                auto gpit = globalPointedToType_.find(ref->name);
                if (gpit != globalPointedToType_.end()) info.pointedToType = gpit->second;
                auto lv = globalPtrLevel_.find(ref->name);
                if (lv != globalPtrLevel_.end()) info.ptrDepth = lv->second;
            }
            return info;
        }
    }
    
    if (auto* ma = dynamic_cast<MemberAccess*>(expr)) {
        IRTypeInfo baseInfo = getExprTypeInfo(ma->structExpr.get());
        std::string sName = getAggregateName(baseInfo.typeName);

        // Phase 102: Resolve typedef-to-struct mappings
        if (sName.find("struct ") != 0) {
            std::string resolved = resolveTypedefToStruct(sName);
            if (resolved != sName) {
                sName = getAggregateName(resolved);
            } else {
                std::string potentialStruct = "struct " + sName;
                if (structs_.find(potentialStruct) != structs_.end()) {
                    sName = potentialStruct;
                }
            }
        }

        int accOffset = 0;
        auto* mit = findStructMember(sName, ma->memberName, accOffset);
        if (mit) {
            IRTypeInfo info;
            info.type = mapType(mit->type, mit->pointerLevel);
            info.typeName = mit->type;
            info.isSigned = mit->isSigned;
            info.totalSize = ir::typeSize(info.type);
            // an array member (of anything, pointers too) or a struct member:
            // the member's full size (sizeof s.a)
            if ((!mit->arrayDims.empty() || mit->pointerLevel == 0) && mit->size > 0 && mit->bitWidth == 0)
                info.totalSize = mit->size;
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
        vecSubscript(*aa);
        // i[p] is p[i]
        {
            IRTypeInfo ai = getExprTypeInfo(aa->arrayExpr.get());
            IRTypeInfo ii = getExprTypeInfo(aa->indexExpr.get());
            bool aPtr = ai.type == ir::Type::PTR || dynamic_cast<StringLiteral*>(aa->arrayExpr.get());
            bool iPtr = ii.type == ir::Type::PTR || dynamic_cast<StringLiteral*>(aa->indexExpr.get());
            if (!aPtr && iPtr) std::swap(aa->arrayExpr, aa->indexExpr);
        }
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
            bool isSigned = false;  // cc45: unsigned unless declared signed
            std::vector<int> dims;
            
            auto pit = localPointedToType_.find(ref->name);
            if (pit != localPointedToType_.end()) {
                baseType = pit->second;
                isSigned = localSigned_[ref->name];
                tn = localPointedToTypeName_[ref->name];
            } else {
                auto gpit = globalPointedToType_.find(ref->name);
                if (gpit != globalPointedToType_.end()) {
                    baseType = gpit->second;
                    auto gsit = globalSigned_.find(ref->name);
                    if (gsit != globalSigned_.end()) isSigned = gsit->second;
                } else {
                    auto tit = localTypes_.find(ref->name);
                    if (tit != localTypes_.end()) {
                        baseType = tit->second;
                        isSigned = localSigned_[ref->name];
                    } else {
                        auto gtit = globalTypes_.find(ref->name);
                        if (gtit != globalTypes_.end()) {
                            baseType = gtit->second;
                            auto gsit = globalSigned_.find(ref->name);
                            if (gsit != globalSigned_.end()) isSigned = gsit->second;
                        }
                    }
                }
            }

            auto ait = localArrayDims_.find(ref->name);
            if (ait != localArrayDims_.end()) dims = ait->second;
            else if (paramArrayDims_.count(ref->name)) dims = paramArrayDims_[ref->name];
            else {
                auto gait = globalArrayDims_.find(ref->name);
                if (gait != globalArrayDims_.end()) dims = gait->second;
            }

            // With pointers involved: the element is the declared type at
            // pointer level (level - indices past the array dimensions)
            {
                bool isLocalRef = localTypes_.count(ref->name) > 0;
                auto& lvls = isLocalRef ? localPtrLevel_ : globalPtrLevel_;
                auto lit = lvls.find(ref->name);
                int through = depth - (int)dims.size();
                if (lit != lvls.end() && lit->second > 0 && through >= 0 && lit->second >= through) {
                    std::string declTn = isLocalRef ? localTypeNames_[ref->name] : globalTypeNames_[ref->name];
                    int lvl = lit->second - through;
                    ir::Type et = mapType(declTn, lvl);
                    return {et, declTn, isSigned, lvl > 0 ? mapType(declTn, lvl - 1) : ir::Type::VOID,
                            getTypeSize(declTn, lvl)};
                }
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
                bool ptrElems = varType == ir::Type::PTR;
                if (paramArrayDims_.count(ref->name)) ptrElems = localPtrLevel_[ref->name] > 0;  // int a[][M]
                if (ptrElems && !dims.empty()) {
                    // Array of pointers: element type is PTR, pointed-to is baseType
                    return {ir::Type::PTR, tn, isSigned, baseType, 2};
                }
            }
            return {baseType, tn, isSigned, ir::Type::VOID, getTypeSize(tn, 0)};
        } else {
            // A member with pointers: the element is the member's type at the
            // pointer level left after the indices past its array dimensions
            // (char *names[3]: names[i] a pointer, names[i][k] a char)
            if (auto* ma = dynamic_cast<MemberAccess*>(root)) {
                if (auto* mi = arrayMemberOf(ma); mi && mi->pointerLevel > 0) {
                    int through = depth - (int)mi->arrayDims.size();
                    if (through >= 0 && mi->pointerLevel >= through) {
                        int lvl = mi->pointerLevel - through;
                        return {mapType(mi->type, lvl), mi->type, mi->isSigned,
                                lvl > 0 ? mapType(mi->type, lvl - 1) : ir::Type::VOID, getTypeSize(mi->type, lvl)};
                    }
                }
            }
            // General fallback for non-variable roots (e.g. member arrays, function returns, derefs)
            IRTypeInfo arrayTypeInfo = getExprTypeInfo(aa->arrayExpr.get());
            ir::Type baseType = arrayTypeInfo.pointedToType;
            if (baseType == ir::Type::VOID) {
                baseType = mapType(tn, 0);
            }
            return {baseType, tn, arrayTypeInfo.isSigned, ir::Type::VOID, getTypeSize(tn, 0)};
        }
    }
    
    if (auto* deref = dynamic_cast<UnaryOperation*>(expr);
        deref && (deref->op == "-" || deref->op == "~" || deref->op == "+" || deref->op == "!")) {
        std::string v = vecOf(deref->operand.get());
        if (!v.empty()) {
            if (deref->op == "!") v = vecMaskType(v);
            std::string tn = "struct " + v;
            return {mapType(tn, 0), tn, false, ir::Type::VOID, getTypeSize(tn, 0)};
        }
    }
    if (auto* deref = dynamic_cast<UnaryOperation*>(expr)) {
        // -<constant> is a (negative) signed constant
        if (deref->op == "-") {
            if (auto* lit = dynamic_cast<IntegerLiteral*>(deref->operand.get())) {
                if (lit->castType.empty()) {
                    IRTypeInfo li = getExprTypeInfo(lit);
                    li.isSigned = true;
                    return li;
                }
            }
        }
        if (deref->op == "*") {
            if (auto* ref = dynamic_cast<VariableReference*>(deref->operand.get())) {
                // *p of a pointer to an array: the array, i.e. a pointer to
                // its first element
                if (ptrArrayRowSize(ref)) {
                    bool local = localTypes_.count(ref->name) > 0;
                    std::string tn = local ? localTypeNames_[ref->name] : globalTypeNames_[ref->name];
                    bool sg = local ? localSigned_[ref->name] : (globalSigned_.count(ref->name) && globalSigned_[ref->name]);
                    return {ir::Type::PTR, tn, sg, mapType(tn, 0), 2};
                }
                // *p of a pointer to a pointer (char **p): a pointer, to what
                // the declared levels leave (Issue #303: it was a pointer to
                // nothing known, so **p loaded 16 bits)
                auto pointerLevels = [&](const std::string& tn, bool sg, int lvl) -> IRTypeInfo {
                    IRTypeInfo r{ir::Type::PTR, tn, sg, lvl - 1 >= 2 ? ir::Type::PTR : mapType(tn, 0), 2};
                    r.ptrDepth = lvl - 1;
                    return r;
                };
                auto pit = localPointedToType_.find(ref->name);
                if (pit != localPointedToType_.end()) {
                    std::string tn = localTypeNames_.count(ref->name) ? localTypeNames_[ref->name] : "";
                    auto lv = localPtrLevel_.find(ref->name);
                    if (pit->second == ir::Type::PTR && !tn.empty() && lv != localPtrLevel_.end() && lv->second >= 2)
                        return pointerLevels(tn, localSigned_[ref->name], lv->second);
                    int sz = tn.empty() ? ir::typeSize(pit->second) : getTypeSize(tn, 0);
                    return {pit->second, tn, localSigned_[ref->name], ir::Type::VOID, sz};
                }
                auto gpit = globalPointedToType_.find(ref->name);
                if (gpit != globalPointedToType_.end()) {
                    std::string tn = globalTypeNames_.count(ref->name) ? globalTypeNames_[ref->name] : "";
                    auto gsit = globalSigned_.find(ref->name);
                    bool sg = gsit != globalSigned_.end() && gsit->second;
                    auto lv = globalPtrLevel_.find(ref->name);
                    if (gpit->second == ir::Type::PTR && !tn.empty() && lv != globalPtrLevel_.end() && lv->second >= 2)
                        return pointerLevels(tn, sg, lv->second);
                    int sz = tn.empty() ? ir::typeSize(gpit->second) : getTypeSize(tn, 0);
                    return {gpit->second, tn, sg, ir::Type::VOID, sz};
                }
            }
            // General case: derive pointee type from operand expression
            // Handles *(cast_expr), *(ptr + offset), etc.
            IRTypeInfo operandInfo = getExprTypeInfo(deref->operand.get());
            // (a pointer to a pointer: one level less, **p of a char **)
            if (operandInfo.type == ir::Type::PTR && operandInfo.pointedToType == ir::Type::PTR &&
                operandInfo.ptrDepth >= 2 && !operandInfo.typeName.empty()) {
                IRTypeInfo r{ir::Type::PTR, operandInfo.typeName, operandInfo.isSigned,
                             operandInfo.ptrDepth - 1 >= 2 ? ir::Type::PTR : mapType(operandInfo.typeName, 0), 2};
                r.ptrDepth = operandInfo.ptrDepth - 1;
                return r;
            }
            if (operandInfo.type == ir::Type::PTR && operandInfo.pointedToType != ir::Type::VOID) {
                int sz = operandInfo.typeName.empty() ? ir::typeSize(operandInfo.pointedToType) : getTypeSize(operandInfo.typeName, 0);
                return {operandInfo.pointedToType, operandInfo.typeName, operandInfo.isSigned, ir::Type::VOID, sz};
            }
        }
        if (deref->op == "&") {
            IRTypeInfo sub = getExprTypeInfo(deref->operand.get());
            // A pointer's typeName names its pointee (as for pointer variables):
            // &struct_value points to the struct itself
            std::string tn = sub.typeName;
            if (sub.type == ir::Type::PTR || !structs_.count(getAggregateName(tn))) tn += "*";
            return {ir::Type::PTR, tn, false, sub.type, 2};
        }
        // ++, --, ++_POST, --_POST: same type as operand
        if (deref->op == "++" || deref->op == "--" ||
            deref->op == "++_POST" || deref->op == "--_POST") {
            return getExprTypeInfo(deref->operand.get());
        }
    }

    // (p = e), x += e: the value has the type of what is assigned to
    if (auto* as = dynamic_cast<Assignment*>(expr); as && as->target) {
        return getExprTypeInfo(as->target.get());
    }
    if (auto* call = dynamic_cast<FunctionCall*>(expr); call && call->name == "__builtin_shuffle" && !call->arguments.empty()) {
        return getExprTypeInfo(call->arguments[0].get());
    }
    if (auto* call = dynamic_cast<FunctionCall*>(expr)) {
        // Struct/union/float result (returned through the hidden pointer): the declared type
        std::string cname = resolvedFunctionName(call->name);
        auto sit = sretSize_.find(cname);
        auto fit = allFunctions_.find(cname);
        if (sit != sretSize_.end() && fit != allFunctions_.end()) {
            const std::string& rt = fit->second->returnType;
            return {mapType(rt, 0), rt, false, ir::Type::VOID, sit->second};
        }
        auto it = functionReturnTypes_.find(cname);
        if (it != functionReturnTypes_.end()) {
            bool sig = false; // unsigned by default
            auto sit = functionReturnSigned_.find(cname);
            if (sit != functionReturnSigned_.end()) {
                sig = sit->second;
            }
            // A pointer result: what it points to (f() + 1 scales by it)
            if (fit != allFunctions_.end() && fit->second->returnPointerLevel > 0) {
                const std::string& rt = fit->second->returnType;
                int rpl = fit->second->returnPointerLevel;
                return {ir::Type::PTR, rt, sig, mapType(rt, rpl - 1), 2};
            }
            return {it->second, "", sig, ir::Type::VOID, ir::typeSize(it->second)};
        }
        // a call through a function pointer: its signature's result
        std::string callee;
        if (auto sig = callSignature(*call, callee)) {
            if (sig->returnPointerLevel > 0)
                return {ir::Type::PTR, sig->returnType, sig->returnIsSigned,
                        mapType(sig->returnType, sig->returnPointerLevel - 1), 2};
            ir::Type rt = mapType(sig->returnType, 0);
            if (rt != ir::Type::VOID)
                return {rt, sig->returnType, sig->returnIsSigned, ir::Type::VOID, getTypeSize(sig->returnType, 0)};
        }
    }

    // cond ? a : b — the usual arithmetic conversions of a and b (a float
    // branch makes the result float; a pointer branch makes it a pointer)
    if (auto* ce = dynamic_cast<ConditionalExpression*>(expr)) {
        IRTypeInfo a = ce->thenExpr ? getExprTypeInfo(ce->thenExpr.get()) : getExprTypeInfo(ce->condition.get());
        IRTypeInfo b = getExprTypeInfo(ce->elseExpr.get());
        if (a.type == ir::Type::F32) return a;
        if (b.type == ir::Type::F32) return b;
        if (a.type == ir::Type::PTR) return a;
        if (b.type == ir::Type::PTR) return b;
        return ir::typeSize(a.type) >= ir::typeSize(b.type) ? a : b;
    }

    // Binary operations: propagate type and signedness
    if (auto* bin = dynamic_cast<BinaryOperation*>(expr)) {
        if (bin->op == ",") {
            return getExprTypeInfo(bin->right.get());
        }
        // A vector operation: the vector type (a comparison: the mask vector)
        if (bin->op != "&&" && bin->op != "||") {
            std::string v = vecOf(bin->left.get());
            if (v.empty()) v = vecOf(bin->right.get());
            if (!v.empty()) {
                const std::string& o = bin->op;
                if (o == "==" || o == "!=" || o == "<" || o == ">" || o == "<=" || o == ">=") v = vecMaskType(v);
                std::string tn = "struct " + v;
                return {mapType(tn, 0), tn, false, ir::Type::VOID, getTypeSize(tn, 0)};
            }
        }
        // For arithmetic/bitwise ops, result type comes from operands
        auto lhs = getExprTypeInfo(bin->left.get());
        auto rhs = getExprTypeInfo(bin->right.get());
        // Pointer difference: a signed element count (ptrdiff_t)
        if (bin->op == "-" && lhs.type == ir::Type::PTR && rhs.type == ir::Type::PTR)
            return {ir::Type::I16, "", true, ir::Type::VOID, 2};
        // Pointer +/- integer: the pointer's type (keeps the pointed-to type)
        if ((bin->op == "+" || bin->op == "-") && lhs.type == ir::Type::PTR && rhs.type != ir::Type::PTR)
            return lhs;
        if (bin->op == "+" && rhs.type == ir::Type::PTR && lhs.type != ir::Type::PTR)
            return rhs;
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
        // + - * << of chars are ints (the value is computed in 16 bits and
        // can need them): a char type here would let an enclosing char
        // operation truncate it (a + (b + c) lost the carry of b + c)
        if (t == ir::Type::I8 && (bin->op == "+" || bin->op == "-" || bin->op == "*" || bin->op == "<<"))
            t = ir::Type::I16;
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
        inst.resultType = ir::Type::F32; inst.src1 = src;
        inst.src2 = ir::Operand::imm(isSigned ? 1 : 0, ir::Type::I8);  // signed source?
        emit(inst);
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

ir::SourceLoc IRBuilder::declLoc(ASTNode& node) {
    ir::SourceLoc sl = loc(node);
    if (!node.sourceFile.empty()) sl.file = node.sourceFile;
    sl.column = node.column;
    return sl;
}

// ============================================================================
// Top-level
// ============================================================================

void IRBuilder::declareBuiltinPrototypes() {
    struct Proto { const char* name; const char* ret; std::vector<std::pair<const char*, int>> params; };
    static const std::vector<Proto> protos = [] {
        std::vector<Proto> v;
        for (const char* n : {"fabs", "floor", "ceil", "sqrt", "trunc", "round", "sin", "cos", "tan",
                              "atan", "log", "log10", "log2", "exp"}) {
            v.push_back({n, "float", {{"float", 0}}});
        }
        for (const char* n : {"fmod", "pow", "atan2", "copysign", "fmax", "fmin", "fdim"})
            v.push_back({n, "float", {{"float", 0}, {"float", 0}}});
        v.push_back({"ldexp", "float", {{"float", 0}, {"int", 0}}});
        v.push_back({"frexp", "float", {{"float", 0}, {"int", 1}}});
        v.push_back({"modf", "float", {{"float", 0}, {"float", 1}}});
        v.push_back({"signbit", "int", {{"float", 0}}});
        return v;
    }();
    for (const auto& p : protos) {
        for (int f = 0; f < 2; ++f) {   // name and name##f
            std::string name = std::string(p.name) + (f ? "f" : "");
            auto fd = std::make_unique<FunctionDeclaration>(name, p.ret);
            fd->isPrototype = true;
            fd->isSigned = std::string(p.ret) == "int";
            int i = 0;
            for (const auto& [t, ptr] : p.params) {
                Parameter prm;
                prm.type = t;
                prm.pointerLevel = ptr;
                prm.isSigned = std::string(t) == "int";
                prm.name = "a" + std::to_string(i++);
                fd->parameters.push_back(prm);
            }
            visit(*fd);
            builtinProtoNames_.insert(name);
            builtinProtos_.push_back(std::move(fd));
        }
    }
    // Integer bit builtins (int 16-bit, long 32-bit): their argument and
    // result widths, so a call needs no declaration
    struct IProto { const char* name; const char* ret; bool retSigned; const char* param; bool paramSigned; };
    static const IProto iprotos[] = {
        {"__builtin_ffs", "int", true, "int", true},
        {"__builtin_ffsl", "int", true, "long", true},
        {"__builtin_ffsll", "int", true, "long", true},
        {"__builtin_clz", "int", true, "int", false},
        {"__builtin_clzl", "int", true, "long", false},
        {"__builtin_clzll", "int", true, "long", false},
        {"__builtin_ctz", "int", true, "int", false},
        {"__builtin_ctzl", "int", true, "long", false},
        {"__builtin_ctzll", "int", true, "long", false},
        {"__builtin_popcount", "int", true, "int", false},
        {"__builtin_popcountl", "int", true, "long", false},
        {"__builtin_popcountll", "int", true, "long", false},
        {"__builtin_parity", "int", true, "int", false},
        {"__builtin_parityl", "int", true, "long", false},
        {"__builtin_parityll", "int", true, "long", false},
        {"__builtin_bswap16", "int", false, "int", false},
        {"__builtin_bswap32", "long", false, "long", false},
    };
    // The variadic stdio builtins (__builtin_printf etc. become these names):
    // without a declaration their arguments would be passed as for a fixed
    // parameter list instead of on the stack
    {
        struct VProto { const char* name; std::vector<std::pair<const char*, int>> params; };
        static const std::vector<VProto> vprotos = {
            {"printf", {{"char", 1}}},
            {"sprintf", {{"char", 1}, {"char", 1}}},
            {"snprintf", {{"char", 1}, {"int", 0}, {"char", 1}}},
        };
        for (const auto& p : vprotos) {
            auto fd = std::make_unique<FunctionDeclaration>(p.name, "int");
            fd->isPrototype = true;
            fd->isSigned = true;
            fd->isVariadic = true;
            int i = 0;
            for (const auto& [t, ptr] : p.params) {
                Parameter prm;
                prm.type = t;
                prm.pointerLevel = ptr;
                prm.isSigned = ptr == 0;
                prm.name = "a" + std::to_string(i++);
                fd->parameters.push_back(prm);
            }
            visit(*fd);
            builtinProtoNames_.insert(p.name);
            builtinProtos_.push_back(std::move(fd));
        }
    }
    // memmove / memset (__fastcall__, as <string.h>): the calls copy and fill
    // loops become (tryLoopIdiom) need no declaration
    {
        struct MProto { const char* name; std::vector<std::pair<const char*, int>> params; };
        static const std::vector<MProto> mprotos = {
            {"memmove", {{"void", 1}, {"void", 1}, {"int", 0}}},
            {"memset", {{"void", 1}, {"int", 0}, {"int", 0}}},
        };
        for (const auto& p : mprotos) {
            auto fd = std::make_unique<FunctionDeclaration>(p.name, "void");
            fd->returnPointerLevel = 1;
            fd->isPrototype = true;
            fd->isFastcall = true;
            int i = 0;
            for (const auto& [t, ptr] : p.params) {
                Parameter prm;
                prm.type = t;
                prm.pointerLevel = ptr;
                prm.isSigned = ptr == 0;
                prm.name = "a" + std::to_string(i++);
                fd->parameters.push_back(prm);
            }
            visit(*fd);
            builtinProtoNames_.insert(p.name);
            builtinProtos_.push_back(std::move(fd));
        }
    }
    // int __overflow_add/_sub/_mul(unsigned long a, unsigned long b,
    //                unsigned char *res, unsigned int flags): the
    //                __builtin_*_overflow helpers, one per operation
    for (const char* hn : {"__overflow_add", "__overflow_sub", "__overflow_mul"}) {
        auto fd = std::make_unique<FunctionDeclaration>(hn, "int");
        fd->isPrototype = true;
        fd->isSigned = true;
        const std::pair<const char*, int> ps[] = {{"long", 0}, {"long", 0}, {"char", 1}, {"int", 0}};
        int i = 0;
        for (const auto& [t, ptr] : ps) {
            Parameter prm;
            prm.type = t;
            prm.pointerLevel = ptr;
            prm.name = "a" + std::to_string(i++);
            fd->parameters.push_back(prm);
        }
        visit(*fd);
        builtinProtoNames_.insert(hn);
        builtinProtos_.push_back(std::move(fd));
    }
    // the ones the library implements in (ZP) asm
    static const std::set<std::string> asmBuiltins = {
        "__builtin_bswap32", "__builtin_clz", "__builtin_ctz", "__builtin_ffs", "__builtin_popcount"};
    for (const auto& p : iprotos) {
        auto fd = std::make_unique<FunctionDeclaration>(p.name, p.ret);
        fd->isPrototype = true;
        fd->isSigned = p.retSigned;
        fd->isFastcall = asmBuiltins.count(p.name) > 0;
        Parameter prm;
        prm.type = p.param;
        prm.pointerLevel = 0;
        prm.isSigned = p.paramSigned;
        prm.name = "x";
        fd->parameters.push_back(prm);
        visit(*fd);
        builtinProtoNames_.insert(p.name);
        builtinProtos_.push_back(std::move(fd));
    }
}

// File-scope declarations of one object must agree (C99 6.7p4, 6.2.2p7): the
// same type -- base type, signedness, pointer level, qualifiers, __zp, the
// array's dimensions (an incomplete first one, a[], completes) -- and a
// static one after a non-static one, or a definition without static after a
// static one, is an error, and so is a second initializer. (A later
// different declaration used to win:
// int a[27]; ... static int a; gave a 2 bytes.)
void IRBuilder::checkGlobalRedeclarations(TranslationUnit& node) {
    std::vector<VariableDeclaration*> decls;
    for (auto& d : node.topLevelDecls) {
        if (auto* vd = dynamic_cast<VariableDeclaration*>(d.get())) decls.push_back(vd);
        else if (auto* cs = dynamic_cast<CompoundStatement*>(d.get()); cs && cs->isDeclGroup)
            for (auto& g : cs->statements)
                if (auto* vd = dynamic_cast<VariableDeclaration*>(g.get())) decls.push_back(vd);
    }
    auto typeText = [](const VariableDeclaration* v) {
        bool integer = v->type == "char" || v->type == "short" || v->type == "int" || v->type == "long";
        std::string t = (v->isConst ? "const " : "") +
                        std::string(integer ? (v->isSigned ? "signed " : "unsigned ") : "") + v->type;
        if (v->isFunctionPointer) return t + " (*)(...)";
        if (v->pointerLevel) t += " " + std::string(v->pointerLevel, '*') + (v->isPointerConst ? " const" : "");
        for (size_t i = 0; i < v->arrayDims.size(); ++i)
            t += "[" + (v->arrayDims[i] || i ? std::to_string(v->arrayDims[i]) : std::string()) + "]";
        if (v->addressSpace == 1) t = "__zp " + t;
        return t;
    };
    auto sameType = [](const VariableDeclaration* a, const VariableDeclaration* b) {
        if (a->type != b->type || a->pointerLevel != b->pointerLevel || a->isSigned != b->isSigned ||
            a->isConst != b->isConst || a->isFunctionPointer != b->isFunctionPointer ||
            (a->pointerLevel && a->isPointerConst != b->isPointerConst) ||
            (a->addressSpace == 1) != (b->addressSpace == 1) ||
            a->arrayDims.size() != b->arrayDims.size())
            return false;
        for (size_t i = 0; i < a->arrayDims.size(); ++i)
            if (a->arrayDims[i] != b->arrayDims[i] && !(i == 0 && (a->arrayDims[0] == 0 || b->arrayDims[0] == 0)))
                return false;
        return true;
    };
    std::map<std::string, VariableDeclaration*> first;
    std::map<std::string, bool> internal;   // its linkage (from the first declaration)
    std::map<std::string, int> initAt;      // the line of its initializer
    for (auto* v : decls) {
        auto it = first.find(v->name);
        if (it == first.end()) {
            first[v->name] = v;
            internal[v->name] = v->isStatic;
            if (v->initializer) initAt[v->name] = v->line;
            continue;
        }
        if (v->initializer && initAt.count(v->name)) {
            errors_.push_back(formatDiagnostic(v->sourceFile, v->line, v->column, Severity::Error,
                "redefinition of '" + v->name + "' (previous definition at line " + std::to_string(initAt[v->name]) + ")"));
            continue;
        }
        if (v->initializer) initAt[v->name] = v->line;
        const VariableDeclaration* p = it->second;
        std::string where = " (previous declaration at line " + std::to_string(p->line) + ")";
        if (!sameType(p, v))
            errors_.push_back(formatDiagnostic(v->sourceFile, v->line, v->column, Severity::Error,
                "conflicting types for '" + v->name + "': '" + typeText(v) + "', was '" + typeText(p) + "'" + where));
        else if (v->isStatic && !internal[v->name])
            errors_.push_back(formatDiagnostic(v->sourceFile, v->line, v->column, Severity::Error,
                "static declaration of '" + v->name + "' follows non-static declaration" + where));
        else if (!v->isStatic && !v->isExtern && internal[v->name])
            errors_.push_back(formatDiagnostic(v->sourceFile, v->line, v->column, Severity::Error,
                "non-static declaration of '" + v->name + "' follows static declaration" + where));
        // (an array's size, once known, is kept for the later ones)
        if (!p->arrayDims.empty() && p->arrayDims[0] == 0 && !v->arrayDims.empty() && v->arrayDims[0]) it->second = v;
    }
}

void IRBuilder::visit(TranslationUnit& node) {
    // #pragma encoding (asm(".encoding ...")) takes effect where it appears:
    // literals before the first one are PETSCII
    currentStringEncoding_ = StringEncoding::PETSCII;
    for (auto& decl : node.topLevelDecls)
        if (auto* fd = dynamic_cast<FunctionDeclaration*>(decl.get())) {
            unitFunctionNames_.insert(fd->name);
            if (!fd->isPrototype) unitDefinedFunctions_.insert(fd->name);
        }
    // a name is either a variable or a function at file scope
    for (auto& decl : node.topLevelDecls)
        if (auto* vd = dynamic_cast<VariableDeclaration*>(decl.get()); vd && unitFunctionNames_.count(vd->name))
            errors_.push_back(formatDiagnostic(vd->sourceFile, vd->line, vd->column, Severity::Error,
                "'" + vd->name + "' redeclared as a different kind of symbol (a variable and a function)"));
    checkGlobalRedeclarations(node);
    declareBuiltinPrototypes();
    // Now visit all declarations
    for (auto& decl : node.topLevelDecls) {
        decl->accept(*this);
    }
}

// ============================================================================
// Functions
// ============================================================================

void IRBuilder::warnRecursiveForcedInline(const std::string& name, ASTNode& at) {
    if (!warnedRecursiveInline_.insert(name).second) return;
    warnings_.push_back(formatDiagnostic(at.sourceFile, at.line, at.column, Severity::Warning,
        "always_inline function '" + name + "' is recursive; emitting a normal call"));
}

// A static function's symbol gets the module's name appended, so modules with
// static functions of the same name link together (their SAC storage and entry
// symbols are exported like any function's).
std::string IRBuilder::staticFunctionName(const std::string& name) {
    std::string tag = module_.sourceFile;
    size_t slash = tag.find_last_of("/\\");
    if (slash != std::string::npos) tag = tag.substr(slash + 1);
    size_t dot = tag.find('.');
    if (dot != std::string::npos) tag = tag.substr(0, dot);
    for (auto& c : tag) if (!isalnum((unsigned char)c)) c = '_';
    return tag.empty() ? name : name + "__" + tag;
}

// Symbol of a called function: a static function's module-unique name
std::string IRBuilder::resolvedFunctionName(const std::string& name) const {
    auto it = staticRenames_.find(name);
    return it == staticRenames_.end() ? name : it->second;
}

// The current function passes its frame pointer as a child's static link:
// it needs a real stack frame, and its own static link stays in that frame
// for the child to follow up the chain
void IRBuilder::markStaticLinkProvider() {
    if (!currentFunc_) return;
    currentFunc_->isRecurse = true;
    if (currentFunc_->isNested && currentFunc_->staticLinkVreg >= 0) {
        currentFunc_->memoryVregs.insert((uint32_t)currentFunc_->staticLinkVreg);
        usedVregs_.insert((uint32_t)currentFunc_->staticLinkVreg);
        keepStaticLink(currentFunc_);
    }
}

// The static link gets a frame slot (frame locals are laid out from
// localNamesOrder) even when the function itself never reads it
void IRBuilder::keepStaticLink(ir::Function* f) {
    auto& order = f->localNamesOrder;
    if (std::find(order.begin(), order.end(), "__static_link") == order.end())
        order.push_back("__static_link");
}

// ADDR_UPLEVEL over more than one level: name the functions whose frames
// are passed through (args[2..]), whose static links lead further up
void IRBuilder::addUplevelChain(ir::Inst& inst, int levels) {
    for (int k = 0; k < levels - 1; k++) {
        int idx = (int)functionStack_.size() - 1 - k;
        if (idx < 0) break;
        ir::Function* f = functionStack_[idx].func;
        if (f->staticLinkVreg >= 0) {
            f->memoryVregs.insert((uint32_t)f->staticLinkVreg);
            functionStack_[idx].usedVregs.insert((uint32_t)f->staticLinkVreg);
            keepStaticLink(f);
        }
        f->isRecurse = true;
        inst.args.push_back(ir::Operand::global(f->name));
    }
}

void IRBuilder::visit(FunctionDeclaration& node) {
    // break N counts only this function's loops (a nested function's body)
    struct LoopBase { size_t& b; size_t saved; LoopBase(size_t& r, size_t v) : b(r), saved(r) { r = v; } ~LoopBase() { b = saved; } }
        loopBase(loopBase_, loopStack_.size());
    // Module-private name for a static function (also when only its prototype is static)
    if (!node.isMethod) {
        auto rn = staticRenames_.find(node.name);
        if (rn != staticRenames_.end()) node.name = rn->second;
        else if (node.isStatic && !node.parentFunc) {
            std::string unique = staticFunctionName(node.name);
            if (unique != node.name) {
                staticRenames_[node.name] = unique;
                node.name = unique;
            }
        }
    }
    // __attribute__((aligned(N))) on any declaration aligns the function
    if (node.alignment > 1) funcAlign_[node.name] = std::max(funcAlign_[node.name], node.alignment);
    // Attributes may appear on a prototype or the definition; accumulate by name
    if (node.isAlwaysInline) alwaysInlineNames_.insert(node.name);
    // (only a function whose value comes back in registers: a struct, union,
    // _Complex, float or long long one writes it through its hidden __sret
    // pointer, so its call is not free of effects)
    bool regReturn = node.returnPointerLevel > 0 ||
                     (!structOf(node.returnType) && mapType(node.returnType, 0) != ir::Type::F32);
    if (node.purity > 0 && regReturn) {
        int& p = module_.pureFunctions["_" + node.name];
        p = std::max(p, node.purity);
    }
    // (a __far function's code stays in far memory: not copied into its callers)
    if (node.isNoInline || node.isFar) noInlineNames_.insert(node.name);
    if (alwaysInlineNames_.count(node.name) && noInlineNames_.count(node.name) && node.body) {
        warnings_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column, Severity::Warning,
            "function '" + node.name + "' has both always_inline and noinline; ignoring always_inline"));
        alwaysInlineNames_.erase(node.name);
    }

    // Record parameter info and return type for all functions (including prototypes)
    {
        std::vector<ParamInfo> pinfo;
        std::vector<ir::Type> ptypes;
        std::vector<bool> psigned;

        // Struct/union/float results go through a hidden first parameter
        bool retFloat = node.returnPointerLevel == 0 &&
                        mapType(node.returnType, 0) == ir::Type::F32;
        bool retAggregate = node.returnPointerLevel == 0 && structs_.count(getAggregateName(node.returnType));
        if (retFloat || retAggregate) {
            sretSize_[node.name] = retFloat ? 5 : getTypeSize(node.returnType, 0);
            if (retFloat) sretFloat_.insert(node.name);
            if (node.parameters.empty() || node.parameters[0].name != "__sret") {
                Parameter hidden;
                hidden.type = "void";
                hidden.pointerLevel = 1;
                hidden.name = "__sret";
                node.parameters.insert(node.parameters.begin(), hidden);
            }
        }

        std::vector<int> aggSizes;
        for (const auto& p : node.parameters) {
            pinfo.push_back({p.isConst, p.pointerLevel});
            // A struct/union parameter (other than 1/2/4 bytes) is passed as a
            // pointer to a caller-made copy
            int agg = aggregateBlockSize(p.type, p.pointerLevel);
            aggSizes.push_back(agg);
            ptypes.push_back(agg ? ir::Type::PTR : mapType(p.type, p.pointerLevel));
            psigned.push_back(p.isSigned);
        }
        paramAggSizes_[node.name] = aggSizes;
        bool wasZp = usesZpConv(node.name);
        if (node.isFastcall) fastcallFunctions_.insert(node.name);
        funcParamInfo_[node.name] = pinfo;
        functionParamTypes_[node.name] = ptypes;
        functionParamSigned_[node.name] = psigned;
        functionReturnTypes_[node.name] = sretSize_.count(node.name) ? ir::Type::PTR
                                                                      : mapType(node.returnType, node.returnPointerLevel);
        functionReturnSigned_[node.name] = node.isSigned;
        if (node.isVariadic) variadicFunctions_.insert(node.name);
        // ln45 learns per object (cc45.zpcall.<name>), not per call, that a
        // stack-mode object calls a function the ZP way: stack calls made
        // before a __fastcall__ declaration would not be bridged
        if (node.isFastcall && !zpCallMode && !wasZp && usesZpConv(node.name) &&
            stackCalledFunctions_.count("_" + node.name))
            errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column, Severity::Error,
                "'" + node.name + "' declared __fastcall__ after it was called with the stack convention; "
                "declare it __fastcall__ before the first call"));
        if (node.isRegparm) regparmFunctions_.insert(node.name);
        allFunctions_[node.name] = &node;
        if (builtinProtos_.empty() || builtinProtos_.back().get() != &node) builtinProtoNames_.erase(node.name);
    }
    if (node.isPrototype || !node.body) return;

    ir::Function fn;
    fn.name = "_" + node.name;
    definedFunctions_.insert(fn.name);
    fn.returnType = mapType(node.returnType, node.returnPointerLevel);
    fn.conv = usesZpConv(node.name) ? ir::CallConv::ZP : ir::CallConv::STACK;
    fn.isVariadic = node.isVariadic;
    fn.isStatic = node.isStatic;
    // a struct method is defined by every unit that includes its struct (as a
    // C++ inline member function): weak, so the units link together
    fn.isWeak = weakNextFunction_ || node.isMethod;
    fn.isMethod = node.isMethod;
    weakNextFunction_ = false;
    fn.optOverrides = optOverrides_;
    fn.isRecurse = recurseNextFunction_;
    recurseNextFunction_ = false;
    fn.isInterrupt = node.isInterrupt;
    fn.declLine = node.line;
    fn.isNaked = node.isNaked;
    fn.isRegparm = node.isRegparm;
    fn.isFar = node.isFar;
    fn.isNested = node.isNested;
    if (funcAlign_.count(node.name)) fn.align = funcAlign_[node.name];


    // Register inline candidate if marked inline, -finline-functions, or trivial method
    if (!node.isVariadic && !node.isInterrupt && !node.isNaked) {
        int stmtCount = node.body ? (int)node.body->statements.size() : 0;
        bool forced = alwaysInlineNames_.count(node.name) > 0;
        bool shouldInline = node.isInline || inlineFunctions || forced;
        // Auto-inline trivial methods (≤3 statements — getters, setters, simple computations)
        if (!shouldInline && node.isMethod && stmtCount <= 3) {
            shouldInline = true;
        }
        if (noInlineNames_.count(node.name)) shouldInline = false;
        // A body that names its own symbols or must exist once is not copied
        // into callers (always_inline still forces it): inline asm (it may
        // name the parameters, @_p_x), labels and goto (a second copy in the
        // same caller duplicates them), nested functions, static locals
        if (!forced && shouldInline) {
            std::function<bool(Statement*)> blocks = [&](Statement* st) -> bool {
                if (!st) return false;
                if (dynamic_cast<AsmStatement*>(st) || dynamic_cast<LabelledStatement*>(st) ||
                    dynamic_cast<GotoStatement*>(st) || dynamic_cast<FunctionDeclaration*>(st)) return true;
                if (auto* d = dynamic_cast<VariableDeclaration*>(st)) return d->isStatic || !d->vlaDimExprs.empty();
                if (auto* c = dynamic_cast<CompoundStatement*>(st)) {
                    for (auto& x : c->statements) if (blocks(x.get())) return true;
                    return false;
                }
                if (auto* i = dynamic_cast<IfStatement*>(st)) return blocks(i->thenBranch.get()) || blocks(i->elseBranch.get());
                if (auto* w = dynamic_cast<WhileStatement*>(st)) return blocks(w->body.get());
                if (auto* w = dynamic_cast<DoWhileStatement*>(st)) return blocks(w->body.get());
                if (auto* f = dynamic_cast<ForStatement*>(st)) return blocks(f->initializer.get()) || blocks(f->body.get());
                if (auto* w = dynamic_cast<SwitchStatement*>(st)) return blocks(w->body.get());
                if (auto* r = dynamic_cast<RepeatStatement*>(st)) return blocks(r->body.get());
                return false;
            };
            if (blocks(node.body.get())) shouldInline = false;
            // &param: the inliner binds a parameter to the argument's value
            // (often a constant temp), which has no address
            std::set<std::string> params;
            for (const auto& p : node.parameters) params.insert(p.name);
            std::function<bool(Expression*)> addrOfParam = [&](Expression* e) -> bool {
                if (!e) return false;
                if (auto* u = dynamic_cast<UnaryOperation*>(e)) {
                    if (u->op == "&") {
                        Expression* x = u->operand.get();
                        while (auto* m = dynamic_cast<MemberAccess*>(x)) { if (m->isArrow) break; x = m->structExpr.get(); }
                        while (auto* a = dynamic_cast<ArrayAccess*>(x)) x = a->arrayExpr.get();
                        if (auto* v = dynamic_cast<VariableReference*>(x); v && params.count(v->name)) return true;
                    }
                    return addrOfParam(u->operand.get());
                }
                if (auto* b = dynamic_cast<BinaryOperation*>(e)) return addrOfParam(b->left.get()) || addrOfParam(b->right.get());
                if (auto* a = dynamic_cast<Assignment*>(e)) return addrOfParam(a->target.get()) || addrOfParam(a->expression.get());
                if (auto* c = dynamic_cast<ConditionalExpression*>(e))
                    return addrOfParam(c->condition.get()) || addrOfParam(c->thenExpr.get()) || addrOfParam(c->elseExpr.get());
                if (auto* c = dynamic_cast<CastExpression*>(e)) return addrOfParam(c->expression.get());
                if (auto* a = dynamic_cast<ArrayAccess*>(e)) return addrOfParam(a->arrayExpr.get()) || addrOfParam(a->indexExpr.get());
                if (auto* m = dynamic_cast<MemberAccess*>(e)) return addrOfParam(m->structExpr.get());
                if (auto* f = dynamic_cast<FunctionCall*>(e)) {
                    for (auto& a : f->arguments) if (addrOfParam(a.get())) return true;
                    return addrOfParam(f->callExpr.get());
                }
                if (dynamic_cast<StatementExpression*>(e) || dynamic_cast<CompoundLiteral*>(e) ||
                    dynamic_cast<InitializerList*>(e)) return true;   // not looked into: be safe
                return false;
            };
            std::function<bool(Statement*)> stmtAddr = [&](Statement* st) -> bool {
                if (!st) return false;
                if (auto* es = dynamic_cast<ExpressionStatement*>(st)) return addrOfParam(es->expression.get());
                if (auto* r = dynamic_cast<ReturnStatement*>(st)) return addrOfParam(r->expression.get());
                if (auto* d = dynamic_cast<VariableDeclaration*>(st)) return addrOfParam(d->initializer.get());
                if (auto* c = dynamic_cast<CompoundStatement*>(st)) {
                    for (auto& x : c->statements) if (stmtAddr(x.get())) return true;
                    return false;
                }
                if (auto* i = dynamic_cast<IfStatement*>(st))
                    return addrOfParam(i->condition.get()) || stmtAddr(i->thenBranch.get()) || stmtAddr(i->elseBranch.get());
                if (auto* w = dynamic_cast<WhileStatement*>(st)) return addrOfParam(w->condition.get()) || stmtAddr(w->body.get());
                if (auto* w = dynamic_cast<DoWhileStatement*>(st)) return addrOfParam(w->condition.get()) || stmtAddr(w->body.get());
                if (auto* f = dynamic_cast<ForStatement*>(st))
                    return stmtAddr(f->initializer.get()) || addrOfParam(f->condition.get()) ||
                           addrOfParam(f->increment.get()) || stmtAddr(f->body.get());
                if (auto* w = dynamic_cast<SwitchStatement*>(st)) return addrOfParam(w->expression.get()) || stmtAddr(w->body.get());
                if (auto* r = dynamic_cast<RepeatStatement*>(st)) return stmtAddr(r->body.get());
                return false;
            };
            if (!params.empty() && stmtAddr(node.body.get())) shouldInline = false;
            // Variable-length array parameters: their sizes are evaluated on
            // entry and give the parameter its run-time stride
            for (const auto& p : node.parameters) {
                if (p.vlaSizeExpr) shouldInline = false;
                for (const auto& e : p.innerDimExprs) if (e) shouldInline = false;
            }
        }
        if (sretSize_.count(node.name)) shouldInline = false;  // result goes through __sret
        for (int agg : paramAggSizes_[node.name]) if (agg) shouldInline = false;  // by-copy aggregate param
        if (shouldInline && (forced || stmtCount <= INLINE_MAX_STMTS)) {
            inlineCandidates_[node.name] = &node;
        }
    }

    if (sretSize_.count(node.name)) fn.returnType = ir::Type::PTR;  // returns the __sret pointer

    std::vector<std::pair<std::string, int>> sigParams;   // (as a pointer to it declares them)
    for (const auto& p : node.parameters) {
        fn.paramTypes.push_back(aggregateBlockSize(p.type, p.pointerLevel) ? ir::Type::PTR
                                                                            : mapType(p.type, p.pointerLevel));
        fn.paramNames.push_back(p.name);
        if (p.name != "__sret" && p.name != "__static_link") sigParams.push_back({p.type, p.pointerLevel});
    }
    fn.ptrSig = ptrSigKey(sigParams, node.isVariadic);

    module_.functions.push_back(std::move(fn));
    ir::Function* fnPtr = &module_.functions.back();

    // Phase 2, Phase 3: Detect original leaf status from AST BEFORE IR optimization
    // This captures whether the original code had function calls, before inlining
    // Use a simple recursive checker that looks for any FunctionCall nodes
    fnPtr->originalIsLeaf = true;  // default to leaf
    fnPtr->originalCallees.clear();

    if (node.body) {
        std::function<bool(ASTNode*)> hasFunctionCalls = [&](ASTNode* n) -> bool {
            if (!n) return false;

            // Check if this is a FunctionCall node
            if (auto* fc = dynamic_cast<FunctionCall*>(n)) {
                fnPtr->originalCallees.insert(resolvedFunctionName(fc->name));
                fnPtr->originalIsLeaf = false;
                return true;
            }

            // Recursively check child nodes
            if (auto* compound = dynamic_cast<CompoundStatement*>(n)) {
                for (auto& s : compound->statements) if (hasFunctionCalls(s.get())) fnPtr->originalIsLeaf = false;
            } else if (auto* expr = dynamic_cast<ExpressionStatement*>(n)) {
                if (hasFunctionCalls(expr->expression.get())) fnPtr->originalIsLeaf = false;
            } else if (auto* ifstmt = dynamic_cast<IfStatement*>(n)) {
                if (hasFunctionCalls(ifstmt->condition.get())) fnPtr->originalIsLeaf = false;
                if (hasFunctionCalls(ifstmt->thenBranch.get())) fnPtr->originalIsLeaf = false;
                if (hasFunctionCalls(ifstmt->elseBranch.get())) fnPtr->originalIsLeaf = false;
            } else if (auto* whilestmt = dynamic_cast<WhileStatement*>(n)) {
                if (hasFunctionCalls(whilestmt->condition.get())) fnPtr->originalIsLeaf = false;
                if (hasFunctionCalls(whilestmt->body.get())) fnPtr->originalIsLeaf = false;
            } else if (auto* forstmt = dynamic_cast<ForStatement*>(n)) {
                if (hasFunctionCalls(forstmt->initializer.get())) fnPtr->originalIsLeaf = false;
                if (hasFunctionCalls(forstmt->condition.get())) fnPtr->originalIsLeaf = false;
                if (hasFunctionCalls(forstmt->increment.get())) fnPtr->originalIsLeaf = false;
                if (hasFunctionCalls(forstmt->body.get())) fnPtr->originalIsLeaf = false;
            } else if (auto* retstmt = dynamic_cast<ReturnStatement*>(n)) {
                if (hasFunctionCalls(retstmt->expression.get())) fnPtr->originalIsLeaf = false;
            } else if (auto* assign = dynamic_cast<Assignment*>(n)) {
                if (hasFunctionCalls(assign->target.get())) fnPtr->originalIsLeaf = false;
                if (hasFunctionCalls(assign->expression.get())) fnPtr->originalIsLeaf = false;
            } else if (auto* binop = dynamic_cast<BinaryOperation*>(n)) {
                if (hasFunctionCalls(binop->left.get())) fnPtr->originalIsLeaf = false;
                if (hasFunctionCalls(binop->right.get())) fnPtr->originalIsLeaf = false;
            } else if (auto* unop = dynamic_cast<UnaryOperation*>(n)) {
                if (hasFunctionCalls(unop->operand.get())) fnPtr->originalIsLeaf = false;
            } else if (auto* arrayacc = dynamic_cast<ArrayAccess*>(n)) {
                if (hasFunctionCalls(arrayacc->arrayExpr.get())) fnPtr->originalIsLeaf = false;
                if (hasFunctionCalls(arrayacc->indexExpr.get())) fnPtr->originalIsLeaf = false;
            } else if (auto* memberacc = dynamic_cast<MemberAccess*>(n)) {
                if (hasFunctionCalls(memberacc->structExpr.get())) fnPtr->originalIsLeaf = false;
            } else if (auto* cast = dynamic_cast<CastExpression*>(n)) {
                if (hasFunctionCalls(cast->expression.get())) fnPtr->originalIsLeaf = false;
            } else if (auto* condexpr = dynamic_cast<ConditionalExpression*>(n)) {
                if (hasFunctionCalls(condexpr->condition.get())) fnPtr->originalIsLeaf = false;
                if (hasFunctionCalls(condexpr->thenExpr.get())) fnPtr->originalIsLeaf = false;
                if (hasFunctionCalls(condexpr->elseExpr.get())) fnPtr->originalIsLeaf = false;
            } else if (auto* swstmt = dynamic_cast<SwitchStatement*>(n)) {
                if (hasFunctionCalls(swstmt->expression.get())) fnPtr->originalIsLeaf = false;
                if (hasFunctionCalls(swstmt->body.get())) fnPtr->originalIsLeaf = false;
            } else if (auto* vardecl = dynamic_cast<VariableDeclaration*>(n)) {
                if (hasFunctionCalls(vardecl->initializer.get())) fnPtr->originalIsLeaf = false;
            }

            return !fnPtr->originalIsLeaf;
        };

        hasFunctionCalls(node.body.get());
    }

    if (alwaysInlineNames_.count(node.name) && fnPtr->originalCallees.count(node.name)) {
        warnRecursiveForcedInline(node.name, node);
        inlineCandidates_.erase(node.name);
    }

    // Save current scope if we are nesting
    if (currentFunc_) {
        FunctionScope scope;
        scope.func = currentFunc_;
        scope.block = currentBlock_;
        scope.locals = std::move(locals_);
        scope.localTypes = std::move(localTypes_);
        scope.localTypeNames = std::move(localTypeNames_);
        scope.localSigned = std::move(localSigned_);
        scope.localPlainUnsigned = std::move(localPlainUnsigned_);
        scope.localConst = std::move(localConst_);
        scope.localPointsToConst = std::move(localPointsToConst_);
        scope.localRegister = std::move(localRegister_);
        scope.localPointedToType = std::move(localPointedToType_);
        scope.localArrayDims = std::move(localArrayDims_);
        scope.localPointedToTypeName = std::move(localPointedToTypeName_);
        scope.localPtrLevel = std::move(localPtrLevel_);
        scope.uplevelNames = std::move(uplevelNames_);
        scope.usedVregs = std::move(usedVregs_);
        scope.localDeclLocs = std::move(localDeclLocs_);
        functionStack_.push_back(std::move(scope));
    }
    // Each function starts with no locals; without this, a top-level function
    // inherited the previous function's names and vreg-use set.
    resetFunctionLocalState();

    currentFunc_ = fnPtr;
    startBlock("entry");

    // Phase 113: Track function definition for DWARF debug info
    sourceTracker_.setCurrentFile(node.sourceFile);
    sourceTracker_.trackFunctionDef(node.name, node.line, node.column, 0, 0);  // Address ranges set during codegen

    currentFuncParams_.clear();
    for (const auto& p : node.parameters) {
        currentFuncParams_.insert(p.name);
    }

    for (const auto& p : node.parameters) {
        if (p.isFunctionPointer && p.funcPtrSig) fnPtrSigs_[p.name] = p.funcPtrSig;
        bool byCopy = aggregateBlockSize(p.type, p.pointerLevel) > 0;
        if (byCopy) aggCopyParams_.insert(p.name);
        ir::Type pt = byCopy ? ir::Type::PTR : mapType(p.type, p.pointerLevel);
        auto vreg = allocVreg(pt);
        locals_[p.name] = vreg;
        localDeclLocs_[p.name] = declLoc(node);
        if (p.line > 0) {
            localDeclLocs_[p.name].line = p.line;
            localDeclLocs_[p.name].column = p.column;
            if (!p.sourceFile.empty()) localDeclLocs_[p.name].file = p.sourceFile;
        }
        if (currentFunc_) currentFunc_->localNames[p.name] = vreg.vregId;
        localTypes_[p.name] = pt;
        // Phase 102: Resolve typedef to struct name for parameter types
        std::string resolvedTypeName = p.type;
        if (resolvedTypeName.find("struct ") != 0) {
            std::string resolved = resolveTypedefToStruct(resolvedTypeName);
            if (resolved != resolvedTypeName) {
                resolvedTypeName = resolved;
            }
        }
        localTypeNames_[p.name] = resolvedTypeName;
        localPtrLevel_[p.name] = p.pointerLevel;
        // int a[][M]: indexed like an array a[0][M] at the pointer's value
        if (!p.innerArrayDims.empty()) {
            std::vector<int> d{0};
            d.insert(d.end(), p.innerArrayDims.begin(), p.innerArrayDims.end());
            paramArrayDims_[p.name] = d;
            localPtrLevel_[p.name] = p.pointerLevel - 1;
        } else {
            paramArrayDims_.erase(p.name);
        }
        localSigned_[p.name] = p.isSigned;
        // For non-pointers: const int x → isConst=true, variable is read-only
        // For pointers: const int *p → isConst=true, pointed-to is read-only, pointer itself is writable
        //               int * const p → isPointerConst=true, pointer is read-only
        localConst_[p.name] = (p.pointerLevel == 0 && p.isConst) || p.isPointerConst;
        localPointsToConst_[p.name] = p.isConst && p.pointerLevel > 0;
        if (p.pointerLevel > 0) {
            // For pointer-to-pointer (char **), pointedToType is PTR (not the base type).
            // Only single-level pointers (char *) point directly to the base type.
            if (p.pointerLevel > 1) {
                localPointedToType_[p.name] = ir::Type::PTR;
                localPointedToTypeName_[p.name] = p.type + "*";
            } else {
                localPointedToType_[p.name] = mapType(p.type, 0);
                localPointedToTypeName_[p.name] = p.type;
            }
        }
        if (p.isVolatile) currentFunc_->memoryVregs.insert(vreg.vregId);
        currentFunc_->localSlotVregs.insert(vreg.vregId);

        // Phase 113: Track parameter for DWARF debug info
        // Use function line/column since parameters don't have their own location
        sourceTracker_.trackVariableDecl(p.name, node.line, node.column, p.type, 0, true);
    }

    // Load static link from ZP if nested
    if (fnPtr->isNested) {
        // Numbered after the parameters (vregs 0..n-1 are the parameters)
        ir::Type pt = ir::Type::PTR;
        auto vreg = allocVreg(pt);
        fnPtr->staticLinkVreg = (int)vreg.vregId;
        locals_["__static_link"] = vreg;
        localTypes_["__static_link"] = pt;
        localTypeNames_["__static_link"] = "void*";
        localSigned_["__static_link"] = false;
        localConst_["__static_link"] = true;
        currentFunc_->localNames["__static_link"] = vreg.vregId;
        currentFunc_->localSlotVregs.insert(vreg.vregId);
        // its own frame slot (it lives across the body: a frame slot from
        // the temporaries' pool was shared with a parameter)
        keepStaticLink(currentFunc_);

        ir::Inst loadSl;
        loadSl.op = ir::Op::LOAD_ZP;
        loadSl.dest = vreg;
        loadSl.resultType = pt;
        loadSl.src1 = ir::Operand::global("__static_chain");
        loadSl.loc = loc(node);
        emit(loadSl);
    }

    // Variably modified parameters: evaluate their size expressions now, for
    // their side effects (sub(int i, int a[i++]) increments i)
    for (const auto& p : node.parameters)
        if (p.vlaSizeExpr) p.vlaSizeExpr->accept(*this);
    // Run-time inner dimensions of array parameters (char a[2][n]):
    // evaluated once on entry into hidden locals used for indexing
    for (const auto& p : node.parameters) {
        if (p.innerDimExprs.empty()) continue;
        std::vector<ir::Operand> ops;
        for (const auto& de : p.innerDimExprs) {
            if (!de) { ops.push_back(ir::Operand::none()); continue; }
            de->accept(*this);
            ir::Operand v = emitCast(lastValue_, ir::Type::I16, getExprTypeInfo(de.get()).isSigned);
            auto slot = allocVreg(ir::Type::I16);
            if (currentFunc_) currentFunc_->localSlotVregs.insert(slot.vregId);  // a variable
            ir::Inst st;
            st.op = ir::Op::STORE;
            st.resultType = ir::Type::I16;
            st.src1 = v;
            st.src2 = slot;
            emit(st);
            ops.push_back(slot);
        }
        paramVlaDims_[p.name] = ops;
    }

    // A nested function: the enclosing functions' variables keep their types
    // (a pointer's pointee, an array's dimensions) for expressions here; they
    // stay out of locals_ (reached through the static chain)
    if (fnPtr->isNested) {
        for (int i = (int)functionStack_.size() - 1; i >= 0; i--) {
            const auto& sc = functionStack_[i];
            for (const auto& [n, t] : sc.localTypes) {
                if (locals_.count(n) || localTypes_.count(n)) continue;
                uplevelNames_.insert(n);
                localTypes_[n] = t;
                if (auto it = sc.localTypeNames.find(n); it != sc.localTypeNames.end()) localTypeNames_[n] = it->second;
                if (auto it = sc.localSigned.find(n); it != sc.localSigned.end()) localSigned_[n] = it->second;
                if (auto it = sc.localPointedToType.find(n); it != sc.localPointedToType.end()) localPointedToType_[n] = it->second;
                if (auto it = sc.localArrayDims.find(n); it != sc.localArrayDims.end()) localArrayDims_[n] = it->second;
                if (auto it = sc.localPointedToTypeName.find(n); it != sc.localPointedToTypeName.end()) localPointedToTypeName_[n] = it->second;
                if (auto it = sc.localPtrLevel.find(n); it != sc.localPtrLevel.end()) localPtrLevel_[n] = it->second;
            }
        }
    }

    // Visit body
    node.body->accept(*this);

    // Check for unused local variables and parameters
    for (const auto& [name, vregOp] : locals_) {
        // (__sret and a method's __this are hidden parameters, a nested
        // function's __static_link its static chain)
        if (name != "__sret" && name != "__this" && name != "__static_link" && usedVregs_.find(vregOp.vregId) == usedVregs_.end() &&
            externalUsedVars_.find(name) == externalUsedVars_.end()) {
            ir::SourceLoc sl = localDeclLocs_[name];
            int line = sl.valid() ? sl.line : node.line;
            std::string file = sl.valid() ? sl.file : node.sourceFile;
            int col = sl.valid() ? sl.column : node.column;
            // A function can be lowered more than once (e.g. constant-argument specialization)
            bool isParam = std::any_of(node.parameters.begin(), node.parameters.end(),
                                       [&](const Parameter& p) { return p.name == name; });
            // assigned but never read (as GCC: the value of (c = f()) != 0
            // is the assignment's, not a read of c)
            bool isSet = setVregs_.count(vregOp.vregId) > 0;
            std::string msg = isSet ? std::string(isParam ? "parameter '" : "variable '") + name + "' set but not used"
                                    : std::string(isParam ? "unused parameter '" : "unused variable '") + name + "'";
            std::string key = file + ":" + std::to_string(line) + ":" + std::to_string(col) + ":" + name;
            if (reportedUnused_.insert(key).second)
                warnings_.push_back(formatDiagnostic(file, line, col, Severity::Warning, msg));
        }
    }

    // Ensure function ends with a return
    if (currentBlock_ && (currentBlock_->insts.empty() ||
        (currentBlock_->insts.back().op != ir::Op::RET &&
         currentBlock_->insts.back().op != ir::Op::RET_VOID &&
         currentBlock_->insts.back().op != ir::Op::BR))) {
        ir::Inst ret;
        if (currentFunc_->name == "_main" && currentFunc_->returnType != ir::Type::VOID) {
            // C99 5.1.2.2.3: reaching the } of main returns 0
            ret.op = ir::Op::RET;
            ret.resultType = currentFunc_->returnType;
            ret.src1 = ir::Operand::imm(0, currentFunc_->returnType);
        } else {
            ret.op = ir::Op::RET_VOID;
        }
        emit(ret);
    }

    if (inlineCandidates_.count(node.name) && !alwaysInlineNames_.count(node.name)) {
        int irCount = 0;
        for (const auto& blk : currentFunc_->blocks) irCount += (int)blk.insts.size();
        if (irCount > maxInlineIRSize) inlineCandidates_.erase(node.name);
    }
    // A goto in a nested function to a label of the enclosing function (a
    // non-local goto): the parent's frame is restored (FP from the static
    // link, SP = FP - 1 as at any statement of the parent) and control jumps
    // to the label's global alias in the parent
    bool nonlocalGoto = false;
    if (currentFunc_ && currentFunc_->isNested && currentFunc_->staticLinkVreg >= 0 && !functionStack_.empty()) {
        std::set<std::string> own;
        for (const auto& blk : currentFunc_->blocks) own.insert(blk.label);
        for (auto& blk : currentFunc_->blocks) {
            for (size_t k = 0; k < blk.insts.size(); k++) {
                ir::Inst br = blk.insts[k];
                if (br.op != ir::Op::BR || br.src1.kind != ir::OperandKind::LABEL || own.count(br.src1.name))
                    continue;
                nonlocalGoto = true;
                FunctionScope& parent = functionStack_.back();
                parent.func->isRecurse = true;  // a real stack frame
                nestedLabelAddrs_.insert(br.src1.name);
                ir::Inst addr;
                addr.op = ir::Op::ADDR_LABEL;
                addr.dest = allocVreg(ir::Type::I16);
                addr.src1 = br.src1;
                addr.loc = br.loc;
                ir::Inst jmp;
                jmp.op = ir::Op::BR_INDIRECT;
                jmp.src1 = addr.dest;
                jmp.args = {ir::Operand::vreg(currentFunc_->staticLinkVreg, ir::Type::PTR),
                            ir::Operand::imm(1, ir::Type::I16)};
                jmp.loc = br.loc;
                blk.insts[k] = jmp;
                blk.insts.insert(blk.insts.begin() + k, addr);
                k++;
            }
        }
    }
    if (nonlocalGoto) {
        inlineCandidates_.erase(node.name);
        alwaysInlineNames_.erase(node.name);
    }
    // __builtin_return_address(0): store the captured address at entry
    if (currentFunc_ && currentFunc_->retAddrSlot >= 0 && !currentFunc_->blocks.empty()) {
        ir::Operand slot = ir::Operand::vreg(currentFunc_->retAddrSlot, ir::Type::I16);
        auto v = allocVreg(ir::Type::I16);
        ir::Inst rd;
        rd.op = ir::Op::CPU_REG_READ; rd.dest = v; rd.resultType = ir::Type::I16; rd.asmText = "RETADDR";
        ir::Inst st;
        st.op = ir::Op::STORE; st.resultType = ir::Type::I16; st.src1 = v; st.src2 = slot;
        auto& insts = currentFunc_->blocks[0].insts;
        insts.insert(insts.begin(), st);
        insts.insert(insts.begin(), rd);
    }
    // A label whose address is stored in data (static &&label table) must
    // stay in one copy of the function: never inline it
    if (currentFunc_ && (!currentFunc_->addressTakenLabels.empty() || currentFunc_->retAddrSlot >= 0)) {
        inlineCandidates_.erase(node.name);
        alwaysInlineNames_.erase(node.name);
    }

    // Dead block elimination: remove trailing blocks after while(1) infinite loops.
    // Conservative approach: only remove blocks that follow a block ending with an
    // unconditional back-branch (BR to an earlier block) where no other block
    // branches to the trailing blocks.
    if (currentFunc_ && currentFunc_->blocks.size() > 1) {
        // Find all referenced labels (a label used from data, &&label in a
        // static table, can be reached by goto *)
        // A label whose address a nested function took: reachable by goto *
        for (auto& blk : currentFunc_->blocks)
            if (nestedLabelAddrs_.count(blk.label)) currentFunc_->addressTakenLabels.insert(blk.label);
        std::set<std::string> referenced(currentFunc_->addressTakenLabels.begin(),
                                         currentFunc_->addressTakenLabels.end());
        for (auto& blk : currentFunc_->blocks) {
            for (auto& inst : blk.insts) {
                if (inst.src1.kind == ir::OperandKind::LABEL) referenced.insert(inst.src1.name);
                if (inst.src2.kind == ir::OperandKind::LABEL) referenced.insert(inst.src2.name);
                if (inst.dest.kind == ir::OperandKind::LABEL) referenced.insert(inst.dest.name);
                for (auto& sc : inst.switchCases) referenced.insert(sc.second);
            }
        }
        // Remove blocks nothing reaches: not a branch/switch target, not an
        // address-taken label, not fallen into from the block before (e.g.
        // the code before a case label inside if (0) { ... }). Repeated until
        // stable, since removing a block can orphan the next one.
        auto isTerminator = [](const ir::Inst& in) {
            return in.op == ir::Op::BR || in.op == ir::Op::BR_COND || in.op == ir::Op::RET ||
                   in.op == ir::Op::RET_VOID || in.op == ir::Op::SWITCH || in.op == ir::Op::BR_INDIRECT;
        };
        bool removed = true;
        while (removed && currentFunc_->blocks.size() > 1) {
            removed = false;
            std::set<std::string> reach(referenced);
            for (size_t bi = 0; bi < currentFunc_->blocks.size(); bi++) {
                auto& blk = currentFunc_->blocks[bi];
                if (bi == 0) reach.insert(blk.label);
                bool term = !blk.insts.empty() && isTerminator(blk.insts.back());
                if (!term && bi + 1 < currentFunc_->blocks.size() && (bi == 0 || reach.count(blk.label)))
                    reach.insert(currentFunc_->blocks[bi + 1].label);
            }
            for (size_t bi = 1; bi < currentFunc_->blocks.size(); bi++) {
                if (!reach.count(currentFunc_->blocks[bi].label)) {
                    currentFunc_->blocks.erase(currentFunc_->blocks.begin() + bi);
                    removed = true;
                    break;
                }
            }
            if (removed) {
                // Branches from the removed block no longer count
                referenced = std::set<std::string>(currentFunc_->addressTakenLabels.begin(),
                                                   currentFunc_->addressTakenLabels.end());
                for (auto& blk : currentFunc_->blocks)
                    for (auto& inst : blk.insts) {
                        if (inst.src1.kind == ir::OperandKind::LABEL) referenced.insert(inst.src1.name);
                        if (inst.src2.kind == ir::OperandKind::LABEL) referenced.insert(inst.src2.name);
                        if (inst.dest.kind == ir::OperandKind::LABEL) referenced.insert(inst.dest.name);
                        for (auto& sc : inst.switchCases) referenced.insert(sc.second);
                    }
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
        localPlainUnsigned_ = std::move(scope.localPlainUnsigned);
        localConst_ = std::move(scope.localConst);
        localPointsToConst_ = std::move(scope.localPointsToConst);
        localRegister_ = std::move(scope.localRegister);
        localPointedToType_ = std::move(scope.localPointedToType);
        localArrayDims_ = std::move(scope.localArrayDims);
        localPointedToTypeName_ = std::move(scope.localPointedToTypeName);
        localPtrLevel_ = std::move(scope.localPtrLevel);
        uplevelNames_ = std::move(scope.uplevelNames);
        usedVregs_ = std::move(scope.usedVregs);
        localDeclLocs_ = std::move(scope.localDeclLocs);
        functionStack_.pop_back();
    } else {
        currentFunc_ = nullptr;
        currentBlock_ = nullptr;
        resetFunctionLocalState();
    }
}

bool IRBuilder::isBooleanValue(const ir::Operand& v) const {
    if (v.isImm()) return v.immVal == 0 || v.immVal == 1;
    if (v.kind != ir::OperandKind::VREG) return false;
    if (boolVregs_.count({currentFunc_, v.vregId})) return true;
    if (!currentBlock_) return false;
    for (auto it = currentBlock_->insts.rbegin(); it != currentBlock_->insts.rend(); ++it) {
        if (it->dest.kind == ir::OperandKind::VREG && it->dest.vregId == v.vregId)
            return it->op >= ir::Op::CMP_EQ && it->op <= ir::Op::CMP_GEU;
    }
    return false;
}

std::string IRBuilder::fpSigKey(const FuncPtrSignature* sig) {
    if (!sig || !sig->prototyped) return "";
    std::vector<std::pair<std::string, int>> params;
    for (const auto& p : sig->params) params.push_back({p.type, p.pointerLevel});
    return ptrSigKey(params, sig->isVariadic);
}

std::string IRBuilder::ptrSigKey(const std::vector<std::pair<std::string, int>>& params, bool variadic) {
    std::string key = "s";
    for (size_t i = 0; i < params.size(); i++) {
        const auto& [type, ptr] = params[i];
        int size = aggregateBlockSize(type, ptr) ? 2 : ir::typeSize(mapType(type, ptr));
        key += (i ? "_" : "") + std::to_string(size < 2 ? 2 : size);
    }
    return variadic ? key + "v" : key;
}

int IRBuilder::aggregateBlockSize(const std::string& typeName, int pointerLevel) {
    if (pointerLevel > 0) return 0;
    if (!structs_.count(getAggregateName(typeName))) return 0;
    // A struct with run-time member sizes is always handled by address
    {
        std::string vn = getAggregateName(typeName);
        if (vn.rfind("struct ", 0) == 0) vn = vn.substr(7);
        else if (vn.rfind("union ", 0) == 0) vn = vn.substr(6);
        if (vmStructs_.count(vn)) return std::max(6, structs_[getAggregateName(typeName)].totalSize);
    }
    int size = getTypeSize(typeName, 0);
    return (size == 1 || size == 2 || size == 4) ? 0 : size;
}

// Link-time constant address for a global initializer: &global, or the name of
// a global array or a function (optionally cast). Returns the assembler label,
// or "" when the expression isn't such an address.
std::string IRBuilder::staticAddressLabel(Expression* e) {
    while (auto* c = dynamic_cast<CastExpression*>(e)) e = c->expression.get();
    bool addrOf = false;
    if (auto* u = dynamic_cast<UnaryOperation*>(e)) {
        if (u->op != "&") return "";
        e = u->operand.get();
        addrOf = true;
    }
    auto* v = dynamic_cast<VariableReference*>(e);
    if (!v || locals_.count(v->name)) return "";
    if (globalTypes_.count(v->name)) {
        if (addrOf || globalArrayDims_.count(v->name)) return "_" + v->name;
        return "";
    }
    if (functionReturnTypes_.count(v->name) || staticRenames_.count(v->name) ||
        definedFunctions_.count("_" + v->name)) {
        std::string label = "_" + resolvedFunctionName(v->name);
        calledFunctions_.insert(label);  // extern if defined elsewhere; keeps it alive
        return label;
    }
    return "";
}

// ---------------------------------------------------------------------------
// Link-time constant addresses: label + offset
// ---------------------------------------------------------------------------

static bool constIntOf(Expression* e, int64_t& v) {
    while (auto* c = dynamic_cast<CastExpression*>(e)) e = c->expression.get();
    if (auto* i = dynamic_cast<IntegerLiteral*>(e)) { v = i->value; return true; }
    if (auto* u = dynamic_cast<UnaryOperation*>(e); u && (u->op == "-" || u->op == "+")) {
        if (!constIntOf(u->operand.get(), v)) return false;
        if (u->op == "-") v = -v;
        return true;
    }
    return false;
}

// Element size and dimensions of an array-valued expression (a global or
// local static array, an array member, a sub-array, a string literal)
bool IRBuilder::arrayShape(Expression* e, int& elemSize, std::vector<int>& dims) {
    if (auto* v = dynamic_cast<VariableReference*>(e)) {
        auto lit = locals_.find(v->name);
        bool local = lit != locals_.end();
        if (local && lit->second.kind != ir::OperandKind::GLOBAL) return false;
        auto& dimMap = local ? localArrayDims_ : globalArrayDims_;
        auto dit = dimMap.find(v->name);
        if (dit == dimMap.end() || dit->second.empty()) return false;
        dims = dit->second;
        bool ptrElem = local ? localPointedToType_.count(v->name) > 0 : globalPointedToType_.count(v->name) > 0;
        std::string tn = local ? localTypeNames_[v->name] : globalTypeNames_[v->name];
        elemSize = ptrElem ? 2 : getTypeSize(tn, 0);
        return elemSize > 0;
    }
    if (auto* str = dynamic_cast<StringLiteral*>(e)) {
        elemSize = 1;
        dims = {(int)str->value.size() + 1};
        return true;
    }
    if (auto* ma = dynamic_cast<MemberAccess*>(e)) {
        auto* mi = arrayMemberOf(ma);
        if (!mi || mi->arrayDims.empty()) return false;
        elemSize = getTypeSize(mi->type, mi->pointerLevel);
        dims = mi->arrayDims;
        return true;
    }
    if (auto* aa = dynamic_cast<ArrayAccess*>(e)) {
        if (!arrayShape(aa->arrayExpr.get(), elemSize, dims) || dims.size() < 2) return false;
        dims.erase(dims.begin());
        return true;
    }
    return false;
}

// The address of an lvalue that lives at a fixed place: label + offset
bool IRBuilder::constLvalue(Expression* e, std::string& label, int& off) {
    while (auto* c = dynamic_cast<CastExpression*>(e)) e = c->expression.get();
    if (auto* v = dynamic_cast<VariableReference*>(e)) {
        auto lit = locals_.find(v->name);
        if (lit != locals_.end()) {
            if (lit->second.kind != ir::OperandKind::GLOBAL) return false;  // a frame local
            label = lit->second.name;
        } else if (globalTypes_.count(v->name)) {
            label = "_" + v->name;
        } else {
            return false;
        }
        off = 0;
        return true;
    }
    if (dynamic_cast<StringLiteral*>(e)) return constAddress(e, label, off);
    if (auto* aa = dynamic_cast<ArrayAccess*>(e)) {
        int64_t k;
        if (!constIntOf(aa->indexExpr.get(), k)) return false;
        int es; std::vector<int> dims;
        if (arrayShape(aa->arrayExpr.get(), es, dims)) {
            int stride = es;
            for (size_t i = 1; i < dims.size(); i++) stride *= dims[i];
            if (!constLvalue(aa->arrayExpr.get(), label, off)) return false;
            off += (int)k * stride;
            return true;
        }
        // p[k] on a constant pointer
        if (!constAddress(aa->arrayExpr.get(), label, off)) return false;
        off += (int)k * pointeeStride(aa->arrayExpr.get());
        return true;
    }
    if (auto* ma = dynamic_cast<MemberAccess*>(e)) {
        std::string t = getExprTypeInfo(ma->structExpr.get()).typeName;
        if (t.rfind("struct ", 0) != 0 && t.rfind("union ", 0) != 0) t = resolveTypedefToStruct(t);
        std::string sName = getAggregateName(t);
        int acc = 0;
        auto* mi = structs_.count(sName) ? findStructMember(sName, ma->memberName, acc) : nullptr;
        if (!mi || mi->bitWidth > 0) return false;
        bool ok = ma->isArrow ? constAddress(ma->structExpr.get(), label, off)
                              : constLvalue(ma->structExpr.get(), label, off);
        if (!ok) return false;
        off += mi->offset + acc;
        return true;
    }
    if (auto* u = dynamic_cast<UnaryOperation*>(e); u && u->op == "*") return constAddress(u->operand.get(), label, off);
    return false;
}

// Size of what a constant pointer expression points to (for p + k)
// A pointer to an array (T (*p)[M], or an int a[][M] parameter): the size of
// the array it points to (0 for anything else, or run-time dimensions)
int IRBuilder::ptrArrayRowSize(Expression* e) {
    auto* vr = dynamic_cast<VariableReference*>(e);
    if (!vr) return 0;
    auto pit = paramArrayDims_.find(vr->name);
    if (pit == paramArrayDims_.end() || pit->second.size() < 2) return 0;
    if (paramVlaDims_.count(vr->name)) return 0;
    bool local = localTypes_.count(vr->name) > 0;
    std::string tn = local ? localTypeNames_[vr->name] : globalTypeNames_[vr->name];
    int lvl = local ? (localPtrLevel_.count(vr->name) ? localPtrLevel_[vr->name] : 0)
                    : (globalPtrLevel_.count(vr->name) ? globalPtrLevel_[vr->name] : 0);
    int size = std::max(1, getTypeSize(tn, lvl));
    for (size_t i = 1; i < pit->second.size(); i++) size *= std::max(1, pit->second[i]);
    return size;
}

int IRBuilder::pointeeStride(Expression* e) {
    if (int row = ptrArrayRowSize(e)) return row;
    while (auto* c = dynamic_cast<CastExpression*>(e)) {
        if (c->pointerLevel > 0) return std::max(1, getTypeSize(c->targetType, c->pointerLevel - 1));
        e = c->expression.get();
    }
    int es; std::vector<int> dims;
    if (arrayShape(e, es, dims)) {
        int stride = es;
        for (size_t i = 1; i < dims.size(); i++) stride *= dims[i];
        return stride;
    }
    IRTypeInfo ti = getExprTypeInfo(e);
    if (ti.type == ir::Type::PTR && ti.pointedToType != ir::Type::VOID) return std::max(1, (int)ir::typeSize(ti.pointedToType));
    return 1;
}

// A link-time constant address value: "abc" + 1, &a[3][5], &s.m, arr + 2,
// a function or array name, &x (globals and local statics)
bool IRBuilder::constAddress(Expression* e, std::string& label, int& off) {
    Expression* inner = e;
    while (auto* c = dynamic_cast<CastExpression*>(inner)) inner = c->expression.get();
    // A pointer made from an integer ((T *)0 in offsetof): no label
    if (auto* il = dynamic_cast<IntegerLiteral*>(inner); il && (il->castPointerLevel > 0 || inner != e)) {
        label.clear();
        off = (int)il->value;
        return true;
    }
    if (auto* str = dynamic_cast<StringLiteral*>(inner)) {
        label = "__str_" + std::to_string(nextLabel_++);
        ir::Module::StringLiteral sl;
        sl.label = label;
        sl.value = str->value;
        sl.escMask = str->escMask;
        sl.encoding = stringEncodingOf(*str);
        sl.isAscii = sl.encoding == 1;
        // (in a function's code: the literal's own; in a static initializer:
        // only for a __noescape object's)
        sl.noescape = initNoescape_ || (str->noescape && currentFunc_ && !inStaticInit_);
        sl.far = sl.noescape && currentFunc_ && currentFunc_->isFar;
        module_.strings.push_back(sl);
        off = 0;
        return true;
    }
    if (auto* u = dynamic_cast<UnaryOperation*>(inner); u && u->op == "&") {
        if (constLvalue(u->operand.get(), label, off)) return true;
        // &f, a function: its address (as f; #326)
        std::string l = staticAddressLabel(inner);
        if (l.empty()) return false;
        label = l;
        off = 0;
        return true;
    }
    if (auto* b = dynamic_cast<BinaryOperation*>(inner); b && (b->op == "+" || b->op == "-")) {
        int64_t k;
        if (constIntOf(b->right.get(), k) && constAddress(b->left.get(), label, off)) {
            int stride = pointeeStride(b->left.get());
            off += (int)(b->op == "+" ? k : -k) * stride;
            return true;
        }
        if (b->op == "+" && constIntOf(b->left.get(), k) && constAddress(b->right.get(), label, off)) {
            off += (int)k * pointeeStride(b->right.get());
            return true;
        }
        return false;
    }
    if (auto* v = dynamic_cast<VariableReference*>(inner)) {
        // An array decays to its address; a function name is its address
        int es; std::vector<int> dims;
        if (arrayShape(inner, es, dims)) return constLvalue(inner, label, off);
        std::string l = staticAddressLabel(inner);
        if (l.empty()) return false;
        label = l;
        off = 0;
        return true;
    }
    if (auto* aa = dynamic_cast<ArrayAccess*>(inner)) {
        // A sub-array decays: a[1] of int a[3][4]
        int es; std::vector<int> dims;
        if (arrayShape(inner, es, dims)) return constLvalue(inner, label, off);
        (void)aa;
    }
    if (auto* ma = dynamic_cast<MemberAccess*>(inner)) {
        int es; std::vector<int> dims;
        if (arrayShape(inner, es, dims)) return constLvalue(inner, label, off);
        (void)ma;
    }
    return false;
}

static std::string labelPlus(const std::string& label, int off) {
    if (off == 0) return label;
    return label + (off > 0 ? "+" : "-") + std::to_string(off > 0 ? off : -off);
}

// ---------------------------------------------------------------------------
// Constant initializer images (aggregate / array globals and local statics)
// ---------------------------------------------------------------------------

struct IRBuilder::InitImageCtx {
    ir::Module::GlobalVar* gv;
    // Run-time mode (a local's initializer): scalars are evaluated and
    // stored at base + offset instead of being laid out in gv's image
    bool runtime = false;
    ir::Operand base;
    ir::SourceLoc sl;
};

const IRBuilder::IRStructInfo* IRBuilder::structOf(const std::string& type) {
    std::string t = type;
    if (t.rfind("struct ", 0) != 0 && t.rfind("union ", 0) != 0) t = resolveTypedefToStruct(t);
    auto it = structs_.find(getAggregateName(t));
    return it == structs_.end() ? nullptr : &it->second;
}

int IRBuilder::objectSize(const std::string& type, int ptr, const std::vector<int>& dims) {
    int n = getTypeSize(type, ptr);
    for (int d : dims) n *= d;
    return n;
}

void IRBuilder::collectVtableSlots(const std::string& type, int ptrLevel, const std::vector<int>& dims, int base,
                                   std::vector<std::pair<int, std::string>>& out) {
    if (ptrLevel > 0) return;
    const IRStructInfo* si = structOf(type);
    if (!si || si->isUnion) return;
    int count = 1;
    for (int d : dims) count *= d;
    for (int k = 0; k < count; k++) {
        int off = base + k * si->totalSize;
        auto vt = si->members.find("__vt");
        if (vt != si->members.end()) out.push_back({off + vt->second.offset, "_" + si->name + "_vtable"});
        for (const auto& mn : si->memberOrder) {
            if (mn == "__vt") continue;
            auto mit = si->members.find(mn);
            if (mit != si->members.end() && mit->second.bitWidth == 0)
                collectVtableSlots(mit->second.type, mit->second.pointerLevel, mit->second.arrayDims, off + mit->second.offset, out);
        }
    }
}

void IRBuilder::addVtableWords(ir::Module::GlobalVar& gv, const std::string& type, int ptrLevel, const std::vector<int>& dims) {
    std::vector<std::pair<int, std::string>> slots;
    collectVtableSlots(type, ptrLevel, dims, 0, slots);
    if (slots.empty()) return;
    if (!gv.hasImage) {
        // no (laid-out) initializer: a zero object with the __vt words
        if (gv.hasInitValue && (gv.initValue != 0 || !gv.initList.empty() || !gv.initLabels.empty())) return;
        gv.initBytes.assign((size_t)std::max(0, objectSize(type, ptrLevel, dims)), 0);
        gv.initWords.clear();
        gv.initStrings.clear();
        gv.hasImage = true;
        gv.hasInitValue = true;
    }
    for (const auto& [off, sym] : slots) gv.initWords[off] = sym;
}

// A name in a static initializer that is declared nowhere is an error, as
// in a function body (the image builders would otherwise store 0 for it)
void IRBuilder::checkStaticInitNames(Expression* e) {
    if (!e) return;
    if (auto* vr = dynamic_cast<VariableReference*>(e)) {
        const std::string& n = vr->name;
        if (!locals_.count(n) && !globalTypes_.count(n) && !globalArrayDims_.count(n) &&
            !functionParamTypes_.count(n) && !functionReturnTypes_.count(n) && !unitFunctionNames_.count(n) &&
            n.rfind("__", 0) != 0)
            throw std::runtime_error(formatDiagnostic(vr->sourceFile, vr->line, vr->column, Severity::Error,
                "'" + n + "' undeclared"));
        return;
    }
    if (auto* il = dynamic_cast<InitializerList*>(e)) {
        for (auto& el : il->elements) checkStaticInitNames(el.get());
    } else if (auto* bo = dynamic_cast<BinaryOperation*>(e)) {
        checkStaticInitNames(bo->left.get());
        checkStaticInitNames(bo->right.get());
    } else if (auto* uo = dynamic_cast<UnaryOperation*>(e)) {
        checkStaticInitNames(uo->operand.get());
    } else if (auto* ce = dynamic_cast<CastExpression*>(e)) {
        checkStaticInitNames(ce->expression.get());
    } else if (auto* co = dynamic_cast<ConditionalExpression*>(e)) {
        checkStaticInitNames(co->condition.get());
        checkStaticInitNames(co->thenExpr.get());
        checkStaticInitNames(co->elseExpr.get());
    } else if (auto* aa = dynamic_cast<ArrayAccess*>(e)) {
        checkStaticInitNames(aa->arrayExpr.get());
        checkStaticInitNames(aa->indexExpr.get());
    } else if (auto* ma = dynamic_cast<MemberAccess*>(e)) {
        checkStaticInitNames(ma->structExpr.get());
    }
}

bool IRBuilder::buildInitImage(ir::Module::GlobalVar& gv, const std::string& type, int ptrLevel,
                               const std::vector<int>& dims, Expression* init) {
    ir::Module::GlobalVar trial = gv;
    trial.initBytes.assign((size_t)std::max(0, objectSize(type, ptrLevel, dims)), 0);
    trial.initWords.clear();
    trial.initStrings.clear();
    InitImageCtx c{&trial};
    if (!initObject(c, 0, type, ptrLevel, dims, init)) return false;
    trial.hasImage = true;
    trial.hasInitValue = true;
    gv = trial;
    return true;
}

// A local array/struct/union with a braced initializer (or a char array from
// a string): an all-constant initializer is laid out like a static one and
// copied in; otherwise the object is zeroed and each value stored. Returns
// false (nothing emitted) when the initializer can't be laid out this way.
bool IRBuilder::initLocalAggregate(const ir::Operand& vreg, const std::string& type, int ptr,
                                   const std::vector<int>& dims, Expression* init, const ir::SourceLoc& sl) {
    int size = objectSize(type, ptr, dims);
    if (size <= 0) return false;
    ir::Module::GlobalVar image;
    image.name = "_" + currentFunc_->name + "__init_" + std::to_string(nextLabel_++);
    image.type = ir::Type::I8;
    image.size = size;
    image.isConst = true;
    image.isStatic = true;
    // only this function copies from it: kept with its code (Issue #301)
    image.noescape = true;
    image.align = std::max(1, typeAlign(type, ptr));
    bool constant = buildInitImage(image, type, ptr, dims, init);
    auto base = allocVreg(ir::Type::PTR);
    ir::Inst addr;
    addr.op = ir::Op::ADDR_LOCAL;
    addr.dest = base;
    addr.resultType = ir::Type::PTR;
    addr.src1 = ir::Operand::vreg(vreg.vregId, ir::Type::PTR);
    addr.loc = sl;
    emit(addr);
    currentFunc_->memoryVregs.insert(vreg.vregId);
    if (constant) {
        module_.globals.push_back(image);
        emitBlockCopy(base, ir::Operand::global(image.name), size, sl);
        return true;
    }
    // Zero, then store each value
    ir::Inst call;
    call.op = ir::Op::CALL_VOID;
    call.resultType = ir::Type::VOID;
    call.src1 = ir::Operand::global("_memset");
    call.args = {base, ir::Operand::imm(0, ir::Type::I16), ir::Operand::imm(size, ir::Type::I16)};
    call.callConv = ir::CallConv::ZP;   // the library's memset is ZP asm (both libraries)
    call.loc = sl;
    emit(call);
    calledFunctions_.insert("_memset");
    InitImageCtx c{&image};
    c.runtime = true;
    c.base = base;
    c.sl = sl;
    initObject(c, 0, type, ptr, dims, init);
    return true;
}

// One object (scalar, array or struct) initialized by a single initializer
bool IRBuilder::initObject(InitImageCtx& c, int off, const std::string& type, int ptr, std::vector<int> dims,
                           Expression* e) {
    Expression* orig = e;
    while (auto* cast = dynamic_cast<CastExpression*>(e)) e = cast->expression.get();
    bool isArray = !dims.empty();
    const IRStructInfo* si = (!isArray && ptr == 0) ? structOf(type) : nullptr;
    // (T){...} of this aggregate type (a GNU constant initializer): its list
    if (auto* cl = dynamic_cast<CompoundLiteral*>(e); cl && cl->initializer && cl->pointerLevel == 0 &&
        (isArray ? !cl->arrayDims.empty() : (si && cl->arrayDims.empty() && structOf(cl->targetType) == si)))
        e = cl->initializer.get();
    if (auto* list = dynamic_cast<InitializerList*>(e)) {
        if (!isArray && !si) {
            // Scalar in braces: int x = {5};
            if (list->elements.empty()) return true;
            return initScalar(c, off, type, ptr, 0, 0, list->elements[0].get());
        }
        size_t idx = 0;
        return fillAggregate(c, off, type, ptr, dims, *list, idx, true);
    }
    if (auto* str = dynamic_cast<StringLiteral*>(e)) {
        if (isArray && dims.size() == 1 && ptr == 0 && getTypeSize(type, 0) == 1) {
            // char a[N] = "text": the text (cut to N), the rest stays zero
            ir::Module::GlobalVar::ImageString is;
            int n = dims[0] > 0 ? std::min<int>(dims[0], (int)str->value.size()) : (int)str->value.size();
            is.value = str->value.substr(0, n);
            is.escMask = str->escMask.substr(0, std::min(str->escMask.size(), (size_t)n));
            is.encoding = stringEncodingOf(*str);
            if (c.runtime) {
                // Copy the text from the string literal (the rest is zeroed)
                if (n > 0) {
                    str->accept(*this);
                    emitBlockCopy(emitPtrOffset(c.base, off, c.sl), lastValue_, n, c.sl);
                }
                return true;
            }
            if (n > 0) c.gv->initStrings[off] = is;
            return true;
        }
    }
    if (c.runtime && !isArray && si) {
        // A struct member initialized from another struct value. One of 1, 2
        // or 4 bytes is carried as an integer value: store it as one
        int sz = objectSize(type, ptr, dims);
        if (aggregateBlockSize(type, ptr) == 0) {
            ir::Type t = mapType(type, ptr);
            bool saved = computeAddressOnly_;
            computeAddressOnly_ = false;
            orig->accept(*this);
            computeAddressOnly_ = saved;
            ir::Inst st;
            st.op = ir::Op::STORE;
            st.resultType = t;
            st.src1 = lastValue_;
            st.src2 = emitPtrOffset(c.base, off, c.sl);
            st.loc = c.sl;
            emit(st);
            return true;
        }
        emitAggregateInto(orig, emitPtrOffset(c.base, off, c.sl), sz, c.sl);
        return true;
    }
    if (isArray || si) return false;  // e.g. a struct copied from another variable
    return initScalar(c, off, type, ptr, 0, 0, c.runtime ? orig : e);
}

// The next sub-object of an aggregate from list[idx]: a braced initializer, a
// string for a char array, or (brace elision) as many scalars as it takes
bool IRBuilder::initElement(InitImageCtx& c, int off, const std::string& type, int ptr, std::vector<int> dims,
                            InitializerList& list, size_t& idx) {
    Expression* e = list.elements[idx].get();
    Expression* bare = e;
    while (auto* cast = dynamic_cast<CastExpression*>(bare)) bare = cast->expression.get();
    bool isArray = !dims.empty();
    bool aggregate = isArray || (ptr == 0 && structOf(type));
    // A complex member from a complex (or real) value: {re, im} when constant,
    // else the value converted to the member's complex type
    if (aggregate && !isArray && !dynamic_cast<InitializerList*>(bare)) {
        std::string sn = getAggregateName(type);
        if (sn.rfind("_Complex_", 0) == 0) {
            double re = 0, im = 0;
            if (evalConstComplex(bare, re, im)) {
                bool isFloat = sn == "_Complex_float";
                auto il = std::make_unique<InitializerList>();
                il->line = e->line; il->column = e->column; il->sourceFile = e->sourceFile;
                for (double v : {re, im}) {
                    if (isFloat) il->elements.push_back(std::make_unique<FloatLiteral>(v));
                    else il->elements.push_back(std::make_unique<IntegerLiteral>((int64_t)v));
                }
                list.elements[idx] = std::move(il);
            } else {
                toComplex(list.elements[idx], sn);
            }
            e = list.elements[idx].get();
            ++idx;
            return initObject(c, off, type, ptr, dims, e);
        }
    }
    // A struct-typed value initializes the whole member (no brace elision)
    bool wholeStruct = false;
    if (aggregate && !isArray && !dynamic_cast<InitializerList*>(bare) && !dynamic_cast<StringLiteral*>(bare)) {
        IRTypeInfo ti = getExprTypeInfo(bare);
        wholeStruct = (ti.type != ir::Type::PTR || isAggCopyParam(bare)) && !ti.typeName.empty() &&
                      structOf(ti.typeName) == structOf(type);
    }
    if (dynamic_cast<InitializerList*>(bare) || !aggregate || wholeStruct ||
        (isArray && dims.size() == 1 && ptr == 0 && getTypeSize(type, 0) == 1 && dynamic_cast<StringLiteral*>(bare))) {
        ++idx;
        return initObject(c, off, type, ptr, dims, e);
    }
    return fillAggregate(c, off, type, ptr, dims, list, idx, false);
}

// The sub-objects of an array or struct from list[idx...]. braced: the list is
// this object's own braces (designators apply); otherwise brace elision stops
// at a designator, which belongs to an enclosing object.
bool IRBuilder::fillAggregate(InitImageCtx& c, int off, const std::string& type, int ptr, std::vector<int> dims,
                              InitializerList& list, size_t& idx, bool braced) {
    // Without braces, the first element's designator is the one that led
    // here (.m = 0 for a union member m); a later one belongs to an enclosing
    // object and ends this sub-object
    size_t first = idx;
    auto designatorAt = [&](size_t i) -> const InitializerList::Designator* {
        if (!braced && i == first) return nullptr;
        return i < list.designators.size() && list.designators[i].isDesignated() ? &list.designators[i] : nullptr;
    };
    if (!dims.empty()) {
        std::vector<int> elemDims(dims.begin() + 1, dims.end());
        int esz = objectSize(type, ptr, elemDims);
        int n = dims[0];
        // A designator may move back after the end was reached ([2].y, [2].x)
        for (int k = 0; idx < list.elements.size(); ++k) {
            if (auto* d = designatorAt(idx)) {
                if (!braced) return true;
                if (d->arrayIndex < 0) return false;
                k = d->arrayIndex;
                if (n > 0 && k >= n) return false;
            } else if (n > 0 && k >= n) {
                break;
            }
            size_t before = idx;
            if (!initElement(c, off + k * esz, type, ptr, elemDims, list, idx)) return false;
            if (idx == before) return false;  // no progress: malformed
        }
        return true;
    }
    const IRStructInfo* si = structOf(type);
    if (!si) return false;
    const auto& order = si->memberOrder;
    for (size_t m = 0; idx < list.elements.size(); ++m) {
        if (auto* d = designatorAt(idx)) {
            if (!braced) return true;
            if (d->memberName.empty()) return false;
            auto pos = std::find(order.begin(), order.end(), d->memberName);
            if (pos == order.end()) return false;
            m = (size_t)(pos - order.begin());
        } else if (m >= order.size()) {
            break;
        } else if (order[m].rfind("__unnamed_bf_", 0) == 0) {
            continue;  // an unnamed bitfield is not initialized
        }
        const IRMemberInfo& mi = si->members.at(order[m]);
        size_t before = idx;
        if (mi.bitWidth > 0) {
            if (!initScalar(c, off + mi.offset, mi.type, 0, mi.bitWidth, mi.bitOffset, list.elements[idx].get()))
                return false;
            ++idx;
        } else if (!initElement(c, off + mi.offset, mi.type, mi.pointerLevel, mi.arrayDims, list, idx)) {
            return false;
        }
        if (idx == before) return false;  // no progress: malformed
        if (si->isUnion) break;  // a union initializes one member
    }
    return true;
}

// A scalar: integer, float, or an address (string literal, &var, array or
// function name) written little-endian at off
bool IRBuilder::initScalar(InitImageCtx& c, int off, const std::string& type, int ptr, int bitWidth, int bitOffset,
                           Expression* e) {
    if (c.runtime) {
        // Evaluate and store, converted to the object's type
        ir::Type t = mapType(type, ptr);
        bool saved = computeAddressOnly_;
        computeAddressOnly_ = false;
        e->accept(*this);
        computeAddressOnly_ = saved;
        ir::Operand v = lastValue_;
        if (v.type != t && v.type != ir::Type::VOID && t != ir::Type::I_N)
            v = emitCast(v, t, getExprTypeInfo(e).isSigned);
        if (bitWidth > 0) {
            // Insert into the storage unit (as an assignment to the field does)
            auto addr = emitPtrOffset(c.base, off, c.sl);
            auto unit = allocVreg(t);
            ir::Inst bf;
            bf.op = ir::Op::BFINS;
            bf.resultType = t;
            bf.dest = unit;
            bf.src1 = v;
            bf.src2 = addr;
            bf.args.push_back(ir::Operand::imm(bitOffset, ir::Type::I8));
            bf.args.push_back(ir::Operand::imm(bitWidth, ir::Type::I8));
            bf.loc = c.sl;
            emit(bf);
            ir::Inst st;
            st.op = ir::Op::STORE;
            st.resultType = t;
            st.src1 = unit;
            st.src2 = addr;
            st.loc = c.sl;
            emit(st);
            return true;
        }
        ir::Inst st;
        st.op = ir::Op::STORE;
        st.resultType = t;
        st.src1 = v;
        st.src2 = emitPtrOffset(c.base, off, c.sl);
        st.loc = c.sl;
        emit(st);
        return true;
    }
    Expression* orig = e;   // (with its casts: a number's value)
    while (auto* cast = dynamic_cast<CastExpression*>(e)) e = cast->expression.get();
    auto& bytes = c.gv->initBytes;
    ir::Type t = mapType(type, ptr);
    int size = ptr > 0 ? 2 : getTypeSize(type, 0);
    if (off < 0 || off + std::max(size, 1) > (int)bytes.size()) return false;
    // Addresses: a 2-byte relocated word (label + offset)
    {
        std::string label;
        int aoff = 0;
        if (size == 2 && constAddress(e, label, aoff)) {
            if (label.empty()) {
                // A plain number (offsetof): its bytes
                c.gv->initBytes[off] = (uint8_t)aoff;
                c.gv->initBytes[off + 1] = (uint8_t)(aoff >> 8);
            } else {
                c.gv->initWords[off] = labelPlus(label, aoff);
            }
            return true;
        }
    }
    // &&a - &&b: the distance between two labels of this function
    if (auto* bo = dynamic_cast<BinaryOperation*>(e); bo && bo->op == "-" && currentFunc_) {
        Expression* l = bo->left.get();
        Expression* r = bo->right.get();
        while (auto* cc = dynamic_cast<CastExpression*>(l)) l = cc->expression.get();
        while (auto* cc = dynamic_cast<CastExpression*>(r)) r = cc->expression.get();
        auto* la = dynamic_cast<LabelAddressExpression*>(l);
        auto* lb = dynamic_cast<LabelAddressExpression*>(r);
        if (la && lb && size == 2) {
            currentFunc_->addressTakenLabels.insert(la->label);
            currentFunc_->addressTakenLabels.insert(lb->label);
            c.gv->initWords[off] = currentFunc_->labelAlias(la->label) + "-" + currentFunc_->labelAlias(lb->label);
            return true;
        }
    }
    // &&label (a static goto * table): the label's global alias
    if (auto* la = dynamic_cast<LabelAddressExpression*>(e); la && currentFunc_) {
        if (size != 2) return false;
        currentFunc_->addressTakenLabels.insert(la->label);
        c.gv->initWords[off] = currentFunc_->labelAlias(la->label);
        return true;
    }
    // Numbers (ConstEval: an integer as its type says, (signed char)200 is -56)
    ConstValue cv;
    if (!ConstEval::eval(orig, *this, cv)) return false;
    bool isFloat = cv.isFloat();
    double fv = cv.asDouble();
    int64_t iv = isFloat ? 0 : cv.intValue();
    if (t == ir::Type::F32 && ptr == 0) {
        uint8_t cbm[5];
        doubleToCBM40(isFloat ? fv : (double)iv, cbm);
        for (int i = 0; i < 5 && off + i < (int)bytes.size(); ++i) bytes[off + i] = cbm[i];
        return true;
    }
    if (isFloat) iv = (int64_t)fv;
    if (bitWidth > 0) {
        // OR the field into its storage unit (little-endian)
        uint64_t unit = 0;
        for (int i = 0; i < size; ++i) unit |= (uint64_t)bytes[off + i] << (8 * i);
        uint64_t mask = (bitWidth >= 64) ? ~0ull : ((1ull << bitWidth) - 1);
        unit = (unit & ~(mask << bitOffset)) | (((uint64_t)iv & mask) << bitOffset);
        for (int i = 0; i < size; ++i) bytes[off + i] = (uint8_t)(unit >> (8 * i));
        return true;
    }
    for (int i = 0; i < size; ++i) bytes[off + i] = (uint8_t)((uint64_t)iv >> (8 * i));
    return true;
}

// ---------------------------------------------------------------------------
// _Complex: the types are struct _Complex_float / _Complex_int (built-in
// definitions, see ComplexBuiltin.hpp) and arithmetic is their overloaded
// operators. Before an operator is dispatched, real operands, casts to a
// complex type and int complex operands mixed with float complex ones are
// rewritten as compound literals of the common complex type.
// ---------------------------------------------------------------------------

// A struct/union parameter passed by value (its vreg holds the copy's address)
bool IRBuilder::isAggCopyParam(Expression* e) {
    auto* v = dynamic_cast<VariableReference*>(e);
    return v && aggCopyParams_.count(v->name) && localTypes_.count(v->name);
}

// "_Complex_float", "_Complex_int" or "" (not a complex value)
std::string IRBuilder::complexKindOf(Expression* e) {
    auto kindOfType = [&](const std::string& t) -> std::string {
        std::string n = getAggregateName(t.rfind("struct ", 0) == 0 ? t : resolveTypedefToStruct(t));
        return (n == "_Complex_float" || n == "_Complex_int") ? n : "";
    };
    if (auto* c = dynamic_cast<CastExpression*>(e))
        return c->pointerLevel ? "" : kindOfType(c->targetType);
    if (auto* cl = dynamic_cast<CompoundLiteral*>(e))
        return (cl->pointerLevel || !cl->arrayDims.empty()) ? "" : kindOfType(cl->targetType);
    if (auto* b = dynamic_cast<BinaryOperation*>(e)) {
        if (b->op == "+" || b->op == "-" || b->op == "*" || b->op == "/") {
            std::string kl = complexKindOf(b->left.get()), kr = complexKindOf(b->right.get());
            if (kl.empty() && kr.empty()) return "";
            if (kl == "_Complex_float" || kr == "_Complex_float") return "_Complex_float";
            Expression* real = kl.empty() ? b->left.get() : (kr.empty() ? b->right.get() : nullptr);
            if (real && getExprTypeInfo(real).type == ir::Type::F32) return "_Complex_float";
            return "_Complex_int";
        }
        if (b->op == ",") return complexKindOf(b->right.get());
        return "";
    }
    if (auto* u = dynamic_cast<UnaryOperation*>(e)) {
        if (u->op == "-" || u->op == "+" || u->op == "~") return complexKindOf(u->operand.get());
    }
    IRTypeInfo info = getExprTypeInfo(e);
    if ((info.type == ir::Type::PTR && !isAggCopyParam(e)) || info.typeName.empty()) return "";
    return kindOfType(info.typeName);
}

// A copy of a side-effect-free lvalue/constant expression, or nullptr
std::unique_ptr<Expression> IRBuilder::cloneSimpleExpr(Expression* e) {
    std::unique_ptr<Expression> r;
    if (auto* v = dynamic_cast<VariableReference*>(e)) r = std::make_unique<VariableReference>(v->name);
    else if (auto* i = dynamic_cast<IntegerLiteral*>(e)) {
        auto l = std::make_unique<IntegerLiteral>(i->value);
        l->castType = i->castType; l->castPointerLevel = i->castPointerLevel; l->castIsSigned = i->castIsSigned;
        r = std::move(l);
    } else if (auto* f = dynamic_cast<FloatLiteral*>(e)) r = std::make_unique<FloatLiteral>(f->value);
    else if (auto* m = dynamic_cast<MemberAccess*>(e)) {
        auto s = cloneSimpleExpr(m->structExpr.get());
        if (s) r = std::make_unique<MemberAccess>(std::move(s), m->memberName, m->isArrow);
    } else if (auto* a = dynamic_cast<ArrayAccess*>(e)) {
        auto b = cloneSimpleExpr(a->arrayExpr.get()), x = cloneSimpleExpr(a->indexExpr.get());
        if (b && x) r = std::make_unique<ArrayAccess>(std::move(b), std::move(x));
    } else if (auto* u = dynamic_cast<UnaryOperation*>(e)) {
        if (u->op == "*" || u->op == "-" || u->op == "&") {
            auto o = cloneSimpleExpr(u->operand.get());
            if (o) r = std::make_unique<UnaryOperation>(u->op, std::move(o));
        }
    }
    if (r) { r->line = e->line; r->column = e->column; r->sourceFile = e->sourceFile; }
    return r;
}

// Rewrite e as a value of complex type `kind` (no-op when it already is one)
// Wide integers (__int(N), struct __intN { char bytes[N/8]; }): the struct
// name when e has that type, else ""
std::string IRBuilder::wideKindOf(Expression* e) {
    // a comparison or logical operation is an int, whatever its operands
    if (auto* b = dynamic_cast<BinaryOperation*>(e)) {
        const std::string& o = b->op;
        if (o == "==" || o == "!=" || o == "<" || o == ">" || o == "<=" || o == ">=" || o == "&&" || o == "||")
            return "";
    }
    if (auto* u = dynamic_cast<UnaryOperation*>(e); u && u->op == "!") return "";
    // an operation on a wide integer is one (v + v + k: the inner sum, a
    // struct the type info calls a pointer, made the outer + pointer
    // arithmetic, #329); a shift by its left operand's
    if (auto* b = dynamic_cast<BinaryOperation*>(e)) {
        const std::string& o = b->op;
        if (o == "<<" || o == ">>") return wideKindOf(b->left.get());
        if (o == "+" || o == "-" || o == "*" || o == "/" || o == "%" || o == "&" || o == "|" || o == "^") {
            std::string l = wideKindOf(b->left.get());
            return l.empty() ? wideKindOf(b->right.get()) : l;
        }
    }
    if (auto* u = dynamic_cast<UnaryOperation*>(e); u && (u->op == "-" || u->op == "~"))
        return wideKindOf(u->operand.get());
    IRTypeInfo ti = getExprTypeInfo(e);
    if (ti.type == ir::Type::PTR && !isAggCopyParam(e)) return "";
    std::string n = getAggregateName(ti.typeName);
    if (n.rfind("struct ", 0) == 0) n = n.substr(7);
    if (n.size() > 5 && n.compare(0, 5, "__int") == 0 &&
        std::all_of(n.begin() + 5, n.end(), ::isdigit) && structs_.count(n))
        return n;
    return "";
}

// An integer as the wide integer type `kind` (no-op when it already is one):
// its bytes, then zeros or sign fill. braced: an initializer list (for a
// declaration's initializer), else a compound literal value
void IRBuilder::toWide(std::unique_ptr<Expression>& e, const std::string& kind, bool braced) {
    if (!e || !wideKindOf(e.get()).empty()) return;
    // c ? x : y: each branch converted (keeps 64-bit LL constants whole)
    if (auto* ce = dynamic_cast<ConditionalExpression*>(e.get()); ce && ce->thenExpr) {
        toWide(ce->thenExpr, kind, false);
        toWide(ce->elseExpr, kind, false);
        return;
    }
    IRTypeInfo ti = getExprTypeInfo(e.get());
    if (ti.type != ir::Type::I8 && ti.type != ir::Type::I16 && ti.type != ir::Type::I32) return;
    auto sit = structs_.find(kind);
    if (sit == structs_.end()) return;
    int n = sit->second.totalSize;
    int srcBytes = ir::typeSize(ti.type);
    bool sgn = ti.isSigned;
    int line = e->line, col = e->column; std::string file = e->sourceFile;
    auto list = std::make_unique<InitializerList>();
    double cre = 0, cim = 0;
    Expression* inner = e.get();
    while (auto* c = dynamic_cast<CastExpression*>(inner)) inner = c->expression.get();
    auto* lit = dynamic_cast<IntegerLiteral*>(inner);
    if (lit || (dynamic_cast<UnaryOperation*>(inner) && evalConstComplex(inner, cre, cim) && cim == 0)) {
        int64_t v = lit ? lit->value : (int64_t)cre;
        // a bare literal keeps its whole value (0x1ffffffffLL); a cast truncates it
        if (srcBytes < 8 && !(lit && inner == e.get() && (lit->castType.empty() || lit->castType == "long"))) {
            int64_t m = (int64_t)1 << (8 * srcBytes);
            v &= m - 1;
            if (sgn && (v & (m >> 1))) v -= m;
        }
        for (int k = 0; k < n; k++) list->elements.push_back(std::make_unique<IntegerLiteral>(k < 8 ? (v >> (8 * k)) & 0xFF : (v < 0 ? 0xFF : 0)));
    } else {
        // A run-time value: through a hidden local (evaluated once)
        std::string tmp = "__wide" + std::to_string(complexTemp_++);
        auto decl = std::make_unique<VariableDeclaration>(ti.type == ir::Type::I32 ? "long" : ti.type == ir::Type::I16 ? "int" : "char", tmp, 0);
        decl->isSigned = sgn;
        decl->initializer = std::move(e);
        decl->line = line; decl->column = col; decl->sourceFile = file;
        // (its value: an aggregate's operand may be built in address mode,
        // which took a variable's address: u128 -= c used the low byte of &c)
        bool oldAddrMode = computeAddressOnly_;
        computeAddressOnly_ = false;
        decl->accept(*this);
        computeAddressOnly_ = oldAddrMode;
        for (int k = 0; k < n; k++) {
            std::unique_ptr<Expression> b;
            if (k < srcBytes) {
                b = std::make_unique<BinaryOperation>(">>", std::make_unique<VariableReference>(tmp), std::make_unique<IntegerLiteral>(8 * k));
            } else if (sgn) {
                // 0xFF when negative: -(tmp < 0)
                b = std::make_unique<UnaryOperation>("-", std::make_unique<BinaryOperation>("<", std::make_unique<VariableReference>(tmp), std::make_unique<IntegerLiteral>(0)));
            } else {
                b = std::make_unique<IntegerLiteral>(0);
            }
            list->elements.push_back(std::make_unique<CastExpression>("char", 0, false, std::move(b)));
        }
    }
    list->line = line; list->column = col; list->sourceFile = file;
    if (braced) { e = std::move(list); return; }
    auto cl = std::make_unique<CompoundLiteral>("struct " + kind, 0, false, std::move(list));
    cl->line = line; cl->column = col; cl->sourceFile = file;
    e = std::move(cl);
}


// Does the body assign the variable `name` (=, op=, ++, --)? Conservative:
// an expression kind not looked into counts as an assignment.
bool IRBuilder::inlineParamAssigned(Statement* body, const std::string& name) {
    std::function<bool(Expression*)> ex = [&](Expression* e) -> bool {
        if (!e) return false;
        auto isName = [&](Expression* t) {
            auto* v = dynamic_cast<VariableReference*>(t);
            return v && v->name == name;
        };
        if (auto* a = dynamic_cast<Assignment*>(e)) return isName(a->target.get()) || ex(a->target.get()) || ex(a->expression.get());
        if (auto* u = dynamic_cast<UnaryOperation*>(e)) {
            if ((u->op == "++" || u->op == "--" || u->op == "++_POST" || u->op == "--_POST") && isName(u->operand.get())) return true;
            return ex(u->operand.get());
        }
        if (auto* b = dynamic_cast<BinaryOperation*>(e)) return ex(b->left.get()) || ex(b->right.get());
        if (auto* c = dynamic_cast<ConditionalExpression*>(e)) return ex(c->condition.get()) || ex(c->thenExpr.get()) || ex(c->elseExpr.get());
        if (auto* c = dynamic_cast<CastExpression*>(e)) return ex(c->expression.get());
        if (auto* a = dynamic_cast<ArrayAccess*>(e)) return ex(a->arrayExpr.get()) || ex(a->indexExpr.get());
        if (auto* m = dynamic_cast<MemberAccess*>(e)) return ex(m->structExpr.get());
        if (auto* f = dynamic_cast<FunctionCall*>(e)) {
            for (auto& a : f->arguments) if (ex(a.get())) return true;
            return ex(f->callExpr.get());
        }
        if (dynamic_cast<StatementExpression*>(e) || dynamic_cast<BuiltinVaArg*>(e) || dynamic_cast<BuiltinVaStart*>(e))
            return true;
        return false;
    };
    std::function<bool(Statement*)> st = [&](Statement* s) -> bool {
        if (!s) return false;
        if (auto* es = dynamic_cast<ExpressionStatement*>(s)) return ex(es->expression.get());
        if (auto* r = dynamic_cast<ReturnStatement*>(s)) return ex(r->expression.get());
        if (auto* d = dynamic_cast<VariableDeclaration*>(s)) return d->name == name || ex(d->initializer.get());
        if (auto* c = dynamic_cast<CompoundStatement*>(s)) { for (auto& x : c->statements) if (st(x.get())) return true; return false; }
        if (auto* i = dynamic_cast<IfStatement*>(s)) return ex(i->condition.get()) || st(i->thenBranch.get()) || st(i->elseBranch.get());
        if (auto* w = dynamic_cast<WhileStatement*>(s)) return ex(w->condition.get()) || st(w->body.get());
        if (auto* w = dynamic_cast<DoWhileStatement*>(s)) return ex(w->condition.get()) || st(w->body.get());
        if (auto* f = dynamic_cast<ForStatement*>(s)) return st(f->initializer.get()) || ex(f->condition.get()) || ex(f->increment.get()) || st(f->body.get());
        if (auto* w = dynamic_cast<SwitchStatement*>(s)) return ex(w->expression.get()) || st(w->body.get());
        if (auto* r = dynamic_cast<RepeatStatement*>(s)) return st(r->body.get());
        if (auto* c = dynamic_cast<CaseStatement*>(s)) { (void)c; return false; }
        if (auto* l = dynamic_cast<LabelledStatement*>(s)) return st(l->statement.get());
        return false;
    };
    return st(body);
}

// Does the function being built return a struct/union (by value)?
bool IRBuilder::currentFuncReturnsStruct() {
    if (!currentFunc_) return false;
    auto it = allFunctions_.find(currentFunc_->name.substr(1));
    return it != allFunctions_.end() && it->second->returnPointerLevel == 0 && structOf(it->second->returnType);
}

// A wide integer converted to an integer type t: its low bytes (as an
// explicit cast, which visit(CastExpression) lowers)
void IRBuilder::narrowWide(std::unique_ptr<Expression>& e, ir::Type t) {
    if (!e || (t != ir::Type::I8 && t != ir::Type::I16 && t != ir::Type::I32)) return;
    if (wideKindOf(e.get()).empty()) return;
    const char* tn = t == ir::Type::I8 ? "char" : t == ir::Type::I32 ? "long" : "int";
    int line = e->line, col = e->column; std::string file = e->sourceFile;
    e = std::make_unique<CastExpression>(tn, 0, false, std::move(e));
    e->line = line; e->column = col; e->sourceFile = file;
}

void IRBuilder::toComplex(std::unique_ptr<Expression>& e, const std::string& kind) {
    std::string k = complexKindOf(e.get());
    auto* cast = dynamic_cast<CastExpression*>(e.get());
    if (cast && !k.empty()) {
        // (T _Complex)x: x converted to that complex type
        std::unique_ptr<Expression> inner = std::move(cast->expression);
        toComplex(inner, k);
        e = std::move(inner);
    }
    if (k == kind) return;
    bool isFloat = kind == "_Complex_float";
    auto list = std::make_unique<InitializerList>();
    int line = e->line, col = e->column; std::string file = e->sourceFile;
    if (k.empty()) {
        // real value x -> x + 0i
        list->elements.push_back(std::move(e));
        if (isFloat) list->elements.push_back(std::make_unique<FloatLiteral>(0.0));
        else list->elements.push_back(std::make_unique<IntegerLiteral>(0));
    } else {
        // the other complex type: convert both parts
        auto copy = cloneSimpleExpr(e.get());
        double cre, cim;
        if (!copy && evalConstComplex(e.get(), cre, cim)) {
            if (isFloat) {
                list->elements.push_back(std::make_unique<FloatLiteral>(cre));
                list->elements.push_back(std::make_unique<FloatLiteral>(cim));
            } else {
                list->elements.push_back(std::make_unique<IntegerLiteral>((int64_t)cre));
                list->elements.push_back(std::make_unique<IntegerLiteral>((int64_t)cim));
            }
            auto cl = std::make_unique<CompoundLiteral>("struct " + kind, 0, !isFloat, std::move(list));
            cl->line = line; cl->column = col; cl->sourceFile = file;
            e = std::move(cl);
            return;
        }
        if (!copy && currentFunc_) {
            // Evaluate it once into a hidden local, then convert that
            std::string tmp = "__cplx" + std::to_string(complexTemp_++);
            auto decl = std::make_unique<VariableDeclaration>("struct " + k, tmp, 0);
            decl->initializer = std::move(e);
            decl->line = line; decl->column = col; decl->sourceFile = file;
            decl->accept(*this);
            e = std::make_unique<VariableReference>(tmp);
            copy = std::make_unique<VariableReference>(tmp);
        }
        if (!copy) return;  // left for the operator dispatch to reject
        list->elements.push_back(std::make_unique<MemberAccess>(std::move(e), "real", false));
        list->elements.push_back(std::make_unique<MemberAccess>(std::move(copy), "imag", false));
    }
    auto cl = std::make_unique<CompoundLiteral>("struct " + kind, 0, !isFloat, std::move(list));
    cl->line = line; cl->column = col; cl->sourceFile = file;
    e = std::move(cl);
}

// Constant complex value (real or imaginary literals, casts, + - * / ~)
bool IRBuilder::evalConstComplex(Expression* e, double& re, double& im) {
    if (auto* c = dynamic_cast<CastExpression*>(e)) {
        if (!evalConstComplex(c->expression.get(), re, im)) return false;
        if (complexKindOf(c).empty()) {
            if (c->pointerLevel) return false;
            im = 0;
            if (mapType(c->targetType, 0) != ir::Type::F32) re = (double)(int64_t)re;
        }
        return true;
    }
    if (auto* cl = dynamic_cast<CompoundLiteral*>(e)) {
        if (complexKindOf(cl).empty() || !cl->initializer || cl->initializer->elements.empty() ||
            cl->initializer->elements.size() > 2) return false;
        double a, b, c2 = 0, d = 0;
        if (!evalConstComplex(cl->initializer->elements[0].get(), a, b)) return false;
        if (cl->initializer->elements.size() == 2 &&
            !evalConstComplex(cl->initializer->elements[1].get(), c2, d)) return false;
        re = a; im = c2;
        return true;
    }
    if (auto* u = dynamic_cast<UnaryOperation*>(e)) {
        if (!evalConstComplex(u->operand.get(), re, im)) return false;
        if (u->op == "-") { re = -re; im = -im; return true; }
        if (u->op == "+") return true;
        if (u->op == "~") { im = -im; return true; }
        return false;
    }
    if (auto* b = dynamic_cast<BinaryOperation*>(e)) {
        double a, bi, c, d;
        if (!evalConstComplex(b->left.get(), a, bi) || !evalConstComplex(b->right.get(), c, d)) return false;
        if (b->op == "+") { re = a + c; im = bi + d; return true; }
        if (b->op == "-") { re = a - c; im = bi - d; return true; }
        if (b->op == "*") { re = a * c - bi * d; im = a * d + bi * c; return true; }
        if (b->op == "/") {
            double den = c * c + d * d;
            if (den == 0) return false;
            re = (a * c + bi * d) / den; im = (bi * c - a * d) / den;
            return true;
        }
        return false;
    }
    double v;
    if (auto* i = dynamic_cast<IntegerLiteral*>(e)) { re = (double)i->value; im = 0; return true; }
    if (evalConstFloat(e, v)) { re = v; im = 0; return true; }
    return false;
}

// The size a type, or an expression's type, has at compile time (the
// sizeof visitor's, and constSizeof's when the size is not a run-time one)
int64_t IRBuilder::staticSizeof(SizeofExpression& node) {
    if (node.isType) {
        int64_t sz = getTypeSize(node.typeName, node.pointerLevel);
        if (node.pointerLevel == 0)
            for (int d : node.arrayDims) if (d > 0) sz *= d;
        for (int d : node.declDims) if (d > 0) sz *= d;
        return sz;
    }
    return node.expression ? getExprTypeInfo(node.expression.get()).totalSize : 2;
}

bool IRBuilder::constSizeof(SizeofExpression& node, int64_t& out) {
    if (node.isType) {
        std::string tn = node.pointerLevel == 0 && node.arrayDims.empty() ? getAggregateName(node.typeName) : "";
        if (tn.rfind("struct ", 0) == 0) tn = tn.substr(7);
        else if (tn.rfind("union ", 0) == 0) tn = tn.substr(6);
        if (vmStructs_.count(tn)) return false;
        out = staticSizeof(node);
        return out > 0;
    }
    Expression* x = node.expression.get();
    if (!x) return false;
    // (a variable-length array, a struct with run-time member sizes, a
    // parameter with run-time dimensions: known only at run time)
    Expression* base = x;
    while (true) {
        if (auto* aa = dynamic_cast<ArrayAccess*>(base)) { base = aa->arrayExpr.get(); continue; }
        if (auto* u = dynamic_cast<UnaryOperation*>(base); u && u->op == "*") { base = u->operand.get(); continue; }
        break;
    }
    if (auto* vr = dynamic_cast<VariableReference*>(base))
        if (vlaSizeVars_.count(vr->name) || vmVars_.count(vr->name) || paramVlaDims_.count(vr->name)) return false;
    if (!vmStructOf(x).empty()) return false;
    if (auto* u = dynamic_cast<UnaryOperation*>(x); u && u->op == "*")
        if (int row = ptrArrayRowSize(u->operand.get())) { out = row; return true; }
    out = staticSizeof(node);
    return out > 0;
}

bool IRBuilder::evalConstFloat(Expression* e, double& out) {
    ConstValue v;
    if (!ConstEval::eval(e, *this, v)) return false;
    out = v.asDouble();
    return true;
}

bool IRBuilder::evalConstInt(Expression* e, int64_t& out) {
    ConstValue v;
    if (!ConstEval::eval(e, *this, v)) return false;
    out = v.isFloat() ? (int64_t)std::trunc(v.fvalue) : v.intValue();
    return true;
}

ir::Operand IRBuilder::emitAddressOf(Expression* expr) {
    bool saved = computeAddressOnly_;
    computeAddressOnly_ = true;
    expr->accept(*this);
    computeAddressOnly_ = saved;
    return lastValue_;
}

// The struct with run-time member sizes an expression's value has, or ""
std::string IRBuilder::vmStructOf(Expression* e) {
    if (vmStructs_.empty() || !e) return "";
    // ({ ...; v; }) and a = b: the type of v / a
    while (true) {
        if (auto* se = dynamic_cast<StatementExpression*>(e); se && se->result) { e = se->result.get(); continue; }
        if (auto* as = dynamic_cast<Assignment*>(e)) { e = as->target.get(); continue; }
        break;
    }
    if (auto* vr = dynamic_cast<VariableReference*>(e)) {
        auto it = vmVars_.find(vr->name);
        if (it != vmVars_.end()) return it->second;
    }
    if (auto* va = dynamic_cast<BuiltinVaArg*>(e)) {
        if (va->pointerLevel) return "";
        std::string n = getAggregateName(va->typeName);
        return vmStructs_.count(n) ? n : "";
    }
    IRTypeInfo ti = getExprTypeInfo(e);
    if (ti.type == ir::Type::PTR) return "";
    std::string n = getAggregateName(ti.typeName);
    if (n.rfind("struct ", 0) == 0) n = n.substr(7);
    else if (n.rfind("union ", 0) == 0) n = n.substr(6);
    return vmStructs_.count(n) ? n : "";
}

ir::Operand IRBuilder::emitPtrOffset(const ir::Operand& base, int offset, const ir::SourceLoc& sl) {
    if (offset == 0) return base;
    auto p = allocVreg(ir::Type::PTR);
    ir::Inst add;
    add.op = ir::Op::ADD;
    add.dest = p;
    add.resultType = ir::Type::PTR;
    add.src1 = base;
    add.src2 = ir::Operand::imm(offset, ir::Type::I16);
    add.loc = sl;
    emit(add);
    return p;
}

void IRBuilder::emitBlockCopy(const ir::Operand& dst, const ir::Operand& src, int size, const ir::SourceLoc& sl) {
    // A struct with run-time member sizes: its size from its hidden local
    if (vmCopySize_.kind != ir::OperandKind::NONE) {
        ir::Inst call;
        call.op = ir::Op::CALL_VOID;
        call.resultType = ir::Type::VOID;
        call.src1 = ir::Operand::global("_memcpy");
        call.args = {dst, src, vmCopySize_};
        call.callConv = ir::CallConv::ZP;   // the library's memcpy is ZP asm (both libraries)
        call.loc = sl;
        emit(call);
        calledFunctions_.insert("_memcpy");
        return;
    }
    if (size <= 0) return;
    if (size <= 4) {
        for (int off = 0; off < size;) {
            int chunk = (size - off >= 2) ? 2 : 1;
            ir::Type t = (chunk == 2) ? ir::Type::I16 : ir::Type::I8;
            auto v = allocVreg(t);
            ir::Inst ld;
            ld.op = ir::Op::LOAD;
            ld.dest = v;
            ld.resultType = t;
            ld.src1 = emitPtrOffset(src, off, sl);
            ld.loc = sl;
            emit(ld);
            ir::Inst st;
            st.op = ir::Op::STORE;
            st.resultType = t;
            st.src1 = v;
            st.src2 = emitPtrOffset(dst, off, sl);
            st.loc = sl;
            emit(st);
            off += chunk;
        }
        return;
    }
    ir::Inst call;
    call.op = ir::Op::CALL_VOID;
    call.resultType = ir::Type::VOID;
    call.src1 = ir::Operand::global("_memcpy");
    call.args = {dst, src, ir::Operand::imm(size, ir::Type::I16)};
    call.callConv = ir::CallConv::ZP;   // the library's memcpy is ZP asm (both libraries)
    call.loc = sl;
    emit(call);
    calledFunctions_.insert("_memcpy");
}

// ZP calling convention: parameters go in the 32-byte block at $10 (each slot at
// least 2 bytes). A variadic function, or one whose parameters don't fit, uses
// the stack convention instead; callers apply the same rule to the prototype.
int IRBuilder::zpParamBytes(const std::vector<ir::Type>& types) {
    int n = 0;
    for (auto t : types) n += std::max(2, (int)ir::typeSize(t));
    return n;
}

bool IRBuilder::usesZpConv(const std::string& fname) {
    if (variadicFunctions_.count(fname)) return false;
    if (!zpCallMode && !fastcallFunctions_.count(fname) &&
        !(isLibraryZpRoutine(fname) && !unitFunctionNames_.count(fname))) return false;
    auto it = functionParamTypes_.find(fname);
    return it == functionParamTypes_.end() || zpParamBytes(it->second) <= ZP_PARAM_BYTES;
}

// The library's hand-written assembly routines (lib/stdlib_asm/*.s45): they use
// the ZP convention in both c45.lib and c45_stack.lib. Their headers declare
// them __fastcall__; a call without any declaration (an implicit declaration,
// or __builtin_alloca/__builtin_setjmp/__builtin_abs..., which name the
// library function) calls them with the ZP convention as well, so a
// stack-convention program needs no bridge thunk.
// src/test/test_libcall_conv.sh checks this list against the library.
bool IRBuilder::isLibraryZpRoutine(const std::string& fname) {
    static const std::set<std::string> routines = {
        "abs", "alloca", "assert_fail_abort", "atoi", "calloc", "cbm_drive_status", "exit", "free",
        "getchar", "isalnum", "isalpha", "isblank", "iscntrl", "isdigit", "isgraph", "islower",
        "isprint", "ispunct", "isspace", "isupper", "isxdigit", "itoa", "labs", "longjmp", "ltoa",
        "malloc", "memcmp", "memcpy", "memmove", "memset", "putchar", "puts", "realloc",
        "setjmp", "strcat", "strchr", "strcmp", "strcpy", "strlen", "strncat", "strncmp",
        "strncpy", "strrchr", "tolower", "toupper", "utoa", "putstr",
        // conio (stdlib_asm/conio.s45)
        "clrscr", "cputch", "cputs", "cputsxy", "cursoroff", "cursoron", "getattr_xy", "getch",
        "getch_xy", "gotoxy", "kbhit", "putch", "revers", "screenheight", "screenwidth",
        "textattr", "textbackground", "textcolor", "wherex", "wherey",
        // clock.s45, rand.s45, strtok.s45, strtoul.s45
        "clock", "rand", "srand", "strtok", "strtok_r", "strtol", "strtoul"};
    return routines.count(fname) > 0;
}

bool IRBuilder::usesZpConvSig(const FuncPtrSignature& sig) {
    if (!zpCallMode || sig.isVariadic) return false;   // a variadic function takes the stack convention
    std::vector<ir::Type> types;
    if (sig.returnPointerLevel == 0 && (mapType(sig.returnType, 0) == ir::Type::F32 ||
                                        structs_.count(getAggregateName(sig.returnType))))
        types.push_back(ir::Type::PTR);  // hidden result pointer
    for (const auto& p : sig.params)
        types.push_back(aggregateBlockSize(p.type, p.pointerLevel) ? ir::Type::PTR : mapType(p.type, p.pointerLevel));
    return zpParamBytes(types) <= ZP_PARAM_BYTES;
}

// Evaluate a call argument. A by-copy aggregate parameter (aggSize > 0) gets the
// address of a fresh copy of the argument.
ir::Operand IRBuilder::evalArgument(Expression* arg, int aggSize, const ir::SourceLoc& sl) {
    // A struct with run-time member sizes goes by its own address (va_arg
    // copies it)
    if (!vmStructOf(arg).empty()) return emitAddressOf(arg);
    if (aggSize > 0 && currentFunc_) {
        auto buf = allocVreg(ir::Type::I16);
        currentFunc_->vregSizes[buf.vregId] = aggSize;
        currentFunc_->memoryVregs.insert(buf.vregId);
        auto addr = allocVreg(ir::Type::PTR);
        ir::Inst a;
        a.op = ir::Op::ADDR_LOCAL;
        a.dest = addr;
        a.resultType = ir::Type::PTR;
        a.src1 = ir::Operand::vreg(buf.vregId, ir::Type::PTR);
        a.loc = sl;
        emit(a);
        emitAggregateInto(arg, addr, aggSize, sl);
        return addr;
    }
    // an argument is a value, also in a call whose struct result is wanted
    // as an address (a struct-valued ?:'s arm)
    bool saved = computeAddressOnly_;
    computeAddressOnly_ = false;
    arg->accept(*this);
    computeAddressOnly_ = saved;
    return lastValue_;
}

int IRBuilder::paramAggSize(const std::string& fname, size_t index) {
    auto it = paramAggSizes_.find(fname);
    if (it == paramAggSizes_.end() || index >= it->second.size()) return 0;
    return it->second[index];
}

// Hidden result pointer for a call to a struct/union/float-returning function:
// the caller-supplied destination (sretDest_) or a fresh temporary buffer (buf).
ir::Operand IRBuilder::sretCallBuffer(const std::string& fname, ir::Operand& buf, const ir::SourceLoc& sl) {
    if (sretDestSet_) {
        sretDestSet_ = false;
        buf = ir::Operand();
        return sretDest_;
    }
    if (!currentFunc_) return ir::Operand();
    buf = allocVreg(sretFloat_.count(fname) ? ir::Type::F32 : ir::Type::I16);
    currentFunc_->vregSizes[buf.vregId] = sretSize_[fname];
    currentFunc_->memoryVregs.insert(buf.vregId);
    ir::Operand addr = allocVreg(ir::Type::PTR);
    ir::Inst a;
    a.op = ir::Op::ADDR_LOCAL;
    a.dest = addr;
    a.resultType = ir::Type::PTR;
    a.src1 = ir::Operand::vreg(buf.vregId, ir::Type::PTR);
    a.loc = sl;
    emit(a);
    return addr;
}

// Value of an sret call (after the CALL_VOID): a float or 1/2/4-byte aggregate is
// loaded from the buffer; larger aggregates are the buffer (or destination) itself.
void IRBuilder::sretCallResult(const std::string& fname, const ir::Operand& buf, const ir::Operand& addr,
                               const ir::SourceLoc& sl) {
    int size = sretSize_[fname];
    ir::Type t = ir::Type::VOID;
    if (sretFloat_.count(fname)) t = ir::Type::F32;
    else if (buf.kind == ir::OperandKind::VREG)
        t = size == 1 ? ir::Type::I8 : size == 2 ? ir::Type::I16 : size == 4 ? ir::Type::I32 : ir::Type::VOID;
    if (t != ir::Type::VOID) {
        auto v = allocVreg(t);
        ir::Inst ld;
        ld.op = ir::Op::LOAD;
        ld.dest = v;
        ld.resultType = t;
        ld.src1 = addr;
        ld.loc = sl;
        emit(ld);
        lastValue_ = v;
    } else {
        // Larger aggregates: the buffer, or its address when an address is wanted
        lastValue_ = (buf.kind == ir::OperandKind::VREG && !computeAddressOnly_) ? buf : addr;
    }
    lastValueSigned_ = false;
}

// Call to an operator method built by hand (this, rhs...): handles the hidden result pointer
// sretBuf/sretAddr: the hidden result buffer when already claimed (sretCallBuffer)
// before the arguments were evaluated.
ir::Operand IRBuilder::addressOfValue(const ir::Operand& v, const ir::SourceLoc& sl) {
    if (v.type == ir::Type::PTR || v.type == ir::Type::VOID || !currentFunc_) return v;
    auto slot = allocVreg(v.type);
    currentFunc_->localSlotVregs.insert(slot.vregId);
    currentFunc_->memoryVregs.insert(slot.vregId);
    usedVregs_.insert(slot.vregId);
    ir::Inst st;
    st.op = ir::Op::STORE;
    st.resultType = v.type;
    st.src1 = v;
    st.src2 = slot;
    st.loc = sl;
    emit(st);
    auto addr = allocVreg(ir::Type::PTR);
    ir::Inst la;
    la.op = ir::Op::ADDR_LOCAL;
    la.dest = addr;
    la.resultType = ir::Type::PTR;
    la.src1 = slot;
    la.loc = sl;
    emit(la);
    return addr;
}

void IRBuilder::emitMethodCall(const std::string& mangledName, std::vector<ir::Operand> args, const ir::SourceLoc& sl,
                               const ir::Operand* sretBuf, const ir::Operand* sretAddr) {
    ir::Inst call;
    call.src1 = ir::Operand::global("_" + mangledName);
    call.callConv = usesZpConv(mangledName) ? ir::CallConv::ZP : ir::CallConv::STACK;
    call.loc = sl;
    if (sretSize_.count(mangledName)) {
        ir::Operand buf, addr;
        if (sretAddr) { buf = *sretBuf; addr = *sretAddr; }
        else addr = sretCallBuffer(mangledName, buf, sl);
        args.insert(args.begin(), addr);
        call.op = ir::Op::CALL_VOID;
        call.resultType = ir::Type::VOID;
        call.args = args;
        emit(call);
        sretCallResult(mangledName, buf, addr, sl);
        return;
    }
    auto retType = functionReturnTypes_[mangledName];
    auto dest = allocVreg(retType);
    call.op = (retType == ir::Type::VOID) ? ir::Op::CALL_VOID : ir::Op::CALL;
    call.dest = dest;
    call.resultType = retType;
    call.args = args;
    emit(call);
    lastValue_ = dest;
    lastValueSigned_ = functionReturnSigned_[mangledName];
}

void IRBuilder::emitAggregateInto(Expression* expr, const ir::Operand& dst, int size, const ir::SourceLoc& sl,
                                  bool dstMayAlias) {
    // A struct with run-time member sizes: copied with its run-time size
    if (vmCopySize_.kind == ir::OperandKind::NONE) {
        std::string vn = vmStructOf(expr);
        if (!vn.empty()) {
            vmCopySize_ = locals_[vmStructs_[vn].sizeVar];
            emitAggregateInto(expr, dst, size, sl, dstMayAlias);
            vmCopySize_ = ir::Operand::none();
            return;
        }
    }
    if (auto* call = dynamic_cast<FunctionCall*>(expr)) {
        // An aggregate-returning call (function, method or function pointer)
        // writes its result straight into dst through the hidden pointer —
        // unless dst may be memory the callee reads (*q = f() where f reads
        // an overlapping *p): then into its own buffer, copied afterwards
        sretDest_ = dst;
        sretDestSet_ = !dstMayAlias;
        bool saved = computeAddressOnly_;
        computeAddressOnly_ = false;
        call->accept(*this);
        computeAddressOnly_ = saved;
        bool consumed = !dstMayAlias && !sretDestSet_;
        sretDestSet_ = false;
        if (consumed) return;
        // The result's address (an sret call leaves its buffer vreg)
        ir::Operand src = lastValue_;
        if (src.isVreg() && src.type != ir::Type::PTR && currentFunc_ && currentFunc_->memoryVregs.count(src.vregId)) {
            auto a = allocVreg(ir::Type::PTR);
            ir::Inst al;
            al.op = ir::Op::ADDR_LOCAL;
            al.dest = a;
            al.resultType = ir::Type::PTR;
            al.src1 = ir::Operand::vreg(src.vregId, ir::Type::PTR);
            al.loc = sl;
            emit(al);
            src = a;
        }
        emitBlockCopy(dst, src, size, sl);
        return;
    }
    auto src = emitAddressOf(expr);
    emitBlockCopy(dst, src, size, sl);
}

void IRBuilder::resetFunctionLocalState() {
    aggCopyParams_.clear();
    vmVars_.clear();
    vmStructs_.clear();
    locals_.clear();
    localTypes_.clear();
    localTypeNames_.clear();
    localSigned_.clear();
    localPlainUnsigned_.clear();
    localConst_.clear();
    localPointsToConst_.clear();
    localRegister_.clear();
    localPointedToType_.clear();
    localArrayDims_.clear();
    localPtrLevel_.clear();
    localPointedToTypeName_.clear();
    uplevelNames_.clear();
    paramArrayDims_.clear();
    // Global pointers to arrays are indexed like int a[][M] parameters
    for (const auto& [n, d] : globalPtrArrayDims_) paramArrayDims_[n] = d;
    paramVlaDims_.clear();
    vlaSizeVars_.clear();
    usedVregs_.clear();
    setVregs_.clear();
    localDeclLocs_.clear();
}

// Does a block declare a variable-length array directly (or in a
// multi-declarator group)? Its stack space is released when the block ends.
static bool declaresVla(CompoundStatement& node) {
    if (node.isDeclGroup) return false;  // its block does it
    for (auto& st : node.statements) {
        if (auto* vd = dynamic_cast<VariableDeclaration*>(st.get()); vd && !vd->vlaDimExprs.empty() && !vd->isStatic)
            return true;
        if (auto* grp = dynamic_cast<CompoundStatement*>(st.get())) {
            for (auto& g : grp->statements)
                if (auto* vd = dynamic_cast<VariableDeclaration*>(g.get()); vd && !vd->vlaDimExprs.empty() && !vd->isStatic)
                    return true;
        }
    }
    return false;
}

// The names declared directly by decls (a declaration, a group of them, or
// with list a block's statements) that already name a variable: their
// bindings are saved, to be restored when the scope ends
std::vector<std::function<void()>> IRBuilder::saveShadowed(Statement* decls, bool list) {
    std::vector<std::function<void()>> r;
    if (!currentFunc_ || !decls) return r;
    std::vector<std::string> names;
    std::function<void(Statement*, bool)> collect = [&](Statement* st, bool top) {
        if (auto* vd = dynamic_cast<VariableDeclaration*>(st)) names.push_back(vd->name);
        else if (auto* cs = dynamic_cast<CompoundStatement*>(st); cs && (top || cs->isDeclGroup))
            for (auto& x : cs->statements) collect(x.get(), false);
    };
    collect(decls, list);
    for (const auto& n : names) {
        // not shadowing (a global's name is: after the block it is the global again)
        if (!locals_.count(n) && !uplevelNames_.count(n) && !globalTypes_.count(n)) continue;
        auto save = [&](auto& m) {
            auto it = m.find(n);
            if (it != m.end()) { auto v = it->second; r.push_back([&m, n, v]() { m[n] = v; }); }
            else r.push_back([&m, n]() { m.erase(n); });
        };
        save(locals_); save(localTypes_); save(localTypeNames_); save(localSigned_);
        save(localPlainUnsigned_); save(localConst_); save(localPointsToConst_); save(localRegister_);
        save(localPointedToType_); save(localPointedToTypeName_); save(localConstPtrValue_);
        save(localArrayDims_); save(localPtrLevel_); save(paramArrayDims_); save(paramVlaDims_);
        save(vlaSizeVars_); save(localDeclLocs_); save(vmVars_);
        bool up = uplevelNames_.count(n) > 0;
        r.push_back([this, n, up]() { if (up) uplevelNames_.insert(n); else uplevelNames_.erase(n); });
        bool agg = aggCopyParams_.count(n) > 0;
        r.push_back([this, n, agg]() { if (agg) aggCopyParams_.insert(n); else aggCopyParams_.erase(n); });
    }
    return r;
}

void IRBuilder::restoreShadowed(std::vector<std::function<void()>>& r) {
    for (auto it = r.rbegin(); it != r.rend(); ++it) (*it)();
    r.clear();
}

void IRBuilder::visit(CompoundStatement& node) {
    // (a declaration group is not a scope: its names are the enclosing block's)
    auto shadowed = node.isDeclGroup ? std::vector<std::function<void()>>() : saveShadowed(&node, true);
    bool vla = currentFunc_ && declaresVla(node);
    ir::Operand spSlot;
    if (vla) {
        // Save the stack pointer: the arrays' space is released at the end
        spSlot = allocVreg(ir::Type::I16);
        currentFunc_->localSlotVregs.insert(spSlot.vregId);
        auto sp = allocVreg(ir::Type::I16);
        ir::Inst rd;
        rd.op = ir::Op::CPU_REG_READ; rd.dest = sp; rd.resultType = ir::Type::I16; rd.asmText = "SP";
        emit(rd);
        ir::Inst st;
        st.op = ir::Op::STORE; st.resultType = ir::Type::I16; st.src1 = sp; st.src2 = spSlot;
        emit(st);
        vlaBlocks_.push_back({spSlot, loopStack_.size()});
    }
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
    if (vla) {
        restoreVlaStack(spSlot);
        vlaBlocks_.pop_back();
    }
    restoreShadowed(shadowed);
}

// SP = the value saved when a block with variable-length arrays began
void IRBuilder::restoreVlaStack(const ir::Operand& spSlot) {
    ir::Inst wr;
    wr.op = ir::Op::CPU_REG_WRITE; wr.resultType = ir::Type::I16; wr.asmText = "SP"; wr.src1 = spSlot;
    emit(wr);
}

// break/continue leave the blocks opened inside the current loop: release
// their variable-length arrays (from the outermost of them)
// A jump out of the loops/switches deeper than `depth`: release the VLAs
// declared in them (the outermost one's saved stack pointer)
void IRBuilder::releaseVlasForJump(size_t depth) {
    for (const auto& vb : vlaBlocks_)
        if (vb.loopDepth > depth) { restoreVlaStack(vb.spSlot); return; }
}

void IRBuilder::visit(StatementExpression& node) {
    bool savedAddrOnly = computeAddressOnly_;
    computeAddressOnly_ = false;
    // one scope for the statements and the value ({ int a = 7; a; })
    auto shadowed = saveShadowed(node.body.get(), true);
    if (node.body)
        for (auto& st : node.body->statements) st->accept(*this);
    computeAddressOnly_ = savedAddrOnly;
    if (node.result) node.result->accept(*this);
    else lastValue_ = ir::Operand::imm(0, ir::Type::I16);
    restoreShadowed(shadowed);
}

// ============================================================================
// Variables
// ============================================================================

void IRBuilder::visit(VariableDeclaration& node) {
    if (node.isVolatile) volatileNames_.insert(node.name);
    // extern in a block: the file-scope variable (a local of the name hidden
    // to the block's end, which restores it: saveShadowed), declared as at
    // file scope
    if (currentFunc_ && node.isExtern) {
        const std::string& n = node.name;
        locals_.erase(n); localTypes_.erase(n); localTypeNames_.erase(n); localSigned_.erase(n);
        localPlainUnsigned_.erase(n); localConst_.erase(n); localPointsToConst_.erase(n); localRegister_.erase(n);
        localPointedToType_.erase(n); localPointedToTypeName_.erase(n); localConstPtrValue_.erase(n);
        localArrayDims_.erase(n); localPtrLevel_.erase(n); paramArrayDims_.erase(n); paramVlaDims_.erase(n);
        vlaSizeVars_.erase(n); localDeclLocs_.erase(n); vmVars_.erase(n);
        uplevelNames_.erase(n); aggCopyParams_.erase(n);
        if (globalTypes_.count(n)) return;      // (declared at file scope already)
        ir::Function* f = currentFunc_;
        currentFunc_ = nullptr;
        visit(node);
        currentFunc_ = f;
        return;
    }
    // A nested function's own variable hides the enclosing function's one:
    // its types are not the enclosing variable's
    if (currentFunc_ && uplevelNames_.erase(node.name)) {
        localTypes_.erase(node.name);
        localTypeNames_.erase(node.name);
        localSigned_.erase(node.name);
        localPointedToType_.erase(node.name);
        localArrayDims_.erase(node.name);
        localPointedToTypeName_.erase(node.name);
        localPtrLevel_.erase(node.name);
    }
    if (node.isFunctionPointer && node.funcPtrSig) fnPtrSigs_[node.name] = node.funcPtrSig;
    // a run-time array size needs automatic storage
    if ((!currentFunc_ || node.isStatic) && !node.isExtern) {
        for (const auto& d : node.vlaDimExprs)
            if (d) {
                errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column, Severity::Error,
                    "array size of '" + node.name + "' is not a constant expression"));
                break;
            }
    }
    // An integer variable initialized from a wide integer: its low bytes
    if (node.pointerLevel == 0 && node.arrayDims.empty() && node.initializer &&
        !dynamic_cast<InitializerList*>(node.initializer.get()) && !structOf(node.type))
        narrowWide(node.initializer, mapType(node.type, 0));
    // A global or static aggregate initialized with (T){...} of its own type:
    // the literal's brace list
    if ((!currentFunc_ || node.isStatic) && node.pointerLevel == 0 && node.initializer) {
        Expression* ie = node.initializer.get();
        while (auto* ce = dynamic_cast<CastExpression*>(ie)) ie = ce->expression.get();
        if (auto* cl = dynamic_cast<CompoundLiteral*>(ie); cl && cl->initializer && cl->pointerLevel == 0 &&
            cl->arrayDims.empty() == node.arrayDims.empty() && getAggregateName(cl->targetType) == getAggregateName(node.type)) {
            std::unique_ptr<InitializerList> list = std::move(cl->initializer);
            node.initializer = std::move(list);
        }
    }
    // A variable of a struct with run-time member sizes: storage of the
    // run-time size on the stack (alloca); the variable is its address
    if (currentFunc_ && !node.isStatic && !node.isExtern && node.pointerLevel == 0 && node.arrayDims.empty()) {
        std::string sn = getAggregateName(node.type);
        if (sn.rfind("struct ", 0) == 0) sn = sn.substr(7);
        else if (sn.rfind("union ", 0) == 0) sn = sn.substr(6);
        auto vsit = vmStructs_.find(sn);
        if (vsit != vmStructs_.end()) {
            currentFunc_->originalIsLeaf = false;
            currentFunc_->originalCallees.insert("_alloca");
            if (!functionReturnTypes_.count("alloca")) {
                auto proto = std::make_unique<FunctionDeclaration>("alloca", "void");
                proto->returnPointerLevel = 1;
                proto->isPrototype = true;
                proto->isFastcall = true;   // the library's alloca is ZP asm
                Parameter prm;
                prm.type = "int";
                prm.pointerLevel = 0;
                prm.name = "size";
                proto->parameters.push_back(prm);
                auto* savedFunc = currentFunc_;
                auto savedBlock = currentBlock_;
                proto->accept(*this);
                currentFunc_ = savedFunc;
                currentBlock_ = savedBlock;
                vlaAllocaProtos_.push_back(std::move(proto));
            }
            auto call = std::make_unique<FunctionCall>("alloca");
            call->arguments.push_back(std::make_unique<VariableReference>(vsit->second.sizeVar));
            call->line = node.line; call->column = node.column; call->sourceFile = node.sourceFile;
            auto pd = std::make_unique<VariableDeclaration>(node.type, node.name, 1);
            pd->initializer = std::make_unique<CastExpression>(node.type, 1, false, std::move(call));
            pd->line = node.line; pd->column = node.column; pd->sourceFile = node.sourceFile;
            pd->accept(*this);
            vmVars_[node.name] = sn;
            if (node.initializer) {
                VariableReference self(node.name);
                self.line = node.line; self.column = node.column; self.sourceFile = node.sourceFile;
                ir::Operand dst = emitAddressOf(&self);
                emitAggregateInto(node.initializer.get(), dst, structs_[sn].totalSize, loc(node));
            }
            return;
        }
    }
    // A variable-length array (int a[n][4]): the run-time dimensions are
    // evaluated once into hidden locals, the array is allocated on the stack
    // (alloca) and the variable is a pointer to it, indexed with run-time
    // strides; sizeof reads the computed size
    if (currentFunc_ && !node.isStatic && !node.vlaDimExprs.empty()) {
        std::string base = "__vla_" + node.name + "_" + std::to_string(nextLabel_++);
        std::unique_ptr<Expression> size = std::make_unique<SizeofExpression>(node.type, node.pointerLevel);
        std::vector<ir::Operand> dimOps;
        for (size_t i = 0; i < node.arrayDims.size(); i++) {
            std::unique_ptr<Expression> dimRef;
            if (i < node.vlaDimExprs.size() && node.vlaDimExprs[i]) {
                std::string dn = base + "_d" + std::to_string(i);
                auto dd = std::make_unique<VariableDeclaration>("int", dn, 0);
                dd->isSigned = true;
                dd->line = node.line; dd->column = node.column; dd->sourceFile = node.sourceFile;
                dd->accept(*this);
                // dn = <dimension>
                node.vlaDimExprs[i]->accept(*this);
                ir::Operand dv = emitCast(lastValue_, ir::Type::I16, getExprTypeInfo(node.vlaDimExprs[i].get()).isSigned);
                ir::Inst st;
                st.op = ir::Op::STORE; st.resultType = ir::Type::I16; st.src1 = dv; st.src2 = locals_[dn];
                emit(st);
                usedVregs_.insert(locals_[dn].vregId);
                dimRef = std::make_unique<VariableReference>(dn);
                dimOps.push_back(locals_[dn]);
            } else {
                dimRef = std::make_unique<IntegerLiteral>(node.arrayDims[i]);
                dimOps.push_back(ir::Operand::none());
            }
            size = std::make_unique<BinaryOperation>("*", std::move(size), std::move(dimRef));
        }
        std::string sz = base + "_size";
        auto sd = std::make_unique<VariableDeclaration>("int", sz, 0);
        sd->initializer = std::move(size);
        sd->line = node.line; sd->column = node.column; sd->sourceFile = node.sourceFile;
        sd->accept(*this);
        usedVregs_.insert(locals_[sz].vregId);
        // The function calls alloca: it is not a leaf (its ZP parameters
        // would be overwritten by the call's)
        currentFunc_->originalIsLeaf = false;
        currentFunc_->originalCallees.insert("_alloca");
        // T *name = (T *)alloca(size)  (declaring void *alloca(unsigned) if needed)
        if (!functionReturnTypes_.count("alloca")) {
            auto proto = std::make_unique<FunctionDeclaration>("alloca", "void");
            proto->returnPointerLevel = 1;
            proto->isPrototype = true;
            proto->isFastcall = true;   // the library's alloca is ZP asm
            Parameter prm;
            prm.type = "int";
            prm.pointerLevel = 0;
            prm.name = "size";
            proto->parameters.push_back(prm);
            auto* savedFunc = currentFunc_;
            auto savedBlock = currentBlock_;
            proto->accept(*this);
            currentFunc_ = savedFunc;
            currentBlock_ = savedBlock;
            vlaAllocaProtos_.push_back(std::move(proto));
        }
        auto call = std::make_unique<FunctionCall>("alloca");
        call->arguments.push_back(std::make_unique<VariableReference>(sz));
        call->line = node.line; call->column = node.column; call->sourceFile = node.sourceFile;
        auto pd = std::make_unique<VariableDeclaration>(node.type, node.name, node.pointerLevel + 1);
        pd->isSigned = node.isSigned;
        pd->initializer = std::make_unique<CastExpression>(node.type, node.pointerLevel + 1, node.isSigned, std::move(call));
        pd->line = node.line; pd->column = node.column; pd->sourceFile = node.sourceFile;
        pd->accept(*this);
        // Indexed like an array a[d0][d1]... at the pointer's value
        paramArrayDims_[node.name] = node.arrayDims;
        std::vector<ir::Operand> inner(dimOps.begin() + 1, dimOps.end());
        paramVlaDims_[node.name] = inner;
        localPtrLevel_[node.name] = node.pointerLevel;
        vlaSizeVars_[node.name] = sz;
        return;
    }
    // char a[] = "text": the size includes the terminating 0
    if (node.arrayDims.size() == 1 && node.arrayDims[0] <= 0 && node.pointerLevel == 0)
        if (auto* str = dynamic_cast<StringLiteral*>(node.initializer.get()))
            node.arrayDims[0] = (int)str->value.size() + 1;
    bool isStructOrUnion = (node.type.rfind("struct ", 0) == 0 || node.type.rfind("union ", 0) == 0);
    if (isStructOrUnion && node.pointerLevel == 0) {
        std::string sName = getAggregateName(node.type);
        if (!structs_.count(sName)) {
            std::string errMsg = "Unknown struct/union type: " + node.type;
            throw std::runtime_error(formatSemanticError(errMsg, node.sourceFile, node.line, node.column));
        }
    }

    // Wide integer variable initialized from an integer
    if (node.pointerLevel == 0 && node.arrayDims.empty() && node.initializer &&
        !dynamic_cast<InitializerList*>(node.initializer.get())) {
        std::string wn = getAggregateName(node.type);
        if (wn.rfind("struct ", 0) == 0) wn = wn.substr(7);
        if (wn.rfind("__int", 0) == 0 && structs_.count(wn)) toWide(node.initializer, wn, true);
    }
    // Complex variable initialized from an expression: a constant initializer
    // (globals/statics) becomes {real, imag}; otherwise the value is converted
    if (node.pointerLevel == 0 && node.arrayDims.empty() && node.initializer &&
        !dynamic_cast<InitializerList*>(node.initializer.get())) {
        std::string k = getAggregateName(node.type);
        if (k == "_Complex_float" || k == "_Complex_int") {
            double re, im;
            if ((!currentFunc_ || node.isStatic) && evalConstComplex(node.initializer.get(), re, im)) {
                auto list = std::make_unique<InitializerList>();
                if (k == "_Complex_float") {
                    list->elements.push_back(std::make_unique<FloatLiteral>(re));
                    list->elements.push_back(std::make_unique<FloatLiteral>(im));
                } else {
                    list->elements.push_back(std::make_unique<IntegerLiteral>((int64_t)re));
                    list->elements.push_back(std::make_unique<IntegerLiteral>((int64_t)im));
                }
                node.initializer = std::move(list);
            } else {
                toComplex(node.initializer, k);
            }
        }
    }

    ir::Type t = mapType(node.type, node.pointerLevel);

    if (!currentFunc_) {
        // Extern declaration: add to externs list, no BSS allocation
        auto recordGlobalType = [&]() {
            if (node.pointerLevel > 0) {
                globalPointedToType_[node.name] = node.pointerLevel >= 2 ? ir::Type::PTR : mapType(node.type, 0);   // (char **g: a pointer)
                globalPointedToTypeName_[node.name] = node.type;
            }
            if (!node.arrayDims.empty()) globalArrayDims_[node.name] = node.arrayDims;
            globalTypes_[node.name] = t;
            globalTypeNames_[node.name] = node.type;
            // T (*p)[M] is indexed like an int a[][M] parameter: its
            // elements are Ts
            globalPtrLevel_[node.name] = node.pointerLevel - (node.ptrArrayDims.empty() ? 0 : 1);
            globalSigned_[node.name] = node.isSigned;
            globalPlainUnsigned_[node.name] = node.isPlainUnsigned;
            globalRegister_[node.name] = node.isRegister;
        };
        if (node.isExtern) {
            module_.externs.push_back("_" + node.name);
            if (node.addressSpace == 1)
                module_.zpExterns["_" + node.name] = getTypeSize(node.type, node.pointerLevel) *
                                                     (node.arraySize() < 0 ? 1 : node.arraySize());
            recordGlobalType();  // its type is still needed to use it
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
        gv.addressSpace = node.addressSpace;  // Phase 97: Preserve address space qualifier
        gv.noescape = noescapeGlobal(node);
        gv.align = std::max({1, node.alignment, alignValue(node.alignmentExpr.get()),
                             typeAlign(node.type, node.pointerLevel)});
        NoescapeInit noescapeInit(initNoescape_, inStaticInit_, gv.noescape);
        declaredAlign_[node.name] = gv.align;
        if (!node.ptrArrayDims.empty()) {
            std::vector<int> d{0};
            d.insert(d.end(), node.ptrArrayDims.begin(), node.ptrArrayDims.end());
            globalPtrArrayDims_[node.name] = d;
            paramArrayDims_[node.name] = d;
        }
        if (node.initializer) checkStaticInitNames(node.initializer.get());
        if (node.initializer) {
            // -1.5: a negated float literal
            double negFloat = 0;
            bool isNegFloat = false;
            if (auto* un = dynamic_cast<UnaryOperation*>(node.initializer.get())) {
                if (un->op == "-") {
                    if (auto* f = dynamic_cast<FloatLiteral*>(un->operand.get())) { negFloat = -f->value; isNegFloat = true; }
                }
            }
            double constF = 0;
            bool floatExpr = t == ir::Type::F32 && node.pointerLevel == 0 && node.arrayDims.empty() &&
                             !dynamic_cast<InitializerList*>(node.initializer.get()) &&
                             evalConstFloat(node.initializer.get(), constF);
            if (floatExpr) {
                // float d = 1024.0 - 1.0 / 32768.0; — evaluated at compile time
                gv.hasInitValue = true;
                std::memcpy(&gv.initValue, &constF, sizeof(double));
            } else if (auto* flit = dynamic_cast<FloatLiteral*>(node.initializer.get())) {
                gv.hasInitValue = true;
                // Store double bits in initValue for CBM conversion in IRCodeGen
                std::memcpy(&gv.initValue, &flit->value, sizeof(double));
            } else if (isNegFloat) {
                gv.hasInitValue = true;
                std::memcpy(&gv.initValue, &negFloat, sizeof(double));
            } else if (auto* lit = dynamic_cast<IntegerLiteral*>(node.initializer.get())) {
                gv.hasInitValue = true;
                ConstValue lv = ConstValue::ofLiteral(*lit);   // (its value as its type says)
                if (t == ir::Type::F32) {
                    // float f = 10; — stored as the double's bits like a float literal
                    double dv = lv.asDouble();
                    std::memcpy(&gv.initValue, &dv, sizeof(double));
                } else {
                    gv.initValue = lv.intValue();
                }
            } else if (dynamic_cast<StringLiteral*>(node.initializer.get()) && !node.arrayDims.empty() &&
                       buildInitImage(gv, node.type, node.pointerLevel, node.arrayDims, node.initializer.get())) {
                // char a[] = "text": the characters (not a pointer to them)
            } else if (std::string clabel; node.pointerLevel > 0 && node.arrayDims.empty() &&
                       [&] { int coff = 0; if (!constAddress(node.initializer.get(), clabel, coff)) return false;
                             if (clabel.empty()) { gv.initValue = coff; return true; }  // (T *)0x1234
                             clabel = labelPlus(clabel, coff); return true; }()) {
                // int *p = &a[3][5]; const char *s = "abc" + 1; char *q = buf + 2;
                gv.hasInitValue = true;
                if (!clabel.empty()) gv.initLabels.push_back(clabel);
            } else if (auto* slit = dynamic_cast<StringLiteral*>(node.initializer.get())) {
                // Scalar string literal init: char *p = "hello"
                std::string label = "__str_" + std::to_string(nextLabel_++);
                ir::Module::StringLiteral sl;
                sl.label = label;
                sl.value = slit->value;
                sl.escMask = slit->escMask;
                sl.encoding = stringEncodingOf(*slit);
                sl.isAscii = sl.encoding == 1;
                sl.noescape = initNoescape_;
                module_.strings.push_back(sl);
                gv.hasInitValue = true;
                gv.initLabels.push_back(label); // symbolic reference
            } else if (std::string label = staticAddressLabel(node.initializer.get()); !label.empty()) {
                // int *p = &x; void (*f)(void) = func; char *q = buf;
                gv.hasInitValue = true;
                gv.initLabels.push_back(label);
            } else if (int noff = 0; node.pointerLevel == 0 && t != ir::Type::F32 &&
                       [&] { std::string nl; return constAddress(node.initializer.get(), nl, noff) && nl.empty(); }()) {
                // A constant computed through an address: static int o = offsetof(T, m);
                gv.hasInitValue = true;
                gv.initValue = noff;
            } else if (int64_t cv; node.arrayDims.empty() && t != ir::Type::F32 &&
                       !dynamic_cast<InitializerList*>(node.initializer.get()) &&
                       evalConstInt(node.initializer.get(), cv)) {
                // int n = sizeof a / sizeof a[0]; (an integer constant expression)
                gv.hasInitValue = true;
                gv.initValue = cv;
            } else if (auto* cast = dynamic_cast<CastExpression*>(node.initializer.get());
                       cast && dynamic_cast<IntegerLiteral*>(cast->expression.get())) {
                gv.hasInitValue = true;
                gv.initValue = static_cast<IntegerLiteral*>(cast->expression.get())->value;
            } else if (dynamic_cast<InitializerList*>(node.initializer.get()) &&
                       buildInitImage(gv, node.type, node.pointerLevel, node.arrayDims, node.initializer.get())) {
                // Laid out byte by byte (structs, nested arrays, designators)
            } else if (auto* il = dynamic_cast<InitializerList*>(node.initializer.get())) {
                gv.hasInitValue = true;
                for (auto const& elem : il->elements) {
                    if (auto* elit = dynamic_cast<IntegerLiteral*>(elem.get())) {
                        gv.initList.push_back(ConstValue::ofLiteral(*elit).intValue());
                        gv.initLabels.push_back("");
                    } else if (auto* slit = dynamic_cast<StringLiteral*>(elem.get())) {
                        // String literal in pointer array: create string constant and store label
                        std::string label = "__str_" + std::to_string(nextLabel_++);
                        ir::Module::StringLiteral sl;
                        sl.label = label;
                        sl.value = slit->value;
                        sl.escMask = slit->escMask;
                        sl.encoding = stringEncodingOf(*slit);
                        sl.isAscii = sl.encoding == 1;
                        sl.noescape = initNoescape_;
                        module_.strings.push_back(sl);
                        gv.initList.push_back(0);
                        gv.initLabels.push_back(label);
                    } else if (std::string alabel = staticAddressLabel(elem.get()); !alabel.empty()) {
                        gv.initList.push_back(0);
                        gv.initLabels.push_back(alabel);
                    } else if (int64_t ev; evalConstInt(elem.get(), ev)) {
                        gv.initList.push_back(ev);
                        gv.initLabels.push_back("");
                    } else {
                        // (it used to become 0, silently)
                        Expression* at = elem ? elem.get() : node.initializer.get();
                        errors_.push_back(formatDiagnostic(at->sourceFile.empty() ? node.sourceFile : at->sourceFile,
                            at->line ? at->line : node.line, at->line ? at->column : node.column, Severity::Error,
                            "initializer element of '" + node.name + "' is not a compile-time constant"));
                        gv.initList.push_back(0);
                        gv.initLabels.push_back("");
                    }
                }
            } else {
                // (it used to leave the variable zeroed, silently)
                errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column, Severity::Error,
                    "initializer of '" + node.name + "' is not a compile-time constant"));
            }
        }
        recordGlobalType();
        addVtableWords(gv, node.type, node.pointerLevel, node.arrayDims);
        module_.globals.push_back(gv);
        return;
    }

    if (currentFunc_ && !node.isStatic && !node.isExtern && node.addressSpace == 1)
        warnings_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column, Severity::Warning,
            "__zp has no effect on a local variable '" + node.name + "' (a static __zp one is in the base page)"));

    // Local static variables: allocate as globals, not frame locals
    if (node.isStatic && currentFunc_) {
        // Create unique global name
        std::string globalName = "_" + currentFunc_->name + "__local_" + node.name;

        // Add to globals list (DATA segment)
        ir::Module::GlobalVar gv;
        gv.name = globalName;
        gv.type = t;
        gv.size = getTypeSize(node.type, node.pointerLevel);
        if (node.arraySize() > 0) gv.size *= node.arraySize();
        gv.isConst = node.isConst;
        gv.isStatic = true;
        gv.addressSpace = node.addressSpace;  // Phase 97: Preserve address space qualifier
        gv.noescape = noescapeGlobal(node);
        gv.align = std::max({1, node.alignment, alignValue(node.alignmentExpr.get()),
                             typeAlign(node.type, node.pointerLevel)});
        NoescapeInit noescapeInit(initNoescape_, inStaticInit_, gv.noescape);
        declaredAlign_[node.name] = gv.align;

        // Handle initializer
        if (node.initializer) checkStaticInitNames(node.initializer.get());
        bool aggregateInit = node.initializer &&
            (dynamic_cast<InitializerList*>(node.initializer.get()) ||
             (dynamic_cast<StringLiteral*>(node.initializer.get()) && !node.arrayDims.empty()));
        if (aggregateInit && buildInitImage(gv, node.type, node.pointerLevel, node.arrayDims, node.initializer.get())) {
            // static struct S s = {...}; static char buf[8] = "text";
        } else if (double cf; node.initializer && t == ir::Type::F32 && node.pointerLevel == 0 &&
                   evalConstFloat(node.initializer.get(), cf)) {
            gv.hasInitValue = true;
            std::memcpy(&gv.initValue, &cf, sizeof(double));
        } else if (node.initializer) {
            if (auto* lit = dynamic_cast<IntegerLiteral*>(node.initializer.get())) {
                gv.hasInitValue = true;
                ConstValue lv = ConstValue::ofLiteral(*lit);   // (its value as its type says)
                if (t == ir::Type::F32) {
                    double dv = lv.asDouble();
                    std::memcpy(&gv.initValue, &dv, sizeof(double));
                } else {
                    gv.initValue = lv.intValue();
                }
            } else if (auto* flit = dynamic_cast<FloatLiteral*>(node.initializer.get())) {
                gv.hasInitValue = true;
                std::memcpy(&gv.initValue, &flit->value, sizeof(double));
            } else if (int64_t cv; node.arrayDims.empty() && t != ir::Type::F32 &&
                       evalConstInt(node.initializer.get(), cv)) {
                // static int n = sizeof a; (an integer constant expression)
                gv.hasInitValue = true;
                gv.initValue = cv;
            }
        }
        // (no initializer: BSS, zeroed by crt0 like an uninitialized global;
        // an explicit = 0 stays in DATA, which -fbss-fill=V does not touch)

        addVtableWords(gv, node.type, node.pointerLevel, node.arrayDims);
        module_.globals.push_back(gv);

        // Store mapping for scope lookup
        locals_[node.name] = ir::Operand::global(globalName);
        localTypes_[node.name] = t;
        // Phase 102: Resolve typedef to struct name for global types
        std::string resolvedGlobalTypeName = node.type;
        if (resolvedGlobalTypeName.find("struct ") != 0) {
            std::string resolved = resolveTypedefToStruct(resolvedGlobalTypeName);
            if (resolved != resolvedGlobalTypeName) {
                resolvedGlobalTypeName = resolved;
            }
        }
        localTypeNames_[node.name] = resolvedGlobalTypeName;
        localPtrLevel_[node.name] = node.pointerLevel;
        localSigned_[node.name] = node.isSigned;
        localPlainUnsigned_[node.name] = node.isPlainUnsigned;
        localConst_[node.name] = node.isConst && node.pointerLevel == 0;
        localPointsToConst_[node.name] = node.isConst && node.pointerLevel > 0;
        localDeclLocs_[node.name] = declLoc(node);
        if (node.pointerLevel > 0) {
            localPointedToType_[node.name] = mapType(node.type, node.pointerLevel - 1);
            localPointedToTypeName_[node.name] = resolvedGlobalTypeName;
        }
        // A local static array: its dimensions (indexing strides, decay)
        if (!node.arrayDims.empty()) localArrayDims_[node.name] = node.arrayDims;
        return;  // Skip frame allocation
    }

    // Local variable — allocate a vReg
    auto vreg = allocVreg(t);
    locals_[node.name] = vreg;
    localRegister_[node.name] = node.isRegister;
    localDeclLocs_[node.name] = declLoc(node);
    if (currentFunc_) {
        currentFunc_->localNames[node.name] = vreg.vregId;
        currentFunc_->localNamesOrder.push_back(node.name);
        if (node.isRegister) {
            currentFunc_->registerVregs.insert(vreg.vregId);
        }
    }
    localTypes_[node.name] = t;
    // Phase 102: Resolve typedef to struct name for variable types
    std::string resolvedVarTypeName = node.type;
    if (resolvedVarTypeName.find("struct ") != 0) {
        std::string resolved = resolveTypedefToStruct(resolvedVarTypeName);
        if (resolved != resolvedVarTypeName) {
            resolvedVarTypeName = resolved;
        }
    }
    localTypeNames_[node.name] = resolvedVarTypeName;
    localPtrLevel_[node.name] = node.pointerLevel;
    // T (*p)[M]: indexed like an int a[][M] parameter
    if (!node.ptrArrayDims.empty()) {
        std::vector<int> d{0};
        d.insert(d.end(), node.ptrArrayDims.begin(), node.ptrArrayDims.end());
        paramArrayDims_[node.name] = d;
        localPtrLevel_[node.name] = node.pointerLevel - 1;
    } else {
        paramArrayDims_.erase(node.name);
    }
    localSigned_[node.name] = node.isSigned;
    localPlainUnsigned_[node.name] = node.isPlainUnsigned;
    localConst_[node.name] = node.isConst && node.pointerLevel == 0;
    localPointsToConst_[node.name] = node.isConst && node.pointerLevel > 0;
    if (node.pointerLevel > 0) {
        localPointedToType_[node.name] = mapType(node.type, node.pointerLevel - 1);
        localPointedToTypeName_[node.name] = resolvedVarTypeName;
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
    if (node.isRegister && currentFunc_) currentFunc_->registerVregs.insert(vreg.vregId);
    if (currentFunc_) currentFunc_->localSlotVregs.insert(vreg.vregId);
    if (currentFunc_) {
        int la = std::max({1, node.alignment, alignValue(node.alignmentExpr.get()),
                           typeAlign(node.type, node.pointerLevel)});
        if (la > 1) currentFunc_->vregAlign[vreg.vregId] = la;
        declaredAlign_[node.name] = la;
    }
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

    // Phase 113: Track local variable declaration for DWARF debug info
    if (currentFunc_) {
        sourceTracker_.trackVariableDecl(node.name, node.line, node.column,
                                        resolvedVarTypeName, vreg.vregId, false);
    }

    // Emit initializer (skip for local statics - they're initialized at startup in DATA segment)
    // Braced initializer of a local array/struct/union (nested braces, brace
    // elision, designators, unions), or a local char array from a string
    bool localAggInit = false;
    if (node.initializer && !node.isStatic) {
        Expression* init = node.initializer.get();
        bool isAgg = !node.arrayDims.empty() || (node.pointerLevel == 0 && structOf(node.type));
        bool charArrayFromString = dynamic_cast<StringLiteral*>(init) && node.arrayDims.size() == 1 &&
                                   node.pointerLevel == 0 && getTypeSize(node.type, 0) == 1;
        if (((dynamic_cast<InitializerList*>(init) && isAgg) || charArrayFromString) &&
            initLocalAggregate(vreg, node.type, node.pointerLevel, node.arrayDims, init, loc(node)))
            localAggInit = true;
    }
    if (node.initializer && !node.isStatic && !localAggInit) {
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

                // IMPORTANT: Use ADDR_LOCAL to explicitly compute frame address before store.
                // This ensures correctness when variables are allocated to frame locations.
                // While IRCodeGen may optimize this to direct frame store for IN_FRAME vregs,
                // the explicit address computation makes the IR more robust and correct.
                // Test case: src/test-resources/test_scalar_init.c
                auto addr = allocVreg(ir::Type::PTR);
                ir::Inst addrInst;
                addrInst.op = ir::Op::ADDR_LOCAL;
                addrInst.dest = addr;
                addrInst.resultType = ir::Type::PTR;
                addrInst.src1 = ir::Operand::vreg(vreg.vregId, ir::Type::PTR);
                addrInst.loc = loc(node);
                emit(addrInst);

                ir::Inst store;
                store.op = ir::Op::STORE;
                store.resultType = t;
                store.src1 = lastValue_;
                store.src2 = addr;
                store.loc = loc(node);
                emit(store);
            }
        } else if (auto* cl = dynamic_cast<CompoundLiteral*>(node.initializer.get());
                   cl && cl->initializer && node.pointerLevel == 0 && node.arrayDims.empty() &&
                   cl->pointerLevel == 0 && cl->arrayDims.empty() && structOf(node.type) &&
                   structOf(cl->targetType) == structOf(node.type) &&
                   initLocalAggregate(vreg, node.type, 0, {}, cl->initializer.get(), loc(node))) {
            // struct S x = (struct S){...}: laid out directly in the variable
        } else if (int aggSize = node.arrayDims.empty() ? aggregateBlockSize(node.type, node.pointerLevel) : 0;
                   aggSize > 0 && !dynamic_cast<CompoundLiteral*>(node.initializer.get())) {
            // Struct/union initialised from another aggregate: copy by address
            currentFunc_->memoryVregs.insert(vreg.vregId);
            auto dst = allocVreg(ir::Type::PTR);
            ir::Inst addrInst;
            addrInst.op = ir::Op::ADDR_LOCAL;
            addrInst.dest = dst;
            addrInst.resultType = ir::Type::PTR;
            addrInst.src1 = ir::Operand::vreg(vreg.vregId, ir::Type::PTR);
            addrInst.loc = loc(node);
            emit(addrInst);
            emitAggregateInto(node.initializer.get(), dst, aggSize, loc(node));
        } else {
            // Simple scalar initializer
            node.initializer->accept(*this);
            ir::Operand initVal = lastValue_;
            // float x = 3; int i = 2.5; long l = c; — convert to the variable's
            // type (widening by the value's signedness)
            if (initVal.type != t && initVal.type != ir::Type::VOID && t != ir::Type::I_N &&
                initVal.type != ir::Type::I_N)
                initVal = emitCast(initVal, t, getExprTypeInfo(node.initializer.get()).isSigned);
            ir::Inst store;
            store.op = ir::Op::STORE;
            store.resultType = t;
            store.src1 = initVal;
            store.src2 = vreg;
            store.loc = loc(node);
            emit(store);
        }
    }

    // Virtual methods: point every __vt of the object (also in arrays and
    // nested members) at its struct's vtable -- after the initializer, which
    // may zero or copy the whole object. (Static locals get theirs in DATA.)
    if (currentFunc_ && !node.isStatic) {
        std::vector<std::pair<int, std::string>> slots;
        collectVtableSlots(node.type, node.pointerLevel, node.arrayDims, 0, slots);
        if (!slots.empty()) {
            auto baseAddr = allocVreg(ir::Type::PTR);
            ir::Inst addr;
            addr.op = ir::Op::ADDR_LOCAL;
            addr.dest = baseAddr;
            addr.resultType = ir::Type::PTR;
            addr.src1 = vreg;
            addr.loc = loc(node);
            emit(addr);
            for (const auto& [off, sym] : slots) {
                ir::Operand dst = baseAddr;
                if (off != 0) {
                    dst = allocVreg(ir::Type::PTR);
                    ir::Inst add;
                    add.op = ir::Op::ADD;
                    add.dest = dst;
                    add.resultType = ir::Type::PTR;
                    add.src1 = baseAddr;
                    add.src2 = ir::Operand::imm(off, ir::Type::I16);
                    add.loc = loc(node);
                    emit(add);
                }
                auto vtAddr = allocVreg(ir::Type::PTR);
                ir::Inst loadVt;
                loadVt.op = ir::Op::ADDR_GLOBAL;
                loadVt.dest = vtAddr;
                loadVt.resultType = ir::Type::PTR;
                loadVt.src1 = ir::Operand::global(sym);
                loadVt.loc = loc(node);
                emit(loadVt);
                ir::Inst storeVt;
                storeVt.op = ir::Op::STORE;
                storeVt.resultType = ir::Type::I16;
                storeVt.src1 = vtAddr;
                storeVt.src2 = dst;
                storeVt.loc = loc(node);
                emit(storeVt);
            }
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
    // Encoding: an @/a/p/s prefix, otherwise the pragma state
    int encoding = stringEncodingOf(node);
    // Record the string literal in the module for data section emission
    ir::Module::StringLiteral sl;
    sl.label = name;
    sl.value = node.value;
    sl.escMask = node.escMask;
    sl.isAscii = (encoding == 1);
    sl.encoding = encoding;
    sl.wide = node.isWide;
    // #pragma cc45 noescape_strings: with the code of the function using it
    sl.noescape = (node.noescape || initNoescape_) && currentFunc_;
    sl.far = sl.noescape && currentFunc_->isFar;
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
    if (!vmVars_.empty() && vmVars_.count(node.name) && locals_.count(node.name) && !inVmVarLoad_) {
        // A struct with run-time member sizes: the variable holds its address
        bool saved = computeAddressOnly_;
        computeAddressOnly_ = false;
        inVmVarLoad_ = true;
        visit(node);
        inVmVarLoad_ = false;
        computeAddressOnly_ = saved;
        return;
    }
    if (!locals_.count(node.name)) {
        auto rn = staticRenames_.find(node.name);
        if (rn != staticRenames_.end()) node.name = rn->second;
    }
    // Check if this is a function name used as a value (function pointer);
    // a local or parameter of the same name shadows the function
    auto fit = (locals_.count(node.name) ||
                (builtinProtoNames_.count(node.name) && globalTypes_.count(node.name)))
                   ? allFunctions_.end() : allFunctions_.find(node.name);
    if (fit != allFunctions_.end()) {
        auto* fn = fit->second;
        // a printf-like function as a value: its calls' formats are unknown
        if (printfLikeFunctions().count(node.name)) needAllFormats();
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
                markStaticLinkProvider();
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

        // By-copy aggregate parameter: the parameter holds the copy's address
        if (aggCopyParams_.count(node.name)) {
            lastValue_ = it->second;
            lastValueSigned_ = false;
            return;
        }

        // Array-to-pointer decay: when an array name is used as a value
        // (not in address-only mode), it decays to a pointer to its first element.
        bool isArray = localArrayDims_.count(node.name) > 0;

        if ((computeAddressOnly_ || isArray) && it->second.kind == ir::OperandKind::GLOBAL) {
            // A local static lives in a global: the operand is its address
            lastValue_ = it->second;
        } else if (computeAddressOnly_ || isArray) {
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
        } else if (it->second.kind == ir::OperandKind::GLOBAL) {
            // A scalar local static: load its value from the global
            ir::Type t = localTypes_.count(node.name) ? localTypes_[node.name] : ir::Type::I16;
            auto v = allocVreg(t);
            ir::Inst ld;
            ld.op = ir::Op::LOAD;
            ld.dest = v;
            ld.resultType = t;
            ld.src1 = it->second;
            ld.loc = loc(node);
            emit(ld);
            lastValue_ = v;
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
            // The nested function reaches it through the parent's frame
            // pointer (static chain): the parent needs a real stack frame
            scope.func->isRecurse = true;
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
            addUplevelChain(addrInst, levels);
            addrInst.loc = loc(node);
            emit(addrInst);

            auto sad = scope.localArrayDims.find(node.name);
            bool isArray = sad != scope.localArrayDims.end() && !sad->second.empty();
            if (computeAddressOnly_ || isArray) {
                lastValue_ = addr;   // (an array is its address)
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

    // An identifier declared nowhere (C99 6.5.1p2): without this it would be
    // an unresolved symbol the assembler reads as address 0
    if (!globalTypes_.count(node.name) && !globalArrayDims_.count(node.name) &&
        node.name.rfind("__", 0) != 0) {
        throw std::runtime_error(formatDiagnostic(node.sourceFile, node.line, node.column, Severity::Error,
            "'" + node.name + "' undeclared"));
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
    if (auto* vr = dynamic_cast<VariableReference*>(node.target.get())) {
        auto it = locals_.find(vr->name);
        if (it != locals_.end()) setVregs_.insert(it->second.vregId);
    }
    // An integer target of a wide integer value: its low bytes
    if (node.op == "=" && wideKindOf(node.target.get()).empty()) {
        IRTypeInfo tt = getExprTypeInfo(node.target.get());
        if (!structOf(tt.typeName)) narrowWide(node.expression, tt.type);
    }
    // A vector target: v op= x is v = v op x
    if (node.op != "=" && !vecOf(node.target.get()).empty()) {
        if (auto copy = cloneSimpleExpr(node.target.get())) {
            auto bin = std::make_unique<BinaryOperation>(node.op.substr(0, node.op.size() - 1), std::move(copy), std::move(node.expression));
            bin->line = node.line; bin->column = node.column; bin->sourceFile = node.sourceFile;
            node.expression = std::move(bin);
            node.op = "=";
        }
    }
    // Wide integer target: x op= y as x = x op y, an integer value converted
    if (std::string w = wideKindOf(node.target.get()); !w.empty()) {
        bool simple = node.op.size() == 2 && node.op[1] == '=' && std::string("+-*/%&|^").find(node.op[0]) != std::string::npos;
        bool shift = node.op == "<<=" || node.op == ">>=";   // (#330)
        if (simple || shift) {
            if (auto copy = cloneSimpleExpr(node.target.get())) {
                auto bin = std::make_unique<BinaryOperation>(node.op.substr(0, node.op.size() - 1), std::move(copy), std::move(node.expression));
                bin->line = node.line; bin->column = node.column; bin->sourceFile = node.sourceFile;
                node.expression = std::move(bin);
                node.op = "=";
            }
        }
        if (node.op == "=") toWide(node.expression, w, false);
    }
    // Complex target: the value as that complex type; x op= y as x = x op y
    if (std::string k = complexKindOf(node.target.get()); !k.empty()) {
        if (node.op == "+=" || node.op == "-=" || node.op == "*=" || node.op == "/=") {
            if (auto copy = cloneSimpleExpr(node.target.get())) {
                auto bin = std::make_unique<BinaryOperation>(node.op.substr(0, 1), std::move(copy), std::move(node.expression));
                bin->line = node.line; bin->column = node.column; bin->sourceFile = node.sourceFile;
                node.expression = std::move(bin);
                node.op = "=";
            }
        }
        if (node.op == "=") toComplex(node.expression, k);
    }
    // A struct or union target of x op= y: its operator method (operator+=:
    // S__operator_add_assign(&x, y), as the binary operators dispatch);
    // without one it is an error (a small struct is not an integer)
    if (node.op != "=") {
        IRTypeInfo ti = getExprTypeInfo(node.target.get());
        std::string sName = ti.typeName.empty() ? std::string() : getAggregateName(ti.typeName);
        if (!sName.empty() && ti.type != ir::Type::PTR && structs_.count(sName)) {
            static const std::map<std::string, std::string> assignOps = {
                {"+=", "add_assign"}, {"-=", "sub_assign"}, {"*=", "mul_assign"},
                {"/=", "div_assign"}, {"%=", "mod_assign"}, {"&=", "band_assign"},
                {"|=", "bor_assign"}, {"^=", "bxor_assign"}, {"<<=", "shl_assign"}, {">>=", "shr_assign"},
            };
            auto it = assignOps.find(node.op);
            if (it != assignOps.end() && functionReturnTypes_.count(sName + "__operator_" + it->second)) {
                BinaryOperation bin(node.op, std::move(node.target), std::move(node.expression));
                bin.line = node.line; bin.column = node.column; bin.sourceFile = node.sourceFile;
                bin.accept(*this);
                node.target = std::move(bin.left);       /* the AST stays whole (inlining copies it) */
                node.expression = std::move(bin.right);
                return;
            }
            errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column, Severity::Error,
                "invalid operands to " + node.op + " ('" + ti.typeName + "' has no operator" + node.op + ")"));
            lastValue_ = ir::Operand::imm(0, ir::Type::I16);
            return;
        }
    }
    // 1. Semantic checks for constness
    if (auto* ma = dynamic_cast<MemberAccess*>(node.target.get())) {
        // Determine the struct type of the variable being accessed
        std::string structTypeName;
        if (auto* vr = dynamic_cast<VariableReference*>(ma->structExpr.get())) {
            // Look up the variable's type in local scope first, then global scope
            auto lit = localTypeNames_.find(vr->name);
            if (lit != localTypeNames_.end()) {
                structTypeName = lit->second;
            } else {
                auto git = globalTypeNames_.find(vr->name);
                if (git != globalTypeNames_.end()) {
                    structTypeName = git->second;
                }
            }
        }

        // If we found the struct type, check if the member is const
        if (!structTypeName.empty()) {
            // Remove "struct " or "union " prefix if present
            if (structTypeName.substr(0, 7) == "struct ") {
                structTypeName = structTypeName.substr(7);
            } else if (structTypeName.substr(0, 6) == "union ") {
                structTypeName = structTypeName.substr(6);
            }

            // Look up the struct definition
            auto sit = structs_.find(structTypeName);
            if (sit != structs_.end()) {
                auto mit = sit->second.members.find(ma->memberName);
                if (mit != sit->second.members.end() && mit->second.isConst) {
                    errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column,
                        Severity::Error, "Compile Error: Assignment to read-only location"));
                    return;
                }
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
                    // a store through the pointer uses it
                    if (auto lit = locals_.find(ref->name); lit != locals_.end()) usedVregs_.insert(lit->second.vregId);
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

    // Struct/union assignment: copy by address
    if (node.op == "=") {
        IRTypeInfo ti = getExprTypeInfo(node.target.get());
        int aggSize = (ti.type == ir::Type::PTR && !isAggCopyParam(node.target.get()))
                      ? 0 : aggregateBlockSize(ti.typeName, 0);
        if (aggSize > 0) {
            auto dst = emitAddressOf(node.target.get());
            // Only a local variable can't be memory the callee reads
            bool localTarget = false;
            if (auto* vr = dynamic_cast<VariableReference*>(node.target.get())) {
                auto lit = locals_.find(vr->name);
                localTarget = lit != locals_.end() && lit->second.kind != ir::OperandKind::GLOBAL;
            }
            emitAggregateInto(node.expression.get(), dst, aggSize, loc(node), !localTarget);
            lastValue_ = dst;
            return;
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
            // (a wider signed type holds every value of a narrower unsigned one)
            ir::Type lt = lhsVal.type;
            if (!bothSigned && lt != ir::Type::PTR && rhsInfo.type != ir::Type::PTR &&
                lt != ir::Type::F32 && rhsInfo.type != ir::Type::F32) {
                int ls = (int)ir::typeSize(lt), rs = (int)ir::typeSize(rhsInfo.type);
                if ((lhsSigned && ls > rs) || (rhsInfo.isSigned && rs > ls)) bothSigned = true;
            }
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
        else if (node.op == ">>=") binOp = lhsSigned ? ir::Op::ASR : ir::Op::SHR;

        // Float operands: x op= y is computed in floating point and converted
        // back to x's type (int += float included)
        ir::Op floatOp = ir::Op::NOP;
        if (node.op == "+=") floatOp = ir::Op::FADD;
        else if (node.op == "-=") floatOp = ir::Op::FSUB;
        else if (node.op == "*=") floatOp = ir::Op::FMUL;
        else if (node.op == "/=") floatOp = ir::Op::FDIV;
        if (floatOp != ir::Op::NOP && (lhsVal.type == ir::Type::F32 || rhs.type == ir::Type::F32)) {
            auto fl = emitCast(lhsVal, ir::Type::F32, lhsSigned);
            auto fr = emitCast(rhs, ir::Type::F32, rhsInfo.isSigned);
            auto fd = allocVreg(ir::Type::F32);
            ir::Inst inst;
            inst.op = floatOp;
            inst.dest = fd;
            inst.resultType = ir::Type::F32;
            inst.src1 = fl;
            inst.src2 = fr;
            inst.loc = loc(node);
            emit(inst);
            rhs = emitCast(fd, lhsVal.type, lhsSigned);
            binOp = ir::Op::NOP;
        }

        // a signed and an unsigned char divided: in (signed) int, as for /
        bool mixedChars = (binOp == ir::Op::DIV || binOp == ir::Op::DIV_U ||
                           binOp == ir::Op::MOD || binOp == ir::Op::MOD_U) &&
                          lhsVal.type == ir::Type::I8 && rhs.type == ir::Type::I8 &&
                          lhsSigned != rhsInfo.isSigned;
        ir::Type narrowType = lhsVal.type;
        if (mixedChars) {
            binOp = (binOp == ir::Op::DIV || binOp == ir::Op::DIV_U) ? ir::Op::DIV : ir::Op::MOD;
            lhsVal = emitCast(lhsVal, ir::Type::I16, lhsSigned);
        }

        if (binOp != ir::Op::NOP) {
            ir::Type opType = lhsVal.type;
            // (widened by its own signedness: l += (signed char)-1 adds -1)
            auto castRhs = emitCast(rhs, opType, rhsInfo.isSigned);
            // p += n / p -= n: scale n by the pointed-to element size
            if (opType == ir::Type::PTR && (binOp == ir::Op::ADD || binOp == ir::Op::SUB)) {
                IRTypeInfo pinfo = getExprTypeInfo(node.target.get());
                int elemSize = 1;
                if (pinfo.pointedToType != ir::Type::VOID) elemSize = ir::typeSize(pinfo.pointedToType);
                if (const IRStructInfo* si = pinfo.typeName.empty() ? nullptr : structOf(pinfo.typeName))
                    elemSize = si->totalSize;
                if (elemSize > 1) {
                    auto scaled = allocVreg(ir::Type::PTR);
                    ir::Inst mul;
                    mul.op = ir::Op::MUL_U;
                    mul.dest = scaled;
                    mul.resultType = ir::Type::PTR;
                    mul.src1 = castRhs;
                    mul.src2 = ir::Operand::imm(elemSize, ir::Type::PTR);
                    mul.loc = loc(node);
                    emit(mul);
                    castRhs = scaled;
                }
            }
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
            if (mixedChars) rhs = emitCast(dest, narrowType, false);
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
        // The member of the struct actually accessed (another struct may
        // have a bitfield of the same name)
        {
            std::string t = getExprTypeInfo(ma->structExpr.get()).typeName;
            if (t.rfind("struct ", 0) != 0 && t.rfind("union ", 0) != 0) t = resolveTypedefToStruct(t);
            int accOffset = 0;
            auto* mit = structs_.count(getAggregateName(t))
                      ? findStructMember(getAggregateName(t), ma->memberName, accOffset) : nullptr;
            if (mit && mit->bitWidth > 0) {
                bitWidth = mit->bitWidth;
                bitOffset = mit->bitOffset;
                memberOffset = mit->offset + accOffset;
                memberType = mapType(mit->type, mit->pointerLevel);
            }
        }
        if (bitWidth > 0) {
            bool oldAddrMode = computeAddressOnly_;
            computeAddressOnly_ = !ma->isArrow;
            ma->structExpr->accept(*this);
            computeAddressOnly_ = oldAddrMode;
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

            // The value in the field's own type (a short into a long:29
            // sign-extends: its upper bits are the field's)
            if (rhs.type != memberType && rhs.type != ir::Type::VOID && memberType != ir::Type::I_N)
                rhs = emitCast(rhs, memberType, node.op == "=" ? getExprTypeInfo(node.expression.get()).isSigned
                                                                : lastValueSigned_);
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

            // Store the modified unit back (its own width: a char unit is 1 byte)
            ir::Inst store;
            store.op = ir::Op::STORE;
            store.src1 = bfinsResult;
            store.src2 = addr;
            store.resultType = memberType;
            store.loc = loc(node);
            emit(store);

            lastValue_ = rhs;
            return;
        }
    }

    // 4. Standard assignment
    IRTypeInfo lhsInfo = getExprTypeInfo(node.target.get());
    ir::Type storeType = lhsInfo.type;
    // A conversion (widening, int -> float) follows the value's signedness
    auto rhsVal = emitCast(rhs, storeType, getExprTypeInfo(node.expression.get()).isSigned);

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
                    // a store through the pointer uses it
                    if (auto lit = locals_.find(ref->name); lit != locals_.end()) usedVregs_.insert(lit->second.vregId);
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

// ============================================================================
// GCC vector extension: typedef T V __attribute__((vector_size(N))) is the
// struct __vec_<T>_<s|u>_<n> { T e[n]; } (made by the parser). Operators work
// element by element; v[i] is v.e[i].
// ============================================================================

// v[i] of a vector is v.e[i] (the subscript is rewritten in place)
void IRBuilder::vecSubscript(ArrayAccess& node) {
    if (vecOf(node.arrayExpr.get()).empty()) return;
    auto ma = std::make_unique<MemberAccess>(std::move(node.arrayExpr), "e", false);
    ma->line = node.line; ma->column = node.column; ma->sourceFile = node.sourceFile;
    node.arrayExpr = std::move(ma);
}

// The vector struct of a type name, or ""
std::string IRBuilder::vecOfType(const std::string& typeName, int ptrLevel) {
    if (ptrLevel > 0) return "";
    std::string n = getAggregateName(typeName);
    return (n.rfind("__vec_", 0) == 0 && structs_.count(n)) ? n : "";
}

// The vector struct an expression's value has, or ""
std::string IRBuilder::vecOf(Expression* e) {
    if (!e) return "";
    IRTypeInfo ti = getExprTypeInfo(e);
    // a pointer to vectors or an array of them (PTR) is not one; a vector
    // value is I16/I32 or a by-copy parameter
    if (ti.type == ir::Type::PTR && !isAggCopyParam(e)) return "";
    std::string v = vecOfType(ti.typeName, 0);
    if (v.empty()) return "";
    // a pointer to a vector is not one
    if (auto* vr = dynamic_cast<VariableReference*>(e)) {
        auto pl = localPtrLevel_.find(vr->name);
        if (pl != localPtrLevel_.end() && locals_.count(vr->name) && pl->second > 0) return "";
        auto gp = globalPtrLevel_.find(vr->name);
        if (!locals_.count(vr->name) && gp != globalPtrLevel_.end() && gp->second > 0) return "";
    }
    return v;
}

// The element of a vector struct
void IRBuilder::vecElem(const std::string& vn, std::string& type, bool& isSigned, int& count) {
    auto& si = structs_[vn];
    auto it = si.members.find("e");
    type = it->second.type;
    isSigned = it->second.isSigned;
    count = it->second.arrayDims.empty() ? 1 : it->second.arrayDims[0];
}

// The vector of signed integers as wide as vn's elements (a comparison's result)
std::string IRBuilder::vecMaskType(const std::string& vn) {
    std::string et; bool sg; int n;
    vecElem(vn, et, sg, n);
    int es = std::max(1, getTypeSize(et, 0));
    std::string it = es == 1 ? "char" : es == 2 ? "int" : "long";
    std::string name = "__vec_" + it + "_s_" + std::to_string(n);
    if (!structs_.count(name)) {
        auto def = std::make_unique<StructDefinition>(name, false);
        StructMember sm;
        sm.type = it;
        sm.pointerLevel = 0;
        sm.isSigned = true;
        sm.name = "e";
        sm.arrayDims = {n};
        def->members.push_back(std::move(sm));
        bool saved = registeringStructs_;
        registeringStructs_ = true;  // layout only
        def->accept(*this);
        registeringStructs_ = saved;
        vecNodes_.push_back(std::move(def));
    }
    return name;
}

// A hidden local holding a value (an operand evaluated once): a pointer to a
// vector's elements, or a scalar converted to the element type
std::string IRBuilder::vecTemp(const ir::Operand& v, ir::Type t, const std::string& typeName, int ptrLevel, bool sg) {
    std::string name = "__vop" + std::to_string(vecTempCount_++);
    auto slot = allocVreg(t);
    currentFunc_->localSlotVregs.insert(slot.vregId);
    usedVregs_.insert(slot.vregId);
    locals_[name] = slot;
    localTypes_[name] = t;
    localTypeNames_[name] = typeName;
    localPtrLevel_[name] = ptrLevel;
    localSigned_[name] = sg;
    if (ptrLevel > 0) {
        localPointedToType_[name] = mapType(typeName, 0);
        localPointedToTypeName_[name] = typeName;
    }
    ir::Inst st;
    st.op = ir::Op::STORE;
    st.resultType = t;
    st.src1 = v;
    st.src2 = slot;
    emit(st);
    return name;
}

// An operand of a vector operation: "" for none, else a hidden local:
// isVec - a pointer to the elements; else the scalar converted
std::string IRBuilder::vecOperand(Expression* e, bool isVec, const std::string& et, bool sg) {
    if (!e) return "";
    if (isVec) {
        ir::Operand a = emitAddressOf(e);
        return vecTemp(a, ir::Type::PTR, et, 1, sg);
    }
    bool saved = computeAddressOnly_;
    computeAddressOnly_ = false;
    e->accept(*this);
    computeAddressOnly_ = saved;
    ir::Type t = mapType(et, 0);
    ir::Operand v = emitCast(lastValue_, t, getExprTypeInfo(e).isSigned);
    return vecTemp(v, t, et, 0, sg);
}

// The result of a vector operation: a hidden local array of rn's size; each
// element from elem(i). The value: its address, or (a 1, 2 or 4-byte vector,
// a register value) its contents
// The hidden local array holding a vector operation's result
std::string IRBuilder::vecResultArray(const std::string& rn, ASTNode& at) {
    std::string et; bool sg; int n;
    vecElem(rn, et, sg, n);
    std::string res = "__vres" + std::to_string(vecTempCount_++);
    auto decl = std::make_unique<VariableDeclaration>(et, res, 0);
    decl->isSigned = sg;
    decl->arrayDims = {n};
    decl->line = at.line; decl->column = at.column; decl->sourceFile = at.sourceFile;
    bool saved = computeAddressOnly_;
    computeAddressOnly_ = false;
    decl->accept(*this);
    computeAddressOnly_ = saved;
    usedVregs_.insert(locals_[res].vregId);
    vecNodes_.push_back(std::move(decl));
    return res;
}

// The value of a vector result array: its address, or (a 1, 2 or 4-byte
// vector, a register value) its contents
void IRBuilder::vecResultValue(const std::string& rn, const std::string& res, ASTNode& at) {
    VariableReference rv(res);
    rv.line = at.line; rv.column = at.column; rv.sourceFile = at.sourceFile;
    bool saved = computeAddressOnly_;
    computeAddressOnly_ = false;
    rv.accept(*this);
    computeAddressOnly_ = saved;
    ir::Operand addr = lastValue_;
    int size = getTypeSize("struct " + rn, 0);
    if (!computeAddressOnly_ && (size == 1 || size == 2 || size == 4)) {
        ir::Type t = mapType("struct " + rn, 0);
        auto v = allocVreg(t);
        ir::Inst ld;
        ld.op = ir::Op::LOAD;
        ld.dest = v;
        ld.resultType = t;
        ld.src1 = addr;
        emit(ld);
        lastValue_ = v;
        return;
    }
    lastValue_ = addr;
}

void IRBuilder::vecResult(const std::string& rn, const std::function<std::unique_ptr<Expression>(const VecIndex&)>& elem,
                          ASTNode& at) {
    std::string et; bool sg; int n;
    vecElem(rn, et, sg, n);
    std::string res = vecResultArray(rn, at);
    bool saved = computeAddressOnly_;
    computeAddressOnly_ = false;
    // Unrolled when the estimated code fits -fmax-unroll-bytes, else a loop.
    // Per element (measured): about 20 bytes for 1/2-byte elements (indexed
    // loads and the store; * / % are calls), 210 for 4-byte and 60 for float
    int es = std::max(1, getTypeSize(et, 0));
    int perElem = mapType(et, 0) == ir::Type::F32 ? 60 : es <= 2 ? 20 : 210;
    if (n * perElem <= maxUnrollBytes) {
        // unrolled
        for (int i = 0; i < n; i++) {
            VecIndex idx = [i]() -> std::unique_ptr<Expression> { return std::make_unique<IntegerLiteral>(i); };
            auto lhs = std::make_unique<ArrayAccess>(std::make_unique<VariableReference>(res), idx());
            auto as = std::make_unique<Assignment>(std::move(lhs), std::make_unique<CastExpression>(et, 0, sg, elem(idx)));
            as->line = at.line; as->column = at.column; as->sourceFile = at.sourceFile;
            as->accept(*this);
            vecNodes_.push_back(std::move(as));
        }
    } else {
        // for (iv = 0; iv < n; iv = iv + 1) res[iv] = elem(iv);
        std::string iv = "__vi" + std::to_string(vecTempCount_++);
        auto ivd = std::make_unique<VariableDeclaration>("int", iv, 0);
        ivd->line = at.line; ivd->column = at.column; ivd->sourceFile = at.sourceFile;
        ivd->accept(*this);
        usedVregs_.insert(locals_[iv].vregId);
        vecNodes_.push_back(std::move(ivd));
        VecIndex idx = [iv]() -> std::unique_ptr<Expression> { return std::make_unique<VariableReference>(iv); };
        auto body = std::make_unique<ExpressionStatement>(std::make_unique<Assignment>(
            std::make_unique<ArrayAccess>(std::make_unique<VariableReference>(res), idx()),
            std::make_unique<CastExpression>(et, 0, sg, elem(idx))));
        auto loop = std::make_unique<ForStatement>(
            std::make_unique<ExpressionStatement>(std::make_unique<Assignment>(std::make_unique<VariableReference>(iv), std::make_unique<IntegerLiteral>(0))),
            std::make_unique<BinaryOperation>("<", std::make_unique<VariableReference>(iv), std::make_unique<IntegerLiteral>(n)),
            std::make_unique<Assignment>(std::make_unique<VariableReference>(iv),
                std::make_unique<BinaryOperation>("+", std::make_unique<VariableReference>(iv), std::make_unique<IntegerLiteral>(1))),
            std::move(body));
        loop->line = at.line; loop->column = at.column; loop->sourceFile = at.sourceFile;
        loop->accept(*this);
        vecNodes_.push_back(std::move(loop));
    }
    computeAddressOnly_ = saved;
    vecResultValue(rn, res, at);
}

static std::unique_ptr<Expression> vecRef(const std::string& tmp, bool isVec, const IRBuilder::VecIndex& i) {
    if (isVec) return std::make_unique<ArrayAccess>(std::make_unique<VariableReference>(tmp), i());
    return std::make_unique<VariableReference>(tmp);
}

// a op b with a vector operand (the other a vector of the same type or a
// scalar); comparisons give -1/0 per element
void IRBuilder::emitVectorBinary(BinaryOperation& node) {
    std::string vl = vecOf(node.left.get()), vr = vecOf(node.right.get());
    std::string vn = !vl.empty() ? vl : vr;
    std::string et; bool sg; int n;
    vecElem(vn, et, sg, n);
    const std::string& op = node.op;
    bool cmp = op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=";
    int es = std::max(1, getTypeSize(et, 0));
    static const std::map<std::string, int> direct = {{"+", 0}, {"-", 1}, {"&", 2}, {"|", 3}, {"^", 4}};
    if (direct.count(op) && (es == 1 || es == 2) && !vl.empty() && mapType(et, 0) != ir::Type::F32) {
        // Byte-wise: one VEC_BIN (unrolled when it fits -fmax-unroll-bytes:
        // about 8 bytes per element byte, a loop adds 8)
        bool scalar = vr.empty();
        ir::Operand pa = emitAddressOf(node.left.get());
        ir::Operand pb;
        if (scalar) {
            bool saved = computeAddressOnly_;
            computeAddressOnly_ = false;
            node.right->accept(*this);
            computeAddressOnly_ = saved;
            pb = emitCast(lastValue_, ir::Type::I16, getExprTypeInfo(node.right.get()).isSigned);
        } else {
            pb = emitAddressOf(node.right.get());
        }
        std::string res = vecResultArray(vn, node);
        VariableReference rv(res);
        rv.line = node.line; rv.column = node.column; rv.sourceFile = node.sourceFile;
        ir::Operand pr = emitAddressOf(&rv);
        ir::Inst vb;
        vb.op = ir::Op::VEC_BIN;
        vb.resultType = ir::Type::VOID;
        vb.src1 = pa;
        vb.src2 = pb;
        bool unroll = n * es * 8 <= maxUnrollBytes;
        vb.args = {pr, ir::Operand::imm(direct.at(op), ir::Type::I16), ir::Operand::imm(es, ir::Type::I16),
                   ir::Operand::imm(n, ir::Type::I16), ir::Operand::imm(scalar ? 1 : 0, ir::Type::I16),
                   ir::Operand::imm(unroll ? 1 : 0, ir::Type::I16)};
        vb.loc = loc(node);
        emit(vb);
        vecResultValue(vn, res, node);
        return;
    }
    std::string a = vecOperand(node.left.get(), !vl.empty(), et, sg);
    std::string b = vecOperand(node.right.get(), !vr.empty(), et, sg);
    bool av = !vl.empty(), bv = !vr.empty();
    std::string rn = cmp ? vecMaskType(vn) : vn;
    vecResult(rn, [&](const VecIndex& i) -> std::unique_ptr<Expression> {
        std::unique_ptr<Expression> e = std::make_unique<BinaryOperation>(op, vecRef(a, av, i), vecRef(b, bv, i));
        if (cmp) e = std::make_unique<UnaryOperation>("-", std::move(e));
        return e;
    }, node);
}

// -v, ~v, +v of a vector (!v: each element == 0, as -1/0)
void IRBuilder::emitVectorUnary(UnaryOperation& node) {
    std::string vn = vecOf(node.operand.get());
    std::string et; bool sg; int n;
    vecElem(vn, et, sg, n);
    std::string a = vecOperand(node.operand.get(), true, et, sg);
    std::string op = node.op;
    std::string rn = op == "!" ? vecMaskType(vn) : vn;
    vecResult(rn, [&](const VecIndex& i) -> std::unique_ptr<Expression> {
        if (op == "!")
            return std::make_unique<UnaryOperation>("-", std::make_unique<BinaryOperation>("==", vecRef(a, true, i), std::make_unique<IntegerLiteral>(0)));
        return std::make_unique<UnaryOperation>(op, vecRef(a, true, i));
    }, node);
}

// __builtin_shuffle(a, mask) / (a, b, mask): element i is a[mask[i] % n]
// (two vectors: a then b, indexed modulo 2n)
void IRBuilder::emitVectorShuffle(FunctionCall& node) {
    std::string vn = vecOf(node.arguments[0].get());
    std::string et; bool sg; int n;
    vecElem(vn, et, sg, n);
    bool two = node.arguments.size() == 3;
    Expression* maskE = node.arguments.back().get();
    std::string mn = vecOf(maskE);
    std::string mt; bool msg; int mcount;
    vecElem(mn, mt, msg, mcount);
    std::string a = vecOperand(node.arguments[0].get(), true, et, sg);
    std::string b = two ? vecOperand(node.arguments[1].get(), true, et, sg) : "";
    std::string m = vecOperand(maskE, true, mt, msg);
    vecResult(vn, [&](const VecIndex& i) -> std::unique_ptr<Expression> {
        auto idx = [&](int mod) {
            return std::make_unique<BinaryOperation>("&", vecRef(m, true, i), std::make_unique<IntegerLiteral>(mod - 1));
        };
        if (!two)
            return std::make_unique<ArrayAccess>(std::make_unique<VariableReference>(a), idx(n));
        // (m & (2n-1)) < n ? a[m & (n-1)] : b[m & (n-1)]
        return std::make_unique<ConditionalExpression>(
            std::make_unique<BinaryOperation>("<", idx(2 * n), std::make_unique<IntegerLiteral>(n)),
            std::make_unique<ArrayAccess>(std::make_unique<VariableReference>(a), idx(n)),
            std::make_unique<ArrayAccess>(std::make_unique<VariableReference>(b), idx(n)));
    }, node);
}

void IRBuilder::visit(BinaryOperation& node) {
    // GCC vectors: element by element
    if (node.op != "," && node.op != "&&" && node.op != "||" && currentFunc_ &&
        (!vecOf(node.left.get()).empty() || !vecOf(node.right.get()).empty())) {
        emitVectorBinary(node);
        return;
    }
    // Wide integer with an integer operand: the integer converted
    if (node.op != "," && node.op != "&&" && node.op != "||" && node.op != "<<" && node.op != ">>") {
        std::string wl = wideKindOf(node.left.get()), wr = wideKindOf(node.right.get());
        if (!wl.empty() && wr.empty()) toWide(node.right, wl, false);
        else if (wl.empty() && !wr.empty()) toWide(node.left, wr, false);
    }
    // Complex arithmetic/equality: both operands as the common complex type,
    // then the struct's operator method does the work
    if (node.op == "+" || node.op == "-" || node.op == "*" || node.op == "/" ||
        node.op == "==" || node.op == "!=") {
        std::string kl = complexKindOf(node.left.get()), kr = complexKindOf(node.right.get());
        if (!kl.empty() || !kr.empty()) {
            std::string k = "_Complex_int";
            if (kl == "_Complex_float" || kr == "_Complex_float") k = "_Complex_float";
            else {
                Expression* real = kl.empty() ? node.left.get() : (kr.empty() ? node.right.get() : nullptr);
                if (real && getExprTypeInfo(real).type == ir::Type::F32) k = "_Complex_float";
            }
            toComplex(node.left, k);
            toComplex(node.right, k);
        }
    }

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

        // Both paths write one result vreg, normalized to 0/1
        auto result = allocVreg(ir::Type::I8);
        boolVregs_.insert({currentFunc_, result.vregId});

        // RHS evaluation block (merge)
        startBlock(mergeLabel);
        node.right->accept(*this);
        auto rhs = lastValue_;
        {
            ir::Inst set;
            if (isBooleanValue(rhs)) {
                set.op = ir::Op::COPY;
                set.src1 = rhs;
            } else {
                set.op = ir::Op::CMP_NE;
                set.src1 = rhs;
                set.src2 = ir::Operand::imm(0, rhs.type);
            }
            set.dest = result;
            set.resultType = ir::Type::I8;
            set.loc = loc(node);
            emit(set);
        }

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
        {
            ir::Inst c;
            c.op = ir::Op::CONST;
            c.dest = result;
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

        // if/while conditions don't come through here; emitConditionBranches
        // branches on && / || directly.
        startBlock(doneLabel);
        lastValue_ = result;
        return;
    }

    IRTypeInfo lhsInfo = getExprTypeInfo(node.left.get());
    IRTypeInfo rhsInfo = getExprTypeInfo(node.right.get());

    // `x < 0` / `0 > x` can never be true when x is unsigned (cc45 integers are
    // unsigned unless declared signed)
    {
        auto isZero = [](Expression* e) {
            auto* lit = dynamic_cast<IntegerLiteral*>(e);
            return lit && lit->value == 0;
        };
        auto isUnsignedInt = [](const IRTypeInfo& t) {
            return !t.isSigned && (t.type == ir::Type::I8 || t.type == ir::Type::I16 || t.type == ir::Type::I32);
        };
        bool alwaysFalse = (node.op == "<" && isZero(node.right.get()) && isUnsignedInt(lhsInfo)) ||
                           (node.op == ">" && isZero(node.left.get()) && isUnsignedInt(rhsInfo));
        bool alwaysTrue = (node.op == ">=" && isZero(node.right.get()) && isUnsignedInt(lhsInfo)) ||
                          (node.op == "<=" && isZero(node.left.get()) && isUnsignedInt(rhsInfo));
        if (alwaysFalse || alwaysTrue) {
            // a plain-int variable: ConstantFolder reported it (it explains why)
            Expression* var = isZero(node.right.get()) ? node.left.get() : node.right.get();
            std::string vn;
            std::string what = alwaysFalse ? "< 0 is always false" : ">= 0 is always true";
            std::string key = node.sourceFile + ":" + std::to_string(node.line) + ":" + std::to_string(node.column) + ":lt0";
            if (!plainUnsignedVar(var, vn) && reportedUnused_.insert(key).second)
                warnings_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column, Severity::Warning,
                    "comparison of unsigned expression " + what + " (declare it 'signed' if it can be negative)"));
        }
    }

    // Operator overloading: if LHS is a struct with an operator method, dispatch to it
    // (a wide integer operation's own result too: v + v + k, #329)
    std::string lhsWide = wideKindOf(node.left.get());
    if ((!lhsInfo.typeName.empty() && (lhsInfo.type != ir::Type::PTR || isAggCopyParam(node.left.get()))) ||
        !lhsWide.empty()) {
        std::string sName = lhsWide.empty() ? getAggregateName(lhsInfo.typeName) : lhsWide;
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
                    // Claim the result buffer before evaluating the operands
                    ir::Operand sretBuf, sretAddr;
                    bool mSret = sretSize_.count(mangledName) > 0;
                    if (mSret) sretAddr = sretCallBuffer(mangledName, sretBuf, loc(node));

                    // Compute 'this' pointer (address of LHS)
                    bool savedAddrMode = computeAddressOnly_;
                    computeAddressOnly_ = true;
                    node.left->accept(*this);
                    computeAddressOnly_ = false;
                    // a small struct value (an operator's result) has no address
                    auto thisPtr = addressOfValue(lastValue_, loc(node));

                    // Evaluate RHS (an aggregate operand is passed as a copy)
                    auto rhsVal = evalArgument(node.right.get(), paramAggSize(mangledName, mSret ? 2 : 1), loc(node));
                    // an integer operand widened to the parameter's type, by its
                    // own signedness (v >> c, an unsigned char for operator>>(int):
                    // the stack convention pushed X's stale high byte, #330)
                    {
                        auto pit = functionParamTypes_.find(mangledName);
                        size_t ri = mSret ? 2 : 1;
                        auto isInt = [](ir::Type t) { return t == ir::Type::I8 || t == ir::Type::I16 || t == ir::Type::I32; };
                        if (pit != functionParamTypes_.end() && ri < pit->second.size() && isInt(rhsVal.type) &&
                            isInt(pit->second[ri]) && ir::typeSize(rhsVal.type) < ir::typeSize(pit->second[ri]))
                            rhsVal = emitCast(rhsVal, pit->second[ri], getExprTypeInfo(node.right.get()).isSigned);
                    }

                    // Call StructName__operator_OP(&lhs, rhs)
                    computeAddressOnly_ = savedAddrMode;  // result form follows the caller's mode
                    emitMethodCall(mangledName, {thisPtr, rhsVal}, loc(node),
                                   mSret ? &sretBuf : nullptr, mSret ? &sretAddr : nullptr);
                    return;
                }
                // a struct or union value with no such operator (C has none)
                if (node.op.find('=') == std::string::npos || node.op == "==" || node.op == "!=" ||
                    node.op == "<=" || node.op == ">=") {
                    errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column, Severity::Error,
                        "invalid operands to binary " + node.op + " ('" + lhsInfo.typeName + "' has no operator" +
                        node.op + ")"));
                    lastValue_ = ir::Operand::imm(0, ir::Type::I16);
                    return;
                }
            }
        }
    }

    // Pre-determine if we can keep this operation at I8 (char width).
    // Only when BOTH operands are I8, or one is I8 and the other is a small
    // literal with no explicit wider type. Never narrow a wider operand to I8.
    bool forceI8 = false;
    // A signed and an unsigned char both promote to (signed) int: compared,
    // divided or taken modulo in 16 bits, signed (-1 < 255; in 8 bits they
    // are the same byte)
    bool mixedChars = lhsInfo.type == ir::Type::I8 && rhsInfo.type == ir::Type::I8 &&
                      lhsInfo.isSigned != rhsInfo.isSigned &&
                      (node.op == "<" || node.op == ">" || node.op == "<=" || node.op == ">=" ||
                       node.op == "==" || node.op == "!=" || node.op == "/" || node.op == "%");
    if (mixedChars) {
        // computed in int
    } else if (lhsInfo.type == ir::Type::I8 && rhsInfo.type == ir::Type::I8) {
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
    ir::Type resultType = forceI8 ? ir::Type::I8 : mixedChars ? ir::Type::I16 : lhs.type;
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
        // p + n on a pointer to an array steps over whole arrays
        if (int row = ptrArrayRowSize(lhsInfo.type == ir::Type::PTR ? node.left.get() : node.right.get()))
            elemSize = row;
        if (node.op == "-" && lhsInfo.type == ir::Type::PTR && rhsInfo.type == ir::Type::PTR) {
            // Pointer difference: (a - b) / sizeof(*a), a signed integer
            auto diff = allocVreg(ir::Type::I16);
            ir::Inst sub;
            sub.op = ir::Op::SUB;
            sub.dest = diff;
            sub.resultType = ir::Type::I16;
            sub.src1 = lhsVal;
            sub.src2 = rhsVal;
            sub.loc = loc(node);
            emit(sub);
            lastValue_ = diff;
            if (elemSize > 1) {
                auto q = allocVreg(ir::Type::I16);
                ir::Inst div;
                div.op = ir::Op::DIV;
                div.dest = q;
                div.resultType = ir::Type::I16;
                div.src1 = diff;
                div.src2 = ir::Operand::imm(elemSize, ir::Type::I16);
                div.loc = loc(node);
                emit(div);
                lastValue_ = q;
            }
            return;
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
    bool bothSigned = (lhsInfo.isSigned && rhsInfo.isSigned) || mixedChars;
    if (!bothSigned && (lhsInfo.isSigned || rhsInfo.isSigned)) {
        auto* lhsLit = dynamic_cast<IntegerLiteral*>(node.left.get());
        auto* rhsLit = dynamic_cast<IntegerLiteral*>(node.right.get());
        if (lhsInfo.isSigned && !rhsInfo.isSigned && rhsLit && rhsLit->castType.empty() && rhsLit->value >= 0 && rhsLit->value <= 32767)
            bothSigned = true;
        else if (rhsInfo.isSigned && !lhsInfo.isSigned && lhsLit && lhsLit->castType.empty() && lhsLit->value >= 0 && lhsLit->value <= 32767)
            bothSigned = true;
        // A wider signed type holds every value of a narrower unsigned one:
        // the common type is the signed one (long vs unsigned short; an int
        // vs an unsigned char, which C promotes to a signed int)
        auto intSize = [](ir::Type t) { return (int)ir::typeSize(t); };
        if (!bothSigned && lhsInfo.type != ir::Type::PTR && rhsInfo.type != ir::Type::PTR &&
            lhsInfo.type != ir::Type::F32 && rhsInfo.type != ir::Type::F32) {
            int ls = intSize(lhsInfo.type), rs = intSize(rhsInfo.type);
            if ((lhsInfo.isSigned && ls > rs) || (rhsInfo.isSigned && rs > ls)) bothSigned = true;
        }
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
        else {
            std::string errMsg = "Unsupported operator '" + node.op + "' for float type";
            throw std::runtime_error(formatSemanticError(errMsg, node.sourceFile, node.line, node.column));
        }
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
    // A shift has the (promoted) type of its left operand: the count's
    // signedness doesn't matter
    else if (node.op == ">>") op = lhsInfo.isSigned ? ir::Op::ASR : ir::Op::SHR;
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
    if ((node.op == "-" || node.op == "~" || node.op == "+" || node.op == "!") && currentFunc_ &&
        !vecOf(node.operand.get()).empty()) {
        emitVectorUnary(node);
        return;
    }
    // ++/-- on an lvalue other than a plain variable (*p++, a[i], p->m):
    // its address is computed once (the operand may have side effects),
    // then load / add / store through it
    if (node.op == "++" || node.op == "--" || node.op == "++_POST" || node.op == "--_POST") {
        Expression* opnd = node.operand.get();
        IRTypeInfo info = getExprTypeInfo(opnd);
        bool scalar = info.type == ir::Type::I8 || info.type == ir::Type::I16 || info.type == ir::Type::I32 ||
                      info.type == ir::Type::PTR;
        bool isBitfield = false;
        if (auto* ma = dynamic_cast<MemberAccess*>(opnd)) {
            std::string t = getExprTypeInfo(ma->structExpr.get()).typeName;
            if (t.rfind("struct ", 0) != 0 && t.rfind("union ", 0) != 0) t = resolveTypedefToStruct(t);
            int acc = 0;
            auto* mi = structs_.count(getAggregateName(t)) ? findStructMember(getAggregateName(t), ma->memberName, acc) : nullptr;
            isBitfield = mi && mi->bitWidth > 0;
        }
        bool structValue = info.type != ir::Type::PTR && structOf(info.typeName);
        if (scalar && !isBitfield && !structValue && !dynamic_cast<VariableReference*>(opnd) &&
            (dynamic_cast<UnaryOperation*>(opnd) || dynamic_cast<ArrayAccess*>(opnd) || dynamic_cast<MemberAccess*>(opnd))) {
            // Read-only targets (as checked for the other ++/-- forms below)
            if (auto* deref = dynamic_cast<UnaryOperation*>(opnd); deref && deref->op == "*") {
                if (auto* vr = dynamic_cast<VariableReference*>(deref->operand.get())) {
                    auto pit = localPointsToConst_.find(vr->name);
                    auto ptit = localPointedToType_.find(vr->name);
                    bool isMultiPtr = ptit != localPointedToType_.end() && ptit->second == ir::Type::PTR;
                    if (pit != localPointsToConst_.end() && pit->second && !isMultiPtr) {
                        errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column,
                            Severity::Error, "Compile Error: Increment/decrement of read-only location"));
                        return;
                    }
                }
            }
            if (auto* ma = dynamic_cast<MemberAccess*>(opnd)) {
                IRTypeInfo baseInfo = getExprTypeInfo(ma->structExpr.get());
                int accOffset = 0;
                auto* mit = findStructMember(getAggregateName(baseInfo.typeName), ma->memberName, accOffset);
                if (mit && mit->isConst) {
                    errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column,
                        Severity::Error, "Compile Error: Increment/decrement of read-only member"));
                    return;
                }
            }
            bool isPost = node.op.size() > 2;
            bool isInc = node.op[0] == '+';
            int step = 1;
            if (info.type == ir::Type::PTR) {
                if (info.pointedToType != ir::Type::VOID) step = std::max(1, (int)ir::typeSize(info.pointedToType));
                if (const IRStructInfo* si = structOf(info.typeName)) step = si->totalSize;
            }
            auto addr = emitAddressOf(opnd);
            auto oldV = allocVreg(info.type);
            ir::Inst ld;
            ld.op = ir::Op::LOAD; ld.dest = oldV; ld.resultType = info.type; ld.src1 = addr; ld.loc = loc(node);
            emit(ld);
            auto newV = allocVreg(info.type);
            ir::Inst ar;
            ar.op = isInc ? ir::Op::ADD : ir::Op::SUB; ar.dest = newV; ar.resultType = info.type;
            ar.src1 = oldV; ar.src2 = ir::Operand::imm(step, info.type); ar.loc = loc(node);
            emit(ar);
            ir::Inst st;
            st.op = ir::Op::STORE; st.resultType = info.type; st.src1 = newV; st.src2 = addr; st.loc = loc(node);
            emit(st);
            lastValue_ = isPost ? oldV : newV;
            lastValueSigned_ = info.isSigned;
            return;
        }
    }

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
        if (!srcInfo.typeName.empty() && (srcInfo.type != ir::Type::PTR || isAggCopyParam(node.operand.get()))) {
            std::string sName = getAggregateName(srcInfo.typeName);
            if (structs_.count(sName)) {
                static const std::map<std::string, std::string> unaryOpMap = {
                    {"~", "bnot"}, {"-", "neg"}, {"!", "lnot"}, {"++", "inc"}, {"--", "dec"},
                };
                auto opIt = unaryOpMap.find(node.op);
                if (opIt != unaryOpMap.end()) {
                    std::string mangledName = sName + "__operator_" + opIt->second;
                    if (functionReturnTypes_.count(mangledName)) {
                        ir::Operand sretBuf, sretAddr;
                        bool mSret = sretSize_.count(mangledName) > 0;
                        if (mSret) sretAddr = sretCallBuffer(mangledName, sretBuf, loc(node));
                        bool savedAddrMode = computeAddressOnly_;
                        computeAddressOnly_ = true;
                        node.operand->accept(*this);
                        computeAddressOnly_ = savedAddrMode;
                        auto thisPtr = addressOfValue(lastValue_, loc(node));
                        emitMethodCall(mangledName, {thisPtr}, loc(node),
                                       mSret ? &sretBuf : nullptr, mSret ? &sretAddr : nullptr);
                        return;
                    }
                }
            }
        }
    }

    // Integer promotion: a char-width operand of - or ~ is widened to int
    // first (-(x < y) is -1, not 255)
    if ((node.op == "-" || node.op == "~") && src.type == ir::Type::I8)
        src = emitCast(src, ir::Type::I16, getExprTypeInfo(node.operand.get()).isSigned);

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
        if (auto* vr = dynamic_cast<VariableReference*>(node.operand.get()); vr && vmVars_.count(vr->name) && locals_.count(vr->name)) {
            vr->accept(*this);  // a struct with run-time member sizes: the variable holds the address
            return;
        }
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
            if (it != locals_.end()) usedVregs_.insert(it->second.vregId);  // &x uses x
            if (it != locals_.end() && aggCopyParams_.count(vr->name)) {
                // A by-value struct parameter holds its copy's address
                lastValue_ = it->second;
            } else if (it != locals_.end() && it->second.kind == ir::OperandKind::GLOBAL) {
                // A local static: its global's address
                lastValue_ = it->second;
            } else if (it != locals_.end()) {
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
        // *p of a pointer to an array (T (*p)[M], or an int a[][M]
        // parameter) is that array: its address, the pointer's value
        if (auto* pr = dynamic_cast<VariableReference*>(node.operand.get());
            pr && !computeAddressOnly_ && paramArrayDims_.count(pr->name) && paramArrayDims_[pr->name].size() >= 2) {
            node.operand->accept(*this);
            return;
        }
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

        // Store back to the variable
        // Check if operand is a bitfield member — need BFINS instead of STORE
        bool isBitfield = false;
        int bfWidth = 0, bfOffset = 0, bfMemberOffset = 0;
        ir::Type bfMemberType = ir::Type::I8;
        if (auto* ma = dynamic_cast<MemberAccess*>(node.operand.get())) {
            IRTypeInfo baseInfo = getExprTypeInfo(ma->structExpr.get());
            std::string sName = getAggregateName(baseInfo.typeName);
            int accOffset = 0;
            auto* mit = findStructMember(sName, ma->memberName, accOffset);
            if (mit && mit->bitWidth > 0) {
                isBitfield = true;
                bfWidth = mit->bitWidth;
                bfOffset = mit->bitOffset;
                bfMemberOffset = mit->offset + accOffset;
                bfMemberType = mapType(mit->type, mit->pointerLevel);
            }
        }

        if (isBitfield) {
            // Bitfield increment: use BFINS to write back into storage unit
            auto* ma = dynamic_cast<MemberAccess*>(node.operand.get());
            bool oldAddrMode = computeAddressOnly_;
            computeAddressOnly_ = !ma->isArrow;
            ma->structExpr->accept(*this);
            computeAddressOnly_ = oldAddrMode;
            auto base = lastValue_;

            auto addr = allocVreg(ir::Type::PTR);
            ir::Inst add;
            add.op = ir::Op::ADD;
            add.dest = addr;
            add.resultType = ir::Type::PTR;
            add.src1 = base;
            add.src2 = ir::Operand::imm(bfMemberOffset, ir::Type::I16);
            add.loc = loc(node);
            emit(add);

            ir::Inst bfins;
            bfins.op = ir::Op::BFINS;
            bfins.resultType = bfMemberType;
            bfins.src1 = dest;
            bfins.src2 = addr;
            bfins.args.push_back(ir::Operand::imm(bfOffset, ir::Type::I8));
            bfins.args.push_back(ir::Operand::imm(bfWidth, ir::Type::I8));
            bfins.loc = loc(node);
            emit(bfins);
        } else {
            ir::Operand addr;
            if (auto* vr = dynamic_cast<VariableReference*>(node.operand.get())) {
                auto lit = locals_.find(vr->name);
                bool uplevel = false;
                if (lit == locals_.end())
                    for (auto& scope : functionStack_) if (scope.locals.count(vr->name)) uplevel = true;
                if (lit != locals_.end()) {
                    addr = lit->second; // local vreg
                } else if (uplevel) {
                    // A variable of an enclosing function (nested function):
                    // its address in that function's frame
                    addr = emitAddressOf(vr);
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
        }

        lastValue_ = isPost ? oldVal : dest;
    }
}

void IRBuilder::visit(CastExpression& node) {
    // (integer)wide: the wide integer's low bytes
    if (node.pointerLevel == 0 && !wideKindOf(node.expression.get()).empty()) {
        ir::Type t = mapType(node.targetType, 0);
        if (t == ir::Type::I8 || t == ir::Type::I16 || t == ir::Type::I32) {
            ir::Operand addr = emitAddressOf(node.expression.get());
            auto v = allocVreg(t);
            ir::Inst ld;
            ld.op = ir::Op::LOAD;
            ld.dest = v;
            ld.resultType = t;
            ld.src1 = addr;
            ld.loc = loc(node);
            emit(ld);
            lastValue_ = v;
            lastValueSigned_ = node.isSigned;
            return;
        }
    }
    // A vector cast to another vector type of the same size: the same bits
    if (node.pointerLevel == 0 && !vecOfType(node.targetType, 0).empty() && !vecOf(node.expression.get()).empty()) {
        node.expression->accept(*this);
        return;
    }
    node.expression->accept(*this);
    auto src = lastValue_;
    ir::Type destType = mapType(node.targetType, node.pointerLevel);

    // Use emitCast which handles float↔int conversions (FTOI/ITOF). An
    // int -> float conversion depends on the source's signedness, not the
    // target's: (float)(signed long)x
    // A widening integer cast also extends by the source's signedness
    // ((unsigned short)(signed char)-31 is 0xFFE1)
    bool sig = node.isSigned;
    if ((destType == ir::Type::F32 && src.type != ir::Type::F32) ||
        (destType != ir::Type::F32 && src.type != ir::Type::F32 && src.type != ir::Type::PTR &&
         ir::typeSize(destType) > ir::typeSize(src.type) && ir::typeSize(src.type) > 0))
        sig = getExprTypeInfo(node.expression.get()).isSigned;
    lastValue_ = emitCast(src, destType, sig);
}

// The signature of the function pointer a call goes through (a variable, an
// array element, a struct member; (*fp)(...) too), or null for a direct call
// or an unknown one; calleeName is the function or variable called
std::shared_ptr<FuncPtrSignature> IRBuilder::callSignature(FunctionCall& node, std::string& calleeName) {
    calleeName = node.name;
    Expression* e = node.callExpr.get();
    while (auto* u = dynamic_cast<UnaryOperation*>(e)) {
        if (u->op != "*") break;
        e = u->operand.get();
    }
    if (auto* vr = dynamic_cast<VariableReference*>(e)) calleeName = vr->name;
    if (!functionReturnTypes_.count(calleeName) && !definedFunctions_.count("_" + calleeName) &&
        (locals_.count(calleeName) || globalTypes_.count(calleeName))) {
        auto fit = fnPtrSigs_.find(calleeName);
        if (fit != fnPtrSigs_.end()) return fit->second;
    }
    // a struct member (s.f(...), p->f(...)): the member's signature
    if (auto* ma = dynamic_cast<MemberAccess*>(e))
        if (const IRMemberInfo* mi = arrayMemberOf(ma)) return mi->funcPtrSig;
    // an element of an array of function pointers (t[i](...))
    if (auto* aa = dynamic_cast<ArrayAccess*>(e)) {
        Expression* b = aa->arrayExpr.get();
        while (auto* inner = dynamic_cast<ArrayAccess*>(b)) b = inner->arrayExpr.get();
        if (auto* vr = dynamic_cast<VariableReference*>(b)) {
            auto fit = fnPtrSigs_.find(vr->name);
            if (fit != fnPtrSigs_.end()) return fit->second;
        }
    }
    return nullptr;
}

void IRBuilder::visit(FunctionCall& node) {
    noteFormatNeeds(node);
    if (node.name == "__builtin_shuffle" && (node.arguments.size() == 2 || node.arguments.size() == 3) && currentFunc_ &&
        !vecOf(node.arguments[0].get()).empty()) {
        emitVectorShuffle(node);
        return;
    }
    std::string declaredName = node.name;   // (allFunctions_ knows a static function by it)
    if (!node.callExpr && !locals_.count(node.name)) {
        auto rn = staticRenames_.find(node.name);
        if (rn != staticRenames_.end()) node.name = rn->second;
    }
    // An integer argument for a wide integer parameter (__int(N)): converted
    // (a static function's too, and after a struct result's hidden pointer:
    // foo(0) for a u128 parameter copied 16 bytes from address 0)
    if (!node.callExpr) {
        auto fit = allFunctions_.find(declaredName);
        if (fit == allFunctions_.end()) fit = allFunctions_.find(node.name);
        if (fit != allFunctions_.end()) {
            const auto& params = fit->second->parameters;
            // (a struct result's hidden pointer is not an argument here)
            size_t skip = !params.empty() && params[0].name == "__sret" ? 1 : 0;
            for (size_t i = 0; i < node.arguments.size() && i + skip < params.size(); i++) {
                const auto& par = params[i + skip];
                if (par.pointerLevel) continue;
                std::string wn = getAggregateName(par.type);
                if (wn.rfind("struct ", 0) == 0) wn = wn.substr(7);
                if (wn.rfind("__int", 0) == 0 && structs_.count(wn)) toWide(node.arguments[i], wn, false);
            }
        }
    }
    // Struct method call: p.method(args) or ptr->method(args)
    // callExpr is MemberAccess, check if member is a known method
    if (node.callExpr) {
        if (auto* ma = dynamic_cast<MemberAccess*>(node.callExpr.get())) {
            // Look up struct type of the LHS
            IRTypeInfo baseInfo = getExprTypeInfo(ma->structExpr.get());
            std::string sName = getAggregateName(baseInfo.typeName);
            // Check if this member is a method (mangled name exists as a function)
            std::string mangledName = sName + "__" + ma->memberName;
            // A method of a base struct (struct D : B): B's, with the same
            // 'this' (the base's members come first)
            for (std::string ps = sName; !functionReturnTypes_.count(mangledName);) {
                auto sit = structs_.find(ps);
                if (sit == structs_.end() || sit->second.parent.empty()) break;
                ps = sit->second.parent;
                if (functionReturnTypes_.count(ps + "__" + ma->memberName)) {
                    sName = ps;
                    mangledName = ps + "__" + ma->memberName;
                }
            }
            std::string irName = "_" + mangledName;
            if (functionReturnTypes_.count(mangledName)) {
                // Struct/float result: hidden result pointer goes first (claim the
                // caller's destination before anything else is evaluated)
                ir::Operand sretBuf, sretAddr;
                bool mSret = sretSize_.count(mangledName) > 0;
                if (mSret) sretAddr = sretCallBuffer(mangledName, sretBuf, loc(node));

                // Compute 'this' pointer
                bool oldAddrMode = computeAddressOnly_;
                if (!ma->isArrow) {
                    computeAddressOnly_ = true;
                }
                ma->structExpr->accept(*this);
                computeAddressOnly_ = oldAddrMode;
                auto thisPtr = lastValue_;
                // A global object (g.get()): its operand names the variable,
                // whose value a small struct would pass; 'this' is its address
                if (!ma->isArrow && thisPtr.kind == ir::OperandKind::GLOBAL) {
                    auto addr = allocVreg(ir::Type::PTR);
                    ir::Inst add;
                    add.op = ir::Op::ADD;
                    add.dest = addr;
                    add.resultType = ir::Type::PTR;
                    add.src1 = thisPtr;
                    add.src2 = ir::Operand::imm(0, ir::Type::I16);
                    add.loc = loc(node);
                    emit(add);
                    thisPtr = addr;
                }

                // Build argument list with 'this' prepended
                std::vector<ir::Operand> args;
                if (mSret) args.push_back(sretAddr);
                args.push_back(thisPtr);
                for (auto& arg : node.arguments)
                    args.push_back(evalArgument(arg.get(), paramAggSize(mangledName, args.size()), loc(node)));

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
                inst.callConv = usesZpConv(mangledName) ? ir::CallConv::ZP : ir::CallConv::STACK;

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
                    if (inlineIt != inlineCandidates_.end() && inlineExpansionStack_.count(mangledName) &&
                        alwaysInlineNames_.count(mangledName))
                        warnRecursiveForcedInline(mangledName, node);
                    if (!mSret && inlineIt != inlineCandidates_.end() &&
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
                        auto savedInlineReturnTarget = inlineReturnTarget_;
                        auto savedInlineReturnLabel = inlineReturnLabel_;
                        inlineReturnTarget_ = inlineResult;
                        inlineReturnLabel_ = newLabel("inline_ret");
                        auto savedInlineSourceFunc = inlineSourceFunc_;
                        inlineSourceFunc_ = mangledName;

                        size_t savedLoopBase = loopBase_;
                        loopBase_ = loopStack_.size();
                        if (inlineFunc->body) inlineFunc->body->accept(*this);
                        loopBase_ = savedLoopBase;

                        // Emit fall-through branch to merge label for implicit return
                        // (explicit returns already branch via visit(ReturnStatement))
                        {
                            bool needBr = true;
                            if (currentBlock_ && !currentBlock_->insts.empty()) {
                                auto lastOp = currentBlock_->insts.back().op;
                                if (lastOp == ir::Op::BR || lastOp == ir::Op::RET || lastOp == ir::Op::RET_VOID)
                                    needBr = false;
                            }
                            if (needBr) {
                                ir::Inst br;
                                br.op = ir::Op::BR;
                                br.src1 = ir::Operand::label(inlineReturnLabel_);
                                emit(br);
                            }
                        }

                        startBlock(inlineReturnLabel_);
                        lastValue_ = inlineResult;
                        inlineReturnTarget_ = savedInlineReturnTarget;
                        inlineReturnLabel_ = savedInlineReturnLabel;
                        inlineSourceFunc_ = savedInlineSourceFunc;

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

                if (mSret) {
                    if (inst.op == ir::Op::CALL) inst.op = ir::Op::CALL_VOID;
                    inst.resultType = ir::Type::VOID;
                    emit(inst);
                    sretCallResult(mangledName, sretBuf, sretAddr, loc(node));
                    return;
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

    // __builtin_{add,sub,mul}_overflow(a, b, &r), the _p forms (the third
    // operand only gives the type) and the typed __builtin_[su]op[l|ll]_overflow:
    // a call of __overflow_<op> (lib/stdlib/__builtin_overflow.c) with the operands
    // widened to 32 bits and the result type's size and signedness
    if (node.name.rfind("__builtin_", 0) == 0 && node.arguments.size() == 3 &&
        node.name.size() > 19 && (node.name.compare(node.name.size() - 9, 9, "_overflow") == 0 ||
                                  node.name.compare(node.name.size() - 11, 11, "_overflow_p") == 0)) {
        std::string core = node.name.substr(10);          // e.g. "sadd_overflow"
        bool isP = core.size() > 2 && core.compare(core.size() - 2, 2, "_p") == 0;
        if (isP) core.resize(core.size() - 2);
        core.resize(core.size() - 9);                       // drop "_overflow"
        int typed = 0;                                      // 1 signed, 2 unsigned
        int typedSize = 2;
        if (!isP && (core[0] == 's' || core[0] == 'u') && core.size() >= 4 &&
            (core.compare(1, 3, "add") == 0 || core.compare(1, 3, "sub") == 0 || core.compare(1, 3, "mul") == 0)) {
            typed = core[0] == 's' ? 1 : 2;
            std::string sfx = core.substr(4);
            if (sfx == "l" || sfx == "ll") typedSize = 4;
            else if (!sfx.empty()) typed = -1;
            core = core.substr(1, 3);
        }
        int op = core == "add" ? 0 : core == "sub" ? 1 : core == "mul" ? 2 : -1;
        if (op >= 0 && typed >= 0) {
            // The result type
            int rsize; bool rsigned;
            if (typed) {
                rsize = typedSize; rsigned = typed == 1;
            } else if (isP) {
                IRTypeInfo ri = getExprTypeInfo(node.arguments[2].get());
                rsize = ir::typeSize(ri.type); rsigned = ri.isSigned;
            } else if (auto* ad = dynamic_cast<UnaryOperation*>(node.arguments[2].get()); ad && ad->op == "&") {
                IRTypeInfo ri = getExprTypeInfo(ad->operand.get());
                rsize = ir::typeSize(ri.type); rsigned = ri.isSigned;
            } else {
                auto deref = std::make_unique<UnaryOperation>("*", std::move(node.arguments[2]));
                IRTypeInfo ri = getExprTypeInfo(deref.get());
                node.arguments[2] = std::move(deref->operand);
                rsize = ir::typeSize(ri.type); rsigned = ri.isSigned;
            }
            if (rsize != 1 && rsize != 2 && rsize != 4) rsize = 4;
            // Operands: converted to the builtin's type (typed forms), then
            // widened to long by their own signedness
            bool sg[2];
            std::vector<std::unique_ptr<Expression>> wide;
            for (int k = 0; k < 2; k++) {
                std::unique_ptr<Expression> e = std::move(node.arguments[k]);
                if (typed) {
                    sg[k] = typed == 1;
                    e = std::make_unique<CastExpression>(typedSize == 4 ? "long" : "int", 0, sg[k], std::move(e));
                } else {
                    IRTypeInfo ti = getExprTypeInfo(e.get());
                    sg[k] = ti.isSigned && ti.type != ir::Type::PTR;
                }
                wide.push_back(std::make_unique<CastExpression>("long", 0, sg[k], std::move(e)));
            }
            unsigned flags = (unsigned)op | (sg[0] ? 4u : 0u) | (sg[1] ? 8u : 0u) |
                             (rsigned ? 16u : 0u) | ((unsigned)rsize << 8);
            FunctionCall call(op == 0 ? "__overflow_add" : op == 1 ? "__overflow_sub" : "__overflow_mul");
            call.line = node.line; call.column = node.column; call.sourceFile = node.sourceFile;
            call.arguments.push_back(std::move(wide[0]));
            call.arguments.push_back(std::move(wide[1]));
            if (isP) {
                // Not evaluated: no result is stored
                call.arguments.push_back(std::make_unique<IntegerLiteral>(0));
            } else {
                call.arguments.push_back(std::make_unique<CastExpression>("char", 1, false, std::move(node.arguments[2])));
            }
            call.arguments.push_back(std::make_unique<IntegerLiteral>((int64_t)flags));
            call.accept(*this);
            // Give the operands back to the original node
            for (int k = 0; k < 2; k++) {
                auto* c = static_cast<CastExpression*>(call.arguments[k].get());
                std::unique_ptr<Expression> e = std::move(c->expression);
                if (typed) e = std::move(static_cast<CastExpression*>(e.get())->expression);
                node.arguments[k] = std::move(e);
            }
            if (!isP) node.arguments[2] = std::move(static_cast<CastExpression*>(call.arguments[2].get())->expression);
            return;
        }
    }

    // __builtin_return_address(0): the caller's return address (the address
    // after its JSR), captured at function entry. Outer frames give 0
    if (node.name == "__builtin_return_address" && node.arguments.size() == 1 && currentFunc_) {
        auto* lit = dynamic_cast<IntegerLiteral*>(node.arguments[0].get());
        if (lit && lit->value == 0 && !currentFunc_->isRegparm && !currentFunc_->isNaked) {
            if (currentFunc_->retAddrSlot < 0) {
                auto slot = allocVreg(ir::Type::I16);
                currentFunc_->localSlotVregs.insert(slot.vregId);
                currentFunc_->retAddrSlot = slot.vregId;
                usedVregs_.insert(slot.vregId);
            }
            lastValue_ = ir::Operand::vreg(currentFunc_->retAddrSlot, ir::Type::I16);
        } else {
            lastValue_ = ir::Operand::imm(0, ir::Type::I16);
        }
        return;
    }

    // alloca / __builtin_alloca intrinsic: allocate frame buffer, return address
    if ((node.name == "alloca" || node.name == "__builtin_alloca") && node.arguments.size() == 1) {
        // Check for constant size before evaluating (avoid emitting a CONST vreg)
        int constSize = -1;
        if (auto* lit = dynamic_cast<IntegerLiteral*>(node.arguments[0].get())) {
            constSize = (int)lit->value;
        } else if (auto* cast = dynamic_cast<CastExpression*>(node.arguments[0].get())) {
            if (auto* lit2 = dynamic_cast<IntegerLiteral*>(cast->expression.get())) {
                constSize = (int)lit2->value;
            }
        }

        if (constSize > 0) {
            // Constant size: allocate a frame buffer (same as local array)
            auto buf = allocVreg(ir::Type::I16);
            if (currentFunc_) {
                currentFunc_->memoryVregs.insert(buf.vregId);
                currentFunc_->vregSizes[buf.vregId] = constSize;
                currentFunc_->vregAlign[buf.vregId] = 2;  // alloca: the widest natural alignment
            }
            // Return address of the buffer
            ir::Inst addr;
            addr.op = ir::Op::ADDR_LOCAL;
            addr.src1 = buf;
            addr.dest = allocVreg(ir::Type::PTR);
            addr.resultType = ir::Type::PTR;
            addr.loc = loc(node);
            emit(addr);
            lastValue_ = addr.dest;
        } else {
            // Variable size: fall through to runtime alloca call
            // (runtime alloca.s45 handles this via stack manipulation)
            goto normal_call;
        }
        return;
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
        // Write bank and MSB first, LSB last ($D700 write triggers DMA)
        emitAsm("lda #0");
        emitAsm("sta $D702");                    // DMA list bank = 0 (STZ would store Z)
        // Job address = SP+1, 16 bits (the stack may be on any page)
        emitAsm("tsx");
        emitAsm("tsy");
        emitAsm("txa");
        emitAsm("clc");
        emitAsm("adc #1");
        emitAsm("tax");
        emitAsm("tya");
        emitAsm("adc #0");
        emitAsm("sta $D701");                    // DMA addr MSB
        emitAsm("stx $D700");                    // DMA addr LSB — triggers DMA
        // Drop the 12-byte job, then restore A
        for (int i = 0; i < 12; i++) emitAsm("pla");
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
        emitAsm("lda #0");    emitAsm("pha");    // src addr hi (unused by fill)
        emitAsm("lda $02");   emitAsm("pha");    // fill value (src addr lo)
        emitAsm("lda $07");   emitAsm("pha");    // length hi
        emitAsm("lda $06");   emitAsm("pha");    // length lo
        emitAsm("lda #$03");  emitAsm("pha");    // command: fill=0x03
        // Trigger DMA — write bank and MSB first, LSB last ($D700 triggers)
        emitAsm("lda #0");
        emitAsm("sta $D702");                    // DMA list bank = 0 (STZ would store Z)
        // Job address = SP+1, 16 bits (the stack may be on any page)
        emitAsm("tsx");
        emitAsm("tsy");
        emitAsm("txa");
        emitAsm("clc");
        emitAsm("adc #1");
        emitAsm("tax");
        emitAsm("tya");
        emitAsm("adc #0");
        emitAsm("sta $D701");                    // DMA addr MSB
        emitAsm("stx $D700");                    // DMA addr LSB — triggers DMA
        // Drop the 12-byte job, then restore A
        for (int i = 0; i < 12; i++) emitAsm("pla");
        emitAsm("pla");                          // restore A
        return;
    }

normal_call:
    // Check for passing &const_var to non-const pointer parameter
    auto pit = funcParamInfo_.find(node.name);
    auto fitConst = allFunctions_.find(node.name);
    if (pit != funcParamInfo_.end() && fitConst != allFunctions_.end()) {
        FunctionDeclaration* funcDecl = fitConst->second;
        for (size_t i = 0; i < node.arguments.size() && i < pit->second.size() && i < funcDecl->parameters.size(); i++) {
            const auto& pinfo = pit->second[i];
            const auto& param = funcDecl->parameters[i];
            // If param is a pointer and argument is &const_var
            if (pinfo.pointerLevel > 0) {
                // Check if the parameter type is const (e.g., "const int *p")
                bool paramTypeIsConst = param.type.find("const") != std::string::npos;
                // Check if parameter pointer itself is const (e.g., "int * const p")
                bool paramPtrIsConst = pinfo.isConst;

                // Warn if parameter does NOT point to const (it's just "int *p")
                if (!paramTypeIsConst && !paramPtrIsConst) {
                    if (auto* addr = dynamic_cast<UnaryOperation*>(node.arguments[i].get())) {
                        if (addr->op == "&") {
                            if (auto* vr = dynamic_cast<VariableReference*>(addr->operand.get())) {
                                auto cit = localConst_.find(vr->name);
                                if (cit != localConst_.end() && cit->second) {
                                    emitWarning(formatDiagnostic(node.sourceFile, node.line, node.column,
                                        Severity::Warning, "passing argument discards 'const' qualifier"));
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Evaluate arguments
    std::vector<ir::Operand> args;

    // For struct-returning functions: just call normally, struct returned in AXYZ
    // No hidden parameters needed - simpler and cleaner
    ir::Operand structDest;
    if (structReturningFunctions_.count(node.name) > 0) {
        // Get return type to determine size
        ir::Type retType = ir::Type::I16;
        auto it = functionReturnTypes_.find(node.name);
        if (it != functionReturnTypes_.end()) retType = it->second;

        // Allocate a vreg for the struct destination
        structDest = allocVreg(retType);
        if (currentFunc_) {
            currentFunc_->memoryVregs.insert(structDest.vregId);
        }
    }

    // Struct/union/float result: pass the result buffer as the hidden first argument
    ir::Operand sretBuf, sretAddr;
    // Callee name; for a call expression such as (*fp)(...), the function or
    // function-pointer variable it dereferences
    std::string calleeName;
    std::shared_ptr<FuncPtrSignature> fpSig = callSignature(node, calleeName);
    std::string sretKey = calleeName;
    if (fpSig && fpSig->returnPointerLevel == 0) {
        // Call through a function pointer whose target returns a struct/union/float
        bool retFloat = mapType(fpSig->returnType, 0) == ir::Type::F32;
        const IRStructInfo* retStruct = structOf(fpSig->returnType);  // also typedef names
        bool retAggregate = retStruct != nullptr;
        if (retFloat || retAggregate) {
            sretKey = "(*)" + calleeName;
            sretSize_[sretKey] = retFloat ? 5 : retStruct->totalSize;
            if (retFloat) sretFloat_.insert(sretKey);
        }
    }
    bool isSret = sretSize_.count(sretKey) > 0;
    if (isSret) {
        sretAddr = sretCallBuffer(sretKey, sretBuf, loc(node));
        args.push_back(sretAddr);
    }

    for (auto& arg : node.arguments) {
        int agg = 0;
        if (fpSig) {
            size_t pi = args.size() - (isSret ? 1 : 0);
            if (pi < fpSig->params.size())
                agg = aggregateBlockSize(fpSig->params[pi].type, fpSig->params[pi].pointerLevel);
        } else {
            agg = paramAggSize(calleeName, args.size());
        }
        // A struct/union passed through '...' (or without a prototype) goes
        // as the address of a copy, like a by-value parameter
        size_t named = functionParamTypes_.count(calleeName) ? functionParamTypes_[calleeName].size() : 0;
        if (agg == 0 && args.size() >= named) {
            IRTypeInfo ai = getExprTypeInfo(arg.get());
            if (ai.type != ir::Type::PTR || isAggCopyParam(arg.get()))
                agg = aggregateBlockSize(ai.typeName, 0);
        }
        args.push_back(evalArgument(arg.get(), agg, loc(node)));
    }

    ir::Inst inst;
    inst.args = args;
    inst.loc = loc(node);

    // Calling convention: the callee's (see usesZpConv). A call without a
    // prototype applies the rule to the arguments passed.
    bool calleeZp;
    if (fpSig) calleeZp = usesZpConvSig(*fpSig);
    else if (functionParamTypes_.count(calleeName) || variadicFunctions_.count(calleeName)) calleeZp = usesZpConv(calleeName);
    else {
        std::vector<ir::Type> at;
        for (auto& a : args) at.push_back(a.type);
        bool libZp = isLibraryZpRoutine(calleeName) && !unitFunctionNames_.count(calleeName);
        calleeZp = (zpCallMode || libZp) && zpParamBytes(at) <= ZP_PARAM_BYTES;
    }
    inst.callConv = calleeZp ? ir::CallConv::ZP : ir::CallConv::STACK;
    inst.isRegparm = regparmFunctions_.count(node.name) > 0;

    std::vector<ir::Operand> castArgs;
    std::vector<ir::Type> fpTypes;
    std::vector<bool> fpSigned;
    if (fpSig) {
        // Through a function pointer: convert to its prototype's parameter
        // types (an aggregate passes its copy's address as is)
        if (isSret) { fpTypes.push_back(ir::Type::PTR); fpSigned.push_back(false); }
        for (auto& p : fpSig->params) {
            if (p.type == "void" && p.pointerLevel == 0 && fpSig->params.size() == 1) break;
            bool agg = aggregateBlockSize(p.type, p.pointerLevel) > 0;
            fpTypes.push_back(agg ? ir::Type::PTR : mapType(p.type, p.pointerLevel));
            fpSigned.push_back(p.isSigned);
        }
    }
    if (!fpSig && (node.name == "vscanf" || node.name == "vfscanf" || node.name == "vsscanf"))
        module_.callsVscanf = true;
    auto const& pTypes = fpSig ? fpTypes : functionParamTypes_[node.name];
    auto const& pSigned = fpSig ? fpSigned : functionParamSigned_[node.name];

    for (size_t i = 0; i < args.size(); i++) {
        if (i < pTypes.size()) {
            bool sig = i < pSigned.size() && pSigned[i];
            size_t ai = i - (isSret ? 1 : 0);
            // int -> float converts by the argument's signedness
            if (pTypes[i] == ir::Type::F32 && args[i].type != ir::Type::F32 && (!isSret || i > 0) &&
                ai < node.arguments.size())
                sig = getExprTypeInfo(node.arguments[ai].get()).isSigned;
            // a narrower integer widens by its own signedness (an unsigned
            // char 200 passed to a signed int is 200, not -56); a constant
            // keeps the parameter's (f(-1) to a signed long is -1)
            else if ((!isSret || i > 0) && ai < node.arguments.size()) {
                auto isInt = [](ir::Type t) { return t == ir::Type::I8 || t == ir::Type::I16 || t == ir::Type::I32; };
                Expression* a = node.arguments[ai].get();
                bool negated = false;
                while (auto* u = dynamic_cast<UnaryOperation*>(a)) {
                    if (u->op != "-" && u->op != "+" && u->op != "~") break;
                    negated = negated || u->op == "-" || u->op == "~";
                    a = u->operand.get();
                }
                if (auto* lit = dynamic_cast<IntegerLiteral*>(a); lit && lit->value < 0) negated = true;
                bool widens = isInt(args[i].type) && isInt(pTypes[i]) && ir::typeSize(args[i].type) < ir::typeSize(pTypes[i]);
                if (widens && !dynamic_cast<IntegerLiteral*>(a))
                    sig = getExprTypeInfo(node.arguments[ai].get()).isSigned;
                // (a negated constant of a signed type, -fsigned-default's -1,
                // to an unsigned long is all ones, not 0x0000FFFF)
                else if (widens && negated && getExprTypeInfo(node.arguments[ai].get()).isSigned)
                    sig = true;
            }
            castArgs.push_back(emitCast(args[i], pTypes[i], sig));
        } else {
            // The variadic arguments of a function declared with "...": the
            // default argument promotions -- a char/_Bool becomes an int,
            // zero- or sign-extended by its own type (the callee reads
            // va_arg(ap, int); pushing only A left X's stale high byte).
            // (Unprototyped calls are left as they are: a callee defined
            // with char parameters packs them in one byte each.)
            size_t ai = i - (isSret ? 1 : 0);
            if (args[i].type == ir::Type::F32) {
                module_.floatVarArg = true;
                if (!fpSig && (node.name == "sprintf" || node.name == "_sprintf"))
                    module_.floatSprintfArg = true;
            }
            // scanf reading a float: link the %f-aware scanf family
            if (!fpSig && args[i].type == ir::Type::PTR && ai < node.arguments.size() &&
                (node.name == "scanf" || node.name == "fscanf" || node.name == "sscanf") &&
                getExprTypeInfo(node.arguments[ai].get()).pointedToType == ir::Type::F32)
                module_.floatScanfArg = true;
            if (args[i].type == ir::Type::I8 && (fpSig ? fpSig->isVariadic : variadicFunctions_.count(node.name) > 0) &&
                ai < node.arguments.size()) {
                bool sig = getExprTypeInfo(node.arguments[ai].get()).isSigned;
                castArgs.push_back(emitCast(args[i], ir::Type::I16, sig));
            } else {
                castArgs.push_back(args[i]);
            }
        }
    }
    // Handle nested function static link
    auto fitNested = allFunctions_.find(node.name);
    if (fitNested != allFunctions_.end() && fitNested->second->isNested) {
        FunctionDeclaration* calleeParent = fitNested->second->parentFunc;
        ir::Operand sl;

        if (currentFunc_ && "_" + calleeParent->name == currentFunc_->name) {
            // Case 1: Calling a direct child. SL = current FP.
            sl = allocVreg(ir::Type::PTR);
            markStaticLinkProvider();
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
                    addUplevelChain(addrUp, levels);
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
        // Indirect call via function pointer. *fp designates the function fp
        // points to (its value is the same address): (*fp)(...) == fp(...)
        Expression* target = node.callExpr.get();
        while (auto* u = dynamic_cast<UnaryOperation*>(target)) {
            if (u->op != "*") break;
            auto* vr = dynamic_cast<VariableReference*>(u->operand.get());
            if (!vr || !(fnPtrSigs_.count(vr->name) || allFunctions_.count(vr->name) ||
                         functionReturnTypes_.count(vr->name))) break;
            target = u->operand.get();
        }
        target->accept(*this);
        inst.src1 = lastValue_;
        inst.op = ir::Op::CALL_INDIRECT;
        inst.callSig = fpSigKey(fpSig.get());
        if (isSret) {
            inst.resultType = ir::Type::VOID;
            emit(inst);
            sretCallResult(sretKey, sretBuf, sretAddr, loc(node));
        } else {
            // Result type from the pointer's signature or the function (int when unknown)
            ir::Type rt = ir::Type::I16;
            bool rs = true;
            if (fpSig) { rt = mapType(fpSig->returnType, fpSig->returnPointerLevel); rs = fpSig->returnIsSigned; }
            else if (functionReturnTypes_.count(calleeName)) { rt = functionReturnTypes_[calleeName]; rs = functionReturnSigned_[calleeName]; }
            if (rt == ir::Type::VOID) {
                inst.resultType = ir::Type::VOID;
                emit(inst);
            } else {
                auto dest = allocVreg(rt);
                inst.dest = dest;
                inst.resultType = rt;
                emit(inst);
                lastValue_ = dest;
            }
            lastValueSigned_ = rs;
        }
    } else {

        // Check for inline expansion
        auto inlineIt = inlineCandidates_.find(node.name);
        if (inlineIt != inlineCandidates_.end() && inlineExpansionStack_.count(node.name) &&
            alwaysInlineNames_.count(node.name))
            warnRecursiveForcedInline(node.name, node);
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
            auto savedLocalRegister = localRegister_;
            auto savedLocalPointedToTypeName = localPointedToTypeName_;
            auto savedLocalConstPtrValue = localConstPtrValue_;
            auto savedLocalArrayDims = localArrayDims_;
            auto savedLocalPtrLevel = localPtrLevel_;
            auto savedParamArrayDims = paramArrayDims_;
            auto savedParamVlaDims = paramVlaDims_;
            // The inlined body sees its own parameters and locals and the
            // globals, never the caller's locals (a caller's array `C` must
            // not change what the callee's pointer parameter `C` means, and a
            // caller's local must not hide a global the callee uses). A nested
            // function is inlined into its parent, whose locals it does use.
            if (!inlineFunc->isNested) {
            locals_.clear();
            localTypes_.clear();
            localSigned_.clear();
            localConst_.clear();
            localTypeNames_.clear();
            localPointsToConst_.clear();
            localPointedToType_.clear();
            localRegister_.clear();
            localPointedToTypeName_.clear();
            localConstPtrValue_.clear();
            localArrayDims_.clear();
            localPtrLevel_.clear();
            paramArrayDims_.clear();
            paramVlaDims_.clear();
            }

            // Map parameters to argument vregs
            for (size_t i = 0; i < inlineFunc->parameters.size() && i < args.size(); i++) {
                const auto& p = inlineFunc->parameters[i];
                ir::Type pt = mapType(p.type, p.pointerLevel);
                auto argOp = (i < castArgs.size()) ? castArgs[i] : args[i];

                // Create a local vreg for the parameter and copy the arg into it.
                // A parameter the body assigns (n = n - 1, n++) is a local
                // variable: a slot (a STORE to a slot assigns it; to a plain
                // temp it would be a store through a pointer)
                // A struct or union passed by value (in a 4-byte integer) is
                // read through its address (d.i): a slot in memory too
                bool byValueAggregate = p.pointerLevel == 0 && structOf(p.type);
                auto paramVreg = allocVreg(pt);
                if (byValueAggregate || inlineParamAssigned(inlineFunc->body.get(), p.name)) {
                    currentFunc_->localSlotVregs.insert(paramVreg.vregId);
                    if (p.isVolatile || byValueAggregate) currentFunc_->memoryVregs.insert(paramVreg.vregId);
                    ir::Inst st;
                    st.op = ir::Op::STORE;
                    st.resultType = pt;
                    st.src1 = argOp;
                    st.src2 = paramVreg;
                    st.loc = loc(node);
                    emit(st);
                } else {
                    ir::Inst copy;
                    copy.op = ir::Op::COPY;
                    copy.dest = paramVreg;
                    copy.src1 = argOp;
                    copy.resultType = pt;
                    copy.loc = loc(node);
                    emit(copy);
                }

                locals_[p.name] = paramVreg;
                localTypes_[p.name] = pt;
                localTypeNames_[p.name] = p.type;
                localPtrLevel_[p.name] = p.pointerLevel;
                if (!p.innerArrayDims.empty()) {
                    std::vector<int> d{0};
                    d.insert(d.end(), p.innerArrayDims.begin(), p.innerArrayDims.end());
                    paramArrayDims_[p.name] = d;
                    localPtrLevel_[p.name] = p.pointerLevel - 1;
                } else {
                    paramArrayDims_.erase(p.name);
                }
                localSigned_[p.name] = p.isSigned;
                localConst_[p.name] = (p.pointerLevel == 0 && p.isConst);
                if (p.pointerLevel > 0) {
                    if (p.pointerLevel > 1) {
                        localPointedToType_[p.name] = ir::Type::PTR;
                        localPointedToTypeName_[p.name] = p.type + "*";
                    } else {
                        localPointedToType_[p.name] = mapType(p.type, 0);
                        localPointedToTypeName_[p.name] = p.type;
                    }
                }
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
            auto savedInlineSourceFunc = inlineSourceFunc_;
            inlineReturnTarget_ = resultVreg;
            inlineReturnLabel_ = mergeLabel;
            inlineSourceFunc_ = node.name;

            // Visit the inlined function body
            size_t savedLoopBase = loopBase_;
            loopBase_ = loopStack_.size();
            inlineFunc->body->accept(*this);
            loopBase_ = savedLoopBase;

            // Emit fall-through branch to merge label for implicit return
            {
                bool needBr = true;
                if (currentBlock_ && !currentBlock_->insts.empty()) {
                    auto lastOp = currentBlock_->insts.back().op;
                    if (lastOp == ir::Op::BR || lastOp == ir::Op::RET || lastOp == ir::Op::RET_VOID)
                        needBr = false;
                }
                if (needBr) {
                    ir::Inst br;
                    br.op = ir::Op::BR;
                    br.src1 = ir::Operand::label(mergeLabel);
                    emit(br);
                }
            }

            // Emit merge block
            startBlock(mergeLabel);
            inlineSourceFunc_ = savedInlineSourceFunc;

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
            localRegister_ = savedLocalRegister;
            localPointedToTypeName_ = savedLocalPointedToTypeName;
            localConstPtrValue_ = savedLocalConstPtrValue;
            localArrayDims_ = savedLocalArrayDims;
            localPtrLevel_ = savedLocalPtrLevel;
            paramArrayDims_ = savedParamArrayDims;
            paramVlaDims_ = savedParamVlaDims;

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
                auto varIt = locals_.find(node.name);
                ir::Operand ptrVal;
                if (varIt != locals_.end()) {
                    // Local function pointer parameter or variable
                    // The vreg already contains the function address value.
                    // Use it directly without dereferencing (unlike data pointers).
                    ptrVal = varIt->second;
                    usedVregs_.insert(ptrVal.vregId);
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
                inst.op = ir::Op::CALL_INDIRECT;
                inst.callSig = fpSigKey(fpSig.get());
                if (isSret) {
                    inst.resultType = ir::Type::VOID;
                    emit(inst);
                    sretCallResult(sretKey, sretBuf, sretAddr, loc(node));
                } else {
                    // Result type from the pointer's signature (int when unknown)
                    ir::Type rt = fpSig ? mapType(fpSig->returnType, fpSig->returnPointerLevel) : ir::Type::I16;
                    if (rt == ir::Type::VOID) {
                        inst.resultType = ir::Type::VOID;
                        emit(inst);
                    } else {
                        auto dest = allocVreg(rt);
                        inst.dest = dest;
                        inst.resultType = rt;
                        emit(inst);
                        lastValue_ = dest;
                    }
                    lastValueSigned_ = fpSig ? fpSig->returnIsSigned : true;
                }
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
                    if (allowImplicitFunctionDecl) {
                        warnings_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column,
                            Severity::Warning, "implicit declaration of function '" + node.name + "' [-fimplicit-function-declaration]"));
                    } else {
                        errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column,
                            Severity::Error, "implicit declaration of function '" + node.name + "' (use -fimplicit-function-declaration to allow)"));
                    }
                }

                if (isSret) {
                    inst.op = ir::Op::CALL_VOID;
                    inst.resultType = ir::Type::VOID;
                    emit(inst);
                    sretCallResult(node.name, sretBuf, sretAddr, loc(node));
                } else if (structReturningFunctions_.count(node.name) > 0) {
                    // Struct function returns struct value in AXYZ (4 bytes for I32)
                    // Call it with struct return type (I32)
                    inst.op = ir::Op::CALL;
                    inst.dest = structDest;
                    inst.resultType = retType;
                    emit(inst);

                    lastValue_ = structDest;  // Return the allocated destination
                    lastValueSigned_ = false;
                } else if (retType == ir::Type::VOID) {
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
                    auto rsit = functionReturnSigned_.find(node.name);
                    lastValueSigned_ = rsit != functionReturnSigned_.end() && rsit->second;
                }
            }
        }
    }
}

// The printf family and the position of its format argument
static const std::map<std::string, size_t>& printfLikeFunctions() {
    static const std::map<std::string, size_t> m = {
        {"printf", 0}, {"vprintf", 0}, {"fprintf", 1}, {"vfprintf", 1},
        {"sprintf", 1}, {"vsprintf", 1}, {"snprintf", 2}, {"vsnprintf", 2},
        {"cprintf", 0}, {"cprintfxy", 2}, {"fcm_cprintf", 4}};
    return m;
}

// The printf formatter links its integer conversions only when a format
// string needs them (lib/stdlib/fmtconv.h): __fmt_int_linked for an int or
// char argument, __fmt_long_linked for a long one. A call with a literal
// format needs what the literal's conversions use; a printf-like function
// passing its own format parameter on needs nothing (its callers' formats
// count); any other format -- and a printf-like function used as a value
// -- needs everything.
void IRBuilder::needAllFormats() {
    auto& refs = currentFunc_ ? currentFunc_->asmRefs : module_.asmRefs;
    refs.erase("___fmt_int_linked");   // the 32-bit digits serve int arguments too
    refs.insert("___fmt_long_linked");
}

void IRBuilder::noteFormatNeeds(FunctionCall& call) {
    if (call.callExpr) return;
    const auto& table = printfLikeFunctions();
    auto it = table.find(call.name);
    if (it == table.end()) return;
    if (it->second >= call.arguments.size()) return;
    Expression* fmt = call.arguments[it->second].get();
    if (auto* sl = dynamic_cast<StringLiteral*>(fmt)) {
        auto& refs = currentFunc_ ? currentFunc_->asmRefs : module_.asmRefs;
        const std::string& f = sl->value;
        for (size_t i = 0; i < f.size(); i++) {
            if (f[i] != '%') continue;
            i++;
            while (i < f.size() && std::strchr("-+ #0", f[i])) i++;
            while (i < f.size() && (std::isdigit((unsigned char)f[i]) || f[i] == '*')) i++;
            if (i < f.size() && f[i] == '.') { i++; while (i < f.size() && (std::isdigit((unsigned char)f[i]) || f[i] == '*')) i++; }
            bool isLong = false;
            while (i < f.size() && std::strchr("hlLjzt", f[i])) { if (f[i] == 'l' || f[i] == 'L' || f[i] == 'j') isLong = true; i++; }
            if (i >= f.size()) break;
            char c = f[i];
            if (std::strchr("diuxXob", c)) refs.insert(isLong ? "___fmt_long_linked" : "___fmt_int_linked");
            else if (c == 'p') refs.insert("___fmt_int_linked");
        }
        // the 32-bit digits also serve int arguments: one hook is enough
        if (refs.count("___fmt_long_linked")) refs.erase("___fmt_int_linked");
        return;
    }
    // a printf-like function passing its own format parameter on
    if (auto* vr = dynamic_cast<VariableReference*>(fmt); vr && currentFunc_) {
        std::string self = currentFunc_->name;
        if (!self.empty() && self[0] == '_') self = self.substr(1);
        auto own = table.find(self);
        if (own != table.end() && own->second < currentFunc_->paramNames.size() &&
            currentFunc_->paramNames[own->second] == vr->name)
            return;
    }
    needAllFormats();
}

void IRBuilder::visit(ExpressionStatement& node) {
    if (auto* call = dynamic_cast<FunctionCall*>(node.expression.get())) simplifyPrintfCall(*call);
    if (node.expression) node.expression->accept(*this);
}

// printf("text") whose result is unused and whose text has no conversion
// (%): write the text without the formatter, as GCC does -- puts("text")
// for "text\n" (in the default encoding, where \n is the $0D puts writes),
// else putstr("text"), puts without the newline. The library's printf
// only (not one this unit defines), and only when the replacement is
// declared (<stdio.h>).
void IRBuilder::simplifyPrintfCall(FunctionCall& call) {
    if (call.callExpr || call.name != "printf" || call.arguments.size() != 1) return;
    if (unitDefinedFunctions_.count("printf")) return;
    auto* sl = dynamic_cast<StringLiteral*>(call.arguments[0].get());
    if (!sl || sl->isWide || sl->value.find('%') != std::string::npos) return;
    bool newlineAtEnd = !sl->value.empty() && sl->value.back() == '\n' &&
                        sl->escMask.size() == sl->value.size() && sl->escMask.back() == 'n';
    if (newlineAtEnd && stringEncodingOf(*sl) == 0 && functionParamTypes_.count("puts") &&
        !unitDefinedFunctions_.count("puts")) {
        sl->value.pop_back();
        sl->escMask.pop_back();
        call.name = "puts";
    } else if (functionParamTypes_.count("putstr") && !unitDefinedFunctions_.count("putstr")) {
        call.name = "putstr";
    }
}

// ============================================================================
// Control flow
// ============================================================================

void IRBuilder::visit(ReturnStatement& node) {
    // Inline expansion: return stores to inline result vreg and branches to merge.
    // The value has the inlined function's return type (the result vreg's),
    // not that of the function it is inlined into (currentFunc_)
    if (!inlineReturnLabel_.empty()) {
        if (node.expression) {
            ir::Type retType = inlineReturnTarget_.type;
            narrowWide(node.expression, retType);
            node.expression->accept(*this);
            auto val = lastValue_;

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
    if (currentFunc_ && !currentFuncReturnsStruct()) narrowWide(node.expression, currentFunc_->returnType);

    // Struct/union/float result: copy into the caller's buffer (*__sret), return the pointer
    if (currentFunc_) {
        std::string fname = currentFunc_->name.substr(currentFunc_->name[0] == '_' ? 1 : 0);
        auto sit = sretSize_.find(fname);
        if (sit != sretSize_.end() && locals_.count("__sret")) {
            // Read the pointer out of the parameter (a STORE to the parameter's own
            // slot would assign the variable instead of storing through it)
            ir::Operand dst = allocVreg(ir::Type::PTR);
            ir::Inst cp;
            cp.op = ir::Op::COPY;
            cp.dest = dst;
            cp.src1 = locals_["__sret"];
            cp.resultType = ir::Type::PTR;
            cp.loc = loc(node);
            emit(cp);
            if (node.expression) {
                if (sretFloat_.count(fname)) {
                    node.expression->accept(*this);
                    IRTypeInfo info = getExprTypeInfo(node.expression.get());
                    auto fval = emitCast(lastValue_, ir::Type::F32, info.isSigned);
                    ir::Inst st;
                    st.op = ir::Op::STORE;
                    st.resultType = ir::Type::F32;
                    st.src1 = fval;
                    st.src2 = dst;
                    st.loc = loc(node);
                    emit(st);
                } else {
                    emitAggregateInto(node.expression.get(), dst, sit->second, loc(node));
                }
            }
            ir::Inst ret;
            ret.op = ir::Op::RET;
            ret.src1 = dst;
            ret.resultType = ir::Type::PTR;
            ret.loc = loc(node);
            emit(ret);
            return;
        }
    }

    // Struct-returning functions return struct value in AXYZ (4 bytes)
    // The call site will copy AXYZ to the destination vreg via special COPY handling
    bool isStructReturn = false;
    if (node.expression && currentFunc_) {
        std::string funcNameToCheck = currentFunc_->name;
        // Remove "_" prefix if present
        if (funcNameToCheck[0] == '_') {
            funcNameToCheck = funcNameToCheck.substr(1);
        }
        isStructReturn = structReturningFunctions_.count(funcNameToCheck) > 0;
    }
    if (isStructReturn) {
        // For struct returns: return the struct value (I32 = 4 bytes)
        node.expression->accept(*this);
        auto structVal = lastValue_;
        ir::Type retType = currentFunc_->returnType;

        // Emit normal RET with struct value
        ir::Inst ret;
        ret.op = ir::Op::RET;
        ret.src1 = structVal;
        ret.resultType = retType;  // I32 for 4-byte struct
        ret.loc = loc(node);
        emit(ret);
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

    // A constant condition (kept by the folder because a case/goto label is
    // inside): a plain branch, the other side is only reached by that label
    if (auto* lit = dynamic_cast<IntegerLiteral*>(node.condition.get())) {
        ir::Inst br;
        br.op = ir::Op::BR;
        br.src1 = ir::Operand::label(lit->value ? thenLabel : falseTarget);
        br.loc = loc(node);
        emit(br);
    } else {
        // Optimization: for && and || conditions, emit chained branches directly
        // to the if's true/false targets — no boolean materialization.
        emitConditionBranches(node.condition.get(), thenLabel, falseTarget, loc(node));
    }

    // the branch not expected to run (__unlikely / __likely with an else):
    // its blocks are cold, placed at the function's end
    bool hinted = !dynamic_cast<IntegerLiteral*>(node.condition.get());
    bool coldThen = hinted && node.hint < 0;
    bool coldElse = hinted && node.hint > 0 && node.elseBranch;

    if (coldThen) coldDepth_++;
    startBlock(thenLabel);
    node.thenBranch->accept(*this);
    ir::Inst brEnd;
    brEnd.op = ir::Op::BR;
    brEnd.src1 = ir::Operand::label(endLabel);
    emit(brEnd);
    if (coldThen) coldDepth_--;

    if (node.elseBranch) {
        if (coldElse) coldDepth_++;
        startBlock(elseLabel);
        node.elseBranch->accept(*this);
        ir::Inst brEnd2;
        brEnd2.op = ir::Op::BR;
        brEnd2.src1 = ir::Operand::label(endLabel);
        emit(brEnd2);
        if (coldElse) coldDepth_--;
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
    if (loopIdioms && tryLoopIdiom(node)) return;
    std::string condLabel = newLabel("for_cond");
    std::string bodyLabel = newLabel("for_body");
    std::string incLabel = newLabel("for_inc");
    std::string endLabel = newLabel("for_end");

    loopStack_.push_back({endLabel, incLabel});

    // for (int i = ...): i's scope is the loop
    auto shadowed = saveShadowed(node.initializer.get(), false);
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
    restoreShadowed(shadowed);
}

// ============================================================================
// Stub visitors (TODO: implement in future IR phases)
// ============================================================================

void IRBuilder::visit(ConditionalExpression& node) {
    // the condition is a value even when the result is wanted as an address
    bool savedAddrOnly = computeAddressOnly_;
    computeAddressOnly_ = false;
    node.condition->accept(*this);
    computeAddressOnly_ = savedAddrOnly;
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
    
    // (an arm narrower than the result widened: c ? x : 0 with a long x
    // copied a 16-bit 0, leaving the top bytes as they were)
    if (dest.type == ir::Type::F32 || thenVal.type == ir::Type::F32 ||
        ir::typeSize(thenVal.type) != ir::typeSize(dest.type))
        thenVal = emitCast(thenVal, dest.type, lastValueSigned_);
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
    if (dest.type == ir::Type::F32 || elseVal.type == ir::Type::F32 ||
        ir::typeSize(elseVal.type) != ir::typeSize(dest.type))
        elseVal = emitCast(elseVal, dest.type, lastValueSigned_);
    
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
        std::string errMsg = "No matching association in _Generic selection";
        throw std::runtime_error(formatSemanticError(errMsg, node.sourceFile, node.line, node.column));
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
    vecSubscript(node);
    // (*p)[i] of a pointer to an array is p[0][i]
    if (auto* u = dynamic_cast<UnaryOperation*>(node.arrayExpr.get());
        u && u->op == "*" && ptrArrayRowSize(u->operand.get())) {
        std::unique_ptr<Expression> deref = std::move(node.arrayExpr);
        auto* du = static_cast<UnaryOperation*>(deref.get());
        auto row = std::make_unique<ArrayAccess>(std::move(du->operand), std::make_unique<IntegerLiteral>(0));
        row->line = node.line; row->column = node.column; row->sourceFile = node.sourceFile;
        node.arrayExpr = std::move(row);
        visit(node);
        // Give the operand back to the original expression
        du->operand = std::move(static_cast<ArrayAccess*>(node.arrayExpr.get())->arrayExpr);
        node.arrayExpr = std::move(deref);
        return;
    }
    // i[p] is p[i]: put the pointer/array operand first
    {
        IRTypeInfo ai = getExprTypeInfo(node.arrayExpr.get());
        IRTypeInfo ii = getExprTypeInfo(node.indexExpr.get());
        bool aPtr = ai.type == ir::Type::PTR || dynamic_cast<StringLiteral*>(node.arrayExpr.get());
        bool iPtr = ii.type == ir::Type::PTR || dynamic_cast<StringLiteral*>(node.indexExpr.get());
        if (!aPtr && iPtr) std::swap(node.arrayExpr, node.indexExpr);
    }
    bool oldAddrMode = computeAddressOnly_;
    computeAddressOnly_ = false;

    // Evaluate base address
    if (auto* ref = dynamic_cast<VariableReference*>(node.arrayExpr.get())) {
        auto ait = localArrayDims_.find(ref->name);
        if (ait != localArrayDims_.end() && !ait->second.empty() && !uplevelNames_.count(ref->name)) {
            // Local array — compute address via ADDR_LOCAL (a local static
            // array is a global: its operand is the address)
            auto it = locals_.find(ref->name);
            if (it != locals_.end()) usedVregs_.insert(it->second.vregId);  // a[i] uses a
            if (it != locals_.end() && it->second.kind == ir::OperandKind::GLOBAL) {
                lastValue_ = it->second;
            } else if (it != locals_.end()) {
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

    // Evaluate index (a constant one is folded into the address below)
    bool constIdx = false;
    long constIdxVal = 0;
    {
        Expression* ie = node.indexExpr.get();
        while (auto* c = dynamic_cast<CastExpression*>(ie)) {
            if (c->pointerLevel || mapType(c->targetType, 0) == ir::Type::F32) break;
            ie = c->expression.get();
        }
        if (auto* lit = dynamic_cast<IntegerLiteral*>(ie); lit && lit->castPointerLevel == 0) {
            constIdx = true;
            constIdxVal = (long)lit->value;
        }
    }
    ir::Operand index;
    if (!constIdx) {
        node.indexExpr->accept(*this);
        index = lastValue_;
    } else {
        index = ir::Operand::imm(constIdxVal, ir::Type::I16);
    }

    computeAddressOnly_ = oldAddrMode;


    // Determine element size from array type (handling multi-dimensional arrays)
    int elemSize = 2; // default
    std::vector<ir::Operand> rtDims;  // run-time strides (see below)
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
        else if (paramArrayDims_.count(ref->name)) dims = paramArrayDims_[ref->name];  // int a[][M]
        else {
            auto gait = globalArrayDims_.find(ref->name);
            if (gait != globalArrayDims_.end()) dims = gait->second;
        }

        // Indexing past the array dimensions goes through pointer elements:
        // each further index steps by the type one pointer level down
        // (p[i][k] for char *p[], pp[0][1] for char **pp)
        bool isLocalRef = localTypes_.count(ref->name) > 0;
        int ptrLevel = isLocalRef ? (localPtrLevel_.count(ref->name) ? localPtrLevel_[ref->name] : -1)
                                  : (globalPtrLevel_.count(ref->name) ? globalPtrLevel_[ref->name] : -1);
        int through = depth - (int)dims.size();
        if (through > 0 && ptrLevel >= through) {
            std::string tn = isLocalRef ? localTypeNames_[ref->name] : globalTypeNames_[ref->name];
            elemSize = std::max(1, getTypeSize(tn, ptrLevel - through));
        } else if (!dims.empty() && depth <= (int)dims.size()) {
            std::string tn;
            if (localTypeNames_.count(ref->name)) tn = localTypeNames_[ref->name];
            else tn = globalTypeNames_[ref->name];

            // For pointer arrays (e.g. char *arr[7]), each element is a pointer (2 bytes),
            // not the pointed-to type size.  Detect via localPointedToType_/globalPointedToType_.
            bool isPointerElement = localPointedToType_.count(ref->name) || globalPointedToType_.count(ref->name);
            // The element's own pointer level when known (an int a[][M]
            // parameter is a pointer, but its elements are ints)
            if (isLocalRef && localPtrLevel_.count(ref->name)) isPointerElement = localPtrLevel_[ref->name] > 0;
            else if (!isLocalRef && globalPtrLevel_.count(ref->name)) isPointerElement = globalPtrLevel_[ref->name] > 0;
            int stride = isPointerElement ? 2 : getTypeSize(tn, 0);
            for (size_t i = depth; i < dims.size(); i++) {
                stride *= dims[i];
            }
            elemSize = stride;
        } else {
            elemSize = ir::typeSize(baseType);
        }
    } else if (auto* cl = dynamic_cast<CompoundLiteral*>(root); cl && !cl->arrayDims.empty()) {
        // (T[N][M]){...}[i]: stride over the dimensions not yet indexed
        elemSize = getTypeSize(cl->targetType, cl->pointerLevel);
        for (size_t i = depth; i < cl->arrayDims.size(); i++) elemSize *= cl->arrayDims[i];
    } else if (auto* ma = dynamic_cast<MemberAccess*>(root)) {
        // Array member of a struct (e.g., sid1->voice[1], vic4->sprite[0])
        // Look up the member's element type from struct info
        // A multi-dimensional member (double x[2][3]): the stride covers the
        // dimensions not yet indexed
        if (auto* mi = arrayMemberOf(ma)) {
            // A member with run-time dimensions (struct with run-time sizes):
            // the remaining dimensions' strides at run time
            std::vector<std::string> mdv;
            if (!vmStructs_.empty()) {
                IRTypeInfo bi = getExprTypeInfo(ma->structExpr.get());
                std::string vn = getAggregateName(bi.typeName);
                if (vn.rfind("struct ", 0) == 0) vn = vn.substr(7);
                else if (vn.rfind("union ", 0) == 0) vn = vn.substr(6);
                auto vsit = vmStructs_.find(vn);
                if (vsit != vmStructs_.end() && vsit->second.memberDims.count(ma->memberName))
                    mdv = vsit->second.memberDims[ma->memberName];
            }
            if (!mdv.empty() && !mi->arrayDims.empty()) {
                elemSize = std::max(1, getTypeSize(mi->type, mi->pointerLevel));
                for (size_t i = depth; i < mi->arrayDims.size(); i++) {
                    if (i < mdv.size() && !mdv[i].empty()) rtDims.push_back(locals_[mdv[i]]);
                    else elemSize *= mi->arrayDims[i];
                }
            } else if (int through = depth - (int)mi->arrayDims.size(); through > 0 && mi->pointerLevel >= through) {
                // Past the member's array dimensions, through pointers: s->p[i]
                // (a pointer member), s->names[i][k] (char *names[3]) step by
                // the type one pointer level further down
                elemSize = getTypeSize(mi->type, mi->pointerLevel - through);
            } else if (!mi->arrayDims.empty()) {
                elemSize = getTypeSize(mi->type, mi->pointerLevel);
                for (size_t i = depth; i < mi->arrayDims.size(); i++) elemSize *= mi->arrayDims[i];
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

    // A parameter with run-time inner dimensions (char a[2][n]): the stride
    // is (element size x constant dims) x the run-time dims, multiplied at
    // run time
    if (auto* ref = dynamic_cast<VariableReference*>(root)) {
        auto vit = paramVlaDims_.find(ref->name);
        auto pit = paramArrayDims_.find(ref->name);
        if (vit != paramVlaDims_.end() && pit != paramArrayDims_.end()) {
            const auto& dims = pit->second;
            std::string tn = localTypeNames_[ref->name];
            int lvl = localPtrLevel_.count(ref->name) ? localPtrLevel_[ref->name] : 0;
            int constStride = std::max(1, getTypeSize(tn, lvl));
            for (size_t i = depth; i < dims.size(); i++) {
                if (dims[i] > 0) constStride *= dims[i];
                else if (i - 1 < vit->second.size() && vit->second[i - 1].kind != ir::OperandKind::NONE)
                    rtDims.push_back(vit->second[i - 1]);
            }
            if (!rtDims.empty()) elemSize = constStride;
        }
    }

    // Elements of a struct with run-time member sizes: its run-time size
    if (rtDims.empty()) {
        std::string vn = vmStructOf(&node);
        if (!vn.empty()) {
            rtDims.push_back(locals_[vmStructs_[vn].sizeVar]);
            elemSize = 1;
        }
    }

    // addr = base + index * elemSize
    ir::Operand addr = allocVreg(ir::Type::PTR);
    if (constIdx && rtDims.empty()) {
        // A constant index: base + constant (base itself for 0)
        long off = constIdxVal * elemSize;
        // (a variable's slot as an address would mean the variable itself:
        // only a temporary pointer is used as is)
        if (off == 0 && base.isVreg() && base.type == ir::Type::PTR && currentFunc_ &&
            !currentFunc_->localSlotVregs.count(base.vregId) && !currentFunc_->memoryVregs.count(base.vregId)) {
            addr = base;
        } else {
            ir::Inst add;
            add.op = ir::Op::ADD;
            add.dest = addr;
            add.resultType = ir::Type::PTR;
            add.src1 = base;
            add.src2 = ir::Operand::imm(off, ir::Type::I16);
            add.loc = loc(node);
            emit(add);
        }
    } else if (!rtDims.empty()) {
        ir::Operand off = emitCast(index, ir::Type::I16, getExprTypeInfo(node.indexExpr.get()).isSigned);
        auto mulBy = [&](const ir::Operand& f) {
            auto r = allocVreg(ir::Type::I16);
            ir::Inst m;
            m.op = ir::Op::MUL;
            m.dest = r;
            m.resultType = ir::Type::I16;
            m.src1 = off;
            m.src2 = f;
            m.loc = loc(node);
            emit(m);
            off = r;
        };
        for (const auto& d : rtDims) mulBy(d);  // the slot vreg is the dimension's value
        if (elemSize != 1) mulBy(ir::Operand::imm(elemSize, ir::Type::I16));
        ir::Inst add;
        add.op = ir::Op::ADD;
        add.dest = addr;
        add.resultType = ir::Type::PTR;
        add.src1 = base;
        add.src2 = off;
        add.loc = loc(node);
        emit(add);
    } else {
    ir::Inst inst;
    inst.op = ir::Op::ADDR_ELEM;
    inst.dest = addr;
    inst.resultType = ir::Type::PTR;
    inst.src1 = base;
    inst.src2 = index;
    inst.args.push_back(ir::Operand::imm(elemSize, ir::Type::I16));
    inst.loc = loc(node);
    emit(inst);
    }

    // Determine if we need to LOAD the final value or just use the sub-array address
    bool isFinal = true;
    if (auto* ref = dynamic_cast<VariableReference*>(root)) {
        std::vector<int> dims;
        auto ait = localArrayDims_.find(ref->name);
        if (ait != localArrayDims_.end()) dims = ait->second;
        else if (paramArrayDims_.count(ref->name)) dims = paramArrayDims_[ref->name];
        else {
            auto git = globalArrayDims_.find(ref->name);
            if (git != globalArrayDims_.end()) dims = git->second;
        }
        if (depth < (int)dims.size()) isFinal = false;
    } else if (auto* ma = dynamic_cast<MemberAccess*>(root)) {
        if (auto* mi = arrayMemberOf(ma); mi && depth < (int)mi->arrayDims.size()) isFinal = false;
    } else if (auto* cl = dynamic_cast<CompoundLiteral*>(root)) {
        if (depth < (int)cl->arrayDims.size()) isFinal = false;
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

// The struct member a MemberAccess names, or nullptr
const IRBuilder::IRMemberInfo* IRBuilder::arrayMemberOf(MemberAccess* ma) {
    std::string t = getExprTypeInfo(ma->structExpr.get()).typeName;
    if (t.rfind("struct ", 0) != 0 && t.rfind("union ", 0) != 0) t = resolveTypedefToStruct(t);
    std::string sName = getAggregateName(t);
    if (!structs_.count(sName)) return nullptr;
    int off = 0;
    return findStructMember(sName, ma->memberName, off);
}

void IRBuilder::visit(MemberAccess& node) {
    // For p->member: load the pointer value (not address-of)
    // For s.member: compute address of the struct variable
    bool oldAddrMode = computeAddressOnly_;
    computeAddressOnly_ = !node.isArrow;
    node.structExpr->accept(*this);
    auto base = lastValue_;
    computeAddressOnly_ = oldAddrMode;

    // f().m, (a + b).m of a small struct returned in registers: the value
    // has no address; it is stored into a hidden local first
    if (!node.isArrow && (dynamic_cast<FunctionCall*>(node.structExpr.get()) ||
                          dynamic_cast<BinaryOperation*>(node.structExpr.get()) ||
                          dynamic_cast<UnaryOperation*>(node.structExpr.get())))
        base = addressOfValue(base, loc(node));

    IRTypeInfo baseInfo = getExprTypeInfo(node.structExpr.get());
    std::string sName = getAggregateName(baseInfo.typeName);

    // Phase 102: Resolve typedef-to-struct mappings
    // If sName doesn't have "struct " prefix and is a typedef, resolve it
    if (sName.find("struct ") != 0) {
        std::string resolved = resolveTypedefToStruct(sName);
        if (resolved != sName) {
            sName = getAggregateName(resolved);
        } else {
            // Try the pattern: struct sName (if it was a simple name)
            std::string potentialStruct = "struct " + sName;
            if (structs_.find(potentialStruct) != structs_.end()) {
                sName = potentialStruct;
            }
        }
    }

    auto sit = structs_.find(sName);

    int accumulatedOffset = 0;
    auto* mit = findStructMember(sName, node.memberName, accumulatedOffset);
    if (!mit) {
        std::string errMsg = "Member '" + node.memberName + "' not found in struct '" + sName + "'";
        throw std::runtime_error(formatSemanticError(errMsg, node.sourceFile, node.line, node.column));
    }

    int memberOffset = mit->offset + accumulatedOffset;
    // A struct with run-time member sizes: the member's offset from its
    // hidden local
    {
        std::string vn = sName;
        if (vn.rfind("struct ", 0) == 0) vn = vn.substr(7);
        else if (vn.rfind("union ", 0) == 0) vn = vn.substr(6);
        auto vsit = vmStructs_.find(vn);
        if (vsit != vmStructs_.end() && vsit->second.offsetVar.count(node.memberName)) {
            ir::Operand off = locals_[vsit->second.offsetVar[node.memberName]];
            auto nb = allocVreg(ir::Type::PTR);
            ir::Inst a;
            a.op = ir::Op::ADD;
            a.dest = nb;
            a.resultType = ir::Type::PTR;
            a.src1 = base;
            a.src2 = off;
            a.loc = loc(node);
            emit(a);
            base = nb;
            memberOffset = 0;
        }
    }
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
        // A signed field: sign-extend from its width (shift up, then
        // arithmetic shift back down)
        int unitBits = 8 * ir::typeSize(loadType);
        if (info.isSigned && bitWidth < unitBits) {
            int sh = unitBits - bitWidth;
            auto up = allocVreg(loadType);
            ir::Inst shl;
            shl.op = ir::Op::SHL;
            shl.dest = up;
            shl.resultType = loadType;
            shl.src1 = extracted;
            shl.src2 = ir::Operand::imm(sh, ir::Type::I8);
            shl.loc = loc(node);
            emit(shl);
            auto down = allocVreg(loadType);
            ir::Inst asr;
            asr.op = ir::Op::ASR;
            asr.dest = down;
            asr.resultType = loadType;
            asr.src1 = up;
            asr.src2 = ir::Operand::imm(sh, ir::Type::I8);
            asr.loc = loc(node);
            emit(asr);
            lastValue_ = down;
        }
    } else {
        lastValue_ = val;
    }
}

void IRBuilder::visit(CompoundLiteral& node) {
    // (T[]){...}: the size comes from the initializer
    if (!node.arrayDims.empty() && node.arrayDims[0] <= 0 && node.initializer) {
        int n = 0, pos = 0;
        auto& il = *node.initializer;
        for (size_t i = 0; i < il.elements.size(); i++) {
            auto d = il.getDesignator(i);
            if (d.arrayIndex >= 0) pos = d.arrayIndex;
            n = std::max(n, ++pos);
        }
        node.arrayDims[0] = n;
    }
    bool isStructOrUnion = (node.targetType.rfind("struct ", 0) == 0 || node.targetType.rfind("union ", 0) == 0);
    if (isStructOrUnion && node.pointerLevel == 0) {
        std::string sName = getAggregateName(node.targetType);
        if (!structs_.count(sName)) {
            std::string errMsg = "Unknown struct type: " + node.targetType;
            throw std::runtime_error(formatSemanticError(errMsg, node.sourceFile, node.line, node.column));
        }
    }

    ir::Type t = mapType(node.targetType, node.pointerLevel);
    int size = getTypeSize(node.targetType, node.pointerLevel);
    if (!node.arrayDims.empty()) {
        int total = 1;
        for (int d : node.arrayDims) total *= d;
        size *= total;
    }

    if (node.initializer && node.initializer->elements.size() == 1 && node.arrayDims.empty() &&
        !isStructOrUnion && !structOf(node.targetType)) {
        // Scalar compound literal (int){5}: just evaluate the value
        bool savedAddrOnly = computeAddressOnly_;
        computeAddressOnly_ = false;
        node.initializer->elements[0]->accept(*this);
        computeAddressOnly_ = savedAddrOnly;
        if (lastValue_.type != t && t != ir::Type::VOID && lastValue_.type != ir::Type::VOID)
            lastValue_ = emitCast(lastValue_, t, getExprTypeInfo(node.initializer->elements[0].get()).isSigned);
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

        // The elements are values even when the literal's address is wanted
        bool savedAddrOnly = computeAddressOnly_;
        computeAddressOnly_ = false;
        // Laid out like a local's initializer (nested braces, designators,
        // bitfields, unnamed bitfields skipped)
        bool laidOut = currentFunc_ && node.initializer && (isStructOrUnion || !node.arrayDims.empty()) &&
                       initLocalAggregate(buf, node.targetType, node.pointerLevel, node.arrayDims,
                                          node.initializer.get(), loc(node));
        if (node.initializer && !laidOut) {
            std::string sName = getAggregateName(node.targetType);
            bool isStruct = structs_.count(sName) > 0;
            if (isStruct) {
                // Store each member at its correct offset using struct layout
                auto& si = structs_[sName];
                // Positional: the next named member (unnamed bitfields are skipped)
                std::vector<std::string> named;
                for (auto& mn : si.memberOrder)
                    if (mn.rfind("__unnamed_bf_", 0) != 0) named.push_back(mn);
                for (size_t i = 0; i < node.initializer->elements.size() && i < named.size(); i++) {
                    node.initializer->elements[i]->accept(*this);
                    auto val = lastValue_;
                    auto& mi = si.members[named[i]];
                    ir::Type mt = mapType(mi.type, mi.pointerLevel);
                    if ((mt == ir::Type::F32) != (val.type == ir::Type::F32) && mt != ir::Type::I_N &&
                        mi.pointerLevel == 0 && val.type != ir::Type::VOID)
                        val = emitCast(val, mt, getExprTypeInfo(node.initializer->elements[i].get()).isSigned);
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
        computeAddressOnly_ = savedAddrOnly;
        lastValue_ = temp;
        // A small struct/union is carried as an integer value (I8/I16/I32):
        // its value is the buffer's contents, not its address
        if (isStructOrUnion && node.pointerLevel == 0 && node.arrayDims.empty() && !computeAddressOnly_ &&
            (t == ir::Type::I8 || t == ir::Type::I16 || t == ir::Type::I32)) {
            auto v = allocVreg(t);
            ir::Inst ld;
            ld.op = ir::Op::LOAD;
            ld.dest = v;
            ld.resultType = t;
            ld.src1 = temp;
            ld.loc = loc(node);
            emit(ld);
            lastValue_ = v;
        }
    }
}

int IRBuilder::typeAlign(const std::string& type, int ptrLevel) {
    if (ptrLevel > 0) return 1;
    auto sit = structs_.find(getAggregateName(type));
    return sit != structs_.end() ? std::max(1, sit->second.align) : 1;
}

int IRBuilder::naturalAlign(const std::string& type, int ptrLevel) {
    if (ptrLevel > 0) return 2;
    if (const IRStructInfo* si = structOf(type)) return std::max(1, si->naturalAlign);
    if (type == "float" || type == "double" || type == "long double") return 1;
    int n = getTypeSize(type, 0);
    return (n == 2 || n == 4) ? n : 1;
}

// _Alignas(expr): a constant expression (ConstEval: _Alignas(sizeof x * 2)
// was taken as 1); at least 1
int IRBuilder::alignValue(Expression* e) {
    int64_t v;
    if (!e || !evalConstInt(e, v)) return 1;
    return v > 1 ? (int)v : 1;
}

int IRBuilder::alignofValue(AlignofExpression& node) {
    if (node.expression) {
        // __alignof__(expr): a function's or object's declared alignment,
        // else its type's (the expression is not evaluated)
        int a = 1;
        if (auto* vr = dynamic_cast<VariableReference*>(node.expression.get())) {
            auto fa = funcAlign_.find(vr->name);
            auto da = declaredAlign_.find(vr->name);
            if (fa != funcAlign_.end()) a = fa->second;
            else if (da != declaredAlign_.end()) a = da->second;
            else {
                IRTypeInfo ti = getExprTypeInfo(vr);
                a = typeAlign(ti.typeName, ti.type == ir::Type::PTR ? 1 : 0);
            }
        } else {
            IRTypeInfo ti = getExprTypeInfo(node.expression.get());
            a = typeAlign(ti.typeName, ti.type == ir::Type::PTR ? 1 : 0);
        }
        return std::max(1, a);
    }
    return typeAlign(node.typeName, node.pointerLevel);
}

void IRBuilder::visit(AlignofExpression& node) {
    lastValue_ = ir::Operand::imm(alignofValue(node), ir::Type::I16);
}

void IRBuilder::visit(SizeofExpression& node) {
    // sizeof a variable-length array: its size computed at the declaration
    if (auto* vr = dynamic_cast<VariableReference*>(node.expression.get())) {
        auto vit = vlaSizeVars_.find(vr->name);
        if (vit != vlaSizeVars_.end()) {
            VariableReference sv(vit->second);
            sv.accept(*this);
            return;
        }
    }
    // sizeof of a struct with run-time member sizes (or of such a variable)
    {
        std::string tn;
        if (node.isType && node.pointerLevel == 0 && node.arrayDims.empty()) tn = getAggregateName(node.typeName);
        else if (!node.isType) {
            if (auto* vr = dynamic_cast<VariableReference*>(node.expression.get())) {
                auto vit = vmVars_.find(vr->name);
                if (vit != vmVars_.end()) tn = vit->second;
            }
        }
        if (tn.rfind("struct ", 0) == 0) tn = tn.substr(7);
        else if (tn.rfind("union ", 0) == 0) tn = tn.substr(6);
        auto vsit = vmStructs_.find(tn);
        if (vsit != vmStructs_.end()) {
            VariableReference sv(vsit->second.sizeVar);
            sv.accept(*this);
            return;
        }
    }
    // sizeof *p of a pointer to an array: the array's size
    if (!node.isType) {
        if (auto* u = dynamic_cast<UnaryOperation*>(node.expression.get()); u && u->op == "*") {
            if (int row = ptrArrayRowSize(u->operand.get())) {
                lastValue_ = ir::Operand::imm(row, ir::Type::I16);
                return;
            }
        }
    }
    // sizeof a[i]... of a parameter with run-time inner dimensions
    // (char a[2][n]): element size x the remaining dimensions, the run-time
    // ones read from their entry-time values
    if (!node.isType) {
        Expression* e = node.expression.get();
        size_t depth = 0;
        while (auto* aa = dynamic_cast<ArrayAccess*>(e)) { e = aa->arrayExpr.get(); depth++; }
        auto* ref = dynamic_cast<VariableReference*>(e);
        if (ref && depth > 0 && paramVlaDims_.count(ref->name) && paramArrayDims_.count(ref->name)) {
            const auto& dims = paramArrayDims_[ref->name];
            const auto& rt = paramVlaDims_[ref->name];
            std::string tn = localTypeNames_[ref->name];
            int lvl = localPtrLevel_.count(ref->name) ? localPtrLevel_[ref->name] : 0;
            int constSize = std::max(1, getTypeSize(tn, lvl));
            std::vector<ir::Operand> rtDims;
            for (size_t i = depth; i < dims.size(); i++) {
                if (dims[i] > 0) constSize *= dims[i];
                else if (i - 1 < rt.size() && rt[i - 1].kind != ir::OperandKind::NONE) rtDims.push_back(rt[i - 1]);
            }
            if (!rtDims.empty()) {
                ir::Operand v = ir::Operand::imm(constSize, ir::Type::I16);
                for (const auto& d : rtDims) {
                    auto r = allocVreg(ir::Type::I16);
                    ir::Inst m;
                    m.op = ir::Op::MUL;
                    m.dest = r;
                    m.resultType = ir::Type::I16;
                    m.src1 = d;  // the slot vreg is the dimension's value
                    m.src2 = v;
                    m.loc = loc(node);
                    emit(m);
                    v = r;
                }
                lastValue_ = v;
                return;
            }
        }
    }
    int sz = (int)staticSizeof(node);
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

// The case and default labels of a switch: its body's statements, not those
// of a nested switch. (It walks less than ASTWalker where a label cannot be:
// an assignment's target, a call's callee, goto *expr, _Alignof, a
// declaration's other expressions, _Static_assert, nested functions.)
class CaseCollector : public ASTWalker {
public:
    IRBuilder::SwitchCtx& ctx;
    IRBuilder& builder;
    CaseCollector(IRBuilder::SwitchCtx& c, IRBuilder& b) : ctx(c), builder(b) {}
    using ASTWalker::visit;

    void visit(Assignment& node) override { if (node.expression) node.expression->accept(*this); }
    void visit(FunctionCall& node) override { for (auto& arg : node.arguments) arg->accept(*this); }
    void visit(AlignofExpression&) override {}
    void visit(VariableDeclaration& node) override { if (node.initializer) node.initializer->accept(*this); }
    void visit(SwitchContinueStatement& node) override { ctx.hasSwitchContinue = true; if (node.target) node.target->accept(*this); }
    void visit(GotoStatement&) override {}
    void visit(SwitchStatement& node) override {
        if (node.expression) node.expression->accept(*this);
        // Do NOT visit body to avoid collecting cases from nested switches
    }
    // A case label's value (ConstEval: case sizeof x, case (signed char)200);
    // one that is not a constant is an error (it was taken as 0)
    int64_t labelValue(Expression* e, CaseStatement& node) {
        int64_t v = 0;
        if (!builder.evalConstInt(e, v)) {
            Expression* at = e ? e : nullptr;
            builder.errors_.push_back(formatDiagnostic(at && at->line ? at->sourceFile : node.sourceFile,
                at && at->line ? at->line : node.line, at && at->line ? at->column : node.column, Severity::Error,
                "case label is not an integer constant expression"));
        }
        return v;
    }
    void visit(CaseStatement& node) override {
        int64_t val = labelValue(node.value.get(), node);
        int64_t endVal = val;
        bool isRange = false;
        if (node.rangeEnd) {
            endVal = labelValue(node.rangeEnd.get(), node);
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
    void visit(StaticAssert&) override {}
    void visit(StructDefinition&) override {}
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

    // continue <value> re-runs the dispatch with a new selector: keep the
    // selector in a slot (assigned by store) and give the dispatch a label
    if (ctx.hasSwitchContinue && currentFunc_) {
        ir::Type st = expr.type == ir::Type::VOID ? ir::Type::I16 : expr.type;
        ctx.selSlot = allocVreg(st);
        currentFunc_->localSlotVregs.insert(ctx.selSlot.vregId);
        ir::Inst stSel;
        stSel.op = ir::Op::STORE;
        stSel.resultType = st;
        stSel.src1 = expr;
        stSel.src2 = ctx.selSlot;
        stSel.loc = loc(node);
        emit(stSel);
        ctx.dispatchLabel = newLabel("switch_dispatch");
        ir::Inst brDisp;
        brDisp.op = ir::Op::BR;
        brDisp.src1 = ir::Operand::label(ctx.dispatchLabel);
        emit(brDisp);
        startBlock(ctx.dispatchLabel);
        expr = ctx.selSlot;
        ctx.expr = expr;
    }

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

void IRBuilder::visit(BreakStatement& node) {
    size_t open = loopStack_.size() - loopBase_;
    if (node.levels > 1 && (size_t)node.levels > open) {
        errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column, Severity::Error,
            "Compile Error: 'break " + std::to_string(node.levels) + "' leaves " +
            std::to_string(node.levels) + " loops/switches, but only " + std::to_string(open) +
            (open == 1 ? " encloses it" : " enclose it")));
        return;
    }
    if (open == 0) return;
    size_t target = loopStack_.size() - node.levels;
    releaseVlasForJump(target);
    ir::Inst br;
    br.op = ir::Op::BR;
    br.src1 = ir::Operand::label(loopStack_[target].breakLabel);
    emit(br);
}

void IRBuilder::visit(ContinueStatement&) {
    if (loopStack_.empty() || loopStack_.back().continueLabel.empty()) return;
    releaseVlasForJump(loopStack_.size() - 1);
    ir::Inst br;
    br.op = ir::Op::BR;
    br.src1 = ir::Operand::label(loopStack_.back().continueLabel);
    emit(br);
}

// continue <value>: jump to the case of the innermost switch that <value>
// selects (the dispatch runs again with it); continue default: to default
// (past the switch when there is none)
void IRBuilder::visit(SwitchContinueStatement& node) {
    if (switchStack_.empty()) {
        errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column, Severity::Error,
            "Compile Error: 'continue " + std::string(node.target ? "<value>" : "default") + "' outside a switch"));
        return;
    }
    const SwitchCtx& sc = switchStack_.back();
    releaseVlasForJump(loopStack_.size() - 1);
    ir::Inst br;
    br.op = ir::Op::BR;
    if (!node.target) {
        br.src1 = ir::Operand::label(sc.defaultLabel);
    } else {
        node.target->accept(*this);
        ir::Operand v = emitCast(lastValue_, sc.selSlot.type, getExprTypeInfo(node.target.get()).isSigned);
        ir::Inst st;
        st.op = ir::Op::STORE;
        st.resultType = sc.selSlot.type;
        st.src1 = v;
        st.src2 = sc.selSlot;
        st.loc = loc(node);
        emit(st);
        br.src1 = ir::Operand::label(sc.dispatchLabel);
    }
    br.loc = loc(node);
    emit(br);
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
    // #pragma cc45 overlay(name) at file scope: the module's overlay (as
    // -foverlay=name), which code generation emits and places by
    if (!currentFunc_ && node.code.rfind(".overlay ", 0) == 0) {
        std::string n = node.code.substr(9);
        while (!n.empty() && std::isspace((unsigned char)n.back())) n.pop_back();
        while (!n.empty() && std::isspace((unsigned char)n.front())) n.erase(0, 1);
        if (!n.empty()) { module_.overlay = n; return; }
    }
    if (node.code == ".weak_next") {
        weakNextFunction_ = true;
        return;
    }
    if (node.code == ".recurse_next") {
        recurseNextFunction_ = true;
        return;
    }
    if (node.code.rfind(".optimize ", 0) == 0) {
        // #pragma cc45 optimize(a, no-b): IR passes for the function it is in,
        // or (at file scope) for the functions that follow
        std::map<std::string, bool>& target = currentFunc_ ? currentFunc_->optOverrides : optOverrides_;
        std::stringstream list(node.code.substr(10));
        std::string item;
        while (std::getline(list, item, ',')) {
            item.erase(0, item.find_first_not_of(" \t"));
            item.erase(item.find_last_not_of(" \t") + 1);
            if (item.empty()) continue;
            bool on = item.compare(0, 3, "no-") != 0 && item.compare(0, 3, "no_") != 0;
            std::string name = on ? item : item.substr(3);
            const OptimizationInfo* o = OptimizationCatalog::find(name);
            std::string where = loc(node).valid() ? loc(node).file + ":" + std::to_string(loc(node).line) + ": " : "";
            if (!o) {
                emitWarning(where + "warning: unknown optimization '" + name + "' in #pragma cc45 optimize");
            } else if (o->impl == OptimizationImpl::IR_PASS) {
                target[o->name] = on;
            } else if (o->impl == OptimizationImpl::AST_PASS) {
                // done before IR generation (the AST pass reads the pragma itself)
            } else if (o->impl == OptimizationImpl::NONE) {
                if (futureWarnings)
                    emitWarning(where + "warning: optimization '" + o->kebabName() + "' is not implemented yet; #pragma cc45 optimize(" + item + ") has no effect");
            } else if (on) {
                // a file-wide or always-on optimization: asking for it is fine,
                // only turning it off for one function cannot be done
            } else if (o->impl == OptimizationImpl::ALWAYS_ON) {
                emitWarning(where + "warning: '" + o->kebabName() + "' is part of code generation and cannot be disabled");
            } else {
                emitWarning(where + "warning: '" + o->kebabName() + "' applies to the whole file: use -fno-" + o->kebabName() + " on the command line");
            }
        }
        return;
    }
    if (node.code.rfind(".max_unroll_bytes ", 0) == 0) {
        maxUnrollBytes = std::stoi(node.code.substr(18));
        module_.maxUnrollBytes = maxUnrollBytes;
        return;
    }
    if (node.code.rfind(".max_inline_ir_size ", 0) == 0) {
        maxInlineIRSize = std::stoi(node.code.substr(20));
        return;
    }
    if (node.code == ".no_zp_save") {
        module_.saveZP = false;
        return;
    }
    if (node.code == ".no_stack16") {
        module_.stackTop = 0;
        return;
    }
    if (node.code.rfind(".stack_size ", 0) == 0) {
        std::string v = node.code.substr(12);
        int base = 10;
        if (!v.empty() && v[0] == '$') { v = v.substr(1); base = 16; }
        else if (v.size() > 2 && v[0] == '0' && (v[1] == 'x' || v[1] == 'X')) { v = v.substr(2); base = 16; }
        try { module_.stackSize = (int)(std::stoul(v, nullptr, base) & 0xFFFF); } catch (...) {}
        return;
    }
    if (node.code.rfind(".stack_top ", 0) == 0) {
        std::string v = node.code.substr(11);
        int base = 10;
        if (!v.empty() && v[0] == '$') { v = v.substr(1); base = 16; }
        else if (v.size() > 2 && v[0] == '0' && (v[1] == 'x' || v[1] == 'X')) { v = v.substr(2); base = 16; }
        try { module_.stackTop = (int)(std::stoul(v, nullptr, base) & 0xFFFF); } catch (...) {}
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
    // asm("...") at file scope: assembler text for the module (cc45's own
    // pragma directives were handled above; the accepted-without-effect
    // .crt_* ones are dropped)
    if (!currentFunc_) {
        if (node.code.rfind(".crt_", 0) != 0) module_.fileAsm.push_back(node.code);
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

    // When inlining, rewrite @_p_name and @_l_name references to use
    // the callee's SAC storage symbols, since proc-scoped .var declarations
    // won't exist in the caller's scope.
    if (!inlineSourceFunc_.empty()) {
        std::string irName = "_" + inlineSourceFunc_;
        std::string& text = inst.asmText;
        // Rewrite @_p_name → _func__param_name (direct SAC label)
        size_t pos = 0;
        while ((pos = text.find("@_p_", pos)) != std::string::npos) {
            // Extract parameter name: alphanumeric chars after @_p_
            size_t nameStart = pos + 4;
            size_t nameEnd = nameStart;
            while (nameEnd < text.size() && (isalnum(text[nameEnd]) || text[nameEnd] == '_'))
                nameEnd++;
            std::string paramName = text.substr(nameStart, nameEnd - nameStart);
            // Also strip ", sp" suffix if present (SAC uses absolute addressing)
            std::string suffix;
            if (nameEnd + 4 <= text.size() && text.substr(nameEnd, 4) == ", sp") {
                suffix = ", sp";
            }
            std::string replacement = irName + "__param_" + paramName;
            text.replace(pos, (nameEnd - pos) + suffix.size(), replacement);
            pos += replacement.size();
        }
        // Rewrite @_l_name → _caller__local_N
        // The inlined local lives in the *caller's* SAC storage (the inliner
        // allocated a new vreg for it in the caller's frame).
        std::string callerName = currentFunc_ ? currentFunc_->name : irName;
        pos = 0;
        while ((pos = text.find("@_l_", pos)) != std::string::npos) {
            size_t nameStart = pos + 4;
            size_t nameEnd = nameStart;
            while (nameEnd < text.size() && (isalnum(text[nameEnd]) || text[nameEnd] == '_'))
                nameEnd++;
            std::string localName = text.substr(nameStart, nameEnd - nameStart);
            std::string suffix;
            if (nameEnd + 4 <= text.size() && text.substr(nameEnd, 4) == ", sp") {
                suffix = ", sp";
            }
            // Look up vreg ID for this local in the caller's context
            auto it = locals_.find(localName);
            if (it != locals_.end()) {
                std::string replacement = callerName + "__local_" + std::to_string(it->second.vregId);
                text.replace(pos, (nameEnd - pos) + suffix.size(), replacement);
                pos += replacement.size();
            } else {
                pos = nameEnd;
            }
        }
    }

    inst.loc = loc(node);
    emit(inst);
}

void IRBuilder::visit(StaticAssert&) { /* compile-time only */ }

void IRBuilder::visit(EnumDefinition&) { /* type-level only, values folded by ConstantFolder */ }

void IRBuilder::visit(StructDefinition& node) {
    IRStructInfo info;
    info.name = node.name;
    info.isUnion = node.isUnion;
    info.parent = node.parentStruct;
    int startOffset = 0, startAlign = 1, startNatural = 1;

    // Phase 3: Inherit parent struct members at offset 0
    if (!node.parentStruct.empty()) {
        auto pit = structs_.find(node.parentStruct);
        if (pit != structs_.end()) {
            const auto& parentInfo = pit->second;
            for (const auto& pName : parentInfo.memberOrder) {
                info.members[pName] = parentInfo.members.at(pName);
                info.memberOrder.push_back(pName);
            }
            startOffset = parentInfo.totalSize;
            startAlign = parentInfo.align;
            startNatural = parentInfo.naturalAlign;
        }
    }

    std::vector<StructLayout::Member> lm;
    for (const auto& m : node.members) {
        StructLayout::Member l;
        if (m.bitWidth > 0) {
            l.unitType = m.type;
            l.size = getTypeSize(m.type, 0);
            l.bitWidth = m.bitWidth;
            l.naturalAlign = naturalAlign(m.type, 0);
        } else {
            l.size = getTypeSize(m.type, m.pointerLevel);
            if (m.arraySize() >= 0) l.size *= m.arraySize();  // a flexible member d[]: 0
            // Explicit alignment (_Alignas / aligned) and aligned struct types
            l.explicitAlign = std::max({1, m.alignment, alignValue(m.alignmentExpr.get()),
                                        typeAlign(m.type, m.pointerLevel)});
            l.naturalAlign = naturalAlign(m.type, m.pointerLevel);
        }
        lm.push_back(l);
    }
    StructLayout::Result lr = StructLayout::layout(lm, node.isUnion, node.isUnpacked, node.alignment,
                                                   startOffset, startAlign, startNatural);

    for (size_t i = 0; i < node.members.size(); i++) {
        const auto& m = node.members[i];
        IRMemberInfo mi;
        mi.type = m.type;
        mi.pointerLevel = m.pointerLevel;
        mi.isSigned = m.isSigned;
        mi.isConst = m.isConst;
        mi.arrayDims = m.arrayDims;
        mi.bitWidth = m.bitWidth;
        mi.funcPtrSig = m.funcPtrSig;
        mi.size = lm[i].size;
        mi.offset = lr.placed[i].offset;
        if (m.bitWidth > 0) mi.bitOffset = lr.placed[i].bitOffset;
        info.members[m.name] = mi;
        info.memberOrder.push_back(m.name);
    }
    info.totalSize = lr.size;
    info.align = lr.align;
    info.naturalAlign = lr.naturalAlign;
    structs_[node.name] = info;

    // A struct defined in a function with run-time member sizes (char x[n]):
    // its member offsets and size are computed here, at the definition, into
    // hidden locals. The static layout is kept for type lookups, with a size
    // that is never 1, 2 or 4 (such a struct is always handled by address)
    if (currentFunc_ && !registeringStructs_) {
        bool vm = false;
        for (const auto& m : node.members)
            for (const auto& d : m.vlaDimExprs) if (d) vm = true;
        if (!vm) {
            vmStructs_.erase(node.name);
        } else {
            VmStruct vs;
            std::string base = "__vms" + std::to_string(nextLabel_++);
            int k = 0;
            auto declInt = [&](const std::string& n, std::unique_ptr<Expression> init) {
                auto d = std::make_unique<VariableDeclaration>("int", n, 0);
                d->initializer = std::move(init);
                d->line = node.line; d->column = node.column; d->sourceFile = node.sourceFile;
                d->accept(*this);
                usedVregs_.insert(locals_[n].vregId);
            };
            std::string running;   // the size so far ("" = 0)
            for (auto& m : node.members) {
                std::unique_ptr<Expression> sz = std::make_unique<IntegerLiteral>(std::max(1, getTypeSize(m.type, m.pointerLevel)));
                auto& dimVars = vs.memberDims[m.name];
                for (size_t j = 0; j < m.arrayDims.size(); j++) {
                    std::unique_ptr<Expression> dim;
                    if (j < m.vlaDimExprs.size() && m.vlaDimExprs[j]) {
                        std::string dn = base + "_d" + std::to_string(k) + "_" + std::to_string(j);
                        declInt(dn, std::move(m.vlaDimExprs[j]));
                        dim = std::make_unique<VariableReference>(dn);
                        dimVars.push_back(dn);
                    } else {
                        dimVars.push_back("");
                        dim = std::make_unique<IntegerLiteral>(std::max(0, m.arrayDims[j]));
                    }
                    sz = std::make_unique<BinaryOperation>("*", std::move(sz), std::move(dim));
                }
                std::string sn = base + "_s" + std::to_string(k);
                declInt(sn, std::move(sz));
                std::string on = base + "_o" + std::to_string(k);
                if (node.isUnion || running.empty())
                    declInt(on, std::make_unique<IntegerLiteral>(0));
                else
                    declInt(on, std::make_unique<VariableReference>(running));
                vs.offsetVar[m.name] = on;
                std::string next = base + "_r" + std::to_string(k);
                if (node.isUnion && !running.empty()) {
                    // the largest member
                    declInt(next, std::make_unique<ConditionalExpression>(
                        std::make_unique<BinaryOperation>(">", std::make_unique<VariableReference>(running), std::make_unique<VariableReference>(sn)),
                        std::make_unique<VariableReference>(running), std::make_unique<VariableReference>(sn)));
                } else {
                    declInt(next, std::make_unique<BinaryOperation>("+", std::make_unique<VariableReference>(on),
                                                                     std::make_unique<VariableReference>(sn)));
                }
                running = next;
                k++;
            }
            vs.sizeVar = running.empty() ? base + "_r0" : running;
            if (running.empty()) declInt(vs.sizeVar, std::make_unique<IntegerLiteral>(0));
            vmStructs_[node.name] = vs;
            int& ts = structs_[node.name].totalSize;
            if (ts < 6) ts = 6;
        }
    }
    parsedStructIsFinal_[node.name] = node.isFinal;

    // The registration pre-pass only records the layout: methods and vtables
    // come in the main pass, after the declarations that precede the struct
    // (a method calling a function declared above it)
    if (registeringStructs_) return;

    // Emit struct methods as top-level functions (once: the definition is
    // revisited when a method body declares a variable of its own struct type)
    if (methodsEmitted_.insert(node.name).second) {
        for (auto& method : node.methods) method->accept(*this);
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
    // va_start addresses the last named parameter's stack slot: keep the
    // parameter there (not in a zero-page register copy)
    {
        auto lp = locals_.find(node.lastParamName);
        if (lp != locals_.end() && lp->second.isVreg() && currentFunc_)
            currentFunc_->memoryVregs.insert(lp->second.vregId);
    }
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
    // va_arg(ap, type): load value from *ap, then advance ap by its slot size.
    // ap is always read as a value here, even when the caller wants an
    // address (copying a struct out of the argument list)
    bool addrMode = computeAddressOnly_;
    computeAddressOnly_ = false;
    node.ap->accept(*this);
    computeAddressOnly_ = addrMode;
    auto apAddr = lastValue_;

    ir::Type argType = mapType(node.typeName, node.pointerLevel);
    int argSize = ir::typeSize(argType);
    if (argSize < 2) argSize = 2; // default argument promotion

    // A struct/union (other than 1, 2 or 4 bytes) is passed through '...' as
    // the address of the caller's copy, like a by-value parameter
    int agg = aggregateBlockSize(node.typeName, node.pointerLevel);
    if (agg > 0) { argType = ir::Type::PTR; argSize = 2; }

    ir::Operand val;
    if (addrMode && agg == 0) {
        // Address of the argument itself (a small struct passed by value)
        val = allocVreg(ir::Type::PTR);
        ir::Inst cur;
        cur.op = ir::Op::ADD;
        cur.dest = val;
        cur.resultType = ir::Type::PTR;
        cur.src1 = apAddr;
        cur.src2 = ir::Operand::imm(0, ir::Type::I16);
        cur.loc = loc(node);
        emit(cur);
    } else {
        // Load the argument from *ap (ap is a ZP pointer pair — single dereference)
        val = allocVreg(argType);
        ir::Inst loadVal;
        loadVal.op = ir::Op::LOAD;
        loadVal.dest = val;
        loadVal.resultType = argType;
        loadVal.src1 = apAddr;
        loadVal.loc = loc(node);
        emit(loadVal);
    }

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

    // Store back. ap is normally a variable's slot (STORE assigns it); an
    // inlined va_list parameter is a plain vreg, which is reassigned instead
    // (a STORE would write through the pointer)
    bool isSlot = !apAddr.isVreg() || !currentFunc_ ||
                  currentFunc_->localSlotVregs.count(apAddr.vregId) ||
                  currentFunc_->memoryVregs.count(apAddr.vregId);
    ir::Inst storeAp;
    if (isSlot) {
        storeAp.op = ir::Op::STORE;
        storeAp.resultType = ir::Type::PTR;
        storeAp.src1 = newAp;
        storeAp.src2 = apAddr;
    } else {
        storeAp.op = ir::Op::COPY;
        storeAp.dest = apAddr;
        storeAp.resultType = ir::Type::PTR;
        storeAp.src1 = newAp;
    }
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
    // In a nested function the label may be the enclosing function's
    // (a non-local label): that function keeps it and gives it an alias
    if (currentFunc_ && currentFunc_->isNested) nestedLabelAddrs_.insert(node.label);
    ir::Inst addr;
    addr.op = ir::Op::ADDR_LABEL;
    addr.dest = allocVreg(ir::Type::I16);
    addr.src1 = ir::Operand::label(node.label);
    addr.loc = loc(node);
    emit(addr);
    lastValue_ = addr.dest;
}

// Phase 102: Typedef to Struct Resolution Implementation

void IRBuilder::registerAllStructDefinitions(TranslationUnit& unit) {
    // Phase 102.1: Walk the entire AST and register all struct definitions
    // This allows us to later resolve typedef'd struct types correctly

    // Walk through all top-level declarations looking for struct definitions
    // and variable declarations that might establish typedef mappings
    for (const auto& decl : unit.topLevelDecls) {
        if (auto* structDef = dynamic_cast<StructDefinition*>(decl.get())) {
            // Register the struct definition immediately
            registeringStructs_ = true;
            visit(*structDef);
            registeringStructs_ = false;
        }
        // Note: Variable declarations are visited normally during IR generation
        // and will establish typedef mappings through getExprTypeInfo()
    }
}

std::string IRBuilder::resolveTypedefToStruct(const std::string& typedefName) {
    // Phase 102.2: Resolve a typedef name to its underlying struct name
    auto it = typedefToStruct_.find(typedefName);
    if (it != typedefToStruct_.end()) {
        return it->second;
    }

    // If not found, return the original name (caller will handle lookup)
    // But also try to infer: if it doesn't start with "struct ", it might be
    // a typedef that maps to "struct <name>"
    if (typedefName.find("struct ") != 0) {
        // Try looking for "struct " + typedefName
        std::string potentialStructName = "struct " + typedefName;
        // This will be checked by the caller
    }

    return typedefName;
}

void IRBuilder::registerTypedefToStruct(const std::string& typedefName, const std::string& structName) {
    // Phase 102.3: Register a typedef-to-struct mapping
    // Example: registerTypedefToStruct("digi_system_t", "struct digi_system")
    if (!typedefName.empty() && !structName.empty()) {
        typedefToStruct_[typedefName] = structName;
    }
}

void IRBuilder::setTypedefMappings(const std::map<std::string, std::string>& typedefToBaseType) {
    // Phase 102.4: Register struct/union typedefs from the parser
    // Map: typedef_name → baseType (e.g., "digi_system_t" → "struct digi_system")

    for (const auto& [typedefName, baseType] : typedefToBaseType) {
        // Only register struct/union typedefs
        if (baseType.find("struct ") == 0 || baseType.find("union ") == 0) {
            // Register the mapping: typedef name → struct/union name
            registerTypedefToStruct(typedefName, baseType);
        }
    }
}
