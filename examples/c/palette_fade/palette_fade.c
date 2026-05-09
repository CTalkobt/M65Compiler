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

// Delay to make fade visible
void delay_frame(void) {
    unsigned int i;
    unsigned int j;
    for (i = 0; i < 10; i++) {
        for (j = 0; j < 3000; j++) { }
    }
}

int main(void) {
    unsigned char *border_color = (unsigned char *)0xD020;
    unsigned char level;

    // DEBUG: Red border = starting (should appear immediately)
    *border_color = 2;

    // Unlock VIC-IV palette registers
    unlock_viciv();

    // DEBUG: Green border = unlock done
    *border_color = 5;

    // Save original palette for restoration
    save_palette();

    // DEBUG: Yellow border = save done, entering fade loop
    *border_color = 7;

    // Oscillating fade loop: 0 → 255 → 0
    while (1) {
        // Fade in: 0 → 255
        *border_color = 3;  // Cyan = fade in starting
        for (level = 0; level < 255; level++) {
            apply_fade(level);
            delay_frame();
            // Show progress: toggle every 16 levels
            if (level & 16) {
                *border_color = 3;  // Cyan
            } else {
                *border_color = 14; // Light gray
            }
        }
        apply_fade(255);
        delay_frame();
        *border_color = 3;  // Cyan at peak fade

        // Fade out: 255 → 0
        *border_color = 4;  // Purple = fade out starting
        for (level = 255; level > 0; level--) {
            apply_fade(level);
            delay_frame();
            // Show progress: toggle every 16 levels
            if (level & 16) {
                *border_color = 4;  // Purple
            } else {
                *border_color = 14; // Light gray
            }
        }
        apply_fade(0);
        delay_frame();
        *border_color = 4;  // Purple at minimum fade
    }

    return 0;
}
