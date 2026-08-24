/* palette.c — Color Palette Management for MEGA65
 *
 * Implements palette control functions using VIC-IV hardware.
 * Supports palette banks, color loading, fade effects, and color transformations.
 */

#include <palette.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * HARDWARE DEFINITIONS
 * ========================================================================== */

/* VIC-IV palette registers (each color = 3 bytes: R, G, B) */
#define VIC4_PALETTE_BASE   0xD100  /* Palette RAM base address */
#define COLORS_PER_BANK     16
#define BYTES_PER_COLOR     3

/* ============================================================================
 * STATIC STATE
 * ========================================================================== */

static palette_bank_t current_bank = PALETTE_BANK_0;
static palette_t current_palette = {0};

/* ============================================================================
 * INTERNAL HELPERS
 * ========================================================================== */

/**
 * _get_palette_address - Calculate VIC-IV palette address
 */
static unsigned int _get_palette_address(palette_bank_t bank, int color_index) {
    return VIC4_PALETTE_BASE + (bank * COLORS_PER_BANK * BYTES_PER_COLOR) +
           (color_index * BYTES_PER_COLOR);
}

/**
 * _read_hardware_color - Read color from VIC-IV hardware
 */
static rgb_color_t _read_hardware_color(palette_bank_t bank, int index) {
    rgb_color_t color = {0, 0, 0};
    unsigned int addr = _get_palette_address(bank, index);

    color.r = *(volatile unsigned char *)addr;
    color.g = *(volatile unsigned char *)(addr + 1);
    color.b = *(volatile unsigned char *)(addr + 2);

    return color;
}

/**
 * _write_hardware_color - Write color to VIC-IV hardware
 */
static void _write_hardware_color(palette_bank_t bank, int index,
                                   rgb_color_t color) {
    unsigned int addr = _get_palette_address(bank, index);

    *(volatile unsigned char *)addr = color.r;
    *(volatile unsigned char *)(addr + 1) = color.g;
    *(volatile unsigned char *)(addr + 2) = color.b;
}

/**
 * _clamp_uint8 - Clamp value to 0-255 range
 */
static uint8_t _clamp_uint8(int value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (uint8_t)value;
}

/* ============================================================================
 * PALETTE BANK MANAGEMENT
 * ========================================================================== */

int palette_select_bank(palette_bank_t bank) {
    if (bank > 3) {
        return 0;  /* Only 4 banks (0-3) available */
    }

    current_bank = bank;

    /* Load palette from hardware into cache */
    for (int i = 0; i < COLORS_PER_BANK; ++i) {
        current_palette.colors[i] = _read_hardware_color(bank, i);
    }

    return 1;
}

palette_bank_t palette_get_bank(void) {
    return current_bank;
}

int palette_get_color(int index, rgb_color_t *out_color) {
    if (index < 0 || index >= COLORS_PER_BANK || !out_color) {
        return 0;
    }

    *out_color = current_palette.colors[index];
    return 1;
}

void palette_set_color(int index, uint8_t r, uint8_t g, uint8_t b) {
    if (index < 0 || index >= COLORS_PER_BANK) {
        return;
    }

    rgb_color_t color = {r, g, b};
    current_palette.colors[index] = color;

    /* Write to hardware */
    _write_hardware_color(current_bank, index, color);
}

void palette_set_color_rgb(int index, rgb_color_t color) {
    palette_set_color(index, color.r, color.g, color.b);
}

/* ============================================================================
 * PRESET PALETTES
 * ========================================================================== */

static const palette_t palette_standard = {
    /* C64 standard palette */
    {{0, 0, 0},        /* 0: Black */
     {255, 255, 255},  /* 1: White */
     {136, 0, 0},      /* 2: Red */
     {170, 255, 238},  /* 3: Cyan */
     {204, 85, 204},   /* 4: Magenta */
     {85, 170, 85},    /* 5: Green */
     {85, 85, 170},    /* 6: Blue */
     {238, 238, 119},  /* 7: Yellow */
     {221, 136, 85},   /* 8: Orange */
     {102, 68, 0},     /* 9: Brown */
     {255, 119, 119},  /* 10: Light Red */
     {51, 51, 51},     /* 11: Dark Gray */
     {119, 119, 119},  /* 12: Medium Gray */
     {170, 255, 102},  /* 13: Light Green */
     {170, 170, 255},  /* 14: Light Blue */
     {187, 187, 187}}  /* 15: Light Gray */
};

