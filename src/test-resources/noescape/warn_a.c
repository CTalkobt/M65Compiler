/* noescape/warn_a.c — ln45 warns when overlay a passes the address of
 * __noescape data to a function that can reach another overlay (#301) */
#pragma cc45 overlay(a)
#pragma cc45 noescape_strings
void msgf(const char *f, const char *s);
void show(const char *s);
static void local_show(const char *s) { show(s); }
static const char *const __noescape table[] = { "one", "two" };
void (*hook)(const char *);
void a_run(unsigned char i, unsigned char b) {
    const char *p = b ? "TOP" : "BOTTOM";
    msgf("pattern: %s", p);
    show(table[i]);
    local_show("same overlay");
    msgf(__escape "safe %s", __escape "x");
    hook(table[0]);
}
