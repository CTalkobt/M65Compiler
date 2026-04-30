// Test runtime-indexed global array stores via loops
// Results written to $4000+ for mmemu validation

volatile char *result = (char *)0x4000;

char scores[5] = 0;
int grid[3][4] = 0;

void main() {
    int i;
    int j;

    // Fill 1D array with loop
    for (i = 0; i < 5; i++) {
        scores[i] = i + 1;
    }

    // Fill 2D array with nested loop: grid[r][c] = r * 10 + c
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 4; j++) {
            grid[i][j] = i * 10 + j;
        }
    }

    // Test 1: scores[0] = 1
    result[0] = scores[0];

    // Test 2: scores[4] = 5
    result[1] = scores[4];

    // Test 3: grid[0][0] = 0
    result[2] = grid[0][0];

    // Test 4: grid[1][2] = 12 = $0C
    result[3] = grid[1][2];

    // Test 5: grid[2][3] = 23 = $17
    result[4] = grid[2][3];

    // Test 6: marker
    result[5] = 0xAA;
}
