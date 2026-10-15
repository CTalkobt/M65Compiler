/*
 * rtc_clock — the MEGA65's real-time clock, in big digits
 *
 * Shows the time in large digits (a 5x7 font drawn with reverse spaces),
 * the weekday and date, a bar of the seconds, and a stopwatch timed by the
 * video frames.
 *
 * The RTC: BCD registers at $FFD7110-$FFD7116 (seconds, minutes, hours,
 * day, month, year 00-99, day of week 0-6 with 0 = Sunday), outside the
 * 16-bit address space: a DMA job copies them into a buffer each frame
 * (dma_copy_far at 28-bit addresses). Bit 7 of the hours is the 24-hour
 * mode; 12-hour display is done here. (xemu fills the registers from the
 * host's clock; its month is BCD(month - 1) + 1, e.g. $0A for October,
 * which the BCD decoding below also reads as 10.)
 *
 * Keys (<input.h>):
 *   H 12/24 hours   S stopwatch start/stop   R stopwatch reset
 *   C colours       RUN/STOP quits
 *
 * The screen is our own buffer (the VIC-IV screen and colour pointers are
 * moved to it, so BASIC's screen comes back at the end). Interrupts are
 * off while it runs; frames are counted on the raster (50 or 60 a second).
 *
 * Build with -DRTC_TEST for the check run in xemu: the RTC is read, then
 * read again once the seconds change; the values must be valid BCD in
 * range and one second apart. Results at $C0C0 (01 = pass), the decoded
 * time at $C0C8.
 */
#include <mega65.h>
#include <dma.h>
#include <input.h>

#define REG(a) (*(volatile unsigned char *)(a))
#define RTC_LIN 0xFFD7110UL

/* the RTC registers, copied by DMA */
static unsigned char rtc[7];

static void rtc_fetch(void) {
    dma_copy_far((unsigned long)(unsigned int)rtc, RTC_LIN, 7);
}
static unsigned char bcd(unsigned char b) {
    return (b >> 4) * 10 + (b & 0x0F);
}

struct now {
    unsigned char sec, min, hour, day, month, year, wday;
};
static struct now t;

static void rtc_decode(struct now *n) {
    n->sec = bcd(rtc[0] & 0x7F);
    n->min = bcd(rtc[1] & 0x7F);
    n->hour = bcd(rtc[2] & 0x3F);
    n->day = bcd(rtc[3] & 0x3F);
    n->month = bcd(rtc[4] & 0x1F);
    n->year = bcd(rtc[5]);
    n->wday = rtc[6] & 0x07;
}

static unsigned char frame;

static void wait_frame(void) {
    while (REG(0xD012) == 0) ;
    while (REG(0xD012) != 0 || (REG(0xD011) & 0x80)) ;
}

#ifdef RTC_TEST

#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned char valid(const struct now *n) {
    return n->sec < 60 && n->min < 60 && n->hour < 24 && n->day >= 1 && n->day <= 31 &&
           n->month >= 1 && n->month <= 12 && n->year < 100 && n->wday < 7 &&
           (rtc[0] & 0x0F) < 10 && (rtc[1] & 0x0F) < 10 && (rtc[2] & 0x0F) < 10 &&
           (rtc[3] & 0x0F) < 10 && (rtc[5] & 0x0F) < 10;
}

int main(void) {
    struct now a, b;
    unsigned int frames = 0;
    unsigned char next;
    rtc_fetch();
    rtc_decode(&a);
    OUT[0] = valid(&a);
    OUT[1] = (rtc[2] & 0x80) != 0;             /* 24-hour mode */
    do {                                        /* until the seconds change */
        wait_frame();
        rtc_fetch();
        rtc_decode(&b);
    } while (b.sec == a.sec && ++frames < 3000);
    OUT[2] = valid(&b) && frames < 3000;
    next = a.sec == 59 ? 0 : a.sec + 1;
    OUT[3] = b.sec == next;
    OUT[4] = 0xAA;
    OUT[8] = b.hour; OUT[9] = b.min; OUT[10] = b.sec;
    OUT[11] = b.day; OUT[12] = b.month; OUT[13] = b.year; OUT[14] = b.wday;
    return 0;
}

#else

/* ------------------------------------------------------------------------ */
/* The screen: our own 80x25 buffer, colour RAM from offset $0800           */

#define COLOUR_LIN 0xFF80800UL
static unsigned char scr[2000];
static unsigned char cols;
static unsigned int stride;

