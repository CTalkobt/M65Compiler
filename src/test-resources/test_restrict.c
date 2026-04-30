// Test: restrict qualifier (C99)
// All three spelling variants must parse and compile correctly.
// restrict is a hint only — it does not change codegen.

void copy(int * restrict dst, const int * restrict src) {
    *dst = *src;
}

void fill(int * __restrict dst, int val) {
    *dst = val;
}

void swap(int * __restrict__ a, int * __restrict__ b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

// restrict as base-type qualifier (before type)
void zero(restrict int *p) {
    *p = 0;
}

// restrict combined with const and volatile
void mixed(const int * restrict volatile p) {
    int x = *p;
}

// restrict on local variable declaration
int use_local(void) {
    int val = 42;
    int * restrict p = &val;
    return *p;
}

int main(void) {
    int a = 10;
    int b = 20;

    copy(&b, &a);
    if (b != 10) return 1;

    fill(&a, 99);
    if (a != 99) return 2;

    swap(&a, &b);
    if (a != 10) return 3;
    if (b != 99) return 4;

    zero(&a);
    if (a != 0) return 5;

    int c = use_local();
    if (c != 42) return 6;

    return 0;
}
