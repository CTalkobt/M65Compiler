/* basic_sprite.c — the sprite statements and functions of BASIC 65 programs
 * compiled by basic45 --compile (#369)
 *
 * As BASIC 65's (b65.src V920413: sprite, sprbit, sprnum, Sprite_CLR,
 * sprcolor, movspr, get_arcor, angle_mode, metric, sprsav, collision,
 * rsprite, rspcolor, rsppos, bump), the VIC-IV's registers written directly
 * (BASIC's own tables not used: basic_spr.s45's __bas_sprt, moved by its
 * interrupt hook). The arguments in __bas_n / __bas_m / __float_a, the
 * sprite in __bas_sprn (_bas_sprnum):
 *   _bas_sprnum   __bas_n (0-255) a sprite 0-7 (else ?ILLEGAL QUANTITY)
 *   _bas_sprbit   SPRITE's 0 / 1 (__bas_n) into VIC register __bas_m
 *   _bas_sprcol   SPRITE's colour (any byte)
 *   _bas_sprclr   SPRITE CLR: all off, unexpanded, in front, hires; stopped;
 *                 their data pointers at $0600 (sprite 0) on
 *   _bas_sprmc    SPRCOLOR: multicolour __bas_m (0, 1) := __bas_n (0-15)
 *   _bas_getsad   __float_a as BASIC's getsad: rounded down, its low 16 bits
 *                 (negative too; ?ILLEGAL QUANTITY from 2^32 on) into __bas_n
 *   _bas_mvget    MOVSPR: the sprite's position (__bas_spx / __bas_spy)
 *   _bas_mvarg    coordinate __bas_m & 3 (x, y, the target's x, y) := __bas_n,
 *                 + it (bit 4) or - it (bit 5): relative to the position, or
 *                 to the last target (BASIC's xdest / ydest)
 *   _bas_mvput    the sprite at the position
 *   _bas_mvto     ... TO target, speed __float_a: the moves by metric's
 *                 sums (its |d| and carry as the ROM's), speed * 256 its speed
 *   _bas_mvang    angle # speed: the moves sin(angle) * speed, -cos(angle) * speed
 *   _bas_rspr     RSPRITE(sprite, __bas_n): 0 on, 1 colour, 2 behind, 3 x
 *                 expanded, 4 y expanded, 5 multicolour into __bas_n
 *   _bas_rspc     RSPCOLOR(__bas_n): multicolour 1 / 2
 *   _bas_rsppos   RSPPOS(sprite, __bas_n): x, y (into __bas_n; __float_a too
 *                 unless __bas_m), 2 the speed (into __float_a)
 *   _bas_bump     BUMP(__bas_n): the collisions seen since the last (1
 *                 sprite/sprite, 2 sprite/background), cleared
 *   _bas_sprgs    SPRSAV sprite, a$: its 64 bytes as __bas_sr's string
 *   _bas_sprps    SPRSAV __bas_sa, sprite: the string (64 characters, else
 *                 ?ILLEGAL QUANTITY) as its data
 *   _bas_spr64    the source string's length checked (before the target)
 *   _bas_sprcp    SPRSAV sprite __bas_m, sprite
 *   _bas_collide  COLLISION __bas_n (1-3), its line's code __bas_cline (0:
 *                 off)
 */

extern int _bas_n, _bas_m;
extern unsigned char *_bas_sa, *_bas_sr;
extern unsigned char _float_a[5], _float_b[5];
extern void _bas_error(void);
extern void _bas_int(void);
extern void _bas_ayint(void);
extern void _bas_sqr(void);
extern void _bas_fdiv(void);
extern void _float_sin(void);
extern void _float_cos(void);
extern void _bas_getpos(void);
extern void _bas_putpos(void);

struct spr { int spd; unsigned char xco, yco; int xmo, ymo; unsigned int xtg, ytg; };
extern struct spr _bas_sprt[8];
extern unsigned char _bas_sprn;
extern unsigned int _bas_spx, _bas_spy;
extern unsigned char _bas_coll[2];
extern unsigned char _bas_intv, _bas_trip0, _bas_trip1, _bas_trip2;
extern unsigned int _bas_cadr0, _bas_cadr1, _bas_cadr2;
unsigned int _bas_cline;

#define VIC ((volatile unsigned char *)0xD000)
#define SPRDATA ((unsigned char *)0x0600)      /* BASIC's sprite_base */

static unsigned int dest[2];     /* BASIC's xdest / ydest: the last target */

