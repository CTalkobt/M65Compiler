/* time.c — Return jiffy clock as 32-bit time_t */

typedef long time_t;

int clock(void);

time_t time(time_t *tp) {
    time_t t = (time_t)clock();
    if (tp) *tp = t;
    return t;
}
