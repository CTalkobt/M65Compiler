/* strftime.c — calendar time formatted (<time.h>), the "C" locale
 *
 * The C99 conversions; E and O modifiers are skipped. %z is +0000 and %Z
 * empty: the RTC has no time zone. The ISO 8601 week (%G %g %V) starts on
 * Monday; week 1 holds the year's first Thursday.
 */

#pragma cc45 signed_default

#include <time.h>

static const char *const day_names[7] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};
static const char *const mon_names[12] = {
    "January", "February", "March", "April", "May", "June", "July",
    "August", "September", "October", "November", "December"
};

static char *out;
static unsigned int room;       /* bytes left before the NUL's */
static unsigned char full;

static void put(char c) {
    if (!room) {
        full = 1;
        return;
    }
    *out++ = c;
    room--;
}

static void puts_n(const char *s, int n) {
    while (*s && n--) put(*s++);
}

/* v in at least width digits, padded with pad (0: none) */
static void num(long v, int width, char pad) {
    char d[11];
    int n = 0;
    if (v < 0) {
        put('-');
        v = -v;
        if (width) width--;
    }
    do {
        d[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    while (pad && width-- > n) put(pad);
    while (n) put(d[--n]);
}

/* 52 or 53: the ISO weeks of year y */
static int iso_weeks(long y) {
    long p = (y + y / 4 - y / 100 + y / 400) % 7;
    long q = ((y - 1) + (y - 1) / 4 - (y - 1) / 100 + (y - 1) / 400) % 7;
    return (p == 4 || q == 3) ? 53 : 52;
}

/* The ISO week (1-53) of tm; *year its ISO year */
static int iso_week(const struct tm *tm, long *year) {
    long y = tm->tm_year + 1900L;
    int wd = (tm->tm_wday + 6) % 7 + 1;          /* Monday 1 ... Sunday 7 */
    int w = (tm->tm_yday + 1 - wd + 10) / 7;
    if (w < 1) {
        y--;
        w = iso_weeks(y);
    } else if (w > iso_weeks(y)) {
        y++;
        w = 1;
    }
    *year = y;
    return w;
}

static void format(const char *f, const struct tm *tm);

static void conv(char c, const struct tm *tm) {
    long y = tm->tm_year + 1900L, iy;
    int h12 = tm->tm_hour % 12;
    if (!h12) h12 = 12;
    switch (c) {
    case 'a': puts_n(day_names[tm->tm_wday % 7], 3); break;
    case 'A': puts_n(day_names[tm->tm_wday % 7], 99); break;
    case 'h':
    case 'b': puts_n(mon_names[tm->tm_mon % 12], 3); break;
    case 'B': puts_n(mon_names[tm->tm_mon % 12], 99); break;
    case 'c': format("%a %b %e %H:%M:%S %Y", tm); break;
    case 'C': num(y / 100, 2, '0'); break;
    case 'd': num(tm->tm_mday, 2, '0'); break;
    case 'D': format("%m/%d/%y", tm); break;
    case 'e': num(tm->tm_mday, 2, ' '); break;
    case 'F': format("%Y-%m-%d", tm); break;
    case 'g': iso_week(tm, &iy); num(iy % 100, 2, '0'); break;
    case 'G': iso_week(tm, &iy); num(iy, 0, 0); break;
    case 'H': num(tm->tm_hour, 2, '0'); break;
    case 'I': num(h12, 2, '0'); break;
    case 'j': num(tm->tm_yday + 1, 3, '0'); break;
    case 'm': num(tm->tm_mon + 1, 2, '0'); break;
    case 'M': num(tm->tm_min, 2, '0'); break;
    case 'n': put('\n'); break;
    case 'p': puts_n(tm->tm_hour < 12 ? "AM" : "PM", 2); break;
    case 'r': format("%I:%M:%S %p", tm); break;
    case 'R': format("%H:%M", tm); break;
    case 'S': num(tm->tm_sec, 2, '0'); break;
    case 't': put('\t'); break;
    case 'T': format("%H:%M:%S", tm); break;
    case 'u': num((tm->tm_wday + 6) % 7 + 1, 0, 0); break;
    case 'U': num((tm->tm_yday + 7 - tm->tm_wday) / 7, 2, '0'); break;
    case 'V': num(iso_week(tm, &iy), 2, '0'); break;
    case 'w': num(tm->tm_wday, 0, 0); break;
    case 'W': num((tm->tm_yday + 7 - (tm->tm_wday + 6) % 7) / 7, 2, '0'); break;
    case 'x': format("%m/%d/%y", tm); break;
    case 'X': format("%H:%M:%S", tm); break;
    case 'y': num(y % 100, 2, '0'); break;
    case 'Y': num(y, 0, 0); break;
    case 'z': puts_n("+0000", 5); break;
    case 'Z': break;
    case '%': put('%'); break;
    default: put('%'); put(c); break;
    }
}

static void format(const char *f, const struct tm *tm) {
    char c;
    while ((c = *f++) != 0) {
        if (c != '%') {
            put(c);
            continue;
        }
        c = *f++;
        if (c == 'E' || c == 'O') c = *f++;
        if (!c) {
            put('%');
            break;
        }
        conv(c, tm);
    }
}

unsigned int strftime(char *restrict s, unsigned int max, const char *restrict fmt,
                      const struct tm *restrict tm) {
    if (!max) return 0;
    out = s;
    room = max - 1;
    full = 0;
    format(fmt, tm);
    *out = 0;
    if (full) {
        *s = 0;
        return 0;
    }
    return (unsigned int)(out - s);
}
