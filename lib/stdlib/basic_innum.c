/* basic_innum.c — numbers for INPUT and READ of BASIC 65 programs compiled by
 * basic45 --compile (#369, #484)
 *
 *   _bas_iitemn: INPUT's next item (_bas_iitem's) for a number variable into
 *     __float_a; not a number (or more after one): ?REDO FROM START,
 *     __bas_n := 1 (the INPUT starts again).
 *   _bas_readn: READ's next DATA item (_bas_read's) for a number variable into
 *     __float_a; not a number: ?TYPE MISMATCH.
 * The item's text (no quotes, trailing spaces cut) is read by strtof, which is
 * linked only by a program reading numbers.
 */
#include <stdlib.h>

extern unsigned char *_bas_sr;
extern int _bas_n, _bas_m;
extern unsigned char _float_a[5];
extern void _bas_error(void);
extern void _bas_iitem(void);
extern void _bas_read(void);
extern void _bas_iredo(void);

static unsigned char buf[82];        /* (basic_in.c's NUMLEN 80 characters) */

/* the text in buf into __float_a: 0 when it is not all of a number */
static unsigned char num(void) {
    char *end;
    float f;
    unsigned char k;
    buf[buf[0] + 1] = 0;
    f = strtof((char *)buf + 1, &end);
    if (*end) return 0;
    for (k = 0; k < 5; k++) _float_a[k] = ((unsigned char *)&f)[k];
    return 1;
}

void _bas_iitemn(void) {
    _bas_sr = buf;
    _bas_m = 3;
    _bas_iitem();
    if (!num()) _bas_iredo();
}

void _bas_readn(void) {
    _bas_sr = buf;
    _bas_m = 3;
    _bas_read();
    if (!num()) {
        _bas_n = 22;
        _bas_error();
    }
}
