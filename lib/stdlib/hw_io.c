/* hw_io.c — SD card, floppy (F011), Ethernet and RTC by the MEGA65's
 * registers (<hw_io.h>, #645; mega65-core sdcardio.vhdl, ethernet.vhdl)
 *
 * The buffers are reached by DMA at their 28-bit addresses: the SD card's
 * $FFD6E00, the F011's $FFD6C00, Ethernet's $FFDE800 (512, 512, 2048
 * bytes). The hypervisor's DOS calls are in stdlib_asm/hw_hyper.s45.
 */

#include <hw_io.h>
#include <dma.h>
#include <time.h>

#define REG(a) (*(volatile unsigned char *)(a))

#define SD_BUF  0xFFD6E00UL
#define FDC_BUF 0xFFD6C00UL
#define ETH_BUF 0xFFDE800UL

/* ===== SD card ===== */

/* until the controller is not busy, or a timeout: 1 ready */
static int sd_wait(void) {
    unsigned long n;
    for (n = 0; n < 400000UL; n++)
        if (!(REG(0xD680) & SD_BUSY)) return 1;
    return 0;
}

int hw_sd_is_busy(void) {
    return (REG(0xD680) & SD_BUSY) != 0;
}

int hw_sd_has_error(void) {
    return (REG(0xD680) & SD_ERROR) != 0;
}

int hw_sd_is_sdhc(void) {
    return (REG(0xD680) & SD_SDHC) != 0;
}

void hw_sd_reset(void) {
    unsigned char sdhc = (unsigned char)(REG(0xD680) & SD_SDHC);
    REG(0xD680) = 0x00;         /* reset on */
    REG(0xD680) = 0x01;         /* and off */
    sd_wait();
    REG(0xD680) = sdhc ? 0x41 : 0x40;   /* the card's addressing as it was */
}

/* (the hypervisor has the card set up: it only has to be ready. No
 * command here: xemu reads a command written to $D680 back for a while,
 * and $44, the error flags' clear, would read as an error) */
int hw_sd_init(void) {
    return sd_wait() && !hw_sd_has_error();
}

/* the SD card's sector buffer in the CPU's view ($FFD6xxx: the SD card's
 * with $D689 bit 7 set, else the F011's) for a copy; the bit back after */
static void sd_copy(unsigned long dst, unsigned long src) {
    unsigned char sel = REG(0xD689);
    REG(0xD689) = (unsigned char)(sel | 0x80);
    dma_copy_far(dst, src, 512);
    REG(0xD689) = sel;
}

/* the sector's address: a block number on SDHC, else bytes */
static void sd_address(unsigned long sector) {
    if (!hw_sd_is_sdhc()) sector <<= 9;
    REG(0xD681) = (unsigned char)sector;
    REG(0xD682) = (unsigned char)(sector >> 8);
    REG(0xD683) = (unsigned char)(sector >> 16);
    REG(0xD684) = (unsigned char)(sector >> 24);
}

int hw_sd_read_sector(unsigned long sector, void *buffer) {
    if (!buffer || !sd_wait()) return 0;
    sd_address(sector);
    REG(0xD680) = SD_CMD_READ;
    if (!sd_wait() || hw_sd_has_error()) return 0;
    sd_copy((unsigned long)(unsigned int)buffer, SD_BUF);
    return 1;
}

int hw_sd_write_sector(unsigned long sector, const void *buffer) {
    /* (sector 0, the card's partition table, never: its gate is not opened) */
    if (!buffer || sector == 0 || !sd_wait()) return 0;
    sd_copy(SD_BUF, (unsigned long)(unsigned int)buffer);
    sd_address(sector);
    REG(0xD680) = 0x57;         /* the write gate open (about 1 ms) */
    REG(0xD680) = SD_CMD_WRITE;
    if (!sd_wait() || hw_sd_has_error()) return 0;
    return 1;
}

int hw_sd_flush(void) {
    if (!sd_wait()) return 0;
    REG(0xD680) = SD_CMD_FLUSH;
    return sd_wait() && !hw_sd_has_error();
}

/* ===== Floppy (F011) ===== */

/* the F011's buffer in the CPU's view ($D689 bit 7 clear) for a copy */
static void fdc_copy(unsigned long dst, unsigned long src, unsigned int n) {
    unsigned char sel = REG(0xD689);
    REG(0xD689) = (unsigned char)(sel & 0x7F);
    dma_copy_far(dst, src, n);
    REG(0xD689) = sel;
}

static int fdc_wait(void) {
    unsigned long n;
    for (n = 0; n < 2000000UL; n++)
        if (!(REG(0xD082) & 0x80)) return 1;
    return 0;
}

int hw_fdc_is_busy(void) {
    return (REG(0xD082) & 0x80) != 0;
}

int hw_fdc_get_status(void) {
    return REG(0xD082);
}

int hw_fdc_init(void) {
    REG(0xD080) = 0x20;         /* drive 0, the motor on, side 0 */
    REG(0xD081) = 0x00;         /* any command cancelled */
    return fdc_wait();
}

/* the head to physical track t (0-79) by steps from where it is */
static int fdc_seek(unsigned char t) {
    unsigned char n;
    /* (to track 0 first: TK0, $D082 bit 0) */
    for (n = 0; n < 90 && !(REG(0xD082) & 0x01); n++) {
        REG(0xD081) = 0x10;     /* a step out */
        if (!fdc_wait()) return 0;
    }
    for (n = 0; n < t; n++) {
        REG(0xD081) = 0x18;     /* a step in */
        if (!fdc_wait()) return 0;
    }
    return 1;
}

