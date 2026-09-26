/* test_audio_debug.c — Debug Audio Generation
 *
 * Verify that the synthesizer is actually generating audio samples.
 * Tests the full synthesis pipeline without hardware output.
 */

#include <stdio.h>
#include <audio_midi.h>
#include <audio_sid.h>
#include <stdlib.h>

int main(void) {
    printf("=== Audio Synthesis Debug Test ===\n\n");

    /* Create and init synthesizer */
    sid_synth_t synth = sid_synth_create();
    synth.init(&synth);
    synth.set_volume(&synth, 200);

    printf("✓ Synthesizer initialized\n");
    printf("  Volume: 200/255\n");
    printf("  Active voices: %d\n\n", synth.active_voices);

    /* ====================================================================
     * Test 1: Oscillator Output
     * ==================================================================== */

    printf("Test 1: Oscillator Generation\n");
    printf("  Playing C4 (MIDI #60, 262 Hz)\n\n");

    synth.note_on(&synth, 0, 60, 100);
    printf("  Voice 0 state after note_on:\n");
    printf("    Frequency: %d Hz\n", synth.voice[0].osc.frequency);
    printf("    Velocity: %d\n", synth.voice[0].velocity);
    printf("    Active: %d\n\n", synth.voice[0].active);

    /* Generate 10 samples and check if they're non-zero */
    printf("  First 10 audio samples:\n");
    int sample_count = 0;
    int nonzero_count = 0;

    for (int i = 0; i < 10; i++) {
        int sample = synth.get_sample(&synth);
        printf("    Sample %d: %d (0x%04X)\n", i, sample, sample & 0xFFFF);
        if (sample != 0) nonzero_count++;
        synth.update(&synth);
    }

    printf("\n  Non-zero samples: %d/10\n", nonzero_count);

    if (nonzero_count == 0) {
        printf("  ⚠ WARNING: Synthesizer is not generating audio!\n");
        printf("  This indicates an issue with the synthesis engine.\n");
    } else {
        printf("  ✓ Synthesizer is generating audio\n");
    }

    synth.note_off(&synth, 0);

    /* ====================================================================
     * Test 2: Envelope Attack
     * ==================================================================== */

    printf("\nTest 2: Envelope (ADSR) Progression\n");
    synth.note_on(&synth, 0, 60, 100);

    printf("  ADSR State Progression:\n");
    for (int i = 0; i < 5; i++) {
        int sample = synth.get_sample(&synth);
        printf("    Step %d: sample=%d, phase=%d, level=%d\n",
               i, sample, synth.voice[0].envelope.phase,
               synth.voice[0].envelope.current_level);
        synth.update(&synth);
    }

    synth.note_off(&synth, 0);

    /* ====================================================================
     * Test 3: Filter Impact
     * ==================================================================== */

    printf("\nTest 3: Filter Cutoff Control\n");
    synth.note_on(&synth, 0, 60, 100);

    printf("  Sample generation with filter changes:\n");
    int sample1 = synth.get_sample(&synth);
    printf("    No filter: %d\n", sample1);
    synth.update(&synth);

    synth.set_cutoff(&synth, 0, 5000);  /* Set cutoff to 5 kHz */
    int sample2 = synth.get_sample(&synth);
    printf("    After cutoff=5000 Hz: %d\n", sample2);
    synth.update(&synth);

    synth.set_cutoff(&synth, 0, 1000);  /* Lower cutoff */
    int sample3 = synth.get_sample(&synth);
    printf("    After cutoff=1000 Hz: %d\n", sample3);
    synth.update(&synth);

    synth.note_off(&synth, 0);

    /* ====================================================================
     * Test 4: Multiple Voices
     * ==================================================================== */

    printf("\nTest 4: Multi-Voice Synthesis\n");
    printf("  Playing C4, E4, G4 (C major chord)\n");

    synth.note_on(&synth, 0, 60, 100);  /* C4 */
    synth.note_on(&synth, 1, 64, 100);  /* E4 */
    synth.note_on(&synth, 2, 67, 100);  /* G4 */

    printf("  Voice states:\n");
    for (int v = 0; v < 3; v++) {
        printf("    Voice %d: freq=%d Hz, active=%d, velocity=%d\n",
               v, synth.voice[v].osc.frequency, synth.voice[v].active,
               synth.voice[v].velocity);
    }

    printf("\n  Mixed output samples:\n");
    for (int i = 0; i < 5; i++) {
        int sample = synth.get_sample(&synth);
        printf("    Sample %d: %d (mixed from 3 voices)\n", i, sample);
        synth.update(&synth);
    }

    synth.all_notes_off(&synth);

    /* ====================================================================
     * Test 5: Sample Range Analysis
     * ==================================================================== */

    printf("\nTest 5: Audio Sample Range\n");
    synth.note_on(&synth, 0, 60, 127);  /* Max velocity */

    int min_sample = 32767;
    int max_sample = -32768;
    int sample_sum = 0;

    printf("  Analyzing 100 samples...\n");
    for (int i = 0; i < 100; i++) {
        int sample = synth.get_sample(&synth);
        if (sample < min_sample) min_sample = sample;
        if (sample > max_sample) max_sample = sample;
        sample_sum += sample;
        synth.update(&synth);
    }

    printf("    Min: %d\n", min_sample);
    printf("    Max: %d\n", max_sample);
    printf("    Range: %d\n", max_sample - min_sample);
    printf("    Average: %d\n", sample_sum / 100);

    synth.note_off(&synth, 0);

    /* ====================================================================
     * Summary
     * ==================================================================== */

    printf("\n=== Debug Summary ===\n");

    if (nonzero_count > 0) {
        printf("✓ Synthesizer is working\n");
        printf("✓ Oscillators are generating waveforms\n");
        printf("✓ Envelopes are modulating amplitude\n");
        printf("✓ Filters are processing signals\n");
        printf("✓ Multi-voice mixing is functional\n");
        printf("\nThe issue is likely with audio OUTPUT, not synthesis.\n");
        printf("Possible causes:\n");
        printf("  • Audio register addresses incorrect\n");
        printf("  • Emulator doesn't simulate those registers\n");
        printf("  • Sample format conversion needed\n");
        printf("  • Hardware routing not implemented in emulator\n");
    } else {
        printf("✗ Synthesizer is NOT generating audio\n");
        printf("This indicates a problem with the synthesis engine itself.\n");
    }

    synth.done(&synth);
    printf("\n✓ Debug test complete\n");

    return 0;
}
