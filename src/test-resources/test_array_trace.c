// Trace array access step-by-step to find the bug

volatile unsigned char *output = (unsigned char *)0x4000;

int main() {
    // Step 1: Initialize array on frame
    short arr[3] = {100, 200, 300};

    // Step 2: Store arr values at output[0-5] to verify initialization
    unsigned char lo = arr[0] & 0xFF;
    unsigned char hi = (arr[0] >> 8) & 0xFF;
    output[0] = lo;     // Should be 100 (0x64)
    output[1] = hi;     // Should be 0

    lo = arr[1] & 0xFF;
    hi = (arr[1] >> 8) & 0xFF;
    output[2] = lo;     // Should be 200 (0xC8)
    output[3] = hi;     // Should be 0

    lo = arr[2] & 0xFF;
    hi = (arr[2] >> 8) & 0xFF;
    output[4] = lo;     // Should be 44 (0x2C)
    output[5] = hi;     // Should be 1

    // Step 3: Read via index directly
    short val = arr[1];
    output[6] = val & 0xFF;       // Should be 0xC8
    output[7] = (val >> 8) & 0xFF;   // Should be 0

    // Step 4: Test with loop - verify all elements accessible
    int i;
    for (i = 0; i < 3; i++) {
        short v = arr[i];
        output[8 + i*2] = v & 0xFF;
        output[8 + i*2 + 1] = (v >> 8) & 0xFF;
    }

    // Step 5: Mark where we got to
    output[14] = 0xAA;

    return 0;
}
