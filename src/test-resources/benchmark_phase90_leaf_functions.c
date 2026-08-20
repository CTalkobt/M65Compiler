/**
 * Phase 90.5 Benchmark: Leaf Functions
 *
 * Measures code size savings for leaf functions (no locals, no calls).
 * Expected: 15-20 bytes saved per leaf function
 */

// Simple leaf functions - should skip FP setup entirely
int abs_value(int x) {
    return (x < 0) ? -x : x;
}

int max_of_two(int a, int b) {
    return (a > b) ? a : b;
}

int min_of_two(int a, int b) {
    return (a < b) ? a : b;
}

int clamp(int val, int min_v, int max_v) {
    if (val < min_v) return min_v;
    if (val > max_v) return max_v;
    return val;
}

int is_even(int n) {
    return (n & 1) == 0;
}

int is_positive(int n) {
    return n > 0;
}

int bit_count(int x) {
    int count = 0;
    if (x & 0x0001) count++;
    if (x & 0x0002) count++;
    if (x & 0x0004) count++;
    if (x & 0x0008) count++;
    if (x & 0x0010) count++;
    if (x & 0x0020) count++;
    if (x & 0x0040) count++;
    if (x & 0x0080) count++;
    return count;
}

int sign_of(int x) {
    if (x > 0) return 1;
    if (x < 0) return -1;
    return 0;
}

int swap_bytes(int x) {
    int lo = x & 0xFF;
    int hi = (x >> 8) & 0xFF;
    return (lo << 8) | hi;
}

int main(void) {
    int results = 0;
    results += abs_value(-42);
    results += max_of_two(10, 20);
    results += min_of_two(10, 20);
    results += clamp(50, 0, 100);
    results += is_even(42);
    results += is_positive(-5);
    results += bit_count(0xFF);
    results += sign_of(-10);
    results += swap_bytes(0x1234);
    return results;
}
