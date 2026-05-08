// Palette fade example for MEGA65
//
// Demonstrates mixed C+Assembly calling conventions with VIC-IV palette.
// Uses ZP calling convention (-fzpcall) for efficient parameter passing.
//
// Color palette oscillates: 0 → 255 → 0
// Assembly routine scales each R/G/B component by fade level (0-256).

// Assembly function declarations
void unlock_viciv(void);
void save_palette(void);
void restore_palette(void);
void apply_fade(unsigned char level);

int main(void) {
    unsigned char *border_color = (unsigned char *)0xD020;

    // Unlock VIC-IV palette registers
    unlock_viciv();

    // Save original palette for restoration
    save_palette();

    // Oscillating fade loop: 0 → 255 → 0
    while (1) {
        unsigned char level;

        // Fade in: 0 → 255
        for (level = 0; level < 255; level++) {
            apply_fade(level);
            *border_color = (level & 0x10) ? 1 : 0;  // Toggle border on/off
        }
        apply_fade(255);
        *border_color = 1;

        // Fade out: 255 → 0
        for (level = 255; level > 0; level--) {
            apply_fade(level);
            *border_color = (level & 0x10) ? 1 : 0;  // Toggle border on/off
        }
        apply_fade(0);
        *border_color = 0;
    }

    return 0;
}
