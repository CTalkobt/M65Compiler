/* test_sid_screen.c — SID Synthesizer Test with Screen Output
 *
 * Prints debug output directly to MEGA65 screen RAM ($0400-$07FF)
 * so we can actually see what's happening.
 */

#include <audio_sid.h>
#include <stdlib.h>

/* Screen RAM location for MEGA65 */
#define SCREEN_RAM ((volatile unsigned char *)0x0400)
#define SCREEN_WIDTH 40
#define SCREEN_HEIGHT 25

/* Current screen position */
static int screen_x = 0;
static int screen_y = 0;

/* Print a single character to screen */
void screen_putchar(unsigned char c) {
    int pos = screen_y * SCREEN_WIDTH + screen_x;
    if (pos < SCREEN_WIDTH * SCREEN_HEIGHT) {
        SCREEN_RAM[pos] = c;
    }
    screen_x++;
    if (screen_x >= SCREEN_WIDTH) {
        screen_x = 0;
        screen_y++;
    }
}

/* Print a string to screen */
void screen_print(const char *str) {
    while (*str) {
        screen_putchar(*str);
        str++;
    }
}

/* Print a decimal number */
void screen_print_num(int num) {
    if (num < 0) {
        screen_putchar('-');
        num = -num;
    }

    if (num >= 10000) screen_putchar('0' + (num / 10000) % 10);
    if (num >= 1000) screen_putchar('0' + (num / 1000) % 10);
    if (num >= 100) screen_putchar('0' + (num / 100) % 10);
    if (num >= 10) screen_putchar('0' + (num / 10) % 10);
    screen_putchar('0' + (num % 10));
}

/* Newline */
void screen_nl(void) {
    screen_x = 0;
    screen_y++;
}

int main(void) {
    screen_print("SID SYNTH TEST");
    screen_nl();
    screen_print("===============");
    screen_nl();
    screen_nl();

    /* Create and init synthesizer */
    sid_synth_t synth = sid_synth_create();
    synth.init(&synth);
    synth.set_volume(&synth, 200);

    screen_print("Synth created");
    screen_nl();
    screen_nl();

    /* ====================================================================
     * Test 1: Single Note
     * ==================================================================== */

    screen_print("Test 1: C4 Note");
    screen_nl();

    synth.note_on(&synth, 0, 60, 100);

    screen_print("Note on voice 0");
    screen_nl();
    screen_print("Active: ");
    screen_print_num(synth.voice[0].active);
    screen_nl();
    screen_print("Note: ");
    screen_print_num(synth.voice[0].note);
    screen_nl();
    screen_print("Freq: ");
    screen_print_num(synth.voice[0].osc.frequency);
    screen_print("Hz");
    screen_nl();
    screen_nl();

    /* ====================================================================
     * Test 2: Generate Samples
     * ==================================================================== */

    screen_print("Samples:");
    screen_nl();

    int min_val = 32767;
    int max_val = -32768;
    int nonzero = 0;

    for (int i = 0; i < 10; i++) {
        int sample = synth.get_sample(&synth);

        if (i < 5) {  /* Only show first 5 samples on screen */
            screen_print_num(i);
            screen_print(": ");
            screen_print_num(sample);
            screen_nl();
        }

        if (sample != 0) nonzero++;
        if (sample < min_val) min_val = sample;
        if (sample > max_val) max_val = sample;

        synth.update(&synth);
    }

    screen_nl();
    screen_print("Range: ");
    screen_print_num(min_val);
    screen_print(" to ");
    screen_print_num(max_val);
    screen_nl();
    screen_print("Non-zero: ");
    screen_print_num(nonzero);
    screen_print("/10");
    screen_nl();
    screen_nl();

    /* ====================================================================
     * Test 3: Results
     * ==================================================================== */

    if (nonzero > 0) {
        screen_print("RESULT: WORKING!");
        screen_nl();
        screen_print("Audio is generating");
    } else {
        screen_print("RESULT: NOT WORKING");
        screen_nl();
        screen_print("No audio generated");
    }

    screen_nl();
    screen_nl();
    screen_print("Check volume!");

    synth.done(&synth);

    return 0;
}
