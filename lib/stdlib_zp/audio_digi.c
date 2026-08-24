/* audio_digi.c — MEGA65 DIGI Audio Chip Implementation (Phase 9)
 *
 * Digital PCM audio playback system with struct method API.
 * Follows RRB pattern: standalone functions assigned to method pointers.
 */

#include <audio_digi.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * DIGI SYSTEM METHOD IMPLEMENTATIONS (standalone functions)
 * ============================================================================ */

void digi_system_t__init(struct digi_system  *p) {
    p->volume = DIGI_VOLUME_DEFAULT;
    p->frequency = DIGI_FREQ_DEFAULT;
    p->playing = 0;
    p->paused = 0;
    p->current_sample = NULL;
    p->current_sample_length = 0;
    p->current_position = 0;
    p->current_frequency = DIGI_FREQ_DEFAULT;
    p->current_format = DIGI_FORMAT_UNSIGNED_8BIT;
    p->current_looping = 0;
}

void digi_system_t__done(struct digi_system  *p) {
    p->stop(p);
    p->volume = 0;
    p->frequency = 0;
}

int digi_system_t__play_sample(struct digi_system *this, unsigned char *sample, int length,
                               int frequency, digi_format_t format, int looping) {
    if (!sample || length <= 0 || frequency <= 0) return -1;
    if (length > DIGI_MAX_SAMPLE_SIZE) return -1;

    p->current_sample = sample;
    p->current_sample_length = length;
    p->current_position = 0;
    p->current_frequency = frequency;
    p->current_format = format;
    p->current_looping = looping;

    p->playing = 1;
    p->paused = 0;

    return 0;
}

void digi_system_t__stop(struct digi_system  *p) {
    p->playing = 0;
    p->paused = 0;
    p->current_sample = NULL;
    p->current_position = 0;
}

void digi_system_t__pause(struct digi_system  *p) {
    if (p->playing && !p->paused) {
        p->paused = 1;
        p->playing = 0;
    }
}

void digi_system_t__resume(struct digi_system  *p) {
    if (p->current_sample && p->paused) {
        p->paused = 0;
        p->playing = 1;
    }
}

int digi_system_t__is_playing(struct digi_system  *p) {
    return p->playing;
}

int digi_system_t__get_position(struct digi_system  *p) {
    if (!p->playing && !p->paused) return -1;
    return p->current_position;
}

int digi_system_t__seek(struct digi_system *this, int position) {
    if (!p->current_sample || position < 0 || position >= p->current_sample_length) {
        return -1;
    }
    p->current_position = position;
    return 0;
}

void digi_system_t__set_volume(struct digi_system *this, int volume) {
    if (volume < DIGI_VOLUME_MIN) volume = DIGI_VOLUME_MIN;
    if (volume > DIGI_VOLUME_MAX) volume = DIGI_VOLUME_MAX;
    p->volume = volume;
}

int digi_system_t__get_volume(struct digi_system  *p) {
    return p->volume;
}

void digi_system_t__set_frequency(struct digi_system *this, int frequency) {
    if (frequency > 0) {
        p->current_frequency = frequency;
    }
}

int digi_system_t__get_frequency(struct digi_system  *p) {
    return p->current_frequency;
}

void digi_system_t__update(struct digi_system  *p) {
    if (!p->playing || !p->current_sample) return;

    p->current_position++;

    if (p->current_position >= p->current_sample_length) {
        if (p->current_looping) {
            p->current_position = 0;
        } else {
            p->playing = 0;
            p->current_sample = NULL;
        }
    }
}

/* ============================================================================
 * DIGI SAMPLE METHOD IMPLEMENTATIONS (standalone functions)
 * ============================================================================ */

void digi_sample_t__destroy(struct digi_sample  *p) {
    if (!p) return;
    free(p->data);
    free(p);
}

void digi_sample_t__silence(struct digi_sample *this, digi_format_t format) {
    if (!this || !p->data || p->length <= 0) return;

    unsigned char silence_value = (format == DIGI_FORMAT_UNSIGNED_8BIT) ? 0x80 : 0x00;
    memset(p->data, silence_value, p->length);
}

