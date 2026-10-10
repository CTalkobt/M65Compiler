#pragma cc45 no_zp_save
/* Float arguments and return values. Expected at $C0C0: 01 01 AA */
#define R(i) (*(volatile unsigned char *)(0xC0C0 + (i)))
float half(float x) { return x * 0.5; }
float add3(float a, float b, float c) { return a + b + c; }
void main() {
    float h = half(3.0);
    R(0) = (h > 1.4 && h < 1.6);
    R(1) = (add3(1.0, 2.0, 4.0) == 7.0);
    R(2) = 0xAA;
}
