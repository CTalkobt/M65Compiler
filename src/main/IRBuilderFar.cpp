// IRBuilderFar.cpp — far pointers (#68): T __far *p holds a 28-bit address
// in 4 bytes, a scalar cc45 names "__farp T" (FarPointer.hpp). What it
// points to is read and written by FAR_LOAD / FAR_STORE (32-bit indirect
// addressing, [bp],z); *p, p[i], p->m and (far object).m are such far
// lvalues, &lvalue a far pointer; p ± n moves by elements, p - q counts them.
// A near pointer converts to a far one (bank 0), a far one to a near one by
// its low 16 bits.
#include "IRBuilder.hpp"
#include "Diagnostic.hpp"

static std::string farPtrName(const std::string& type, bool isSigned) {
    return (isSigned ? "__farp signed " : "__farp ") + type;
}

// The far pointer type of a value ("" for another type)
std::string IRBuilder::farTypeOf(Expression* e) {
    if (!farUsed_ || !e) return "";
    IRTypeInfo ti = getExprTypeInfo(e);
    return ti.type == ir::Type::I32 && isFarPointerType(ti.typeName) ? ti.typeName : "";
}

IRBuilder::FarElem IRBuilder::farPointeeOf(const std::string& farType) {
    FarElem el;
    el.type = farPointee(farType, &el.isSigned);
    return el;
}

// A struct or union (not a pointer to one, not an array of them)
static bool farAggregate(const std::string& type, int ptrLevel, size_t dims) {
    return ptrLevel == 0 && dims == 0 && (type.rfind("struct ", 0) == 0 || type.rfind("union ", 0) == 0);
}

int IRBuilder::farElemSize(const FarElem& el) {
    int n = getTypeSize(el.type, el.ptrLevel);
    for (int d : el.dims) n *= d;
    return n;
}

// The type of a far lvalue's value: an array member decays to a far pointer
// to its elements
IRBuilder::IRTypeInfo IRBuilder::farElemInfo(const FarElem& el) {
    if (!el.dims.empty()) return {ir::Type::I32, farPtrName(el.type, el.isSigned), false, ir::Type::VOID, 4};
    if (el.ptrLevel > 0) {
        IRTypeInfo t{ir::Type::PTR, el.type, el.isSigned, mapType(el.type, el.ptrLevel - 1), 2};
        t.ptrDepth = el.ptrLevel;
        return t;
    }
    return {mapType(el.type, 0), el.type, el.isSigned, ir::Type::VOID, getTypeSize(el.type, 0)};
}

// e's value as a 32-bit far address
ir::Operand IRBuilder::farValue(Expression* e) {
    bool saved = computeAddressOnly_;
    computeAddressOnly_ = false;
    e->accept(*this);
    computeAddressOnly_ = saved;
    return emitCast(lastValue_, ir::Type::I32, false);
}

ir::Operand IRBuilder::farOffset(const ir::Operand& base, const ir::Operand& offset, bool subtract, ASTNode& at) {
    if (offset.isImm() && offset.immVal == 0) return base;
    auto r = allocVreg(ir::Type::I32);
    ir::Inst add;
    add.op = subtract ? ir::Op::SUB : ir::Op::ADD;
    add.dest = r;
    add.resultType = ir::Type::I32;
    add.src1 = base;
    add.src2 = offset;
    add.loc = loc(at);
    emit(add);
    return r;
}

// An integer expression times an element size, as a 32-bit offset
ir::Operand IRBuilder::farScaled(Expression* index, int size, ASTNode& at) {
    bool saved = computeAddressOnly_;
    computeAddressOnly_ = false;
    index->accept(*this);
    computeAddressOnly_ = saved;
    ir::Operand i = emitCast(lastValue_, ir::Type::I32, getExprTypeInfo(index).isSigned);
    if (size == 1) return i;
    if (i.isImm()) return ir::Operand::imm(i.immVal * size, ir::Type::I32);
    auto r = allocVreg(ir::Type::I32);
    ir::Inst mul;
    mul.op = ir::Op::MUL;
    mul.dest = r;
    mul.resultType = ir::Type::I32;
    mul.src1 = i;
    mul.src2 = ir::Operand::imm(size, ir::Type::I32);
    mul.loc = loc(at);
    emit(mul);
    return r;
}

