// Phase 94 + Phase 93: 3D Striped Struct Array Test
// Tests 3D arrays of structs with depth offset + striped 2D

struct Color {
    unsigned char r, g, b;  // 3 bytes (aligned to 4)
};

__striped struct Color cube[2][4][4];

int main() {
    // Initialize 2×4×4 striped struct array
    int d, r, c, idx = 0;
    for (d = 0; d < 2; d++) {
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 4; c++) {
                cube[d][r][c].r = idx + 0;
                cube[d][r][c].g = idx + 1;
                cube[d][r][c].b = idx + 2;
                idx += 3;
            }
        }
    }

    // Test depth 0
    if (cube[0][0][0].r != 0) return 1;
    if (cube[0][0][0].g != 1) return 2;
    if (cube[0][0][0].b != 2) return 3;

    // Test depth 0, different position
    if (cube[0][2][1].r != 21) return 4;
    if (cube[0][2][1].g != 22) return 5;

    // Test depth 1
    if (cube[1][0][0].r != 48) return 6;
    if (cube[1][0][0].g != 49) return 7;
    if (cube[1][3][3].r != 93) return 8;

    return 0;
}
