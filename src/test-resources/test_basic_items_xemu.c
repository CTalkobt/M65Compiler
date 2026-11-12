/* test_basic_items_xemu.c — basic45 --compile's INPUT and READ items in
 * assembly (basic_items.s45, #369) against their former C (basic_in.c's
 * _bas_iitem, _bas_read, _bas_restore, copied here as the reference): the
 * same items, positions, errors and statement changes.
 *
 * Results at $C0C0: [0] INPUT's differences, [1] READ's (0 = pass), [2] $AA
 */
#include <setjmp.h>
#include <string.h>

extern unsigned char *_bas_sr;
extern int _bas_n, _bas_m;
extern unsigned char _bas_ibuf[161];
extern unsigned char _bas_ipos;
extern void _bas_iitem(void), _bas_read(void), _bas_restore(void);

#define OUT ((volatile unsigned char *)0xC0C0)
#define NUMLEN 80

/* DATA: a length byte and the statement's text each, $FF after the last */
unsigned char _bas_data[] = {
    5, '1', ',', '2', ',', '3',
    12, ' ', '"', 'A', ',', 'B', '"', ' ', ',', 'X', 'Y', ' ', ' ',
    0,
    9, '"', 'Q', '"', 'Z', ',', '4', '.', '5', ' ',
    3, ',', ',', '7',
    0xFF,
};

static jmp_buf back;
static int raised;
void _bas_error(void) { raised = _bas_n; longjmp(back, 1); }
/* INPUT# (a channel): ?REDO FROM START is ?FILE DATA, an error caught here;
 * no case reads another line (_bas_morein: the keyboard) */
extern unsigned char _bas_chan;
extern void _bas_iredo(void), _bas_morein(void);
static unsigned char redone, asked;

/* --- the reference: basic_in.c as it was --- */
#define line _bas_ibuf
#define pos _bas_ipos
static void ref_raise(int n) { _bas_n = n; _bas_error(); }
static void skipsp(void) { while (line[pos] == ' ') pos++; }
static void ref_iitem(void) {
    unsigned char c, start, n;
    if (_bas_m & 4) {
        unsigned char *r = _bas_sr;
        if (!line[pos]) _bas_morein();
        n = 0;
        while (line[pos]) r[++n] = line[pos++];
        r[0] = n;
        _bas_n = 0;
        return;
    }
    skipsp();
    if (line[pos] == 0 || line[pos] == ':') { _bas_morein(); skipsp(); }
    {
        unsigned char *r = _bas_sr;
        if (!(_bas_m & 2) && line[pos] == '"') {
            start = ++pos;
            while (line[pos] && line[pos] != '"') pos++;
            n = pos - start;
            if (line[pos] == '"') pos++;
            skipsp();
            c = line[pos];
            if (c && c != ',' && c != ':') goto redo;
        } else {
            start = pos;
            while (line[pos] && line[pos] != ',' && line[pos] != ':') pos++;
            n = pos - start;
            if (_bas_m & 2) {
                while (n && line[start + n - 1] == ' ') n--;
                if (n > NUMLEN) n = NUMLEN;
            }
        }
        r[0] = n;
        for (c = 0; c < n; c++) r[c + 1] = line[start + c];
    }
    if (line[pos] == ',') pos++;
    _bas_n = 0;
    return;
redo:
    _bas_iredo();
}
#undef line
#undef pos
static unsigned char *drec;
static unsigned char dpos, dneed = 1;
static void ref_restore(void) { drec = _bas_data + _bas_n; dneed = 2; }
static void ref_read(void) {
    unsigned char *t, len, start, n, c;
    if (!drec) { drec = _bas_data; dneed = 2; }
    if (dneed) {
        if (dneed == 1 && drec[0] != 0xFF) drec += drec[0] + 1;
        if (drec[0] == 0xFF) ref_raise(13);
        dpos = 0;
        dneed = 0;
    }
    len = drec[0];
    t = drec + 1;
    while (dpos < len && t[dpos] == ' ') dpos++;
    {
        unsigned char *r = _bas_sr;
        if (!(_bas_m & 2) && dpos < len && t[dpos] == '"') {
            start = ++dpos;
            while (dpos < len && t[dpos] != '"') dpos++;
            n = dpos - start;
            if (dpos < len) dpos++;
            while (dpos < len && t[dpos] == ' ') dpos++;
            if (dpos < len && t[dpos] != ',') ref_raise(22);
        } else {
            start = dpos;
            while (dpos < len && t[dpos] != ',') dpos++;
            n = dpos - start;
            if (_bas_m & 2) {
                while (n && t[start + n - 1] == ' ') n--;
                if (n > NUMLEN) n = NUMLEN;
            }
        }
        r[0] = n;
        for (c = 0; c < n; c++) r[c + 1] = t[start + c];
    }
    if (dpos < len) dpos++;
    else dneed = 1;
}

