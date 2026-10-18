/* noescape/ovl_a.c — overlay a: its constants kept in the overlay */
#pragma cc45 overlay(a)
#pragma cc45 noescape_strings
#include <string.h>

unsigned int b_sum(const char *s);

static const char __noescape names[] = "autoindent\0ai\0expandtab";
const unsigned char __noescape weights[4] = { 1, 2, 3, 4 };       /* exported */
static const char *const __noescape table[] = { "one", "three", "seventeen" };
static const unsigned int __attribute__((noescape)) scale = 10;

/* a resident pointer (data) to a literal: its address is stored, so the
 * literal stays resident (read in main after another overlay ran) */
static const char *stored = "stored";

/* "autoindent" 10 + "expandtab" 9 + weights 4 + "seventeen" 9 = 32 */
unsigned int a_names(void) {
    unsigned int n = strlen(names) + strlen(names + 14) + weights[3] + strlen(table[2]);
    if (strcmp(table[0], "one") != 0 || n != 32) return 0;
    return 1234;
}

unsigned int a_locals(void) {
    static const char __noescape word[] = "local";
    char buf[] = "copied text";
    unsigned int n = strlen(word) + strlen(buf);            /* 5 + 11 */
    if (n != 16 || buf[0] != 'c' || memcmp(buf + 7, "text", 4) != 0) return 0;
    return 5678 + scale - 10;
}

unsigned int a_to_b(void) {
    return b_sum(__escape "xyz");
}

/* after b ran in the region, a is copied back: its constants are there */
unsigned int a_again(void) {
    unsigned int n = b_sum(__escape "");
    if (strcmp(table[1], "three") != 0 || weights[0] != 1 || names[0] != 'a') return 0;
    return 4321 + n;
}

const char *a_stored(void) { return stored; }
