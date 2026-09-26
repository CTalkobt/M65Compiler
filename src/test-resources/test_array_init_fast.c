/* Fast Array Initialization Test with xemu Exit Signal
 * 
 * Optimized test that signals xemu to exit after completion
 * Using -prgexit flag, the test can complete in seconds instead of timeout
 * 
 * Exit mechanism: Print "READY." to trigger xemu -prgexit shutdown
 */

#include <stdio.h>
#include <mega65.h>

/* Memory marker location */
#define TEST_MARKER_ADDR ((volatile unsigned char *)0x4000)

/* Global arrays */
unsigned char g_char[5] = {0x10, 0x20, 0x30, 0x40, 0x50};
unsigned int g_int[3] = {1000, 2000, 3000};
unsigned char g_zero[4] = {};

void print_results(void) {
    printf("\n");
    printf("Array Init Test Results:\n");
    printf("------------------------\n");
    printf("char[5]: [%02X %02X %02X %02X %02X]\n",
           g_char[0], g_char[1], g_char[2], g_char[3], g_char[4]);
    printf("int[3]:  [%04X %04X %04X]\n",
           g_int[0], g_int[1], g_int[2]);
    printf("zero[4]: [%02X %02X %02X %02X]\n",
           g_zero[0], g_zero[1], g_zero[2], g_zero[3]);
    printf("------------------------\n");
}

void verify_arrays(void) {
    unsigned char pass = 1;
    
    printf("Verification:\n");
    
    if (g_char[0] == 0x10 && g_char[4] == 0x50) {
        printf("  [OK] char array\n");
    } else {
        printf("  [FAIL] char array\n");
        pass = 0;
    }
    
    if (g_int[0] == 1000 && g_int[2] == 3000) {
        printf("  [OK] int array\n");
    } else {
        printf("  [FAIL] int array\n");
        pass = 0;
    }
    
    if (g_zero[0] == 0 && g_zero[3] == 0) {
        printf("  [OK] zero array\n");
    } else {
        printf("  [FAIL] zero array\n");
        pass = 0;
    }
    
    if (pass) {
        printf("\nResult: PASS\n");
    } else {
        printf("\nResult: FAIL\n");
    }
}

void main(void) {
    /* Write execution marker */
    TEST_MARKER_ADDR[0] = 0xA5;
    TEST_MARKER_ADDR[1] = 0x54;  /* 'T' */
    TEST_MARKER_ADDR[2] = 0x45;  /* 'E' */
    TEST_MARKER_ADDR[3] = 0xFF;
    
    /* Quick test output */
    printf("\nMEGA65 Array Test (Fast)\n");
    print_results();
    verify_arrays();
    
    /* Update completion marker */
    TEST_MARKER_ADDR[1] = 0x44;  /* 'D' */
    TEST_MARKER_ADDR[2] = 0x4F;  /* 'O' */
    
    /* Signal xemu to exit via -prgexit flag
     * This prints "READY." which triggers xemu shutdown when -prgexit is used
     * Much faster than waiting for timeout!
     */
    printf("\nREADY.\n");
}
