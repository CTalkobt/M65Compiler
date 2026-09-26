// Phase 88 Benchmark: Compound Assignment Math
// Tests: numerical algorithms with heavy compound assignment usage
// Expected: 15-25% code reduction for compound chains

#include <stdio.h>

// Accumulator pattern - common in numerical computing
int accumulate_series(int n) {
    int sum = 0;
    int term = 1;
    for (int i = 1; i <= n; i++) {
        term *= i;
        sum += term;
    }
    return sum;
}

// Running statistics with multiple compound operations
void compute_stats(int* data, int len, int* out_sum, int* out_product) {
    int sum = 0;
    int product = 1;
    for (int i = 0; i < len; i++) {
        sum += data[i];
        product *= data[i];
    }
    *out_sum = sum;
    *out_product = product;
}

// Iterative numerical method (Newton's method style)
int iterative_refine(int x, int target, int iterations) {
    for (int i = 0; i < iterations; i++) {
        x += (target - x) / 2;
        x -= x / 10;
    }
    return x;
}

// Compound assignment chains - PRIMARY OPTIMIZATION TARGET
int chained_operations(int a, int b, int c, int d, int e) {
    int result = a;
    result += b;
    result -= c;
    result *= d;
    result += e;
    result -= a;
    result *= 2;
    result += c;
    result -= d;
    result *= e;
    return result;
}

// Mixed compound assignments
long bitwise_accum(int* flags, int len) {
    long acc = 0;
    for (int i = 0; i < len; i++) {
        acc |= flags[i];
        acc &= (flags[i] | 0xFF00);
        acc ^= (flags[i] << i);
    }
    return acc;
}

// Nested compound operations
int nested_compound(int x) {
    int y = x;
    for (int i = 0; i < 5; i++) {
        y += i;
        y -= 1;
        y *= 2;
    }
    return y;
}

int main() {
    printf("sum=%d\n", accumulate_series(5));

    int data[] = {2, 3, 4, 5};
    int s, p;
    compute_stats(data, 4, &s, &p);
    printf("sum=%d prod=%d\n", s, p);

    printf("refined=%d\n", iterative_refine(10, 50, 3));
    printf("chained=%d\n", chained_operations(1, 2, 3, 4, 5));
    printf("bitwise=%ld\n", bitwise_accum(data, 4));
    printf("nested=%d\n", nested_compound(3));

    return 0;
}
