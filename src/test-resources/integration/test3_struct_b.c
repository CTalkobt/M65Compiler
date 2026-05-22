// Integration Test 3: Struct across TUs (callee)
#include "test3_struct_across.h"

int sum_point(struct Point *p) {
    return p->x + p->y;
}
