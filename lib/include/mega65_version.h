/* mega65_version.h — MEGA65 FPGA version detection and feature discovery
 *
 * Identifies MEGA65 revision and available hardware features based on
 * the FPGA version register ($D07C).
 *
 * Usage:
 *   #include <mega65_version.h>
 *   mega65_revision_t rev = mega65_get_revision();
 *   if (mega65_has_feature(FEATURE_H640)) {
 *       // 640-pixel mode available
 *   }
 */

#ifndef MEGA65_VERSION_H
#define MEGA65_VERSION_H

#include <stdint.h>

/* ============================================================================
 * MEGA65 Revision Enumeration
 * ========================================================================== */

typedef enum {
    MEGA65_UNKNOWN = 0,
    MEGA65_R1      = 1,   /* First prototype */
    MEGA65_R2      = 2,   /* Early release */
    MEGA65_R3      = 3,   /* Mid-production */
    MEGA65_R4      = 4,   /* Current standard */
    MEGA65_R5      = 5,   /* Future revision */
} mega65_revision_t;

/* ============================================================================
 * Hardware Feature Flags
 * ========================================================================== */

typedef enum {
    FEATURE_H640        = 0x01,   /* 640-pixel horizontal mode */
    FEATURE_V400        = 0x02,   /* 400-line vertical mode */
    FEATURE_FCM         = 0x04,   /* Full-color text mode */
    FEATURE_BITPLANE    = 0x08,   /* Bitplane graphics mode */
    FEATURE_DMA         = 0x10,   /* F018B DMA controller */
    FEATURE_FAST        = 0x20,   /* 40 MHz CPU speed */
    FEATURE_ETHERNET    = 0x40,   /* Ethernet interface */
    FEATURE_SDCARD      = 0x80,   /* SD card interface */
    FEATURE_HYPERVISOR  = 0x100,  /* Hypervisor mode */
    FEATURE_FDC         = 0x200,  /* Floppy disk controller */
} mega65_feature_t;

/* ============================================================================
 * Version Structure
 * ========================================================================== */

typedef struct {
    uint8_t raw;                   /* Raw $D07C register value */
    uint8_t fpga_year;             /* 2-digit year (20xx) */
    uint8_t fpga_month;            /* Month (01-12) */
    uint8_t fpga_day;              /* Day (01-31) */
    mega65_revision_t revision;    /* Detected MEGA65 revision */
    uint16_t feature_flags;        /* Available hardware features */
} mega65_version_info_t;

/* ============================================================================
 * Public Functions
 * ========================================================================== */

/* Read raw version register ($D07C) */
uint8_t mega65_get_version_raw(void);

/* Get complete version information */
mega65_version_info_t mega65_get_version_info(void);

/* Get detected MEGA65 revision */
mega65_revision_t mega65_get_revision(void);

/* Extract FPGA build date (returns YYYYMMDD as uint32_t, or 0 if unknown) */
uint32_t mega65_get_fpga_date(void);

/* Check if a feature is available */
int mega65_has_feature(mega65_feature_t feature);

/* Get human-readable revision name */
const char* mega65_revision_name(mega65_revision_t rev);

/* Get feature name as string */
const char* mega65_feature_name(mega65_feature_t feature);

/* Get all available features as human-readable string */
void mega65_features_to_string(char* buffer, unsigned int buffer_size);

/* Print version information to stdout (debug) */
void mega65_version_print(void);

#endif
