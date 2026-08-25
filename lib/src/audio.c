/* sprite_audio_advanced.c — Advanced Audio Implementation (Phases 46-50) */

#include "audio.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>

#define MAX_LISTENERS 4
#define MAX_SOURCES 32
#define MAX_SYNTHS 16
#define MAX_MIXERS 8
#define MAX_PROCESSORS 32
#define MAX_SEQUENCES 8
#define MAX_STREAMS 4

/* Phase 46: 3D Spatial Audio */
typedef struct {
    float x, y, z;
    float forward_x, forward_y;
    float up_x, up_y;
} audio_listener_impl;

typedef struct {
    audio_listener_impl *listener;
    float x, y, z;
    float vx, vy, vz;
    float min_distance, max_distance;
    float pan, volume;
} audio_source_impl;

/* Phase 47: Procedural Audio */
typedef struct {
    sprite_synth_wave_t wave;
    float frequency;
    float amplitude;
    float phase;
    float attack, decay, sustain, release;
    float envelope_time;
    int sample_rate;
} audio_synth_impl;

/* Phase 48: Mixing */
typedef struct {
    int channel_id;
    float volume;
} mixer_channel_t;

typedef struct {
    mixer_channel_t channels[32];
    int channel_count;
    float master_volume;
    int sample_rate;
} audio_mixer_impl;

typedef struct {
    sprite_audio_effect_t effect;
    float params[8];
    float state[16];
} audio_processor_impl;

/* Phase 49: MIDI Sequencing */
typedef struct {
    sprite_midi_note_t notes[256];
    int note_count;
    int bpm;
    int time_signature;
    int current_time_ms;
    int is_playing;
} audio_sequence_impl;

/* Phase 50: Real-time Streaming */
typedef struct {
    int sample_rate;
    sprite_audio_callback_t callback;
    void *user_data;
    int is_running;
} audio_stream_impl;

static audio_listener_impl listeners[MAX_LISTENERS];
static int listener_count = 0;

static audio_source_impl sources[MAX_SOURCES];
static int source_count = 0;

static audio_synth_impl synths[MAX_SYNTHS];
static int synth_count = 0;

static audio_mixer_impl mixers[MAX_MIXERS];
static int mixer_count = 0;

static audio_processor_impl processors[MAX_PROCESSORS];
static int processor_count = 0;

static audio_sequence_impl sequences[MAX_SEQUENCES];
static int sequence_count = 0;

static audio_stream_impl streams[MAX_STREAMS];
static int stream_count = 0;

/* Phase 46 Implementation */
sprite_audio_listener_t sprite_audio_listener_create(void) {
    if (listener_count >= MAX_LISTENERS) return INVALID_LISTENER;
    audio_listener_impl *listener = &listeners[listener_count];
    listener->x = listener->y = listener->z = 0.0f;
    listener->forward_x = 0.0f; listener->forward_y = 1.0f;
    listener->up_x = 0.0f; listener->up_y = 1.0f;
    return (sprite_audio_listener_t)(intptr_t)listener_count++;
}

void sprite_audio_listener_destroy(sprite_audio_listener_t listener) {}

void sprite_audio_listener_set_position(sprite_audio_listener_t listener, float x, float y, float z) {
    intptr_t idx = (intptr_t)listener;
    if (idx < 0 || idx >= listener_count) return;
    listeners[idx].x = x;
    listeners[idx].y = y;
    listeners[idx].z = z;
}

void sprite_audio_listener_set_orientation(sprite_audio_listener_t listener, float forward_x, float forward_y, float up_x, float up_y) {
    intptr_t idx = (intptr_t)listener;
    if (idx < 0 || idx >= listener_count) return;
    listeners[idx].forward_x = forward_x;
    listeners[idx].forward_y = forward_y;
    listeners[idx].up_x = up_x;
    listeners[idx].up_y = up_y;
}

