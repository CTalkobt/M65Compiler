// Phase 4.2: IPO Hints Emission Test
// Verifies that IPO hints are collected and embedded in .o45 files

int global_value = 42;

int simple_leaf_func(int x) {
    return x + 1;
}

int caller_func(int a, int b) {
    return simple_leaf_func(a) + simple_leaf_func(b);
}

int main(void) {
    return caller_func(10, 20);
}