static unsigned char scode(unsigned char c) {
    if (c >= 0x40 && c < 0x60) return c - 0x40;
    if (c >= 0x60 && c < 0x80) return c - 0x20;
    if (c >= 0xC0 && c < 0xE0) return c - 0x80;
    return c;
}
static unsigned char *at(unsigned char x, unsigned char y) {
    return &scr[y * stride + x];
}
static unsigned char slen(const char *s) {
    unsigned char n = 0;
    while (s[n]) n++;
    return n;
}
static void put(unsigned char x, unsigned char y, const char *s) {
    unsigned char *d = at(x, y);
    while (*s) *d++ = scode((unsigned char)*s++);
}
static void put_centred(unsigned char y, const char *s) {
    unsigned char *d = at(0, y), i;
    for (i = 0; i < cols; i++) d[i] = ' ';
    put((cols - slen(s)) / 2, y, s);
}
static void colour(unsigned char x, unsigned char y, unsigned char w, unsigned char c) {
    dma_fill_far(COLOUR_LIN + y * stride + x, w, c);
}

/* The big digits: 5x7, a row a byte (bit 4 = the left column) */
static const unsigned char font[11][7] = {
    { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E },   /* 0 */
    { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E },   /* 1 */
    { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F },   /* 2 */
    { 0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E },   /* 3 */
    { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 },   /* 4 */
    { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E },   /* 5 */
    { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E },   /* 6 */
    { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 },   /* 7 */
    { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E },   /* 8 */
    { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C },   /* 9 */
    { 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00 },   /* : */
};
#define BIG_Y 5
#define COLON 10

/* a big character at column x: 5 wide (the colon 3) */
static void big(unsigned char x, unsigned char ch) {
    unsigned char r, c, w = ch == COLON ? 3 : 5, shift = ch == COLON ? 1 : 0;
    for (r = 0; r < 7; r++) {
        unsigned char *d = at(x, BIG_Y + r), bits = font[ch][r] >> shift;
        for (c = 0; c < w; c++) d[c] = (bits >> (w - 1 - c)) & 1 ? 0xA0 : ' ';
    }
}

/* ------------------------------------------------------------------------ */

static const char *wday_name[7] = { "sunday", "monday", "tuesday", "wednesday",
                                    "thursday", "friday", "saturday" };
static const char *month_name[12] = { "january", "february", "march", "april", "may", "june",
                                      "july", "august", "september", "october",
                                      "november", "december" };

/* colour schemes: hours, minutes, seconds */
static const unsigned char schemes[4][3] = {
    { 7, 3, 13 }, { 1, 1, 1 }, { 10, 8, 7 }, { 14, 3, 5 } };
static unsigned char scheme, h12, ntsc;
static unsigned char sw_run;
static unsigned long sw_frames;     /* the stopwatch, in frames */

static unsigned char big_x;         /* where the big time starts */
static unsigned char show_secs;     /* the seconds in big digits too (80 columns) */

static void num2(char *b, unsigned char v) {
    b[0] = '0' + v / 10;
    b[1] = '0' + v % 10;
}

static void draw_time(void) {
    unsigned char h = t.hour, x = big_x, i;
    if (h12) {
        h = h % 12;
        if (h == 0) h = 12;
    }
    big(x, h / 10); big(x + 6, h % 10);
    big(x + 12, COLON);
    big(x + 16, t.min / 10); big(x + 22, t.min % 10);
    if (show_secs) {
        big(x + 28, COLON);
        big(x + 32, t.sec / 10); big(x + 38, t.sec % 10);
    }
    if (h12) put(x + (show_secs ? 44 : 28), BIG_Y + 6, t.hour < 12 ? "am" : "pm");
    else put(x + (show_secs ? 44 : 28), BIG_Y + 6, "  ");
    for (i = 0; i < 7; i++) {
        colour(x, BIG_Y + i, 12, schemes[scheme][0]);        /* hours */
        colour(x + 12, BIG_Y + i, 16, schemes[scheme][1]);   /* :minutes */
        if (show_secs) colour(x + 28, BIG_Y + i, 18, schemes[scheme][2]);
    }
}

static void draw_date(void) {
    char buf[40];
    const char *w = wday_name[t.wday % 7], *m = month_name[(t.month + 11) % 12];
    unsigned char n = 0;
    unsigned int y = 2000 + t.year;
    while (*w) buf[n++] = *w++;
    buf[n++] = ' ';
    if (t.day >= 10) buf[n++] = '0' + t.day / 10;
    buf[n++] = '0' + t.day % 10;
    buf[n++] = ' ';
    while (*m) buf[n++] = *m++;
    buf[n++] = ' ';
    buf[n++] = '0' + y / 1000; buf[n++] = '0' + y / 100 % 10;
    buf[n++] = '0' + y / 10 % 10; buf[n++] = '0' + y % 10;
    buf[n] = 0;
    put_centred(BIG_Y + 9, buf);
}

