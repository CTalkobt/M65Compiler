#include "LoopInterchange.hpp"
#include <algorithm>
#include <sstream>
#include <vector>

// ============================================================================
// Expression helpers (the AST has no generic child iterator)
// ============================================================================

namespace {

bool isIncDec(const std::string& op) {
    return op == "++" || op == "--" || op == "++_POST" || op == "--_POST";
}

// Calls f on every direct sub-expression. Returns false for an expression
// kind that is not understood here (the caller then treats it as unsafe).
bool forEachChild(Expression* e, const std::function<void(Expression*)>& f) {
    if (!e) return true;
    if (dynamic_cast<IntegerLiteral*>(e) || dynamic_cast<FloatLiteral*>(e) ||
        dynamic_cast<StringLiteral*>(e) || dynamic_cast<VariableReference*>(e))
        return true;
    if (auto* b = dynamic_cast<BinaryOperation*>(e)) { f(b->left.get()); f(b->right.get()); return true; }
    if (auto* u = dynamic_cast<UnaryOperation*>(e)) { f(u->operand.get()); return true; }
    if (auto* a = dynamic_cast<ArrayAccess*>(e)) { f(a->arrayExpr.get()); f(a->indexExpr.get()); return true; }
    if (auto* m = dynamic_cast<MemberAccess*>(e)) { f(m->structExpr.get()); return true; }
    if (auto* c = dynamic_cast<ConditionalExpression*>(e)) {
        f(c->condition.get()); f(c->thenExpr.get()); f(c->elseExpr.get()); return true;
    }
    if (auto* c = dynamic_cast<CastExpression*>(e)) { f(c->expression.get()); return true; }
    if (auto* a = dynamic_cast<Assignment*>(e)) { f(a->target.get()); f(a->expression.get()); return true; }
    if (auto* s = dynamic_cast<SizeofExpression*>(e)) { (void)s; return true; }   // not evaluated
    if (auto* s = dynamic_cast<AlignofExpression*>(e)) { (void)s; return true; }
    return false;   // calls, statement expressions, compound literals, va_arg, __cpu, ...
}

// Every variable name the expression reads or writes; false if it contains
// something forEachChild does not understand
bool collectNames(Expression* e, std::set<std::string>& names) {
    if (!e) return true;
    if (auto* v = dynamic_cast<VariableReference*>(e)) { names.insert(v->name); return true; }
    if (dynamic_cast<SizeofExpression*>(e) || dynamic_cast<AlignofExpression*>(e)) return true;
    bool ok = true;
    if (!forEachChild(e, [&](Expression* c) { if (!collectNames(c, names)) ok = false; })) return false;
    return ok;
}

// Assignments and ++/-- anywhere in e
bool hasWrites(Expression* e) {
    if (!e) return false;
    if (dynamic_cast<Assignment*>(e)) return true;
    if (auto* u = dynamic_cast<UnaryOperation*>(e); u && isIncDec(u->op)) return true;
    bool w = false;
    forEachChild(e, [&](Expression* c) { if (hasWrites(c)) w = true; });
    return w;
}

// The variable a loop header manages, from `int i = e` or `i = e`
std::string initVar(Statement* init, Expression** initValue) {
    *initValue = nullptr;
    if (auto* d = dynamic_cast<VariableDeclaration*>(init)) {
        *initValue = d->initializer.get();
        return d->pointerLevel == 0 && d->arrayDims.empty() ? d->name : "";
    }
    if (auto* es = dynamic_cast<ExpressionStatement*>(init)) {
        if (auto* a = dynamic_cast<Assignment*>(es->expression.get()); a && a->op == "=") {
            if (auto* v = dynamic_cast<VariableReference*>(a->target.get())) {
                *initValue = a->expression.get();
                return v->name;
            }
        }
    }
    return "";
}

// The increment writes only v: v++, ++v, v--, --v, v += e, v -= e, v = v +/- e
bool simpleIncrement(Expression* inc, const std::string& v) {
    auto isV = [&](Expression* e) {
        auto* r = dynamic_cast<VariableReference*>(e);
        return r && r->name == v;
    };
    if (auto* u = dynamic_cast<UnaryOperation*>(inc); u && isIncDec(u->op)) return isV(u->operand.get());
    if (auto* a = dynamic_cast<Assignment*>(inc)) {
        if (!isV(a->target.get()) || hasWrites(a->expression.get())) return false;
        return a->op == "+=" || a->op == "-=" || a->op == "=";
    }
    return false;
}

// An array element access `base[s0][s1]...`: the base name and subscripts
// (outermost dimension first); false when the base is not a plain name
bool arrayAccess(Expression* e, std::string& base, std::vector<Expression*>& subs) {
    std::vector<Expression*> rev;
    while (auto* a = dynamic_cast<ArrayAccess*>(e)) {
        rev.push_back(a->indexExpr.get());
        e = a->arrayExpr.get();
    }
    auto* v = dynamic_cast<VariableReference*>(e);
    if (!v || rev.empty()) return false;
    base = v->name;
    subs.assign(rev.rbegin(), rev.rend());
    return true;
}

// Structural equality of two side-effect-free expressions
bool sameExpr(Expression* a, Expression* b) {
    if (!a || !b) return a == b;
    if (auto* x = dynamic_cast<VariableReference*>(a)) {
        auto* y = dynamic_cast<VariableReference*>(b);
        return y && x->name == y->name;
    }
    if (auto* x = dynamic_cast<IntegerLiteral*>(a)) {
        auto* y = dynamic_cast<IntegerLiteral*>(b);
        return y && x->value == y->value;
    }
    if (auto* x = dynamic_cast<BinaryOperation*>(a)) {
        auto* y = dynamic_cast<BinaryOperation*>(b);
        return y && x->op == y->op && sameExpr(x->left.get(), y->left.get()) && sameExpr(x->right.get(), y->right.get());
    }
    if (auto* x = dynamic_cast<UnaryOperation*>(a)) {
        auto* y = dynamic_cast<UnaryOperation*>(b);
        return y && x->op == y->op && sameExpr(x->operand.get(), y->operand.get());
    }
    if (auto* x = dynamic_cast<CastExpression*>(a)) {
        auto* y = dynamic_cast<CastExpression*>(b);
        return y && x->targetType == y->targetType && x->pointerLevel == y->pointerLevel &&
               x->isSigned == y->isSigned && sameExpr(x->expression.get(), y->expression.get());
    }
    return false;
}

bool mentions(Expression* e, const std::string& v) {
    std::set<std::string> n;
    collectNames(e, n);
    return n.count(v) > 0;
}

} // namespace

