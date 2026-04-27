// Test: Nested structs and local struct variables
// Validates struct member access for both local and nested structs.

struct Inner {
    int value;
    char flag;
};

struct Outer {
    struct Inner a;
    struct Inner b;
    int total;
};

int main() {
    struct Outer obj;
    obj.a.value = 10;
    obj.a.flag = 1;
    obj.b.value = 20;
    obj.b.flag = 0;
    obj.total = obj.a.value + obj.b.value;

    if (obj.a.value != 10) return 1;
    if (obj.a.flag != 1) return 2;
    if (obj.b.value != 20) return 3;
    if (obj.b.flag != 0) return 4;
    if (obj.total != 30) return 5;

    // Modify through member access
    obj.a.value = obj.total;
    if (obj.a.value != 30) return 6;

    return 0;
}