sprite_audio_source_t sprite_audio_source_create(sprite_audio_listener_t listener) {
    if (source_count >= MAX_SOURCES) return INVALID_SOURCE;
    audio_source_impl *source = &sources[source_count];
    intptr_t idx = (intptr_t)listener;
    source->listener = (idx >= 0 && idx < listener_count) ? &listeners[idx] : NULL;
    source->x = source->y = source->z = 0.0f;
    source->vx = source->vy = source->vz = 0.0f;
    source->min_distance = 1.0f;
    source->max_distance = 100.0f;
    source->pan = 0.5f;
    source->volume = 1.0f;
    return (sprite_audio_source_t)(intptr_t)source_count++;
}

void sprite_audio_source_destroy(sprite_audio_source_t source) {}

void sprite_audio_source_set_position(sprite_audio_source_t source, float x, float y, float z) {
    intptr_t idx = (intptr_t)source;
    if (idx < 0 || idx >= source_count) return;
    sources[idx].x = x;
    sources[idx].y = y;
    sources[idx].z = z;
}

void sprite_audio_source_set_velocity(sprite_audio_source_t source, float vx, float vy, float vz) {
    intptr_t idx = (intptr_t)source;
    if (idx < 0 || idx >= source_count) return;
    sources[idx].vx = vx;
    sources[idx].vy = vy;
    sources[idx].vz = vz;
}

void sprite_audio_source_set_attenuation(sprite_audio_source_t source, float min_dist, float max_dist) {
    intptr_t idx = (intptr_t)source;
    if (idx < 0 || idx >= source_count) return;
    sources[idx].min_distance = min_dist;
    sources[idx].max_distance = max_dist;
}

int sprite_audio_source_update_spatial(sprite_audio_source_t source) {
    intptr_t idx = (intptr_t)source;
    if (idx < 0 || idx >= source_count) return 0;

    audio_source_impl *s = &sources[idx];
    if (!s->listener) return 0;

    float dx = s->x - s->listener->x;
    float dy = s->y - s->listener->y;
    float distance = sqrtf(dx*dx + dy*dy);

    if (distance < s->min_distance) distance = s->min_distance;
    if (distance > s->max_distance) distance = s->max_distance;

    float attenuation = 1.0f - (distance - s->min_distance) / (s->max_distance - s->min_distance);
    s->volume = attenuation;

    float angle = atan2f(dy, dx);
    s->pan = 0.5f + 0.5f * sinf(angle);

    return 1;
}

/* Phase 47: Procedural Audio */
sprite_audio_synth_t sprite_audio_synth_create(int sample_rate) {
    if (synth_count >= MAX_SYNTHS) return INVALID_SYNTH;
    audio_synth_impl *synth = &synths[synth_count];
    synth->wave = SYNTH_SINE;
    synth->frequency = 440.0f;
    synth->amplitude = 0.5f;
    synth->phase = 0.0f;
    synth->attack = 0.01f;
    synth->decay = 0.05f;
    synth->sustain = 0.7f;
    synth->release = 0.1f;
    synth->sample_rate = sample_rate;
    synth->envelope_time = 0.0f;
    return (sprite_audio_synth_t)(intptr_t)synth_count++;
}

void sprite_audio_synth_destroy(sprite_audio_synth_t synth) {}

void sprite_audio_synth_set_wave(sprite_audio_synth_t synth, sprite_synth_wave_t wave) {
    intptr_t idx = (intptr_t)synth;
    if (idx < 0 || idx >= synth_count) return;
    synths[idx].wave = wave;
}

void sprite_audio_synth_set_frequency(sprite_audio_synth_t synth, float frequency) {
    intptr_t idx = (intptr_t)synth;
    if (idx < 0 || idx >= synth_count) return;
    synths[idx].frequency = frequency;
}

void sprite_audio_synth_set_amplitude(sprite_audio_synth_t synth, float amplitude) {
    intptr_t idx = (intptr_t)synth;
    if (idx < 0 || idx >= synth_count) return;
    synths[idx].amplitude = amplitude;
}

void sprite_audio_synth_set_envelope(sprite_audio_synth_t synth, float attack, float decay, float sustain, float release) {
    intptr_t idx = (intptr_t)synth;
    if (idx < 0 || idx >= synth_count) return;
    synths[idx].attack = attack;
    synths[idx].decay = decay;
    synths[idx].sustain = sustain;
    synths[idx].release = release;
}

