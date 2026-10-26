// IRBuilderFixed.cpp — fixed-point types (#72): __fixed(I, F), a signed
// integer of I + F bits (16 or 32) whose value is the integer / 2^F;
// __fixed8_8 and __fixed16_16 are __fixed(8,8) and __fixed(16,16). A type is
// named __fixedI_F inside cc45 and coded as (bits << 8) | F (0: not fixed).
// Constants are scaled when compiled; + - and comparisons are the integers';
// * and / scale (16 bits inline, 32 bits by __fx32_mul / __fx32_div of
// <fixed.h>); int, float and other fixed types convert to and from them
// (casts, assignments, initializers, arguments, returns, ?:).
#include "IRBuilder.hpp"
#include "Diagnostic.hpp"
#include "ConstEval.hpp"
#include "FixedType.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>

// "__fixedI_F" with I + F 16 or 32, I and F at least 1: its code
int IRBuilder::fixedFracOf(const std::string& typeName) {
    int f = 0, bits = fixedTypeBits(typeName, &f);
    return bits ? (bits << 8) | f : 0;
}

// as written in C: __fixed(I, F)
static std::string fixedShown(int code) {
    int bits = code >> 8, f = code & 0xFF;
    return "__fixed(" + std::to_string(bits - f) + ", " + std::to_string(f) + ")";
}

std::string IRBuilder::fixedTypeName(int code) {
    int bits = code >> 8, f = code & 0xFF;
    return "__fixed" + std::to_string(bits - f) + "_" + std::to_string(f);
}

// two operands' fixed types: the wider; of one width, the same one (-1: two
// different ones of one width, which need a cast)
int IRBuilder::fixedCombine(int a, int b) {
    if (!a) return b;
    if (!b) return a;
    if ((a >> 8) != (b >> 8)) return (a >> 8) > (b >> 8) ? a : b;
    return a == b ? a : -1;
}

IRBuilder::IRTypeInfo IRBuilder::fixedInfo(int code) {
    bool w32 = (code >> 8) == 32;
    return {w32 ? ir::Type::I32 : ir::Type::I16, fixedTypeName(code), true, ir::Type::VOID, w32 ? 4 : 2};
}

int IRBuilder::fixedFrac(Expression* e) {
    if (!e) return 0;
    IRTypeInfo ti = getExprTypeInfo(e);
    if (ti.type == ir::Type::PTR) return 0;
    return fixedFracOf(ti.typeName);
}

// A constant e as a fixed value: the integer, rounded to the nearest step
bool IRBuilder::fixedConstRaw(Expression* e, int code, int64_t& raw) {
    // ((__fixed8_8)1.5 and a fixed constant of the same type: the value inside)
    while (auto* c = dynamic_cast<CastExpression*>(e)) {
        if (c->pointerLevel || !fixedFracOf(c->targetType)) break;
        e = c->expression.get();
    }
    if (fixedFrac(e)) return false;                       // (a fixed variable)
    // (a float constant as it is: evalConstInt would drop its fraction)
    ConstValue cv;
    if (!ConstEval::eval(e, *this, cv)) return false;
    double v;
    if (cv.isFloat()) v = cv.fvalue;
    else if (cv.isInt()) v = (double)cv.intValue();
    else return false;
    int bits = code >> 8, frac = code & 0xFF;
    double scaled = std::round(v * std::ldexp(1.0, frac));
    // (out of the type's range: a warning; the integer wraps)
    double lim = std::ldexp(1.0, bits - 1);
    if (scaled >= lim || scaled < -lim) {
        long r = (long)std::ldexp(1.0, bits - 1 - frac);
        warnings_.push_back(formatDiagnostic(e->sourceFile, e->line, e->column, Severity::Warning,
            "the constant does not fit " + fixedShown(code) + " (-" + std::to_string(r) + " to below " +
            std::to_string(r) + "): it wraps"));
    }
    raw = (int64_t)scaled & (bits == 32 ? 0xFFFFFFFFLL : 0xFFFFLL);
    return true;
}

