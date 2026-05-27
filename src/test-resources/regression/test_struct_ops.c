// test_struct_ops: struct operations regression tests
// Returns 0 (A=$00) on success, non-zero on failure.

struct Point {
    int x;
    int y;
};

struct Rect {
    struct Point origin;
    struct Point size;
};

struct Point make_point(int x, int y) {
    struct Point p;
    p.x = x;
    p.y = y;
    return p;
}

int area(struct Rect *r) {
    return r->size.x * r->size.y;
}

int main() {
    struct Point p;
    struct Rect r;

    // Basic member access
    p.x = 10;
    p.y = 20;
    if (p.x != 10) return 1;
    if (p.y != 20) return 2;

    // Struct return
    p = make_point(30, 40);
    if (p.x != 30) return 3;
    if (p.y != 40) return 4;

    // Nested struct
    r.origin.x = 0;
    r.origin.y = 0;
    r.size.x = 100;
    r.size.y = 50;
    if (r.origin.x != 0) return 5;
    if (r.size.y != 50) return 6;

    // Pointer to struct
    if (area(&r) != 5000) return 7;

    // Array of structs
    struct Point pts[4];
    int i;
    for (i = 0; i < 4; i++) {
        pts[i].x = i * 10;
        pts[i].y = i * 20;
    }
    if (pts[2].x != 20) return 8;
    if (pts[3].y != 60) return 9;

    // Struct assignment (copy)
    struct Point q;
    q = pts[1];
    if (q.x != 10) return 10;
    if (q.y != 20) return 11;

    return 0;
}
