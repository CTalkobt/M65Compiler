// IRBuilderFixed.cpp — fixed-point types (#72): __fixed8_8 (16 bits, 8 of
// them the fraction) and __fixed16_16 (32 bits, 16): signed integers whose
// value is the integer / 2^frac. Constants are scaled when compiled; + - and
// comparisons are the integers'; * and / scale (8.8 inline, 16.16 by
// __fx16_mul / __fx16_div of <fixed.h>); int, float and the other fixed
// type convert to and from them (casts, assignments, arguments, returns).
#include "IRBuilder.hpp"
#include "Diagnostic.hpp"
#include "ConstEval.hpp"
#include <cmath>
#include <cstring>

int IRBuilder::fixedFracOf(const std::string& typeName) {
    if (typeName == "__fixed8_8") return 8;
    if (typeName == "__fixed16_16") return 16;
    return 0;
}

const char* IRBuilder::fixedTypeName(int frac) { return frac == 16 ? "__fixed16_16" : "__fixed8_8"; }

int IRBuilder::fixedFrac(Expression* e) {
    if (!e) return 0;
    IRTypeInfo ti = getExprTypeInfo(e);
    if (ti.type == ir::Type::PTR) return 0;
    return fixedFracOf(ti.typeName);
}

// A constant e as a fixed value: the integer, rounded to the nearest step
bool IRBuilder::fixedConstRaw(Expression* e, int frac, int64_t& raw) {
    // (__fixed8_8)1.5 and a fixed constant of the same type: the value inside
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
    double scaled = std::round(v * (double)(1L << frac));
    raw = (int64_t)scaled;
    raw &= frac == 16 ? 0xFFFFFFFFLL : 0xFFFFLL;
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

// e as a fixed value with frac fraction bits: its integer (I16 / I32)
ir::Operand IRBuilder::fixedValue(Expression* e, int frac, const ir::SourceLoc& sl) {
    ir::Type t = frac == 16 ? ir::Type::I32 : ir::Type::I16;
    int64_t raw;
    if (fixedConstRaw(e, frac, raw)) return ir::Operand::imm(raw, t);
    int from = fixedFrac(e);
    IRTypeInfo ti = getExprTypeInfo(e);
    e->accept(*this);
    ir::Operand v = lastValue_;
    if (from == frac) return emitCast(v, t, true);
    if (from) {
        // 8.8 -> 16.16: widened, 8 more fraction bits; 16.16 -> 8.8: 8 fewer
        if (from < frac) {
            v = emitCast(v, ir::Type::I32, true);
            return fixedOp(ir::Op::SHL, v, ir::Operand::imm(8, ir::Type::I32), ir::Type::I32, sl);
        }
        v = fixedOp(ir::Op::ASR, v, ir::Operand::imm(8, ir::Type::I32), ir::Type::I32, sl);
        return emitCast(v, ir::Type::I16, true);
    }
    if (v.type == ir::Type::F32) {
        // a float: times 2^frac, to an integer
        ir::Inst k;
        k.op = ir::Op::FCONST;
        k.dest = allocVreg(ir::Type::F32);
        k.resultType = ir::Type::F32;
        double scale = (double)(1L << frac);
        int64_t bits;
        std::memcpy(&bits, &scale, sizeof bits);
        k.src1 = ir::Operand::imm(bits, ir::Type::F32);
        k.loc = sl;
        emit(k);
        ir::Operand f = fixedOp(ir::Op::FMUL, v, k.dest, ir::Type::F32, sl);
        return emitCast(f, t, true);
    }
    // an integer: in the fixed type's width (by its signedness), shifted up
    v = emitCast(v, t, ti.isSigned);
    return fixedOp(ir::Op::SHL, v, ir::Operand::imm(frac, t), t, sl);
}

// The fixed integer raw (frac fraction bits) as type t: a float, or an
// integer (the fraction dropped, toward 0 as a float's cast does)
ir::Operand IRBuilder::fixedTo(ir::Operand raw, int frac, ir::Type t, bool isSigned, const ir::SourceLoc& sl) {
    ir::Type ft = frac == 16 ? ir::Type::I32 : ir::Type::I16;
    raw = emitCast(raw, ft, true);
    if (t == ir::Type::F32) {
        ir::Operand f = emitCast(raw, ir::Type::F32, true);
        ir::Inst k;
        k.op = ir::Op::FCONST;
        k.dest = allocVreg(ir::Type::F32);
        k.resultType = ir::Type::F32;
        double scale = 1.0 / (double)(1L << frac);
        int64_t bits;
        std::memcpy(&bits, &scale, sizeof bits);
        k.src1 = ir::Operand::imm(bits, ir::Type::F32);
        k.loc = sl;
        emit(k);
        return fixedOp(ir::Op::FMUL, f, k.dest, ir::Type::F32, sl);
    }
    // toward 0: a negative value gets 2^frac - 1 added before the shift (the
    // sign spread over the word, masked)
    int bits = 8 * ir::typeSize(ft);
    ir::Operand sign = fixedOp(ir::Op::ASR, raw, ir::Operand::imm(bits - 1, ft), ft, sl);
    ir::Operand bias = fixedOp(ir::Op::AND, sign, ir::Operand::imm((1L << frac) - 1, ft), ft, sl);
    ir::Operand adj = fixedOp(ir::Op::ADD, raw, bias, ft, sl);
    ir::Operand whole = fixedOp(ir::Op::ASR, adj, ir::Operand::imm(frac, ft), ft, sl);
    return emitCast(whole, t, true);
    (void)isSigned;
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
    int frac = std::max(fl, fr);
    ir::Type t = frac == 16 ? ir::Type::I32 : ir::Type::I16;
    ir::SourceLoc sl = loc(node);
    // 16.16 * and /: the library (the hardware multiplier / divider)
    if (frac == 16 && (op == "*" || op == "/")) {
        const char* fn = op == "*" ? "__fx16_mul" : "__fx16_div";
        if (!functionReturnTypes_.count(fn)) {
            errors_.push_back(formatDiagnostic(node.sourceFile, node.line, node.column, Severity::Error,
                std::string(fn) + " is not declared (#include <fixed.h>)"));
            lastValue_ = ir::Operand::imm(0, t);
            return true;
        }
        FunctionCall call(fn);
        call.line = node.line; call.column = node.column; call.sourceFile = node.sourceFile;
        auto la = std::make_unique<CastExpression>("__fixed16_16", 0, true, std::move(node.left));
        auto ra = std::make_unique<CastExpression>("__fixed16_16", 0, true, std::move(node.right));
        for (auto* c : {la.get(), ra.get()}) { c->line = node.line; c->column = node.column; c->sourceFile = node.sourceFile; }
        call.arguments.push_back(std::move(la));
        call.arguments.push_back(std::move(ra));
        visit(call);
        // (the AST stays whole)
        node.left = std::move(static_cast<CastExpression*>(call.arguments[0].get())->expression);
        node.right = std::move(static_cast<CastExpression*>(call.arguments[1].get())->expression);
        lastValueSigned_ = true;
        return true;
    }
    ir::Operand a = fixedValue(node.left.get(), frac, sl);
    ir::Operand b = fixedValue(node.right.get(), frac, sl);
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
        // 8.8: the 32-bit product, 8 bits down (toward 0, as 16.16's: a
        // negative one gets 255 added first)
        ir::Operand wa = emitCast(a, ir::Type::I32, true), wb = emitCast(b, ir::Type::I32, true);
        ir::Operand p = fixedOp(ir::Op::MUL, wa, wb, ir::Type::I32, sl);
        ir::Operand sign = fixedOp(ir::Op::ASR, p, ir::Operand::imm(31, ir::Type::I32), ir::Type::I32, sl);
        ir::Operand bias = fixedOp(ir::Op::AND, sign, ir::Operand::imm(255, ir::Type::I32), ir::Type::I32, sl);
        p = fixedOp(ir::Op::ADD, p, bias, ir::Type::I32, sl);
        p = fixedOp(ir::Op::ASR, p, ir::Operand::imm(8, ir::Type::I32), ir::Type::I32, sl);
        lastValue_ = emitCast(p, ir::Type::I16, true);
    } else {
        // 8.8: (a << 8) / b in 32 bits
        ir::Operand wa = emitCast(a, ir::Type::I32, true), wb = emitCast(b, ir::Type::I32, true);
        wa = fixedOp(ir::Op::SHL, wa, ir::Operand::imm(8, ir::Type::I32), ir::Type::I32, sl);
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
