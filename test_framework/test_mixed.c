// Leaf function - should use SAC
int leaf_add(int a, int b) {
    return a + b;
}

// Complex function - conservative optimization
int complex_func(int x) {
    int sum = 0;
    for (int i = 0; i < x; i++) {
        if (i % 2 == 0) sum += i;
        else sum -= i;
    }
    return sum;
}

int main() {
    return leaf_add(1, 2) + complex_func(10);
}
