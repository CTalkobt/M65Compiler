/* test_serial_xemu.c — <serial.h> on the buffered UART ($D0E0)
 *
 * test_xemu.sh runs it with xemu's -serialtcp connected to
 * src/test/serial_echo.py, which sends back each byte XOR $55.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] the driver: NO_DRIVER / NOT_OPEN before install, INSTALLED twice,
 *       INIT_FAILED for another frame, BAUD_UNAVAIL
 *   [1] ser_open at 115200: the divisor (80 MHz / 115200 - 1 = 693) at
 *       $D0E4-$D0E6
 *   [2] five bytes sent, the five echoed back (waiting up to 3 seconds of
 *       the RTC, the host's time in xemu)
 *   [3] NO_DATA when nothing came; ser_status; ser_ioctl's divisor, an
 *       unknown ioctl
 *   [4] close, NOT_OPEN, uninstall, ser_load_driver
 *   [5] $AA sentinel
 */
#include <serial.h>
#include <time.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define REG(a) (*(volatile unsigned char *)(a))

static const unsigned char sent[5] = { 0x41, 0x42, 0x00, 0xFF, 0x0D };

static int t_driver(void) {
    struct ser_params p = { SER_BAUD_9600, SER_BITS_8, SER_STOP_1, SER_PAR_NONE, SER_HS_NONE };
    char c;
    int ok = ser_open(&p) == SER_ERR_NO_DRIVER && ser_get(&c) == SER_ERR_NOT_OPEN;
    ok = ok && ser_install(ser_static_stddrv) == SER_ERR_OK && ser_install(ser_static_stddrv) == SER_ERR_INSTALLED;
    p.databits = SER_BITS_7;
    ok = ok && ser_open(&p) == SER_ERR_INIT_FAILED;
    p.databits = SER_BITS_8;
    p.handshake = SER_HS_HW;
    ok = ok && ser_open(&p) == SER_ERR_INIT_FAILED;
    p.handshake = SER_HS_NONE;
    p.baudrate = 0x17;
    return ok && ser_open(&p) == SER_ERR_BAUD_UNAVAIL;
}

static int t_open(void) {
    struct ser_params p = { SER_BAUD_115200, SER_BITS_8, SER_STOP_1, SER_PAR_NONE, SER_HS_NONE };
    return ser_open(&p) == SER_ERR_OK && REG(0xD0E4) == (693 & 0xFF) && REG(0xD0E5) == (693 >> 8) &&
           REG(0xD0E6) == 0;
}

static int t_echo(void) {
    unsigned char i, n = 0;
    char c;
    time_t start;
    for (i = 0; i < 5; i++)
        if (ser_put((char)sent[i]) != SER_ERR_OK) return 0;
    start = time(NULL);
    while (n < 5 && time(NULL) - start < 3) {
        if (ser_get(&c) == SER_ERR_OK) {
            if ((unsigned char)c != (sent[n] ^ 0x55)) return 0;
            n++;
        }
    }
    return n == 5;
}

static int t_misc(void) {
    char c;
    unsigned char st = 0xEE;
    unsigned long div = 10;
    int ok = ser_get(&c) == SER_ERR_NO_DATA && ser_status(&st) == SER_ERR_OK && st == 0;
    ok = ok && ser_ioctl(SER_IOCTL_DIVISOR, &div) == SER_ERR_OK && REG(0xD0E4) == 10 && REG(0xD0E5) == 0;
    return ok && ser_ioctl(5, &div) == SER_ERR_INV_IOCTL;
}

static int t_close(void) {
    int ok = ser_close() == SER_ERR_OK && ser_close() == SER_ERR_NOT_OPEN && ser_put('x') == SER_ERR_NOT_OPEN;
    ok = ok && ser_uninstall() == SER_ERR_OK && ser_uninstall() == SER_ERR_NO_DRIVER;
    return ok && ser_load_driver(ser_stddrv) == SER_ERR_OK && ser_unload() == SER_ERR_OK;
}

int main(void) {
    OUT[0] = t_driver();
    OUT[1] = t_open();
    OUT[2] = t_echo();
    OUT[3] = t_misc();
    OUT[4] = t_close();
    OUT[5] = 0xAA;
    return 0;
}