ir::Operand IRBuilder::fixedOp(ir::Op op, ir::Operand a, ir::Operand b, ir::Type t, const ir::SourceLoc& sl) {
    ir::Inst i;
    i.op = op;
    i.dest = allocVreg(t);
    i.resultType = t;
    i.src1 = a;
    i.src2 = b;
    i.loc = sl;
    emit(i);
    return i.dest;
}

// a float constant (FCONST)
static ir::Inst floatConst(ir::Operand dest, double d, const ir::SourceLoc& sl) {
    ir::Inst k;
    k.op = ir::Op::FCONST;
    k.dest = dest;
    k.resultType = ir::Type::F32;
    int64_t bits;
    std::memcpy(&bits, &d, sizeof bits);
    k.src1 = ir::Operand::imm(bits, ir::Type::F32);
    k.loc = sl;
    return k;
}

// a signed integer shifted toward 0 by n bits (a negative one gets 2^n - 1
// added first: the sign spread over the word, masked)
ir::Operand IRBuilder::fixedShiftDown(ir::Operand v, int n, ir::Type t, const ir::SourceLoc& sl) {
    if (n <= 0) return v;
    int bits = 8 * ir::typeSize(t);
    ir::Operand sign = fixedOp(ir::Op::ASR, v, ir::Operand::imm(bits - 1, t), t, sl);
    ir::Operand bias = fixedOp(ir::Op::AND, sign, ir::Operand::imm((int64_t)((1ULL << n) - 1), t), t, sl);
    ir::Operand adj = fixedOp(ir::Op::ADD, v, bias, t, sl);
    return fixedOp(ir::Op::ASR, adj, ir::Operand::imm(n, t), t, sl);
}

// e as a fixed value of type code: its integer (I16 / I32)
ir::Operand IRBuilder::fixedValue(Expression* e, int code, const ir::SourceLoc& sl) {
    int bits = code >> 8, frac = code & 0xFF;
    ir::Type t = bits == 32 ? ir::Type::I32 : ir::Type::I16;
    int64_t raw;
    if (fixedConstRaw(e, code, raw)) return ir::Operand::imm(raw, t);
    int from = fixedFrac(e);
    IRTypeInfo ti = getExprTypeInfo(e);
    e->accept(*this);
    ir::Operand v = lastValue_;
    if (from == code) return emitCast(v, t, true);
    if (from) {
        // another fixed type: in 32 bits, its fraction to this one's
        int ff = from & 0xFF;
        v = emitCast(v, ir::Type::I32, true);
        if (frac > ff) v = fixedOp(ir::Op::SHL, v, ir::Operand::imm(frac - ff, ir::Type::I32), ir::Type::I32, sl);
        else if (ff > frac) v = fixedShiftDown(v, ff - frac, ir::Type::I32, sl);
        return emitCast(v, t, true);
    }
    if (v.type == ir::Type::F32) {
        // a float: times 2^frac, to an integer
        ir::Operand k = allocVreg(ir::Type::F32);
        emit(floatConst(k, std::ldexp(1.0, frac), sl));
        ir::Operand f = fixedOp(ir::Op::FMUL, v, k, ir::Type::F32, sl);
        return emitCast(f, t, true);
    }
    // an integer: in the fixed type's width (by its signedness), shifted up
    v = emitCast(v, t, ti.isSigned);
    return fixedOp(ir::Op::SHL, v, ir::Operand::imm(frac, t), t, sl);
}

// The fixed integer raw (type code) as type t: a float, or an integer (the
// fraction dropped, toward 0 as a float's cast does)
ir::Operand IRBuilder::fixedTo(ir::Operand raw, int code, ir::Type t, bool isSigned, const ir::SourceLoc& sl) {
    int frac = code & 0xFF;
    ir::Type ft = (code >> 8) == 32 ? ir::Type::I32 : ir::Type::I16;
    raw = emitCast(raw, ft, true);
    if (t == ir::Type::F32) {
        ir::Operand f = emitCast(raw, ir::Type::F32, true);
        ir::Operand k = allocVreg(ir::Type::F32);
        emit(floatConst(k, std::ldexp(1.0, -frac), sl));
        return fixedOp(ir::Op::FMUL, f, k, ir::Type::F32, sl);
    }
    (void)isSigned;
    return emitCast(fixedShiftDown(raw, frac, ft, sl), t, true);
}

