#ifndef _INTWIDE_H
#define _INTWIDE_H

// intwide.h — Wide integer types via operator-overloaded structs
//
// Usage: #include <intwide.h>
//        __int(64) a = {0};
//        __int(64) b = {0};
//        __int(64) c = a + b;

// Runtime function declarations
void __intN_add(char *r, char *a, char *b, int n);
void __intN_sub(char *r, char *a, char *b, int n);
int  __intN_cmp_u(char *a, char *b, int n);
void __intN_neg(char *a, int n);
void __intN_not(char *a, int n);
void __intN_and(char *r, char *a, char *b, int n);
void __intN_or(char *r, char *a, char *b, int n);
void __intN_xor(char *r, char *a, char *b, int n);
void __intN_mul(char *r, char *a, char *b, int n);

struct __int64 {
    char bytes[8];

    struct __int64 operator+(struct __int64 o) {
        struct __int64 r;
        int i = 0; while (i < 8) { r.bytes[i] = this->bytes[i]; i = i + 1; }
        __intN_add(r.bytes, r.bytes, o.bytes, 8);
        return r;
    }
    struct __int64 operator-(struct __int64 o) {
        struct __int64 r;
        int i = 0; while (i < 8) { r.bytes[i] = this->bytes[i]; i = i + 1; }
        __intN_sub(r.bytes, r.bytes, o.bytes, 8);
        return r;
    }
    struct __int64 operator*(struct __int64 o) {
        struct __int64 r;
        __intN_mul(r.bytes, this->bytes, o.bytes, 8);
        return r;
    }
    int operator==(struct __int64 o) {
        return __intN_cmp_u(this->bytes, o.bytes, 8) == 0;
    }
    int operator!=(struct __int64 o) {
        return __intN_cmp_u(this->bytes, o.bytes, 8) != 0;
    }
    struct __int64 operator~() {
        struct __int64 r;
        int i = 0; while (i < 8) { r.bytes[i] = this->bytes[i]; i = i + 1; }
        __intN_not(r.bytes, 8);
        return r;
    }
    struct __int64 operator-() {
        struct __int64 r;
        int i = 0; while (i < 8) { r.bytes[i] = this->bytes[i]; i = i + 1; }
        __intN_neg(r.bytes, 8);
        return r;
    }
};

struct __int128 {
    char bytes[16];

    struct __int128 operator+(struct __int128 o) {
        struct __int128 r;
        int i = 0; while (i < 16) { r.bytes[i] = this->bytes[i]; i = i + 1; }
        __intN_add(r.bytes, r.bytes, o.bytes, 16);
        return r;
    }
    struct __int128 operator-(struct __int128 o) {
        struct __int128 r;
        int i = 0; while (i < 16) { r.bytes[i] = this->bytes[i]; i = i + 1; }
        __intN_sub(r.bytes, r.bytes, o.bytes, 16);
        return r;
    }
    struct __int128 operator*(struct __int128 o) {
        struct __int128 r;
        __intN_mul(r.bytes, this->bytes, o.bytes, 16);
        return r;
    }
    int operator==(struct __int128 o) {
        return __intN_cmp_u(this->bytes, o.bytes, 16) == 0;
    }
    int operator!=(struct __int128 o) {
        return __intN_cmp_u(this->bytes, o.bytes, 16) != 0;
    }
    struct __int128 operator~() {
        struct __int128 r;
        int i = 0; while (i < 16) { r.bytes[i] = this->bytes[i]; i = i + 1; }
        __intN_not(r.bytes, 16);
        return r;
    }
    struct __int128 operator-() {
        struct __int128 r;
        int i = 0; while (i < 16) { r.bytes[i] = this->bytes[i]; i = i + 1; }
        __intN_neg(r.bytes, 16);
        return r;
    }
};

// Aliases
#define __uint64 __int64
#define __uint128 __int128

#endif // _INTWIDE_H
