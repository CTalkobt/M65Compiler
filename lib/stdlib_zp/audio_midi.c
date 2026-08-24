/* audio_midi_zp.c — MEGA65 MIDI Input Handler & SID Router (Phase 11, ZP Convention)
 *
 * Zero-page calling convention version of audio_midi.c
 * Parameters passed in ZP ($20-$2A) instead of stack for performance.
 *
 * ZP Parameter Map:
 *   $20-$21: this (pointer)
 *   $22-$23: synth (pointer, if needed)
 *   $24: channel
 *   $25: note
 *   $26: velocity/value
 *   $27-$28: bend (2 bytes)
 */

#include <audio_midi.h>
#include <stdlib.h>

/* ============================================================================
 * MIDI INPUT HANDLER - Parse incoming MIDI bytes
 * ============================================================================ */

void midi_input_t__init(struct midi_input *this) {
    this->running_status = 0;
    this->buffer_index = 0;
    this->message_ready = 0;
    this->expected_length = 0;
    this->last_channel = 0;
    for (int i = 0; i < 3; i++) {
        this->message_buffer[i] = 0;
    }
}

void midi_input_t__done(struct midi_input *this) {
    this->running_status = 0;
    this->message_ready = 0;
}

int midi_input_t__read_byte(struct midi_input *this) {
    /* Placeholder: would read from UART/MIDI device */
    return -1;
}

int midi_input_t__has_message(struct midi_input *this) {
    return this->message_ready;
}

void midi_input_t__clear_message(struct midi_input *this) {
    this->message_ready = 0;
    this->buffer_index = 0;
}

/* ============================================================================
 * MIDI-TO-SID VOICE ROUTER - Convert MIDI to SID synthesis (ZP optimized)
 * ============================================================================ */

void midi_to_sid_t__init(struct midi_to_sid *this, sid_synth_t *synth) {
    this->synth = synth;
    this->omni_mode = 1;
    this->mono_mode = 0;
    this->sustain_pedal = 0;
    this->reverb_send = 50;
    this->chorus_send = 0;
    this->delay_time = 0;
    this->num_voices = (synth && synth->num_sids) ? (synth->num_sids * 3) : 3;

    for (int i = 0; i < 12; i++) {
        this->voice_alloc[i].sid_voice = i;
        this->voice_alloc[i].midi_channel = i % 16;
        this->voice_alloc[i].active = 0;
        this->voice_alloc[i].current_note = -1;
        this->filter_cutoff[i] = 10000;
        this->filter_resonance[i] = 0;
    }

    for (int i = 0; i < 16; i++) {
        this->channel_volume[i] = MIDI_DEFAULT_VOLUME;
        this->channel_pan[i] = MIDI_DEFAULT_PAN;
        this->channel_portamento[i] = MIDI_DEFAULT_PORTAMENTO;
        this->channel_vibrato_depth[i] = 0;
        this->channel_vibrato_rate[i] = 0;
    }
}

void midi_to_sid_t__done(struct midi_to_sid *this) {
    if (this->synth) {
        this->synth->all_notes_off(this->synth);
    }
}

void midi_to_sid_t__note_on(struct midi_to_sid *this, int channel, int note, int velocity) {
    if (channel < 0 || channel >= MIDI_CHANNELS) return;
    if (note < MIDI_NOTE_MIN || note > MIDI_NOTE_MAX) return;
    if (velocity == 0) {
        this->note_off(this, channel, note);
        return;
    }

    int voice = (channel * 3 / 16) % this->num_voices;
    if (this->omni_mode) {
        for (int i = 0; i < this->num_voices; i++) {
            if (!this->voice_alloc[i].active) {
                voice = i;
                break;
            }
        }
    }

    this->synth->note_on(this->synth, voice, note, velocity);
    this->voice_alloc[voice].active = 1;
    this->voice_alloc[voice].midi_channel = channel;
    this->voice_alloc[voice].current_note = note;
    this->synth->set_portamento(this->synth, this->channel_portamento[channel]);
    this->synth->set_vibrato(this->synth, voice,
                            this->channel_vibrato_depth[channel],
                            this->channel_vibrato_rate[channel]);
}

void midi_to_sid_t__note_off(struct midi_to_sid *this, int channel, int note) {
    if (channel < 0 || channel >= MIDI_CHANNELS) return;

    for (int i = 0; i < this->num_voices; i++) {
        if (this->voice_alloc[i].active &&
            this->voice_alloc[i].midi_channel == channel &&
            this->voice_alloc[i].current_note == note) {
            this->synth->note_off(this->synth, i);
            this->voice_alloc[i].active = 0;
            break;
        }
    }
}

