// Phase 92.3 Test: Basic striped array access
// Tests that __striped keyword is recognized and generates optimized indexing

__striped int matrix[8][8];

int main() {
    // Simple striped array write and read
    matrix[2][3] = 42;
    int val = matrix[2][3];
    return val;  // Should return 42
}
