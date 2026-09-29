/* mega65_version.c — MEGA65 version detection implementation */

#include <mega65_version.h>
#include <mega65.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Version Detection Tables
 * ========================================================================== */

/* FPGA version code to revision mapping */
typedef struct {
    uint8_t fpga_year;
    uint8_t fpga_month;
    mega65_revision_t revision;
    uint16_t features;
} version_entry_t;

static const version_entry_t version_table[] = {
    /* Year, Month, Revision, Features */
    { 16,  5, MEGA65_R1, FEATURE_DMA | FEATURE_ETHERNET | FEATURE_SDCARD },
    { 16,  8, MEGA65_R2, FEATURE_DMA | FEATURE_FAST | FEATURE_ETHERNET | FEATURE_SDCARD },
    { 17,  6, MEGA65_R3, FEATURE_H640 | FEATURE_FCM | FEATURE_DMA | FEATURE_FAST | FEATURE_ETHERNET | FEATURE_SDCARD },
    { 18,  3, MEGA65_R4, FEATURE_H640 | FEATURE_V400 | FEATURE_FCM | FEATURE_BITPLANE | FEATURE_DMA | FEATURE_FAST | FEATURE_ETHERNET | FEATURE_SDCARD | FEATURE_HYPERVISOR | FEATURE_FDC },
    { 19,  0, MEGA65_R5, FEATURE_H640 | FEATURE_V400 | FEATURE_FCM | FEATURE_BITPLANE | FEATURE_DMA | FEATURE_FAST | FEATURE_ETHERNET | FEATURE_SDCARD | FEATURE_HYPERVISOR | FEATURE_FDC },
};

static const size_t version_table_size = sizeof(version_table) / sizeof(version_table[0]);

/* ============================================================================
 * Helper Functions
 * ========================================================================== */

static mega65_revision_t detect_revision(uint8_t fpga_year, uint8_t fpga_month) {
    int i;
    for (i = 0; i < version_table_size; ++i) {
        if (version_table[i].fpga_year == fpga_year &&
            version_table[i].fpga_month == fpga_month) {
            return version_table[i].revision;
        }
    }
    return MEGA65_UNKNOWN;
}

static uint16_t detect_features(uint8_t fpga_year, uint8_t fpga_month) {
    int i;
    for (i = 0; i < version_table_size; ++i) {
        if (version_table[i].fpga_year == fpga_year &&
            version_table[i].fpga_month == fpga_month) {
            return version_table[i].features;
        }
    }
    return 0;
}

/* ============================================================================
 * Public Functions - Version Reading
 * ========================================================================== */

uint8_t mega65_get_version_raw(void) {
    return vic4->version;
}

mega65_version_info_t mega65_get_version_info(void) {
    mega65_version_info_t info;
    uint8_t raw = mega65_get_version_raw();

    info.raw = raw;

    /* Extract year and month from raw register
     * Format: bits 7:6 = reserved, bits 5:0 = year/month encoded
     * Typical format: upper 6 bits encode date
     */
    info.fpga_year = 16 + ((raw >> 4) & 0x0F);  /* Years 2016+ */
    info.fpga_month = ((raw & 0x0F) * 3);       /* Approximate month spacing */
    info.fpga_day = 1;                          /* Day not encoded, default to 1st */

    info.revision = detect_revision(info.fpga_year, info.fpga_month);
    info.feature_flags = detect_features(info.fpga_year, info.fpga_month);

    return info;
}

mega65_revision_t mega65_get_revision(void) {
    mega65_version_info_t info = mega65_get_version_info();
    return info.revision;
}

uint32_t mega65_get_fpga_date(void) {
    mega65_version_info_t info = mega65_get_version_info();
    uint32_t date = 0;

    if (info.fpga_year > 0 && info.fpga_month > 0) {
        date = (2000 + info.fpga_year) * 10000 + info.fpga_month * 100 + info.fpga_day;
    }

    return date;
}

/* ============================================================================
 * Public Functions - Feature Detection
 * ========================================================================== */

