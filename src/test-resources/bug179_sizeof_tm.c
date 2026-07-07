// Test: Check sizeof(struct tm) in both compilation contexts

#include <time.h>
#include <stdio.h>

volatile char *result_addr = (char *)0x4000;

// This function will be called from library code
void check_tm_size_via_lib(int sz) {
    result_addr[0] = (sz >> 0) & 0xFF;
    result_addr[1] = (sz >> 8) & 0xFF;
}

void main() {
    // Direct check in user code
    int user_size = sizeof(struct tm);
    result_addr[0] = (user_size >> 0) & 0xFF;
    result_addr[1] = (user_size >> 8) & 0xFF;

    // Marker
    result_addr[2] = 0xAA;
}
