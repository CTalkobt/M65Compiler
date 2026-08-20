// Phase 93: 3D Striped Array Basic Access Test
// Tests basic 3D array indexing with striped optimization

__striped int cube[2][4][4];

int main() {
    // Initialize a 2×4×4 striped array
    int d, r, c;
    int idx = 1;
    for (d = 0; d < 2; d++) {
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 4; c++) {
                cube[d][r][c] = idx++;
            }
        }
    }

    // Test basic access patterns
    if (cube[0][0][0] != 1) return 1;      // First element of first matrix
    if (cube[0][0][3] != 4) return 2;      // Last in first row, first matrix
    if (cube[0][3][3] != 16) return 3;     // Last element of first matrix
    if (cube[1][0][0] != 17) return 4;     // First element of second matrix
    if (cube[1][1][2] != 23) return 5;     // Middle access, second matrix
    if (cube[1][3][3] != 32) return 6;     // Last element of second matrix

    return 0;
}