// ============================================================================
// Declarations
// ============================================================================

const LoopInterchange::VarInfo* LoopInterchange::lookup(const std::string& name) const {
    auto it = locals_.find(name);
    if (it != locals_.end()) return &it->second;
    auto g = globals_.find(name);
    return g != globals_.end() ? &g->second : nullptr;
}

void LoopInterchange::collectLocals(Statement* s) {
    if (!s) return;
    if (auto* d = dynamic_cast<VariableDeclaration*>(s)) {
        VarInfo vi;
        vi.isArray = !d->arrayDims.empty() && d->pointerLevel == 0;
        vi.isVolatile = d->isVolatile;
        vi.isFloat = d->type == "float" || d->type == "double" || d->type == "long double";
        vi.isPointer = d->pointerLevel > 0;
        // A name declared twice in the function (shadowing): unknown
        if (locals_.count(d->name)) locals_[d->name].isVolatile = true;
        else locals_[d->name] = vi;
        return;
    }
    if (auto* c = dynamic_cast<CompoundStatement*>(s)) { for (auto& x : c->statements) collectLocals(x.get()); return; }
    if (auto* f = dynamic_cast<ForStatement*>(s)) { collectLocals(f->initializer.get()); collectLocals(f->body.get()); return; }
    if (auto* w = dynamic_cast<WhileStatement*>(s)) { collectLocals(w->body.get()); return; }
    if (auto* w = dynamic_cast<DoWhileStatement*>(s)) { collectLocals(w->body.get()); return; }
    if (auto* i = dynamic_cast<IfStatement*>(s)) { collectLocals(i->thenBranch.get()); collectLocals(i->elseBranch.get()); return; }
    if (auto* w = dynamic_cast<SwitchStatement*>(s)) { collectLocals(w->body.get()); return; }
    if (auto* l = dynamic_cast<LabelledStatement*>(s)) { collectLocals(l->statement.get()); return; }
    if (auto* r = dynamic_cast<RepeatStatement*>(s)) { collectLocals(r->body.get()); return; }
}

// ============================================================================
// The transformation
// ============================================================================

