/* test_cc65_more_xemu.c — adler32, joystick.h, sleep, getopt and cc65's
 * cbm_k_ / cbm_load / cbm_save (test_xemu.sh gives it an empty D81 as
 * drive 8)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] adler32: "Wikipedia" ($11E60398), 1000 $FF bytes (the sums past
 *       65521), 3000 bytes in two pieces, NULL
 *   [1] joy_install / joy_count / joy_read (nothing held: 0) / the JOY_
 *       macros / joy_uninstall
 *   [2] sleep(1): one second of frames
 *   [3] getopt: grouped letters, attached and separate arguments, an
 *       unknown letter, "--", a missing argument (':' with a leading ':')
 *   [4] get_tv, waitvsync, cbm_k_getin from the KERNAL's keyboard buffer,
 *       cbm_k_iobase
 *   [5] cbm_save / cbm_load (to data and to the file's own address), a
 *       missing file (_oserror), cbm_k_setlfs / cbm_k_setnam / cbm_k_load
 *   [6] $AA sentinel
 */
#pragma cc45 signed_default

#include <zlib.h>
#include <input.h>
#include <joystick.h>
#include <unistd.h>
#include <cbm.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define REG(a) (*(volatile unsigned char *)(a))

static unsigned char big[3000];

static int t_adler(void) {
    static const unsigned char wiki[9] = { 0x57, 0x69, 0x6B, 0x69, 0x70, 0x65, 0x64, 0x69, 0x61 };
    unsigned long a = adler32(0, NULL, 0);
    unsigned int i;
    int ok = a == 1 && adler32(a, wiki, 9) == 0x11E60398UL;
    memset(big, 0xFF, 1000);
    ok = ok && adler32(1, big, 1000) == 0xE6E9E446UL;
    for (i = 0; i < 3000; i++) big[i] = (unsigned char)(i * 7);
    a = adler32(adler32(1, big, 1234), big + 1234, 3000 - 1234);
    return ok && a == 0xCA38D628UL && adler32(1, big, 3000) == a;
}

static int t_joy(void) {
    unsigned char v;
    int ok = joy_install(joy_static_stddrv) == JOY_ERR_OK && joy_count() == 2;
    v = joy_read(JOY_1);
    ok = ok && v == input_joy(1) && joy_read(JOY_2) == input_joy(2) && joy_read(5) == 0;
    ok = ok && JOY_UP(0x01) && !JOY_DOWN(0x01) && JOY_FIRE(0x10) && JOY_BTN_2(0x20) && !JOY_RIGHT(0x07);
    return ok && joy_uninstall() == JOY_ERR_OK && joy_load_driver(joy_stddrv) == JOY_ERR_OK;
}

static int t_sleep(void) {
    clock_t t0 = clock(), d;
    unsigned int r = sleep(1);
    d = clock() - t0;
    return r == 0 && d >= CLOCKS_PER_SEC && d <= CLOCKS_PER_SEC + 2;
}

static char a0[] = "prog", a1[] = "-ab", a2[] = "-cfoo", a3[] = "-d", a4[] = "bar", a5[] = "-x", a6[] = "--",
            a7[] = "rest", b1[] = "-c";

static int t_getopt(void) {
    char *argv[] = { a0, a1, a2, a3, a4, a5, a6, a7, NULL };
    char *argv2[] = { a0, b1, NULL };
    const char *opts = "abc:d:";
    int ok;
    opterr = 0;
    ok = getopt(8, argv, opts) == 'a' && optind == 1;
    ok = ok && getopt(8, argv, opts) == 'b' && optind == 2;
    ok = ok && getopt(8, argv, opts) == 'c' && strcmp(optarg, "foo") == 0 && optind == 3;
    ok = ok && getopt(8, argv, opts) == 'd' && strcmp(optarg, "bar") == 0 && optind == 5;
    ok = ok && getopt(8, argv, opts) == '?' && optopt == 'x';
    ok = ok && getopt(8, argv, opts) == -1 && optind == 7 && strcmp(argv[optind], "rest") == 0;
    optind = 1;
    ok = ok && getopt(2, argv2, ":c:") == ':' && optopt == 'c';
    optind = 1;
    ok = ok && getopt(2, argv2, "c:") == '?' && getopt(2, argv2, "c:") == -1;
    return ok;
}

static int t_kernal(void) {
    unsigned char tv = get_tv();
    int ok = (tv == TV_PAL) == !(REG(0xD06F) & 0x80);
    waitvsync();
    REG(0x02B0) = 0x41;
    REG(0x00D0) = 1;
    ok = ok && cbm_k_getin() == 0x41 && cbm_k_getin() == 0;
    return ok && cbm_k_iobase() >= 0xD000;
}

static unsigned char data[100], back[100];

static int t_disk(void) {
    unsigned int i, end;
    int ok;
    for (i = 0; i < 100; i++) data[i] = (unsigned char)(i * 3 + 1);
    ok = cbm_save("testsave", 8, data, 100) == 0 && _oserror == 0;
    ok = ok && cbm_load("testsave", 8, back) == 100 && memcmp(data, back, 100) == 0;
    memset(data, 0, 100);
    ok = ok && cbm_load("testsave", 8, NULL) == (unsigned int)data + 100 && data[99] == (unsigned char)(99 * 3 + 1);
    ok = ok && cbm_load("nofile", 8, back) == 0 && _oserror != 0;
    cbm_k_setlfs(0, 8, 0);
    cbm_k_setnam("testsave");
    end = cbm_k_load(0, (unsigned int)back);
    return ok && end == (unsigned int)back + 100;
}

int main(void) {
    OUT[0] = t_adler();
    OUT[1] = t_joy();
    OUT[2] = t_sleep();
    OUT[3] = t_getopt();
    OUT[4] = t_kernal();
    OUT[5] = t_disk();
    OUT[6] = 0xAA;
    return 0;
}