// Whether e is an object reached through a far pointer: its element (and,
// with addr, the code computing its 32-bit address)
bool IRBuilder::farLvalue(Expression* e, FarElem& el, ir::Operand* addr) {
    if (!farUsed_ || !e) return false;
    if (auto* u = dynamic_cast<UnaryOperation*>(e); u && u->op == "*") {
        std::string ft = farTypeOf(u->operand.get());
        if (ft.empty()) return false;
        el = farPointeeOf(ft);
        if (addr) *addr = farValue(u->operand.get());
        return true;
    }
    if (auto* a = dynamic_cast<ArrayAccess*>(e)) {
        std::string ft = farTypeOf(a->arrayExpr.get());
        Expression* base = a->arrayExpr.get();
        Expression* index = a->indexExpr.get();
        if (ft.empty()) {   // (i[p])
            ft = farTypeOf(a->indexExpr.get());
            std::swap(base, index);
        }
        if (ft.empty()) return false;
        el = farPointeeOf(ft);
        if (addr) {
            ir::Operand b = farValue(base);
            *addr = farOffset(b, farScaled(index, farElemSize(el), *e), false, *e);
        }
        return true;
    }
    if (auto* m = dynamic_cast<MemberAccess*>(e)) {
        std::string sname;
        ir::Operand ba;
        if (m->isArrow) {
            std::string ft = farTypeOf(m->structExpr.get());
            if (ft.empty()) return false;
            sname = farPointee(ft);
            if (addr) ba = farValue(m->structExpr.get());
        } else {
            FarElem base;
            if (!farLvalue(m->structExpr.get(), base, addr ? &ba : nullptr)) return false;
            if (base.ptrLevel || !base.dims.empty()) return false;
            sname = base.type;
        }
        int off = 0;
        const IRMemberInfo* mi = findStructMember(getAggregateName(sname), m->memberName, off);
        if (!mi) return false;
        el.type = mi->type;
        el.ptrLevel = mi->pointerLevel;
        el.isSigned = mi->isSigned;
        el.dims = mi->arrayDims;
        el.bitWidth = mi->bitWidth;
        if (addr) *addr = farOffset(ba, ir::Operand::imm(off + mi->offset, ir::Type::I32), false, *e);
        return true;
    }
    return false;
}

// getExprTypeInfo of a far lvalue or of &lvalue
bool IRBuilder::farExprTypeInfo(Expression* e, IRTypeInfo& out) {
    if (!farUsed_) return false;
    FarElem el;
    if (farLvalue(e, el, nullptr)) { out = farElemInfo(el); return true; }
    if (auto* u = dynamic_cast<UnaryOperation*>(e); u && u->op == "&" && farLvalue(u->operand.get(), el, nullptr) &&
        el.ptrLevel == 0) {
        out = {ir::Type::I32, farPtrName(el.type, el.isSigned), false, ir::Type::VOID, 4};
        return true;
    }
    return false;
}

bool IRBuilder::farError(ASTNode& at, const std::string& msg) {
    errors_.push_back(formatDiagnostic(at.sourceFile, at.line, at.column, Severity::Error, msg));
    lastValue_ = ir::Operand::imm(0, ir::Type::I16);
    return true;
}

// The value of a far lvalue (*p, p[i], p->m) or of &lvalue: true when e is one
bool IRBuilder::emitFarRead(Expression& e) {
    FarElem el;
    if (auto* u = dynamic_cast<UnaryOperation*>(&e); u && u->op == "&") {
        if (!farLvalue(u->operand.get(), el, nullptr)) return false;
        ir::Operand a;
        farLvalue(u->operand.get(), el, &a);
        lastValue_ = a;
        return true;
    }
    if (!farLvalue(&e, el, nullptr)) return false;
    if (el.bitWidth) return farError(e, "a bit-field through a far pointer is not supported");
    if (farAggregate(el.type, el.ptrLevel, el.dims.size())) return farError(e, "a struct or union through a far pointer is not copied: use its members");
    ir::Operand a;
    farLvalue(&e, el, &a);
    if (!el.dims.empty()) {
        if (el.dims.size() > 1) return farError(e, "a multi-dimensional array through a far pointer is not supported");
        lastValue_ = a;   // (an array: its address)
        return true;
    }
    if (computeAddressOnly_)
        return farError(e, "an object reached through a far pointer has no near address");
    IRTypeInfo ti = farElemInfo(el);
    if (ti.type != ir::Type::I8 && ti.type != ir::Type::I16 && ti.type != ir::Type::I32 && ti.type != ir::Type::PTR)
        return farError(e, "only integers and pointers can be read through a far pointer (not '" + el.type + "')");
    auto d = allocVreg(ti.type);
    ir::Inst ld;
    ld.op = ir::Op::FAR_LOAD;
    ld.dest = d;
    ld.resultType = ti.type;
    ld.src1 = a;
    ld.loc = loc(e);
    emit(ld);
    lastValue_ = d;
    return true;
}

