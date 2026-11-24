/* test_mega65_version_xemu.c — <mega65_version.h> (#645): the model ID
 * ($D629), the FPGA date ($D630/$D631, days since 2020-01-01) and the
 * firmware ID ($D632-$D635) as the hardware gives them (xemu: model 3, its
 * own build date, "Xemu")
 *
 * Results at $C0C0 (01 = pass):
 *   [0] mega65_get_version_raw is $D629; the revision is that model (3:
 *       MEGA65_R3) and its name
 *   [1] the date: the info's year / month / day and mega65_get_fpga_date
 *       as gmtime gives the same day; the days as the registers
 *   [2] the firmware ID ($D632 the low byte)
 *   [3] mega65_revision_name for the others; an ID the header does not
 *       name: "unknown"
 *   [4] mega65_version_print's line
 *   [5] $AA sentinel
 */
#include <mega65_version.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define REG(a) (*(volatile unsigned char *)(a))

int main(void) {
    mega65_version_info_t v;
    unsigned int days;
    time_t t;
    struct tm *tm;
    char want[48];
    int ok;
    OUT[5] = 0;

    v = mega65_get_version_info();
    ok = mega65_get_version_raw() == REG(0xD629) && v.raw == REG(0xD629) && v.revision == mega65_get_revision();
    ok = ok && v.raw == 3 && v.revision == MEGA65_R3 && strcmp(mega65_revision_name(v.revision), "MEGA65 R3") == 0;
    OUT[0] = ok;

    days = REG(0xD630) | ((unsigned int)REG(0xD631) << 8);
    /* (the library's time_t counts from 2000-01-01: 2020-01-01 is 631152000) */
    t = 631152000L + (time_t)days * 86400L;
    tm = gmtime(&t);
    ok = v.fpga_days == days && v.fpga_year == (unsigned char)(tm->tm_year - 100) &&
         v.fpga_month == (unsigned char)(tm->tm_mon + 1) && v.fpga_day == (unsigned char)tm->tm_mday;
    ok = ok && mega65_get_fpga_date() ==
                   (unsigned long)(tm->tm_year + 1900) * 10000UL + (unsigned long)(tm->tm_mon + 1) * 100 + tm->tm_mday;
    ok = ok && days > 1000;              /* (2022 or later) */
    OUT[1] = ok;

    /* 'X' 'e' 'm' 'u' as ASCII ($58 $65 $6D $75) */
    ok = mega65_get_fpga_id() == 0x756D6558UL && v.fpga_id == 0x756D6558UL;
    OUT[2] = ok;

    ok = strcmp(mega65_revision_name(MEGA65_R6), "MEGA65 R6") == 0 &&
         strcmp(mega65_revision_name(MEGA65_NEXYS4DDR), "Nexys4DDR") == 0 &&
         strcmp(mega65_revision_name(MEGA65_EMULATOR), "emulator") == 0 &&
         strcmp(mega65_revision_name((mega65_revision_t)0x30), "unknown") == 0 &&
         strcmp(mega65_revision_name(MEGA65_UNKNOWN), "unknown") == 0;
    OUT[3] = ok;

    /* the printed line, from the screen's first row (screen codes) */
    printf("\x93");                      /* (clear the screen, cursor home) */
    mega65_version_print();
    sprintf(want, "MEGA65 R3, FPGA 20%02u-%02u-%02u, ID 756d6558", v.fpga_year, v.fpga_month, v.fpga_day);
    {
        unsigned char *scr = (unsigned char *)0x0800;
        unsigned char i, c;
        ok = 1;
        for (i = 0; want[i]; i++) {
            c = (unsigned char)want[i];
            /* PETSCII to screen code: letters $41-$5A -> $01-$1A, $C1-$DA -> $41-$5A */
            if (c >= 0x40 && c < 0x60) c -= 0x40;
            else if (c >= 0xC0 && c < 0xE0) c -= 0x80;
            if (scr[i] != c) ok = 0;
        }
    }
    OUT[4] = ok;
    OUT[5] = 0xAA;
    return 0;
}
