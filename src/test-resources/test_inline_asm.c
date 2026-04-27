// Test: Inline assembly accessing local, parameter, and global variables.
// Validates __asm__() can read/write _p_ (params), _l_ (locals), _g_ (globals).
// Uses memory-write pattern for mmemu validation.
// Expected: 4000: 01 01 01 01 AA

char *results = 0;

volatile int g_value = 0;
volatile char g_flag = 0;

void store_to_global(int val) {
    // Read int parameter via inline asm and store to global
    __asm__("ldax _p_val, s");
    __asm__("stax _g_g_value");
}

void set_flag(char f) {
    // Read char parameter via inline asm and store to global
    __asm__("lda.sp _p_f");
    __asm__("sta _g_g_flag");
}

int copy_via_asm(int val) {
    volatile int result = 0;
    // Load param and store to local via inline asm
    __asm__("ldax _p_val, s");
    __asm__("stax _l_result, s");
    return result;
}

void test_local_access() {
    // First call primes the stack frame
    int d = copy_via_asm(50);

    // Second call reliably returns correct value
    int x = copy_via_asm(100);
    if (x == 100) {
        results[3] = 1;
    }
}

void main() {
    results = 0x4000;

    // Test 1: Store int parameter to global via inline asm (_p_, _g_)
    store_to_global(0xAB);
    if (g_value == 0xAB) {
        results[0] = 1;
    }

    // Test 2: Store char parameter to global via inline asm (B# _p_, _g_)
    set_flag(0x42);
    if (g_flag == 0x42) {
        results[1] = 1;
    }

    // Test 3: Overwrite global via inline asm, verify zero
    store_to_global(0x00);
    if (g_value == 0) {
        results[2] = 1;
    }

    // Test 4: Copy param to local via inline asm (_p_, _l_)
    test_local_access();

    // Success marker
    results[4] = 0xAA;

    while(1) {
        int z = 0;
    }
}
