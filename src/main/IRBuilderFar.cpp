// IRBuilderFar.cpp — far pointers (#68): T __far *p holds a 28-bit address
// in 4 bytes, a scalar cc45 names "__farp T" (FarPointer.hpp). What it
// points to is read and written by FAR_LOAD / FAR_STORE (32-bit indirect
// addressing, [bp],z); *p, p[i], p->m and (far object).m are such far
// lvalues, &lvalue a far pointer; p ± n moves by elements, p - q counts them.
// A normal pointer converts to a far one (bank 0), a far one to a normal
// one by its low 16 bits.
//
// T __near *p is the same with a 1-byte offset into the program's base page
// ("__nearp T"): NEAR_LOAD / NEAR_STORE ($00,x), I8 address arithmetic (it
// stays in the page); converted to a normal or far pointer by NEAR_ADDR (the
// base page register B its high byte), to a near one from a normal pointer
// by its low byte. FarElem::near says which kind of pointer reached it.
#include "IRBuilder.hpp"
#include "Diagnostic.hpp"

static std::string farPtrName(const std::string& type, bool isSigned, bool near = false) {
    return std::string(near ? "__nearp " : "__farp ") + (isSigned ? "signed " : "") + type;
}

// The far or near pointer type of a value ("" for another type)
std::string IRBuilder::farTypeOf(Expression* e) {
    if (!farUsed_ || !e) return "";
    IRTypeInfo ti = getExprTypeInfo(e);
    if (ti.type == ir::Type::I32 && isFarPointerType(ti.typeName)) return ti.typeName;
    if (ti.type == ir::Type::I8 && isNearPointerType(ti.typeName)) return ti.typeName;
    return "";
}

IRBuilder::FarElem IRBuilder::farPointeeOf(const std::string& farType) {
    FarElem el;
    el.type = farPointee(farType, &el.isSigned);
    el.near = isNearPointerType(farType);
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
    if (!el.dims.empty()) {
        if (el.near) return {ir::Type::I8, farPtrName(el.type, el.isSigned, true), false, ir::Type::VOID, 1};
        return {ir::Type::I32, farPtrName(el.type, el.isSigned), false, ir::Type::VOID, 4};
    }
    if (el.ptrLevel > 0) {
        IRTypeInfo t{ir::Type::PTR, el.type, el.isSigned, mapType(el.type, el.ptrLevel - 1), 2};
        t.ptrDepth = el.ptrLevel;
        return t;
    }
    return {mapType(el.type, 0), el.type, el.isSigned, ir::Type::VOID, getTypeSize(el.type, 0)};
}

// e's value as a 32-bit far address (an 8-bit near one)
ir::Operand IRBuilder::farValue(Expression* e, bool near) {
    bool saved = computeAddressOnly_;
    computeAddressOnly_ = false;
    e->accept(*this);
    computeAddressOnly_ = saved;
    return emitCast(lastValue_, near ? ir::Type::I8 : ir::Type::I32, false);
}

ir::Operand IRBuilder::farOffset(const ir::Operand& base, const ir::Operand& offset, bool subtract, ASTNode& at,
                                 bool near) {
    if (offset.isImm() && offset.immVal == 0) return base;
    ir::Type t = near ? ir::Type::I8 : ir::Type::I32;
    auto r = allocVreg(t);
    ir::Inst add;
    add.op = subtract ? ir::Op::SUB : ir::Op::ADD;
    add.dest = r;
    add.resultType = t;
    add.src1 = base;
    add.src2 = offset;
    add.loc = loc(at);
    emit(add);
    return r;
}

// An integer expression times an element size, as a 32-bit offset (8-bit:
// near)
ir::Operand IRBuilder::farScaled(Expression* index, int size, ASTNode& at, bool near) {
    bool saved = computeAddressOnly_;
    computeAddressOnly_ = false;
    index->accept(*this);
    computeAddressOnly_ = saved;
    ir::Type t = near ? ir::Type::I8 : ir::Type::I32;
    ir::Operand i = emitCast(lastValue_, t, getExprTypeInfo(index).isSigned);
    if (size == 1) return i;
    if (i.isImm()) return ir::Operand::imm(near ? (i.immVal * size) & 0xFF : i.immVal * size, t);
    auto r = allocVreg(t);
    ir::Inst mul;
    mul.op = ir::Op::MUL;
    mul.dest = r;
    mul.resultType = t;
    mul.src1 = i;
    mul.src2 = ir::Operand::imm(size, t);
    mul.loc = loc(at);
    emit(mul);
    return r;
}