// The fixed-point arithmetic of a binary operation (true: lastValue_ is
// its value); not one of fixed operands: false
bool IRBuilder::emitFixedBinary(BinaryOperation& node) {
    const std::string& op = node.op;
    int fl = fixedFrac(node.left.get()), fr = fixedFrac(node.right.get());
    if (!fl && !fr) return false;
    // (a pointer or a struct on the other side: not fixed arithmetic)
    IRTypeInfo li = getExprTypeInfo(node.left.get()), ri = getExprTypeInfo(node.right.get());
    if (li.type == ir::Type::PTR || ri.type == ir::Type::PTR) return false;
    bool arith = op == "+" || op == "-" || op == "*" || op == "/";
    bool cmp = op == "<" || op == ">" || op == "<=" || op == ">=" || op == "==" || op == "!=";
    if (!arith && !cmp) {
        if (op == "<<" || op == ">>" || op == "&&" || op == "||" || op == ",") return false;   // (the integer's)
        errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column, Severity::Error,
            "'" + op + "' is not defined on fixed-point values"));
        lastValue_ = ir::Operand::imm(0, ir::Type::I16);
        return true;
    }
    int code = fixedCombine(fl, fr);
    if (code < 0) {
        errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column, Severity::Error,
            fixedShown(fl) + " and " + fixedShown(fr) + " in one operation: cast one to the other"));
        lastValue_ = ir::Operand::imm(0, ir::Type::I16);
        return true;
    }
    int bits = code >> 8, frac = code & 0xFF;
    ir::Type t = bits == 32 ? ir::Type::I32 : ir::Type::I16;
    ir::SourceLoc sl = loc(node);
    // fixed * integer, fixed / integer: the integer times / over the fixed
    // value's integer (exact, nothing scaled: -160 * step with step in
    // __fixed(4, 28) does not make -160 a __fixed(4, 28) first)
    auto isInteger = [&](const IRTypeInfo& ti, Expression* e) {
        ConstValue cv;   // (-0.5 is not one, whatever its type says)
        if (ConstEval::eval(e, *this, cv) && cv.isFloat()) return false;
        return !fixedFrac(e) && (ti.type == ir::Type::I8 || ti.type == ir::Type::I16 || ti.type == ir::Type::I32) &&
               !structOf(ti.typeName);
    };
    bool intRight = isInteger(ri, node.right.get()), intLeft = isInteger(li, node.left.get());
    if ((op == "*" && (intRight || intLeft)) || (op == "/" && intRight)) {
        Expression* fx = intRight ? node.left.get() : node.right.get();
        Expression* n = intRight ? node.right.get() : node.left.get();
        bool nSigned = (intRight ? ri : li).isSigned;
        ir::Operand a = fixedValue(fx, code, sl);
        int64_t c;
        ir::Operand b;
        if (evalConstInt(n, c)) b = ir::Operand::imm(c, t);
        else {
            n->accept(*this);
            b = emitCast(lastValue_, t, nSigned);
        }
        lastValue_ = fixedOp(op == "*" ? ir::Op::MUL : ir::Op::DIV, a, b, t, sl);
        lastValueSigned_ = true;
        return true;
    }
    // 32 bits, * and /: the library (the hardware multiplier / divider)
    if (bits == 32 && (op == "*" || op == "/")) {
        const char* fn = op == "*" ? "__fx32_mul" : "__fx32_div";
        if (!functionReturnTypes_.count(fn)) {
            errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column, Severity::Error,
                std::string(fn) + " is not declared (#include <fixed.h>)"));
            lastValue_ = ir::Operand::imm(0, t);
            return true;
        }
        // __fx32_mul(a, b, F): a and b as the type (their integers)
        std::string tn = fixedTypeName(code);
        FunctionCall call(fn);
        call.line = node.line; call.column = node.column; call.sourceFile = node.sourceFile;
        auto la = std::make_unique<CastExpression>(tn, 0, true, std::move(node.left));
        auto ra = std::make_unique<CastExpression>(tn, 0, true, std::move(node.right));
        auto fa = std::make_unique<IntegerLiteral>(frac);
        for (Expression* c : {(Expression*)la.get(), (Expression*)ra.get(), (Expression*)fa.get()}) {
            c->line = node.line; c->column = node.column; c->sourceFile = node.sourceFile;
        }
        call.arguments.push_back(std::move(la));
        call.arguments.push_back(std::move(ra));
        call.arguments.push_back(std::move(fa));
        visit(call);
        // (the AST stays whole)
        node.left = std::move(static_cast<CastExpression*>(call.arguments[0].get())->expression);
        node.right = std::move(static_cast<CastExpression*>(call.arguments[1].get())->expression);
        lastValue_ = emitCast(lastValue_, ir::Type::I32, true);
        lastValueSigned_ = true;
        return true;
    }
    ir::Operand a = fixedValue(node.left.get(), code, sl);
    ir::Operand b = fixedValue(node.right.get(), code, sl);
    if (cmp) {
        ir::Op c = op == "<" ? ir::Op::CMP_LT : op == ">" ? ir::Op::CMP_GT : op == "<=" ? ir::Op::CMP_LE
                 : op == ">=" ? ir::Op::CMP_GE : op == "==" ? ir::Op::CMP_EQ : ir::Op::CMP_NE;
        lastValue_ = fixedOp(c, a, b, ir::Type::I8, sl);
        lastValueSigned_ = false;
        return true;
    }
    if (op == "+" || op == "-") {
        lastValue_ = fixedOp(op == "+" ? ir::Op::ADD : ir::Op::SUB, a, b, t, sl);
    } else if (op == "*") {
        // 16 bits: the 32-bit product, F bits down (toward 0)
        ir::Operand wa = emitCast(a, ir::Type::I32, true), wb = emitCast(b, ir::Type::I32, true);
        ir::Operand p = fixedOp(ir::Op::MUL, wa, wb, ir::Type::I32, sl);
        lastValue_ = emitCast(fixedShiftDown(p, frac, ir::Type::I32, sl), ir::Type::I16, true);
    } else {
        // 16 bits: (a << F) / b in 32 bits
        ir::Operand wa = emitCast(a, ir::Type::I32, true), wb = emitCast(b, ir::Type::I32, true);
        wa = fixedOp(ir::Op::SHL, wa, ir::Operand::imm(frac, ir::Type::I32), ir::Type::I32, sl);
        ir::Operand q = fixedOp(ir::Op::DIV, wa, wb, ir::Type::I32, sl);
        lastValue_ = emitCast(q, ir::Type::I16, true);
    }
    lastValueSigned_ = true;
    return true;
}

// e converted for a target of type targetType (pointer level ptr): wrapped
// in a cast when a fixed-point value meets another type (either way)
void IRBuilder::fixedConvertTo(std::unique_ptr<Expression>& e, const std::string& targetType, int ptr) {
    if (!e || ptr) return;
    int to = fixedFracOf(targetType), from = fixedFrac(e.get());
    if (to == from) return;
    if (!to && !from) return;
    if (to && dynamic_cast<InitializerList*>(e.get())) return;
    IRTypeInfo ti = getExprTypeInfo(e.get());
    if (ti.type == ir::Type::PTR || structOf(ti.typeName)) return;
    if (!to && structOf(targetType)) return;
    int line = e->line, col = e->column;
    std::string file = e->sourceFile;
    e = std::make_unique<CastExpression>(targetType, 0, true, std::move(e));
    e->line = line; e->column = col; e->sourceFile = file;
}
