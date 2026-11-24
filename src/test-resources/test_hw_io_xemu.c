/* test_hw_io_xemu.c — <hw_io.h> (#645), on a scratch SD card image that
 * test_xemu.sh makes (disk45 sd create: the ROM, MEGA65.D81 named "hwio" as
 * drive 0, HWIO.BIN of the bytes 0, 7, 14, ... 105 in the FAT32 root)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] SD: hw_sd_init, sector 0 read (the MBR's $55 $AA), its write
 *       refused
 *   [1] SD: a sector before the first partition written, read back, put
 *       back as it was; flush
 *   [2] F011 (drive 0, the D81): track 40 sector 0 (the header: "D", the
 *       name), sector 1 (the BAM: 40/2), seek; sectors 24 and 25 of track
 *       41 (one physical sector's halves, side 1) written and read back
 *   [3] Ethernet: init, the MAC (six bytes, not all 0), send (a frame too
 *       short: 0), nothing received
 *   [4] RTC: the date and time read (2024-2099, in range); set_time 1
 *   [5] hypervisor: set_filename / load_file to bank 4 (the bytes), a name
 *       too long, a file not there; hw_hyper_trap($00): the DOS version
 *   [6] $AA sentinel
 */
#include <hw_io.h>
#include <dma.h>
#include <string.h>

#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned char buf[512], save[512], pat[256], back[256];

int main(void) {
    unsigned long start, s;
    unsigned int i;
    unsigned char mac[6];
    rtc_time_t t;
    int ok;
    OUT[6] = 0;

    ok = hw_sd_init() && hw_sd_read_sector(0, buf) && buf[510] == 0x55 && buf[511] == 0xAA;
    ok = ok && hw_sd_write_sector(0, buf) == 0;
    start = buf[454] | ((unsigned long)buf[455] << 8) | ((unsigned long)buf[456] << 16) |
            ((unsigned long)buf[457] << 24);
    OUT[0] = ok;

    /* (a sector between the hypervisor's (1) and the first partition) */
    s = 100;
    ok = start > s + 1 && hw_sd_read_sector(s, save);
    for (i = 0; i < 512; i++) buf[i] = (unsigned char)(i * 3 + 1);
    ok = ok && hw_sd_write_sector(s, buf);
    memset(buf, 0, sizeof buf);
    ok = ok && hw_sd_read_sector(s, buf);
    for (i = 0; i < 512; i++) ok = ok && buf[i] == (unsigned char)(i * 3 + 1);
    ok = ok && hw_sd_write_sector(s, save) && hw_sd_flush() && !hw_sd_has_error();
    OUT[1] = ok;

    ok = hw_fdc_init() && hw_fdc_read_sector(40, 0, buf);
    ok = ok && buf[0] == 40 && buf[1] == 3 && buf[2] == 0x44 && memcmp(buf + 4, "hwio", 4) == 0;
    ok = ok && hw_fdc_read_sector(40, 1, buf) && buf[0] == 40 && buf[1] == 2 && buf[2] == 0x44;
    ok = ok && hw_fdc_seek_track(41) && hw_fdc_seek_track(0) == 0 && hw_fdc_read_sector(81, 0, buf) == 0 &&
         hw_fdc_read_sector(40, 40, buf) == 0;
    for (i = 0; i < 256; i++) pat[i] = (unsigned char)(255 - i);
    ok = ok && hw_fdc_write_sector(41, 25, pat);
    for (i = 0; i < 256; i++) pat[i] = (unsigned char)i;
    ok = ok && hw_fdc_write_sector(41, 24, pat);
    ok = ok && hw_fdc_read_sector(41, 25, back) && back[0] == 255 && back[255] == 0;
    ok = ok && hw_fdc_read_sector(41, 24, back) && memcmp(back, pat, 256) == 0;
    ok = ok && !hw_fdc_is_busy();
    OUT[2] = ok;

    ok = hw_eth_init();
    hw_eth_get_mac(mac);
    ok = ok && (mac[0] | mac[1] | mac[2] | mac[3] | mac[4] | mac[5]) != 0;
    ok = ok && hw_eth_send_packet(buf, 4) == 0 && hw_eth_send_packet(0, 64) == 0;
    ok = ok && !hw_eth_rx_ready() && hw_eth_recv_packet(buf, 512) == 0;
    OUT[3] = ok;

    ok = hw_rtc_read_time(&t) && t.year >= 2024 && t.year <= 2099 && t.month >= 1 && t.month <= 12 &&
         t.day >= 1 && t.day <= 31 && t.hour <= 23 && t.minute <= 59 && t.second <= 59;
    ok = ok && hw_rtc_set_time(&t) && hw_rtc_read_time(0) == 0;
    OUT[4] = ok;

    dma_fill_far(0x40000UL, 16, 0xEE);
    ok = hw_hyper_set_filename("hwio.bin") && hw_hyper_load_file(0x40000UL);
    dma_copy_far((unsigned long)(unsigned int)back, 0x40000UL, 16);
    for (i = 0; i < 16; i++) ok = ok && back[i] == (unsigned char)(i * 7);
    ok = ok && hw_hyper_set_filename("a-name-that-is-longer-than-sixty-three-characters-for-the-hypervisor") == 0;
    ok = ok && hw_hyper_set_filename("nope.bin") && hw_hyper_load_file(0x40000UL) == 0;
    ok = ok && hw_hyper_trap(0x00, 0) == 1;
    OUT[5] = ok;
    OUT[6] = 0xAA;
    return 0;
}
