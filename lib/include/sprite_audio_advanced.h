/* sprite_audio_advanced.h — Advanced Audio System (Phases 46-50)
 *
 * Phases 46-50: 3D spatial audio, procedural synthesis, mixing, sequencing, processing
 */

#ifndef SPRITE_AUDIO_ADVANCED_H
#define SPRITE_AUDIO_ADVANCED_H

typedef void *sprite_audio_listener_t;
typedef void *sprite_audio_source_t;
typedef void *sprite_audio_synth_t;
typedef void *sprite_audio_mixer_t;
typedef void *sprite_audio_processor_t;

#define INVALID_LISTENER NULL
#define INVALID_SOURCE NULL
#define INVALID_SYNTH NULL
#define INVALID_MIXER NULL
#define INVALID_PROCESSOR NULL

typedef enum {
    SYNTH_SINE = 0,
    SYNTH_SQUARE = 1,
    SYNTH_TRIANGLE = 2,
    SYNTH_SAWTOOTH = 3,
    SYNTH_NOISE = 4,
} sprite_synth_wave_t;

typedef enum {
    EFFECT_REVERB = 0,
    EFFECT_DELAY = 1,
    EFFECT_CHORUS = 2,
    EFFECT_DISTORTION = 3,
    EFFECT_EQ = 4,
} sprite_audio_effect_t;

/* Phase 46: 3D Spatial Audio */
sprite_audio_listener_t sprite_audio_listener_create(void);
void sprite_audio_listener_destroy(sprite_audio_listener_t listener);
void sprite_audio_listener_set_position(sprite_audio_listener_t listener, float x, float y, float z);
void sprite_audio_listener_set_orientation(sprite_audio_listener_t listener, float forward_x, float forward_y, float up_x, float up_y);

sprite_audio_source_t sprite_audio_source_create(sprite_audio_listener_t listener);
void sprite_audio_source_destroy(sprite_audio_source_t source);
void sprite_audio_source_set_position(sprite_audio_source_t source, float x, float y, float z);
void sprite_audio_source_set_velocity(sprite_audio_source_t source, float vx, float vy, float vz);
void sprite_audio_source_set_attenuation(sprite_audio_source_t source, float min_dist, float max_dist);
int sprite_audio_source_update_spatial(sprite_audio_source_t source);

/* Phase 47: Procedural Audio Generation */
sprite_audio_synth_t sprite_audio_synth_create(int sample_rate);
void sprite_audio_synth_destroy(sprite_audio_synth_t synth);
void sprite_audio_synth_set_wave(sprite_audio_synth_t synth, sprite_synth_wave_t wave);
void sprite_audio_synth_set_frequency(sprite_audio_synth_t synth, float frequency);
void sprite_audio_synth_set_amplitude(sprite_audio_synth_t synth, float amplitude);
int sprite_audio_synth_generate(sprite_audio_synth_t synth, void *buffer, int samples);
void sprite_audio_synth_set_envelope(sprite_audio_synth_t synth, float attack, float decay, float sustain, float release);

/* Phase 48: Mixing & Effects */
sprite_audio_mixer_t sprite_audio_mixer_create(int channels, int sample_rate);
void sprite_audio_mixer_destroy(sprite_audio_mixer_t mixer);
void sprite_audio_mixer_set_master_volume(sprite_audio_mixer_t mixer, float volume);
void sprite_audio_mixer_add_channel(sprite_audio_mixer_t mixer, int channel_id);
void sprite_audio_mixer_remove_channel(sprite_audio_mixer_t mixer, int channel_id);
void sprite_audio_mixer_set_channel_volume(sprite_audio_mixer_t mixer, int channel_id, float volume);
int sprite_audio_mixer_process(sprite_audio_mixer_t mixer, void *buffer, int samples);

sprite_audio_processor_t sprite_audio_processor_create(sprite_audio_effect_t effect);
void sprite_audio_processor_destroy(sprite_audio_processor_t processor);
void sprite_audio_processor_set_param(sprite_audio_processor_t processor, int param_id, float value);
int sprite_audio_processor_process(sprite_audio_processor_t processor, void *buffer, int samples);

/* Phase 49: MIDI Sequencing */
typedef struct {
    int note;
    int velocity;
    int duration_ms;
    int start_time_ms;
} sprite_midi_note_t;

typedef void *sprite_midi_sequence_t;

sprite_midi_sequence_t sprite_midi_sequence_create(int bpm, int time_signature);
void sprite_midi_sequence_destroy(sprite_midi_sequence_t sequence);
void sprite_midi_sequence_add_note(sprite_midi_sequence_t sequence, int track, sprite_midi_note_t note);
int sprite_midi_sequence_get_note_count(sprite_midi_sequence_t sequence, int track);
int sprite_midi_sequence_update(sprite_midi_sequence_t sequence, int delta_ms);
void sprite_midi_sequence_play(sprite_midi_sequence_t sequence);
void sprite_midi_sequence_stop(sprite_midi_sequence_t sequence);

/* Phase 50: Real-time Audio Processing */
typedef int (*sprite_audio_callback_t)(void *user_data, void *buffer, int samples);

typedef void *sprite_audio_stream_t;

sprite_audio_stream_t sprite_audio_stream_create(int sample_rate, sprite_audio_callback_t callback, void *user_data);
void sprite_audio_stream_destroy(sprite_audio_stream_t stream);
void sprite_audio_stream_start(sprite_audio_stream_t stream);
void sprite_audio_stream_stop(sprite_audio_stream_t stream);
int sprite_audio_stream_is_running(sprite_audio_stream_t stream);

#endif
