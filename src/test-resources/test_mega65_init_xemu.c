/* test_mega65_init_xemu.c — crt0_mega65(_zp).o45's _init_features, linked
 * by name before or after the library (#741): 40 MHz as BASIC 65's SPEED
 * sets it, the other bits of the registers kept
 *
 * Results at $C0C0 (01 = pass):
 *   [0] after start-up: FAST ($D031 bit 6) and VFAST ($D054 bit 6) set
 *       (BASIC 65 already runs at 40 MHz, so this alone does not show the
 *       routine ran: test_xemu.sh checks in the map that crt0's call goes
 *       to the object's _init_features)
 *   [1] init_features() again after clearing both, with $D054 bit 4
 *       (SPR640) set as a marker: FAST and VFAST set again
 *   [2] the marker kept (the register is not overwritten)
 *   [3] the hot register bit ($D05D bit 7) as it was
 *   [4] $AA sentinel
 */
#define OUT   ((volatile unsigned char *)0xC0C0)
#define D031  (*(volatile unsigned char *)0xD031)
#define D054  (*(volatile unsigned char *)0xD054)
#define D05D  (*(volatile unsigned char *)0xD05D)

void init_features(void);

int main(void) {
    unsigned char hot;

    OUT[0] = (D031 & 0x40) && (D054 & 0x40);

    hot = D05D & 0x80;
    D031 &= ~0x40;
    D054 = (D054 & ~0x40) | 0x10;
    init_features();
    OUT[1] = (D031 & 0x40) && (D054 & 0x40);
    OUT[2] = (D054 & 0x10) != 0;
    OUT[3] = (D05D & 0x80) == hot;
    D054 &= ~0x10;

    OUT[4] = 0xAA;
    return 0;
}
