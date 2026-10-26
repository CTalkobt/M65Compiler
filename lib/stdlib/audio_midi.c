/* audio_midi.c — MIDI messages played on the SIDs (see <audio_midi.h>) */

#include <audio_midi.h>

/* ============================================================================
 * Parser
 * ========================================================================== */

void midi_parser_init(midi_parser_t *p) {
    p->running = 0;
    p->have = 0;
    p->sysex = 0;
}

/* Data bytes of a channel message type */
static unsigned char data_len(unsigned char status) {
    unsigned char t = status & 0xF0;
    return (t == MIDI_PROGRAM_CHANGE || t == MIDI_CHANNEL_PRESSURE) ? 1 : 2;
}

unsigned char midi_parse(midi_parser_t *restrict p, unsigned char b, midi_msg_t *restrict msg) {
    if (b >= 0xF8) return 0;                /* real-time: anywhere, ignored */
    if (b & 0x80) {
        if (b == 0xF0) { p->sysex = 1; p->running = 0; return 0; }
        if (b >= 0xF0) {                    /* system common, end of SysEx */
            p->sysex = 0;
            p->running = 0;
            return 0;
        }
        p->sysex = 0;
        p->running = b;
        p->have = 0;
        return 0;
    }
    if (p->sysex || !p->running) return 0;  /* SysEx data, or no status yet */
    p->data[p->have++] = b;
    if (p->have < data_len(p->running)) return 0;
    msg->status = p->running;
    msg->data1 = p->data[0];
    msg->data2 = p->have > 1 ? p->data[1] : 0;
    p->have = 0;                            /* running status: the next data bytes */
    return 1;
}

/* ============================================================================
 * Synth
 * ========================================================================== */

void midi_synth_init(midi_synth_t *s, unsigned char first_voice, unsigned char num_voices,
                     const sid_instrument_t *inst) {
    unsigned char i;
    if (num_voices > MIDI_SYNTH_VOICES) num_voices = MIDI_SYNTH_VOICES;
    s->first_voice = first_voice;
    s->num_voices = num_voices;
    s->clock = 0;
    s->programs = 0;
    s->num_programs = 0;
    for (i = 0; i < MIDI_SYNTH_VOICES; i++) {
        s->state[i] = 0;
        s->age[i] = 0;
    }
    for (i = 0; i < 16; i++) {
        s->sustain[i] = 0;
        s->bend[i] = 0;
        s->inst[i] = inst;
    }
}

void midi_synth_set_instrument(midi_synth_t *s, unsigned char channel, const sid_instrument_t *inst) {
    s->inst[channel & 15] = inst;
}

void midi_synth_set_programs(midi_synth_t *s, const sid_instrument_t *programs, unsigned char count) {
    s->programs = programs;
    s->num_programs = count;
}

/* The frequency register of a note bent by bend (+-8192 = +-2 semitones) */
static unsigned int bent(unsigned char note, int bend) {
    unsigned int f = sid_note_to_reg(note);
    unsigned int g;
    if (bend > 0) {
        g = sid_note_to_reg(note + 2);
        return f + (unsigned int)((unsigned long)(g - f) * (unsigned int)bend / 8192UL);
    }
    if (bend < 0 && note >= 2) {
        g = sid_note_to_reg(note - 2);
        return f - (unsigned int)((unsigned long)(f - g) * (unsigned int)(-bend) / 8192UL);
    }
    return f;
}

static void release(midi_synth_t *s, unsigned char i) {
    s->state[i] = 0;
    sid_note_off(s->first_voice + i);
}

