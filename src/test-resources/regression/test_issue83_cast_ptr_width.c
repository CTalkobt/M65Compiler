// test_issue83_cast_ptr_width: cast-pointer dereference store width
// Issue #83: *(volatile unsigned char *)ADDR = val was generating 16-bit
// stores instead of 8-bit, clobbering the adjacent byte.
// Returns 0 (A=$00) on success, non-zero on failure.

int main() {
    unsigned char buf[8];
    unsigned char *p;
    int i;

    // Initialize buffer to known pattern
    for (i = 0; i < 8; i++) {
        buf[i] = 0xAA;
    }

    // Cast-pointer char store: must only write 1 byte
    p = &buf[2];
    *(unsigned char *)p = 0x55;
    if (buf[2] != 0x55) return 1;  // target byte changed
    if (buf[3] != 0xAA) return 2;  // adjacent byte must be untouched

    // Cast-pointer int store: must write 2 bytes
    p = &buf[4];
    *(unsigned int *)p = 0x1234;
    if (buf[4] != 0x34) return 3;  // low byte
    if (buf[5] != 0x12) return 4;  // high byte
    if (buf[6] != 0xAA) return 5;  // adjacent must be untouched

    // Volatile cast-pointer char store
    p = &buf[0];
    *(volatile unsigned char *)p = 0x77;
    if (buf[0] != 0x77) return 6;  // target byte
    if (buf[1] != 0xAA) return 7;  // adjacent untouched

    // Direct integer literal cast-pointer store (the original bug pattern)
    buf[0] = 0xAA;
    buf[1] = 0xBB;
    *(unsigned char *)&buf[0] = 0xCC;
    if (buf[0] != 0xCC) return 8;
    if (buf[1] != 0xBB) return 9;  // must NOT be clobbered

    return 0;
}
