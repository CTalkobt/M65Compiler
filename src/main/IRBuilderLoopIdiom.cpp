// IRBuilder: copy and fill loops as memmove / memset (Issue #323)
//
//     for (iv = A; iv < B; iv++) { T[iv + c] = S[iv + d]; U[iv] = 0; ... }
//
// becomes
//
//     iv = A;
//     if (iv < B) { memmove(&T[iv + c], &S[iv + d], (B - iv) * sizeof T[0]);
//                   memset(&U[iv], 0, (B - iv) * sizeof U[0]); iv = B; }
//
// The loop: iv a local integer stepped by ++ or --, a condition iv <, <=,
// >, >= B, A and B made of constants and variables (no array elements,
// no calls), so the count is known before it runs. The body: only element
// copies and fills of arrays (not pointers) at iv + constant, each target a
// different array; a copy within one array (a shift) only in the direction
// memmove gives the loop's result (T[i] = T[i + 1] going up, T[i] = T[i - 1]
// going down); an inner loop over a whole row (for (k = 0; k < W; k++)
// T[iv][k] = S[iv][k], T's rows W elements) is one row element. The
// memmove / memset routines themselves keep their loops.
#include "IRBuilder.hpp"
#include <map>
#include <set>

namespace {

template <typename T>
std::unique_ptr<T> at(std::unique_ptr<T> n, const ASTNode& from) {
    n->line = from.line;
    n->column = from.column;
    n->sourceFile = from.sourceFile;
    return n;
}

// A copy of a bound, index or array base (the forms the pattern allows);
// null for anything else
std::unique_ptr<Expression> cloneSimple(Expression* e) {
    if (!e) return nullptr;
    if (auto* l = dynamic_cast<IntegerLiteral*>(e)) {
        auto c = at(std::make_unique<IntegerLiteral>(l->value), *e);
        c->castType = l->castType;
        c->castPointerLevel = l->castPointerLevel;
        c->castIsSigned = l->castIsSigned;
        return c;
    }
    if (auto* v = dynamic_cast<VariableReference*>(e)) return at(std::make_unique<VariableReference>(v->name), *e);
    if (auto* b = dynamic_cast<BinaryOperation*>(e)) {
        if (b->op != "+" && b->op != "-" && b->op != "*" && b->op != "/" && b->op != "<<" && b->op != ">>" &&
            b->op != "&" && b->op != "|")
            return nullptr;
        auto l = cloneSimple(b->left.get()), r = cloneSimple(b->right.get());
        if (!l || !r) return nullptr;
        return at(std::make_unique<BinaryOperation>(b->op, std::move(l), std::move(r)), *e);
    }
    if (auto* u = dynamic_cast<UnaryOperation*>(e)) {
        if (u->op != "-" && u->op != "+" && u->op != "~") return nullptr;
        auto o = cloneSimple(u->operand.get());
        if (!o) return nullptr;
        return at(std::make_unique<UnaryOperation>(u->op, std::move(o)), *e);
    }
    if (auto* c = dynamic_cast<CastExpression*>(e)) {
        auto o = cloneSimple(c->expression.get());
        if (!o) return nullptr;
        return at(std::make_unique<CastExpression>(c->targetType, c->pointerLevel, c->isSigned, std::move(o)), *e);
    }
    if (auto* s = dynamic_cast<SizeofExpression*>(e)) {
        if (s->isType) {
            auto c = at(std::make_unique<SizeofExpression>(s->typeName, s->pointerLevel), *e);
            c->arrayDims = s->arrayDims;
            c->declDims = s->declDims;
            return c;
        }
        auto o = cloneSimple(s->expression.get());
        if (!o) return nullptr;
        return at(std::make_unique<SizeofExpression>(std::move(o)), *e);
    }
    if (auto* m = dynamic_cast<MemberAccess*>(e)) {
        auto o = cloneSimple(m->structExpr.get());
        if (!o) return nullptr;
        return at(std::make_unique<MemberAccess>(std::move(o), m->memberName, m->isArrow), *e);
    }
    return nullptr;
}

// the names an expression reads; whether it has an array element
struct Reads : public ASTWalker {
    std::set<std::string> names;
    bool element = false;
    using ASTWalker::visit;
    void visit(VariableReference& n) override { names.insert(n.name); }
    void visit(ArrayAccess& n) override { element = true; ASTWalker::visit(n); }
};

// iv, iv + c, iv - c, c + iv: c
bool offsetOf(Expression* e, const std::string& iv, long& c) {
    if (auto* v = dynamic_cast<VariableReference*>(e)) { if (v->name != iv) return false; c = 0; return true; }
    auto* b = dynamic_cast<BinaryOperation*>(e);
    if (!b || (b->op != "+" && b->op != "-")) return false;
    auto* l = dynamic_cast<VariableReference*>(b->left.get());
    auto* r = dynamic_cast<IntegerLiteral*>(b->right.get());
    if (l && r && l->name == iv) { c = b->op == "+" ? (long)r->value : -(long)r->value; return true; }
    auto* ll = dynamic_cast<IntegerLiteral*>(b->left.get());
    auto* rr = dynamic_cast<VariableReference*>(b->right.get());
    if (ll && rr && rr->name == iv && b->op == "+") { c = (long)ll->value; return true; }
    return false;
}

// a printed array base (x, s.a, p->a) to compare bases
std::string baseKey(Expression* e) {
    if (auto* v = dynamic_cast<VariableReference*>(e)) return v->name;
    if (auto* m = dynamic_cast<MemberAccess*>(e)) {
        std::string k = baseKey(m->structExpr.get());
        return k.empty() ? "" : k + (m->isArrow ? "->" : ".") + m->memberName;
    }
    return "";
}

std::string rootName(Expression* e) {
    while (auto* m = dynamic_cast<MemberAccess*>(e)) e = m->structExpr.get();
    auto* v = dynamic_cast<VariableReference*>(e);
    return v ? v->name : "";
}

// an integer constant (a literal, negated, cast): its value
bool constValue(Expression* e, long& v) {
    bool neg = false;
    while (true) {
        if (auto* c = dynamic_cast<CastExpression*>(e)) { if (c->pointerLevel) return false; e = c->expression.get(); continue; }
        if (auto* u = dynamic_cast<UnaryOperation*>(e); u && u->op == "-") { neg = !neg; e = u->operand.get(); continue; }
        break;
    }
    auto* l = dynamic_cast<IntegerLiteral*>(e);
    if (!l) return false;
    v = neg ? -(long)l->value : (long)l->value;
    return true;
}

// for (iv = A / type iv = A; iv <op> B; iv++ / iv-- / ++iv / --iv)
bool loopHeader(ForStatement& f, std::string& iv, Expression*& start, std::string& op, Expression*& bound, int& step) {
    if (auto* es = dynamic_cast<ExpressionStatement*>(f.initializer.get())) {
        auto* as = dynamic_cast<Assignment*>(es->expression.get());
        auto* v = as ? dynamic_cast<VariableReference*>(as->target.get()) : nullptr;
        if (!v || as->op != "=") return false;
        iv = v->name;
        start = as->expression.get();
    } else if (auto* vd = dynamic_cast<VariableDeclaration*>(f.initializer.get())) {
        if (!vd->initializer || vd->pointerLevel || !vd->arrayDims.empty()) return false;
        iv = vd->name;
        start = vd->initializer.get();
    } else {
        return false;
    }
    auto* u = dynamic_cast<UnaryOperation*>(f.increment.get());
    auto* uv = u ? dynamic_cast<VariableReference*>(u->operand.get()) : nullptr;
    if (!uv || uv->name != iv) return false;
    if (u->op == "++" || u->op == "++_POST") step = 1;
    else if (u->op == "--" || u->op == "--_POST") step = -1;
    else return false;
    auto* c = dynamic_cast<BinaryOperation*>(f.condition.get());
    auto* cv = c ? dynamic_cast<VariableReference*>(c->left.get()) : nullptr;
    if (!cv || cv->name != iv) return false;
    op = c->op;
    if (!(op == "!=" || (step > 0 && (op == "<" || op == "<=")) || (step < 0 && (op == ">" || op == ">=")))) return false;
    bound = c->right.get();
    return true;
}

}  // namespace

