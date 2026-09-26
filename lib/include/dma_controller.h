#ifndef DMA_CONTROLLER_H
#define DMA_CONTROLLER_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================================
 * Phase 29: DMA Controller Interface
 *
 * MEGA65 F018B DMA controller for hardware-accelerated bulk transfers
 * ========================================================================== */

/* ============================================================================
 * DMA Request Structure
 * ========================================================================== */

typedef struct {
    uint16_t src_addr;      /* Source address (within bank, 0x0000-0xFFFF) */
    uint8_t src_bank;       /* Source bank (0-7) */
    uint16_t dst_addr;      /* Destination address (within bank) */
    uint8_t dst_bank;       /* Destination bank (0-7) */
    uint16_t length;        /* Bytes to transfer (0-65535) */
} dma_request_t;

/* ============================================================================
 * DMA Operations
 * ========================================================================== */

/* Check if DMA controller is available on this MEGA65 */
int dma_available(void);

/* Execute synchronous DMA transfer (blocks until complete) */
void dma_execute(const dma_request_t* req);

/* Check if DMA is currently busy */
int dma_is_busy(void);

/* Wait for current DMA operation to complete */
void dma_wait(void);

/* Get DMA transfer speed (bytes per microsecond) */
int dma_speed_bytes_per_us(void);

/* ============================================================================
 * DMA Configuration
 * ========================================================================== */

/* Enable DMA controller */
void dma_enable(void);

/* Disable DMA controller */
void dma_disable(void);

/* Set interrupt-on-completion flag (future: async support) */
void dma_set_interrupt_enable(int enable);

/* ============================================================================
 * Optimized Transfer Helpers
 * ========================================================================== */

/* Transfer with automatic strategy selection */
void dma_transfer_smart(
    uint8_t dst_bank, uint16_t dst_addr,
    uint8_t src_bank, uint16_t src_addr,
    size_t len
);

/* Fill memory with pattern */
void dma_fill(
    uint8_t dst_bank, uint16_t dst_addr,
    uint8_t pattern, size_t len
);

/* ============================================================================
 * Debug and Diagnostics
 * ========================================================================== */

/* Get current DMA status */
uint8_t dma_get_status(void);

/* Print DMA register state (debug) */
void dma_print_status(void);

/* ============================================================================
 * Constants
 * ========================================================================== */

/* Minimum transfer size to justify DMA overhead */
#define DMA_THRESHOLD 64

/* Maximum transfer size per request */
#define DMA_MAX_SIZE 65536

/* DMA register base address */
#define DMA_BASE 0xDE00

#endif
