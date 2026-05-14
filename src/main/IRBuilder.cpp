#include "IRBuilder.hpp"

IRBuilder::IRBuilder() {}

void IRBuilder::setSourceInfo(const std::string& filename) {
    module_.sourceFile = filename;
}

void IRBuilder::generate(TranslationUnit& unit) {
    unit.accept(*this);
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
    return ir::Operand::vreg(currentFunc_->allocVreg(), t);
}

ir::Type IRBuilder::mapType(const std::string& typeName, int ptrLevel) {
    if (ptrLevel > 0) return ir::Type::PTR;
    if (typeName == "char" || typeName == "unsigned char") return ir::Type::I8;
    if (typeName == "long" || typeName == "unsigned long") return ir::Type::I32;
    if (typeName == "void") return ir::Type::VOID;
    return ir::Type::I16;
}

int IRBuilder::getTypeSize(const std::string& typeName, int ptrLevel) {
    if (ptrLevel > 0) return 2;
    if (typeName == "char" || typeName == "unsigned char") return 1;
    if (typeName == "int" || typeName == "unsigned int" ||
        typeName == "short" || typeName == "unsigned short" ||
        typeName == "unsigned") return 2;
    if (typeName == "long" || typeName == "unsigned long") return 4;
    auto it = structs_.find(typeName);
    if (it != structs_.end()) return it->second.totalSize;
    return 2; // default for unknown types
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
    if (node.isPrototype || !node.body) return;

    ir::Function fn;
    fn.name = "_" + node.name;
    fn.returnType = mapType(node.returnType, node.returnPointerLevel);
    fn.conv = (zpCallMode || node.isFastcall) ? ir::CallConv::ZP : ir::CallConv::STACK;
    fn.isVariadic = node.isVariadic;
    fn.isStatic = node.isStatic;

    for (const auto& p : node.parameters) {
        fn.paramTypes.push_back(mapType(p.type, p.pointerLevel));
        fn.paramNames.push_back(p.name);
    }

    module_.functions.push_back(std::move(fn));
    currentFunc_ = &module_.functions.back();
    locals_.clear();
    localTypes_.clear();

    startBlock("entry");

    // Create vRegs for parameters
    for (const auto& p : node.parameters) {
        ir::Type pt = mapType(p.type, p.pointerLevel);
        auto vreg = allocVreg(pt);
        locals_[p.name] = vreg;
        localTypes_[p.name] = pt;
    }

    // Visit body
    node.body->accept(*this);

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
        gv.size = ir::typeSize(t);
        if (node.arraySize() > 0) gv.size *= node.arraySize();
        gv.isConst = node.isConst;
        gv.isStatic = node.isStatic;
        if (node.initializer) {
            if (auto* lit = dynamic_cast<IntegerLiteral*>(node.initializer.get())) {
                gv.hasInitValue = true;
                gv.initValue = lit->value;
            }
        }
        module_.globals.push_back(gv);
        return;
    }

    // Local variable — allocate a vReg
    auto vreg = allocVreg(t);
    locals_[node.name] = vreg;
    localTypes_[node.name] = t;
    localSigned_[node.name] = node.isSigned;

    // Emit initializer if present
    if (node.initializer) {
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
        lastValue_ = it->second;
        auto sit = localSigned_.find(node.name);
        lastValueSigned_ = (sit != localSigned_.end()) ? sit->second : false;
    } else {
        auto dest = allocVreg(ir::Type::I16);
        ir::Inst inst;
        inst.op = ir::Op::LOAD;
        inst.dest = dest;
        inst.resultType = ir::Type::I16;
        inst.src1 = ir::Operand::global("_" + node.name);
        inst.loc = loc(node);
        emit(inst);
        lastValue_ = dest;
    }
}

void IRBuilder::visit(Assignment& node) {
    // Evaluate RHS
    node.expression->accept(*this);
    auto rhs = lastValue_;

    // Check for bitfield assignment: target is MemberAccess to a bitfield
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
            // Evaluate struct base address
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

    // Simple assignment to variable reference
    if (auto* vr = dynamic_cast<VariableReference*>(node.target.get())) {
        auto it = locals_.find(vr->name);
        if (it != locals_.end()) {
            ir::Inst store;
            store.op = ir::Op::STORE;
            store.resultType = it->second.type;
            store.src1 = rhs;
            store.src2 = it->second;
            store.loc = loc(node);
            emit(store);
            lastValue_ = rhs;
            return;
        }
        // Global variable: store directly via global name
        ir::Inst store;
        store.op = ir::Op::STORE;
        store.resultType = rhs.type;
        store.src1 = rhs;
        store.src2 = ir::Operand::global("_" + vr->name);
        store.loc = loc(node);
        emit(store);
        lastValue_ = rhs;
        return;
    }

    // Generic: evaluate target address, store
    node.target->accept(*this);
    auto addr = lastValue_;
    ir::Inst store;
    store.op = ir::Op::STORE;
    store.resultType = rhs.type;
    store.src1 = rhs;
    store.src2 = addr;
    store.loc = loc(node);
    emit(store);
    lastValue_ = rhs;
}