// A __bank(n) variable (#69): kept in far memory, reached through its
// address in a hidden static global of the file (.dword <sym>, R_LINEAR32)
void IRBuilder::declareBanked(VariableDeclaration& node) {
    farUsed_ = true;
    BankedVar bv;
    bv.ref = "__fa_" + node.name;
    bv.elem.type = node.type;
    bv.elem.ptrLevel = node.pointerLevel;
    bv.elem.isSigned = node.isSigned;
    bv.elem.dims = node.arrayDims;
    if (!bankedVars_.count(node.name)) {
        ir::Module::GlobalVar ref;
        ref.name = bv.ref;
        ref.type = ir::Type::I32;
        ref.size = 4;
        ref.isStatic = true;
        ref.isConst = true;
        ref.hasInitValue = true;
        ref.initLabels.push_back("_" + node.name);
        ref.farLabel = true;
        module_.globals.push_back(ref);
    }
    bankedVars_[node.name] = bv;
}

// Whether e is an object reached through a far pointer: its element (and,
// with addr, the code computing its 32-bit address)
bool IRBuilder::farLvalue(Expression* e, FarElem& el, ir::Operand* addr) {
    if (!farUsed_ || !e) return false;
    if (auto* v = dynamic_cast<VariableReference*>(e)) {
        // a __bank(n) variable (not a local of the name)
        auto it = bankedVars_.find(v->name);
        if (it == bankedVars_.end() || locals_.count(v->name)) return false;
        el = it->second.elem;
        if (addr) {
            auto a = allocVreg(ir::Type::I32);
            ir::Inst ld;
            ld.op = ir::Op::LOAD;
            ld.dest = a;
            ld.resultType = ir::Type::I32;
            ld.src1 = ir::Operand::global(it->second.ref);
            ld.loc = loc(*e);
            emit(ld);
            *addr = a;
        }
        return true;
    }
    if (auto* u = dynamic_cast<UnaryOperation*>(e); u && u->op == "*") {
        std::string ft = farTypeOf(u->operand.get());
        if (ft.empty()) return false;
        el = farPointeeOf(ft);
        if (addr) *addr = farValue(u->operand.get(), el.near);
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
            ir::Operand b = farValue(base, el.near);
            *addr = farOffset(b, farScaled(index, farElemSize(el), *e, el.near), false, *e, el.near);
        }
        return true;
    }
    if (auto* m = dynamic_cast<MemberAccess*>(e)) {
        std::string sname;
        ir::Operand ba;
        bool near;
        if (m->isArrow) {
            std::string ft = farTypeOf(m->structExpr.get());
            if (ft.empty()) return false;
            sname = farPointee(ft);
            near = isNearPointerType(ft);
            if (addr) ba = farValue(m->structExpr.get(), near);
        } else {
            FarElem base;
            if (!farLvalue(m->structExpr.get(), base, addr ? &ba : nullptr)) return false;
            if (base.ptrLevel || !base.dims.empty()) return false;
            sname = base.type;
            near = base.near;
        }
        int off = 0;
        const IRMemberInfo* mi = findStructMember(getAggregateName(sname), m->memberName, off);
        if (!mi) return false;
        el.type = mi->type;
        el.ptrLevel = mi->pointerLevel;
        el.isSigned = mi->isSigned;
        el.dims = mi->arrayDims;
        el.bitWidth = mi->bitWidth;
        el.near = near;
        if (addr) *addr = farOffset(ba, ir::Operand::imm(off + mi->offset, near ? ir::Type::I8 : ir::Type::I32), false,
                                    *e, near);
        return true;
    }
    return false;
}

