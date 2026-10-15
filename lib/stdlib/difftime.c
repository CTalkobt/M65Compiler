/* difftime.c — Difference between two time_t values, in seconds */

#include <time.h>

signed long difftime(time_t t2, time_t t1) {
    return t2 - t1;
}