static void raise(int n) {
    _bas_n = n;
    _bas_error();
}

static float geta(void) {
    float f;
    unsigned char k;
    for (k = 0; k < 5; k++) ((unsigned char *)&f)[k] = _float_a[k];
    return f;
}

static void seta(float f) {
    unsigned char k;
    for (k = 0; k < 5; k++) _float_a[k] = ((unsigned char *)&f)[k];
}

static void setb(float f) {
    unsigned char k;
    for (k = 0; k < 5; k++) _float_b[k] = ((unsigned char *)&f)[k];
}

/* a / b, ?DIVISION BY ZERO */
static float fdiv(float a, float b) {
    seta(a);
    setb(b);
    _bas_fdiv();
    return geta();
}

/* f * 256 as the ROM's exponent + 8 (0 stays 0) */
static float x256(float f) {
    seta(f);
    if (_float_a[0]) _float_a[0] += 8;
    return geta();
}

/* the ROM's flpint (ayint: rounded down, ?ILLEGAL QUANTITY outside -32768..32767) */
static int flpint(float f) {
    seta(f);
    _bas_ayint();
    return _bas_n;
}

void _bas_sprnum(void) {
    if ((unsigned)_bas_n >= 8) raise(14);
    _bas_sprn = (unsigned char)_bas_n;
}

void _bas_sprbit(void) {
    unsigned char b = (unsigned char)(1 << _bas_sprn);
    if ((unsigned)_bas_n > 1) raise(14);
    if (_bas_n) VIC[_bas_m] |= b;
    else VIC[_bas_m] &= (unsigned char)~b;
}

void _bas_sprcol(void) {
    VIC[39 + _bas_sprn] = (unsigned char)_bas_n;
}

void _bas_sprclr(void) {
    unsigned char k, *p;
    __asm__("php\n sei");
    VIC[21] = 0;
    VIC[23] = 0;
    VIC[27] = 0;
    VIC[28] = 0;
    VIC[29] = 0;
    for (k = 0; k < 96; k++) ((unsigned char *)_bas_sprt)[k] = 0;
    /* the pointers after the 40 or 80 column screen at $0800 */
    p = (unsigned char *)((VIC[0x31] & 0x80) ? 0x0FF8 : 0x0BF8);
    for (k = 0; k < 8; k++) p[k] = (unsigned char)(0x0600 / 64 + k);
    __asm__("plp");
}

void _bas_sprmc(void) {
    if ((unsigned)_bas_n > 15) raise(14);
    VIC[37 + _bas_m] = (unsigned char)_bas_n;
}

void _bas_getsad(void) {
    float f, m;
    unsigned int v;
    if (_float_a[0] >= 0xA1) raise(14);
    _bas_int();
    f = geta();
    m = f < 0 ? -f : f;
    /* (exact: whole numbers below 2^32) */
    m = m - 65536.0f * (float)(unsigned long)(m / 65536.0f);
    v = (unsigned int)(unsigned long)m;
    _bas_n = (int)(f < 0 ? (unsigned int)(0u - v) : v);
}

void _bas_mvget(void) {
    _bas_getpos();
}

void _bas_mvarg(void) {
    unsigned char w = (unsigned char)(_bas_m & 3);
    unsigned int *c = w == 0 ? &_bas_spx : w == 1 ? &_bas_spy : &dest[w - 2];
    unsigned int v = (unsigned int)_bas_n;
    if (_bas_m & 0x10) *c += v;
    else if (_bas_m & 0x20) *c -= v;
    else *c = v;
}

void _bas_mvput(void) {
    _bas_putpos();
}

/* the moves and targets in, then the speed (an error between leaves the
 * old speed, as the ROM's) */
static void start(int xmo, int ymo, float speed) {
    struct spr *s = &_bas_sprt[_bas_sprn];
    int spd;
    __asm__("php\n sei");
    s->xtg = dest[0];
    s->ytg = dest[1];
    s->xmo = xmo;
    s->ymo = ymo;
    __asm__("plp");
    seta(speed);
    _float_a[0] += 8;
    spd = flpint(geta());
    __asm__("php\n sei");
    s->spd = spd;
    __asm__("plp");
}

/* |d| as the ROM's (the high byte complemented, the low one negated) */
static unsigned int mag(unsigned int d, unsigned char neg) {
    if (!neg) return d;
    return (unsigned int)((((d >> 8) ^ 0xFF) << 8) | ((0u - d) & 0xFF));
}

