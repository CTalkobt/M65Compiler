// test_devices_mmemu: FDC, SD card, Ethernet, Hypervisor struct validation
// Validates struct offsets by writing and reading back at known addresses.
// Results at $4000; run on mmemu rawMega65.

#include <mega65.h>

volatile unsigned char *r = (char *)0x4000;

void main() {
    // --- FDC registers ---
    fdc->track = 0x0A;            // $D084
    fdc->sector = 0x05;           // $D085
    fdc->side = 0x01;             // $D086
    fdc->data = 0xCC;             // $D087

    r[0] = *(volatile unsigned char *)0xD084;  // expect 0x0A
    r[1] = *(volatile unsigned char *)0xD085;  // expect 0x05
    r[2] = *(volatile unsigned char *)0xD086;  // expect 0x01
    r[3] = *(volatile unsigned char *)0xD087;  // expect 0xCC

    // --- SD card registers ---
    sd->addr0 = 0x10;             // $D682
    sd->addr1 = 0x20;             // $D683
    sd->addr2 = 0x30;             // $D684
    sd->addr3 = 0x40;             // $D685
    sd->command = 0x01;           // $D681

    r[4] = *(volatile unsigned char *)0xD682;  // expect 0x10
    r[5] = *(volatile unsigned char *)0xD683;  // expect 0x20
    r[6] = *(volatile unsigned char *)0xD684;  // expect 0x30
    r[7] = *(volatile unsigned char *)0xD685;  // expect 0x40
    r[8] = *(volatile unsigned char *)0xD681;  // expect 0x01

    // --- Ethernet registers ---
    eth->ctrl = 0x06;             // $D6E0 (TXEN|RXEN)
    eth->txszlo = 0x5A;           // $D6E1
    eth->txszhi = 0x00;           // $D6E2

    r[9]  = *(volatile unsigned char *)0xD6E0;  // expect 0x06
    r[10] = *(volatile unsigned char *)0xD6E1;  // expect 0x5A
    r[11] = *(volatile unsigned char *)0xD6E2;  // expect 0x00

    // --- Hypervisor trap ---
    hyper->trap[0] = 0x42;        // $D640

    r[12] = *(volatile unsigned char *)0xD640;  // expect 0x42

    // --- Constants ---
    r[13] = FDC_CMD_READ;         // 0x40
    r[14] = SD_CMD_READ;          // 0x01
    r[15] = ETH_TXEN | ETH_RXEN; // 0x06

    __asm__("brk");
}
