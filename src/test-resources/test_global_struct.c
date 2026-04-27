// Test: Struct member access and pointer arrow access within one function
// Validates dot and arrow operators on local structs.

struct Data {
    char id;
    int value;
    int flags;
};

int main() {
    struct Data d;
    d.id = 5;
    d.value = 500;
    d.flags = 99;

    if (d.id != 5) return 1;
    if (d.value != 500) return 2;
    if (d.flags != 99) return 3;

    // Arrow access through pointer
    struct Data *p = &d;
    p->value = 1000;
    if (d.value != 1000) return 4;

    // Compute from members
    d.flags = d.value + d.id;
    if (d.flags != 1005) return 5;

    return 0;
}