void digi_sample_t__apply_volume(struct digi_sample *this, int volume, digi_format_t format) {
    if (!this || !p->data || p->length <= 0) return;
    if (volume < 0) volume = 0;
    if (volume > 255) volume = 255;

    if (format == DIGI_FORMAT_UNSIGNED_8BIT) {
        for (int i = 0; i < p->length; i++) {
            int centered = (int)p->data[i] - 128;
            int scaled = (centered * volume) / 255;
            p->data[i] = (unsigned char)((scaled + 128) & 0xFF);
        }
    } else {
        for (int i = 0; i < p->length; i++) {
            int val = (int)(signed char)p->data[i];
            int scaled = (val * volume) / 255;
            p->data[i] = (unsigned char)(scaled & 0xFF);
        }
    }
}

void digi_sample_t__reverse(struct digi_sample  *p) {
    if (!this || !p->data || p->length <= 0) return;

    int left = 0;
    int right = p->length - 1;

    while (left < right) {
        unsigned char tmp = p->data[left];
        p->data[left] = p->data[right];
        p->data[right] = tmp;
        left++;
        right--;
    }
}

/* ============================================================================
 * DIGI SYSTEM & SAMPLE CREATION
 * ============================================================================ */

digi_system_t digi_system_create(void) {
    digi_system_t digi;

    digi.volume = DIGI_VOLUME_DEFAULT;
    digi.frequency = DIGI_FREQ_DEFAULT;
    digi.playing = 0;
    digi.paused = 0;
    digi.current_sample = NULL;
    digi.current_sample_length = 0;
    digi.current_position = 0;
    digi.current_frequency = DIGI_FREQ_DEFAULT;
    digi.current_format = DIGI_FORMAT_UNSIGNED_8BIT;
    digi.current_looping = 0;

    digi.init = &digi_system_t__init;
    digi.done = &digi_system_t__done;
    digi.play_sample = &digi_system_t__play_sample;
    digi.stop = &digi_system_t__stop;
    digi.pause = &digi_system_t__pause;
    digi.resume = &digi_system_t__resume;
    digi.is_playing = &digi_system_t__is_playing;
    digi.get_position = &digi_system_t__get_position;
    digi.seek = &digi_system_t__seek;
    digi.set_volume = &digi_system_t__set_volume;
    digi.get_volume = &digi_system_t__get_volume;
    digi.set_frequency = &digi_system_t__set_frequency;
    digi.get_frequency = &digi_system_t__get_frequency;
    digi.update = &digi_system_t__update;

    return digi;
}

struct digi_sample *digi_sample_create(int length) {
    if (length <= 0 || length > DIGI_MAX_SAMPLE_SIZE) return NULL;

    struct digi_sample *sample = (struct digi_sample *)malloc(sizeof(digi_sample_t));
    if (!sample) return NULL;

    sample->data = (unsigned char *)malloc(length);
    if (!sample->data) {
        free(sample);
        return NULL;
    }

    sample->length = length;
    sample->position = 0;
    sample->volume = DIGI_VOLUME_DEFAULT;
    sample->frequency = DIGI_FREQ_DEFAULT;
    sample->format = DIGI_FORMAT_UNSIGNED_8BIT;
    sample->looping = 0;
    sample->playing = 0;

    sample->destroy = &digi_sample_t__destroy;
    sample->silence = &digi_sample_t__silence;
    sample->apply_volume = &digi_sample_t__apply_volume;
    sample->reverse = &digi_sample_t__reverse;

    return sample;
}

/* ============================================================================
 * SAMPLE MIXING UTILITY
 * ============================================================================ */

void digi_sample_mix_static(unsigned char *dest,
                            unsigned char *src1, unsigned char *src2,
                            int length, int mix_level) {
    if (!dest || !src1 || !src2 || length <= 0) return;
    if (mix_level < 0) mix_level = 0;
    if (mix_level > 255) mix_level = 255;

    int reverse_level = 255 - mix_level;

    for (int i = 0; i < length; i++) {
        int mixed = ((int)src1[i] * mix_level + (int)src2[i] * reverse_level) / 255;
        dest[i] = (unsigned char)(mixed & 0xFF);
    }
}
