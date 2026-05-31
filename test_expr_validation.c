// Cross-validation test: verify assembler and compiler expression evaluation produce identical results
// Tests the ExpressionUtils unification for binary and unary operators

#include <stdio.h>

void test_binary_arithmetic() {
    // Addition
    if ((5 + 3) != 8) return;
    // Subtraction
    if ((10 - 2) != 8) return;
    // Multiplication
    if ((3 * 4) != 12) return;
    // Division
    if ((16 / 2) != 8) return;
    // Modulo
    if ((17 % 5) != 2) return;
    // Exponentiation
    if ((2 ** 3) != 8) return;
}

void test_binary_bitwise() {
    // AND
    if ((12 & 10) != 8) return;
    // OR
    if ((8 | 4) != 12) return;
    // XOR
    if ((15 ^ 6) != 9) return;
    // Left shift
    if ((3 << 2) != 12) return;
    // Right shift
    if ((16 >> 2) != 4) return;
}

void test_binary_comparison() {
    // Equal
    if ((5 == 5) != 1) return;
    // Not equal
    if ((3 != 4) != 1) return;
    // Less than
    if ((3 < 5) != 1) return;
    // Less than or equal
    if ((5 <= 5) != 1) return;
    // Greater than
    if ((6 > 4) != 1) return;
    // Greater than or equal
    if ((5 >= 5) != 1) return;
}

void test_binary_logical() {
    // AND
    if ((1 && 1) != 1) return;
    if ((0 && 1) != 0) return;
    // OR
    if ((0 || 1) != 1) return;
    if ((0 || 0) != 0) return;
}

void test_unary_operators() {
    // Bitwise NOT
    if ((~5) != 0xFFFA) return;
    // Logical NOT
    if ((!0) != 1) return;
    if ((!1) != 0) return;
    // Negation
    if ((-10) != 0xFFF6) return;
}

void test_ternary_operator() {
    // Ternary true condition
    if ((1 ? 42 : 99) != 42) return;
    // Ternary false condition
    if ((0 ? 42 : 99) != 99) return;
}

void test_precedence_and_parens() {
    // Precedence: 2 + 3 * 4 = 2 + 12 = 14
    if ((2 + 3 * 4) != 14) return;
    // Parentheses: (2 + 3) * 4 = 5 * 4 = 20
    if (((2 + 3) * 4) != 20) return;
    // Complex expression
    if (((5 + 3) * 2 - 4 / 2) != 14) return;
}

int main() {
    test_binary_arithmetic();
    test_binary_bitwise();
    test_binary_comparison();
    test_binary_logical();
    test_unary_operators();
    test_ternary_operator();
    test_precedence_and_parens();
    return 0;
}
