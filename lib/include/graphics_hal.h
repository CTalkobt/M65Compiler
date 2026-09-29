/* graphics_hal.h — VIC-IV Hardware Abstraction Layer
 *
 * Provides low-level VIC-IV register access, memory banking,
 * graphics buffer management, and clipping utilities for MEGA65.
 *
 * Phase 0 of graphics.h library implementation.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

/* ============================================================================
 * GRAPHICS MEMORY LAYOUT (MEGA65 128KB Extended)
 * ============================================================================ */

/* Standard memory regions (as pointers) */
#define GRAPHICS_SCREEN_RAM         ((unsigned char *)0x0400)  /* Text screen: 40×25 */
#define GRAPHICS_COLOR_RAM          ((unsigned char *)0xD800)  /* Color attributes (1KB) */
#define GRAPHICS_BITMAP_ADDR        ((unsigned char *)0x2000)  /* Bitmap mode data (8KB) */
#define GRAPHICS_MULTICOLOR_ADDR    ((unsigned char *)0x2000)  /* Multicolor mode */

/* Hardware registers */
#define VIC_REGISTERS               ((unsigned char *)0xD000)  /* VIC-IV base */
#define CIA2_REGISTERS              ((unsigned char *)0xDC00)  /* CIA2 base */

/* Extended memory (with banking) */
#define GRAPHICS_BANK_SIZE          0x4000  /* 16KB per bank (typical) */
#define GRAPHICS_MAX_BANKS          8       /* 8 banks = 128KB total */

/* ============================================================================
 * VIC-IV REGISTER DEFINITIONS
 * ============================================================================ */

/* VIC-IV register offsets from $D000 */
#define VICIV_MODE_CONTROL_0        0x11    /* $D011 - Display control */
#define VICIV_MODE_CONTROL_1        0x16    /* $D016 - Control register */
#define VICIV_MODE_CONTROL_2        0x31    /* $D031 - Extended mode register */
#define VICIV_BITMAP_ADDR_0         0x18    /* $D018 - Memory setup */
#define VICIV_SCREEN_ADDR           0x18    /* $D018 - High bits of screen/bitmap */
#define VICIV_BORDER_COLOR          0x20    /* $D020 - Border color */
#define VICIV_BG_COLOR_0            0x21    /* $D021 - Background color */
#define VICIV_BG_COLOR_1            0x22    /* $D022 - Extra background color 1 */
#define VICIV_BG_COLOR_2            0x23    /* $D023 - Extra background color 2 */
#define VICIV_BG_COLOR_3            0x24    /* $D024 - Extra background color 3 */

/* Sprite registers */
#define VICIV_SPRITE_X_BASE         0x00    /* $D000-$D007 - Sprite X positions */
#define VICIV_SPRITE_Y_BASE         0x01    /* $D001-$D008 - Sprite Y positions */
#define VICIV_SPRITE_ENABLE         0x15    /* $D015 - Sprite enable bits */
#define VICIV_SPRITE_EXPAND_X       0x1D    /* $D01D - Sprite X expansion */
#define VICIV_SPRITE_EXPAND_Y       0x17    /* $D017 - Sprite Y expansion */
#define VICIV_SPRITE_PRIORITY       0x1B    /* $D01B - Sprite priority */
#define VICIV_SPRITE_MULTICOLOR     0x1C    /* $D01C - Sprite multicolor */

/* CIA2 register (for bank switching) */
#define CIA2_DDR_A                  0xDC02  /* Data direction register A */
#define CIA2_PORT_A                 0xDC00  /* Port A (bits 0-1 = bank select) */

/* ============================================================================
 * VIC-IV MODE BITS
 * ============================================================================ */

#define VICIV_BMM                   0x20    /* Bitmap mode enable (bit 5 of $D011) */
#define VICIV_MCM                   0x10    /* Multicolor mode (bit 4 of $D016) */
#define VICIV_EXTENDED_TEXT_MODE    0x80    /* Extended text mode (bit 7 of $D031) */
#define VICIV_EXTENDED_ATTR_TEXT    0x40    /* Extended attr text (bit 6 of $D031) */

/* ============================================================================
 * GRAPHICS STATE & CONFIGURATION
 * ============================================================================ */

typedef struct {
    unsigned int screen_addr;       /* Current screen/bitmap address */
    unsigned int color_addr;        /* Current color RAM address */
    unsigned char current_bank;     /* Current bank (0-7) */
    unsigned char bitmap_mode;      /* 1 if in bitmap mode, 0 if text */
    unsigned char multicolor_mode;  /* 1 if multicolor mode */
    int clip_x1, clip_y1;          /* Clip rectangle top-left */
    int clip_x2, clip_y2;          /* Clip rectangle bottom-right */
    int screen_width;              /* Current screen width */
    int screen_height;             /* Current screen height */
} graphics_hal_state_t;

extern graphics_hal_state_t graphics_hal_state;

/* ============================================================================
 * HAL INITIALIZATION & CLEANUP
 * ============================================================================ */

/**
 * graphics_hal_init - Initialize graphics HAL
 *
 * Sets up VIC-IV register access, initializes banking system,
 * and prepares graphics memory regions.
 */
void graphics_hal_init(void);

/**
 * graphics_hal_done - Cleanup graphics HAL
 *
 * Restores VIC-IV to default state and releases resources.
 */
void graphics_hal_done(void);

/* ============================================================================
 * BANK MANAGEMENT
 * ============================================================================ */

/**
 * graphics_set_bank - Switch to specified memory bank
 *
 * Parameters:
 *   bank — Bank number (0-7, 0-15 with extended addressing)
 */
