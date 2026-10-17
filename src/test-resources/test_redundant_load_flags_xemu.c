/* test_redundant_load_flags_xemu.c — a comparison's stored result tested by
 * a branch (Issue #282)
 *
 * cr = c == 0x0D stores the boolean, and the if reloads it for the flags of
 * its branch; ca45 removed that reload as redundant (A still held the
 * value) although an ldx #0 in between had set the flags from X, so a CR
 * was never recognised at -O2.
 * Run in both conventions.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] the issue's loop: CR and LF both replaced, the last cr flag 0
 *   [1] the stored comparison counts CRs (two of five characters)
 *   [2] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned char vals[] = { 'a', 0x0D, 'b', 0x0A };
static unsigned char pos;
int getx(void) { if (pos >= 4) return 0xFFFF; return vals[pos++]; }

static unsigned char text[] = { 'x', 0x0D, 0x0D, 'y', 0x0A };
unsigned char tpos;
int nextc(void) { if (tpos >= 5) return 0xFFFF; return text[tpos++]; }

int main(void) {
    unsigned char r[8];
    int c;
    unsigned char i = 0, cr = 0, crs = 0, ok;

    while ((c = getx()) != 0xFFFF) {
        cr = c == 0x0D;
        if (c == 0x0D || c == 0x0A) { r[i++] = 0x22; continue; }
        r[i++] = (unsigned char)c;
    }
    OUT[0] = i == 4 && r[0] == 'a' && r[1] == 0x22 && r[2] == 'b' && r[3] == 0x22 && cr == 0;

    while ((c = nextc()) != 0xFFFF) {
        ok = c == 0x0D;
        if (ok) crs++;
    }
    OUT[1] = crs == 2;
    OUT[2] = 0xAA;
    return 0;
}
