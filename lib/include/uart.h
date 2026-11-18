/* FOR THE FUTURE: NOT IMPLEMENTED.
 *
 * No library defines what this header declares: a call of one of its
 * functions would compile and then fail to link. Its contents are kept for
 * later work and are left out unless CC45_FUTURE is defined (-DCC45_FUTURE),
 * so including it by accident declares nothing.
 * (doc/architecture/stdlib.md, "Headers Without a Library Implementation")
 */
#ifdef CC45_FUTURE

/*
 * MEGA65 Serial UART Interface
 *
 * Direct UART hardware access for serial communication.
 * Used primarily for test output via xemu -serialtcp.
 *
 * No library implements these: <serial.h> is the library's serial port
 * (cc65's API on the buffered UART at $D0E0, which xemu -serialtcp uses).
 */

#ifndef _UART_H
#define _UART_H

#include <stddef.h>

/* MEGA65 UART Hardware Registers */
#define UART_DATA_REG     0xD600  /* Data register (read/write) */
#define UART_STATUS_REG   0xD601  /* Status register */
#define UART_RXRDY        0x01    /* Receive ready flag (bit 0) */
#define UART_TXRDY        0x02    /* Transmit ready flag (bit 1) */

/* Write a single character to UART, waiting for TX ready */
void uart_putchar(unsigned char c);

/* Write a string to UART (null-terminated) */
void uart_puts(const char *str);

/* Read a single character from UART, waiting for RX ready */
unsigned char uart_getchar(void);

/* Check if character is available to read (non-blocking) */
unsigned char uart_rx_ready(void);

/* Check if UART is ready for transmission (non-blocking) */
unsigned char uart_tx_ready(void);

#endif

#endif /* CC45_FUTURE */
