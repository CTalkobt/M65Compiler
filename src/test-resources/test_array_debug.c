// Debug version of array test - traces intermediate values

volatile unsigned char *output = (unsigned char *)0x4000;

int main() {
    // Local array - frame-allocated
    short arr[3] = {100, 200, 300};

    // Output initialization values first
    output[0] = arr[0];  // Should be 100 (0x64)
    output[1] = arr[0] >> 8;
    output[2] = arr[1];  // Should be 200 (0xC8)
    output[3] = arr[1] >> 8;
    output[4] = arr[2];  // Should be 44 (0x2C)
    output[5] = arr[2] >> 8;

    // Now test direct indexing vs function call
    short val1 = arr[1];  // Direct access
    output[6] = val1;
    output[7] = val1 >> 8;

    // Test with loop access
    int i;
    for (i = 0; i < 3; i++) {
        output[8 + i * 2] = arr[i];
        output[8 + i * 2 + 1] = arr[i] >> 8;
    }

    // Mark end
    output[0x1F] = 0xAA;

    return 0;
}
