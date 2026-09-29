/*
 * MEGA65 Serial UART Interface
 *
 * Direct UART hardware access for serial communication.
 * Used primarily for test output via xemu -serialtcp.
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
