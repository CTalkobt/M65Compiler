// Integration Test 6: BSS zeroing through linker
// Uninitialized globals should be zero after link.
// Expected: $4000 = $00 $00 $2A $AA
int bss_var1;
int bss_var2;
int init_var = 42;
volatile char *r = (char *)0x4000;

int main() {
    r[0] = (char)bss_var1;      // 0
    r[1] = (char)bss_var2;      // 0
    r[2] = (char)init_var;      // 42 = $2A
    r[3] = 0xAA;
    return 0;
}
