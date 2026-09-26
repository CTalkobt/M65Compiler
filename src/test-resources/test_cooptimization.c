// Test co-optimization grouping detection
// Phase 86: Identifies groups of related functions for coordinated optimization

// Group 1: Small utility functions (< 20 bytes each)
int clamp_min(int val, int min) {
    if (val < min) return min;
    return val;
}

int clamp_max(int val, int max) {
    if (val > max) return max;
    return val;
}

int clamp(int val, int min, int max) {
    return clamp_max(clamp_min(val, min), max);
}

// Group 2: Related loop helpers
int sum_range(int start, int end) {
    int sum = 0;
    for (int i = start; i <= end; i++) {
        sum += i;
    }
    return sum;
}

int product_range(int start, int end) {
    int prod = 1;
    for (int i = start; i <= end; i++) {
        prod *= i;
    }
    return prod;
}

// Main function using both groups
int main() {
    int a = clamp(15, 10, 20);
    int b = clamp(5, 10, 20);
    int c = clamp(25, 10, 20);

    int sum = sum_range(1, 5);
    int prod = product_range(1, 5);

    return a + b + c + sum + prod;
}