int sprite_audio_synth_generate(sprite_audio_synth_t synth, void *buffer, int samples) {
    intptr_t idx = (intptr_t)synth;
    if (idx < 0 || idx >= synth_count) return 0;

    audio_synth_impl *s = &synths[idx];
    float dt = 1.0f / s->sample_rate;
    int16_t *buf = (int16_t *)buffer;

    for (int i = 0; i < samples; i++) {
        float value = 0.0f;
        switch (s->wave) {
            case SYNTH_SINE:
                value = sinf(2.0f * 3.14159f * s->phase);
                break;
            case SYNTH_SQUARE:
                value = (s->phase < 0.5f) ? 1.0f : -1.0f;
                break;
            case SYNTH_TRIANGLE:
                value = (s->phase < 0.5f) ? 4.0f * s->phase - 1.0f : 3.0f - 4.0f * s->phase;
                break;
            case SYNTH_SAWTOOTH:
                value = 2.0f * s->phase - 1.0f;
                break;
            case SYNTH_NOISE:
                value = (float)(rand() % 2) * 2.0f - 1.0f;
                break;
        }

        buf[i] = (int16_t)(value * s->amplitude * 32767.0f);
        s->phase += s->frequency * dt;
        if (s->phase > 1.0f) s->phase -= 1.0f;
    }

    return samples;
}

/* Phase 48: Mixing */
sprite_audio_mixer_t sprite_audio_mixer_create(int channels, int sample_rate) {
    if (mixer_count >= MAX_MIXERS) return INVALID_MIXER;
    audio_mixer_impl *mixer = &mixers[mixer_count];
    mixer->channel_count = 0;
    mixer->master_volume = 1.0f;
    mixer->sample_rate = sample_rate;
    return (sprite_audio_mixer_t)(intptr_t)mixer_count++;
}

void sprite_audio_mixer_destroy(sprite_audio_mixer_t mixer) {}

void sprite_audio_mixer_set_master_volume(sprite_audio_mixer_t mixer, float volume) {
    intptr_t idx = (intptr_t)mixer;
    if (idx < 0 || idx >= mixer_count) return;
    mixers[idx].master_volume = volume;
}

void sprite_audio_mixer_add_channel(sprite_audio_mixer_t mixer, int channel_id) {
    intptr_t idx = (intptr_t)mixer;
    if (idx < 0 || idx >= mixer_count) return;
    audio_mixer_impl *m = &mixers[idx];
    if (m->channel_count < 32) {
        m->channels[m->channel_count].channel_id = channel_id;
        m->channels[m->channel_count].volume = 1.0f;
        m->channel_count++;
    }
}

void sprite_audio_mixer_remove_channel(sprite_audio_mixer_t mixer, int channel_id) {
    intptr_t idx = (intptr_t)mixer;
    if (idx < 0 || idx >= mixer_count) return;
}

void sprite_audio_mixer_set_channel_volume(sprite_audio_mixer_t mixer, int channel_id, float volume) {
    intptr_t idx = (intptr_t)mixer;
    if (idx < 0 || idx >= mixer_count) return;
    audio_mixer_impl *m = &mixers[idx];
    for (int i = 0; i < m->channel_count; i++) {
        if (m->channels[i].channel_id == channel_id) {
            m->channels[i].volume = volume;
            return;
        }
    }
}

int sprite_audio_mixer_process(sprite_audio_mixer_t mixer, void *buffer, int samples) {
    intptr_t idx = (intptr_t)mixer;
    if (idx < 0 || idx >= mixer_count) return 0;
    return samples;
}

sprite_audio_processor_t sprite_audio_processor_create(sprite_audio_effect_t effect) {
    if (processor_count >= MAX_PROCESSORS) return INVALID_PROCESSOR;
    audio_processor_impl *proc = &processors[processor_count];
    proc->effect = effect;
    memset(proc->params, 0, sizeof(proc->params));
    memset(proc->state, 0, sizeof(proc->state));
    return (sprite_audio_processor_t)(intptr_t)processor_count++;
}

