// Test: Operator precedence and associativity
// Validates that expressions evaluate in the correct order without extra parens.

int main() {
    // Multiplication before addition: 2 + 3 * 4 = 14
    int r1 = 2 + 3 * 4;
    if (r1 != 14) return 1;

    // Subtraction and multiplication: 10 - 2 * 3 = 4
    int r2 = 10 - 2 * 3;
    if (r2 != 4) return 2;

    // Left-to-right for same precedence: 20 - 5 - 3 = 12
    int r3 = 20 - 5 - 3;
    if (r3 != 12) return 3;

    // Bitwise AND lower than comparison: (5 & 3) == 1
    int r4 = 5 & 3;
    if (r4 != 1) return 4;

    // Bitwise OR lower than AND: 1 | 2 & 3 = 1 | (2 & 3) = 1 | 2 = 3
    int r5 = 1 | 2 & 3;
    if (r5 != 3) return 5;

    // XOR between AND and OR: 5 ^ 3 | 1 = (5 ^ 3) | 1 ... wait
    // Precedence: & > ^ > |
    // 5 & 3 ^ 1 | 2 = ((5 & 3) ^ 1) | 2 = (1 ^ 1) | 2 = 0 | 2 = 2
    int r6 = 5 & 3 ^ 1 | 2;
    if (r6 != 2) return 6;

    // Shift lower than add: 1 + 2 << 3 = (1 + 2) << 3 = 24
    // Actually in C, << is lower than +, so: 1 + (2 << 3) = 1 + 16 = 17
    // Wait, C precedence: + is higher than <<
    // So: (1 + 2) << 3 = 3 << 3 = 24
    // No! << has lower precedence than +
    // + has precedence 6, << has precedence 7 (lower number = higher precedence? no.)
    // In C: multiplicative > additive > shift
    // So 1 + 2 << 3 = (1 + 2) << 3 = 24
    int r7 = 1 + 2 << 3;
    if (r7 != 24) return 7;

    // Parentheses override: (2 + 3) * 4 = 20
    int r8 = (2 + 3) * 4;
    if (r8 != 20) return 8;

    // Logical AND lower than bitwise OR: 3 | 0 && 0 | 1 = (3 | 0) && (0 | 1) = 3 && 1 = 1
    int r9 = 3 | 0 && 0 | 1;
    // Actually: && has lower precedence than |
    // So: (3 | 0) && (0 | 1) = 3 && 1 = 1
    if (r9 != 1) return 9;

    // Unary minus with multiply: -3 * 2 = -6
    int r10 = -3 * 2;
    if (r10 != -6) return 10;

    // Comparison chaining: 5 > 3 gives 1, then 1 < 10 gives 1... but C evaluates left-to-right
    // (5 > 3) == 1
    int r11 = 5 > 3;
    if (r11 != 1) return 11;

    // Nested: a + b * c - d
    int a = 2;
    int b = 3;
    int c = 4;
    int d = 5;
    int r12 = a + b * c - d;  // 2 + 12 - 5 = 9
    if (r12 != 9) return 12;

    return 0;
}
