// Debug test for array access

volatile char *result = (char *)0x4000;

void main() {
    short arr[3] = {100, 200, 300};

    result[0] = arr[0];  // Should be 100 (0x64)
    result[1] = arr[1];  // Should be 200 (0xC8)
    result[2] = arr[2];  // Should be 300 (0x2C)
}
