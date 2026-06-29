#ifndef _COMPLEX_H
#define _COMPLEX_H

// cc45 _Complex implementation via operator-overloaded structs

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

struct _Complex_float {
    float real;
    float imag;

    struct _Complex_float operator+(struct _Complex_float o) {
        struct _Complex_float r;
        r.real = this->real + o.real;
        r.imag = this->imag + o.imag;
        return r;
    }

    struct _Complex_float operator-(struct _Complex_float o) {
        struct _Complex_float r;
        r.real = this->real - o.real;
        r.imag = this->imag - o.imag;
        return r;
    }

    struct _Complex_float operator*(struct _Complex_float o) {
        struct _Complex_float r;
        r.real = this->real * o.real - this->imag * o.imag;
        r.imag = this->real * o.imag + this->imag * o.real;
        return r;
    }

    struct _Complex_float operator/(struct _Complex_float o) {
        struct _Complex_float r;
        float denom = o.real * o.real + o.imag * o.imag;
        r.real = (this->real * o.real + this->imag * o.imag) / denom;
        r.imag = (this->imag * o.real - this->real * o.imag) / denom;
        return r;
    }

    int operator==(struct _Complex_float o) {
        return this->real == o.real && this->imag == o.imag;
    }

    int operator!=(struct _Complex_float o) {
        return this->real != o.real || this->imag != o.imag;
    }
};

// GCC builtins
#define __real__(z) ((z).real)
#define __imag__(z) ((z).imag)

static struct _Complex_float __builtin_conjf(struct _Complex_float z) {
    struct _Complex_float r;
    r.real = z.real;
    r.imag = 0.0 - z.imag;
    return r;
}

static struct _Complex_float __builtin_creal(struct _Complex_float z) {
    struct _Complex_float r;
    r.real = z.real;
    r.imag = 0.0;
    return r;
}

#endif // _COMPLEX_H
