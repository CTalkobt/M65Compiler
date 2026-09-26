/* test_midi_synth_simple.c — Simple MIDI Synthesizer Test
 *
 * Standalone program that plays a melody using MIDI-to-SID routing.
 * No screen output - just generates and plays audio.
 *
 * Run from BASIC: SYS $2000
 */

#include <audio_midi.h>
#include <audio_sid.h>
#include <stdlib.h>

/* Simple delay function for timing between notes */
void delay_ms(int milliseconds) {
    for (int i = 0; i < milliseconds * 100; i++) {
        __asm("nop");
    }
}

int main(void) {
    /* Initialize synthesizer */
    sid_synth_t synth = sid_synth_create();
    synth.init(&synth);
    synth.set_volume(&synth, 180);

    /* Initialize MIDI receiver */
    midi_receiver_t receiver = midi_receiver_create();
    receiver.init(&receiver, &synth);

    /* Configure synthesis */
    synth.set_portamento(&synth, 50);
    for (int v = 0; v < 3; v++) {
        synth.set_cutoff(&synth, v, 8000);
        synth.set_resonance(&synth, v, 100);
    }

    /* Play a simple melody: C major scale */
    int melody[] = {60, 62, 64, 65, 67, 69, 71, 72};
    int note_duration = 8820;  /* 0.2 seconds at 44.1 kHz */

    /* ====================================================================
     * Play each note
     * ==================================================================== */
    for (int n = 0; n < 8; n++) {
        int note = melody[n];

        /* Trigger note */
        receiver.router.note_on(&receiver.router, 0, note, 100);
        synth.note_on(&synth, 0, note, 100);

        /* Generate audio for this note duration */
        for (int sample = 0; sample < note_duration; sample++) {
            int audio = synth.get_sample(&synth);
            synth.update(&synth);

            /* If hardware audio output were available, write here:
             * volatile char *audio_out = (volatile char *)0xD701;
             * *audio_out = (audio >> 8) & 0xFF;
             */
        }

        /* Release note */
        synth.note_off(&synth, 0);
        receiver.router.note_off(&receiver.router, 0, note);
    }

    /* ====================================================================
     * Play a chord (C major: C-E-G)
     * ==================================================================== */
    int chord[] = {60, 64, 67};
    int chord_duration = 44100;  /* 1 second */

    for (int v = 0; v < 3; v++) {
        synth.note_on(&synth, v, chord[v], 100);
        receiver.router.note_on(&receiver.router, v, chord[v], 100);
    }

    /* Generate audio for chord */
    for (int sample = 0; sample < chord_duration; sample++) {
        int audio = synth.get_sample(&synth);
        synth.update(&synth);
    }

    /* Release all notes */
    for (int v = 0; v < 3; v++) {
        synth.note_off(&synth, v);
    }

    /* ====================================================================
     * Release and shutdown
     * ==================================================================== */
    receiver.done(&receiver);
    synth.done(&synth);

    return 0;
}
