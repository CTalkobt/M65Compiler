// Test call graph analysis and cross-function optimization detection
// Demonstrates: leaf functions, single-caller candidates, reachability analysis

// Leaf function: no calls, safe to inline (single caller)
int simple_add(int a, int b) {
    return a + b;
}

// Leaf function: no calls, safe to inline (multiple callers)
int simple_mul(int x, int y) {
    return x * y;
}

// Leaf function: pure computation
int square(int n) {
    return n * n;
}

// Helper for recursion test (never actually called)
int unused_helper(int n) {
    return n + 1;
}

// Caller of simple functions
int compute_sum(int a, int b, int c) {
    int s = simple_add(a, b);
    return simple_add(s, c);
}

// Another caller of simple functions
int compute_product(int x, int y, int z) {
    return simple_mul(simple_mul(x, y), z);
}

// Chain caller: good for call graph analysis
int chain_func(int n) {
    return square(simple_add(n, 1));
}

int main() {
    int a = compute_sum(1, 2, 3);
    int b = compute_product(2, 3, 4);
    int c = chain_func(5);
    return a + b + c;
}
