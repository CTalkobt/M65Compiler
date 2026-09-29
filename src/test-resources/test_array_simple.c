// Simple array access test
volatile char *result = (char *)0x4000;

int arr[3] = {100, 200, 300};

void main() {
    result[0] = arr[0];     // Should be 100 (0x64)
    result[1] = arr[1];     // Should be 200 (0xC8)
    result[2] = arr[2];     // Should be 300 (0x2C)
    result[3] = 0xFF;       // Marker
}
