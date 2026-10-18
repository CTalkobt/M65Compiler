/* noescape/main.c — constants kept with an overlay's code (Issue #301):
 * __noescape objects and #pragma cc45 noescape_strings literals in code
 * overlays (copied and mapped) and in __far functions
 *
 * Results at $C0C0 (01 = pass):
 *   [0] overlay a: literals under the pragma, a static and an exported
 *       __noescape array, a table of pointers (its strings move too)
 *   [1] overlay a: a static local __noescape, a char array initialized from
 *       a literal, literals passed to resident functions (strlen)
 *   [2] an __escape literal passed from overlay a into overlay b (resident:
 *       b reads it while a is not loaded)
 *   [3] after a call into b, a's constants are there again (copied back)
 *   [4] __far functions: literals and a __noescape table in the far chunk
 *   [5] a resident function of the far object keeps its literal resident
 *   [6] a's resident pointer to a literal (a stored address) stays resident
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

unsigned int a_names(void);
unsigned int a_locals(void);
unsigned int a_to_b(void);
unsigned int a_again(void);
const char *a_stored(void);
unsigned int far_sum(void);
unsigned int near_len(void);

static unsigned int len(const char *s) { unsigned int n = 0; while (s[n]) n++; return n; }

int main(void) {
    OUT[0] = a_names() == 1234;
    OUT[1] = a_locals() == 5678;
    OUT[2] = a_to_b() == ('x' & 7) + ('y' & 7) + ('z' & 7) + 100;
    OUT[3] = a_again() == 4321;
    OUT[4] = far_sum() == 8 + 9 + 3;
    OUT[5] = near_len() == 9;
    OUT[6] = len(a_stored()) == 6;
    OUT[7] = 0xAA;
    return 0;
}
