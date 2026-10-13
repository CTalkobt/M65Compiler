#pragma once

// The types behind _Complex / __complex__ and imaginary literals. cc45 adds
// these definitions to any translation unit that uses complex types (see
// ParseStage), so <complex.h> is optional. Complex arithmetic is ordinary
// operator overloading on these structs; IRBuilder converts real operands,
// casts and mixed int/float complex operands to the struct form first.

namespace cc45 {

inline const char* complexBuiltinSource() {
    return R"CC45CPLX(
struct _Complex_int {
    signed int real;
    signed int imag;

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

    struct _Complex_int operator~() {
        struct _Complex_int r;
        r.real = this->real;
        r.imag = 0 - this->imag;
        return r;
    }

    struct _Complex_int operator-() {
        struct _Complex_int r;
        r.real = 0 - this->real;
        r.imag = 0 - this->imag;
        return r;
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

    struct _Complex_float operator~() {
        struct _Complex_float r;
        r.real = this->real;
        r.imag = 0.0 - this->imag;
        return r;
    }

    struct _Complex_float operator-() {
        struct _Complex_float r;
        r.real = 0.0 - this->real;
        r.imag = 0.0 - this->imag;
        return r;
    }
};

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
)CC45CPLX";
}

} // namespace cc45
