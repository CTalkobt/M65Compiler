// Integration Test 3: Struct across TUs (caller)
// Expected: $4000 = $1E $AA
#include "test3_struct_across.h"

volatile char *r = (char *)0x4000;

int main() {
    struct Point p;
    p.x = 10;
    p.y = 20;
    r[0] = (char)sum_point(&p);  // 30 = $1E
    r[1] = 0xAA;
    __asm__("brk");
    return 0;
}
