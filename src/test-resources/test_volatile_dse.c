// Test: Volatile variables are not eliminated by dead store elimination
// Validates that volatile prevents both variable elimination and DSE.

volatile int hw_reg;

int main() {
    // Volatile locals must not be eliminated
    volatile int counter = 0;
    counter = 1;
    counter = 2;
    counter = 3;
    // All stores must be emitted even though only the last is "live"

    // Non-volatile can be optimized
    int temp = 0;
    temp = 100;
    // temp = 0 may be eliminated

    // Mix: volatile write + read
    volatile int status = 0;
    status = 42;
    int val = status;
    if (val != 42) return 1;

    return 0;
}
