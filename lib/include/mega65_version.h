/* mega65_version.h — which MEGA65 the program runs on (#645)
 *
 * From the hardware's own registers:
 *   $D629        the model ID (UARTMISC:M65MODEL): mega65_revision_t
 *   $D630/$D631  the FPGA design's date, days since 2020-01-01
 *                (FPGA:FWDATEL/H)
 *   $D632-$D635  the FPGA firmware ID (its build's git commit), 4 bytes
 *
 * Usage:
 *   #include <mega65_version.h>
 *   if (mega65_get_revision() == MEGA65_R3) ...
 *   mega65_version_print();     MEGA65 R3, FPGA 2024-05-13, ID 1a2b3c4d
 *
 * Every MEGA65 core has the VIC-IV modes (H640, V400, full-colour text),
 * the F018B DMA, 40 MHz, the SD card and the floppy controller, so there is
 * nothing to ask about those. xemu reports model 3 (MEGA65 R3) unless set
 * otherwise (its -model option). Library: stdlib/mega65_version.c. Test:
 * test_mega65_version_xemu.c.
 */

#ifndef MEGA65_VERSION_H
#define MEGA65_VERSION_H

#include <stdint.h>

/* The model IDs ($D629) */
typedef enum {
    MEGA65_UNKNOWN          = 0x00,
    MEGA65_R1               = 0x01,   /* first prototype */
    MEGA65_R2               = 0x02,
    MEGA65_R3               = 0x03,   /* the DevKit and the first production (R3A) */
    MEGA65_R4               = 0x04,
    MEGA65_R5               = 0x05,
    MEGA65_R6               = 0x06,
    MEGA65_MEGAPHONE_R1     = 0x21,
    MEGA65_MEGAPHONE_R4     = 0x22,
    MEGA65_NEXYS4           = 0x40,
    MEGA65_NEXYS4DDR        = 0x41,
    MEGA65_NEXYS4DDR_WIDGET = 0x42,
    MEGA65_QMTECH_A100T     = 0x60,
    MEGA65_QMTECH_A200T     = 0x61,
    MEGA65_QMTECH_A325T     = 0x62,
    MEGA65_WUKONG           = 0xFD,
    MEGA65_SIMULATION       = 0xFE,
    MEGA65_EMULATOR         = 0xFF,   /* an emulator / other */
} mega65_revision_t;

typedef struct {
    uint8_t raw;                   /* the model ID ($D629) */
    uint8_t fpga_year;             /* 2-digit year (20xx) */
    uint8_t fpga_month;            /* 1-12 */
    uint8_t fpga_day;              /* 1-31 */
    mega65_revision_t revision;    /* the model */
    uint16_t fpga_days;            /* the date as days since 2020-01-01 */
    uint32_t fpga_id;              /* the firmware ID ($D632 the low byte) */
} mega65_version_info_t;

/* The model ID ($D629) */
uint8_t mega65_get_version_raw(void);

/* Everything above at once */
mega65_version_info_t mega65_get_version_info(void);

/* The model: the ID, or MEGA65_UNKNOWN for one this header does not name */
mega65_revision_t mega65_get_revision(void);

/* The FPGA design's date as YYYYMMDD (20240513) */
uint32_t mega65_get_fpga_date(void);

/* The FPGA firmware ID ($D632-$D635, $D632 the low byte) */
uint32_t mega65_get_fpga_id(void);

/* The model's name ("MEGA65 R3", "Nexys4DDR", ...; "unknown") */
const char *mega65_revision_name(mega65_revision_t rev);

/* "MEGA65 R3, FPGA 2024-05-13, ID 1a2b3c4d" and a newline, on stdout */
void mega65_version_print(void);

#endif
