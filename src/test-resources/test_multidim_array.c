// Test multi-dimensional array support
// Results written to $4000+ for mmemu validation
// Uses constant indices (runtime index stores have a known SP offset bug)

volatile char *result = (char *)0x4000;

// Pre-initialized globals
int grid[3][4] = 0;
char scores[5] = 0;

void fill_scores() {
    // Use direct constant-index stores to avoid the runtime index SP bug
    scores[0] = 1;
    scores[1] = 2;
    scores[2] = 3;
    scores[3] = 4;
    scores[4] = 5;
}

void fill_grid() {
    // Row 0
    grid[0][0] = 0;
    grid[0][1] = 1;
    grid[0][2] = 2;
    grid[0][3] = 3;
    // Row 1
    grid[1][0] = 10;
    grid[1][1] = 11;
    grid[1][2] = 12;
    grid[1][3] = 13;
    // Row 2
    grid[2][0] = 20;
    grid[2][1] = 21;
    grid[2][2] = 22;
    grid[2][3] = 23;
}

void main() {
    fill_scores();
    fill_grid();

    // Test 1: 1D array read (scores[2] = 3)
    result[0] = scores[2];

    // Test 2: 2D constant index read (grid[1][2] = 12 = $0C)
    result[1] = grid[1][2];

    // Test 3: 2D last element (grid[2][3] = 23 = $17)
    result[2] = grid[2][3];

    // Test 4: 2D first element (grid[0][0] = 0)
    result[3] = grid[0][0];

    // Test 5: sizeof 2D array = 3*4*2 = 24 = $18
    result[4] = sizeof(grid);

    // Test 6: Success marker
    result[5] = 0xAA;
}
