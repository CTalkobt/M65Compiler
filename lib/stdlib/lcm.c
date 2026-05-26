/* lcm.c — Least common multiple for cc45 / MEGA65 */

int abs(int value);
int gcd(int a, int b);

int lcm(int a, int b) {
    if (a == 0 || b == 0) return 0;
    int g = gcd(a, b);
    int result = a / g * b;
    return abs(result);
}
