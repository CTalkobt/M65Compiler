// Phase 92.4 Test: Striped array initialization
// Tests that __striped arrays are correctly initialized in striped layout

// Test 1: Simple static striped array with initializer
__striped int sprite[4][4] = {
    {1, 2, 3, 4},
    {5, 6, 7, 8},
    {9, 10, 11, 12},
    {13, 14, 15, 16}
};

// Test 2: Larger striped array
__striped int matrix[8][8] = {
    {1, 2, 3, 4, 5, 6, 7, 8},
    {9, 10, 11, 12, 13, 14, 15, 16},
    {17, 18, 19, 20, 21, 22, 23, 24},
    {25, 26, 27, 28, 29, 30, 31, 32},
    {33, 34, 35, 36, 37, 38, 39, 40},
    {41, 42, 43, 44, 45, 46, 47, 48},
    {49, 50, 51, 52, 53, 54, 55, 56},
    {57, 58, 59, 60, 61, 62, 63, 64}
};

int main() {
    // Test reading initialized values
    // Verify sprite[row][col] returns correct value

    // Expected: sprite[0][0] = 1
    int val00 = sprite[0][0];
    if (val00 != 1) return 1;

    // Expected: sprite[0][3] = 4
    int val03 = sprite[0][3];
    if (val03 != 4) return 2;

    // Expected: sprite[2][1] = 10
    int val21 = sprite[2][1];
    if (val21 != 10) return 3;

    // Expected: sprite[3][3] = 16
    int val33 = sprite[3][3];
    if (val33 != 16) return 4;

    // Test larger matrix
    int m00 = matrix[0][0];
    if (m00 != 1) return 5;

    int m77 = matrix[7][7];
    if (m77 != 64) return 6;

    int m34 = matrix[3][4];
    if (m34 != 30) return 7;

    return 0;  // Success
}
