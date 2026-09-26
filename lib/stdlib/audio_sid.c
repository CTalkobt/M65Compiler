/* audio_sid.c — MEGA65 SID Audio Chip Implementation (Phase 10)
 *
 * Sound synthesis with oscillators, ADSR envelopes, filters, and effects.
 * Follows RRB pattern: standalone functions assigned to method pointers.
 */

#include <audio_sid.h>
#include <stdlib.h>

/* ============================================================================
 * ADSR ENVELOPE METHOD IMPLEMENTATIONS
 * ============================================================================ */

void sid_adsr_t__note_on(struct sid_adsr *this) {
    this->phase = ADSR_ATTACK;
    this->elapsed = 0;
    this->current_level = 0;
    this->active = 1;
}

void sid_adsr_t__note_off(struct sid_adsr *this) {
    this->phase = ADSR_RELEASE;
    this->elapsed = 0;
}

void sid_adsr_t__update(struct sid_adsr *this) {
    if (!this->active) return;

    this->elapsed += 10;  /* Millisecond increment */

    switch (this->phase) {
        case ADSR_ATTACK:
            if (this->attack > 0) {
                this->current_level = (this->elapsed * 255) / this->attack;
            } else {
                this->current_level = 255;
            }
            if (this->current_level >= 255) {
                this->current_level = 255;
                this->phase = ADSR_DECAY;
                this->elapsed = 0;
            }
            break;

        case ADSR_DECAY:
            if (this->decay > 0) {
                int decayed = (this->elapsed * (255 - this->sustain)) / this->decay;
                this->current_level = 255 - decayed;
                if (this->current_level < this->sustain) {
                    this->current_level = this->sustain;
                    this->phase = ADSR_SUSTAIN;
                    this->elapsed = 0;
                }
            } else {
                this->current_level = this->sustain;
                this->phase = ADSR_SUSTAIN;
                this->elapsed = 0;
            }
            break;

        case ADSR_SUSTAIN:
            this->current_level = this->sustain;
            break;

        case ADSR_RELEASE:
            if (this->release > 0) {
                int released = (this->elapsed * this->sustain) / this->release;
                this->current_level = this->sustain - released;
            } else {
                this->current_level = 0;
            }
            if (this->current_level <= 0) {
                this->current_level = 0;
                this->active = 0;
            }
            break;
    }
}

int sid_adsr_t__get_level(struct sid_adsr *this) {
    return this->current_level;
}

/* ============================================================================
 * OSCILLATOR METHOD IMPLEMENTATIONS
 * ============================================================================ */

void sid_oscillator_t__set_frequency(struct sid_oscillator *this, int freq) {
    if (freq < SID_FREQ_MIN) freq = SID_FREQ_MIN;
    if (freq > SID_FREQ_MAX) freq = SID_FREQ_MAX;
    this->frequency = freq;
}

void sid_oscillator_t__set_waveform(struct sid_oscillator *this, sid_waveform_t wave) {
    this->waveform = wave;
    this->phase = 0;  /* Reset phase on waveform change */
}

void sid_oscillator_t__set_pulse_width(struct sid_oscillator *this, int width) {
    if (width < 0) width = 0;
    if (width > 255) width = 255;
    this->pulse_width = width;
}

int sid_oscillator_t__generate_sample(struct sid_oscillator *this) {
    int sample = 0;

    /* Advance phase based on frequency */
    this->phase += (this->frequency * 256) / 44100;  /* 44.1kHz sample rate */
    this->phase &= 0xFFFFFF;  /* Wrap at 24-bit */

    int phase_byte = (this->phase >> 16) & 0xFF;  /* 0-255 */

    switch (this->waveform) {
        case SID_WAVE_TRIANGLE:
            /* Triangle: -32768 to +32767, centered */
            sample = ((phase_byte < 128) ? (phase_byte * 512) : ((255 - phase_byte) * 512)) - 32768;
            break;

        case SID_WAVE_SAWTOOTH:
            /* Sawtooth: -32768 to +32767, centered */
            sample = (phase_byte * 256) - 32768;
            break;

        case SID_WAVE_PULSE:
            /* Pulse: -32768 or +32767 */
            sample = (phase_byte < this->pulse_width) ? 32767 : -32768;
            break;

        case SID_WAVE_NOISE:
            /* Pseudo-random noise: -32768 to +32767 */
            int noise_byte = (this->phase ^ (this->phase >> 8) ^ (this->phase >> 16)) & 0xFF;
            sample = (noise_byte * 256) - 32768;
            break;
    }

    return sample;
}

/* ============================================================================
 * FILTER METHOD IMPLEMENTATIONS
 * ============================================================================ */