static const palette_t palette_grayscale = {
    /* 16 shades of gray from black to white */
    {{0, 0, 0},           /* 0 */
     {17, 17, 17},        /* 1 */
     {34, 34, 34},        /* 2 */
     {51, 51, 51},        /* 3 */
     {68, 68, 68},        /* 4 */
     {85, 85, 85},        /* 5 */
     {102, 102, 102},     /* 6 */
     {119, 119, 119},     /* 7 */
     {136, 136, 136},     /* 8 */
     {153, 153, 153},     /* 9 */
     {170, 170, 170},     /* 10 */
     {187, 187, 187},     /* 11 */
     {204, 204, 204},     /* 12 */
     {221, 221, 221},     /* 13 */
     {238, 238, 238},     /* 14 */
     {255, 255, 255}}     /* 15 */
};

static const palette_t palette_dos = {
    /* DOS 16-color palette */
    {{0, 0, 0},           /* 0: Black */
     {0, 0, 170},         /* 1: Blue */
     {0, 170, 0},         /* 2: Green */
     {0, 170, 170},       /* 3: Cyan */
     {170, 0, 0},         /* 4: Red */
     {170, 0, 170},       /* 5: Magenta */
     {170, 85, 0},        /* 6: Brown */
     {170, 170, 170},     /* 7: Light Gray */
     {85, 85, 85},        /* 8: Dark Gray */
     {85, 85, 255},       /* 9: Light Blue */
     {85, 255, 85},       /* 10: Light Green */
     {85, 255, 255},      /* 11: Light Cyan */
     {255, 85, 85},       /* 12: Light Red */
     {255, 85, 255},      /* 13: Light Magenta */
     {255, 255, 85},      /* 14: Yellow */
     {255, 255, 255}}     /* 15: White */
};

static const palette_t palette_apple2 = {
    /* Apple II style palette */
    {{0, 0, 0},           /* 0: Black */
     {97, 0, 166},        /* 1: Magenta */
     {0, 79, 166},        /* 2: Blue */
     {179, 79, 255},      /* 3: Light Blue */
     {0, 166, 79},        /* 4: Green */
     {128, 128, 128},     /* 5: Gray */
     {0, 255, 255},       /* 6: Cyan */
     {198, 255, 255},     /* 7: Light Cyan */
     {166, 79, 0},        /* 8: Brown */
     {255, 102, 0},       /* 9: Orange */
     {192, 192, 192},     /* 10: Light Gray */
     {255, 153, 255},     /* 11: Pink */
     {79, 255, 0},        /* 12: Light Green */
     {255, 255, 0},       /* 13: Yellow */
     {179, 255, 102},     /* 14: Pale Green */
     {255, 255, 255}}     /* 15: White */
};

void palette_load_standard(void) {
    palette_load_custom(palette_standard.colors);
}

void palette_load_grayscale(void) {
    palette_load_custom(palette_grayscale.colors);
}

void palette_load_custom(const rgb_color_t colors[16]) {
    if (!colors) return;

    for (int i = 0; i < COLORS_PER_BANK; ++i) {
        palette_set_color(i, colors[i].r, colors[i].g, colors[i].b);
    }
}

int palette_load_by_name(const char *name) {
    if (!name) return 0;

    if (strcmp(name, "standard") == 0) {
        palette_load_standard();
        return 1;
    } else if (strcmp(name, "grayscale") == 0) {
        palette_load_grayscale();
        return 1;
    } else if (strcmp(name, "dos") == 0) {
        palette_load_custom(palette_dos.colors);
        return 1;
    } else if (strcmp(name, "apple2") == 0) {
        palette_load_custom(palette_apple2.colors);
        return 1;
    }

    return 0;
}

/* ============================================================================
 * PALETTE EFFECTS & TRANSFORMATIONS
 * ========================================================================== */

void palette_fade_in(int speed) {
    if (speed < 1) speed = 1;
    if (speed > 20) speed = 20;

    /* Save target palette */
    palette_t target;
    for (int i = 0; i < COLORS_PER_BANK; ++i) {
        target.colors[i] = current_palette.colors[i];
    }

    /* Fade from black */
    for (int step = 0; step <= speed; ++step) {
        for (int i = 0; i < COLORS_PER_BANK; ++i) {
            uint8_t r = (target.colors[i].r * step) / speed;
            uint8_t g = (target.colors[i].g * step) / speed;
            uint8_t b = (target.colors[i].b * step) / speed;

            palette_set_color(i, r, g, b);
        }

        /* Simple delay (jiffy-based would be more accurate) */
        for (volatile int j = 0; j < 10000; ++j);
    }
}