static void note_on(midi_synth_t *restrict s, unsigned char ch, unsigned char note) {
    unsigned char i, pick = 0xFF, oldest = 0;
    unsigned char best_age = 0;
    /* the same note again on its voice, else a free voice, else the oldest */
    for (i = 0; i < s->num_voices; i++)
        if (s->state[i] && s->channel[i] == ch && s->note[i] == note) pick = i;
    for (i = 0; pick == 0xFF && i < s->num_voices; i++)
        if (!s->state[i]) pick = i;
    if (pick == 0xFF) {
        for (i = 0; i < s->num_voices; i++) {
            unsigned char a = (unsigned char)(s->clock - s->age[i]);
            if (a >= best_age) { best_age = a; oldest = i; }
        }
        pick = oldest;
    }
    s->state[pick] = 1;
    s->note[pick] = note;
    s->channel[pick] = ch;
    s->age[pick] = s->clock++;
    sid_gate(s->first_voice + pick, 0);
    sid_set_instrument(s->first_voice + pick, s->inst[ch]);
    sid_set_freq_reg(s->first_voice + pick, bent(note, s->bend[ch]));
    sid_gate(s->first_voice + pick, 1);
}

static void note_off(midi_synth_t *s, unsigned char ch, unsigned char note) {
    unsigned char i;
    for (i = 0; i < s->num_voices; i++)
        if (s->state[i] == 1 && s->channel[i] == ch && s->note[i] == note) {
            if (s->sustain[ch]) s->state[i] = 2;
            else release(s, i);
        }
}

void midi_synth_all_off(midi_synth_t *s) {
    unsigned char i;
    for (i = 0; i < s->num_voices; i++)
        if (s->state[i]) release(s, i);
}

void midi_synth_message(midi_synth_t *restrict s, const midi_msg_t *restrict m) {
    unsigned char ch = MIDI_CHANNEL(m), i;
    switch (MIDI_TYPE(m)) {
    case MIDI_NOTE_ON:
        if (m->data2) { note_on(s, ch, m->data1); break; }
        /* velocity 0: note off */
    case MIDI_NOTE_OFF:
        note_off(s, ch, m->data1);
        break;
    case MIDI_CONTROL_CHANGE:
        if (m->data1 == MIDI_CC_SUSTAIN) {
            s->sustain[ch] = m->data2 >= 64;
            if (!s->sustain[ch])
                for (i = 0; i < s->num_voices; i++)
                    if (s->state[i] == 2 && s->channel[i] == ch) release(s, i);
        } else if (m->data1 == MIDI_CC_VOLUME) {
            sid_set_volume(m->data2 >> 3);
        } else if (m->data1 == MIDI_CC_ALL_NOTES_OFF || m->data1 == MIDI_CC_ALL_SOUND_OFF) {
            for (i = 0; i < s->num_voices; i++)
                if (s->state[i] && s->channel[i] == ch) release(s, i);
        }
        break;
    case MIDI_PROGRAM_CHANGE:
        if (s->programs && s->num_programs) s->inst[ch] = &s->programs[m->data1 % s->num_programs];
        break;
    case MIDI_PITCH_BEND:
        s->bend[ch] = (int)(((unsigned int)m->data2 << 7) | m->data1) - 8192;
        for (i = 0; i < s->num_voices; i++)
            if (s->state[i] && s->channel[i] == ch)
                sid_set_freq_reg(s->first_voice + i, bent(s->note[i], s->bend[ch]));
        break;
    default:
        break;
    }
}

unsigned char midi_synth_active(const midi_synth_t *restrict s) {
    unsigned char i, n = 0;
    for (i = 0; i < s->num_voices; i++)
        if (s->state[i]) n++;
    return n;
}

/* ============================================================================
 * Frequencies
 * ========================================================================== */

/* C-1..B-1 (MIDI 0-11) in 1/256 Hz: 8.1758 Hz * 2^(n/12) */
static const unsigned int base_hz256[12] = {
    2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951
};

unsigned int midi_note_to_hz(unsigned char note) {
    unsigned char octave = note / 12;
    unsigned long f = (unsigned long)base_hz256[note - octave * 12] << octave;
    return (unsigned int)((f + 128) >> 8);
}
