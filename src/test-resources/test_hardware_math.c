#include <stdio.h>

int main() {
    volatile int a = 10;
    volatile int b = 3;
    int c = a % b;
    
    if (c != 1) return 1;
    
    volatile int x = 100;
    volatile int y = 7;
    int z = x % y;
    if (z != 2) return 2;
    
    return 0;
}
