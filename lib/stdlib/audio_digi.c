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

void digi_system_t__init(struct digi_system *this) {
    this->volume = DIGI_VOLUME_DEFAULT;
    this->frequency = DIGI_FREQ_DEFAULT;
    this->playing = 0;
    this->paused = 0;
    this->current_sample = NULL;
    this->current_sample_length = 0;
    this->current_position = 0;
    this->current_frequency = DIGI_FREQ_DEFAULT;
    this->current_format = DIGI_FORMAT_UNSIGNED_8BIT;
    this->current_looping = 0;
}

void digi_system_t__done(struct digi_system *this) {
    this->stop(this);
    this->volume = 0;
    this->frequency = 0;
}

int digi_system_t__play_sample(struct digi_system *this, unsigned char *sample, int length,
                               int frequency, digi_format_t format, int looping) {
    if (!sample || length <= 0 || frequency <= 0) return -1;
    if (length > DIGI_MAX_SAMPLE_SIZE) return -1;

    this->current_sample = sample;
    this->current_sample_length = length;
    this->current_position = 0;
    this->current_frequency = frequency;
    this->current_format = format;
    this->current_looping = looping;

    this->playing = 1;
    this->paused = 0;

    return 0;
}

void digi_system_t__stop(struct digi_system *this) {
    this->playing = 0;
    this->paused = 0;
    this->current_sample = NULL;
    this->current_position = 0;
}

void digi_system_t__pause(struct digi_system *this) {
    if (this->playing && !this->paused) {
        this->paused = 1;
        this->playing = 0;
    }
}

void digi_system_t__resume(struct digi_system *this) {
    if (this->current_sample && this->paused) {
        this->paused = 0;
        this->playing = 1;
    }
}

int digi_system_t__is_playing(struct digi_system *this) {
    return this->playing;
}

int digi_system_t__get_position(struct digi_system *this) {
    if (!this->playing && !this->paused) return -1;
    return this->current_position;
}

int digi_system_t__seek(struct digi_system *this, int position) {
    if (!this->current_sample || position < 0 || position >= this->current_sample_length) {
        return -1;
    }
    this->current_position = position;
    return 0;
}

void digi_system_t__set_volume(struct digi_system *this, int volume) {
    if (volume < DIGI_VOLUME_MIN) volume = DIGI_VOLUME_MIN;
    if (volume > DIGI_VOLUME_MAX) volume = DIGI_VOLUME_MAX;
    this->volume = volume;
}

int digi_system_t__get_volume(struct digi_system *this) {
    return this->volume;
}

void digi_system_t__set_frequency(struct digi_system *this, int frequency) {
    if (frequency > 0) {
        this->current_frequency = frequency;
    }
}

int digi_system_t__get_frequency(struct digi_system *this) {
    return this->current_frequency;
}

void digi_system_t__update(struct digi_system *this) {
    if (!this->playing || !this->current_sample) return;

    this->current_position++;

    if (this->current_position >= this->current_sample_length) {
        if (this->current_looping) {
            this->current_position = 0;
        } else {
            this->playing = 0;
            this->current_sample = NULL;
        }
    }
}

/* ============================================================================
 * DIGI SAMPLE METHOD IMPLEMENTATIONS (standalone functions)
 * ============================================================================ */

void digi_sample_t__destroy(struct digi_sample *this) {
    if (!this) return;
    free(this->data);
    free(this);
}

void digi_sample_t__silence(struct digi_sample *this, digi_format_t format) {
    if (!this || !this->data || this->length <= 0) return;

    unsigned char silence_value = (format == DIGI_FORMAT_UNSIGNED_8BIT) ? 0x80 : 0x00;
    memset(this->data, silence_value, this->length);
}

void digi_sample_t__apply_volume(struct digi_sample *this, int volume, digi_format_t format) {
    if (!this || !this->data || this->length <= 0) return;
    if (volume < 0) volume = 0;
    if (volume > 255) volume = 255;

    if (format == DIGI_FORMAT_UNSIGNED_8BIT) {
        for (int i = 0; i < this->length; i++) {
            int centered = (int)this->data[i] - 128;
            int scaled = (centered * volume) / 255;
            this->data[i] = (unsigned char)((scaled + 128) & 0xFF);
        }
    } else {
        for (int i = 0; i < this->length; i++) {
            int val = (int)(signed char)this->data[i];
            int scaled = (val * volume) / 255;
            this->data[i] = (unsigned char)(scaled & 0xFF);
        }
    }
}

void digi_sample_t__reverse(struct digi_sample *this) {
    if (!this || !this->data || this->length <= 0) return;

    int left = 0;
    int right = this->length - 1;

    while (left < right) {
        unsigned char tmp = this->data[left];
        this->data[left] = this->data[right];
        this->data[right] = tmp;
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
