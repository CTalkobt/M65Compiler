// cl45 fixture: mega65-dma-trigger-order. Writing $D700 (or $D705) starts
// the job: an address byte written after it, with no trigger following,
// belongs to no job.
#define POKE(a, v) (*(volatile unsigned char *)(a) = (v))

struct dma_regs { unsigned char addr_lo, addr_hi, addr_bank; };
#define DMA ((volatile struct dma_regs *)0xD700)

void good(void) {
    POKE(0xD702, 0);
    POKE(0xD701, 0x10);
    POKE(0xD700, 0x00);
}

void late(void) {
    POKE(0xD702, 0);
    POKE(0xD700, 0x00);
    POKE(0xD701, 0x10);         // EXPECT mega65-dma-trigger-order
}

void late_struct(void) {
    DMA->addr_lo = 0;
    DMA->addr_bank = 0;         // EXPECT mega65-dma-trigger-order
}

void late_enhanced(unsigned char hi) {
    volatile unsigned char *d = (volatile unsigned char *)0xD700;
    d[5] = 0;
    d[1] = hi;                  // EXPECT mega65-dma-trigger-order
}

// two jobs in a row: the second's address byte is followed by its trigger
void two_jobs(void) {
    POKE(0xD702, 0);
    POKE(0xD701, 0x10);
    POKE(0xD700, 0x00);
    POKE(0xD701, 0x11);
    POKE(0xD700, 0x00);
}

// another block: not followed
void other_block(unsigned char again) {
    POKE(0xD701, 0x10);
    POKE(0xD700, 0x00);
    if (again) {
        POKE(0xD701, 0x11);
        POKE(0xD700, 0x00);
    }
}
