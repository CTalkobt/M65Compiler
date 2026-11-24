/* mega65_version.c — the MEGA65's model, FPGA date and firmware ID
 * (<mega65_version.h>, #645): $D629, $D630/$D631, $D632-$D635 */

#include <mega65_version.h>
#include <stdio.h>

#define REG(a) (*(volatile unsigned char *)(a))

uint8_t mega65_get_version_raw(void) {
    return REG(0xD629);
}

mega65_revision_t mega65_get_revision(void) {
    unsigned char m = REG(0xD629);
    if ((m >= 0x01 && m <= 0x06) || m == 0x21 || m == 0x22 || (m >= 0x40 && m <= 0x42) ||
        (m >= 0x60 && m <= 0x62) || m >= 0xFD)
        return (mega65_revision_t)m;
    return MEGA65_UNKNOWN;
}

uint32_t mega65_get_fpga_id(void) {
    return (uint32_t)REG(0xD632) | ((uint32_t)REG(0xD633) << 8) | ((uint32_t)REG(0xD634) << 16) |
           ((uint32_t)REG(0xD635) << 24);
}

static const unsigned char mdays[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/* days since 2020-01-01 to a year (2020 + y), month, day */
static void mv_date(unsigned int days, unsigned char *y, unsigned char *m, unsigned char *d) {
    unsigned int year = 2020;
    unsigned char mon = 0;
    for (;;) {
        unsigned int len = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 366 : 365;
        if (days < len) break;
        days -= len;
        year++;
    }
    for (;;) {
        unsigned int len = mdays[mon] + (mon == 1 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
        if (days < len) break;
        days -= len;
        mon++;
    }
    *y = (unsigned char)(year - 2000);
    *m = (unsigned char)(mon + 1);
    *d = (unsigned char)(days + 1);
}

mega65_version_info_t mega65_get_version_info(void) {
    mega65_version_info_t v;
    v.raw = mega65_get_version_raw();
    v.revision = mega65_get_revision();
    v.fpga_days = (uint16_t)(REG(0xD630) | ((unsigned int)REG(0xD631) << 8));
    mv_date(v.fpga_days, &v.fpga_year, &v.fpga_month, &v.fpga_day);
    v.fpga_id = mega65_get_fpga_id();
    return v;
}

uint32_t mega65_get_fpga_date(void) {
    mega65_version_info_t v = mega65_get_version_info();
    return (2000UL + v.fpga_year) * 10000UL + (uint32_t)v.fpga_month * 100 + v.fpga_day;
}

const char *mega65_revision_name(mega65_revision_t rev) {
    switch (rev) {
    case MEGA65_R1: return "MEGA65 R1";
    case MEGA65_R2: return "MEGA65 R2";
    case MEGA65_R3: return "MEGA65 R3";
    case MEGA65_R4: return "MEGA65 R4";
    case MEGA65_R5: return "MEGA65 R5";
    case MEGA65_R6: return "MEGA65 R6";
    case MEGA65_MEGAPHONE_R1: return "MEGAphone R1";
    case MEGA65_MEGAPHONE_R4: return "MEGAphone R4";
    case MEGA65_NEXYS4: return "Nexys4";
    case MEGA65_NEXYS4DDR: return "Nexys4DDR";
    case MEGA65_NEXYS4DDR_WIDGET: return "Nexys4DDR widget";
    case MEGA65_QMTECH_A100T: return "QMtech A100T";
    case MEGA65_QMTECH_A200T: return "QMtech A200T";
    case MEGA65_QMTECH_A325T: return "QMtech A325T";
    case MEGA65_WUKONG: return "Wukong";
    case MEGA65_SIMULATION: return "simulation";
    case MEGA65_EMULATOR: return "emulator";
    default: return "unknown";
    }
}

void mega65_version_print(void) {
    mega65_version_info_t v = mega65_get_version_info();
    printf("%s, FPGA 20%02u-%02u-%02u, ID %08lx\n", mega65_revision_name(v.revision), v.fpga_year,
           v.fpga_month, v.fpga_day, (unsigned long)v.fpga_id);
}
