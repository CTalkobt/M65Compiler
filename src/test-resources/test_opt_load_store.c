// Test for granular load/store and self-assignment optimizations.
// This test is designed to verify that the generated code is correct and returns 0.
// We use volatile to ensure some operations are not optimized away during runtime checks.

volatile int v_x = 42;
int g_x = 100;
int g_y = 200;

int get_val() { return 0x1234; }

int test_self_assignment() {
    int x = 42;
    x = x; // Should be optimized out
    if (x != 42) return 1;
    
    g_x = g_x; // Should be optimized out
    if (g_x != 100) return 2;
    
    v_x = v_x; // Should NOT be optimized out (volatile)
    if (v_x != 42) return 3;
    
    return 0;
}

int test_granular_store() {
    // These have the same low byte (0x34)
    g_x = 0x1134;
    g_y = 0x2234; // Should only store high byte if optimization works
    
    if (g_x != 0x1134) return 4;
    if (g_y != 0x2234) return 5;
    
    return 0;
}

int test_granular_load() {
    int a;
    int b;
    g_x = 0x4455;
    a = g_x; // AX = 0x4455
    b = g_x; // Should be redundant load
    
    if (a != 0x4455) return 6;
    if (b != 0x4455) return 7;
    
    return 0;
}

int main() {
    if (test_self_assignment() != 0) return 1;
    if (test_granular_store() != 0) return 2;
    if (test_granular_load() != 0) return 3;
    
    return 0;
}
