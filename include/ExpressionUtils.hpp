#pragma once
#include <cstdint>
#include <string>
#include <stdexcept>

/**
 * Unified expression evaluation utilities for compiler and assembler.
 *
 * Provides shared operator evaluation logic for:
 * - Arithmetic: +, -, *, /, %, **
 * - Bitwise: &, |, ^, ~, <<, >>
 * - Comparison: ==, !=, <, <=, >, >=
 * - Logical: &&, ||, !
 * - Ternary: ? :
 *
 * This eliminates code duplication between:
 * - AssemblerExpression.cpp (assembler operator evaluation)
 * - IRBuilder.cpp / CodeGenerator.cpp (compiler operator evaluation)
 */

namespace ExpressionUtils {

/**
 * Evaluate a binary operation on two 32-bit unsigned integers.
 * Supports: +, -, *, /, %, **, &, |, ^, <<, >>, ==, !=, <, <=, >, >=, &&, ||
 */
inline uint32_t evaluateBinaryOp(const std::string& op, uint32_t left, uint32_t right) {
    // Arithmetic
    if (op == "+") return left + right;
    if (op == "-") return left - right;
    if (op == "*") return left * right;
    if (op == "/") {
        if (right == 0) throw std::runtime_error("Division by zero in constant expression");
        return left / right;
    }
    if (op == "%") {
        if (right == 0) throw std::runtime_error("Modulo by zero in constant expression");
        return left % right;
    }
    if (op == "**") {
        // Exponentiation: left ^ right
        uint32_t result = 1;
        for (uint32_t i = 0; i < right; i++) result *= left;
        return result;
    }

    // Bitwise
    if (op == "&") return left & right;
    if (op == "|") return left | right;
    if (op == "^") return left ^ right;
    if (op == "<<") return left << (right & 31);  // Clamp shift to 0-31
    if (op == ">>") return left >> (right & 31);

    // Comparison
    if (op == "==") return (left == right) ? 1 : 0;
    if (op == "!=") return (left != right) ? 1 : 0;
    if (op == "<") return (left < right) ? 1 : 0;
    if (op == "<=") return (left <= right) ? 1 : 0;
    if (op == ">") return (left > right) ? 1 : 0;
    if (op == ">=") return (left >= right) ? 1 : 0;

    // Logical
    if (op == "&&") return ((left != 0) && (right != 0)) ? 1 : 0;
    if (op == "||") return ((left != 0) || (right != 0)) ? 1 : 0;

    throw std::runtime_error("Unknown binary operator: " + op);
}

/**
 * Evaluate a unary operation on a 32-bit unsigned integer.
 * Supports: ~, !, -
 */
inline uint32_t evaluateUnaryOp(const std::string& op, uint32_t operand) {
    if (op == "~") return ~operand;
    if (op == "!") return (operand == 0) ? 1 : 0;
    if (op == "-") return (uint32_t)(-(int32_t)operand);

    throw std::runtime_error("Unknown unary operator: " + op);
}

/**
 * Evaluate a ternary conditional expression.
 */
inline uint32_t evaluateTernary(uint32_t condition, uint32_t trueVal, uint32_t falseVal) {
    return (condition != 0) ? trueVal : falseVal;
}

/**
 * Evaluate a binary operation on two 32-bit signed integers.
 * Used by ConstantFolder for C integer constant expression evaluation.
 * Preserves signed semantics: signed comparison, signed division.
 * Throws on division/modulo by zero (caller handles passthrough).
 */
inline int32_t evaluateSignedBinaryOp(const std::string& op, int32_t left, int32_t right) {
    if (op == "+")  return left + right;
    if (op == "-")  return left - right;
    if (op == "*")  return left * right;
    if (op == "/") {
        if (right == 0) throw std::runtime_error("Division by zero in constant expression");
        return left / right;
    }
    if (op == "%") {
        if (right == 0) throw std::runtime_error("Modulo by zero in constant expression");
        return left % right;
    }
    if (op == "&")  return left & right;
    if (op == "|")  return left | right;
    if (op == "^")  return left ^ right;
    if (op == "<<") return left << (right & 31);
    if (op == ">>") return left >> (right & 31);
    if (op == "==") return (left == right) ? 1 : 0;
    if (op == "!=") return (left != right) ? 1 : 0;
    if (op == "<")  return (left <  right) ? 1 : 0;
    if (op == "<=") return (left <= right) ? 1 : 0;
    if (op == ">")  return (left >  right) ? 1 : 0;
    if (op == ">=") return (left >= right) ? 1 : 0;
    if (op == "&&") return ((left != 0) && (right != 0)) ? 1 : 0;
    if (op == "||") return ((left != 0) || (right != 0)) ? 1 : 0;
    throw std::runtime_error("Unknown binary operator: " + op);
}

/**
 * Evaluate a unary operation on a 32-bit signed integer.
 * Preserves signed semantics for proper C constant folding.
 */
inline int32_t evaluateSignedUnaryOp(const std::string& op, int32_t operand) {
    if (op == "~") return ~operand;
    if (op == "!") return (operand == 0) ? 1 : 0;
    if (op == "-") return -operand;
    throw std::runtime_error("Unknown unary operator: " + op);
}

} // namespace ExpressionUtils