int LoopInterchange::optimizeTranslationUnit(TranslationUnit& unit, bool enabledByDefault) {
    count_ = 0;
    globals_.clear();
    for (auto& d : unit.topLevelDecls) {
        auto collect = [&](Statement* s) {
            if (auto* v = dynamic_cast<VariableDeclaration*>(s)) {
                VarInfo vi;
                vi.isArray = !v->arrayDims.empty() && v->pointerLevel == 0;
                vi.isVolatile = v->isVolatile;
                vi.isFloat = v->type == "float" || v->type == "double" || v->type == "long double";
                vi.isPointer = v->pointerLevel > 0;
                globals_[v->name] = vi;
            }
        };
        if (auto* c = dynamic_cast<CompoundStatement*>(d.get()); c && c->isDeclGroup)
            for (auto& x : c->statements) collect(x.get());
        else
            collect(d.get());
    }

    bool fileEnabled = enabledByDefault;
    for (auto& d : unit.topLevelDecls) {
        if (auto* a = dynamic_cast<AsmStatement*>(d.get())) {
            // file-scope #pragma cc45 optimize(...): for the functions after it
            if (a->code.rfind(".optimize ", 0) == 0) {
                std::stringstream list(a->code.substr(10));
                std::string item;
                while (std::getline(list, item, ',')) {
                    if (item == "loop-interchange" || item == "loop_interchange") fileEnabled = true;
                    if (item == "no-loop-interchange" || item == "no_loop_interchange") fileEnabled = false;
                    if (item == "off") fileEnabled = false;
                    if (item == "on") fileEnabled = enabledByDefault;
                }
            }
            continue;
        }
        auto* fn = dynamic_cast<FunctionDeclaration*>(d.get());
        if (!fn || !fn->body) continue;
        function_ = fn->name;
        currentFn_ = fn;
        locals_.clear();
        for (const auto& p : fn->parameters) {
            VarInfo vi;
            vi.isPointer = p.pointerLevel > 0;   // an array parameter is a pointer, never a declared array
            vi.isVolatile = p.isVolatile;
            vi.isFloat = p.type == "float" || p.type == "double" || p.type == "long double";
            locals_[p.name] = vi;
        }
        collectLocals(fn->body.get());
        bool enabled = fileEnabled;
        std::unique_ptr<Statement> body(fn->body.release());
        walk(body, enabled);
        fn->body.reset(static_cast<CompoundStatement*>(body.release()));
    }
    return count_;
}

// Visit statements in order; `enabled` follows #pragma cc45 optimize inside the function
void LoopInterchange::walk(std::unique_ptr<Statement>& s, bool& enabled) {
    if (!s) return;
    if (auto* a = dynamic_cast<AsmStatement*>(s.get())) {
        if (a->code.rfind(".optimize ", 0) == 0) {
            std::stringstream list(a->code.substr(10));
            std::string item;
            while (std::getline(list, item, ',')) {
                if (item == "loop-interchange" || item == "loop_interchange") enabled = true;
                if (item == "no-loop-interchange" || item == "no_loop_interchange") enabled = false;
                if (item == "off") enabled = false;
            }
        }
        return;
    }
    if (auto* c = dynamic_cast<CompoundStatement*>(s.get())) { for (auto& x : c->statements) walk(x, enabled); return; }
    if (auto* f = dynamic_cast<ForStatement*>(s.get())) {
        if (enabled) tryInterchange(*f);
        walk(f->body, enabled);
        return;
    }
    if (auto* w = dynamic_cast<WhileStatement*>(s.get())) { walk(w->body, enabled); return; }
    if (auto* w = dynamic_cast<DoWhileStatement*>(s.get())) { walk(w->body, enabled); return; }
    if (auto* i = dynamic_cast<IfStatement*>(s.get())) { walk(i->thenBranch, enabled); walk(i->elseBranch, enabled); return; }
    if (auto* w = dynamic_cast<SwitchStatement*>(s.get())) { walk(w->body, enabled); return; }
    if (auto* l = dynamic_cast<LabelledStatement*>(s.get())) { walk(l->statement, enabled); return; }
    if (auto* r = dynamic_cast<RepeatStatement*>(s.get())) { walk(r->body, enabled); return; }
}

namespace {

// What the body of the nest does, gathered in one walk
struct BodyFacts {
    bool unsafe = false;                 // a construct that rules interchange out
    std::set<std::string> written;       // scalar names assigned (any way)
    std::set<std::string> reductions;    // scalar names written only by +=, -=, ^=, |=, &=
    std::set<std::string> readNames;     // every name read
    std::map<std::string, int> scalarUses;   // occurrences of each scalar name
    std::set<std::string> declared;      // names declared in the body
    bool storesMemory = false;           // writes an array element (or anything else in memory)
    bool readsThroughPointer = false;    // *p, p->m, p[i] with p not a declared array
    // array element accesses: base -> list of subscript lists, and which are writes
    struct Access { std::vector<Expression*> subs; bool write; };
    std::map<std::string, std::vector<Access>> arrays;
};

} // namespace

