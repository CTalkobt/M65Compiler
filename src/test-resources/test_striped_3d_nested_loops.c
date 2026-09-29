// Phase 93: 3D Striped Array Nested Loop Test
// Tests nested loop patterns with 3D striped arrays

__striped int matrix[3][8][8];

void fill_matrix() {
    int d, r, c, val = 0;
    for (d = 0; d < 3; d++) {
        for (r = 0; r < 8; r++) {
            for (c = 0; c < 8; c++) {
                matrix[d][r][c] = val++;
            }
        }
    }
}

int sum_plane(int depth) {
    int sum = 0, r, c;
    for (r = 0; r < 8; r++) {
        for (c = 0; c < 8; c++) {
            sum += matrix[depth][r][c];
        }
    }
    return sum;
}

int main() {
    fill_matrix();

    // Test sums of each plane
    int s0 = sum_plane(0);  // Sum of plane 0: 0..63 = 2016
    int s1 = sum_plane(1);  // Sum of plane 1: 64..127 = 6080
    int s2 = sum_plane(2);  // Sum of plane 2: 128..191 = 10144

    if (s0 != 2016) return 1;
    if (s1 != 6080) return 2;
    if (s2 != 10144) return 3;

    // Test row sums
    int row_sum = 0, c;
    for (c = 0; c < 8; c++) {
        row_sum += matrix[1][2][c];
    }
    // Row 2 of plane 1: indices 16..23, values 80..87
    int expected = 80 + 81 + 82 + 83 + 84 + 85 + 86 + 87;  // 668
    if (row_sum != expected) return 4;

    // Test column sum
    int col_sum = 0, r;
    for (r = 0; r < 8; r++) {
        col_sum += matrix[2][r][3];
    }
    // Column 3 of plane 2: indices 3,11,19,27,35,43,51,59
    // Values: 131,139,147,155,163,171,179,187 = 1272
    expected = 131 + 139 + 147 + 155 + 163 + 171 + 179 + 187;
    if (col_sum != expected) return 5;

    return 0;
}
