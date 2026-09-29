// Long type (32-bit) validation test
// NOTE: global_a/global_b are initialized globals — may fail until #262 is fixed.
//
// Expected at $C010: 04 C0 01 A0 2A A0 00 E0 93 04 00 AA

#define RESULT(i) (*(volatile unsigned char *)(0xC010 + (i)))

long add_longs(long a, long b) {
    return a + b;
}

long global_a = 100000L;
long global_b = 200000L;
unsigned long global_c;

void main() {
    // sizeof(long) = 4
    RESULT(0) = sizeof(long);

    // 50000+70000=120000=$1D4C0, low byte=$C0
    long sum = add_longs(50000L, 70000L);
    RESULT(1) = (char)sum;

    // comparison: 100000 < 200000
    if (global_a < global_b)
        RESULT(2) = 1;
    else
        RESULT(2) = 0;

    // low byte of 100000 ($186A0) = $A0
    RESULT(3) = (char)global_a;

    // cast int -> long -> truncate: 42
    int small = 42;
    long big = (long)small;
    RESULT(4) = (char)big;

    // cast long -> int (narrowing), low byte of $186A0 = $A0
    int narrow = (int)global_a;
    RESULT(5) = (char)narrow;

    // overflow test: 0xFFFFFFFF + 1 = 0
    global_c = 0xFFFFFFFFL;
    global_c++;
    RESULT(6) = (char)global_c;

    // global = global_a + global_b: 100000 + 200000 = 300000 = $493E0
    // verify all 4 bytes
    global_c = global_a + global_b;
    RESULT(7) = (char)global_c;          // $E0
    RESULT(8) = (char)(global_c >> 8);   // $93
    RESULT(9) = (char)(global_c >> 16);  // $04
    RESULT(10) = (char)(global_c >> 24); // $00

    RESULT(11) = 0xAA;  // marker
}
