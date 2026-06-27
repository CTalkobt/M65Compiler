#ifndef _COMPLEX_H
#define _COMPLEX_H

// cc45 _Complex implementation via operator-overloaded structs
// No FPU — integer complex numbers only (real + imaginary parts)

struct _Complex_int {
    int real;
    int imag;

    struct _Complex_int operator+(struct _Complex_int o) {
        struct _Complex_int r;
        r.real = this->real + o.real;
        r.imag = this->imag + o.imag;
        return r;
    }

    struct _Complex_int operator-(struct _Complex_int o) {
        struct _Complex_int r;
        r.real = this->real - o.real;
        r.imag = this->imag - o.imag;
        return r;
    }

    struct _Complex_int operator*(struct _Complex_int o) {
        struct _Complex_int r;
        r.real = this->real * o.real - this->imag * o.imag;
        r.imag = this->real * o.imag + this->imag * o.real;
        return r;
    }

    int operator==(struct _Complex_int o) {
        return this->real == o.real && this->imag == o.imag;
    }

    int operator!=(struct _Complex_int o) {
        return this->real != o.real || this->imag != o.imag;
    }
};

// C99 _Complex compatibility (integer only)
#ifdef __cc45__
#define _Complex int struct _Complex_int
#endif

// GCC builtins
#define __real__(z) ((z).real)
#define __imag__(z) ((z).imag)

#endif // _COMPLEX_H
