// Test that forces runtime array access using addr_elem/struct_elem

volatile unsigned char *output = (unsigned char *)0x4000;

unsigned char test_array[8] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };

// Use runtime index to force addr_elem generation
int get_array_value(int index) {
    return test_array[index];
}

void main() {
    int i;

    // Write array values to output using runtime indices
    // This should generate addr_elem.16 instructions
    for (i = 0; i < 8; i++) {
        output[i] = get_array_value(i);
    }

    // Mark test complete
    output[8] = 0xAA;
}
