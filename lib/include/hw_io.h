/* hw_io.h — the MEGA65's SD card, floppy controller, Ethernet, real-time
 * clock and hypervisor DOS calls, by their registers (#645)
 *
 * SD card ($D680-$D684; the sector buffer at $FFD6E00, in a program's view
 * of $FFD6xxx while $D689 bit 7 is set, which the library sets around its
 * copies; the F011's otherwise): sectors of 512
 * bytes by number (SDHC: block numbers), a whole card: write a card only
 * where its file system and the hypervisor's mounted disk images are not.
 *   hw_sd_read_sector / hw_sd_write_sector   1 done, 0 an error or a timeout
 * Floppy (the F011, $D080-$D08F; its buffer at $FFD6C00): the drive or
 * disk image mounted as drive 0 (the internal drive, or a D81 the
 * hypervisor attached), by CBM track 1-80 and sector 0-39 (256 bytes, as
 * a D81's): track - 1, side sector / 20, sector (sector % 20) / 2 + 1, the
 * half (sector & 1) of the 512 bytes. hw_fdc_seek_track moves the head.
 * Ethernet ($D6E0-$D6EE; the frame buffers at $FFDE800): a frame of 8 to
 * 2048 bytes sent; received frames read one at a time (its length first
 * in the buffer). hw_eth_init: out of reset, broadcast frames accepted.
 * Real-time clock: hw_rtc_read_time / hw_rtc_set_time through <time.h>'s
 * rtc_read / clock_settime (years 2000-2099; 1 done).
 * Hypervisor DOS (a trap: A the function, at $D640): hw_hyper_set_filename
 * (up to 63 characters, PETSCII to ASCII; copied to a page-aligned buffer
 * below $7F00, as the hypervisor wants), hw_hyper_load_file (the file named,
 * whole, to a 28-bit address: from the SD card's FAT32 partition, the
 * current directory), hw_hyper_trap (any of the DOS functions: A = trap_num,
 * X/Y = param; 1 when it succeeded, carry set). Errors: hw_hyper_trap(0x38,
 * 0) is the hypervisor's error code call.
 *
 * Library: stdlib/hw_io.c, stdlib_asm/hw_hyper.s45. Test:
 * test_hw_io_xemu.c (a scratch SD card image and D81).
 */

#ifndef HW_IO_H
#define HW_IO_H

/* SD Card Control ($D680 commands) */
typedef enum {
    SD_CMD_RESET = 0x00,
    SD_CMD_READ = 0x02,
    SD_CMD_WRITE = 0x03,
    SD_CMD_FLUSH = 0x0C,
} sd_command_t;

/* SD status ($D680 read) */
typedef enum {
    SD_BUSY = 0x03,         /* bits 0-1: the SD controller / card busy */
    SD_RESET = 0x04,
    SD_SDHC = 0x10,
    SD_ERROR = 0x60,        /* bits 5-6: the FSM error, an SD error */
} sd_status_t;

/* Ethernet Control ($D6E0, $D6E1 bits) */
typedef enum {
    ETH_RST = 0x01,         /* $D6E0: 0 holds the controller in reset */
    ETH_TXEN = 0x02,        /* $D6E0: 0 holds transmitting in reset */
    ETH_TXIDLE = 0x80,      /* $D6E0: a frame can be sent */
    ETH_RXIRQ = 0x20,       /* $D6E1: a frame received */
    ETH_TXDONE = 0x10,      /* $D6E1: a frame sent */
} eth_control_t;

/* Floppy Drive Control ($D081 commands) */
typedef enum {
    FDC_CMD_READ = 0x40,
    FDC_CMD_WRITE = 0x80,
    FDC_CMD_STEP_IN = 0x18,
    FDC_CMD_STEP_OUT = 0x10,
    FDC_CMD_RESTORE = 0x00,     /* (the head to track 0: no such command; seek) */
} fdc_command_t;

/* ===== SD Card API ===== */
int hw_sd_init(void);                       /* 1 when the card is ready, no error (the hypervisor set it up) */
int hw_sd_is_busy(void);
int hw_sd_has_error(void);
int hw_sd_is_sdhc(void);
int hw_sd_read_sector(unsigned long sector, void *buffer);
int hw_sd_write_sector(unsigned long sector, const void *buffer);
int hw_sd_flush(void);
void hw_sd_reset(void);                     /* the controller reset, the card's addressing (SDHC) kept */

/* ===== Ethernet API ===== */
int hw_eth_init(void);
void hw_eth_reset(void);
int hw_eth_tx_ready(void);
int hw_eth_rx_ready(void);                  /* a received frame waiting */
int hw_eth_send_packet(const void *data, int length);      /* 1 sent, 0 not */
int hw_eth_recv_packet(void *buffer, int max_length);      /* its length (0 none; cut to max_length) */
void hw_eth_get_mac(unsigned char mac[6]);

/* ===== Floppy Drive API ===== */
int hw_fdc_init(void);                      /* drive 0 selected, motor on: 1 */
int hw_fdc_is_busy(void);
int hw_fdc_read_sector(int track, int sector, void *buffer);       /* 1 done, 0 an error */
int hw_fdc_write_sector(int track, int sector, const void *buffer);
int hw_fdc_seek_track(int track);           /* CBM track 1-80 */
int hw_fdc_get_status(void);                /* $D082 */

/* ===== Real-Time Clock ===== */
typedef struct {
    int year;               /* 2000-2099 */
    int month;              /* 1-12 */
    int day;                /* 1-31 */
    int hour;
    int minute;
    int second;
} rtc_time_t;

int hw_rtc_read_time(rtc_time_t *time);
int hw_rtc_set_time(const rtc_time_t *time);

/* ===== Hypervisor Traps ===== */
__fastcall__ int hw_hyper_trap(int trap_num, unsigned int param);
__fastcall__ int hw_hyper_set_filename(const char *filename);
__fastcall__ int hw_hyper_load_file(unsigned long address);

#endif
