// Test: C99 flexible array members
struct Packet {
    int len;
    char data[];
};

struct IntBuf {
    int count;
    int values[];
};

int main() {
    // sizeof excludes the flexible array member
    int sz = sizeof(struct Packet);  // should be 2 (just len)

    // Access via pointer (typical usage with malloc or overlay)
    struct Packet *p = (struct Packet *)0x4000;
    p->len = 3;
    p->data[0] = 'a';
    p->data[1] = 'b';
    p->data[2] = 'c';

    // 16-bit element flexible array
    struct IntBuf *b = (struct IntBuf *)0x4100;
    b->count = 2;
    b->values[0] = 100;
    b->values[1] = 200;

    int result = p->data[0] + b->values[1];
    return sz + result;
}
