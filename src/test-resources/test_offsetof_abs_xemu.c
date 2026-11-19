/* test_offsetof_abs_xemu.c — #639: offsetof in every constant expression,
 * __builtin_abs / __builtin_labs with <stdlib.h> included (abs / labs are
 * its macros), sizeof(typeof(x)), __CC45_VERSION__.
 *
 * Results at $C0C0 (01 = pass, $AA sentinel):
 *   [0] offsetof in array sizes and enum values   [1] in case labels
 *   [2] in a bit-field width and _Static_assert    [3] __builtin_abs
 *   [4] __builtin_labs                              [5] sizeof(typeof(x))
 *   [6] __CC45_VERSION__ is at least 1.1.0
 */
#include <stddef.h>
#include <stdlib.h>

#define OUT ((volatile unsigned char *)0xC0C0)

struct inner { char a; int b[3]; };
struct rec {
    char tag;
    int count;
    struct inner in[2];
    long last;
};
typedef struct rec rec_t;

_Static_assert(offsetof(struct rec, count) == 1, "count at 1");
_Static_assert(__builtin_offsetof(rec_t, in[1].b[2]) == 3 + 7 + 1 + 4, "in[1].b[2] at 15");
_Static_assert(offsetof(struct rec, last) == 17, "last at 17");

char pad[offsetof(struct rec, in)];                    /* 3 */
enum { OFF_LAST = offsetof(struct rec, last), OFF_B = __builtin_offsetof(struct inner, b[1]) };
struct bits { unsigned v : offsetof(struct rec, in) + 2; };   /* 5 bits */
_Static_assert(sizeof(struct bits) == 2, "an unsigned unit");

volatile signed int vneg = -1234, vpos = 77;
volatile signed long lneg = -100000L;
volatile unsigned char which = 3;

static unsigned char classify(unsigned char k) {
    switch (k) {
    case offsetof(struct rec, count): return 10;
    case offsetof(struct rec, in): return 20;
    case offsetof(struct rec, last): return 30;
    default: return 0;
    }
}

int main(void) {
    struct bits bt;
    long lv;
    int arr[5];
    char c;
    OUT[0] = sizeof pad == 3 && OFF_LAST == 17 && OFF_B == 3;
    which = 17;
    c = classify(which);
    which = 3;
    OUT[1] = c == 30 && classify(which) == 20 && classify(1) == 10 && classify(2) == 0;
    bt.v = 31;
    c = bt.v == 31;
    bt.v = 32;                          /* 5 bits: 0 */
    OUT[2] = c && bt.v == 0;
    OUT[3] = __builtin_abs(vneg) == 1234 && __builtin_abs(vpos) == 77 && __builtin_abs(vneg + 1234) == 0;
    lv = __builtin_labs(lneg);
    OUT[4] = lv == 100000L && __builtin_labs(-lneg) == 100000L;
    arr[0] = 1;
    OUT[5] = arr[0] == 1 && sizeof(typeof(arr)) == 10 && sizeof(typeof(lv)) == 4 && sizeof(typeof(c) *) == 2 &&
             sizeof(typeof(struct rec)) == 21;
#if __CC45_VERSION__ >= 10100
    OUT[6] = 1;
#else
    OUT[6] = 0;
#endif
    OUT[7] = 0xAA;
    return 0;
}
