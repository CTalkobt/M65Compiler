#include <stdio.h>

void parent() {
    int x = 42;
    void nested() {
        printf("Nested: x = %d\n", x);
    }
    nested();
}

int main() {
    parent();
    return 0;
}
