/* test_ln45_layout_xemu.c — ln45 segment placement with -d / -b
 *
 * test_xemu.sh links this with the data above the text (-d 8000) and below
 * it (-t 3000 -d 2100: the image starts at the data, a zero gap, then the
 * text), and checks that data overlapping the text or the BASIC stub is an
 * error.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] initialized globals read back (the data is where it was linked)
 *   [1] BSS globals start at 0 and can be written (BSS after the text)
 *   [2] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

int tbl[4] = { 11, 22, 33, 44 };
char msg[] = "data";
int zero[3];

int main(void) {
    OUT[2] = 0xAA;
    OUT[0] = tbl[0] == 11 && tbl[3] == 44 && msg[0] == 'd' && msg[3] == 'a';
    OUT[1] = zero[0] == 0 && zero[2] == 0;
    zero[1] = 5;
    OUT[1] = OUT[1] && zero[1] == 5 && tbl[1] == 22;
    return 0;
}