int mega65_has_feature(mega65_feature_t feature) {
    mega65_version_info_t info = mega65_get_version_info();
    return (info.feature_flags & feature) != 0;
}

/* ============================================================================
 * Public Functions - String Conversion
 * ========================================================================== */

const char* mega65_revision_name(mega65_revision_t rev) {
    switch (rev) {
        case MEGA65_R1: return "MEGA65 R1 (Prototype)";
        case MEGA65_R2: return "MEGA65 R2 (Early Release)";
        case MEGA65_R3: return "MEGA65 R3 (Mid-Production)";
        case MEGA65_R4: return "MEGA65 R4 (Current)";
        case MEGA65_R5: return "MEGA65 R5 (Future)";
        default:        return "MEGA65 (Unknown Revision)";
    }
}

const char* mega65_feature_name(mega65_feature_t feature) {
    switch (feature) {
        case FEATURE_H640:       return "H640 (640-pixel mode)";
        case FEATURE_V400:       return "V400 (400-line mode)";
        case FEATURE_FCM:        return "FCM (Full-color text)";
        case FEATURE_BITPLANE:   return "Bitplane graphics";
        case FEATURE_DMA:        return "DMA controller";
        case FEATURE_FAST:       return "40 MHz CPU";
        case FEATURE_ETHERNET:   return "Ethernet";
        case FEATURE_SDCARD:     return "SD card";
        case FEATURE_HYPERVISOR: return "Hypervisor";
        case FEATURE_FDC:        return "Floppy controller";
        default:                 return "Unknown feature";
    }
}

void mega65_features_to_string(char* buffer, size_t buffer_size) {
    mega65_version_info_t info = mega65_get_version_info();
    int offset = 0;

    if (buffer_size == 0) return;

    buffer[0] = '\0';

    if (info.feature_flags & FEATURE_H640)       { offset += snprintf(buffer + offset, buffer_size - offset, "%sH640", offset ? ", " : ""); }
    if (info.feature_flags & FEATURE_V400)       { offset += snprintf(buffer + offset, buffer_size - offset, "%sV400", offset ? ", " : ""); }
    if (info.feature_flags & FEATURE_FCM)        { offset += snprintf(buffer + offset, buffer_size - offset, "%sFCM", offset ? ", " : ""); }
    if (info.feature_flags & FEATURE_BITPLANE)   { offset += snprintf(buffer + offset, buffer_size - offset, "%sBitplane", offset ? ", " : ""); }
    if (info.feature_flags & FEATURE_DMA)        { offset += snprintf(buffer + offset, buffer_size - offset, "%sDMA", offset ? ", " : ""); }
    if (info.feature_flags & FEATURE_FAST)       { offset += snprintf(buffer + offset, buffer_size - offset, "%s40MHz", offset ? ", " : ""); }
    if (info.feature_flags & FEATURE_ETHERNET)   { offset += snprintf(buffer + offset, buffer_size - offset, "%sEthernet", offset ? ", " : ""); }
    if (info.feature_flags & FEATURE_SDCARD)     { offset += snprintf(buffer + offset, buffer_size - offset, "%sSD", offset ? ", " : ""); }
    if (info.feature_flags & FEATURE_HYPERVISOR) { offset += snprintf(buffer + offset, buffer_size - offset, "%sHypervisor", offset ? ", " : ""); }
    if (info.feature_flags & FEATURE_FDC)        { offset += snprintf(buffer + offset, buffer_size - offset, "%sFDC", offset ? ", " : ""); }

    if (offset == 0) {
        snprintf(buffer, buffer_size, "(no features detected)");
    }
}

void mega65_version_print(void) {
    mega65_version_info_t info = mega65_get_version_info();
    char features_buffer[256];

    printf("MEGA65 Version Information:\n");
    printf("  Raw Version Register: 0x%02X\n", info.raw);
    printf("  FPGA Date: 20%02d-%02d-%02d\n", info.fpga_year, info.fpga_month, info.fpga_day);
    printf("  Revision: %s\n", mega65_revision_name(info.revision));

    mega65_features_to_string(features_buffer, sizeof(features_buffer));
    printf("  Features: %s\n", features_buffer);
}
