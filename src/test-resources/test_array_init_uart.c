/* Test: Array Initialization with UART Serial Output
 *
 * Tests array initialization and transmits results via UART serial interface.
 * Output can be captured via xemu-xmega65 -serialtcp 127.0.0.1:PORT
 *
 * This test demonstrates proper array initialization on MEGA65:
 * - Global arrays with explicit initialization
 * - Global arrays with zero-fill
 * - Local (stack) arrays
 * - Partial initialization with zero-fill
 *
 * Uses direct UART hardware writes to $D600 for -serialtcp output capture.
 * Also writes memory markers for emulator-based validation.
 */

#include <mega65.h>

/* Memory marker location (for emulator inspection) */
#define TEST_MARKER_ADDR ((volatile unsigned char *)0x4000)

/* MEGA65 Serial TCP UART transmit register (M65 I/O mode) */
#define UART_DATA 0xD0E3

/* Direct UART write for -serialtcp capture */
void uart_putchar(unsigned char c) {
    volatile unsigned char *uart = (unsigned char *)UART_DATA;
    *uart = c;
}

void uart_puts(const char *str) {
    while (*str) {
        uart_putchar(*str++);
    }
}

void uart_puthex_byte(unsigned char val) {
    char hex[3];
    hex[0] = "0123456789ABCDEF"[(val >> 4) & 0xF];
    hex[1] = "0123456789ABCDEF"[val & 0xF];
    hex[2] = 0;
    uart_puts(hex);
}

void uart_puthex_word(unsigned int val) {
    uart_puthex_byte((val >> 8) & 0xFF);
    uart_puthex_byte(val & 0xFF);
}

/* Global arrays with initializer lists */
unsigned char g_char_init[5] = {0x10, 0x20, 0x30, 0x40, 0x50};
unsigned int g_int_init[4] = {1000, 2000, 3000, 4000};

/* Global arrays with partial/zero initialization */
unsigned char g_char_partial[6] = {0xAA, 0xBB};  /* Last 4 are zero-filled */
unsigned int g_int_zero[3] = {};                 /* All zeros */

void print_test_header(void) {
    uart_puts("\n");
    uart_puts("=========================================\n");
    uart_puts("MEGA65 Array Initialization Test\n");
    uart_puts("=========================================\n");
    uart_puts("\n");
    uart_puts("Testing array initialization on MEGA65\n");
    uart_puts("Output: Direct UART via -serialtcp\n");
    uart_puts("\n");
}

void test_char_arrays(void) {
    unsigned char i;

    uart_puts("TEST: Global Char Array (Explicit Init)\n");
    uart_puts("  Expected: [10 20 30 40 50]\n");
    uart_puts("  Actual:   [");
    for (i = 0; i < 5; i++) {
        uart_puthex_byte(g_char_init[i]);
        if (i < 4) uart_puts(" ");
    }
    uart_puts("]\n");

    uart_puts("TEST: Global Char Array (Partial Init)\n");
    uart_puts("  Expected: [AA BB 00 00 00 00]\n");
    uart_puts("  Actual:   [");
    for (i = 0; i < 6; i++) {
        uart_puthex_byte(g_char_partial[i]);
        if (i < 5) uart_puts(" ");
    }
    uart_puts("]\n");
}

void test_int_arrays(void) {
    unsigned char i;

    uart_puts("\nTEST: Global Int Array (Explicit Init)\n");
    uart_puts("  Expected: [03E8 07D0 0BB8 0FA0]\n");
    uart_puts("  Actual:   [");
    for (i = 0; i < 4; i++) {
        uart_puthex_word(g_int_init[i]);
        if (i < 3) uart_puts(" ");
    }
    uart_puts("]\n");

    uart_puts("TEST: Global Int Array (Zero Init)\n");
    uart_puts("  Expected: [0000 0000 0000]\n");
    uart_puts("  Actual:   [");
    for (i = 0; i < 3; i++) {
        uart_puthex_word(g_int_zero[i]);
        if (i < 2) uart_puts(" ");
    }
    uart_puts("]\n");
}

void test_local_arrays(void) {
    unsigned char local_char[4] = {0xCC, 0xDD, 0xEE, 0xFF};
    unsigned int local_int[3] = {10000, 20000, 30000};
    unsigned char i;

    uart_puts("\nTEST: Local Char Array (Stack Init)\n");
    uart_puts("  Expected: [CC DD EE FF]\n");
    uart_puts("  Actual:   [");
    for (i = 0; i < 4; i++) {
        uart_puthex_byte(local_char[i]);
        if (i < 3) uart_puts(" ");
    }
    uart_puts("]\n");

    uart_puts("TEST: Local Int Array (Stack Init)\n");
    uart_puts("  Expected: [2710 4E20 7530]\n");
    uart_puts("  Actual:   [");
    for (i = 0; i < 3; i++) {
        uart_puthex_word(local_int[i]);
        if (i < 2) uart_puts(" ");
    }
    uart_puts("]\n");
}

void verify_arrays(void) {
    unsigned char all_pass = 1;

    uart_puts("\n");
    uart_puts("VERIFICATION:\n");

    /* Verify char init */
    if (g_char_init[0] == 0x10 && g_char_init[4] == 0x50) {
        uart_puts("  [PASS] Global char init array\n");
    } else {
        uart_puts("  [FAIL] Global char init array\n");
        all_pass = 0;
    }

    /* Verify char partial */
    if (g_char_partial[0] == 0xAA && g_char_partial[1] == 0xBB &&
        g_char_partial[2] == 0x00 && g_char_partial[5] == 0x00) {
        uart_puts("  [PASS] Global char partial init array\n");
    } else {
        uart_puts("  [FAIL] Global char partial init array\n");
        all_pass = 0;
    }

    /* Verify int init */
    if (g_int_init[0] == 1000 && g_int_init[3] == 4000) {
        uart_puts("  [PASS] Global int init array\n");
    } else {
        uart_puts("  [FAIL] Global int init array\n");
        all_pass = 0;
    }

    /* Verify int zero */
    if (g_int_zero[0] == 0 && g_int_zero[1] == 0 && g_int_zero[2] == 0) {
        uart_puts("  [PASS] Global int zero init array\n");
    } else {
        uart_puts("  [FAIL] Global int zero init array\n");
        all_pass = 0;
    }

    uart_puts("\n");
    if (all_pass) {
        uart_puts("RESULT: ALL TESTS PASSED\n");
    } else {
        uart_puts("RESULT: SOME TESTS FAILED\n");
    }
    uart_puts("=========================================\n");
    uart_puts("\n");
}

void main(void) {
    /* Write marker to memory for emulator inspection */
    TEST_MARKER_ADDR[0] = 0xA5;  /* Magic marker */
    TEST_MARKER_ADDR[1] = 0x54;  /* 'T' = Test started */
    TEST_MARKER_ADDR[2] = 0x45;  /* 'E' = Executing */
    TEST_MARKER_ADDR[3] = 0xFF;  /* End marker */

    /* Print test header */
    print_test_header();

    /* Run array tests */
    test_char_arrays();
    test_int_arrays();
    test_local_arrays();

    /* Verify results */
    verify_arrays();

    /* Update memory marker to indicate completion */
    TEST_MARKER_ADDR[0] = 0xA5;  /* Magic marker */
    TEST_MARKER_ADDR[1] = 0x44;  /* 'D' = Done */
    TEST_MARKER_ADDR[2] = 0x4F;  /* 'O' = OK */
    TEST_MARKER_ADDR[3] = 0xFF;  /* End marker */

    /* Infinite loop (keep emulation running) */
    while (1)
        ;
}