void palette_fade_out(int speed) {
    if (speed < 1) speed = 1;
    if (speed > 20) speed = 20;

    /* Save current palette */
    palette_t original;
    for (int i = 0; i < COLORS_PER_BANK; ++i) {
        original.colors[i] = current_palette.colors[i];
    }

    /* Fade to black */
    for (int step = speed; step >= 0; --step) {
        for (int i = 0; i < COLORS_PER_BANK; ++i) {
            uint8_t r = (original.colors[i].r * step) / speed;
            uint8_t g = (original.colors[i].g * step) / speed;
            uint8_t b = (original.colors[i].b * step) / speed;

            palette_set_color(i, r, g, b);
        }

        /* Simple delay */
        for (volatile int j = 0; j < 10000; ++j);
    }
}

void palette_rotate(int start, int end, int direction) {
    if (start < 0) start = 0;
    if (end >= COLORS_PER_BANK) end = COLORS_PER_BANK - 1;
    if (start > end) return;

    rgb_color_t temp;

    if (direction > 0) {
        /* Rotate forward: last color moves to first */
        temp = current_palette.colors[end];
        for (int i = end; i > start; --i) {
            current_palette.colors[i] = current_palette.colors[i - 1];
            _write_hardware_color(current_bank, i, current_palette.colors[i]);
        }
        current_palette.colors[start] = temp;
        _write_hardware_color(current_bank, start, temp);
    } else {
        /* Rotate backward: first color moves to last */
        temp = current_palette.colors[start];
        for (int i = start; i < end; ++i) {
            current_palette.colors[i] = current_palette.colors[i + 1];
            _write_hardware_color(current_bank, i, current_palette.colors[i]);
        }
        current_palette.colors[end] = temp;
        _write_hardware_color(current_bank, end, temp);
    }
}

void palette_interpolate(const palette_t *src, const palette_t *dst,
                         float factor, palette_t *out) {
    if (!src || !dst || !out) return;

    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;

    for (int i = 0; i < COLORS_PER_BANK; ++i) {
        out->colors[i].r = _clamp_uint8(
            (int)(src->colors[i].r * (1.0f - factor) +
                  dst->colors[i].r * factor)
        );
        out->colors[i].g = _clamp_uint8(
            (int)(src->colors[i].g * (1.0f - factor) +
                  dst->colors[i].g * factor)
        );
        out->colors[i].b = _clamp_uint8(
            (int)(src->colors[i].b * (1.0f - factor) +
                  dst->colors[i].b * factor)
        );
    }
}

/* ============================================================================
 * COLOR UTILITY FUNCTIONS
 * ========================================================================== */

rgb_color_t palette_invert_color(rgb_color_t color) {
    return (rgb_color_t){
        .r = 255 - color.r,
        .g = 255 - color.g,
        .b = 255 - color.b
    };
}

rgb_color_t palette_desaturate(rgb_color_t color) {
    /* Standard luminance formula: 0.299R + 0.587G + 0.114B */
    uint8_t gray = _clamp_uint8(
        (int)(0.299f * color.r + 0.587f * color.g + 0.114f * color.b)
    );
    return (rgb_color_t){.r = gray, .g = gray, .b = gray};
}

rgb_color_t palette_brighten(rgb_color_t color, float factor) {
    return (rgb_color_t){
        .r = _clamp_uint8((int)(color.r * factor)),
        .g = _clamp_uint8((int)(color.g * factor)),
        .b = _clamp_uint8((int)(color.b * factor))
    };
}

rgb_color_t palette_darken(rgb_color_t color, float factor) {
    return (rgb_color_t){
        .r = _clamp_uint8((int)(color.r * factor)),
        .g = _clamp_uint8((int)(color.g * factor)),
        .b = _clamp_uint8((int)(color.b * factor))
    };
}

/* ============================================================================
 * DEBUG & INSPECTION FUNCTIONS
 * ========================================================================== */

void palette_print_current(void) {
    printf("\n=== Palette Bank %d ===\n\n", current_bank);

    for (int i = 0; i < COLORS_PER_BANK; ++i) {
        printf("Color %2d: RGB(%3d, %3d, %3d)\n",
               i,
               current_palette.colors[i].r,
               current_palette.colors[i].g,
               current_palette.colors[i].b);
    }

    printf("\n");
}

void palette_dump_bank(palette_bank_t bank) {
    palette_t temp;

    /* Temporarily read the requested bank */
    for (int i = 0; i < COLORS_PER_BANK; ++i) {
        temp.colors[i] = _read_hardware_color(bank, i);
    }

    printf("\n=== Palette Bank %d ===\n\n", bank);

    for (int i = 0; i < COLORS_PER_BANK; ++i) {
        printf("Color %2d: RGB(%3d, %3d, %3d)\n",
               i,
               temp.colors[i].r,
               temp.colors[i].g,
               temp.colors[i].b);
    }

    printf("\n");
}
