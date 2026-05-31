// Integration test: Hello World on MEGA65 with ZP save/restore verification

#include <stdio.h>

// Test that ZP registers are preserved across calls
volatile unsigned char *zp_test_addr = (volatile unsigned char *)0x00FB;

int verify_zp_preservation() {
    // Set a value in ZP
    *zp_test_addr = 0xAB;

    // Call printf (which may use ZP internally)
    printf("Hello World\n");

    // Verify ZP value is still intact
    if (*zp_test_addr != 0xAB) {
        printf("ERROR: ZP register was clobbered!\n");
        return 1;
    }

    return 0;
}

int main() {
    // Test ZP preservation
    int status = verify_zp_preservation();

    if (status == 0) {
        printf("SUCCESS: ZP preservation verified\n");
    }

    // Return status (0 = success, non-zero = failure)
    return status;
}
