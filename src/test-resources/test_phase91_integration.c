/**
 * Phase 91 Integration Test
 *
 * Demonstrates end-to-end flow:
 * 1. Compile multi-module program with -fipo flag (future)
 * 2. GlobalFunctionDatabase collects profiles
 * 3. IPOAnalyzer produces optimization decisions
 * 4. Code generation applies specializations/inlining
 */

// === Module 1: Utility Functions ===

// Leaf function: no locals, no calls -> candidate for aggressive optimization
int clamp(int val, int min, int max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

// Single-caller with constants -> inlining candidate
int get_magic_number(void) {
    return 0xDEADBEEF;
}

// Function with constant-only calls -> specialization candidate
int process_event(int type) {
    if (type == 1) return 100;
    if (type == 2) return 200;
    if (type == 3) return 300;
    return 0;
}

// === Module 2: Business Logic ===

// Calls get_magic_number once -> should inline
int initialize_system(void) {
    int magic = get_magic_number();
    return magic & 0xFF;
}

// Calls process_event with constants -> should specialize
int main(void) {
    int v1 = clamp(50, 0, 100);           // Constant call
    int v2 = process_event(1);            // Specialization candidate
    int v3 = process_event(2);            // Same pattern
    int v4 = initialize_system();         // Should inline
    return v1 + v2 + v3 + v4;
}

// === Unused Functions (Dead Code) ===

// Never called -> should be eliminated
int unused_debug_func(int x) {
    return x * 1000;
}

// Never called -> should be eliminated
void print_debug_info(void) {
    // Would print something
}
