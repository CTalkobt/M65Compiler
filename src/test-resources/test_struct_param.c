struct Point {
    int x;
    int y;
};

struct Rect {
    struct Point origin;
    int width;
    int height;
};

void setPoint(struct Point* p, int x, int y) {
    p->x = x;
    p->y = y;
}

int getX(struct Point* p) {
    return p->x;
}

int getY(struct Point* p) {
    return p->y;
}

void initRect(struct Rect* r, int x, int y, int w, int h) {
    r->width = w;
    r->height = h;
    setPoint(&r->origin, x, y);
}

int area(struct Rect* r) {
    return r->width * r->height;
}

void main() {
    struct Point pt;
    setPoint(&pt, 10, 20);
    int px = getX(&pt);
    int py = getY(&pt);

    struct Rect rc;
    initRect(&rc, 5, 5, 100, 50);
    int a = area(&rc);
}
