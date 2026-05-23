// Integration Test 2: Multi-object cross-calls (file A - caller)
// Expected: $4000 = $1E $07 $AA
extern int add(int a, int b);
extern int sub(int a, int b);

volatile char *r = (char *)0x4000;

int main() {
    r[0] = (char)add(10, 20);   // 30 = $1E
    r[1] = (char)sub(20, 13);   // 7 = $07
    r[2] = 0xAA;
    __asm__("brk");
    return 0;
}
