// Test: Per-function clobber tracking and selective invalidation
// Validates that the IR pipeline emits accurate .reg_clobbers directives
// and the assembler optimizer uses them at JSR call sites.
// Expected: program assembles correctly; binary is well-formed.

volatile int g_val;
volatile char g_flag;

// Leaf: only writes to global (clobbers A, X — not Y, Z)
void set_value(int v) {
    g_val = v;
}

// Leaf: only writes a char (clobbers A — not X, Y, Z)
void set_flag(char f) {
    g_flag = f;
}

// Leaf: empty body (clobbers nothing)
void noop(void) {
}

// Non-leaf: calls set_value (clobbers A, X, Y, Z from call)
void caller(void) {
    set_value(42);
    set_flag(1);
    noop();
}

// Return value test: leaf returning value (clobbers A, X)
int get_value(void) {
    return g_val;
}

// Arithmetic leaf: uses add (clobbers A, X, flags C, N, Z, V)
int add(int a, int b) {
    return a + b;
}

void main() {
    set_value(100);
    set_flag(1);
    noop();
    caller();
    int r = get_value();
    int s = add(3, 4);
    g_val = r + s;
}
