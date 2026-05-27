// test_time: validate time.h functions
// Returns 0 (A=$00) on success, non-zero on failure.

#include <time.h>

int main() {
    clock_t c1;
    clock_t c2;
    time_t t1;
    time_t t2;
    long diff;

    // clock() should return a value (can't predict exact value)
    c1 = clock();

    // Call again — should be >= first call
    c2 = clock();
    if (c2 < c1) return 1;

    // time(NULL) should return a value
    t1 = time(0);

    // time(&t2) should store and return same value
    t2 = 0;
    time(&t2);
    if (t2 == 0 && t1 != 0) return 2;

    // difftime
    diff = difftime(10, 3);
    if (diff != 7) return 3;

    diff = difftime(3, 10);
    if (diff != -7) return 4;

    diff = difftime(5, 5);
    if (diff != 0) return 5;

    // CLOCKS_PER_SEC should be 60
    if (CLOCKS_PER_SEC != 60) return 6;

    return 0;
}