bool LoopInterchange::tryInterchange(ForStatement& outer) {
    // 1. A perfect nest
    ForStatement* inner = dynamic_cast<ForStatement*>(outer.body.get());
    if (!inner) {
        auto* c = dynamic_cast<CompoundStatement*>(outer.body.get());
        if (!c || c->statements.size() != 1) return false;
        inner = dynamic_cast<ForStatement*>(c->statements[0].get());
    }
    if (!inner || !inner->body || !outer.condition || !inner->condition || !outer.increment || !inner->increment)
        return false;

    // 2. Rectangular, side-effect-free headers
    Expression *outerInit = nullptr, *innerInit = nullptr;
    std::string ov = initVar(outer.initializer.get(), &outerInit);
    std::string iv = initVar(inner->initializer.get(), &innerInit);
    if (ov.empty() || iv.empty() || ov == iv || !outerInit || !innerInit) return false;
    for (const std::string& v : {ov, iv}) {
        const VarInfo* vi = lookup(v);
        if (!vi || vi->isVolatile || vi->isFloat || vi->isPointer || vi->isArray) return false;
    }
    std::set<std::string> headerNames;   // names the headers read (besides the loop variables)
    auto checkHeader = [&](Expression* init, Expression* cond, Expression* inc,
                           const std::string& own, const std::string& other) {
        if (hasWrites(init) || hasWrites(cond) || !simpleIncrement(inc, own)) return false;
        std::set<std::string> n;
        if (!collectNames(init, n) || !collectNames(cond, n) || !collectNames(inc, n)) return false;
        if (n.count(other)) return false;          // not rectangular
        if (!mentions(cond, own)) return false;
        for (const auto& x : n) {
            const VarInfo* vi = lookup(x);
            if (vi && vi->isVolatile) return false;
            if (x != own) headerNames.insert(x);
        }
        return true;
    };
    if (!checkHeader(outerInit, outer.condition.get(), outer.increment.get(), ov, iv)) return false;
    if (!checkHeader(innerInit, inner->condition.get(), inner->increment.get(), iv, ov)) return false;
    // Pointer arithmetic or array reads in a header: keep it simple
    for (const auto& x : headerNames) {
        const VarInfo* vi = lookup(x);
        if (vi && (vi->isArray || vi->isPointer)) return false;
    }

    // 3. The body
    BodyFacts bf;
    std::function<void(Expression*, bool)> expr;     // (expression, is the target of a write)
    std::function<void(Statement*)> stmt;
    auto scalarWrite = [&](const std::string& name, const std::string& op) {
        bf.written.insert(name);
        if (bf.declared.count(name)) return;   // a temporary of the body: per iteration
        bool red = op == "+=" || op == "-=" || op == "^=" || op == "|=" || op == "&=";
        const VarInfo* vi = lookup(name);
        if (red && vi && !vi->isFloat && !vi->isPointer) bf.reductions.insert(name);
        else bf.unsafe = true;   // plain or float writes to scalars depend on the order
    };
    expr = [&](Expression* e, bool isTarget) {
        if (!e || bf.unsafe) return;
        if (auto* v = dynamic_cast<VariableReference*>(e)) {
            const VarInfo* vi = lookup(v->name);
            if (!vi) return;   // not a variable (an enum constant): a read of a constant
            if (vi->isVolatile) { bf.unsafe = true; return; }
            if (!isTarget) bf.readNames.insert(v->name);
            if (!vi->isArray) bf.scalarUses[v->name]++;
            return;
        }
        if (auto* a = dynamic_cast<Assignment*>(e)) {
            std::string base; std::vector<Expression*> subs;
            if (auto* v = dynamic_cast<VariableReference*>(a->target.get())) {
                scalarWrite(v->name, a->op);
                bf.scalarUses[v->name]++;
                if (a->op != "=") bf.readNames.insert(v->name);
            } else if (arrayAccess(a->target.get(), base, subs)) {
                const VarInfo* vi = lookup(base);
                if (!vi || !vi->isArray || vi->isVolatile) { bf.unsafe = true; return; }
                bf.storesMemory = true;
                bf.arrays[base].push_back({subs, true});
                for (auto* s : subs) { if (hasWrites(s)) { bf.unsafe = true; return; } expr(s, false); }
            } else {
                bf.unsafe = true;   // a store through a pointer or into a struct
                return;
            }
            expr(a->expression.get(), false);
            return;
        }
        if (auto* u = dynamic_cast<UnaryOperation*>(e)) {
            if (isIncDec(u->op)) {
                if (auto* v = dynamic_cast<VariableReference*>(u->operand.get())) {
                    scalarWrite(v->name, u->op == "++" || u->op == "++_POST" ? "+=" : "-=");
                    bf.scalarUses[v->name]++;
                    bf.readNames.insert(v->name);
                } else {
                    bf.unsafe = true;
                }
                return;
            }
            if (u->op == "*") bf.readsThroughPointer = true;
            if (u->op == "&") { bf.unsafe = true; return; }   // an address escapes
            expr(u->operand.get(), false);
            return;
        }
        if (dynamic_cast<ArrayAccess*>(e)) {
            std::string base; std::vector<Expression*> subs;
            if (!arrayAccess(e, base, subs)) { bf.readsThroughPointer = true; bf.unsafe = true; return; }
            const VarInfo* vi = lookup(base);
            if (!vi || vi->isVolatile) { bf.unsafe = true; return; }
            if (!vi->isArray) bf.readsThroughPointer = true;   // p[i]
            else bf.arrays[base].push_back({subs, false});
            for (auto* s : subs) { if (hasWrites(s)) { bf.unsafe = true; return; } expr(s, false); }
            return;
        }
        if (auto* m = dynamic_cast<MemberAccess*>(e)) {
            if (m->isArrow) bf.readsThroughPointer = true;
            if (!dynamic_cast<VariableReference*>(m->structExpr.get())) { bf.unsafe = true; return; }
            auto* v = static_cast<VariableReference*>(m->structExpr.get());
            const VarInfo* vi = lookup(v->name);
            if (!vi || vi->isVolatile) { bf.unsafe = true; return; }
            bf.readNames.insert(v->name);
            return;
        }
        if (!forEachChild(e, [&](Expression* c) { expr(c, false); })) bf.unsafe = true;
    };
    stmt = [&](Statement* s) {
        if (!s || bf.unsafe) return;
        if (auto* es = dynamic_cast<ExpressionStatement*>(s)) { expr(es->expression.get(), false); return; }
        if (auto* c = dynamic_cast<CompoundStatement*>(s)) { for (auto& x : c->statements) stmt(x.get()); return; }
        if (auto* i = dynamic_cast<IfStatement*>(s)) {
            if (hasWrites(i->condition.get())) { bf.unsafe = true; return; }
            expr(i->condition.get(), false); stmt(i->thenBranch.get()); stmt(i->elseBranch.get()); return;
        }
        if (auto* d = dynamic_cast<VariableDeclaration*>(s)) {
            // a block-local temporary: fine unless it shadows a name the nest uses
            bf.declared.insert(d->name);
            if (d->isStatic || d->isVolatile || !d->arrayDims.empty()) { bf.unsafe = true; return; }
            expr(d->initializer.get(), false);
            return;
        }
        if (dynamic_cast<StaticAssert*>(s)) return;
        bf.unsafe = true;   // loops, break/continue/return/goto/labels/switch/asm/repeat
    };
    stmt(inner->body.get());
    if (bf.unsafe) return false;

    // Loop and header variables: not written or declared in the body
    for (const std::string& v : {ov, iv})
        if (bf.written.count(v) || bf.declared.count(v)) return false;
    for (const auto& x : headerNames)
        if (bf.written.count(x) || bf.declared.count(x)) return false;
    // A declared temporary must not shadow anything the body reads from outside
    for (const auto& x : bf.declared) {
        if (headerNames.count(x) || bf.arrays.count(x)) return false;
        auto it = locals_.find(x);
        if (it == locals_.end()) return false;
    }
    // Reductions: only touched by their own update (s += e, with s not in e)
    for (const auto& r : bf.reductions)
        if (bf.scalarUses[r] != 1 || bf.declared.count(r)) return false;
    // Memory: each written array is always accessed at the same element, and
    // that element is determined by both loop variables
    if (bf.storesMemory && bf.readsThroughPointer) return false;
    for (const auto& [base, accs] : bf.arrays) {
        bool written = std::any_of(accs.begin(), accs.end(), [](const BodyFacts::Access& a) { return a.write; });
        if (!written) continue;
        const auto& first = accs.front().subs;
        for (const auto& a : accs) {
            if (a.subs.size() != first.size()) return false;
            for (size_t k = 0; k < first.size(); k++)
                if (!sameExpr(a.subs[k], first[k])) return false;
        }
        bool hasO = false, hasI = false;
        for (auto* s : first) { hasO |= mentions(s, ov); hasI |= mentions(s, iv); }
        if (!hasO || !hasI) return false;
    }

    // 4. The loop variables' values after the nest differ: they must be
    // declared by their for statements, or unused elsewhere in the function
    auto declaredInFor = [](Statement* init) { return dynamic_cast<VariableDeclaration*>(init) != nullptr; };
    if (!declaredInFor(outer.initializer.get()) || !declaredInFor(inner->initializer.get())) {
        // Count references to the loop variables in the whole function and in the nest
        std::function<int(Statement*, const std::string&)> countS;
        std::function<int(Expression*, const std::string&)> countE = [&](Expression* e, const std::string& v) -> int {
            if (!e) return 0;
            if (auto* r = dynamic_cast<VariableReference*>(e)) return r->name == v ? 1 : 0;
            int n = 0;
            if (!forEachChild(e, [&](Expression* c) { n += countE(c, v); })) n += 1000;   // unknown: assume used
            return n;
        };
        countS = [&](Statement* s, const std::string& v) -> int {
            if (!s) return 0;
            if (auto* es = dynamic_cast<ExpressionStatement*>(s)) return countE(es->expression.get(), v);
            if (auto* c = dynamic_cast<CompoundStatement*>(s)) { int n = 0; for (auto& x : c->statements) n += countS(x.get(), v); return n; }
            if (auto* f = dynamic_cast<ForStatement*>(s))
                return countS(f->initializer.get(), v) + countE(f->condition.get(), v) + countE(f->increment.get(), v) + countS(f->body.get(), v);
            if (auto* w = dynamic_cast<WhileStatement*>(s)) return countE(w->condition.get(), v) + countS(w->body.get(), v);
            if (auto* w = dynamic_cast<DoWhileStatement*>(s)) return countE(w->condition.get(), v) + countS(w->body.get(), v);
            if (auto* i = dynamic_cast<IfStatement*>(s)) return countE(i->condition.get(), v) + countS(i->thenBranch.get(), v) + countS(i->elseBranch.get(), v);
            if (auto* r = dynamic_cast<ReturnStatement*>(s)) return countE(r->expression.get(), v);
            if (auto* d = dynamic_cast<VariableDeclaration*>(s)) return countE(d->initializer.get(), v);
            if (auto* w = dynamic_cast<SwitchStatement*>(s)) return countE(w->expression.get(), v) + countS(w->body.get(), v);
            if (auto* l = dynamic_cast<LabelledStatement*>(s)) return countS(l->statement.get(), v);
            if (auto* r = dynamic_cast<RepeatStatement*>(s)) return countS(r->body.get(), v);
            if (dynamic_cast<AsmStatement*>(s)) return 1000;   // inline asm may use it
            return 0;
        };
        for (const std::string& v : {ov, iv}) {
            if (declaredInFor(v == ov ? outer.initializer.get() : inner->initializer.get())) continue;
            if (countS(currentFn_->body.get(), v) != countS(&outer, v)) return false;
        }
    }

    // 5. Profitable: subscripts put the inner variable in an earlier dimension
    int score = 0;
    for (const auto& [base, accs] : bf.arrays) {
        for (const auto& a : accs) {
            int po = -1, pi = -1;
            for (size_t k = 0; k < a.subs.size(); k++) {
                bool o = mentions(a.subs[k], ov), i = mentions(a.subs[k], iv);
                if (o && i) { po = pi = -1; break; }     // a[i + j]: no preference
                if (o && po < 0) po = (int)k;
                if (i && pi < 0) pi = (int)k;
            }
            if (po < 0 || pi < 0) continue;
            score += (pi < po) ? 1 : -1;
        }
    }
    if (score <= 0) return false;

    // Swap the headers: the bodies stay where they are
    std::swap(outer.initializer, inner->initializer);
    std::swap(outer.condition, inner->condition);
    std::swap(outer.increment, inner->increment);
    count_++;
    if (trace) trace(function_, outer.line, "loops over " + ov + " and " + iv + " interchanged (" + iv + " is now the outer loop)");
    return true;
}