// A hidden local holding an operand (vecTemp's), as a variable reference
std::unique_ptr<Expression> IRBuilder::farTempRef(const ir::Operand& v, ir::Type t, const std::string& typeName,
                                                  int ptrLevel, bool sg, ASTNode& at) {
    auto r = std::make_unique<VariableReference>(vecTemp(v, t, typeName, ptrLevel, sg));
    r->line = at.line; r->column = at.column; r->sourceFile = at.sourceFile;
    return r;
}

// target = v / target op= v with target a far lvalue: true when it is one
bool IRBuilder::emitFarAssign(Assignment& node) {
    FarElem el;
    Expression* target = node.target.get();
    if (!farLvalue(target, el, nullptr)) {
        // a far pointer: p += n, p -= n (moved by elements)
        if ((node.op == "+=" || node.op == "-=") && !farTypeOf(target).empty()) {
            auto copy = cloneSimpleExpr(target);
            if (!copy) return farError(node, "'" + node.op + "' on a far pointer with side effects: write p = p " +
                                       node.op.substr(0, 1) + " n");
            auto bin = std::make_unique<BinaryOperation>(node.op.substr(0, 1), std::move(copy), std::move(node.expression));
            bin->line = node.line; bin->column = node.column; bin->sourceFile = node.sourceFile;
            node.expression = std::move(bin);
            node.op = "=";
        }
        return false;
    }
    if (el.bitWidth) return farError(node, "a bit-field through a far pointer is not supported");
    if (farAggregate(el.type, el.ptrLevel, el.dims.size())) return farError(node, "a struct or union through a far pointer is not copied: use its members");
    IRTypeInfo ti = farElemInfo(el);
    if (!el.dims.empty() || (ti.type != ir::Type::I8 && ti.type != ir::Type::I16 && ti.type != ir::Type::I32 &&
                             ti.type != ir::Type::PTR))
        return farError(node, "only integers and pointers can be written through a far pointer (not '" + el.type + "')");
    if (node.op != "=") {
        // target op= v: *t = *t op v, t the address
        if (el.ptrLevel) return farError(node, "'" + node.op + "' on a pointer reached through a far pointer is not supported");
        ir::Operand a;
        farLvalue(target, el, &a);
        auto deref = [&]() {
            auto u = std::make_unique<UnaryOperation>("*", farTempRefOnce(a, el, node));
            u->line = node.line; u->column = node.column; u->sourceFile = node.sourceFile;
            return u;
        };
        farTempName_.clear();
        auto lhs = deref();
        auto bin = std::make_unique<BinaryOperation>(node.op.substr(0, node.op.size() - 1), deref(), std::move(node.expression));
        bin->line = node.line; bin->column = node.column; bin->sourceFile = node.sourceFile;
        BinaryOperation* b = bin.get();
        Assignment as(std::move(lhs), std::move(bin), "=");
        as.line = node.line; as.column = node.column; as.sourceFile = node.sourceFile;
        as.accept(*this);
        node.expression = std::move(b->right);   // (the AST stays whole)
        return true;
    }
    ir::Operand a;
    farLvalue(target, el, &a);
    // the value converted as a cast to the object's type does it (_Bool 0/1,
    // fixed point scaled)
    CastExpression conv(el.type, el.ptrLevel, el.isSigned, std::move(node.expression));
    conv.line = node.line; conv.column = node.column; conv.sourceFile = node.sourceFile;
    bool saved = computeAddressOnly_;
    computeAddressOnly_ = false;
    conv.accept(*this);
    computeAddressOnly_ = saved;
    node.expression = std::move(conv.expression);   // (the AST stays whole)
    ir::Operand v = emitCast(lastValue_, ti.type, el.isSigned);
    ir::Inst st;
    st.op = ir::Op::FAR_STORE;
    st.resultType = ti.type;
    st.src1 = v;
    st.src2 = a;
    st.loc = loc(node);
    emit(st);
    lastValue_ = v;
    return true;
}

