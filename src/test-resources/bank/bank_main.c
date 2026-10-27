/* bank_main.c — __bank(n) (#69): variables and functions kept in a chosen
 * bank (chip RAM 4 and 5, attic RAM $800-$87F), with bank_data.c. ln45
 * writes them into the overlay files, which the program loads at start;
 * the variables are reached as far objects, the functions through their
 * mapped overlays.
 *
 * Results at $C0C0 (01 = pass):
 *   +0 a 10000-byte array in bank 4, an attic RAM table, sizeof
 *   +1 a struct in bank 5, a scalar in bank 4 (=, op=, a far pointer to it)
 *   +2 extern data of the other file (bank 5)
 *   +3 __bank(4) / __bank(5) functions calling each other
 *   +4 their far addresses in their banks            +5 $AA
 */
#define OUT ((volatile unsigned char *)0xC0C0)

struct S { char a; int b; };

__bank(4) char big[10000];
__bank(0x800) long attic_tab[4] = {100000L, 200000L, 300000L, 400000L};
__bank(5) struct S rec = {3, 400};
__bank(4) int cnt;
extern __bank(5) const char msg[];
extern __bank(4) int helper4(int x);

__bank(5) int helper5(int x) { return helper4(x) * 2; }
static void inc(int __far *p) { *p += 1; }

int main(void) {
    int i;
    long s = 0;
    for (i = 0; i < 10000; i++) big[i] = (char)i;
    for (i = 0; i < 4; i++) s += attic_tab[i];
    OUT[0] = big[9999] == (char)9999 && big[300] == (char)300 && s == 1000000L && sizeof big == 10000;

    cnt = rec.b;
    cnt += rec.a;
    inc(&cnt); inc(&cnt);
    OUT[1] = cnt == 405 && rec.a == 3;

    OUT[2] = msg[0] == 'h' && msg[4] == 'o' && msg[5] == 0;

    OUT[3] = helper5(20) == 42;

    OUT[4] = (long)&big[0] >= 0x40000L && (long)&big[0] < 0x50000L && (long)&rec >= 0x50000L &&
             (long)&rec < 0x60000L && (long)&attic_tab[1] >= 0x8000000L;

    OUT[5] = 0xAA;
    return 0;
}
