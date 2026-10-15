#include <dma_controller.h>
#include <dma.h>

/* ============================================================================
 * DMA controller interface
 *
 * The MEGA65's DMA runs job lists in memory (enhanced F018B jobs:
 * lib/stdlib/dma_far.c). A bank and a 16-bit address form the linear
 * address (bank << 16) | addr. The CPU waits while a job runs, so every
 * transfer is complete when the call returns: there is nothing to wait for.
 * ========================================================================== */

#define LINEAR(bank, addr) (((unsigned long)(bank) << 16) | (uint16_t)(addr))

static int g_dma_enabled = 1;
static uint8_t g_dma_status;   /* 0: idle (no job outlives its call) */

int dma_available(void) {
    return 1;   /* every MEGA65 has the DMA controller */
}

void dma_enable(void) {
    g_dma_enabled = 1;
}

/* Disabled: the transfers below do nothing */
void dma_disable(void) {
    g_dma_enabled = 0;
}

int dma_is_busy(void) {
    return 0;
}

void dma_wait(void) {
}

int dma_speed_bytes_per_us(void) {
    return 40;   /* about one byte per 40.5 MHz cycle for a copy */
}

void dma_set_interrupt_enable(int enable) {
    (void)enable;   /* jobs are synchronous */
}

void dma_execute(const dma_request_t* req) {
    if (!g_dma_enabled || !req) return;
    dma_copy_far(LINEAR(req->dst_bank, req->dst_addr), LINEAR(req->src_bank, req->src_addr), req->length);
}

void dma_transfer_smart(
    uint8_t dst_bank, uint16_t dst_addr,
    uint8_t src_bank, uint16_t src_addr,
    size_t len
) {
    if (!g_dma_enabled) return;
    dma_copy_far(LINEAR(dst_bank, dst_addr), LINEAR(src_bank, src_addr), len);
}

/* dma.h's dma_fill macro would rename this function */
#undef dma_fill
void dma_fill(
    uint8_t dst_bank, uint16_t dst_addr,
    uint8_t pattern, size_t len
) {
    if (!g_dma_enabled) return;
    dma_fill_far(LINEAR(dst_bank, dst_addr), len, pattern);
}

uint8_t dma_get_status(void) {
    return g_dma_status;
}

void dma_print_status(void) {
}
