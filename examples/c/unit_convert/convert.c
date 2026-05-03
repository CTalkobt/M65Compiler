/* convert.c — Unit converter using long arithmetic
 *
 * Demonstrates: long type, sprintf, sscanf, puts
 *
 * Converts distances between miles and kilometers using
 * 32-bit fixed-point arithmetic (multiply then divide)
 * to avoid floating point.
 *
 * 1 mile  = 1.609344 km  ≈ 1609/1000
 * 1 km    = 0.621371 mi  ≈  621/1000
 */

#include <stdio.h>
#include <stdlib.h>

/* Fixed-point conversion: miles -> km = miles * 1609 / 1000 */
long miles_to_km(long miles) {
    return miles * 1609L / 1000L;
}

/* Fixed-point conversion: km -> miles = km * 621 / 1000 */
long km_to_miles(long km) {
    return km * 621L / 1000L;
}

int main() {
    puts("unit converter starting");

    char buf[40];
    long val;
    long result;

    /* --- Miles to Kilometers --- */

    val = 26L;  /* marathon distance */
    result = miles_to_km(val);
    puts("before sprintf");
    sprintf(buf, "%ld miles = %ld km", val, result);
    puts("after sprintf");
    puts(buf);

    val = 100L;
    result = miles_to_km(val);
    sprintf(buf, "%ld miles = %ld km", val, result);
    puts(buf);

    val = 93000000L;  /* earth-sun distance */
    result = miles_to_km(val);
    sprintf(buf, "%ld miles = %ld km", val, result);
    puts(buf);

    /* --- Kilometers to Miles --- */

    val = 42L;  /* marathon in km */
    result = km_to_miles(val);
    sprintf(buf, "%ld km = %ld miles", val, result);
    puts(buf);

    val = 384400L;  /* earth-moon distance */
    result = km_to_miles(val);
    sprintf(buf, "%ld km = %ld miles", val, result);
    puts(buf);

    return 0;
}
