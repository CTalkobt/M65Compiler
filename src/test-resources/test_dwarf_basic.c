// Phase 113.6: Basic DWARF debug information test
// Tests: DIE generation, line number mapping, debug section creation

int add(int a, int b) {
    int result = a + b;
    return result;
}

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    int fact = n * factorial(n - 1);
    return fact;
}

struct Point {
    int x;
    int y;
};

struct Point make_point(int x, int y) {
    struct Point p;
    p.x = x;
    p.y = y;
    return p;
}

int main(void) {
    int a = 5;
    int b = 3;
    int sum = add(a, b);

    int fact = factorial(5);

    struct Point origin = make_point(0, 0);
    struct Point dest = make_point(10, 20);

    return sum + fact;
}