// The hidden far pointer to a far lvalue (made once per operation)
std::unique_ptr<Expression> IRBuilder::farTempRefOnce(const ir::Operand& a, const FarElem& el, ASTNode& at) {
    if (farTempName_.empty()) {
        auto r = farTempRef(a, ir::Type::I32, farPtrName(el.type, el.isSigned), 0, false, at);
        farTempName_ = static_cast<VariableReference*>(r.get())->name;
        return r;
    }
    auto r = std::make_unique<VariableReference>(farTempName_);
    r->line = at.line; r->column = at.column; r->sourceFile = at.sourceFile;
    return r;
}

// ++/-- of a far lvalue or of a far pointer: true when the operand is one
bool IRBuilder::emitFarIncDec(UnaryOperation& node) {
    bool post = node.op.size() > 2;
    std::string op = node.op.substr(0, 1);
    FarElem el;
    Expression* target = node.operand.get();
    bool lv = farLvalue(target, el, nullptr);
    std::string ft = lv ? "" : farTypeOf(target);
    if (!lv && ft.empty()) return false;
    std::unique_ptr<Expression> place, place2;   // the operand, twice
    if (lv) {
        if (el.ptrLevel || !el.dims.empty() || el.bitWidth)
            return farError(node, "++/-- of this object through a far pointer is not supported");
        ir::Operand a;
        farLvalue(target, el, &a);
        farTempName_.clear();
        place = std::make_unique<UnaryOperation>("*", farTempRefOnce(a, el, node));
        place2 = std::make_unique<UnaryOperation>("*", farTempRefOnce(a, el, node));
    } else {
        place = cloneSimpleExpr(target);
        place2 = cloneSimpleExpr(target);
        if (!place || !place2) return farError(node, "++/-- of a far pointer with side effects: write p = p " + op + " 1");
    }
    for (auto* p : {place.get(), place2.get()}) { p->line = node.line; p->column = node.column; p->sourceFile = node.sourceFile; }
    IRTypeInfo ti = getExprTypeInfo(place.get());
    ir::Operand old;
    if (post) {
        // the old value, kept in a hidden local (a variable's value is its
        // own slot, which the assignment changes)
        place->accept(*this);
        place = farTempRef(lastValue_, ti.type, ti.typeName, 0, ti.isSigned, node);
        old = locals_[static_cast<VariableReference*>(place.get())->name];
    }
    auto one = std::make_unique<IntegerLiteral>(1);
    auto bin = std::make_unique<BinaryOperation>(op, std::move(place), std::move(one));
    bin->line = node.line; bin->column = node.column; bin->sourceFile = node.sourceFile;
    Assignment as(std::move(place2), std::move(bin), "=");
    as.line = node.line; as.column = node.column; as.sourceFile = node.sourceFile;
    as.accept(*this);
    if (post) lastValue_ = old;
    return true;
}

// p ± n, n + p (moved by elements) and p - q (the elements between): true
// when an operand is a far pointer
bool IRBuilder::emitFarArith(BinaryOperation& node) {
    if (node.op != "+" && node.op != "-") return false;
    std::string lf = farTypeOf(node.left.get()), rf = farTypeOf(node.right.get());
    if (lf.empty() && rf.empty()) return false;
    if (!lf.empty() && !rf.empty()) {
        if (node.op == "+") return farError(node, "two far pointers cannot be added");
        ir::Operand a = farValue(node.left.get());
        ir::Operand b = farValue(node.right.get());
        ir::Operand d = farOffset(a, b, true, node);
        int size = farElemSize(farPointeeOf(lf));
        if (size > 1) {
            auto q = allocVreg(ir::Type::I32);
            ir::Inst div;
            div.op = ir::Op::DIV;
            div.dest = q;
            div.resultType = ir::Type::I32;
            div.src1 = d;
            div.src2 = ir::Operand::imm(size, ir::Type::I32);
            div.loc = loc(node);
            emit(div);
            d = q;
        }
        lastValue_ = d;
        return true;
    }
    if (lf.empty() && node.op == "-") return farError(node, "a far pointer cannot be subtracted from an integer");
    Expression* p = lf.empty() ? node.right.get() : node.left.get();
    Expression* n = lf.empty() ? node.left.get() : node.right.get();
    ir::Operand base = farValue(p);
    lastValue_ = farOffset(base, farScaled(n, farElemSize(farPointeeOf(lf.empty() ? rf : lf)), node), node.op == "-", node);
    return true;
}
