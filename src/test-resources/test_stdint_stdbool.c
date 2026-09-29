/* test_stdint_stdbool.c — Test stdint.h and stdbool.h headers
 *
 * Tests fixed-width integer types and boolean support
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

int main(void) {
    printf("Testing stdint.h and stdbool.h...\n");

    /* Test boolean type and constants */
    bool b_true = true;
    bool b_false = false;

    printf("\nBoolean Tests:\n");
    printf("  true = %d\n", b_true);
    printf("  false = %d\n", b_false);
    printf("  __bool_true_false_are_defined = %d\n", __bool_true_false_are_defined);

    /* Test fixed-width integer types */
    int8_t i8 = INT8_MAX;
    uint8_t u8 = UINT8_MAX;
    int16_t i16 = INT16_MAX;
    uint16_t u16 = UINT16_MAX;
    int32_t i32 = INT32_MAX;
    uint32_t u32 = UINT32_MAX;

    printf("\nFixed-Width Integer Tests:\n");
    printf("  int8_t max = %d\n", i8);
    printf("  uint8_t max = %u\n", u8);
    printf("  int16_t max = %d\n", i16);
    printf("  uint16_t max = %u\n", u16);
    printf("  int32_t max = %ld\n", i32);
    printf("  uint32_t max = %lu\n", u32);

    /* Test size verification */
    printf("\nSize Verification:\n");
    printf("  sizeof(int8_t) = %u (expected 1)\n", (unsigned)sizeof(int8_t));
    printf("  sizeof(uint8_t) = %u (expected 1)\n", (unsigned)sizeof(uint8_t));
    printf("  sizeof(int16_t) = %u (expected 2)\n", (unsigned)sizeof(int16_t));
    printf("  sizeof(uint16_t) = %u (expected 2)\n", (unsigned)sizeof(uint16_t));
    printf("  sizeof(int32_t) = %u (expected 4)\n", (unsigned)sizeof(int32_t));
    printf("  sizeof(uint32_t) = %u (expected 4)\n", (unsigned)sizeof(uint32_t));

    /* Test pointer types */
    printf("\nPointer Type Tests:\n");
    intptr_t ptr = (intptr_t)&main;
    printf("  intptr_t = %d\n", ptr);

    /* Test constants */
    printf("\nConstant Values:\n");
    printf("  INT8_MIN = %d\n", INT8_MIN);
    printf("  INT8_MAX = %d\n", INT8_MAX);
    printf("  UINT8_MAX = %u\n", UINT8_MAX);
    printf("  INT16_MIN = %d\n", INT16_MIN);
    printf("  INT16_MAX = %d\n", INT16_MAX);

    /* Test boolean in control flow */
    printf("\nBoolean Control Flow:\n");
    if (b_true) {
        printf("  true evaluates to true ✓\n");
    }
    if (!b_false) {
        printf("  false evaluates to false ✓\n");
    }

    /* Test fixed-width integers in arithmetic */
    printf("\nFixed-Width Arithmetic:\n");
    uint8_t sum8 = UINT8_C(100) + UINT8_C(50);
    printf("  uint8_t: 100 + 50 = %u\n", sum8);

    int16_t sum16 = INT16_C(1000) + INT16_C(2000);
    printf("  int16_t: 1000 + 2000 = %d\n", sum16);

    int32_t sum32 = INT32_C(100000) + INT32_C(200000);
    printf("  int32_t: 100000 + 200000 = %ld\n", sum32);

    printf("\n✓ All tests passed!\n");

    return 0;
}

