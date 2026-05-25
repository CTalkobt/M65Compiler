// Test repeat() compile-time loop unrolling

int main() {
    unsigned char buf[8];
    int i;

    // Clear buffer
    for (i = 0; i < 8; i++) buf[i] = 0;

    // Test 1: Simple repeat — fill 4 elements
    unsigned char *p = buf;
    repeat(4) {
        *p = 42;
        p++;
    }
    if (buf[0] != 42) return 1;
    if (buf[1] != 42) return 2;
    if (buf[2] != 42) return 3;
    if (buf[3] != 42) return 4;
    if (buf[4] != 0) return 5;  // not touched

    // Test 2: Repeat with variable — buf[x-1] = x for x=1..4
    repeat(unsigned char x, 4) {
        buf[x - 1] = x;
    }
    if (buf[0] != 1) return 6;
    if (buf[1] != 2) return 7;
    if (buf[2] != 3) return 8;
    if (buf[3] != 4) return 9;

    // Test 3: Repeat with variable, int type
    repeat(int y, 3) {
        buf[y - 1] = y * 10;
    }
    if (buf[0] != 10) return 10;
    if (buf[1] != 20) return 11;
    if (buf[2] != 30) return 12;

    // Test 4: repeat(0) = no-op
    repeat(0) {
        return 99;
    }

    // Test 5: repeat(1) = single execution
    buf[7] = 0;
    repeat(1) {
        buf[7] = 77;
    }
    if (buf[7] != 77) return 13;

    return 0;
}