void sprite_audio_processor_destroy(sprite_audio_processor_t processor) {}

void sprite_audio_processor_set_param(sprite_audio_processor_t processor, int param_id, float value) {
    intptr_t idx = (intptr_t)processor;
    if (idx < 0 || idx >= processor_count || param_id < 0 || param_id >= 8) return;
    processors[idx].params[param_id] = value;
}

int sprite_audio_processor_process(sprite_audio_processor_t processor, void *buffer, int samples) {
    intptr_t idx = (intptr_t)processor;
    if (idx < 0 || idx >= processor_count) return 0;
    return samples;
}

/* Phase 49: MIDI Sequencing */
sprite_midi_sequence_t sprite_midi_sequence_create(int bpm, int time_signature) {
    if (sequence_count >= MAX_SEQUENCES) return NULL;
    audio_sequence_impl *seq = &sequences[sequence_count];
    seq->bpm = bpm;
    seq->time_signature = time_signature;
    seq->note_count = 0;
    seq->current_time_ms = 0;
    seq->is_playing = 0;
    return (sprite_midi_sequence_t)(intptr_t)sequence_count++;
}

void sprite_midi_sequence_destroy(sprite_midi_sequence_t sequence) {}

void sprite_midi_sequence_add_note(sprite_midi_sequence_t sequence, int track, sprite_midi_note_t note) {
    intptr_t idx = (intptr_t)sequence;
    if (idx < 0 || idx >= sequence_count) return;
    audio_sequence_impl *seq = &sequences[idx];
    if (seq->note_count < 256) {
        seq->notes[seq->note_count++] = note;
    }
}

int sprite_midi_sequence_get_note_count(sprite_midi_sequence_t sequence, int track) {
    intptr_t idx = (intptr_t)sequence;
    if (idx < 0 || idx >= sequence_count) return 0;
    return sequences[idx].note_count;
}

int sprite_midi_sequence_update(sprite_midi_sequence_t sequence, int delta_ms) {
    intptr_t idx = (intptr_t)sequence;
    if (idx < 0 || idx >= sequence_count) return 0;
    audio_sequence_impl *seq = &sequences[idx];
    if (!seq->is_playing) return 0;
    seq->current_time_ms += delta_ms;
    return 1;
}

void sprite_midi_sequence_play(sprite_midi_sequence_t sequence) {
    intptr_t idx = (intptr_t)sequence;
    if (idx < 0 || idx >= sequence_count) return;
    sequences[idx].is_playing = 1;
}

void sprite_midi_sequence_stop(sprite_midi_sequence_t sequence) {
    intptr_t idx = (intptr_t)sequence;
    if (idx < 0 || idx >= sequence_count) return;
    sequences[idx].is_playing = 0;
    sequences[idx].current_time_ms = 0;
}

/* Phase 50: Real-time Streaming */
sprite_audio_stream_t sprite_audio_stream_create(int sample_rate, sprite_audio_callback_t callback, void *user_data) {
    if (stream_count >= MAX_STREAMS) return NULL;
    audio_stream_impl *stream = &streams[stream_count];
    stream->sample_rate = sample_rate;
    stream->callback = callback;
    stream->user_data = user_data;
    stream->is_running = 0;
    return (sprite_audio_stream_t)(intptr_t)stream_count++;
}

void sprite_audio_stream_destroy(sprite_audio_stream_t stream) {}

void sprite_audio_stream_start(sprite_audio_stream_t stream) {
    intptr_t idx = (intptr_t)stream;
    if (idx < 0 || idx >= stream_count) return;
    streams[idx].is_running = 1;
}

void sprite_audio_stream_stop(sprite_audio_stream_t stream) {
    intptr_t idx = (intptr_t)stream;
    if (idx < 0 || idx >= stream_count) return;
    streams[idx].is_running = 0;
}

int sprite_audio_stream_is_running(sprite_audio_stream_t stream) {
    intptr_t idx = (intptr_t)stream;
    if (idx < 0 || idx >= stream_count) return 0;
    return streams[idx].is_running;
}