void sid_filter_t__set_cutoff(struct sid_filter *this, int freq) {
    if (freq < SID_FREQ_MIN) freq = SID_FREQ_MIN;
    if (freq > SID_FREQ_MAX) freq = SID_FREQ_MAX;
    this->cutoff = freq;
}

void sid_filter_t__set_resonance(struct sid_filter *this, int q) {
    if (q < 0) q = 0;
    if (q > 255) q = 255;
    this->resonance = q;
}

void sid_filter_t__set_type(struct sid_filter *this, sid_filter_type_t type) {
    this->type = type;
}

int sid_filter_t__apply(struct sid_filter *this, int sample) {
    /* Simple one-pole filter (first-order) */
    int cutoff_normalized = (this->cutoff * 255) / SID_FREQ_MAX;
    int alpha = (cutoff_normalized * this->resonance) / 255;

    if (alpha < 0) alpha = 0;
    if (alpha > 255) alpha = 255;

    /* Low-pass: output = alpha * input + (1-alpha) * previous */
    int filtered = (sample * alpha + this->sample_history[0] * (255 - alpha)) / 255;

    /* Update history for next iteration */
    this->sample_history[0] = filtered;

    return filtered;
}

/* ============================================================================
 * VOICE METHOD IMPLEMENTATIONS
 * ============================================================================ */

void sid_voice_t__note_on(struct sid_voice *this, int note, int velocity) {
    if (note < SID_NOTE_C0 || note > SID_NOTE_C8) return;

    this->note = note;
    this->velocity = velocity;
    this->active = 1;

    /* Convert MIDI note to frequency (12-tone equal temperament) */
    int freq = (SID_FREQ_A4 * (1 << (note - 69))) / (1 << 12);
    if (note < 69) {
        freq = SID_FREQ_A4 / (1 << (69 - note) / 12);
    }

    this->osc.set_frequency(&this->osc, freq);
    this->target_frequency = freq;
    this->envelope.note_on(&this->envelope);
}

void sid_voice_t__note_off(struct sid_voice *this) {
    this->active = 0;
    this->envelope.note_off(&this->envelope);
}

void sid_voice_t__update(struct sid_voice *this) {
    if (!this->active && !this->envelope.active) return;

    this->envelope.update(&this->envelope);

    /* Portamento (pitch slide) */
    if (this->portamento > 0 && this->osc.frequency != this->target_frequency) {
        int diff = this->target_frequency - this->osc.frequency;
        int slide_amount = (diff * 10) / this->portamento;
        if (slide_amount == 0) slide_amount = (diff > 0) ? 1 : -1;
        this->osc.frequency += slide_amount;
    }

    /* Vibrato modulation */
    if (this->vibrato_depth > 0) {
        int vibrato_amount = (this->vibrato_depth * this->osc.frequency) / 1200;  /* In cents */
        /* Simplified: add sine modulation at vibrato_rate */
        this->osc.frequency += (vibrato_amount / 4);  /* Reduced for 44.1kHz */
    }
}

int sid_voice_t__get_sample(struct sid_voice *this) {
    if (!this->active && !this->envelope.active) return 0;

    int osc_sample = this->osc.generate_sample(&this->osc);
    int env_level = this->envelope.get_level(&this->envelope);
    int filtered = this->filter.apply(&this->filter, osc_sample);

    /* Apply envelope to filtered sample (velocity already in range 0-127) */
    int output = (filtered * env_level) / 255;

    return output;
}

void sid_voice_t__set_vibrato(struct sid_voice *this, int depth, int rate) {
    this->vibrato_depth = (depth < 0) ? 0 : (depth > 100) ? 100 : depth;
    this->vibrato_rate = (rate < 0) ? 0 : (rate > 20) ? 20 : rate;
}

void sid_voice_t__set_portamento(struct sid_voice *this, int time) {
    this->portamento = (time < 0) ? 0 : (time > 1000) ? 1000 : time;
}

/* ============================================================================
 * SYNTHESIZER METHOD IMPLEMENTATIONS
 * ============================================================================ */

