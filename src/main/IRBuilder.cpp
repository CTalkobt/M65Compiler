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
        module_.globals.push_back(gv);
        return;
    }

    // Local variable — allocate a vReg
    auto vreg = allocVreg(t);
    locals_[node.name] = vreg;
    localTypes_[node.name] = t;

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
    node.right->accept(*this);
    auto rhs = lastValue_;

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
    else if (node.op == "<") { op = ir::Op::CMP_LT; resultType = ir::Type::I8; }
    else if (node.op == "<=") { op = ir::Op::CMP_LE; resultType = ir::Type::I8; }
    else if (node.op == ">") { op = ir::Op::CMP_GT; resultType = ir::Type::I8; }
    else if (node.op == ">=") { op = ir::Op::CMP_GE; resultType = ir::Type::I8; }
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
    inst.src1 = ir::Operand::global("_" + node.name);
    inst.loc = loc(node);

    // Assume non-void return for now (we don't track return types of callees)
    auto dest = allocVreg(ir::Type::I16);
    inst.op = ir::Op::CALL;
    inst.dest = dest;
    inst.resultType = ir::Type::I16;
    emit(inst);
    lastValue_ = dest;
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

    startBlock(endLabel);
}

void IRBuilder::visit(DoWhileStatement& node) {
    std::string bodyLabel = newLabel("do_body");
    std::string condLabel = newLabel("do_cond");
    std::string endLabel = newLabel("do_end");

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

    startBlock(endLabel);
}

void IRBuilder::visit(ForStatement& node) {
    std::string condLabel = newLabel("for_cond");
    std::string bodyLabel = newLabel("for_body");
    std::string incLabel = newLabel("for_inc");
    std::string endLabel = newLabel("for_end");

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

    startBlock(endLabel);
}

// ============================================================================
// Stub visitors (TODO: implement in future IR phases)
// ============================================================================

void IRBuilder::visit(ConditionalExpression& node) {
    // TODO: ternary → BR_COND + PHI
    node.condition->accept(*this);
}

void IRBuilder::visit(GenericSelection&) { /* TODO */ }
void IRBuilder::visit(InitializerList&) { /* TODO */ }

void IRBuilder::visit(ArrayAccess& node) {
    // TODO: ADDR_ELEM + LOAD
    node.arrayExpr->accept(*this);
}

void IRBuilder::visit(MemberAccess& node) {
    // TODO: struct member offset calculation
    node.structExpr->accept(*this);
}

void IRBuilder::visit(CompoundLiteral& node) {
    // TODO: frame-allocated compound literal
    if (node.initializer) node.initializer->accept(*this);
}

void IRBuilder::visit(AlignofExpression&) {
    lastValue_ = ir::Operand::imm(1, ir::Type::I16);
}

void IRBuilder::visit(SizeofExpression& node) {
    int sz = 2;
    if (node.isType) sz = ir::typeSize(mapType(node.typeName, node.pointerLevel));
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
    // TODO: proper SWITCH instruction
    node.expression->accept(*this);
    node.body->accept(*this);
}
void IRBuilder::visit(CaseStatement&) { /* TODO */ }
void IRBuilder::visit(DefaultStatement&) { /* TODO */ }

void IRBuilder::visit(BreakStatement&) {
    // TODO: track break target label
}
void IRBuilder::visit(ContinueStatement&) {
    // TODO: track continue target label
}
void IRBuilder::visit(SwitchContinueStatement&) { /* TODO */ }

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
void IRBuilder::visit(EnumDefinition&) { /* type-level only */ }
void IRBuilder::visit(StructDefinition&) { /* type-level only */ }
void IRBuilder::visit(BuiltinVaStart&) { /* TODO */ }
void IRBuilder::visit(BuiltinVaArg&) { /* TODO */ }
