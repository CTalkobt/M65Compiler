// Phase 94: Striped Struct Array - Basic Access Test
// Tests basic struct array indexing with striped optimization

struct Point {
    int x, y;  // 8 bytes total
};

__striped struct Point matrix[4][4];

int main() {
    // Initialize a 4×4 striped struct array
    int d, r, c, val = 0;
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
            matrix[r][c].x = val++;
            matrix[r][c].y = val++;
        }
    }

    // Test basic access patterns
    if (matrix[0][0].x != 0) return 1;      // First element x
    if (matrix[0][0].y != 1) return 2;      // First element y
    if (matrix[0][3].x != 6) return 3;      // Last in first row x
    if (matrix[0][3].y != 7) return 4;      // Last in first row y
    if (matrix[3][3].x != 30) return 5;     // Last element x
    if (matrix[3][3].y != 31) return 6;     // Last element y

    // Test middle access
    if (matrix[2][1].x != 18) return 7;
    if (matrix[2][1].y != 19) return 8;

    return 0;
}