int hw_fdc_seek_track(int track) {
    if (track < 1 || track > 80) return 0;
    return fdc_seek((unsigned char)(track - 1));
}

/* CBM track 1-80, sector 0-39 to the F011's registers; 0 out of range */
static int fdc_select(int track, int sector) {
    unsigned char s, side;
    if (track < 1 || track > 80 || sector > 39) return 0;
    side = sector >= 20;
    s = (unsigned char)(sector % 20);
    REG(0xD080) = (unsigned char)(0x20 | (side ? 0x08 : 0));
    REG(0xD084) = (unsigned char)(track - 1);
    REG(0xD085) = (unsigned char)(s / 2 + 1);
    REG(0xD086) = side;
    return 1;
}

/* the physical sector selected into the buffer: 1 done */
static int fdc_read_phys(void) {
    if (!fdc_wait()) return 0;
    REG(0xD081) = 0x01;         /* the buffer pointers cleared */
    REG(0xD081) = FDC_CMD_READ;
    if (!fdc_wait()) return 0;
    return !(REG(0xD082) & 0x18);   /* (not found, CRC) */
}

int hw_fdc_read_sector(int track, int sector, void *buffer) {
    if (!buffer || !fdc_select(track, sector) || !fdc_read_phys()) return 0;
    fdc_copy((unsigned long)(unsigned int)buffer, FDC_BUF + ((sector & 1) ? 256 : 0), 256);
    return 1;
}

int hw_fdc_write_sector(int track, int sector, const void *buffer) {
    /* the physical sector read, its half replaced, written back */
    if (!buffer || !fdc_select(track, sector) || !fdc_read_phys()) return 0;
    if (REG(0xD082) & 0x02) return 0;   /* write protected */
    fdc_copy(FDC_BUF + ((sector & 1) ? 256 : 0), (unsigned long)(unsigned int)buffer, 256);
    REG(0xD081) = 0x01;
    REG(0xD081) = FDC_CMD_WRITE;
    if (!fdc_wait()) return 0;
    return !(REG(0xD082) & 0x18);
}

/* ===== Ethernet ===== */

void hw_eth_reset(void) {
    REG(0xD6E0) = 0x00;         /* the controller held in reset */
    REG(0xD6E0) = 0x03;         /* and let go: transmitting too */
}

int hw_eth_init(void) {
    hw_eth_reset();
    REG(0xD6E5) = (unsigned char)(REG(0xD6E5) | 0x10);   /* broadcast frames accepted */
    return 1;
}

int hw_eth_tx_ready(void) {
    return (REG(0xD6E0) & ETH_TXIDLE) != 0;
}

int hw_eth_rx_ready(void) {
    return (REG(0xD6E1) & ETH_RXIRQ) != 0;
}

int hw_eth_send_packet(const void *data, int length) {
    unsigned long n;
    unsigned int len = (unsigned int)length;
    if (!data || len < 8 || len > 2048) return 0;
    for (n = 0; n < 200000UL && !hw_eth_tx_ready(); n++) ;
    if (!hw_eth_tx_ready()) return 0;
    dma_copy_far(ETH_BUF, (unsigned long)(unsigned int)data, len);
    REG(0xD6E2) = (unsigned char)len;
    REG(0xD6E3) = (unsigned char)(len >> 8);
    REG(0xD6E4) = 0x01;         /* transmit */
    return 1;
}

int hw_eth_recv_packet(void *buffer, int max_length) {
    unsigned char hdr[2];
    unsigned int len;
    if (!buffer || !hw_eth_rx_ready()) return 0;
    /* the next frame into the CPU's view: its length, then its bytes */
    REG(0xD6E1) = 0x01;
    REG(0xD6E1) = 0x03;
    dma_copy_far((unsigned long)(unsigned int)hdr, ETH_BUF, 2);
    len = (hdr[0] | ((unsigned int)hdr[1] << 8)) & 0x7FF;
    if (len > (unsigned int)max_length) len = (unsigned int)max_length;
    if (len) dma_copy_far((unsigned long)(unsigned int)buffer, ETH_BUF + 2, len);
    return (int)len;
}

void hw_eth_get_mac(unsigned char mac[6]) {
    unsigned char i;
    for (i = 0; i < 6; i++) mac[i] = REG(0xD6E9 + i);
}

/* ===== Real-time clock ===== */

int hw_rtc_read_time(rtc_time_t *t) {
    struct tm tm;
    if (!t) return 0;
    rtc_read(&tm);
    t->year = tm.tm_year + 1900;
    t->month = tm.tm_mon + 1;
    t->day = tm.tm_mday;
    t->hour = tm.tm_hour;
    t->minute = tm.tm_min;
    t->second = tm.tm_sec;
    return 1;
}

int hw_rtc_set_time(const rtc_time_t *t) {
    struct tm tm;
    struct timespec ts;
    if (!t) return 0;
    tm.tm_year = t->year - 1900;
    tm.tm_mon = t->month - 1;
    tm.tm_mday = t->day;
    tm.tm_hour = t->hour;
    tm.tm_min = t->minute;
    tm.tm_sec = t->second;
    tm.tm_isdst = 0;
    ts.tv_sec = mktime(&tm);
    ts.tv_nsec = 0;
    return clock_settime(CLOCK_REALTIME, &ts) == 0;
}