void sid_synth_t__init(struct sid_synth *this) {
    this->volume = SID_VOLUME_DEFAULT;
    this->sample_rate = 44100;
    this->frame_count = 0;
    this->active_voices = 0;
    this->glide_time = 0;
    this->reverb_level = 0;
    this->chorus_level = 0;
    this->delay_time = 0;
    this->delay_feedback = 0;

    /* Initialize all voices */
    for (int i = 0; i < SID_VOICES_MAX; i++) {
        this->voice[i].note = -1;
        this->voice[i].velocity = 0;
        this->voice[i].active = 0;
        this->voice[i].portamento = 0;
        this->voice[i].vibrato_depth = 0;
        this->voice[i].vibrato_rate = 0;

        /* Initialize oscillator */
        this->voice[i].osc.frequency = 440;
        this->voice[i].osc.phase = 0;
        this->voice[i].osc.waveform = SID_WAVE_TRIANGLE;
        this->voice[i].osc.pulse_width = 128;
        this->voice[i].osc.detune = 0;

        /* Initialize envelope */
        this->voice[i].envelope.attack = 100;
        this->voice[i].envelope.decay = 100;
        this->voice[i].envelope.sustain = 200;
        this->voice[i].envelope.release = 100;
        this->voice[i].envelope.phase = ADSR_RELEASE;
        this->voice[i].envelope.elapsed = 0;
        this->voice[i].envelope.current_level = 0;
        this->voice[i].envelope.active = 0;

        /* Initialize filter */
        this->voice[i].filter.type = SID_FILTER_LOWPASS;
        this->voice[i].filter.cutoff = 10000;
        this->voice[i].filter.resonance = 0;
        this->voice[i].filter.output = 0;
        for (int j = 0; j < 4; j++) {
            this->voice[i].filter.sample_history[j] = 0;
        }
    }
}

void sid_synth_t__done(struct sid_synth *this) {
    this->all_notes_off(this);
    this->volume = 0;
}

void sid_synth_t__note_on(struct sid_synth *this, int voice, int note, int velocity) {
    if (voice < 0 || voice >= SID_VOICES_MAX) return;
    if (note < SID_NOTE_C0 || note > SID_NOTE_C8) return;
    if (velocity < 0 || velocity > 127) velocity = 127;

    this->voice[voice].note_on(&this->voice[voice], note, velocity);
    if (this->voice[voice].active) this->active_voices++;
}

void sid_synth_t__note_off(struct sid_synth *this, int voice) {
    if (voice < 0 || voice >= SID_VOICES_MAX) return;

    if (this->voice[voice].active) this->active_voices--;
    this->voice[voice].note_off(&this->voice[voice]);
}

void sid_synth_t__all_notes_off(struct sid_synth *this) {
    for (int i = 0; i < SID_VOICES_MAX; i++) {
        this->note_off(this, i);
    }
}

int sid_synth_t__get_sample(struct sid_synth *this) {
    int mixed = 0;

    /* Mix all voices */
    for (int i = 0; i < SID_VOICES_MAX; i++) {
        mixed += this->voice[i].get_sample(&this->voice[i]);
    }

    /* Apply master volume */
    int output = (mixed * this->volume) / (255 * 3);

    return (output > 32767) ? 32767 : (output < -32768) ? -32768 : output;
}

void sid_synth_t__update(struct sid_synth *this) {
    for (int i = 0; i < SID_VOICES_MAX; i++) {
        this->voice[i].update(&this->voice[i]);
        if (!this->voice[i].active && !this->voice[i].envelope.active) {
            if (this->active_voices > 0) this->active_voices--;
        }
    }
    this->frame_count++;
}

void sid_synth_t__set_volume(struct sid_synth *this, int volume) {
    if (volume < SID_VOLUME_MIN) volume = SID_VOLUME_MIN;
    if (volume > SID_VOLUME_MAX) volume = SID_VOLUME_MAX;
    this->volume = volume;
}

int sid_synth_t__get_volume(struct sid_synth *this) {
    return this->volume;
}

void sid_synth_t__set_vibrato(struct sid_synth *this, int voice, int depth, int rate) {
    if (voice < 0 || voice >= SID_VOICES_MAX) return;
    this->voice[voice].set_vibrato(&this->voice[voice], depth, rate);
}

void sid_synth_t__set_portamento(struct sid_synth *this, int time) {
    this->glide_time = (time < 0) ? 0 : (time > 5000) ? 5000 : time;
    for (int i = 0; i < SID_VOICES_MAX; i++) {
        this->voice[i].set_portamento(&this->voice[i], this->glide_time);
    }
}

void sid_synth_t__set_reverb(struct sid_synth *this, int level) {
    if (level < 0) level = 0;
    if (level > 255) level = 255;
    this->reverb_level = level;
}

void sid_synth_t__set_chorus(struct sid_synth *this, int level) {
    if (level < 0) level = 0;
    if (level > 255) level = 255;
    this->chorus_level = level;
}

void sid_synth_t__set_delay(struct sid_synth *this, int time, int feedback) {
    if (time < 0) time = 0;
    if (time > 1000) time = 1000;
    if (feedback < 0) feedback = 0;
    if (feedback > 255) feedback = 255;
    this->delay_time = time;
    this->delay_feedback = feedback;
}

void sid_synth_t__set_cutoff(struct sid_synth *this, int voice, int freq) {
    if (voice < 0 || voice >= SID_VOICES_MAX) return;
    this->voice[voice].filter.set_cutoff(&this->voice[voice].filter, freq);
}

