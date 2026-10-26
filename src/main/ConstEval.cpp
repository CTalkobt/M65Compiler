#include "ConstEval.hpp"
#include <algorithm>
#include <cmath>

ConstValue ConstValue::ofLiteral(const IntegerLiteral& l) {
    ConstValue c = ofInt(l.value);
    c.castType = l.castType;
    c.castPointerLevel = l.castPointerLevel;
    c.castIsSigned = l.castIsSigned;
    return c;
}

int64_t ConstValue::intValue() const {
    if (castPointerLevel > 0) return value & 0xFFFF;
    return ConstEval::normalize(value, ConstEval::litWidth(*this), ConstEval::litSigned(*this));
}

double ConstValue::asDouble() const {
    return kind == Kind::Float ? fvalue : (double)intValue();
}

std::unique_ptr<IntegerLiteral> ConstValue::toLiteral() const {
    auto l = std::make_unique<IntegerLiteral>(value);
    l->castType = castType;
    l->castPointerLevel = castPointerLevel;
    l->castIsSigned = castIsSigned;
    return l;
}

namespace ConstEval {

int litWidth(const ConstValue& l) {
    if (l.castPointerLevel > 0) return 2;
    const std::string& t = l.castType;
    if (t.empty()) return (l.value > 65535 || l.value < -32768) ? 4 : 2;
    if (t.find("long") != std::string::npos) return 4;
    if (t.find("char") != std::string::npos || t == "_Bool" || t == "__bit") return 1;
    return 2;
}

bool litSigned(const ConstValue& l) {
    if (l.castPointerLevel > 0) return false;
    if (l.castType.empty()) return l.value <= 32767;
    return l.castIsSigned;
}

int64_t normalize(int64_t v, int width, bool isSigned) {
    if (width >= 8) return v;
    int bits = width * 8;
    uint64_t m = (uint64_t(1) << bits) - 1;
    uint64_t u = (uint64_t)v & m;
    if (isSigned && (u >> (bits - 1))) u |= ~m;
    return (int64_t)u;
}

bool isFloatType(const std::string& type, int pointerLevel) {
    return pointerLevel == 0 && (type == "float" || type == "double" || type == "long double");
}

static bool isCompareOp(const std::string& op) {
    return op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=" || op == "&&" || op == "||";
}

// Floats: + - * / and the comparisons (an integer operand converted)
static bool binaryFloat(const std::string& op, double l, double r, ConstValue& out) {
    if (op == "+") out = ConstValue::ofFloat(l + r);
    else if (op == "-") out = ConstValue::ofFloat(l - r);
    else if (op == "*") out = ConstValue::ofFloat(l * r);
    else if (op == "/") { if (r == 0) return false; out = ConstValue::ofFloat(l / r); }
    else if (op == "==") out = ConstValue::ofInt(l == r);
    else if (op == "!=") out = ConstValue::ofInt(l != r);
    else if (op == "<") out = ConstValue::ofInt(l < r);
    else if (op == ">") out = ConstValue::ofInt(l > r);
    else if (op == "<=") out = ConstValue::ofInt(l <= r);
    else if (op == ">=") out = ConstValue::ofInt(l >= r);
    else if (op == "&&") out = ConstValue::ofInt(l != 0 && r != 0);
    else if (op == "||") out = ConstValue::ofInt(l != 0 || r != 0);
    else return false;
    return true;
}

bool binary(const std::string& op, const ConstValue& left, const ConstValue& right, ConstValue& out) {
    if (left.kind == ConstValue::Kind::None || right.kind == ConstValue::Kind::None) return false;
    if (left.isFloat() || right.isFloat()) return binaryFloat(op, left.asDouble(), right.asDouble(), out);
    // C's usual arithmetic conversions on the two literals: each is taken at
    // its own width/signedness, then both are converted to the common type.
    // Comparisons are then signed or unsigned as that type says
    // ((short)-0x8000 ^ 0U is 0x8000, not -32768). Two plain literals keep
    // plain 32-bit arithmetic (256 * 256 folds to 65536, as before).
    bool plain = left.castType.empty() && right.castType.empty() &&
                 !left.castPointerLevel && !right.castPointerLevel;
    int wl = litWidth(left), wr = litWidth(right);
    bool sl = litSigned(left), sr = litSigned(right);
    int64_t lv = normalize(left.value, wl, sl), rv = normalize(right.value, wr, sr);
    if (wl < 2) wl = 2;
    if (wr < 2) wr = 2;
    bool isShift = op == "<<" || op == ">>";
    int w = isShift ? wl : std::max(wl, wr);
    bool sg = isShift ? sl : (wl == wr ? (sl && sr) : (wl > wr ? sl : sr));
    if (plain) { w = 8; sg = true; }
    lv = normalize(lv, w, sg);
    if (!isShift) rv = normalize(rv, w, sg);
    uint64_t ul = (uint64_t)lv, ur = (uint64_t)rv;
    int64_t result = 0;
    if (op == "+") result = lv + rv;
    else if (op == "-") result = lv - rv;
    else if (op == "*") result = lv * rv;
    else if (op == "/" || op == "%") {
        if (rv == 0) return false;
        if (op == "/") result = sg ? lv / rv : (int64_t)(ul / ur);
        else result = sg ? lv % rv : (int64_t)(ul % ur);
    }
    else if (op == "&") result = lv & rv;
    else if (op == "|") result = lv | rv;
    else if (op == "^") result = lv ^ rv;
    else if (op == "<<") result = (int64_t)(ul << (rv & 63));
    else if (op == ">>") result = sg ? (lv >> (rv & 63)) : (int64_t)(ul >> (rv & 63));
    else if (op == "&&") result = lv && rv;
    else if (op == "||") result = lv || rv;
    else if (op == "==") result = lv == rv;
    else if (op == "!=") result = lv != rv;
    else if (op == "<") result = sg ? lv < rv : ul < ur;
    else if (op == ">") result = sg ? lv > rv : ul > ur;
    else if (op == "<=") result = sg ? lv <= rv : ul <= ur;
    else if (op == ">=") result = sg ? lv >= rv : ul >= ur;
    else return false;
    if (isCompareOp(op)) { out = ConstValue::ofInt(result); return true; }
    result = plain ? (int64_t)(int32_t)result : normalize(result, w, sg);

    out = ConstValue::ofInt(result);
    // Type promotion: if either operand is long, the result is long.
    // If either operand is a pointer, the result is the same pointer type (for +/-).
    if (!plain && w == 4 && !left.castPointerLevel && !right.castPointerLevel) {
        out.castType = "long";
        out.castIsSigned = sg;
    } else if (left.castPointerLevel > 0) {
        out.castType = left.castType;
        out.castPointerLevel = left.castPointerLevel;
        out.castIsSigned = left.castIsSigned;
    } else if (right.castPointerLevel > 0) {
        out.castType = right.castType;
        out.castPointerLevel = right.castPointerLevel;
        out.castIsSigned = right.castIsSigned;
    } else if (!left.castType.empty()) {
        out.castType = left.castType;
        out.castIsSigned = left.castIsSigned;
    } else if (!right.castType.empty()) {
        out.castType = right.castType;
        out.castIsSigned = right.castIsSigned;
    }
    return true;
}

bool unary(const std::string& op, const ConstValue& v, ConstValue& out) {
    if (v.isFloat()) {
        if (op == "-") out = ConstValue::ofFloat(-v.fvalue);
        else if (op == "+") out = v;
        else if (op == "!") out = ConstValue::ofInt(v.fvalue == 0);
        else return false;
        return true;
    }
    if (!v.isInt()) return false;
    if (op == "+") { out = v; return true; }
    int64_t result = 0;
    if (op == "-") result = (int)-v.value;
    else if (op == "!") result = !v.value;
    // ~ in the operand's width: ~0x8000 (an unsigned int) is 0x7FFF,
    // not -32769, which is a long (Issue #289); a char is promoted
    else if (op == "~") {
        int w = litWidth(v);
        result = w >= 2 ? normalize(~v.value, w, litSigned(v)) : (int)~v.value;
    } else {
        return false;
    }
    out = ConstValue::ofInt(result);
    // -c and ~c of a char are ints (~(unsigned char)1 is -2, not 254)
    if (!((op == "-" || op == "~") && litWidth(v) == 1)) {
        out.castType = v.castType;
        out.castPointerLevel = v.castPointerLevel;
        out.castIsSigned = v.castIsSigned;
    }
    return true;
}

bool cast(const std::string& type, int pointerLevel, bool isSigned, const ConstValue& v, ConstValue& out) {
    // A cast to a struct/union type (e.g. (_Complex double)2) isn't a number
    if (pointerLevel == 0 && (type.rfind("struct ", 0) == 0 || type.rfind("union ", 0) == 0)) return false;
    if (isFloatType(type, pointerLevel)) {
        if (v.isFloat()) { out = v; return true; }
        if (!v.isInt()) return false;
        // (double)1: a floating constant (value converted by the literal's
        // own signedness)
        double d = (double)v.value;
        if (!v.castType.empty() && !v.castIsSigned && !v.castPointerLevel) {
            if (v.castType.find("long") != std::string::npos) d = (double)(uint32_t)v.value;
            else if (v.castType.find("char") != std::string::npos) d = (double)(uint8_t)v.value;
            else d = (double)(uint16_t)v.value;
        }
        out = ConstValue::ofFloat(d);
        return true;
    }
    int64_t from;
    if (v.isInt()) from = v.value;
    else if (v.isFloat() && pointerLevel == 0) from = (int64_t)std::trunc(v.fvalue);
    else return false;
    int val = (int)from;
    if (pointerLevel == 0 && (type == "_Bool" || type == "__bit")) val = (val != 0) ? 1 : 0;
    else if (pointerLevel == 0 && type == "char") val = val & 0xFF;
    out = ConstValue::ofInt(val);
    out.castType = type;
    out.castPointerLevel = pointerLevel;
    out.castIsSigned = isSigned;
    return true;
}

static bool truthy(const ConstValue& v) {
    return v.isFloat() ? v.fvalue != 0 : v.intValue() != 0;
}

// e's truth when it is a constant or an object's address (never null)
static bool truthOf(Expression* e, ConstEvalContext& ctx, bool& t) {
    ConstValue v;
    if (eval(e, ctx, v)) { t = truthy(v); return true; }
    if (e && ctx.nonNullAddress(*e)) { t = true; return true; }
    return false;
}

static bool isZero(Expression* e, ConstEvalContext& ctx) {
    ConstValue v;
    return eval(e, ctx, v) && !v.isFloat() && v.intValue() == 0;
}

bool eval(Expression* e, ConstEvalContext& ctx, ConstValue& out) {
    if (!e) return false;
    if (auto* l = dynamic_cast<IntegerLiteral*>(e)) { out = ConstValue::ofLiteral(*l); return true; }
    if (auto* f = dynamic_cast<FloatLiteral*>(e)) { out = ConstValue::ofFloat(f->value); return true; }
    if (auto* v = dynamic_cast<VariableReference*>(e)) return ctx.identifier(*v, out);
    if (auto* u = dynamic_cast<UnaryOperation*>(e)) {
        ConstValue v;
        if (u->op == "!" && !eval(u->operand.get(), ctx, v) && ctx.nonNullAddress(*u->operand)) {
            out = ConstValue::ofInt(0);   // !&x
            return true;
        }
        return eval(u->operand.get(), ctx, v) && unary(u->op, v, out);
    }
    if (auto* b = dynamic_cast<BinaryOperation*>(e)) {
        ConstValue l, r;
        // an address compared with 0, or in && / ||: its truth (never null)
        if (b->op == "==" || b->op == "!=") {
            bool la = ctx.nonNullAddress(*b->left) && isZero(b->right.get(), ctx);
            bool ra = ctx.nonNullAddress(*b->right) && isZero(b->left.get(), ctx);
            if (la || ra) { out = ConstValue::ofInt(b->op == "!="); return true; }
        }
        if (b->op == "&&" || b->op == "||") {
            bool lt, rt;
            if (!truthOf(b->left.get(), ctx, lt)) return false;
            if (b->op == "&&" && !lt) { out = ConstValue::ofInt(0); return true; }
            if (b->op == "||" && lt) { out = ConstValue::ofInt(1); return true; }
            if (!truthOf(b->right.get(), ctx, rt)) return false;
            out = ConstValue::ofInt(rt);
            return true;
        }
        if (!eval(b->left.get(), ctx, l)) return false;
        // && and ||: the right operand only when it decides
        if (b->op == "&&" && !truthy(l)) { out = ConstValue::ofInt(0); return true; }
        if (b->op == "||" && truthy(l)) { out = ConstValue::ofInt(1); return true; }
        if (!eval(b->right.get(), ctx, r)) return false;
        return binary(b->op, l, r, out);
    }
    if (auto* c = dynamic_cast<ConditionalExpression*>(e)) {
        bool t;
        if (!truthOf(c->condition.get(), ctx, t)) return false;
        return eval(t ? c->thenExpr.get() : c->elseExpr.get(), ctx, out);
    }
    if (auto* c = dynamic_cast<CastExpression*>(e)) {
        ConstValue v;
        // (_Bool)&x: 1
        if (c->pointerLevel == 0 && (c->targetType == "_Bool" || c->targetType == "__bit") &&
            !eval(c->expression.get(), ctx, v) && ctx.nonNullAddress(*c->expression)) {
            out = ConstValue::ofInt(1);
            return true;
        }
        return eval(c->expression.get(), ctx, v) && cast(c->targetType, c->pointerLevel, c->isSigned, v, out);
    }
    if (auto* s = dynamic_cast<SizeofExpression*>(e)) {
        int64_t n;
        if (!ctx.sizeOf(*s, n)) return false;
        out = ConstValue::ofInt(n);
        return true;
    }
    if (auto* a = dynamic_cast<AlignofExpression*>(e)) {
        int64_t n;
        if (!ctx.alignOf(*a, n)) return false;
        out = ConstValue::ofInt(n);
        return true;
    }
    return false;
}

}  // namespace ConstEval
