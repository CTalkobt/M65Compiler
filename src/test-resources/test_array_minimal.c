// Minimal test case for array element access bug
// Tests: Local array with runtime index access
// Expected: arr[1] = 200
// Bug: Returns 0x00 instead

volatile unsigned char *output = (unsigned char *)0x4000;

int main() {
    // Local array - frame-allocated
    short arr[3] = {100, 200, 300};

    // Return arr[1] via output buffer
    // If working correctly: output[0] = 200 (0xC8), output[1] = 0 (high byte)
    // If buggy: output[0] = 0x00, output[1] = 0x00
    output[0] = arr[1];  // Low byte
    output[1] = arr[1] >> 8;  // High byte

    // Also test arr[0] and arr[2] for comparison
    output[2] = arr[0];  // Should be 100 (0x64)
    output[3] = arr[2];  // Should be 44 (300 = 0x2C, low byte)

    // Mark test complete
    output[4] = 0xAA;

    return 0;
}