/* one call's outcome */
struct out { int err, n; unsigned char pos, redone, asked, s[90]; };
static unsigned char rbuf[90];
static void run(void (*f)(void), struct out *o) {
    memset(rbuf, 0xEE, sizeof rbuf);
    raised = 0; redone = 0; asked = 0;
    if (!setjmp(back)) f();
    o->err = raised; o->n = _bas_n; o->pos = _bas_ipos; o->redone = redone; o->asked = asked;
    memcpy(o->s, rbuf, sizeof rbuf);
}

/* INPUT lines: their items read (k calls, m each) both ways */
static const char *const lines[] = {
    "A,B,C", "  12 , 34", "\"HI, THERE\",X", "\"Q\"  ,Z", "\"BAD\"X", "\"OPEN", "1:2", "  \"S\" :T", "X  ,  Y  ",
    "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901,5",
};
static const unsigned char calls[] = {3, 2, 2, 2, 1, 1, 1, 1, 2, 2};

int main(void) {
    unsigned char i, k, m, bad[2] = {0, 0};
    static const unsigned char modes[] = {0, 2, 4};
    struct out x, y;
    _bas_sr = rbuf;
    _bas_chan = 1;
    for (i = 0; i < sizeof lines / sizeof lines[0]; i++)
        for (m = 0; m < 3; m++) {
            unsigned char px = 0, py = 0;
            for (k = 0; k < (modes[m] == 4 ? 1 : calls[i]); k++) {
                strcpy((char *)_bas_ibuf, lines[i]);
                _bas_ipos = px; _bas_m = modes[m]; _bas_n = 9;
                run(_bas_iitem, &x);
                px = _bas_ipos;
                strcpy((char *)_bas_ibuf, lines[i]);
                _bas_ipos = py; _bas_m = modes[m]; _bas_n = 9;
                run(ref_iitem, &y);
                py = _bas_ipos;
                if (memcmp(&x, &y, sizeof x)) bad[0]++;
                if (x.redone || y.redone) break;
            }
        }
    /* READ through the DATA, RESTOREs to statements, past the end */
    {
        static const unsigned char plan[] = {
            /* RESTORE offset (255: none), then the reads' modes */
            255, 0, 0, 0, 2, 0, 0, 2, 0, 2, 2, 0, 0xFE,
            6, 0, 2, 0xFE,
            19, 0, 0, 0, 0, 0xFE,
            0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0xFE,
        };
        unsigned char p = 0, rx = 1, ry = 1;
        while (p < sizeof plan) {
            unsigned char o = plan[p++];
            if (o != 255) {
                _bas_n = o; _bas_restore();
                _bas_n = o; ref_restore();
            }
            rx = ry = 1;
            while (plan[p] != 0xFE) {
                _bas_m = plan[p++];
                if (rx) { run(_bas_read, &x); if (x.err) rx = 0; }
                if (ry) { run(ref_read, &y); if (y.err) ry = 0; }
                x.pos = y.pos = 0;   /* (INPUT's position: not READ's) */
                if (memcmp(&x, &y, sizeof x)) bad[1]++;
                if (!rx || !ry) { while (plan[p] != 0xFE) p++; }
            }
            p++;
        }
    }
    OUT[0] = bad[0];
    OUT[1] = bad[1];
    OUT[2] = 0xAA;
    return 0;
}
