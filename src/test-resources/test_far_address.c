// Test: Far address support (__far keyword for 24-bit addressing)
// Demonstrates Phase 98 infrastructure for MEGA65 extended memory

#include <stdio.h>

// Far memory buffer (beyond standard 64K, in extended bank)
__far unsigned char far_buffer[256];

// Far pointer for data structures
__far int* far_data_ptr;

// Far variable
__far long far_counter = 0;

// Access far memory
void test_far_store(void) {
    // Store to far memory
    far_buffer[0] = 42;
    far_buffer[1] = 100;
    far_counter = 12345;
}

// Load from far memory
unsigned char test_far_load(int index) {
    // Load from far memory
    if (index < 256) {
        return far_buffer[index];
    }
    return 0;
}

// Far pointer dereference
int test_far_pointer(void) {
    // If far_data_ptr is initialized to far memory
    if (far_data_ptr != 0) {
        return *far_data_ptr;
    }
    return -1;
}

int main(void) {
    printf("Far address test\n");

    test_far_store();
    unsigned char val = test_far_load(0);
    printf("Far buffer[0] = %d\n", val);

    return 0;
}
