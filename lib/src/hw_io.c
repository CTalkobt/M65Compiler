/* hw_io.c — Extended I/O & Peripherals Implementation */

#include "hw_io.h"
#include <stdint.h>
#include <string.h>

/* SD Card registers at $D680-$D69F */
#define SD_STATUS (*(volatile unsigned char *)0xD680)
#define SD_COMMAND (*(volatile unsigned char *)0xD681)
#define SD_ADDR0 (*(volatile unsigned char *)0xD682)
#define SD_ADDR1 (*(volatile unsigned char *)0xD683)
#define SD_ADDR2 (*(volatile unsigned char *)0xD684)
#define SD_ADDR3 (*(volatile unsigned char *)0xD685)
#define SD_BUF_ADDR_LO (*(volatile unsigned char *)0xD688)
#define SD_BUF_ADDR_HI (*(volatile unsigned char *)0xD689)
#define SD_RDATA (*(volatile unsigned char *)0xD690)

/* Ethernet registers at $D6E0-$D6FF */
#define ETH_CTRL (*(volatile unsigned char *)0xD6E0)
#define ETH_TXSZLO (*(volatile unsigned char *)0xD6E1)
#define ETH_TXSZHI (*(volatile unsigned char *)0xD6E2)
#define ETH_RXBUF (*(volatile unsigned char *)0xD6E8)
#define ETH_TXBUF (*(volatile unsigned char *)0xD6E9)

/* Floppy Drive registers at $D080-$D09F */
#define FDC_CONTROL (*(volatile unsigned char *)0xD080)
#define FDC_COMMAND (*(volatile unsigned char *)0xD081)
#define FDC_STATUS2 (*(volatile unsigned char *)0xD082)
#define FDC_TRACK (*(volatile unsigned char *)0xD084)
#define FDC_SECTOR (*(volatile unsigned char *)0xD085)
#define FDC_DATA (*(volatile unsigned char *)0xD087)

/* Real-Time Clock at $D6F0-$D6FF (internal) */
#define RTC_TENTHS (*(volatile unsigned char *)0xD6F0)
#define RTC_SECONDS (*(volatile unsigned char *)0xD6F1)
#define RTC_MINUTES (*(volatile unsigned char *)0xD6F2)
#define RTC_HOURS (*(volatile unsigned char *)0xD6F3)

/* ===== SD Card Implementation ===== */

int hw_sd_init(void) {
    hw_sd_reset();
    return 1;
}

int hw_sd_is_busy(void) {
    return (SD_STATUS & SD_BUSY) != 0;
}

int hw_sd_has_error(void) {
    return (SD_STATUS & SD_ERROR) != 0;
}

int hw_sd_is_sdhc(void) {
    return (SD_STATUS & SD_SDHC) != 0;
}

int hw_sd_read_sector(unsigned int sector, void *buffer) {
    if (hw_sd_is_busy()) return 0;

    /* Set sector address (32-bit) */
    SD_ADDR0 = (unsigned char)(sector & 0xFF);
    SD_ADDR1 = (unsigned char)((sector >> 8) & 0xFF);
    SD_ADDR2 = (unsigned char)((sector >> 16) & 0xFF);
    SD_ADDR3 = (unsigned char)((sector >> 24) & 0xFF);

    /* Set buffer address (assuming low memory) */
    SD_BUF_ADDR_LO = (unsigned char)((intptr_t)buffer & 0xFF);
    SD_BUF_ADDR_HI = (unsigned char)(((intptr_t)buffer >> 8) & 0xFF);

    /* Issue read command */
    SD_COMMAND = SD_CMD_READ;

    /* Wait for completion */
    while (hw_sd_is_busy()) {
        /* Spin */
    }

    return !hw_sd_has_error();
}

int hw_sd_write_sector(unsigned int sector, const void *buffer) {
    if (hw_sd_is_busy()) return 0;

    /* Set sector address */
    SD_ADDR0 = (unsigned char)(sector & 0xFF);
    SD_ADDR1 = (unsigned char)((sector >> 8) & 0xFF);
    SD_ADDR2 = (unsigned char)((sector >> 16) & 0xFF);
    SD_ADDR3 = (unsigned char)((sector >> 24) & 0xFF);

    /* Set buffer address */
    SD_BUF_ADDR_LO = (unsigned char)((intptr_t)buffer & 0xFF);
    SD_BUF_ADDR_HI = (unsigned char)(((intptr_t)buffer >> 8) & 0xFF);

    /* Issue write command */
    SD_COMMAND = SD_CMD_WRITE;

    /* Wait for completion */
    while (hw_sd_is_busy()) {
        /* Spin */
    }

    return !hw_sd_has_error();
}

int hw_sd_flush(void) {
    if (hw_sd_is_busy()) return 0;
    SD_COMMAND = SD_CMD_FLUSH;

    while (hw_sd_is_busy()) {
        /* Spin */
    }

    return !hw_sd_has_error();
}

