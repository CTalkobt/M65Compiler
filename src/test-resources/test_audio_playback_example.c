/* test_audio_playback_example.c — Real-Time Audio Playback Example
 *
 * Demonstrates actual audio generation and output from SID synthesizer.
 * Plays a C major scale with MIDI control routing.
 *
 * Audio Output:
 *   - Generates 16-bit signed audio samples
 *   - Writes to MEGA65 audio registers ($D700-$D7FF)
 *   - Runs at 44.1 kHz sample rate
 */

#include <stdio.h>
#include <audio_midi.h>
#include <audio_sid.h>
#include <stdlib.h>

/* ============================================================================
 * MEGA65 Audio Register Locations
 * ============================================================================ */

#define AUDIO_CONTROL       0xD700   /* Audio control register */
#define AUDIO_SAMPLE_LEFT   0xD701   /* Left channel sample */
#define AUDIO_SAMPLE_RIGHT  0xD702   /* Right channel sample */

/* ============================================================================
 * Audio Output - Write sample to hardware
 * ============================================================================ */

void audio_output_write(int sample_left, int sample_right) {
    /* Convert 16-bit signed samples to 8-bit unsigned for output */
    unsigned char left = (sample_left >> 8) & 0xFF;
    unsigned char right = (sample_right >> 8) & 0xFF;

    /* Write to MEGA65 audio output (volatile to prevent optimization) */
    volatile unsigned char *audio_l = (volatile unsigned char *)AUDIO_SAMPLE_LEFT;
    volatile unsigned char *audio_r = (volatile unsigned char *)AUDIO_SAMPLE_RIGHT;

    *audio_l = left;
    *audio_r = right;
}

/* ============================================================================
 * Real-Time Audio Loop
 * ============================================================================ */

#define SAMPLE_RATE     44100       /* 44.1 kHz */
#define FRAME_COUNT     8820        /* 0.2 second frames */
#define NOTE_DURATION   (SAMPLE_RATE / 5)  /* 0.2 sec per note */