void sid_synth_t__set_resonance(struct sid_synth *this, int voice, int q) {
    if (voice < 0 || voice >= SID_VOICES_MAX) return;
    this->voice[voice].filter.set_resonance(&this->voice[voice].filter, q);
}

/* ============================================================================
 * SID SYNTHESIZER CREATION
 * ============================================================================ */

sid_synth_t sid_synth_create(void) {
    sid_synth_t synth;

    /* Initialize structure */
    synth.volume = SID_VOLUME_DEFAULT;
    synth.sample_rate = 44100;
    synth.frame_count = 0;
    synth.active_voices = 0;
    synth.master_tuning = 0;
    synth.glide_time = 0;
    synth.reverb_level = 0;
    synth.chorus_level = 0;
    synth.delay_time = 0;
    synth.delay_feedback = 0;

    /* Initialize voices */
    for (int i = 0; i < SID_VOICES_MAX; i++) {
        synth.voice[i].note = -1;
        synth.voice[i].velocity = 0;
        synth.voice[i].active = 0;
        synth.voice[i].portamento = 0;
        synth.voice[i].vibrato_depth = 0;
        synth.voice[i].vibrato_rate = 0;
        synth.voice[i].target_frequency = 440;

        /* Voice oscillators */
        synth.voice[i].osc.frequency = 440;
        synth.voice[i].osc.phase = 0;
        synth.voice[i].osc.waveform = SID_WAVE_TRIANGLE;
        synth.voice[i].osc.pulse_width = 128;
        synth.voice[i].osc.detune = 0;
        synth.voice[i].osc.set_frequency = sid_oscillator_t__set_frequency;
        synth.voice[i].osc.set_waveform = sid_oscillator_t__set_waveform;
        synth.voice[i].osc.set_pulse_width = sid_oscillator_t__set_pulse_width;
        synth.voice[i].osc.generate_sample = sid_oscillator_t__generate_sample;

        /* Voice envelopes */
        synth.voice[i].envelope.attack = 100;
        synth.voice[i].envelope.decay = 100;
        synth.voice[i].envelope.sustain = 200;
        synth.voice[i].envelope.release = 100;
        synth.voice[i].envelope.phase = ADSR_RELEASE;
        synth.voice[i].envelope.elapsed = 0;
        synth.voice[i].envelope.current_level = 0;
        synth.voice[i].envelope.active = 0;
        synth.voice[i].envelope.note_on = sid_adsr_t__note_on;
        synth.voice[i].envelope.note_off = sid_adsr_t__note_off;
        synth.voice[i].envelope.update = sid_adsr_t__update;
        synth.voice[i].envelope.get_level = sid_adsr_t__get_level;

        /* Voice filters */
        synth.voice[i].filter.type = SID_FILTER_LOWPASS;
        synth.voice[i].filter.cutoff = 10000;
        synth.voice[i].filter.resonance = 0;
        synth.voice[i].filter.output = 0;
        for (int j = 0; j < 4; j++) {
            synth.voice[i].filter.sample_history[j] = 0;
        }
        synth.voice[i].filter.set_cutoff = sid_filter_t__set_cutoff;
        synth.voice[i].filter.set_resonance = sid_filter_t__set_resonance;
        synth.voice[i].filter.set_type = sid_filter_t__set_type;
        synth.voice[i].filter.apply = sid_filter_t__apply;

        /* Voice methods */
        synth.voice[i].note_on = sid_voice_t__note_on;
        synth.voice[i].note_off = sid_voice_t__note_off;
        synth.voice[i].update = sid_voice_t__update;
        synth.voice[i].get_sample = sid_voice_t__get_sample;
        synth.voice[i].set_vibrato = sid_voice_t__set_vibrato;
        synth.voice[i].set_portamento = sid_voice_t__set_portamento;
    }

    /* Synth methods */
    synth.init = sid_synth_t__init;
    synth.done = sid_synth_t__done;
    synth.note_on = sid_synth_t__note_on;
    synth.note_off = sid_synth_t__note_off;
    synth.all_notes_off = sid_synth_t__all_notes_off;
    synth.get_sample = sid_synth_t__get_sample;
    synth.update = sid_synth_t__update;
    synth.set_volume = sid_synth_t__set_volume;
    synth.get_volume = sid_synth_t__get_volume;
    synth.set_vibrato = sid_synth_t__set_vibrato;
    synth.set_portamento = sid_synth_t__set_portamento;
    synth.set_reverb = sid_synth_t__set_reverb;
    synth.set_chorus = sid_synth_t__set_chorus;
    synth.set_delay = sid_synth_t__set_delay;
    synth.set_cutoff = sid_synth_t__set_cutoff;
    synth.set_resonance = sid_synth_t__set_resonance;

    return synth;
}
