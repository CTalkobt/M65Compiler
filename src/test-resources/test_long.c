// Test: long type (32-bit on this target)
// Validates: long, unsigned long, signed long, function params/returns,
//            sizeof(long), arithmetic, bitwise, comparison, casting, unary ops.

volatile char *r = (char *)0x4000;

long add_longs(long a, long b) {
    return a + b;
}

unsigned long bitwise_or(unsigned long a, unsigned long b) {
    return a | b;
}

long negate_long(long x) {
    return -x;
}

long global_a = 100000L;
long global_b = 200000L;
unsigned long global_c;

void main() {
    long x = 50000L;
    long y = 70000L;
    long sum = add_longs(x, y);
    unsigned long z = bitwise_or(global_a, global_b);
    long neg = negate_long(x);
    long comp = ~global_a;
    long diff = global_b - global_a;

    // sizeof
    r[0] = sizeof(long);           // 4

    // Function result low byte: 50000 + 70000 = 120000 = $1D4C0
    r[1] = (char)(sum & 0xFF);     // $C0

    // Comparison
    if (global_a < global_b)
        r[2] = 1;
    else
        r[2] = 0;

    // Bitwise AND masking
    unsigned long masked = global_a & 0xFFFF;
    r[3] = (char)(masked & 0xFF);  // $A0 (low byte of 100000 = $186A0)

    // Cast int -> long
    int small = 42;
    long big = (long)small;
    r[4] = (char)(big & 0xFF);     // 42 = $2A

    // Cast long -> int (narrowing)
    int narrow = (int)global_a;
    r[5] = (char)(narrow & 0xFF);  // $A0

    // Increment
    global_c = 0xFFFFFFFFL;
    global_c++;
    r[6] = (char)(global_c & 0xFF); // 0 (overflow)

    r[7] = 0xAA;                   // marker
}