// getExprTypeInfo of a far lvalue or of &lvalue
bool IRBuilder::farExprTypeInfo(Expression* e, IRTypeInfo& out) {
    if (!farUsed_) return false;
    FarElem el;
    if (farLvalue(e, el, nullptr)) {
        out = farElemInfo(el);
        // (an array: its size, as for a string literal; sizeof reads it)
        if (!el.dims.empty()) out.totalSize = farElemSize(el);
        return true;
    }
    if (auto* u = dynamic_cast<UnaryOperation*>(e); u && u->op == "&" && farLvalue(u->operand.get(), el, nullptr) &&
        el.ptrLevel == 0) {
        if (el.near) out = {ir::Type::I8, farPtrName(el.type, el.isSigned, true), false, ir::Type::VOID, 1};
        else out = {ir::Type::I32, farPtrName(el.type, el.isSigned), false, ir::Type::VOID, 4};
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
    ld.op = el.near ? ir::Op::NEAR_LOAD : ir::Op::FAR_LOAD;
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
    st.op = el.near ? ir::Op::NEAR_STORE : ir::Op::FAR_STORE;
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
        auto r = farTempRef(a, el.near ? ir::Type::I8 : ir::Type::I32, farPtrName(el.type, el.isSigned, el.near), 0, false, at);
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
        if (node.op == "+") return farError(node, "two far or near pointers cannot be added");
        bool near = isNearPointerType(lf);
        if (near != isNearPointerType(rf)) return farError(node, "a far and a near pointer cannot be subtracted");
        // (near: the offsets as ints, the difference signed)
        ir::Type t = near ? ir::Type::I16 : ir::Type::I32;
        ir::Operand a = emitCast(farValue(node.left.get(), near), t, false);
        ir::Operand b = emitCast(farValue(node.right.get(), near), t, false);
        auto d = allocVreg(t);
        ir::Inst sub;
        sub.op = ir::Op::SUB;
        sub.dest = d;
        sub.resultType = t;
        sub.src1 = a;
        sub.src2 = b;
        sub.loc = loc(node);
        emit(sub);
        int size = farElemSize(farPointeeOf(lf));
        if (size > 1) {
            auto q = allocVreg(t);
            ir::Inst div;
            div.op = ir::Op::DIV;
            div.dest = q;
            div.resultType = t;
            div.src1 = d;
            div.src2 = ir::Operand::imm(size, t);
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
    FarElem el = farPointeeOf(lf.empty() ? rf : lf);
    ir::Operand base = farValue(p, el.near);
    lastValue_ = farOffset(base, farScaled(n, farElemSize(el), node, el.near), node.op == "-", node, el.near);
    return true;
}

// A near pointer as a normal (or far) one: its base-page address
bool IRBuilder::emitNearCast(CastExpression& node) {
    bool toFar = node.pointerLevel == 0 && isFarPointerType(node.targetType);
    if (node.pointerLevel == 0 && !toFar) return false;
    std::string nt = farTypeOf(node.expression.get());
    if (!isNearPointerType(nt)) return false;
    ir::Operand off = farValue(node.expression.get(), true);
    auto a = allocVreg(ir::Type::PTR);
    ir::Inst na;
    na.op = ir::Op::NEAR_ADDR;
    na.dest = a;
    na.resultType = ir::Type::PTR;
    na.src1 = off;
    na.loc = loc(node);
    emit(na);
    lastValue_ = toFar ? emitCast(a, ir::Type::I32, false) : a;
    return true;
}

// e converted for a pointer or far pointer target: a near pointer value
// wrapped in a cast (its base-page address)
void IRBuilder::nearConvertTo(std::unique_ptr<Expression>& e, const std::string& targetType, int ptr) {
    if (!farUsed_ || !e || (ptr == 0 && !isFarPointerType(targetType))) return;
    std::string ft = farTypeOf(e.get());
    // a far pointer for a normal one: its low 16 bits (a cast says so)
    if (ptr > 0 && isFarPointerType(ft))
        warnings_.push_back(formatDiagnostic(e->sourceFile, e->line, e->column, Severity::Warning,
            "a far pointer converted to a 16-bit pointer keeps its low 16 bits (cast it to say so)"));
    if (!isNearPointerType(ft)) return;
    int line = e->line, col = e->column;
    std::string file = e->sourceFile;
    e = std::make_unique<CastExpression>(ptr ? "void" : targetType, ptr ? 1 : 0, false, std::move(e));
    e->line = line; e->column = col; e->sourceFile = file;
}
