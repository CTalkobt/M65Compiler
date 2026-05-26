/* min.c — Integer minimum for cc45 / MEGA65 (ZP calling convention) */

int min(int a, int b) {
    return (a < b) ? a : b;
}
