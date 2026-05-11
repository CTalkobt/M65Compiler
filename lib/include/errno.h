/* errno.h — Error number definitions for cc45 / MEGA65
 *
 * errno is a global int at a fixed location, set by library functions
 * on error (strtol, malloc, etc.).
 *
 * _errnoc is the CBM/MEGA65 kernal status byte at $90 (ST).
 * It reflects the status of the most recent kernal I/O operation
 * (serial bus, tape, RS-232). Library routines that wrap kernal
 * calls may propagate _errnoc into errno.
 */

#pragma once

extern int errno;
extern char _errnoc;

/* Standard error codes */
#define ERANGE  1   /* Result out of range (strtol, strtoul overflow) */
#define ENOMEM  2   /* Not enough memory (malloc failure) */
#define EINVAL  3   /* Invalid argument */

/* TODO: Enable EDOM validation for future math functions (sqrt, etc.) */
#define EDOM    4   /* Math argument out of domain */
