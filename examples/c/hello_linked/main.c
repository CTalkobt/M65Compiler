#include <stdio.h>

int main() {
    char buf[8];

    buf[0] = '4';
    buf[1] = '2';
    buf[2] = 0;

    puts(buf);
    puts("done");
    return 0;
}
