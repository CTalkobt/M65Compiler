/* hw_io.h — Phase 88: Extended I/O & Peripherals
 *
 * Hardware I/O control for SD card, Ethernet, Floppy drive,
 * and other MEGA65 peripherals.
 */

#ifndef HW_IO_H
#define HW_IO_H

/* SD Card Control */
typedef enum {
    SD_CMD_RESET = 0x00,
    SD_CMD_READ = 0x01,
    SD_CMD_WRITE = 0x02,
    SD_CMD_FLUSH = 0x03,
} sd_command_t;

typedef enum {
    SD_BUSY = 0x80,
    SD_RESET = 0x40,
    SD_ERROR = 0x20,
    SD_SDHC = 0x10,
} sd_status_t;

/* Ethernet Control */
typedef enum {
    ETH_RST = 0x01,
    ETH_TXEN = 0x02,
    ETH_RXEN = 0x04,
    ETH_RXIRQ = 0x10,
    ETH_TXDONE = 0x20,
} eth_control_t;

/* Floppy Drive Control */
typedef enum {
    FDC_CMD_READ = 0x40,
    FDC_CMD_WRITE = 0xA0,
    FDC_CMD_STEP_IN = 0x18,
    FDC_CMD_STEP_OUT = 0x10,
    FDC_CMD_RESTORE = 0x08,
} fdc_command_t;

/* ===== SD Card API ===== */
int hw_sd_init(void);
int hw_sd_is_busy(void);
int hw_sd_has_error(void);
int hw_sd_is_sdhc(void);
int hw_sd_read_sector(unsigned int sector, void *buffer);
int hw_sd_write_sector(unsigned int sector, const void *buffer);
int hw_sd_flush(void);
void hw_sd_reset(void);

/* ===== Ethernet API ===== */
int hw_eth_init(void);
void hw_eth_reset(void);
int hw_eth_tx_ready(void);
int hw_eth_rx_ready(void);
int hw_eth_send_packet(const void *data, int length);
int hw_eth_recv_packet(void *buffer, int max_length);
void hw_eth_get_mac(unsigned char mac[6]);

/* ===== Floppy Drive API ===== */
int hw_fdc_init(void);
int hw_fdc_is_busy(void);
int hw_fdc_read_sector(int track, int sector, void *buffer);
int hw_fdc_write_sector(int track, int sector, const void *buffer);
int hw_fdc_seek_track(int track);
int hw_fdc_get_status(void);

/* ===== Real-Time Clock ===== */
typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} rtc_time_t;

int hw_rtc_read_time(rtc_time_t *time);
int hw_rtc_set_time(const rtc_time_t *time);

/* ===== Hypervisor Traps ===== */
int hw_hyper_trap(int trap_num, unsigned int param);
int hw_hyper_set_filename(const char *filename);
int hw_hyper_load_file(unsigned int address);

#endif
