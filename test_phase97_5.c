// Phase 97.5: .zp-Suffixed Instruction Optimization Test
// Tests that __zp global variables get optimized instructions

__zp int zp_counter = 42;
__zp unsigned char zp_flag = 1;
int normal_value = 100;

void test_zp_read(void) {
    int x = zp_counter;
    unsigned char f = zp_flag;
}

void test_zp_write(void) {
    zp_counter = 99;
    zp_flag = 0;
}

void test_mixed(void) {
    int a = normal_value;    // Load from normal address
    int b = zp_counter;      // Load from __zp (should use lda.zp)

    zp_counter = 123;        // Store to __zp (should use sta.zp)
    normal_value = 456;      // Store to normal address
}

int main(void) {
    test_zp_read();
    test_zp_write();
    test_mixed();
    return zp_counter;
}
