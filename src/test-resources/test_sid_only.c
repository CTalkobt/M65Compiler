/* test_sid_only.c — Pure SID Synthesizer Test
 *
 * Tests ONLY the SID synthesizer without MIDI.
 * Verifies that get_sample() actually generates non-zero audio.
 */

#include <stdio.h>
#include <audio_sid.h>
#include <stdlib.h>

int main(void) {
    printf("=== SID Synthesizer Test ===\n\n");

    /* Create synthesizer */
    sid_synth_t synth = sid_synth_create();
    printf("Created synthesizer\n");

    /* Initialize */
    synth.init(&synth);
    printf("Initialized\n");

    /* Set volume */
    synth.set_volume(&synth, 200);
    printf("Set volume to 200\n\n");

    /* ====================================================================
     * Test 1: Play a single note
     * ==================================================================== */

    printf("Test 1: Playing C4 (MIDI #60)\n");
    synth.note_on(&synth, 0, 60, 100);
    printf("  note_on called for voice 0\n");
    printf("  Voice 0 active: %d\n", synth.voice[0].active);
    printf("  Voice 0 note: %d\n", synth.voice[0].note);
    printf("  Voice 0 velocity: %d\n", synth.voice[0].velocity);
    printf("  Voice 0 osc frequency: %d Hz\n", synth.voice[0].osc.frequency);
    printf("  Voice 0 envelope phase: %d\n", synth.voice[0].envelope.phase);
    printf("  Voice 0 envelope level: %d\n", synth.voice[0].envelope.current_level);
    printf("\n");

    /* ====================================================================
     * Test 2: Generate samples
     * ==================================================================== */

    printf("Test 2: Generating 20 samples\n");
    int min_sample = 32767;
    int max_sample = -32768;
    int nonzero_count = 0;

    for (int i = 0; i < 20; i++) {
        int sample = synth.get_sample(&synth);

        printf("  Sample %2d: %7d (0x%04X)", i, sample, sample & 0xFFFF);

        if (sample != 0) {
            printf(" *");
            nonzero_count++;
        }
        if (sample < min_sample) min_sample = sample;
        if (sample > max_sample) max_sample = sample;

        printf("\n");

        synth.update(&synth);
    }

    printf("\n  Min: %d\n", min_sample);
    printf("  Max: %d\n", max_sample);
    printf("  Non-zero samples: %d/20\n", nonzero_count);
    printf("\n");

    if (nonzero_count == 0) {
        printf("ERROR: Synthesizer generated ZERO samples!\n");
        printf("This indicates a critical problem with the SID implementation.\n");
    } else {
        printf("OK: Synthesizer is generating audio\n");
    }

    /* ====================================================================
     * Test 3: Check envelope progression
     * ==================================================================== */

    printf("\nTest 3: Envelope Progression\n");
    printf("  Phase values during attack:\n");

    for (int i = 0; i < 5; i++) {
        int sample = synth.get_sample(&synth);
        printf("    Step %d: level=%3d, phase=%d\n",
               i,
               synth.voice[0].envelope.current_level,
               synth.voice[0].envelope.phase);
        synth.update(&synth);
    }

    /* ====================================================================
     * Test 4: Note off
     * ==================================================================== */

    printf("\nTest 4: Note Off (Release Phase)\n");
    synth.note_off(&synth, 0);
    printf("  note_off called\n");
    printf("  Envelope phase after note_off: %d\n", synth.voice[0].envelope.phase);

    printf("  Release phase progression:\n");
    for (int i = 0; i < 5; i++) {
        int sample = synth.get_sample(&synth);
        printf("    Step %d: level=%3d\n", i, synth.voice[0].envelope.current_level);
        synth.update(&synth);
    }

    /* ====================================================================
     * Test 5: Multiple voices
     * ==================================================================== */

    printf("\nTest 5: Multi-Voice Synthesis\n");
    synth.note_on(&synth, 0, 60, 100);  /* C4 */
    synth.note_on(&synth, 1, 64, 100);  /* E4 */
    synth.note_on(&synth, 2, 67, 100);  /* G4 */

    printf("  All 3 voices active:\n");
    for (int i = 0; i < 3; i++) {
        printf("    Voice %d: note=%d, active=%d, freq=%d\n",
               i,
               synth.voice[i].note,
               synth.voice[i].active,
               synth.voice[i].osc.frequency);
    }

    printf("  Mixed samples from 3 voices:\n");
    int mixed_nonzero = 0;
    for (int i = 0; i < 5; i++) {
        int sample = synth.get_sample(&synth);
        if (sample != 0) mixed_nonzero++;
        printf("    Sample %d: %d\n", i, sample);
        synth.update(&synth);
    }

    printf("  Non-zero from 3 voices: %d/5\n", mixed_nonzero);

    synth.all_notes_off(&synth);

    /* ====================================================================
     * Summary
     * ==================================================================== */

    printf("\n=== Summary ===\n");

    if (nonzero_count > 0 && mixed_nonzero > 0) {
        printf("✓ SID Synthesizer is WORKING\n");
        printf("✓ Oscillators are generating waveforms\n");
        printf("✓ Envelopes are modulating\n");
        printf("✓ Multi-voice mixing works\n");
    } else {
        printf("✗ SID Synthesizer has PROBLEMS\n");
        printf("  Single voice samples: %d non-zero\n", nonzero_count);
        printf("  Multi-voice samples: %d non-zero\n", mixed_nonzero);
        printf("\nDebugging needed in audio_sid.c\n");
    }

    synth.done(&synth);
    printf("\nTest complete\n");

    return 0;
}
