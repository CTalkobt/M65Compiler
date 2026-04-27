// Test: Type promotion, widening, narrowing, and explicit casts
// Validates correct code generation for mixed-type expressions.

int main() {
    // char widened to int in arithmetic
    char c1 = 200;
    int i1 = 300;
    int r1 = c1 + i1;
    if (r1 != 500) return 1;

    // char * int -> int result
    char c2 = 10;
    int r2 = c2 * 30;
    if (r2 != 300) return 2;

    // int narrowed to char (truncation via assignment)
    int big = 0x1234;
    char low = big;  // truncates to low byte
    if (low != 0x34) return 3;

    // char compared with int (char promoted)
    char c3 = 255;
    int i3 = 255;
    if (c3 != i3) return 4;

    // Arithmetic overflow in char, then widen
    char a = 200;
    char b = 100;
    char sum_c = a + b;  // wraps to 44
    if (sum_c != 44) return 5;

    // Same addition but stored in int (widened before wrap)
    int sum_i = a + b;
    if (sum_i != 300) return 6;

    // Int narrowed to char in expression via intermediate variable
    int x = 1000;
    char x_trunc = x;  // truncates: 1000 & 0xFF = 232
    int y = x_trunc + 1;  // 232 + 1 = 233
    if (y != 233) return 7;

    // Negative-ish char values (unsigned char, so 0xFF = 255)
    char ff = 0xFF;
    int wide = ff;  // should be 255, not -1 (unsigned char)
    if (wide != 255) return 8;

    // Char in conditional (nonzero = true)
    char flag = 1;
    int cond_result = 0;
    if (flag) {
        cond_result = 42;
    }
    if (cond_result != 42) return 9;

    // Zero char in conditional
    char zero = 0;
    cond_result = 99;
    if (zero) {
        cond_result = 0;
    }
    if (cond_result != 99) return 10;

    return 0;
}
