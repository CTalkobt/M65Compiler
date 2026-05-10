// Test: Parameter passing edge cases across conventions

char *results = 0x4000;

// ZP: max parameters before exhaustion (8 bytes = 4 ints)
__fastcall__ int zp_four_params(int a, int b, int c, int d) {
    return a + b + c + d;
}

// Stack: unlimited parameters
int stack_five_params(int a, int b, int c, int d, int e) {
    return a + b + c + d + e;
}

// Stack caller invoking both
void main() {
    // Test 1: ZP with 4 params (maximum for default 8-byte region)
    int r1 = zp_four_params(1, 2, 3, 4);
    results[0] = (char)r1;  // 1 + 2 + 3 + 4 = 10

    // Test 2: Stack with 5 params
    int r2 = stack_five_params(10, 20, 30, 40, 50);
    results[1] = (char)(r2 & 0xFF);  // 10 + 20 + 30 + 40 + 50 = 150

    // Test 3: ZP with mixed types (char + int)
    // Note: chars are promoted to int in C, so they take 2 bytes
    __fastcall__ int zp_mixed(char a, int b) {
        return a + b;
    }
    int r3 = zp_mixed(5, 100);
    results[2] = (char)(r3 & 0xFF);  // 5 + 100 = 105

    // Test 4: Stack caller → ZP with 4 params
    int r4 = zp_four_params(11, 22, 33, 44);
    results[3] = (char)(r4 & 0xFF);  // 11 + 22 + 33 + 44 = 110

    // Sentinel
    results[4] = 0xFF;

    __asm__("brk");
}
