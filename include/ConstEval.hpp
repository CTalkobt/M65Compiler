#pragma once
// ConstEval — cc45's one evaluator of constant expressions (Issue #93)
//
// The arithmetic of ConstantFolder (which folds literals in the AST) and the
// evaluation of whole expressions for static initializers and case labels
// (IRBuilder) share these rules:
//   - an integer is a value and the type a literal carries (IntegerLiteral's
//     castType / castPointerLevel / castIsSigned): a plain literal is int,
//     signed while it fits a signed int, long when outside 16 bits; a
//     suffix or a cast gives the type
//   - two plain literals compute in 32 bits (256 * 256 is 65536); otherwise
//     the usual arithmetic conversions, wrapped to the common type's width
//   - a cast records its type on the value (char casts keep the low byte)
//   - sizeof and _Alignof are plain literals
// What the evaluator cannot know itself (the size of a variable, a declared
// alignment) it asks its caller through ConstEvalContext.
#include "AST.hpp"
#include <cstdint>
#include <string>

struct ConstValue {
    enum class Kind { None, Int, Float } kind = Kind::None;
    int64_t value = 0;             // Int: as IntegerLiteral::value
    std::string castType;          // Int: the type a literal carries ("" = plain)
    int castPointerLevel = 0;
    bool castIsSigned = false;
    double fvalue = 0;             // Float

    static ConstValue ofInt(int64_t v) { ConstValue c; c.kind = Kind::Int; c.value = v; return c; }
    static ConstValue ofFloat(double v) { ConstValue c; c.kind = Kind::Float; c.fvalue = v; return c; }
    static ConstValue ofLiteral(const IntegerLiteral& l);
    bool isInt() const { return kind == Kind::Int; }
    bool isFloat() const { return kind == Kind::Float; }
    // The integer as its type says: (signed char)200 is -56, (unsigned char)300 is 44
    int64_t intValue() const;
    // As a number (an Int by its type)
    double asDouble() const;
    // An IntegerLiteral of an Int (its value and type)
    std::unique_ptr<IntegerLiteral> toLiteral() const;
};

// What the caller knows: sizes and alignments of expressions and types
class ConstEvalContext {
public:
    virtual ~ConstEvalContext() = default;
    virtual bool sizeOf(SizeofExpression& node, int64_t& out) { (void)node; (void)out; return false; }
    virtual bool alignOf(AlignofExpression& node, int64_t& out) { (void)node; (void)out; return false; }
};

namespace ConstEval {

// A literal's width in bytes and signedness (as IRBuilder::getExprTypeInfo)
int litWidth(const ConstValue& v);
bool litSigned(const ConstValue& v);
// v wrapped to width bytes, sign-extended when isSigned
int64_t normalize(int64_t v, int width, bool isSigned);

// One operator on constants: false when it is not a constant (a division by
// zero, an operator that has none, a float operand of an integer operator)
bool binary(const std::string& op, const ConstValue& l, const ConstValue& r, ConstValue& out);
bool unary(const std::string& op, const ConstValue& v, ConstValue& out);
// (type) v: toAggregate / a struct or union target is not a constant
bool cast(const std::string& type, int pointerLevel, bool isSigned, const ConstValue& v, ConstValue& out);
bool isFloatType(const std::string& type, int pointerLevel);

// A whole expression: literals, unary and binary operators (&&, || and ?:
// evaluate only the operand that counts), casts, sizeof / _Alignof
bool eval(Expression* e, ConstEvalContext& ctx, ConstValue& out);

}  // namespace ConstEval
