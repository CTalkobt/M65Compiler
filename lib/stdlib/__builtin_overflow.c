/* __builtin_add_overflow.c — fallback for GCC intrinsic */

#include <stdint.h>

int __builtin_add_overflow(uint32_t a, uint32_t b, uint32_t *res) {
    uint32_t sum = a + b;
    *res = sum;
    // Unsigned overflow: sum is less than either operand
    return (sum < a);
}

int __builtin_mul_overflow(uint32_t a, uint32_t b, uint32_t *res) {
    uint32_t prod = a * b;
    *res = prod;
    if (a == 0 || b == 0) return 0;
    // Check if prod / a == b
    return (prod / a != b);
}
