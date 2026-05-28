// test_issue84_nested_struct_array: nested struct array member access through pointer
// Issue #84: ptr->array_member[n].field was generating a spurious dereference
// instead of address arithmetic, and using wrong element stride.
// Returns 0 (A=$00) on success, non-zero on failure.

struct inner { unsigned char a; unsigned char b; unsigned char c; };
struct outer { struct inner items[4]; unsigned char flag; };

int main() {
    struct outer s;
    struct outer *p;

    p = &s;

    // Write through pointer->array[n].field
    p->items[0].a = 10;
    p->items[0].b = 20;
    p->items[0].c = 30;
    p->items[1].a = 40;
    p->items[1].b = 50;
    p->items[2].a = 60;
    p->items[3].c = 70;
    p->flag = 0xFF;

    // Verify via direct struct access
    if (s.items[0].a != 10) return 1;
    if (s.items[0].b != 20) return 2;
    if (s.items[0].c != 30) return 3;
    if (s.items[1].a != 40) return 4;
    if (s.items[1].b != 50) return 5;
    if (s.items[2].a != 60) return 6;
    if (s.items[3].c != 70) return 7;
    if (s.flag != 0xFF) return 8;

    // Write via direct access, read through pointer
    s.items[2].b = 80;
    s.items[2].c = 90;
    if (p->items[2].b != 80) return 9;
    if (p->items[2].c != 90) return 10;

    return 0;
}
