/* difftime.c — Difference between two time_t values (ZP calling convention) */

typedef long time_t;

long difftime(time_t t2, time_t t1) {
    return t2 - t1;
}
