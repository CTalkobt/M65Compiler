volatile char *result = (char *)0x4000;

struct Point {
    int x;
    int y;
};

struct Point pts[4] = 0;

void main() {
    int i;

    // Fill with loop
    for (i = 0; i < 4; i++) {
        pts[i].x = i * 10;
        pts[i].y = i * 10 + 1;
    }

    // Read back
    result[0] = pts[0].x;   // 0
    result[1] = pts[1].x;   // 10 = $0A
    result[2] = pts[2].y;   // 21 = $15
    result[3] = pts[3].y;   // 31 = $1F
    result[4] = sizeof(pts); // 4 * 4 = 16 = $10
    result[5] = 0xAA;
}