void midi_to_sid_t__control_change(struct midi_to_sid *this, int channel, int cc, int value) {
    if (channel < 0 || channel >= MIDI_CHANNELS) return;
    if (value < MIDI_CC_MIN || value > MIDI_CC_MAX) return;

    switch (cc) {
        case MIDI_CC_MODULATION:
            this->channel_vibrato_depth[channel] = (value * 100) / 127;
            for (int i = 0; i < this->num_voices; i++) {
                if (this->voice_alloc[i].midi_channel == channel && this->voice_alloc[i].active) {
                    this->synth->set_vibrato(this->synth, i,
                                           this->channel_vibrato_depth[channel],
                                           this->channel_vibrato_rate[channel]);
                }
            }
            break;

        case MIDI_CC_PORTAMENTO_TIME:
            this->channel_portamento[channel] = (value * 5000) / 127;
            this->synth->set_portamento(this->synth, this->channel_portamento[channel]);
            break;

        case MIDI_CC_CHANNEL_VOLUME:
            this->channel_volume[channel] = value;
            this->synth->set_volume(this->synth, (value * 255) / 127);
            break;

        case MIDI_CC_FILTER_CUTOFF:
            for (int i = 0; i < this->num_voices; i++) {
                if (this->voice_alloc[i].midi_channel == channel) {
                    this->filter_cutoff[i] = (value * 20000) / 127;
                    this->synth->set_cutoff(this->synth, i, this->filter_cutoff[i]);
                }
            }
            break;

        case MIDI_CC_FILTER_RESONANCE:
            for (int i = 0; i < this->num_voices; i++) {
                if (this->voice_alloc[i].midi_channel == channel) {
                    this->filter_resonance[i] = (value * 255) / 127;
                    this->synth->set_resonance(this->synth, i, this->filter_resonance[i]);
                }
            }
            break;

        case MIDI_CC_SUSTAIN:
            this->sustain_pedal = (value >= 64) ? 1 : 0;
            break;

        case MIDI_CC_REVERB_SEND:
            this->reverb_send = value;
            this->synth->set_reverb(this->synth, (value * 255) / 127);
            break;

        case MIDI_CC_CHORUS_SEND:
            this->chorus_send = value;
            this->synth->set_chorus(this->synth, (value * 255) / 127);
            break;

        case MIDI_CC_ALL_NOTES_OFF:
            this->synth->all_notes_off(this->synth);
            break;

        case MIDI_CC_OMNI_OFF:
            this->omni_mode = 0;
            break;

        case MIDI_CC_OMNI_ON:
            this->omni_mode = 1;
            break;
    }
}

void midi_to_sid_t__pitch_bend(struct midi_to_sid *this, int channel, int bend) {
    for (int i = 0; i < this->num_voices; i++) {
        if (this->voice_alloc[i].midi_channel == channel && this->voice_alloc[i].active) {
            int bend_cents = (bend * 200) / 8192;
            int freq = this->synth->voice[i].osc.frequency;
            int bent_freq = freq + (freq * bend_cents) / 1200;
            this->synth->voice[i].osc.set_frequency(&this->synth->voice[i].osc, bent_freq);
        }
    }
}

void midi_to_sid_t__program_change(struct midi_to_sid *this, int channel, int program) {
    (void)this;
    (void)channel;
    (void)program;
}

void midi_to_sid_t__channel_pressure(struct midi_to_sid *this, int channel, int pressure) {
    if (pressure > 0) {
        this->channel_vibrato_depth[channel] = (pressure * 100) / 127;
    }
}

void midi_to_sid_t__set_omni_mode(struct midi_to_sid *this, int enabled) {
    this->omni_mode = enabled ? 1 : 0;
}

void midi_to_sid_t__set_mono_mode(struct midi_to_sid *this, int enabled) {
    this->mono_mode = enabled ? 1 : 0;
}

void midi_to_sid_t__reset_all_controllers(struct midi_to_sid *this) {
    for (int i = 0; i < 16; i++) {
        this->channel_portamento[i] = MIDI_DEFAULT_PORTAMENTO;
        this->channel_vibrato_depth[i] = 0;
        this->sustain_pedal = 0;
    }
}

int midi_to_sid_t__get_active_voices(struct midi_to_sid *this) {
    int count = 0;
    for (int i = 0; i < this->num_voices; i++) {
        if (this->voice_alloc[i].active) count++;
    }
    return count;
}

/* ============================================================================
 * MIDI RECEIVER - High-level interface
 * ============================================================================ */

void midi_receiver_t__init(struct midi_receiver *this, sid_synth_t *synth) {
    this->synth = synth;
    this->running = 1;
    this->messages_received = 0;
    this->input.init(&this->input);
    this->router.init(&this->router, synth);
}

