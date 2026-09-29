/* hw_audio_mixer.c — Audio Mixer Control Implementation */

#include "hw_audio_mixer.h"
#include <stdint.h>
#include <string.h>

/* Audio mixer registers at $D63C-$D63F */
#define AUDIO_MIXER_BASE 0xD63C
#define MIXER_SID1 (*(volatile unsigned char *)(AUDIO_MIXER_BASE + 0))
#define MIXER_SID2 (*(volatile unsigned char *)(AUDIO_MIXER_BASE + 1))
#define MIXER_SID3 (*(volatile unsigned char *)(AUDIO_MIXER_BASE + 2))
#define MIXER_SID4 (*(volatile unsigned char *)(AUDIO_MIXER_BASE + 3))

typedef struct {
    int target_left, target_right;
    int current_left, current_right;
    int duration, elapsed;
    int active;
} fade_state_t;

static stereo_volume_t volumes[4] = {{8, 8}, {8, 8}, {8, 8}, {8, 8}};
static fade_state_t fades[4] = {{0}};
static int muted[4] = {0};
static int pan_values[4] = {8, 8, 8, 8};

void hw_mixer_init(void) {
    /* Set default volumes: center stereo at half volume */
    for (int i = 0; i < 4; i++) {
        volumes[i].left = 8;
        volumes[i].right = 8;
        muted[i] = 0;
        pan_values[i] = 8;
    }

    MIXER_SID1 = 0x88;
    MIXER_SID2 = 0x88;
    MIXER_SID3 = 0x88;
    MIXER_SID4 = 0x88;
}

void hw_mixer_set_volume(sid_chip_t chip, int left, int right) {
    if (chip < 0 || chip > 3) return;

    if (left < 0) left = 0;
    if (left > 15) left = 15;
    if (right < 0) right = 0;
    if (right > 15) right = 15;

    volumes[chip].left = left;
    volumes[chip].right = right;

    /* Encode as high/low nybbles */
    unsigned char val = ((left & 0x0F) << 4) | (right & 0x0F);

    volatile unsigned char *mixer_reg = (volatile unsigned char *)(AUDIO_MIXER_BASE + chip);
    *mixer_reg = val;
}

stereo_volume_t hw_mixer_get_volume(sid_chip_t chip) {
    stereo_volume_t zero = {0, 0};
    if (chip < 0 || chip > 3) return zero;
    return volumes[chip];
}

void hw_mixer_set_master_volume(int left, int right) {
    for (int i = 0; i < 4; i++) {
        hw_mixer_set_volume((sid_chip_t)i, left, right);
    }
}

void hw_mixer_mute(sid_chip_t chip, int mute) {
    if (chip < 0 || chip > 3) return;
    muted[chip] = mute;

    if (mute) {
        hw_mixer_set_volume(chip, 0, 0);
    } else {
        hw_mixer_set_volume(chip, volumes[chip].left, volumes[chip].right);
    }
}

void hw_mixer_pan(sid_chip_t chip, int pan) {
    if (chip < 0 || chip > 3) return;
    if (pan < 0) pan = 0;
    if (pan > 15) pan = 15;

    pan_values[chip] = pan;

    /* Pan: 0=full left, 8=center, 15=full right */
    int left = (15 - pan) * volumes[chip].left / 15;
    int right = pan * volumes[chip].right / 15;

    hw_mixer_set_volume(chip, left, right);
}

void hw_mixer_fade_to(sid_chip_t chip, int target_left, int target_right, int duration_ms) {
    if (chip < 0 || chip > 3) return;

    fade_state_t *fade = &fades[chip];
    fade->target_left = target_left;
    fade->target_right = target_right;
    fade->current_left = volumes[chip].left;
    fade->current_right = volumes[chip].right;
    fade->duration = duration_ms;
    fade->elapsed = 0;
    fade->active = 1;
}

int hw_mixer_update_fades(int delta_ms) {
    int any_active = 0;

    for (int i = 0; i < 4; i++) {
        fade_state_t *fade = &fades[i];
        if (!fade->active) continue;

        fade->elapsed += delta_ms;
        any_active = 1;

        if (fade->elapsed >= fade->duration) {
            /* Snap to target */
            hw_mixer_set_volume((sid_chip_t)i, fade->target_left, fade->target_right);
            fade->active = 0;
            continue;
        }

        /* Linear interpolation */
        float t = (float)fade->elapsed / (float)fade->duration;
        int new_left = fade->current_left + (int)((fade->target_left - fade->current_left) * t);
        int new_right = fade->current_right + (int)((fade->target_right - fade->current_right) * t);

        hw_mixer_set_volume((sid_chip_t)i, new_left, new_right);
    }

    return any_active;
}

int hw_mixer_is_fading(sid_chip_t chip) {
    if (chip < 0 || chip > 3) return 0;
    return fades[chip].active;
}

void hw_mixer_reset(void) {
    for (int i = 0; i < 4; i++) {
        hw_mixer_set_volume((sid_chip_t)i, 8, 8);
        hw_mixer_mute((sid_chip_t)i, 0);
        hw_mixer_pan((sid_chip_t)i, 8);
        fades[i].active = 0;
    }
}

void hw_mixer_crossfade(sid_chip_t from_chip, sid_chip_t to_chip, int duration_ms) {
    if (from_chip < 0 || from_chip > 3 || to_chip < 0 || to_chip > 3) return;

    /* Fade out from_chip, fade in to_chip */
    hw_mixer_fade_to(from_chip, 0, 0, duration_ms);
    hw_mixer_fade_to(to_chip, 8, 8, duration_ms);
}