void hw_sd_reset(void) {
    SD_COMMAND = SD_CMD_RESET;

    /* Wait for reset to complete */
    int timeout = 10000;
    while (hw_sd_is_busy() && timeout-- > 0) {
        /* Spin */
    }
}

/* ===== Ethernet Implementation ===== */

int hw_eth_init(void) {
    hw_eth_reset();
    return 1;
}

void hw_eth_reset(void) {
    ETH_CTRL |= ETH_RST;
    ETH_CTRL &= ~ETH_RST;
}

int hw_eth_tx_ready(void) {
    return (ETH_CTRL & ETH_TXDONE) != 0;
}

int hw_eth_rx_ready(void) {
    return (ETH_CTRL & ETH_RXIRQ) != 0;
}

int hw_eth_send_packet(const void *data, int length) {
    if (length <= 0 || length > 1536) return 0;

    unsigned char *packet_data = (unsigned char *)data;

    /* Write packet to TX buffer */
    for (int i = 0; i < length; i++) {
        ETH_TXBUF = packet_data[i];
    }

    /* Set packet size and enable TX */
    ETH_TXSZLO = (unsigned char)(length & 0xFF);
    ETH_TXSZHI = (unsigned char)((length >> 8) & 0xFF);
    ETH_CTRL |= ETH_TXEN;

    return 1;
}

int hw_eth_recv_packet(void *buffer, int max_length) {
    if (!hw_eth_rx_ready()) return 0;

    unsigned char *packet_buffer = (unsigned char *)buffer;
    int length = 0;

    /* Read from RX buffer (simplified) */
    while (length < max_length) {
        unsigned char byte = ETH_RXBUF;
        if (length == 0 && byte == 0) break;  /* End of packet */
        packet_buffer[length++] = byte;
    }

    return length;
}

void hw_eth_get_mac(unsigned char mac[6]) {
    /* MAC address at $D6E9-$D6ED (simplified read) */
    volatile unsigned char *mac_addr = (volatile unsigned char *)0xD6E9;
    for (int i = 0; i < 6; i++) {
        mac[i] = mac_addr[i];
    }
}

/* ===== Floppy Drive Implementation ===== */

int hw_fdc_init(void) {
    FDC_COMMAND = FDC_CMD_RESTORE;
    return 1;
}

int hw_fdc_is_busy(void) {
    return (FDC_CONTROL & 0x80) != 0;
}

int hw_fdc_read_sector(int track, int sector, void *buffer) {
    if (hw_fdc_is_busy()) return 0;

    FDC_TRACK = (unsigned char)track;
    FDC_SECTOR = (unsigned char)sector;
    FDC_COMMAND = FDC_CMD_READ;

    while (hw_fdc_is_busy()) {
        /* Spin */
    }

    return hw_fdc_get_status() == 0;
}

int hw_fdc_write_sector(int track, int sector, const void *buffer) {
    if (hw_fdc_is_busy()) return 0;

    FDC_TRACK = (unsigned char)track;
    FDC_SECTOR = (unsigned char)sector;
    FDC_COMMAND = FDC_CMD_WRITE;

    while (hw_fdc_is_busy()) {
        /* Spin */
    }

    return hw_fdc_get_status() == 0;
}

int hw_fdc_seek_track(int track) {
    if (hw_fdc_is_busy()) return 0;

    FDC_TRACK = (unsigned char)track;
    FDC_COMMAND = FDC_CMD_RESTORE;

    while (hw_fdc_is_busy()) {
        /* Spin */
    }

    return 1;
}

int hw_fdc_get_status(void) {
    return FDC_STATUS2;
}

/* ===== Real-Time Clock Implementation ===== */

int hw_rtc_read_time(rtc_time_t *time) {
    if (!time) return 0;

    time->second = RTC_SECONDS;
    time->minute = RTC_MINUTES;
    time->hour = RTC_HOURS;
    time->day = 1;      /* Not directly available */
    time->month = 1;
    time->year = 2024;

    return 1;
}

int hw_rtc_set_time(const rtc_time_t *time) {
    if (!time) return 0;

    RTC_SECONDS = (unsigned char)time->second;
    RTC_MINUTES = (unsigned char)time->minute;
    RTC_HOURS = (unsigned char)time->hour;

    return 1;
}

/* ===== Hypervisor Traps ===== */

#define HYPER_BASE 0xD640

int hw_hyper_trap(int trap_num, unsigned int param) {
    volatile unsigned char *trap_reg = (volatile unsigned char *)(HYPER_BASE + trap_num);
    *trap_reg = (unsigned char)(param & 0xFF);
    return 1;
}

int hw_hyper_set_filename(const char *filename) {
    if (!filename) return 0;
    return hw_hyper_trap(0x2E, (unsigned int)filename);
}

int hw_hyper_load_file(unsigned int address) {
    return hw_hyper_trap(0x36, address);
}
