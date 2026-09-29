// Phase 93: 4D Striped Array Test
// Tests 4D arrays where last 2 dims are striped

__striped int tensor[2][2][4][4];

int main() {
    // Initialize 2×2×4×4 striped tensor
    int d1, d2, r, c;
    int idx = 1;
    for (d1 = 0; d1 < 2; d1++) {
        for (d2 = 0; d2 < 2; d2++) {
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 4; c++) {
                    tensor[d1][d2][r][c] = idx++;
                }
            }
        }
    }

    // Test basic 4D accesses
    if (tensor[0][0][0][0] != 1) return 1;      // tensor[0][0][0][0]
    if (tensor[0][0][0][3] != 4) return 2;      // tensor[0][0][0][3]
    if (tensor[0][0][3][3] != 16) return 3;     // tensor[0][0][3][3]
    if (tensor[0][1][0][0] != 17) return 4;     // tensor[0][1][0][0]
    if (tensor[0][1][3][3] != 32) return 5;     // tensor[0][1][3][3]
    if (tensor[1][0][0][0] != 33) return 6;     // tensor[1][0][0][0]
    if (tensor[1][1][3][3] != 64) return 7;     // tensor[1][1][3][3]

    // Test nested access pattern
    int sum = 0;
    for (d1 = 0; d1 < 2; d1++) {
        for (d2 = 0; d2 < 2; d2++) {
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 4; c++) {
                    sum += tensor[d1][d2][r][c];
                }
            }
        }
    }

    // Sum should be 1+2+...+64 = 2080
    if (sum != 2080) return 8;

    return 0;
}
