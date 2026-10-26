/* test_mega65_regs_xemu.c — the register overlays of <mega65.h>: each
 * member at its register's address (no register written: reading back a
 * write-only SID register, a running CIA timer, or starting an SD-card read
 * or a hypervisor trap is not what is tested). Ported from mmemu's
 * test_sid_cia_mmemu.c and test_devices_mmemu.c, which wrote and read back
 * through mmemu's plain-RAM I/O.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] SID: voice 1-3 members, the volume of SID 1 and 2
 *   [1] CIA 1 and 2: ports, data directions, timer A, control A
 *   [2] FDC, SD card, Ethernet, hypervisor traps
 *   [3] the bit constants (SID, joystick, CIA, FDC, SD, Ethernet)
 *   [4] $AA sentinel
 */
#include <mega65.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define AT(p, a) ((unsigned)(p) == (unsigned)(a))

int main(void) {
    OUT[0] = AT(&SID1_VOICE(0)->freq_lo, 0xD400) && AT(&SID1_VOICE(0)->ctrl, 0xD404) &&
             AT(&SID1_VOICE(0)->attack_decay, 0xD405) && AT(&SID1_VOICE(1)->freq_lo, 0xD407) &&
             AT(&SID1_VOICE(2)->ctrl, 0xD412) && AT(&sid1->voice[1].freq_lo, 0xD407) &&
             AT(&sid1->volume, 0xD418) && AT(&sid2->volume, 0xD438);
    OUT[1] = AT(&cia1->pra, 0xDC00) && AT(&cia1->ddra, 0xDC02) && AT(&cia1->timer_a_lo, 0xDC04) &&
             AT(&cia1->timer_a_hi, 0xDC05) && AT(&cia1->cra, 0xDC0E) && AT(&cia2->pra, 0xDD00) &&
             AT(&cia2->ddrb, 0xDD03);
    OUT[2] = AT(&fdc->track, 0xD084) && AT(&fdc->sector, 0xD085) && AT(&fdc->side, 0xD086) &&
             AT(&fdc->data, 0xD087) && AT(&sd->command, 0xD681) && AT(&sd->addr0, 0xD682) &&
             AT(&sd->addr3, 0xD685) && AT(&eth->ctrl, 0xD6E0) && AT(&eth->txsz_lo, 0xD6E2) &&
             AT(&eth->txsz_hi, 0xD6E3) && AT(&hyper->trap[0], 0xD640) && AT(&hyper->trap[1], 0xD641);
    OUT[3] = (SID_TRIANGLE | SID_GATE) == 0x11 && (JOY_UP | JOY_FIRE) == 0x11 && CIA_CRA_START == 0x01 &&
             FDC_CMD_READ == 0x40 && SD_CMD_READ == 0x01 && (ETH_RST | ETH_TXRST) == 0x03;
    OUT[4] = 0xAA;
    return 0;
}
