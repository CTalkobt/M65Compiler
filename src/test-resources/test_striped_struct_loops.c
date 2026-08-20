// Phase 94: Striped Struct Array Nested Loop Test
// Tests loop patterns with struct field access

struct Vector {
    int x, y;  // 8 bytes
};

__striped struct Vector matrix[8][8];

void init_matrix() {
    int r, c, val = 0;
    for (r = 0; r < 8; r++) {
        for (c = 0; c < 8; c++) {
            matrix[r][c].x = val++;
            matrix[r][c].y = val++;
        }
    }
}

int sum_row(int row) {
    int sum = 0, c;
    for (c = 0; c < 8; c++) {
        sum += matrix[row][c].x;
        sum += matrix[row][c].y;
    }
    return sum;
}

int sum_col(int col) {
    int sum = 0, r;
    for (r = 0; r < 8; r++) {
        sum += matrix[r][col].x;
        sum += matrix[r][col].y;
    }
    return sum;
}

int main() {
    init_matrix();

    // Test row sum
    int r_sum = sum_row(0);  // Sum of 0..15 = 120
    if (r_sum != 120) return 1;

    // Test column sum
    int c_sum = sum_col(0);  // Sum of 0, 2, 4, 6, ... 14 + 1, 3, 5, ... 15
    if (c_sum != 120) return 2;

    // Test different row
    int r_sum_2 = sum_row(1);  // Sum of 16..31 = 376
    if (r_sum_2 != 376) return 3;

    // Test element-by-element sum
    int total = 0, r, c;
    for (r = 0; r < 8; r++) {
        for (c = 0; c < 8; c++) {
            total += matrix[r][c].x;
            total += matrix[r][c].y;
        }
    }
    // Sum of 0..127 = 8128
    if (total != 8128) return 4;

    return 0;
}
