/* serial.h — cc65's serial API on the MEGA65's buffered UART
 * (lib/stdlib_asm/serial.s45)
 *
 * The UART at $D0E0 (xemu connects it to a TCP socket with
 * -serialtcp HOST:PORT). 8 data bits, 1 stop bit, no parity and no
 * handshake are its only frame: ser_open refuses others with
 * SER_ERR_INIT_FAILED. Every SER_BAUD_ rate is there (80 MHz / (divisor +
 * 1)); ser_ioctl(SER_IOCTL_DIVISOR, &divisor) sets any other. The driver
 * is built in: ser_install(ser_static_stddrv) or ser_load_driver(any name)
 * installs it. Bytes wait in the UART's own buffers: ser_get gives
 * SER_ERR_NO_DATA when none came, ser_put SER_ERR_OVERFLOW while the
 * transmit buffer is full. All __fastcall__.
 */
#pragma once

#define SER_BAUD_45_5           0x00
#define SER_BAUD_50             0x01
#define SER_BAUD_75             0x02
#define SER_BAUD_110            0x03
#define SER_BAUD_134_5          0x04
#define SER_BAUD_150            0x05
#define SER_BAUD_300            0x06
#define SER_BAUD_600            0x07
#define SER_BAUD_1200           0x08
#define SER_BAUD_1800           0x09
#define SER_BAUD_2400           0x0A
#define SER_BAUD_3600           0x0B
#define SER_BAUD_4800           0x0C
#define SER_BAUD_7200           0x0D
#define SER_BAUD_9600           0x0E
#define SER_BAUD_19200          0x0F
#define SER_BAUD_38400          0x10
#define SER_BAUD_57600          0x11
#define SER_BAUD_115200         0x12
#define SER_BAUD_230400         0x13
#define SER_BAUD_31250          0x14
#define SER_BAUD_62500          0x15
#define SER_BAUD_56_875         0x16

#define SER_BITS_5              0x00
#define SER_BITS_6              0x01
#define SER_BITS_7              0x02
#define SER_BITS_8              0x03

#define SER_STOP_1              0x00
#define SER_STOP_2              0x01

#define SER_PAR_NONE            0x00
#define SER_PAR_ODD             0x01
#define SER_PAR_EVEN            0x02
#define SER_PAR_MARK            0x03
#define SER_PAR_SPACE           0x04

#define SER_HS_NONE             0x00
#define SER_HS_HW               0x01
#define SER_HS_SW               0x02

/* ser_status's bits (6551's); none is ever set here */
#define SER_STATUS_PE           0x01
#define SER_STATUS_FE           0x02
#define SER_STATUS_OE           0x04
#define SER_STATUS_DCD          0x20
#define SER_STATUS_DSR          0x40

#define SER_ERR_OK              0x00
#define SER_ERR_NO_DRIVER       0x01
#define SER_ERR_CANNOT_LOAD     0x02
#define SER_ERR_INV_DRIVER      0x03
#define SER_ERR_NO_DEVICE       0x04
#define SER_ERR_BAUD_UNAVAIL    0x05
#define SER_ERR_NO_DATA         0x06
#define SER_ERR_OVERFLOW        0x07
#define SER_ERR_INIT_FAILED     0x08
#define SER_ERR_INV_IOCTL       0x09
#define SER_ERR_INSTALLED       0x0A
#define SER_ERR_NOT_OPEN        0x0B

/* ser_ioctl: the bit rate's divisor from an unsigned long */
#define SER_IOCTL_DIVISOR       0x00

struct ser_params {
    unsigned char baudrate;
    unsigned char databits;
    unsigned char stopbits;
    unsigned char parity;
    unsigned char handshake;
};

extern const char ser_stddrv[];
extern const unsigned char ser_static_stddrv[];

__fastcall__ unsigned char ser_load_driver(const char *driver);
__fastcall__ unsigned char ser_unload(void);
__fastcall__ unsigned char ser_install(const void *driver);
__fastcall__ unsigned char ser_uninstall(void);
__fastcall__ unsigned char ser_open(const struct ser_params *params);
__fastcall__ unsigned char ser_close(void);
__fastcall__ unsigned char ser_get(char *b);
__fastcall__ unsigned char ser_put(char b);
__fastcall__ unsigned char ser_status(unsigned char *status);
__fastcall__ unsigned char ser_ioctl(unsigned char code, void *data);