bool IRBuilder::tryLoopIdiom(ForStatement& node) {
    if (!currentFunc_ || !node.initializer || !node.condition || !node.increment || !node.body) return false;
    for (const char* own : {"_memmove", "_memset", "_memcpy", "_memcpy_fast"})
        if (currentFunc_->name == own) return false;
    std::string iv, op;
    Expression *start = nullptr, *bound = nullptr;
    int step = 0;
    if (!loopHeader(node, iv, start, op, bound, step)) return false;
    if (op == "!=") return false;   // (the count would depend on the direction)
    // iv: a local integer, not volatile
    auto lt = localTypes_.find(iv);
    if (!locals_.count(iv) || lt == localTypes_.end() || volatileNames_.count(iv) ||
        (lt->second != ir::Type::I8 && lt->second != ir::Type::I16)) return false;
    // the bounds: constants and variables, not iv, no elements
    if (!cloneSimple(start) || !cloneSimple(bound)) return false;
    Reads rb;
    bound->accept(rb);
    if (rb.element || rb.names.count(iv)) return false;
    Reads rs;
    start->accept(rs);
    if (rs.element || rs.names.count(iv)) return false;

    // An array of this function or a global one (not a pointer, a parameter)
    auto isArray = [&](Expression* base) -> const std::vector<int>* {
        if (auto* v = dynamic_cast<VariableReference*>(base)) {
            if (paramArrayDims_.count(v->name)) return nullptr;
            auto l = localArrayDims_.find(v->name);
            if (l != localArrayDims_.end()) return &l->second;
            if (locals_.count(v->name)) return nullptr;
            auto g = globalArrayDims_.find(v->name);
            return g != globalArrayDims_.end() ? &g->second : nullptr;
        }
        if (auto* m = dynamic_cast<MemberAccess*>(base)) {
            // (a member of a named struct: two pointers may reach one)
            for (Expression* x = m; auto* mm = dynamic_cast<MemberAccess*>(x); x = mm->structExpr.get())
                if (mm->isArrow) return nullptr;
            const IRMemberInfo* mi = arrayMemberOf(m);
            return mi && !mi->arrayDims.empty() ? &mi->arrayDims : nullptr;
        }
        return nullptr;
    };

    struct Item {
        Expression* tbase; long c;           // the target array, its offset from iv
        Expression* sbase = nullptr; long d = 0;   // a copy's source
        bool fill = false; long value = 0;
        int esize = 0;                       // bytes per element (a row's)
        std::string inner; long innerEnd = 0;     // an inner row loop's variable, its final value
        bool row = false;                    // a whole row (an inner loop)
    };
    std::vector<Item> items;

    // one element assignment; ivs: the outer variable, inner (a row loop's)
    auto element = [&](Statement* st, const std::string& inner, long width, Item& it) -> bool {
        auto* es = dynamic_cast<ExpressionStatement*>(st);
        auto* as = es ? dynamic_cast<Assignment*>(es->expression.get()) : nullptr;
        if (!as || as->op != "=") return false;
        auto* ta = dynamic_cast<ArrayAccess*>(as->target.get());
        if (!ta) return false;
        Expression* tRow = ta;
        if (!inner.empty()) {
            auto* k = dynamic_cast<VariableReference*>(ta->indexExpr.get());
            if (!k || k->name != inner) return false;
            tRow = ta->arrayExpr.get();
        }
        auto* tr = dynamic_cast<ArrayAccess*>(tRow);
        if (!tr || !offsetOf(tr->indexExpr.get(), iv, it.c)) return false;
        it.tbase = tr->arrayExpr.get();
        const std::vector<int>* dims = isArray(it.tbase);
        if (!dims || baseKey(it.tbase).empty() || volatileNames_.count(rootName(it.tbase))) return false;
        if (!inner.empty() && (dims->size() != 2 || (*dims)[1] != width)) return false;
        IRTypeInfo te = getExprTypeInfo(as->target.get());
        if (te.totalSize <= 0) return false;
        it.esize = te.totalSize * (inner.empty() ? 1 : (int)width);
        long v;
        if (constValue(as->expression.get(), v)) {
            // memset: a byte element, or every byte the same (0, -1 for an integer)
            bool integral = te.type == ir::Type::I8 || te.type == ir::Type::I16 || te.type == ir::Type::I32 ||
                            te.type == ir::Type::PTR;
            if (!(te.totalSize == 1 || v == 0 || (v == -1 && integral))) return false;
            if (te.type == ir::Type::F32 && v != 0) return false;
            it.fill = true;
            it.value = v & 0xFF;
            return true;
        }
        auto* sa = dynamic_cast<ArrayAccess*>(as->expression.get());
        if (!sa) return false;
        Expression* sRow = sa;
        if (!inner.empty()) {
            auto* k = dynamic_cast<VariableReference*>(sa->indexExpr.get());
            if (!k || k->name != inner) return false;
            sRow = sa->arrayExpr.get();
        }
        auto* sr = dynamic_cast<ArrayAccess*>(sRow);
        if (!sr || !offsetOf(sr->indexExpr.get(), iv, it.d)) return false;
        it.sbase = sr->arrayExpr.get();
        const std::vector<int>* sdims = isArray(it.sbase);
        if (!sdims || baseKey(it.sbase).empty() || volatileNames_.count(rootName(it.sbase))) return false;
        if (!inner.empty() && (sdims->size() != 2 || (*sdims)[1] != width)) return false;
        IRTypeInfo se = getExprTypeInfo(as->expression.get());
        // the same type both sides: a byte-for-byte copy
        return se.totalSize == te.totalSize && se.type == te.type && se.typeName == te.typeName &&
               se.ptrDepth == te.ptrDepth;
    };

    std::vector<Statement*> stmts;
    if (auto* cs = dynamic_cast<CompoundStatement*>(node.body.get())) {
        for (auto& s : cs->statements) stmts.push_back(s.get());
    } else {
        stmts.push_back(node.body.get());
    }
    if (stmts.empty()) return false;
    for (Statement* st : stmts) {
        Item it;
        if (auto* f = dynamic_cast<ForStatement*>(st)) {
            // a whole row: for (k = 0; k < W; k++) T[iv + c][k] = ...
            std::string k, kop;
            Expression *ks = nullptr, *kb = nullptr;
            int kstep = 0;
            long k0, w;
            if (!loopHeader(*f, k, ks, kop, kb, kstep) || kstep != 1 || kop != "<" || k == iv ||
                !constValue(ks, k0) || k0 != 0 || !constValue(kb, w) || w <= 0) return false;
            auto klt = localTypes_.find(k);
            if (!locals_.count(k) || klt == localTypes_.end() || volatileNames_.count(k)) return false;
            if (dynamic_cast<VariableDeclaration*>(f->initializer.get())) return false;
            // (each statement of its body a row of another array)
            std::vector<Statement*> kst;
            if (auto* kc = dynamic_cast<CompoundStatement*>(f->body.get())) {
                for (auto& x : kc->statements) kst.push_back(x.get());
            } else {
                kst.push_back(f->body.get());
            }
            if (kst.empty()) return false;
            for (size_t q = 0; q < kst.size(); q++) {
                Item row;
                if (!element(kst[q], k, w, row)) return false;
                row.inner = q == 0 ? k : "";     // (k's final value set once)
                row.innerEnd = w;
                row.row = true;
                items.push_back(row);
            }
            continue;
        } else if (!element(st, "", 0, it)) {
            return false;
        }
        items.push_back(it);
    }
    // different targets; a source is no other item's target; a shift in
    // memmove's direction; bounds not read from a target
    std::set<std::string> targets;
    for (auto& it : items) {
        std::string k = baseKey(it.tbase);
        if (!targets.insert(k).second) return false;
        if (rb.names.count(rootName(it.tbase)) || rs.names.count(rootName(it.tbase))) return false;
    }
    for (auto& it : items) {
        if (it.fill) continue;
        std::string sk = baseKey(it.sbase), tk = baseKey(it.tbase);
        if (sk == tk) {
            if (it.d == it.c) return false;                // (a copy onto itself)
            if (step > 0 ? it.d < it.c : it.d > it.c) return false;   // (a smear, not a move)
        } else if (targets.count(sk)) {
            return false;
        }
    }
    // Worth a call: a call with its three arguments is 20-30 bytes, about
    // what a loop over one array of bytes or ints takes (name+off,y,
    // #318), so: two arrays or more, a row loop, or elements of 3 bytes or
    // more (longs, structs). A small constant count stays a loop
    bool worth = items.size() >= 2;
    for (auto& it : items) if (it.row || it.esize >= 3) worth = true;
    if (!worth) return false;
    long a = 0, b = 0;
    bool constStart = constValue(start, a);
    bool constBounds = constStart && constValue(bound, b);
    if (constBounds) {
        long n = step > 0 ? b - a + (op == "<=" ? 1 : 0) : a - b + (op == ">=" ? 1 : 0);
        if (n <= 2) return false;
    }

    // iv = A (the loop's initializer: a declaration in it ends with the loop)
    auto shadowed = saveShadowed(node.initializer.get(), false);
    node.initializer->accept(*this);

    auto ivRef = [&]() { return at(std::make_unique<VariableReference>(iv), node); };
    auto lit = [&](long v) { return at(std::make_unique<IntegerLiteral>(v), node); };
    auto plus = [&](std::unique_ptr<Expression> e, long c) -> std::unique_ptr<Expression> {
        if (!c) return e;
        return at(std::make_unique<BinaryOperation>(c > 0 ? "+" : "-", std::move(e), lit(c > 0 ? c : -c)), node);
    };
    // the lowest index the loop touches (+ an item's offset), the count
    // (a constant start: its value rather than iv, which holds it)
    auto startRef = [&]() -> std::unique_ptr<Expression> {
        if (constStart) return lit(a);
        return ivRef();
    };
    // Values every call uses, computed once into locals of their own (not
    // again for each array): the count, the low index (going down to a
    // variable bound), the bytes of an element size two arrays share
    static int tempId = 0;
    auto temp = [&](std::unique_ptr<Expression> init) -> std::string {
        std::string name = "__loop_idiom_" + std::to_string(tempId++);
        auto d = at(std::make_unique<VariableDeclaration>("int", name), node);
        d->initializer = std::move(init);
        d->accept(*this);
        return name;
    };
    auto ref = [&](const std::string& name) { return at(std::make_unique<VariableReference>(name), node); };
    std::string countName, lowName;
    if (!constBounds) {
        std::unique_ptr<Expression> n = step > 0
            ? at(std::make_unique<BinaryOperation>("-", cloneSimple(bound), startRef()), node)
            : at(std::make_unique<BinaryOperation>("-", startRef(), cloneSimple(bound)), node);
        countName = temp(plus(std::move(n), op == "<=" || op == ">=" ? 1 : 0));
        if (step < 0) lowName = temp(plus(cloneSimple(bound), op == ">=" ? 0 : 1));
    }
    // the lowest index the loop touches (+ an item's offset), the count
    auto low = [&](long off) -> std::unique_ptr<Expression> {
        if (step > 0 && constStart) return lit(a + off);
        if (step > 0) return plus(ivRef(), off);
        if (constBounds) return lit(b + (op == ">=" ? 0 : 1) + off);
        return plus(ref(lowName), off);
    };
    auto count = [&]() -> std::unique_ptr<Expression> {
        if (constBounds) return lit(step > 0 ? b - a + (op == "<=" ? 1 : 0) : a - b + (op == ">=" ? 1 : 0));
        return ref(countName);
    };
    std::map<int, int> sizeUses;
    for (auto& it : items) sizeUses[it.esize]++;
    std::map<int, std::string> bytesName;
    if (!constBounds)
        for (auto& [sz, uses] : sizeUses)
            if (sz != 1 && uses >= 2)
                bytesName[sz] = temp(at(std::make_unique<BinaryOperation>("*", count(), lit(sz)), node));
    auto addr = [&](Expression* base, long off) {
        auto e = at(std::make_unique<ArrayAccess>(cloneSimple(base), low(off)), node);
        return at(std::make_unique<UnaryOperation>("&", std::move(e)), node);
    };
    auto block = at(std::make_unique<CompoundStatement>(), node);
    for (auto& it : items) {
        auto call = at(std::make_unique<FunctionCall>(it.fill ? "memset" : "memmove"), node);
        call->arguments.push_back(addr(it.tbase, it.c));
        if (it.fill) call->arguments.push_back(lit(it.value));
        else call->arguments.push_back(addr(it.sbase, it.d));
        std::unique_ptr<Expression> bytes = count();
        if (constBounds) bytes = lit(static_cast<IntegerLiteral*>(bytes.get())->value * it.esize);
        else if (bytesName.count(it.esize)) bytes = ref(bytesName[it.esize]);
        else if (it.esize != 1)
            bytes = at(std::make_unique<BinaryOperation>("*", std::move(bytes), lit(it.esize)), node);
        call->arguments.push_back(std::move(bytes));
        block->statements.push_back(at(std::make_unique<ExpressionStatement>(std::move(call)), node));
    }
    // iv's value after the loop; an inner row loop's variable's
    std::unique_ptr<Expression> fin = cloneSimple(bound);
    if (op == "<=") fin = plus(std::move(fin), 1);
    else if (op == ">=") fin = plus(std::move(fin), -1);
    block->statements.push_back(at(std::make_unique<ExpressionStatement>(
        at(std::make_unique<Assignment>(ivRef(), std::move(fin)), node)), node));
    for (auto& it : items)
        if (!it.inner.empty())
            block->statements.push_back(at(std::make_unique<ExpressionStatement>(
                at(std::make_unique<Assignment>(at(std::make_unique<VariableReference>(it.inner), node), lit(it.innerEnd)), node)), node));
    // if (the loop's first test) { the calls; iv = its final value; } (the
    // block alone when constant bounds say the loop runs)
    if (constBounds) {
        block->accept(*this);
        restoreShadowed(shadowed);
        return true;
    }
    IfStatement guard(std::move(node.condition), std::move(block));
    guard.line = node.line;
    guard.column = node.column;
    guard.sourceFile = node.sourceFile;
    guard.accept(*this);
    restoreShadowed(shadowed);
    return true;
}
