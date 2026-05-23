// Integration Test 5: 32-bit globals across link
// Tests 4-byte DATA relocations through linker.
// Expected: $4000 = $E0 $01 $AA
long ga = 100000L;
long gb = 200000L;
volatile char *r = (char *)0x4000;

int main() {
    long sum = ga + gb;          // 300000 = $493E0
    r[0] = (char)sum;            // $E0
    if (ga < gb) r[1] = 1;
    else r[1] = 0;
    r[2] = 0xAA;
    __asm__("brk");
    return 0;
}
