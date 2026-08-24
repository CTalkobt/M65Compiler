/* graphics_sprites_hw.c — MEGA65 Hardware Sprite Support (Phase 103c)
 *
 * Direct VIC-IV sprite register access for MEGA65.
 * Enables fast sprite rendering using hardware acceleration.
 *
 * Hardware sprites: 8 available, 8×8 to 64×64 pixels, 16-color.
 */

#include <graphics.h>
#include <string.h>

#ifdef __MEGA65__

/* ============================================================================
 * MEGA65 HARDWARE SPRITE RESOURCES
 * ============================================================================ */

#define VIC_BASE            0xD000
#define VIC_SPRITE_X(n)     (VIC_BASE + (n) * 2)      /* $D000-$D00F */
#define VIC_SPRITE_Y(n)     (VIC_BASE + 1 + (n) * 2)
#define VIC_SPRITE_XMSB     (VIC_BASE + 0x10)         /* $D010 */
#define VIC_SPRITE_ENABLE   (VIC_BASE + 0x15)         /* $D015 */
#define VIC_SPRITE_XEXP     (VIC_BASE + 0x1D)         /* $D01D */
#define VIC_SPRITE_YEXP     (VIC_BASE + 0x17)         /* $D017 */
#define VIC_SPRITE_PRI      (VIC_BASE + 0x1B)         /* $D01B */
#define VIC_SPRITE_COLORS   (VIC_BASE + 0x27)         /* $D027-$D02E */

#define SPRITE_AREA_BASE    0x1000                    /* Sprite bitmap storage */
#define SPRITE_SIZE         64                        /* 64 bytes per 8×8 sprite */

/* Hardware sprite allocation tracking */
static sprite_t *hw_sprites[8];
static unsigned char hw_sprite_count = 0;

/* ============================================================================
 * HARDWARE SPRITE ALLOCATION
 * ============================================================================ */

int sprite_alloc_hw(sprite_t *spr) {
    if (hw_sprite_count >= 8) {
        return -1;  /* No hardware sprites available */
    }

    unsigned char sprite_num = hw_sprite_count++;
    spr->sprite_num = sprite_num;
    hw_sprites[sprite_num] = spr;

    /* Enable this sprite in VIC-IV */
    volatile unsigned char *sprite_enable = (volatile unsigned char *)VIC_SPRITE_ENABLE;
    *sprite_enable |= (1 << sprite_num);

    return 0;
}

void sprite_free_hw(sprite_t *spr) {
    if (spr->sprite_num >= 8) return;

    unsigned char sprite_num = spr->sprite_num;

    /* Disable this sprite */
    volatile unsigned char *sprite_enable = (volatile unsigned char *)VIC_SPRITE_ENABLE;
    *sprite_enable &= ~(1 << sprite_num);

    /* Clear from tracking */
    hw_sprites[sprite_num] = NULL;
}

/* ============================================================================
 * HARDWARE SPRITE RENDERING
 * ============================================================================ */

void sprite_upload_bitmap_hw(sprite_t *spr) {
    if (!spr->bitmap || spr->sprite_num >= 8) return;

    /* Calculate sprite area address */
    unsigned char *sprite_area = (unsigned char *)SPRITE_AREA_BASE +
                                 (spr->sprite_num * SPRITE_SIZE);

    /* Copy bitmap data to sprite area
     * For 8×8 sprites: 64 bytes (8 rows × 8 bytes)
     * For larger sprites: depends on resolution */
    int sprite_data_size = SPRITE_SIZE;  /* Start with 8×8 */
    if (spr->width > 8 || spr->height > 8) {
        sprite_data_size = (spr->width / 8) * (spr->height / 8) * 64;
    }

    memcpy(sprite_area, spr->bitmap, sprite_data_size);

    /* Set sprite pointer in $07F8-$07FF */
    volatile unsigned char *sprite_ptr = (volatile unsigned char *)(0x07F8 + spr->sprite_num);
    *sprite_ptr = (sprite_area / 64) & 0xFF;
}

void sprite_set_color_hw(sprite_t *spr) {
    if (spr->sprite_num >= 8) return;

    volatile unsigned char *sprite_color = (volatile unsigned char *)VIC_SPRITE_COLORS;
    sprite_color[spr->sprite_num] = spr->color & 0x0F;
}

static void sprite_set_position_hw(sprite_t *spr) {
    if (spr->sprite_num >= 8) return;

    volatile unsigned char *sprite_x = (volatile unsigned char *)VIC_SPRITE_X(spr->sprite_num);
    volatile unsigned char *sprite_y = (volatile unsigned char *)VIC_SPRITE_Y(spr->sprite_num);
    volatile unsigned char *sprite_xmsb = (volatile unsigned char *)VIC_SPRITE_XMSB;

    /* Set X position (low byte) */
    *sprite_x = spr->x & 0xFF;

    /* Set X position (high bit) if X > 255 */
    if (spr->x > 255) {
        *sprite_xmsb |= (1 << spr->sprite_num);
    } else {
        *sprite_xmsb &= ~(1 << spr->sprite_num);
    }

    /* Set Y position */
    *sprite_y = spr->y & 0xFF;
}

void sprite_render_hw(sprite_t *spr) {
    sprite_set_position_hw(spr);
    sprite_set_color_hw(spr);

    /* Enable sprite */
    volatile unsigned char *sprite_enable = (volatile unsigned char *)VIC_SPRITE_ENABLE;
    *sprite_enable |= (1 << spr->sprite_num);
}

void sprite_erase_hw(sprite_t *spr) {
    if (spr->sprite_num >= 8) return;

    /* Disable sprite */
    volatile unsigned char *sprite_enable = (volatile unsigned char *)VIC_SPRITE_ENABLE;
    *sprite_enable &= ~(1 << spr->sprite_num);
}

#else

/* ============================================================================
 * NON-MEGA65 STUBS
 * ============================================================================ */

int sprite_alloc_hw(sprite_t *spr) {
    (void)spr;
    return -1;  /* No hardware available */
}

void sprite_free_hw(sprite_t *spr) {
    (void)spr;
}

void sprite_upload_bitmap_hw(sprite_t *spr) {
    (void)spr;
}

void sprite_set_color_hw(sprite_t *spr) {
    (void)spr;
}

void sprite_render_hw(sprite_t *spr) {
    (void)spr;
}

void sprite_erase_hw(sprite_t *spr) {
    (void)spr;
}

#endif  /* __MEGA65__ */
