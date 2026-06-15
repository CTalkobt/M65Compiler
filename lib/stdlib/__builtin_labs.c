long __builtin_labs(long x) {
    return x < 0 ? -x : x;
}
