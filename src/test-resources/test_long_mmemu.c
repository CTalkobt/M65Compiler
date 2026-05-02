// mmemu validation test for long type (32-bit)
// Expected bytes at $4000: 04 C0 01 A0 2A A0 00
// Expected bytes at $4007: E0 93 04 00 AA
volatile char *r = (char *)0x4000;

long add_longs(long a, long b) {
    return a + b;
}

long global_a = 100000L;
long global_b = 200000L;
unsigned long global_c;

void main() {
    // sizeof(long) = 4
    r[0] = sizeof(long);

    // 50000+70000=120000=$1D4C0, low byte=$C0
    long sum = add_longs(50000L, 70000L);
    r[1] = (char)sum;

    // comparison: 100000 < 200000
    if (global_a < global_b)
        r[2] = 1;
    else
        r[2] = 0;

    // low byte of 100000 ($186A0) = $A0
    r[3] = (char)global_a;

    // cast int -> long -> truncate: 42
    int small = 42;
    long big = (long)small;
    r[4] = (char)big;

    // cast long -> int (narrowing), low byte of $186A0 = $A0
    int narrow = (int)global_a;
    r[5] = (char)narrow;

    // overflow test: 0xFFFFFFFF + 1 = 0
    global_c = 0xFFFFFFFFL;
    global_c++;
    r[6] = (char)global_c;

    // global = global_a + global_b: 100000 + 200000 = 300000 = $493E0
    // verify all 4 bytes
    global_c = global_a + global_b;
    r[7] = (char)global_c;          // $E0
    r[8] = (char)(global_c >> 8);   // $93
    r[9] = (char)(global_c >> 16);  // $04
    r[10] = (char)(global_c >> 24); // $00

    r[11] = 0xAA;  // marker
}
