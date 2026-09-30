#pragma cc45 no_zp_save
/* test_misc_xemu.c — Validate setjmp.h, time.h, errno.h via xemu
 *
 * Expected at $C080: 01 02 03 04 05 06 AA
 */
#include <setjmp.h>
#include <time.h>
#include <errno.h>

#define RESULT(i) (*(volatile unsigned char *)(0xC080 + (i)))

jmp_buf env;
int setjmp_count;

void do_longjmp(void) {
    longjmp(env, 42);
}

void main() {
    /* Test 1: setjmp returns 0 first time */
    int val = setjmp(env);
    setjmp_count++;
    if (setjmp_count == 1 && val == 0) {
        RESULT(0) = 0x01;
        /* Test 2: longjmp makes setjmp return the given value */
        longjmp(env, 7);
    }
    if (setjmp_count == 2 && val == 7) RESULT(1) = 0x02;

    /* Test 3: longjmp with val=0 returns 1 */
    setjmp_count = 0;
    val = setjmp(env);
    setjmp_count++;
    if (setjmp_count == 1 && val == 0) {
        longjmp(env, 0);
    }
    if (setjmp_count == 2 && val == 1) RESULT(2) = 0x03;

    /* Test 4: clock() returns nonzero (jiffy clock running) */
    unsigned int c = clock();
    /* clock may be 0 at boot — call twice with a tiny delay */
    int i;
    for (i = 0; i < 100; i++) {}  /* brief delay */
    unsigned int c2 = clock();
    if (c2 != c || c != 0) RESULT(3) = 0x04;

    /* Test 5: errno starts at 0 */
    if (errno == 0) RESULT(4) = 0x05;

    /* Test 6: errno can be set and read */
    errno = 42;
    if (errno == 42) RESULT(5) = 0x06;
    errno = 0;

    RESULT(6) = 0xAA;
}
