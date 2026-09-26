// Phase 88 Benchmark: Compound Assignment Algorithms
// Tests: algorithmic patterns with compound assignments
// Expected: 15-25% code reduction for common algorithmic patterns

#include <stdio.h>

// Running average with compound assignment
int running_average(int* values, int count) {
    int sum = 0;
    int weighted_sum = 0;
    for (int i = 0; i < count; i++) {
        sum += values[i];
        weighted_sum += values[i] * (i + 1);
    }
    return sum > 0 ? weighted_sum / sum : 0;
}

// Fibonacci-style recurrence with compound ops
int fibonacci_compound(int n) {
    int a = 0, b = 1;
    for (int i = 0; i < n; i++) {
        int temp = a;
        a = b;
        b += temp;  // b = b + a (compound style)
    }
    return a;
}

// Digit sum with compound operations
int digit_sum(int num) {
    int sum = 0;
    while (num > 0) {
        sum += num % 10;
        num /= 10;
    }
    return sum;
}

// GCD calculation with compound assignments
int gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

// Counting set bits with compound ops
int count_bits(int num) {
    int count = 0;
    while (num > 0) {
        count += num & 1;
        num >>= 1;
    }
    return count;
}

// Polynomial evaluation with Horner's method
int horner_poly(int x, int* coeffs, int degree) {
    int result = coeffs[0];
    for (int i = 1; i <= degree; i++) {
        result *= x;
        result += coeffs[i];
    }
    return result;
}

// Running variance calculation
int variance_calc(int* data, int count) {
    int sum = 0;
    int sum_sq = 0;
    for (int i = 0; i < count; i++) {
        sum += data[i];
        sum_sq += data[i] * data[i];
    }
    int mean = sum / count;
    return (sum_sq / count) - (mean * mean);
}

// Exponential moving average
int ema_update(int current, int ema_prev, int alpha) {
    int delta = current - ema_prev;
    delta *= alpha;
    int ema = ema_prev;
    ema += delta / 100;
    return ema;
}

// String to integer conversion with compound ops
int string_to_int(const char* str) {
    int result = 0;
    for (const char* p = str; *p; p++) {
        result *= 10;
        result += (*p - '0');
    }
    return result;
}

// Checksum calculation
unsigned int checksum(unsigned char* data, int len) {
    unsigned int check = 0;
    for (int i = 0; i < len; i++) {
        check += data[i];
        check ^= (data[i] << (i & 7));
    }
    return check;
}

int main() {
    int values[] = {10, 20, 30, 40, 50};
    printf("avg=%d\n", running_average(values, 5));

    printf("fib(10)=%d\n", fibonacci_compound(10));

    printf("digitsum(12345)=%d\n", digit_sum(12345));
    printf("gcd(48,18)=%d\n", gcd(48, 18));
    printf("bits(15)=%d\n", count_bits(15));

    int poly[] = {1, 2, 3, 4};
    printf("horner(2)=%d\n", horner_poly(2, poly, 3));

    int data[] = {1, 2, 3, 4, 5};
    printf("var=%d\n", variance_calc(data, 5));

    printf("ema=%d\n", ema_update(150, 100, 30));

    unsigned char buf[] = {0x48, 0x65, 0x6C, 0x6C, 0x6F};
    printf("check=%u\n", checksum(buf, 5));

    return 0;
}
