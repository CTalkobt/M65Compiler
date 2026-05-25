// Test DMA intrinsics compilation and assembly
// Verifies __dma_copy and __dma_fill compile correctly

void test_dma_copy(void) {
    unsigned char *src = (unsigned char *)0x2800;
    unsigned char *dst = (unsigned char *)0x2900;
    __dma_copy(dst, src, 256);
}

void test_dma_fill(void) {
    unsigned char *dst = (unsigned char *)0x0400;
    __dma_fill(dst, 1000, 0x20);
}

int main() {
    return 0;
}
