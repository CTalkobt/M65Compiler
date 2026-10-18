/* noescape/far.c — __far functions: their literals and a __noescape table
 * in the far chunk; a resident function's literal stays resident */
#pragma cc45 noescape_strings
#include <string.h>

static const unsigned char __noescape fartab[3] = { 7, 8, 9 };

/* "far text" 8 + fartab[2] 9 + "abc" 3 */
__far unsigned int far_sum(void) {
    return strlen("far text") + fartab[2] + strlen("abc");
}

unsigned int near_len(void) { return strlen("near text"); }
