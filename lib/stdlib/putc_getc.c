/* putc_getc.c — putc and getc as functions (C11 7.21.7.5, 7.21.7.7):
 * <stdio.h> also defines them as macros for fputc / fgetc, so these are
 * linked only for their address or after #undef putc / getc (#750) */

#include <stdio.h>

#undef putc
#undef getc

int putc(int c, FILE *stream) {
    return fputc(c, stream);
}

int getc(FILE *stream) {
    return fgetc(stream);
}
