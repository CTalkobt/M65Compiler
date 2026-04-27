// Test: Dead store elimination and constant propagation
// Validates that sequential constant assignments are folded correctly
// and the final value is preserved in subsequent uses.

int main() {
    int results[4];

    // Sequential constant propagation — intermediate stores are dead
    int sum = 0;
    sum = sum + 1;   // sum = 1 (dead)
    sum = sum + 2;   // sum = 3 (dead)
    sum = sum + 3;   // sum = 6 (dead)
    sum = sum + 4;   // sum = 10
    results[0] = sum;

    // Variable used after folding
    int x = 5;
    x = x * 2;       // x = 10
    results[1] = x;

    // Interleaved variables
    int a = 1;
    int b = 2;
    a = a + b;  // a = 3
    b = b + a;  // b = 5
    results[2] = a;
    results[3] = b;

    if (results[0] != 10) return 1;
    if (results[1] != 10) return 2;
    if (results[2] != 3) return 3;
    if (results[3] != 5) return 4;
    return 0;
}
