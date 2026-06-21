#include <stdio.h>

void parent() {
    int y = 100;
    int x = 42;
    void nested() {
        printf("x=%d, y=%d\n", x, y);
    }
    nested();
}

int main() {
    parent();
    return 0;
}
