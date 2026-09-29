/* Fast Array Initialization Test with UART Serial Output
 *
 * Outputs test results via UART $D0E3 for xemu -serialtcp capture
 * Uses direct UART writes for serial communication
 *
 * Result transmission: Binary data over UART for fast validation
 */

#include <mega65.h>

/* MEGA65 Serial TCP UART transmit register (M65 I/O mode) */
#define UART_DATA 0xD0E3

void uart_putchar(unsigned char c) {
    volatile unsigned char *uart = (unsigned char *)UART_DATA;
    *uart = c;
}

void uart_puts(const char *str) {
    while (*str) {
        uart_putchar(*str++);
    }
}

/* Memory marker location */
#define TEST_MARKER_ADDR ((volatile unsigned char *)0x4000)

/* Global arrays */
unsigned char g_char[5] = {0x10, 0x20, 0x30, 0x40, 0x50};
unsigned int g_int[3] = {1000, 2000, 3000};
unsigned char g_zero[4] = {};

void print_hex_byte(unsigned char val) {
    char hex[3];
    hex[0] = "0123456789ABCDEF"[(val >> 4) & 0xF];
    hex[1] = "0123456789ABCDEF"[val & 0xF];
    hex[2] = 0;
    uart_puts(hex);
}

void print_hex_word(unsigned int val) {
    print_hex_byte((val >> 8) & 0xFF);
    print_hex_byte(val & 0xFF);
}

void print_results(void) {
    uart_puts("\n");
    uart_puts("Array Init Test Results:\n");
    uart_puts("------------------------\n");
    uart_puts("char[5]: [");
    for (int i = 0; i < 5; i++) {
        print_hex_byte(g_char[i]);
        if (i < 4) uart_puts(" ");
    }
    uart_puts("]\n");

    uart_puts("int[3]:  [");
    for (int i = 0; i < 3; i++) {
        print_hex_word(g_int[i]);
        if (i < 2) uart_puts(" ");
    }
    uart_puts("]\n");

    uart_puts("zero[4]: [");
    for (int i = 0; i < 4; i++) {
        print_hex_byte(g_zero[i]);
        if (i < 3) uart_puts(" ");
    }
    uart_puts("]\n");
    uart_puts("------------------------\n");
}

void verify_arrays(void) {
    unsigned char pass = 1;

    uart_puts("Verification:\n");

    if (g_char[0] == 0x10 && g_char[4] == 0x50) {
        uart_puts("  [OK] char array\n");
    } else {
        uart_puts("  [FAIL] char array\n");
        pass = 0;
    }

    if (g_int[0] == 1000 && g_int[2] == 3000) {
        uart_puts("  [OK] int array\n");
    } else {
        uart_puts("  [FAIL] int array\n");
        pass = 0;
    }

    if (g_zero[0] == 0 && g_zero[3] == 0) {
        uart_puts("  [OK] zero array\n");
    } else {
        uart_puts("  [FAIL] zero array\n");
        pass = 0;
    }

    if (pass) {
        uart_puts("\nResult: PASS\n");
    } else {
        uart_puts("\nResult: FAIL\n");
    }
}

void main(void) {
    /* Write execution marker */
    TEST_MARKER_ADDR[0] = 0xA5;
    TEST_MARKER_ADDR[1] = 0x54;  /* 'T' */
    TEST_MARKER_ADDR[2] = 0x45;  /* 'E' */
    TEST_MARKER_ADDR[3] = 0xFF;
    
    /* Quick test output */
    uart_puts("\nMEGA65 Array Test (Fast)\n");
    print_results();
    verify_arrays();

    /* Update completion marker */
    TEST_MARKER_ADDR[1] = 0x44;  /* 'D' */
    TEST_MARKER_ADDR[2] = 0x4F;  /* 'O' */

    /* Signal completion via UART */
    uart_puts("\nRESULT: TESTS COMPLETE\n");
}
