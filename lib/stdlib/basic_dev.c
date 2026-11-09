/* basic_dev.c — basic45 --compile's JOY, POT, LPEN, MOUSE and RMOUSE (#369),
 * as BASIC 65's (b65.src V920413: joy, joybutton23, pot, lpen, mouse,
 * rmouse): the CIA, the paddle registers ($D620-$D623) and the KERNAL's mouse
 * driver directly. The light pen's latched position is in BASIC's bytes
 * ($02E8 / $02E9), which basic_spr.s45's interrupt hook sets as BASIC's IRQ
 * does. Arguments: _bas_n (the function's argument; MOUSE's port), _bas_m
 * (MOUSE's sprite), _bas_g[0..3] (MOUSE's hotspot: given flags, x, y);
 * results in _bas_n.
 */

extern int _bas_n, _bas_m;
extern void _bas_error(void);

#define CIA1 ((volatile unsigned char *)0xDC00)
#define PADDLE ((volatile unsigned char *)0xD620)
#define KBD_D607 (*(volatile unsigned char *)0xD607)
#define KBD_D608 (*(volatile unsigned char *)0xD608)
#define LIGHTPEN ((volatile unsigned char *)0x02E8)
#define VIC ((volatile unsigned char *)0xD000)

static void raise(int n) {
    _bas_n = n;
    _bas_error();
}

static const unsigned char joytab[11] = {4, 2, 3, 0, 6, 8, 7, 0, 5, 1, 0};

/* buttons "2" and "3" of port p (joybutton23): its paddle lines low */
static unsigned char joyb23(unsigned char p) {
    unsigned char r = 0;
    if (PADDLE[p * 2] < 0x10) r |= 1;
    if (PADDLE[p * 2 + 1] < 0x10) r |= 2;
    return r;
}

/* JOY(n), n 1-3 (3: both ports): the direction 0-8, 128 the fire button,
 * 256 / 512 buttons 2 / 3 (the paddle lines), 1024 / 2048 buttons 4 / 5
 * (up + down, left + right: no direction then) */
void _bas_joy(void) {
    unsigned char n = (unsigned char)(_bas_n - 1), v, dir, hi = 0;
    if ((unsigned int)_bas_n - 1 >= 3) raise(14);
    __asm__("php\n sei");
    CIA1[0] = 0xFF;
    KBD_D607 |= 2;
    KBD_D608 |= 2;
    if (n == 2) {
        unsigned char a, b;
        do a = CIA1[0]; while (a != CIA1[0]);
        do b = CIA1[1]; while (b != CIA1[1]);
        v = a & b;
    } else {
        unsigned char x = n ^ 1;
        do v = CIA1[x]; while (v != CIA1[x]);
    }
    KBD_D607 &= ~2;
    KBD_D608 &= ~2;
    CIA1[0] = 0x7F;
    __asm__("plp");
    dir = (v & 15) >= 5 ? joytab[(v & 15) - 5] : 0;
    if (!(v & 0x10)) dir |= 0x80;
    if (!(v & 3)) hi |= 4;
    if (!(v & 12)) hi |= 8;
    if (hi) dir &= 0x80;
    if (n == 2) hi |= joyb23(0) | joyb23(1);
    else hi |= joyb23(n);
    _bas_n = (int)((unsigned int)hi << 8 | dir);
}

/* POT(n), n 1-4: paddle n's value, 256 more with its button (the port's
 * joystick line 2 / 3) */
void _bas_pot(void) {
    unsigned char x = (unsigned char)(_bas_n - 1), v, t;
    if ((unsigned int)_bas_n - 1 >= 4) raise(14);
    __asm__("php\n sei");
    v = PADDLE[x];
    CIA1[0] = 0xFF;
    t = (CIA1[((x & 2) ^ 2) >> 1] & ((x & 1) ? 8 : 4)) ? 0 : 1;
    CIA1[0] = 0x7F;
    __asm__("plp");
    _bas_n = (int)((unsigned int)t << 8 | v);
}

/* LPEN(n), n 0-1: the light pen's latched x (times 2) or y, then 0 */
void _bas_lpen(void) {
    unsigned char v;
    if ((unsigned int)_bas_n >= 2) raise(14);
    __asm__("sei");
    v = LIGHTPEN[_bas_n];
    LIGHTPEN[_bas_n] = 0;
    __asm__("cli");
    _bas_n = _bas_n ? v : (int)((unsigned int)v * 2);
}

/* the KERNAL's mouse driver (_mouse, $E01B): A ports (b7 2, b6 1; 0 off),
 * X the sprite */
#define MOUSE_ENABLE (*(volatile unsigned char *)0x1138)
#define MOUSE_POINTER (*(volatile unsigned char *)0x1139)
#define MOUSE_TOP (*(volatile unsigned char *)0x113C)
#define MOUSE_BOTTOM (*(volatile unsigned char *)0x113D)
#define MOUSE_LEFT (*(volatile unsigned char *)0x113E)
#define MOUSE_RIGHT (*(volatile unsigned char *)0x113F)

struct spr { unsigned int speed; unsigned char rest[10]; };
extern struct spr _bas_sprt[8];
extern unsigned char _bas_g[20];

/* MOUSE OFF */
void _bas_mouseoff(void) {
    __jsr(0xE01B, .A = 0);
}

/* MOUSE ON [, port [, sprite [, hotspot x [, y]]]]: _bas_n the port (0-3),
 * _bas_m the sprite (0-7), _bas_g[0] bit 0 / 1 the hotspot's x / y given in
 * _bas_g[1] / _bas_g[2] (x 0-23, y 0-20) */
void _bas_mouseon(void) {
    unsigned char port = (unsigned char)_bas_n, sp = (unsigned char)_bas_m;
    if ((unsigned int)_bas_n >= 4) raise(14);
    if ((unsigned int)_bas_m >= 8) raise(14);
    _bas_sprt[sp].speed = 0;                /* (a moving sprite stopped) */
    __jsr(0xE01B, .A = 0);
    __jsr(0xE01B, .A = (unsigned char)(port << 6), .X = sp);
    if (_bas_g[0] & 1) {
        if (_bas_g[1] >= 24) raise(14);
        MOUSE_LEFT = (unsigned char)(24 - _bas_g[1]);
        MOUSE_RIGHT = (unsigned char)(87 - _bas_g[1]);
    }
    if (_bas_g[0] & 2) {
        if (_bas_g[2] >= 21) raise(14);
        MOUSE_TOP = (unsigned char)(50 - _bas_g[2]);
        MOUSE_BOTTOM = (unsigned char)(250 - _bas_g[2]);
    }
}

/* RMOUSE's x, y, buttons (128 left, 1 right): -1 each without a mouse */
int _bas_rmv[3];
void _bas_rmouse(void) {
    unsigned char ports = MOUSE_ENABLE & 0xC0, y, b = 0, p;
    _bas_rmv[0] = _bas_rmv[1] = _bas_rmv[2] = -1;
    if (!ports) return;
    __asm__("sei");
    y = MOUSE_POINTER;
    _bas_rmv[0] = VIC[y] | ((VIC[0x10] & (1 << (y >> 1))) ? 256 : 0);
    _bas_rmv[1] = VIC[y + 1];
    CIA1[0] = 0xFF;
    for (p = 0; p < 2; p++) {
        if (ports & 0x80) {
            unsigned char v = (CIA1[p] & 0x11) ^ 0x11;
            b |= v;
            if (v & 0x10) b |= 0x80;
        }
        ports <<= 1;
    }
    b &= 0x81;
    CIA1[0] = 0xFF;
    __asm__("cli");
    _bas_rmv[2] = b;
}