/* the seconds as a bar of 60 cells (30 at 40 columns) */
static void draw_bar(void) {
    unsigned char w = cols >= 80 ? 60 : 30, x0 = (cols - w) / 2, i;
    unsigned char n = cols >= 80 ? t.sec + 1 : (t.sec + 2) / 2;
    unsigned char *d = at(x0, BIG_Y + 11);
    for (i = 0; i < w; i++) d[i] = i < n ? 0xA0 : 0x2E;
}

static void draw_stopwatch(void) {
    char buf[24];
    unsigned char fps = ntsc ? 60 : 50;
    unsigned long s = sw_frames / fps;
    unsigned char tenth = (unsigned char)((sw_frames % fps) * 10 / fps);
    put(0, 0, "");
    buf[0] = 0;
    {
        static const char lbl[] = "stopwatch ";
        unsigned char n = 0, i;
        for (i = 0; lbl[i]; i++) buf[n++] = lbl[i];
        num2(buf + n, (unsigned char)(s / 3600 % 100)); n += 2; buf[n++] = ':';
        num2(buf + n, (unsigned char)(s / 60 % 60)); n += 2; buf[n++] = ':';
        num2(buf + n, (unsigned char)(s % 60)); n += 2; buf[n++] = '.';
        buf[n++] = '0' + tenth;
        buf[n++] = ' ';
        buf[n++] = sw_run ? '>' : ' ';
        buf[n] = 0;
    }
    put_centred(BIG_Y + 14, buf);
}

static void draw_frame(void) {
    unsigned char y;
    for (y = 0; y < 25; y++) colour(0, y, cols, 1);
    put_centred(1, "mega65 real-time clock");
    colour(0, 1, cols, 3);
    put_centred(22, "h 12/24 hours   s stopwatch   r reset");
    put_centred(23, "c colours   run/stop quit");
    colour(0, 22, cols, 14);
    colour(0, 23, cols, 14);
    colour(0, BIG_Y + 9, cols, 15);
    colour(0, BIG_Y + 11, cols, 5);
    colour(0, BIG_Y + 14, cols, 7);
}

static unsigned char saved[8];

static void video_on(void) {
    saved[0] = REG(0xD060); saved[1] = REG(0xD061); saved[2] = REG(0xD062); saved[3] = REG(0xD063);
    saved[4] = REG(0xD064); saved[5] = REG(0xD065);
    saved[6] = REG(0xD020); saved[7] = REG(0xD021);
    cols = REG(0xD05E);
    stride = REG(0xD058) | (REG(0xD059) << 8);
    if (cols < 40) cols = 40;
    if (cols > 80) cols = 80;
    if (stride > 80 || stride < cols) stride = cols;
    show_secs = cols >= 80;
    big_x = (cols - (show_secs ? 46 : 30)) / 2;
    __dma_fill(scr, 2000, ' ');
    dma_fill_far(COLOUR_LIN, 2000, 1);
    REG(0xD060) = (unsigned char)(unsigned int)scr;
    REG(0xD061) = (unsigned char)((unsigned int)scr >> 8);
    REG(0xD062) = 0;
    REG(0xD063) = REG(0xD063) & 0xF0;
    REG(0xD064) = 0x00;
    REG(0xD065) = 0x08;
    REG(0xD020) = 0;
    REG(0xD021) = 0;
}
static void video_off(void) {
    REG(0xD060) = saved[0]; REG(0xD061) = saved[1]; REG(0xD062) = saved[2]; REG(0xD063) = saved[3];
    REG(0xD064) = saved[4]; REG(0xD065) = saved[5];
    REG(0xD020) = saved[6]; REG(0xD021) = saved[7];
}

int main(void) {
    unsigned char c, last_sec = 255, quit = 0;
    __asm__("sei");
    ntsc = REG(0xD06F) & 0x80;
    video_on();
    draw_frame();
    input_flush();
    while (!quit) {
        wait_frame();
        frame++;
        rtc_fetch();
        rtc_decode(&t);
        if (sw_run) sw_frames++;
        if (t.sec != last_sec) {
            last_sec = t.sec;
            draw_time();
            draw_date();
            draw_bar();
        }
        draw_stopwatch();
        while ((c = input_key()) != 0) {
            switch (c) {
                case 'h': case 'H': h12 = !h12; last_sec = 255; break;
                case 's': case 'S': sw_run = !sw_run; break;
                case 'r': case 'R': sw_frames = 0; break;
                case 'c': case 'C': scheme = (scheme + 1) & 3; last_sec = 255; break;
                case CH_STOP: quit = 1; break;         /* RUN/STOP */
            }
        }
    }
    video_off();
    input_flush();
    __asm__("cli");
    return 0;
}

#endif
