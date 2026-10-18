/* noescape/ovl_b.c — overlay b: reads a string overlay a passed it */
#pragma cc45 overlay(b)

unsigned int b_sum(const char *s) {
    unsigned int n = 0;
    if (!*s) return 0;
    while (*s) n += *s++ & 7;
    return n + 100;
}