void IRBuilder::visit(BinaryOperation& node) {
    node.left->accept(*this);
    auto lhs = lastValue_;
    bool lhsSigned = lastValueSigned_;
    node.right->accept(*this);
    auto rhs = lastValue_;
    bool rhsSigned = lastValueSigned_;

    // For comparison operators: use signed ops only if BOTH operands are signed.
    // cc45 treats int as unsigned by default (isSigned=false).
    // Only explicitly `signed int` variables use signed comparison.
    bool bothSigned = lhsSigned && rhsSigned;

    ir::Op op = ir::Op::NOP;
    ir::Type resultType = lhs.type;

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
    else if (node.op == "==") { op = ir::Op::CMP_EQ; resultType = ir::Type::I8; }
    else if (node.op == "!=") { op = ir::Op::CMP_NE; resultType = ir::Type::I8; }
    else if (node.op == "<") { op = bothSigned ? ir::Op::CMP_LT : ir::Op::CMP_LTU; resultType = ir::Type::I8; }
    else if (node.op == "<=") { op = bothSigned ? ir::Op::CMP_LE : ir::Op::CMP_LEU; resultType = ir::Type::I8; }
    else if (node.op == ">") { op = bothSigned ? ir::Op::CMP_GT : ir::Op::CMP_GTU; resultType = ir::Type::I8; }
    else if (node.op == ">=") { op = bothSigned ? ir::Op::CMP_GE : ir::Op::CMP_GEU; resultType = ir::Type::I8; }
    else if (node.op == "&&") { op = ir::Op::AND; resultType = ir::Type::I8; }
    else if (node.op == "||") { op = ir::Op::OR; resultType = ir::Type::I8; }

    auto dest = allocVreg(resultType);
    ir::Inst inst;
    inst.op = op;
    inst.dest = dest;
    inst.resultType = resultType;
    inst.src1 = lhs;
    inst.src2 = rhs;
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
        // Address-of: src is already an address for locals
        lastValue_ = src;
    } else if (node.op == "*") {
        // Dereference
        auto dest = allocVreg(ir::Type::I16);
        ir::Inst inst;
        inst.op = ir::Op::LOAD;
        inst.dest = dest;
        inst.resultType = ir::Type::I16;
        inst.src1 = src;
        inst.loc = loc(node);
        emit(inst);
        lastValue_ = dest;
    } else if (node.op == "++" || node.op == "--") {
        auto one = allocVreg(src.type);
        ir::Inst ci;
        ci.op = ir::Op::CONST;
        ci.dest = one;
        ci.resultType = src.type;
        ci.src1 = ir::Operand::imm(1, src.type);
        emit(ci);
        auto dest = allocVreg(src.type);
        ir::Inst inst;
        inst.op = (node.op == "++") ? ir::Op::ADD : ir::Op::SUB;
        inst.dest = dest;
        inst.resultType = src.type;
        inst.src1 = src;
        inst.src2 = one;
        inst.loc = loc(node);
        emit(inst);
        lastValue_ = dest;
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
    // Evaluate arguments
    std::vector<ir::Operand> args;
    for (auto& arg : node.arguments) {
        arg->accept(*this);
        args.push_back(lastValue_);
    }

    ir::Inst inst;
    inst.args = args;
    inst.loc = loc(node);

    if (node.callExpr) {
        // Indirect call via function pointer
        node.callExpr->accept(*this);
        inst.src1 = lastValue_;
        auto dest = allocVreg(ir::Type::I16);
        inst.op = ir::Op::CALL_INDIRECT;
        inst.dest = dest;
        inst.resultType = ir::Type::I16;
        emit(inst);
        lastValue_ = dest;
    } else {
        inst.src1 = ir::Operand::global("_" + node.name);
        auto dest = allocVreg(ir::Type::I16);
        inst.op = ir::Op::CALL;
        inst.dest = dest;
        inst.resultType = ir::Type::I16;
        emit(inst);
        lastValue_ = dest;
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
        node.expression->accept(*this);
        ir::Inst ret;
        ret.op = ir::Op::RET;
        ret.src1 = lastValue_;
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
    ir::Inst brEnd1;
    brEnd1.op = ir::Op::BR;
    brEnd1.src1 = ir::Operand::label(endLabel);
    emit(brEnd1);

    startBlock(elseLabel);
    node.elseExpr->accept(*this);
    auto elseVal = lastValue_;
    ir::Inst brEnd2;
    brEnd2.op = ir::Op::BR;
    brEnd2.src1 = ir::Operand::label(endLabel);
    emit(brEnd2);

    startBlock(endLabel);
    auto dest = allocVreg(thenVal.type);
    ir::Inst phi;
    phi.op = ir::Op::PHI;
    phi.dest = dest;
    phi.resultType = thenVal.type;
    phi.phiIncoming = {{thenVal, thenLabel}, {elseVal, elseLabel}};
    phi.loc = loc(node);
    emit(phi);
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
    // Evaluate base address
    node.arrayExpr->accept(*this);
    auto base = lastValue_;

    // Evaluate index
    node.indexExpr->accept(*this);
    auto index = lastValue_;

    // Determine element size from array type
    int elemSize = 2; // default
    if (auto* ref = dynamic_cast<VariableReference*>(node.arrayExpr.get())) {
        auto dit = localArrayDims_.find(ref->name);
        if (dit != localArrayDims_.end() && !dit->second.empty()) {
            auto tit = localTypes_.find(ref->name);
            ir::Type elemType = (tit != localTypes_.end()) ? tit->second : ir::Type::I16;
            elemSize = ir::typeSize(elemType);
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

    // Load the element
    auto val = allocVreg(ir::Type::I16);
    ir::Inst load;
    load.op = ir::Op::LOAD;
    load.dest = val;
    load.resultType = ir::Type::I16;
    load.src1 = addr;
    load.loc = loc(node);
    emit(load);
    lastValue_ = val;
}

void IRBuilder::visit(MemberAccess& node) {
    // Evaluate struct expression
    node.structExpr->accept(*this);
    auto base = lastValue_;

    // Look up member info
    int memberOffset = 0;
    ir::Type memberType = ir::Type::I16;
    int bitWidth = 0;
    int bitOffset = 0;

    for (const auto& [sname, sinfo] : structs_) {
        auto mit = sinfo.members.find(node.memberName);
        if (mit != sinfo.members.end()) {
            memberOffset = mit->second.offset;
            memberType = mapType(mit->second.type, mit->second.pointerLevel);
            bitWidth = mit->second.bitWidth;
            bitOffset = mit->second.bitOffset;
            break;
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

    // Load the storage unit
    auto val = allocVreg(memberType);
    ir::Inst load;
    load.op = ir::Op::LOAD;
    load.dest = val;
    load.resultType = memberType;
    load.src1 = addr;
    load.loc = loc(node);
    emit(load);

    if (bitWidth > 0) {
        // Bitfield: extract the field from the loaded storage unit
        auto extracted = allocVreg(memberType);
        ir::Inst bfext;
        bfext.op = ir::Op::BFEXT;
        bfext.dest = extracted;
        bfext.resultType = memberType;
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
        // Aggregate: allocate a frame temp, initialize, return address
        auto temp = allocVreg(ir::Type::PTR);
        ir::Inst alloc;
        alloc.op = ir::Op::ADDR_LOCAL;
        alloc.dest = temp;
        alloc.resultType = ir::Type::PTR;
        alloc.src1 = ir::Operand::imm(0, ir::Type::I16); // frame offset (placeholder)
        alloc.loc = loc(node);
        emit(alloc);

        if (node.initializer) {
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
        // For now use default; a full implementation would call getExprType
        sz = 2;
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

void IRBuilder::visit(SwitchStatement& node) {
    node.expression->accept(*this);
    auto expr = lastValue_;

    std::string breakLabel = newLabel("switch_end");
    std::string defaultLabel = breakLabel; // default falls through to break if no default case

    SwitchCtx ctx;
    ctx.breakLabel = breakLabel;
    ctx.defaultLabel = defaultLabel;
    ctx.expr = expr;
    switchStack_.push_back(ctx);
    loopStack_.push_back({breakLabel, ""}); // break works in switch, continue does not

    // Pre-scan case values by visiting body (CaseStatement/DefaultStatement will register themselves)
    node.body->accept(*this);

    // Emit the switch dispatch at the end (after all case labels are known)
    // Note: cases were already registered during body visit
    auto& sw = switchStack_.back();
    if (!sw.cases.empty()) {
        // The SWITCH instruction is informational — actual comparison+branch was emitted inline by CaseStatement
    }

    loopStack_.pop_back();
    switchStack_.pop_back();
    startBlock(breakLabel);
}

void IRBuilder::visit(CaseStatement& node) {
    if (switchStack_.empty()) return;

    // Evaluate case value
    int64_t caseVal = 0;
    if (auto* lit = dynamic_cast<IntegerLiteral*>(node.value.get())) {
        caseVal = lit->value;
    }

    std::string caseLabel = newLabel("case");
    switchStack_.back().cases.push_back({caseVal, caseLabel});

    // Emit comparison: if expr == caseVal, branch to case body
    auto dest = allocVreg(ir::Type::I8);
    ir::Inst cmp;
    cmp.op = ir::Op::CMP_EQ;
    cmp.dest = dest;
    cmp.resultType = ir::Type::I8;
    cmp.src1 = switchStack_.back().expr;
    cmp.src2 = ir::Operand::imm(caseVal, switchStack_.back().expr.type);
    cmp.loc = loc(node);
    emit(cmp);

    std::string skipLabel = newLabel("case_skip");
    ir::Inst br;
    br.op = ir::Op::BR_COND;
    br.src1 = dest;
    br.dest = ir::Operand::label(caseLabel);
    br.src2 = ir::Operand::label(skipLabel);
    emit(br);

    startBlock(caseLabel);
    // Case body follows (emitted by CompoundStatement visit)

    // The skip block will be started by the next case or end of switch
    // We need to ensure it exists
    startBlock(skipLabel);
}

void IRBuilder::visit(DefaultStatement&) {
    if (switchStack_.empty()) return;

    std::string defaultLabel = newLabel("default");
    switchStack_.back().defaultLabel = defaultLabel;
    switchStack_.back().hasDefault = true;

    startBlock(defaultLabel);
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
        mi.arrayDims = m.arrayDims;
        mi.bitWidth = m.bitWidth;

        if (m.bitWidth > 0) {
            // Bitfield member
            int unitSize = getTypeSize(m.type, 0);
            int unitBits = unitSize * 8;

            // Start new unit if type changes or bits don't fit
            if (bfUnitType != m.type || bfBitsUsed + m.bitWidth > unitBits) {
                closeBitfieldUnit();
                // Align to unit size
                int align = std::max(1, unitSize);
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
                int align = std::max(1, getTypeSize(m.type, m.pointerLevel));
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

    // Find the last named param to compute its address
    auto paramIt = locals_.find(node.lastParamName);
    ir::Operand lastParamAddr;
    if (paramIt != locals_.end()) {
        lastParamAddr = paramIt->second;
    } else {
        lastParamAddr = ir::Operand::imm(0, ir::Type::PTR);
    }

    // ap = &last_param + sizeof(last_param)
    // For simplicity at IR level, emit as an opaque address computation
    auto dest = allocVreg(ir::Type::PTR);
    ir::Inst inst;
    inst.op = ir::Op::ADD;
    inst.dest = dest;
    inst.resultType = ir::Type::PTR;
    inst.src1 = lastParamAddr;
    inst.src2 = ir::Operand::imm(2, ir::Type::I16); // default param promotion size
    inst.loc = loc(node);
    emit(inst);

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

    // Load current ap value (pointer)
    auto apVal = allocVreg(ir::Type::PTR);
    ir::Inst loadAp;
    loadAp.op = ir::Op::LOAD;
    loadAp.dest = apVal;
    loadAp.resultType = ir::Type::PTR;
    loadAp.src1 = apAddr;
    loadAp.loc = loc(node);
    emit(loadAp);

    // Load the argument from *ap
    auto val = allocVreg(argType);
    ir::Inst loadVal;
    loadVal.op = ir::Op::LOAD;
    loadVal.dest = val;
    loadVal.resultType = argType;
    loadVal.src1 = apVal;
    loadVal.loc = loc(node);
    emit(loadVal);

    // Advance ap: ap += argSize
    auto newAp = allocVreg(ir::Type::PTR);
    ir::Inst advance;
    advance.op = ir::Op::ADD;
    advance.dest = newAp;
    advance.resultType = ir::Type::PTR;
    advance.src1 = apVal;
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