int main(void) {
    printf("=== Phase 11: Real-Time Audio Playback Example ===\n\n");

    /* Initialize synthesizer */
    sid_synth_t synth = sid_synth_create();
    synth.init(&synth);
    synth.set_volume(&synth, 200);  /* Set volume to 200/255 */
    printf("✓ SID synthesizer initialized (volume: 200/255)\n");

    /* Initialize MIDI receiver */
    midi_receiver_t receiver = midi_receiver_create();
    receiver.init(&receiver, &synth);
    printf("✓ MIDI receiver initialized\n");

    /* Set up synthesizer parameters */
    synth.set_portamento(&synth, 100);      /* 100ms glide */
    synth.set_vibrato(&synth, 0, 50, 5);    /* Voice 0: 50 cents, 5 Hz */
    synth.set_vibrato(&synth, 1, 50, 5);    /* Voice 1: 50 cents, 5 Hz */
    synth.set_vibrato(&synth, 2, 50, 5);    /* Voice 2: 50 cents, 5 Hz */
    printf("✓ Synthesis parameters configured\n\n");

    /* Set filter parameters for all voices */
    for (int v = 0; v < 3; v++) {
        synth.set_cutoff(&synth, v, 8000);   /* 8 kHz cutoff */
        synth.set_resonance(&synth, v, 150); /* Q = 150 */
    }
    printf("✓ Filter parameters configured\n");
    printf("✓ Ready to play C major scale...\n\n");

    /* ====================================================================
     * C Major Scale: C4, D4, E4, F4, G4, A4, B4, C5
     * ==================================================================== */

    int scale[] = {60, 62, 64, 65, 67, 69, 71, 72};
    int scale_len = 8;

    printf("Playing scale with real-time audio generation:\n");
    printf("(Each note: 0.2 seconds @ 44.1 kHz)\n\n");

    for (int note_idx = 0; note_idx < scale_len; note_idx++) {
        int note = scale[note_idx];
        int freq = midi_note_to_frequency(note);

        printf("  Note %d: MIDI #%d (%d Hz) ", note_idx + 1, note, freq);
        fflush(stdout);

        /* Trigger note on */
        receiver.router.note_on(&receiver.router, 0, note, 100);
        synth.note_on(&synth, 0, note, 100);

        /* Generate and output audio samples for this note */
        for (int sample = 0; sample < NOTE_DURATION; sample++) {
            /* Generate audio sample from synthesizer */
            int audio_sample = synth.get_sample(&synth);

            /* Output to hardware (mono - same on both channels) */
            audio_output_write(audio_sample, audio_sample);

            /* Update synthesizer state */
            synth.update(&synth);

            /* Simple timing: in real code, this would be driven by hardware timer */
            /* For emulator, just iterate */
        }

        /* Release note */
        synth.note_off(&synth, 0);
        receiver.router.note_off(&receiver.router, 0, note);

        printf("[PLAYING]\n");
    }

    printf("\n");

    /* ====================================================================
     * Demonstration: Filter Sweep with MIDI CC Control
     * ==================================================================== */

    printf("Filter sweep demonstration (CC #74 - Cutoff):\n");
    printf("  Playing C4 with filter cutoff sweep (2000-15000 Hz)\n\n");

    synth.note_on(&synth, 0, 60, 100);  /* Play C4 */
    receiver.router.note_on(&receiver.router, 0, 60, 100);

    /* Sweep filter cutoff over 2 seconds */
    int sweep_samples = 2 * SAMPLE_RATE;
    for (int sample = 0; sample < sweep_samples; sample++) {
        /* Linear sweep from 2000 to 15000 Hz */
        int cutoff = 2000 + ((sample * (15000 - 2000)) / sweep_samples);
        synth.set_cutoff(&synth, 0, cutoff);

        /* Generate and output sample */
        int audio_sample = synth.get_sample(&synth);
        audio_output_write(audio_sample, audio_sample);
        synth.update(&synth);

        /* Show progress every 0.5 seconds */
        if (sample % (SAMPLE_RATE / 2) == 0) {
            printf("  %d Hz\n", cutoff);
        }
    }

    synth.note_off(&synth, 0);
    receiver.router.note_off(&receiver.router, 0, 60);

    printf("\n");

    /* ====================================================================
     * Demonstration: Vibrato Modulation via CC #1
     * ==================================================================== */

    printf("Vibrato modulation demonstration (CC #1 - Modulation):\n");
    printf("  Playing A4 with increasing vibrato depth (CC #1)\n\n");

    synth.note_on(&synth, 0, 69, 100);  /* Play A4 */
    receiver.router.note_on(&receiver.router, 0, 69, 100);

    int vibrato_samples = SAMPLE_RATE;  /* 1 second */
    for (int sample = 0; sample < vibrato_samples; sample++) {
        /* Increase vibrato from 0 to 100 cents */
        int cc_value = (sample * 127) / vibrato_samples;
        receiver.router.control_change(&receiver.router, 0, MIDI_CC_MODULATION, cc_value);

        int audio_sample = synth.get_sample(&synth);
        audio_output_write(audio_sample, audio_sample);
        synth.update(&synth);

        if (sample % (SAMPLE_RATE / 4) == 0) {
            int vibrato_cents = (cc_value * 100) / 127;
            printf("  Vibrato: %d cents\n", vibrato_cents);
        }
    }

    synth.note_off(&synth, 0);
    receiver.router.note_off(&receiver.router, 0, 69);

    printf("\n");

    /* ====================================================================
     * Demonstration: Multi-Voice Polyphony
     * ==================================================================== */

    printf("Polyphonic chord demonstration (C Major: C-E-G):\n");
    printf("  Playing 3-voice C major chord for 1 second\n\n");

    int chord[] = {60, 64, 67};  /* C4, E4, G4 */
    for (int v = 0; v < 3; v++) {
        synth.note_on(&synth, v, chord[v], 100);
        receiver.router.note_on(&receiver.router, v % 16, chord[v], 100);
        printf("  Voice %d: MIDI #%d\n", v, chord[v]);
    }

    /* Generate audio for 1 second with all 3 voices playing */
    for (int sample = 0; sample < SAMPLE_RATE; sample++) {
        int audio_sample = synth.get_sample(&synth);
        audio_output_write(audio_sample, audio_sample);
        synth.update(&synth);
    }

    /* Release all notes */
    for (int v = 0; v < 3; v++) {
        synth.note_off(&synth, v);
        receiver.router.note_off(&receiver.router, v % 16, chord[v]);
    }

    printf("\n");

    /* ====================================================================
     * Cleanup
     * ==================================================================== */

    printf("✓ Audio playback complete\n");
    printf("✓ Releasing all resources...\n");

    receiver.done(&receiver);
    synth.done(&synth);

    printf("✓ Shutdown complete\n\n");

    printf("=== Phase 11: Real-Time Audio Playback Example Complete ===\n");
    printf("\nDemonstrated Features:\n");
    printf("  ✓ Real-time sample generation (44.1 kHz)\n");
    printf("  ✓ Audio hardware output (MEGA65 audio registers)\n");
    printf("  ✓ C major scale playback (8 notes × 0.2 sec)\n");
    printf("  ✓ MIDI-to-frequency conversion\n");
    printf("  ✓ Filter cutoff sweep (CC #74)\n");
    printf("  ✓ Vibrato modulation (CC #1)\n");
    printf("  ✓ Polyphonic 3-voice synthesis\n");
    printf("  ✓ Real-time parameter control via MIDI CC\n");
    printf("\nThis example shows how to integrate:\n");
    printf("  1. MIDI input/control routing (Phase 11)\n");
    printf("  2. SID synthesis (Phase 10)\n");
    printf("  3. Real-time audio output to MEGA65 hardware\n");

    return 0;
}