void graphics_set_bank(unsigned char bank);

/**
 * graphics_get_bank - Get current bank
 *
 * Returns:
 *   Current bank number
 */
unsigned char graphics_get_bank(void);

/**
 * graphics_set_display_addr - Set display/screen address in current bank
 *
 * Parameters:
 *   addr — Address within bank ($0000-$3FFF typical)
 */
void graphics_set_display_addr(unsigned int addr);

/**
 * graphics_set_bitmap_addr - Set bitmap data address
 *
 * Parameters:
 *   addr — Address of 8KB bitmap data
 */
void graphics_set_bitmap_addr(unsigned int addr);

/**
 * graphics_set_color_addr - Set color RAM address
 *
 * Parameters:
 *   addr — Address of 1KB color RAM
 */
void graphics_set_color_addr(unsigned int addr);

/* ============================================================================
 * MODE CONTROL
 * ============================================================================ */

/**
 * graphics_enable_bitmap_mode - Switch to bitmap mode
 *
 * Enables VIC-IV bitmap mode ($2000 bitmap, $D800 color).
 * Also sets screen width/height for active mode.
 */
void graphics_enable_bitmap_mode(void);

/**
 * graphics_disable_bitmap_mode - Switch to text mode
 *
 * Restores text mode with normal screen/color RAM.
 */
void graphics_disable_bitmap_mode(void);

/**
 * graphics_enable_multicolor - Enable multicolor mode
 */
void graphics_enable_multicolor(void);

/**
 * graphics_disable_multicolor - Disable multicolor mode
 */
void graphics_disable_multicolor(void);

/**
 * graphics_enable_extended_text - Enable 80-column text mode
 *
 * Requires VIC-IV bit 7 of $D031 and appropriate screen RAM.
 */
void graphics_enable_extended_text(void);

/**
 * graphics_disable_extended_text - Disable 80-column mode
 */
void graphics_disable_extended_text(void);

/* ============================================================================
 * BUFFER ACCESS
 * ============================================================================ */

/**
 * graphics_get_screen_buffer - Get pointer to screen/bitmap RAM
 *
 * Returns pointer to graphics display buffer (may be banked).
 *
 * Returns:
 *   Pointer to screen/bitmap data
 */
unsigned char *graphics_get_screen_buffer(void);

/**
 * graphics_get_color_buffer - Get pointer to color RAM
 *
 * Returns:
 *   Pointer to color attribute data
 */
unsigned char *graphics_get_color_buffer(void);

/**
 * graphics_fill_rect - Fast fill of memory region
 *
 * Fills a memory region starting at addr with len bytes of value val.
 * Optimized assembly implementation for graphics buffers.
 *
 * Parameters:
 *   addr — Starting address
 *   len  — Number of bytes to fill
 *   val  — Byte value to write
 */
void graphics_fill_rect(unsigned int addr, unsigned int len, unsigned char val);

/**
 * graphics_copy_rect - Copy memory region
 *
 * Copies len bytes from src to dest. Uses optimized ASM.
 *
 * Parameters:
 *   dest — Destination address
 *   src  — Source address
 *   len  — Number of bytes to copy
 */
void graphics_copy_rect(unsigned int dest, unsigned int src, unsigned int len);

/* ============================================================================
 * CLIPPING & BOUNDS CHECKING
 * ============================================================================ */

/**
 * graphics_set_clip - Set clipping rectangle
 *
 * All drawing operations will be clipped to this rectangle.
 *
 * Parameters:
 *   x1, y1 — Top-left corner
 *   x2, y2 — Bottom-right corner
 */
void graphics_set_clip(int x1, int y1, int x2, int y2);

/**
 * graphics_clear_clip - Clear clipping rectangle
 *
 * Sets clip to full screen (no clipping).
 */
void graphics_clear_clip(void);

/**
 * graphics_clip_x - Clamp X coordinate to clip rectangle
 *
 * Returns:
 *   Clipped X value, or -1 if outside clip region
 */
int graphics_clip_x(int x);

/**
 * graphics_clip_y - Clamp Y coordinate to clip rectangle
 *
 * Returns:
 *   Clipped Y value, or -1 if outside clip region
 */
int graphics_clip_y(int y);

/**
 * graphics_clip_point - Check if point is within clip rectangle
 *
 * Returns:
 *   1 if point is clipped (inside), 0 if outside
 */
int graphics_clip_point(int x, int y);

/**
 * graphics_clip_line - Clip line to clip rectangle
 *
 * Cohen-Sutherland line clipping.
 *
 * Parameters:
 *   x1, y1, x2, y2 — Input line endpoints
 *   cx1, cy1, cx2, cy2 — Output clipped endpoints
 *
 * Returns:
 *   1 if line visible after clipping, 0 if completely outside
 */
int graphics_clip_line(int x1, int y1, int x2, int y2,
                       int *cx1, int *cy1, int *cx2, int *cy2);

/* ============================================================================
 * REGISTER ACCESS (Low-Level)
 * ============================================================================ */

/**
 * graphics_write_reg - Write VIC-IV register
 *
 * Parameters:
 *   reg   — Register offset from $D000
 *   value — Byte value to write
 */
void graphics_write_reg(unsigned char reg, unsigned char value);

/**
 * graphics_read_reg - Read VIC-IV register
 *
 * Parameters:
 *   reg — Register offset from $D000
 *
 * Returns:
 *   Register value
 */
unsigned char graphics_read_reg(unsigned char reg);

