// test_sid_cia_mmemu: SID and CIA struct overlay validation
// Validates struct offsets for SID voices, filter, CIA ports/timers.
// Results at $4000; run on mmemu rawMega65.

#include <mega65.h>

volatile unsigned char *r = (char *)0x4000;

void main() {
    // --- SID voice 1 via helper macro ---
    SID1_VOICE(0)->freq_lo = 0x0C;       // $D400
    SID1_VOICE(0)->freq_hi = 0x11;       // $D401
    SID1_VOICE(0)->ctrl = 0x11;          // $D404 (triangle+gate)
    SID1_VOICE(0)->attack_decay = 0x09;  // $D405

    // --- SID direct member (non-array) ---
    sid1->volume = 0x0F;                 // $D418

    r[0] = *(volatile unsigned char *)0xD400;  // expect 0x0C
    r[1] = *(volatile unsigned char *)0xD401;  // expect 0x11
    r[2] = *(volatile unsigned char *)0xD404;  // expect 0x11
    r[3] = *(volatile unsigned char *)0xD405;  // expect 0x09
    r[4] = *(volatile unsigned char *)0xD418;  // expect 0x0F

    // --- SID voice 2: verify offset +7 ---
    SID1_VOICE(1)->freq_lo = 0xAA;       // $D407
    SID1_VOICE(1)->freq_hi = 0xBB;       // $D408

    r[5] = *(volatile unsigned char *)0xD407;  // expect 0xAA
    r[6] = *(volatile unsigned char *)0xD408;  // expect 0xBB

    // --- SID voice 3: verify offset +14 ---
    SID1_VOICE(2)->ctrl = 0x80;          // $D412 (noise)

    r[7] = *(volatile unsigned char *)0xD412;  // expect 0x80

    // --- SID2 at $D420 ---
    sid2->volume = 0x05;                 // $D438
    r[8] = *(volatile unsigned char *)0xD438;  // expect 0x05

    // --- CIA 1: ports and timer ---
    cia1->ddra = 0xFF;                   // $DC02
    cia1->timer_a_lo = 0xE8;            // $DC04
    cia1->timer_a_hi = 0x03;            // $DC05
    cia1->cra = 0x09;                   // $DC0E (start+oneshot)

    r[9]  = *(volatile unsigned char *)0xDC02;  // expect 0xFF
    r[10] = *(volatile unsigned char *)0xDC04;  // expect 0xE8
    r[11] = *(volatile unsigned char *)0xDC05;  // expect 0x03
    r[12] = *(volatile unsigned char *)0xDC0E;  // expect 0x09

    // --- CIA 2: port A ---
    cia2->pra = 0x03;                   // $DD00
    cia2->ddrb = 0x55;                  // $DD03

    r[13] = *(volatile unsigned char *)0xDD00;  // expect 0x03
    r[14] = *(volatile unsigned char *)0xDD03;  // expect 0x55

    // --- Bit constants ---
    r[15] = SID_TRIANGLE | SID_GATE;    // 0x11
    r[16] = JOY_UP | JOY_FIRE;         // 0x11
    r[17] = CIA_CRA_START;              // 0x01

    __asm__("brk");
}
