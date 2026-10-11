/* math_double.c — the C library's double-precision names and sign helpers.
 *
 * cc45's double is the same CBM 40-bit format as float, so fabs() etc. are
 * the float routines under their standard names. <math.h> maps them to the
 * f-suffixed versions with macros; these definitions serve code that
 * declares the functions itself or calls __builtin_fabs etc. (math.h is not
 * included here: its macros would rename these definitions).
 */

float sinf(float x);
float cosf(float x);
float tanf(float x);
float atanf(float x);
float logf(float x);
float expf(float x);
float sqrtf(float x);
float fabsf(float x);
float powf(float x, float y);
float truncf(float x);
float floorf(float x);
float ceilf(float x);
float roundf(float x);
float fmodf(float x, float y);
float atan2f(float y, float x);
float log10f(float x);
float log2f(float x);
float ldexpf(float x, signed int n);
float frexpf(float x, signed int *exp);
float modff(float x, float *iptr);
float copysignf(float x, float y);
float fmaxf(float x, float y);
float fminf(float x, float y);
float fdimf(float x, float y);

double sin(double x) { return sinf(x); }
double cos(double x) { return cosf(x); }
double tan(double x) { return tanf(x); }
double atan(double x) { return atanf(x); }
double log(double x) { return logf(x); }
double exp(double x) { return expf(x); }
double sqrt(double x) { return sqrtf(x); }
double fabs(double x) { return fabsf(x); }
double pow(double x, double y) { return powf(x, y); }
double trunc(double x) { return truncf(x); }
double floor(double x) { return floorf(x); }
double ceil(double x) { return ceilf(x); }
double round(double x) { return roundf(x); }
double fmod(double x, double y) { return fmodf(x, y); }
double atan2(double y, double x) { return atan2f(y, x); }
double log10(double x) { return log10f(x); }
double log2(double x) { return log2f(x); }
double ldexp(double x, signed int n) { return ldexpf(x, n); }
double frexp(double x, signed int *e) { return frexpf(x, e); }
double modf(double x, double *ip) { return modff(x, ip); }
double copysign(double x, double y) { return copysignf(x, y); }
double fmax(double x, double y) { return fmaxf(x, y); }
double fmin(double x, double y) { return fminf(x, y); }
double fdim(double x, double y) { return fdimf(x, y); }

/* Sign bit of a CBM float: bit 7 of the first mantissa byte (set for -0.0 too) */
int signbitf(float x) { return (((unsigned char *)&x)[1] & 0x80) != 0; }
int signbit(double x) { return (((unsigned char *)&x)[1] & 0x80) != 0; }
