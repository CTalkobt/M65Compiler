// Test to check if Y register initialization is the bug

volatile unsigned char *output = (unsigned char *)0x4000;

// This function sets Y to a non-zero value before returning
void set_y_nonzero() {
    asm("ldy #5");  // Set Y to 5
}

int main() {
    // Call function that leaves Y non-zero
    set_y_nonzero();

    // Now initialize array - should fail if Y is the bug
    short arr[3] = {100, 200, 300};

    // Output the values
    output[0] = arr[0];  // Should be 100 (0x64)
    output[1] = arr[1];  // Should be 200 (0xC8)
    output[2] = arr[2];  // Should be 44 (0x2C)

    // Also test if we can fix by explicitly setting Y
    short val = arr[1];
    output[3] = val;

    output[4] = 0xAA;
    return 0;
}
