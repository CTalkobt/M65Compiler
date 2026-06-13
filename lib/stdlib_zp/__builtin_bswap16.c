unsigned int __builtin_bswap16(unsigned int x) {
    return ((x >> 8) & 0xFF) | ((x & 0xFF) << 8);
}
