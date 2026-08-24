#include <dma_controller.h>
#include <string.h>
#include <assert.h>

/* ============================================================================
 * Phase 29: DMA Controller Implementation
 * ========================================================================== */

/* MEGA65 F018B DMA Registers */
typedef struct {
    volatile uint8_t command;           /* $DE00 */
    volatile uint8_t count_low;         /* $DE01 */
    volatile uint8_t count_mid;         /* $DE02 */
    volatile uint8_t src_addr_low;      /* $DE03 */
    volatile uint8_t src_addr_mid;      /* $DE04 */
    volatile uint8_t src_addr_high;     /* $DE05 */
    volatile uint8_t dst_addr_low;      /* $DE06 */
    volatile uint8_t dst_addr_mid;      /* $DE07 */
    volatile uint8_t dst_addr_high;     /* $DE08 */
    volatile uint8_t src_bank;          /* $DE09 */
    volatile uint8_t dst_bank;          /* $DE0A */
    volatile uint8_t reserved[5];
    volatile uint8_t list_addr;         /* $DE10 */
    volatile uint8_t list_addr_mid;     /* $DE11 */
    volatile uint8_t list_addr_bank;    /* $DE12 */
} dma_registers_t;

static volatile dma_registers_t* const dma_regs =
    (volatile dma_registers_t*)0xDE00;

/* DMA state tracking */
static int g_dma_available = 1;
static int g_dma_interrupt_enabled = 0;

/* ============================================================================
 * DMA Operations Implementation
 * ========================================================================== */

int dma_available(void) {
    return g_dma_available;
}

void dma_enable(void) {
    /* DMA controller is always available on MEGA65 */
    g_dma_available = 1;
}

void dma_disable(void) {
    g_dma_available = 0;
}

int dma_is_busy(void) {
    /* Check if DMA command register has execute bit set */
    return (dma_regs->command & 0x80) != 0;
}

void dma_wait(void) {
    /* Spin until DMA completes */
    while (dma_is_busy()) {
        /* Busy wait - no CPU instructions to avoid stalling */
    }
}

int dma_speed_bytes_per_us(void) {
    /* F018B DMA runs at ~1 MHz base clock
       Typical throughput: ~1-2 bytes per cycle
       At ~1 MHz: ~1-2 bytes per microsecond */
    return 1;
}

void dma_set_interrupt_enable(int enable) {
    g_dma_interrupt_enabled = enable;
    /* Note: Interrupt handling deferred to Phase 30+ */
}

/* ============================================================================
 * Core DMA Transfer
 * ========================================================================== */

void dma_execute(const dma_request_t* req) {
    assert(req != NULL);
    assert(req->src_bank < 8);
    assert(req->dst_bank < 8);
    assert(req->length > 0);
    assert(req->length <= DMA_MAX_SIZE);

    if (!g_dma_available) {
        return;  /* DMA not available, fallback handled by caller */
    }

    /* Wait for previous transfer to complete */
    dma_wait();

    /* Setup source address and bank */
    dma_regs->src_addr_low = req->src_addr & 0xFF;
    dma_regs->src_addr_mid = (req->src_addr >> 8) & 0xFF;
    dma_regs->src_bank = req->src_bank;

    /* Setup destination address and bank */
    dma_regs->dst_addr_low = req->dst_addr & 0xFF;
    dma_regs->dst_addr_mid = (req->dst_addr >> 8) & 0xFF;
    dma_regs->dst_bank = req->dst_bank;

    /* Setup transfer length (16-bit) */
    dma_regs->count_low = req->length & 0xFF;
    dma_regs->count_mid = (req->length >> 8) & 0xFF;

    /* Start transfer: Copy command (0x00) with enable bit (0x80) */
    /* Command byte: 10000000 = copy + enable */
    dma_regs->command = 0x80;

    /* Transfer initiated; caller may call dma_wait() or check dma_is_busy() */
}

/* ============================================================================
 * Optimized Transfer Helpers
 * ========================================================================== */

void dma_transfer_smart(
    uint8_t dst_bank, uint16_t dst_addr,
    uint8_t src_bank, uint16_t src_addr,
    size_t len
) {
    dma_request_t req = {
        .src_addr = src_addr,
        .src_bank = src_bank,
        .dst_addr = dst_addr,
        .dst_bank = dst_bank,
        .length = len
    };

    dma_execute(&req);
    dma_wait();  /* Synchronous */
}

void dma_fill(
    uint8_t dst_bank, uint16_t dst_addr,
    uint8_t pattern, size_t len
) {
    /* F018B fill operation not yet implemented
       Fallback: use memset in target bank */

    /* Save current bank */
    volatile uint8_t* bank_reg = (volatile uint8_t*)0x01;
    uint8_t saved_bank = *bank_reg;

    /* Switch to destination bank */
    *bank_reg = dst_bank;

    /* Fill memory */
    memset((void*)dst_addr, pattern, len);

    /* Restore bank */
    *bank_reg = saved_bank;
}

/* ============================================================================
 * Debug and Diagnostics
 * ========================================================================== */

uint8_t dma_get_status(void) {
    return dma_regs->command;
}

void dma_print_status(void) {
    /* Debug: print current DMA state */
    /* (Implementation deferred - would need stdio integration) */
}

/* ============================================================================
 * Initialization
 * ========================================================================== */

__attribute__((constructor))
static void dma_init(void) {
    /* Verify DMA is accessible by reading/writing a register */
    volatile uint8_t* test_reg = (volatile uint8_t*)0xDE00;
    uint8_t original = *test_reg;

    /* Attempt to clear the enable bit */
    *test_reg = 0x00;

    /* Verify write succeeded */
    if (*test_reg == 0x00) {
        g_dma_available = 1;
        *test_reg = original;  /* Restore */
    } else {
        g_dma_available = 0;
    }
}
