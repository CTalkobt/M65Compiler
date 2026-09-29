// Phase 94: Striped Struct Array Initialization Test
// Tests static initialization with automatic data reorganization

struct Vertex {
    int x, y;  // 8 bytes
};

__striped struct Vertex mesh[4][4] = {
    {
        {1, 2}, {3, 4}, {5, 6}, {7, 8},
        {9, 10}, {11, 12}, {13, 14}, {15, 16},
        {17, 18}, {19, 20}, {21, 22}, {23, 24},
        {25, 26}, {27, 28}, {29, 30}, {31, 32}
    }
};

int main() {
    // Test initialization correctness
    if (mesh[0][0].x != 1) return 1;
    if (mesh[0][0].y != 2) return 2;
    if (mesh[0][1].x != 3) return 3;
    if (mesh[0][1].y != 4) return 4;
    if (mesh[1][0].x != 9) return 5;
    if (mesh[1][0].y != 10) return 6;
    if (mesh[3][3].x != 31) return 7;
    if (mesh[3][3].y != 32) return 8;

    // Test field access pattern
    int sum = 0;
    int r, c;
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
            sum += mesh[r][c].x;
            sum += mesh[r][c].y;
        }
    }
    // Sum of 1..32 = 528
    if (sum != 528) return 9;

    return 0;
}