void midi_receiver_t__done(struct midi_receiver *this) {
    this->running = 0;
    this->router.done(&this->router);
    this->input.done(&this->input);
}

void midi_receiver_t__process(struct midi_receiver *this) {
    if (!this->input.has_message(&this->input)) return;

    unsigned char status = this->input.message_buffer[0];
    unsigned char byte1 = (this->input.buffer_index > 1) ? this->input.message_buffer[1] : 0;
    unsigned char byte2 = (this->input.buffer_index > 2) ? this->input.message_buffer[2] : 0;

    this->handle_message(this, status, byte1, byte2);
    this->input.clear_message(&this->input);
    this->messages_received++;
}

void midi_receiver_t__handle_message(struct midi_receiver *this,
                                     unsigned char status,
                                     unsigned char byte1,
                                     unsigned char byte2) {
    unsigned char channel = status & 0x0F;
    unsigned char command = status & 0xF0;

    switch (command) {
        case MIDI_NOTE_ON:
            this->router.note_on(&this->router, channel, byte1, byte2);
            break;
        case MIDI_NOTE_OFF:
            this->router.note_off(&this->router, channel, byte1);
            break;
        case MIDI_CONTROL_CHANGE:
            this->router.control_change(&this->router, channel, byte1, byte2);
            break;
        case MIDI_PITCH_BEND:
            {
                int bend = ((byte2 & 0x7F) << 7) | (byte1 & 0x7F);
                bend -= 8192;
                this->router.pitch_bend(&this->router, channel, bend);
            }
            break;
        case MIDI_PROGRAM_CHANGE:
            this->router.program_change(&this->router, channel, byte1);
            break;
        case MIDI_CHANNEL_PRESSURE:
            this->router.channel_pressure(&this->router, channel, byte1);
            break;
    }
}

/* ============================================================================
 * MIDI UTILITY FUNCTIONS
 * ============================================================================ */

int midi_note_to_frequency(int note) {
    if (note < MIDI_NOTE_MIN || note > MIDI_NOTE_MAX) return 440;

    int semitones_from_a4 = note - 69;
    int freq = 440;

    for (int i = 0; i < semitones_from_a4; i++) {
        freq = (freq * 1059) / 1000;
    }
    for (int i = semitones_from_a4; i < 0; i++) {
        freq = (freq * 1000) / 1059;
    }

    return freq;
}

int frequency_to_midi_note(int freq) {
    if (freq < 20 || freq > 20000) return 69;

    int closest_note = 69;
    int closest_diff = (freq > 440) ? (freq - 440) : (440 - freq);

    for (int n = MIDI_NOTE_MIN; n <= MIDI_NOTE_MAX; n++) {
        int f = midi_note_to_frequency(n);
        int diff = (f > freq) ? (f - freq) : (freq - f);
        if (diff < closest_diff) {
            closest_diff = diff;
            closest_note = n;
        }
    }

    return closest_note;
}

/* ============================================================================
 * MIDI RECEIVER CREATION
 * ============================================================================ */

midi_receiver_t midi_receiver_create(void) {
    midi_receiver_t receiver;

    receiver.running = 0;
    receiver.messages_received = 0;
    receiver.synth = NULL;

    receiver.input.init = midi_input_t__init;
    receiver.input.done = midi_input_t__done;
    receiver.input.read_byte = midi_input_t__read_byte;
    receiver.input.has_message = midi_input_t__has_message;
    receiver.input.clear_message = midi_input_t__clear_message;

    receiver.router.init = midi_to_sid_t__init;
    receiver.router.done = midi_to_sid_t__done;
    receiver.router.note_on = midi_to_sid_t__note_on;
    receiver.router.note_off = midi_to_sid_t__note_off;
    receiver.router.control_change = midi_to_sid_t__control_change;
    receiver.router.pitch_bend = midi_to_sid_t__pitch_bend;
    receiver.router.program_change = midi_to_sid_t__program_change;
    receiver.router.channel_pressure = midi_to_sid_t__channel_pressure;
    receiver.router.set_omni_mode = midi_to_sid_t__set_omni_mode;
    receiver.router.set_mono_mode = midi_to_sid_t__set_mono_mode;
    receiver.router.reset_all_controllers = midi_to_sid_t__reset_all_controllers;
    receiver.router.get_active_voices = midi_to_sid_t__get_active_voices;

    receiver.init = midi_receiver_t__init;
    receiver.done = midi_receiver_t__done;
    receiver.process = midi_receiver_t__process;
    receiver.handle_message = midi_receiver_t__handle_message;

    return receiver;
}
