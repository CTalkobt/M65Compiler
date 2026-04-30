// Test multi-dimensional array support
// Results written to $4000+ for mmemu validation

volatile char *result = (char *)0x4000;

int grid[3][4] = 0;
char scores[5] = 0;

void main() {
    // Fill 1D array with constant indices
    scores[0] = 1;
    scores[1] = 2;
    scores[2] = 3;
    scores[3] = 4;
    scores[4] = 5;

    // Fill 2D array with constant indices
    grid[0][0] = 0;
    grid[0][1] = 1;
    grid[1][0] = 10;
    grid[1][2] = 12;
    grid[2][3] = 23;

    // Test 1: 1D array read (scores[2] = 3)
    result[0] = scores[2];

    // Test 2: 2D read (grid[1][2] = 12 = $0C)
    result[1] = grid[1][2];

    // Test 3: 2D read (grid[2][3] = 23 = $17)
    result[2] = grid[2][3];

    // Test 4: 2D first element (grid[0][0] = 0)
    result[3] = grid[0][0];

    // Test 5: sizeof 2D array = 3*4*2 = 24 = $18
    result[4] = sizeof(grid);

    // Test 6: Success marker
    result[5] = 0xAA;
}