void _bas_mvto(void) {
    float speed = geta(), ms;
    unsigned int dy = dest[1] - _bas_spy, dx = dest[0] - _bas_spx, ay, ax;
    unsigned long q;
    int ymo, xmo;
    ay = mag(dy, dest[1] < _bas_spy);
    ax = mag(dx, dest[0] < _bas_spx) & 0x1FF;      /* (clipped, as the ROM's) */
    q = (unsigned long)ay * ay + (unsigned long)ax * ax;
    if (dest[0] >= _bas_spx) q++;                  /* (the ROM's carry) */
    seta((float)q);
    _bas_sqr();
    ms = fdiv(geta(), speed);                      /* metric / speed */
    ymo = flpint(x256(fdiv((float)(int)dy, ms)));
    xmo = flpint(x256(fdiv((float)(int)dx, ms)));
    start(xmo, ymo, speed);
}

void _bas_mvang(void) {
    float speed = geta(), a, s, c;
    int xmo, ymo;
    a = (float)(int)_bas_spx * 0.01745329252f;
    seta(a);
    _float_sin();
    s = geta();
    if (_float_a[0]) s = x256(s) * speed;
    xmo = flpint(s);
    seta(a);
    _float_cos();
    c = geta();
    if (_float_a[0]) c = -x256(c) * speed;
    ymo = flpint(c);
    dest[0] = dest[1] = 0xFFFF;
    start(xmo, ymo, speed);
}

void _bas_rspr(void) {
    static const unsigned char reg[6] = {21, 39, 27, 29, 23, 28};
    unsigned char a = (unsigned char)_bas_n;
    if (a >= 6) raise(14);
    if (a == 1) _bas_n = VIC[39 + _bas_sprn] & 15;
    else _bas_n = (VIC[reg[a]] & (1 << _bas_sprn)) ? 1 : 0;
}

void _bas_rspc(void) {
    unsigned char a = (unsigned char)(_bas_n - 1);
    if (a >= 2) raise(14);
    _bas_n = VIC[37 + a] & 15;
}

void _bas_rsppos(void) {
    unsigned char a = (unsigned char)_bas_n;
    if (a >= 3) raise(14);
    if (a == 2) {
        int spd;
        __asm__("php\n sei");
        spd = _bas_sprt[_bas_sprn].spd;
        __asm__("plp");
        seta((float)spd);
        if (_float_a[0]) _float_a[0] -= 8;
        return;
    }
    _bas_getpos();
    _bas_n = (int)(a ? _bas_spy : _bas_spx);
    if (!_bas_m) seta((float)(unsigned int)_bas_n);
}

void _bas_bump(void) {
    unsigned char a = (unsigned char)(_bas_n - 1), c;
    if (a >= 2) raise(14);
    __asm__("php\n sei");
    c = _bas_coll[a];
    _bas_coll[a] = 0;
    __asm__("plp");
    _bas_n = c;
}

void _bas_spr64(void) {
    if (_bas_sa[0] != 64) raise(14);
}

void _bas_sprgs(void) {
    unsigned char k, *p = SPRDATA + (_bas_sprn << 6);
    _bas_sr[0] = 64;
    for (k = 0; k < 64; k++) _bas_sr[k + 1] = p[k];
}

void _bas_sprps(void) {
    unsigned char k, *p = SPRDATA + (_bas_sprn << 6);
    if (_bas_sa[0] != 64) raise(14);
    for (k = 0; k < 64; k++) p[k] = _bas_sa[k + 1];
}

void _bas_sprcp(void) {
    unsigned char k, *s = SPRDATA + (_bas_m << 6), *d = SPRDATA + (_bas_sprn << 6);
    for (k = 0; k < 64; k++) d[k] = s[k];
}

void _bas_collide(void) {
    unsigned char t = (unsigned char)(_bas_n - 1), b;
    if (t >= 3) raise(14);
    b = (unsigned char)(1 << t);
    __asm__("php\n sei");
    if (t == 0) _bas_cadr0 = _bas_cline;
    else if (t == 1) _bas_cadr1 = _bas_cline;
    else _bas_cadr2 = _bas_cline;
    if (_bas_cline) _bas_intv |= b;
    else {
        _bas_intv &= (unsigned char)~b;
        if (t == 0) _bas_trip0 = 0;         /* (none left to run to no line) */
        else if (t == 1) _bas_trip1 = 0;
        else _bas_trip2 = 0;
    }
    __asm__("plp");
}
