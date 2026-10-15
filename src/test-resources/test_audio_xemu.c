/* test_audio_xemu.c — <audio_sid.h>, <audio_digi.h>, <audio_midi.h>
 *
 * The SID registers are write-only: the library's shadow copy (sid_get_reg)
 * is checked. The audio DMA registers read back. xemu runs PAL.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] sid_init: registers 0, volume 15
 *   [1] frequencies: sid_hz_to_reg, sid_note_to_reg (A4 = 440 Hz), the
 *       octaves, the top clamped
 *   [2] sid_note_on / sid_note_off / sid_gate / sid_set_waveform (the gate
 *       and the waveform kept apart); voices on SIDs 2-4
 *   [3] sid_set_adsr, sid_set_pulse, sid_set_instrument, sid_play
 *   [4] sid_set_mirror: voices 0-5 also written to 6-11
 *   [5] sid_set_volume (filter mode kept), sid_set_filter
 *   [6] digi_init / digi_play / digi_playing / digi_stop / digi_set_volume /
 *       digi_set_rate / digi_position: the audio DMA registers
 *   [7] midi_parse: running status, real-time bytes, SysEx, 1-byte messages
 *   [8] midi_synth: allocation and stealing, note off, velocity 0, sustain,
 *       pitch bend, program change, all notes off; midi_note_to_hz
 *   [9] $AA sentinel
 */
#include <audio_sid.h>
#include <audio_digi.h>
#include <audio_midi.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define REG(a) (*(volatile unsigned char *)(a))

/* voice v's register r in the shadow */
static unsigned char vr(unsigned char v, unsigned char r) {
    return sid_get_reg(v / 3, (v % 3) * 7 + r);
}

static unsigned int vfreq(unsigned char v) {
    return vr(v, SID_REG_FREQ_LO) | ((unsigned int)vr(v, SID_REG_FREQ_HI) << 8);
}

static int t_init(void) {
    unsigned char c, r, ok = 1;
    sid_write(4, SID_REG_AD, 0x55);
    sid_init();
    for (c = 0; c < 4; c++) {
        for (r = 0; r < 0x18; r++) if (sid_get_reg(c, r)) ok = 0;
        if (sid_get_reg(c, 0x18) != 15) ok = 0;
    }
    return ok;
}

static int t_freq(void) {
    return sid_hz_to_reg(440) == 7492 && sid_note_to_reg(69) == 7492 && sid_note_to_reg(81) == 14985 &&
           sid_note_to_reg(60) == 4455 && sid_note_to_reg(96) == 35641 && sid_note_to_reg(12) == 278 &&
           sid_note_to_reg(107) == 65535 && sid_note_to_reg(120) == 65535 && sid_hz_to_reg(5000) == 65535 &&
           sid_hz_to_reg(0) == 0;
}

static int t_notes(void) {
    int ok;
    sid_init();
    sid_set_waveform(0, SID_SAWTOOTH);
    sid_note_on(0, 69);
    ok = vfreq(0) == 7492 && vr(0, SID_REG_CTRL) == (SID_SAWTOOTH | SID_GATE);
    sid_set_waveform(0, SID_PULSE);
    ok = ok && vr(0, SID_REG_CTRL) == (SID_PULSE | SID_GATE);
    sid_note_off(0);
    ok = ok && vr(0, SID_REG_CTRL) == SID_PULSE;
    sid_gate(0, 1);
    ok = ok && vr(0, SID_REG_CTRL) == (SID_PULSE | SID_GATE);
    /* voice 11: SID 4, its voice 3 at register 14 */
    sid_set_waveform(11, SID_TRIANGLE);
    sid_note_on(11, 60);
    ok = ok && sid_get_reg(3, 14) == (4455 & 0xFF) && sid_get_reg(3, 15) == (4455 >> 8) &&
         sid_get_reg(3, 18) == (SID_TRIANGLE | SID_GATE) && vfreq(4) == 0;
    sid_set_freq(5, 1000);
    ok = ok && vfreq(5) == 17028;
    sid_all_off();
    return ok && vr(0, SID_REG_CTRL) == SID_PULSE && vr(11, SID_REG_CTRL) == SID_TRIANGLE;
}

static const sid_instrument_t organ = { SID_PULSE, 0x09, 0xA4, 0x0800 };

static int t_shape(void) {
    int ok;
    sid_init();
    sid_set_adsr(2, 1, 2, 12, 5);
    sid_set_pulse(2, 0x123);
    ok = vr(2, SID_REG_AD) == 0x12 && vr(2, SID_REG_SR) == 0xC5 && vr(2, SID_REG_PW_LO) == 0x23 &&
         vr(2, SID_REG_PW_HI) == 0x01;
    sid_set_pulse(2, 9999);
    ok = ok && vr(2, SID_REG_PW_LO) == 0xFF && vr(2, SID_REG_PW_HI) == 0x0F;
    sid_set_instrument(3, &organ);
    ok = ok && vr(3, SID_REG_AD) == 0x09 && vr(3, SID_REG_SR) == 0xA4 && vr(3, SID_REG_PW_HI) == 0x08 &&
         vr(3, SID_REG_CTRL) == SID_PULSE;
    sid_play(1, &organ, 57);
    return ok && vfreq(1) == 3746 && vr(1, SID_REG_CTRL) == (SID_PULSE | SID_GATE) && vr(1, SID_REG_SR) == 0xA4;
}

static int t_mirror(void) {
    int ok;
    sid_init();
    sid_set_mirror(1);
    sid_play(1, &organ, 69);
    sid_note_on(4, 60);
    ok = vfreq(7) == 7492 && vr(7, SID_REG_CTRL) == (SID_PULSE | SID_GATE) && vr(7, SID_REG_SR) == 0xA4 &&
         vfreq(10) == 4455;
    /* voices 6-11 written directly are not mirrored back */
    sid_note_on(8, 50);
    ok = ok && vfreq(2) == 0;
    sid_set_mirror(0);
    sid_note_on(1, 72);
    return ok && vfreq(1) == sid_note_to_reg(72) && vfreq(7) == 7492;
}

static int t_volume(void) {
    int ok;
    sid_init();
    sid_set_filter(1, 0x5AB, 9, SID_FILTER_LP | SID_FILTER_BP, 5);
    ok = sid_get_reg(1, 0x15) == 0x03 && sid_get_reg(1, 0x16) == 0xB5 && sid_get_reg(1, 0x17) == 0x95 &&
         sid_get_reg(1, 0x18) == 0x3F;
    sid_set_volume(7);
    ok = ok && sid_get_reg(1, 0x18) == 0x37 && sid_get_reg(0, 0x18) == 0x07 && sid_get_reg(3, 0x18) == 0x07;
    sid_set_filter(2, 5000, 0, SID_FILTER_HP, 0);
    sid_init();
    return ok;
}

static unsigned char samples[256];

static int t_digi(void) {
    int ok;
    unsigned int i;
    for (i = 0; i < 256; i++) samples[i] = (unsigned char)i;
    digi_init();
    ok = REG(0xD711) == 0x80 && digi_rate_to_reg(8000) == 3314 && digi_rate_to_reg(44100) == 18268 &&
         !digi_playing(0);
    digi_play(1, samples, 200, 8000, 200, DIGI_LOOP);
    ok = ok && REG(0xD730) == (0x80 | 0x40 | 0x20 | 2) && digi_playing(1) &&
         REG(0xD731) == (unsigned char)(unsigned int)samples && REG(0xD733) == 0 &&
         REG(0xD734) == (3314 & 0xFF) && REG(0xD735) == (3314 >> 8) && REG(0xD736) == 0 &&
         REG(0xD737) == (unsigned char)((unsigned int)samples + 200) &&
         REG(0xD738) == (unsigned char)(((unsigned int)samples + 200) >> 8) && REG(0xD739) == 200;
    digi_set_volume(1, 50, 30);
    digi_set_rate(1, 16000);
    ok = ok && REG(0xD739) == 50 && REG(0xD71D) == 30 && REG(0xD734) == (6628 & 0xFF) &&
         REG(0xD735) == (6628 >> 8);
    ok = ok && (digi_position(1) & 0xFF0000UL) == 0;
    digi_play_far(2, 0x40000UL, 100, 11025, 255, DIGI_SIGNED | DIGI_16BIT);
    ok = ok && REG(0xD740) == (0x80 | 3) && REG(0xD743) == 4 && REG(0xD747) == 100 && REG(0xD748) == 0;
    digi_stop(1);
    ok = ok && !digi_playing(1) && digi_playing(2);
    digi_done();
    return ok && REG(0xD711) == 0 && !digi_playing(2);
}

static int t_parse(void) {
    static const unsigned char bytes[] = {
        0x90, 60, 100,          /* note on ch 0 */
        62, 101,                /* running status: another note on */
        0xF8,                   /* clock (real-time) */
        0xC3, 5,                /* program change ch 3: 1 data byte */
        7,                      /* running status program change */
        0xF0, 0x41, 0x10, 0xF7, /* SysEx */
        30, 31,                 /* data without status after SysEx: ignored */
        0xE1, 0x00, 0x40,       /* pitch bend ch 1 */
        0x81, 0xFE, 60, 0       /* note off with active sensing inside */
    };
    static const unsigned char want[][3] = {
        { 0x90, 60, 100 }, { 0x90, 62, 101 }, { 0xC3, 5, 0 }, { 0xC3, 7, 0 }, { 0xE1, 0x00, 0x40 }, { 0x81, 60, 0 }
    };
    midi_parser_t p;
    midi_msg_t m;
    unsigned char i, n = 0, ok = 1;
    midi_parser_init(&p);
    for (i = 0; i < sizeof bytes; i++) {
        if (midi_parse(&p, bytes[i], &m)) {
            if (n >= 6 || m.status != want[n][0] || m.data1 != want[n][1] || m.data2 != want[n][2]) ok = 0;
            n++;
        }
    }
    return ok && n == 6;
}

static const sid_instrument_t progs[2] = { { SID_TRIANGLE, 0, 0xF0, 0 }, { SID_SAWTOOTH, 0x11, 0x22, 0 } };

static void msg(midi_synth_t *s, unsigned char st, unsigned char d1, unsigned char d2) {
    midi_msg_t m;
    m.status = st;
    m.data1 = d1;
    m.data2 = d2;
    midi_synth_message(s, &m);
}

static int t_synth(void) {
    midi_synth_t s;
    unsigned char i;
    int ok;
    sid_init();
    midi_synth_init(&s, 3, 3, &organ);          /* voices 3-5 */
    msg(&s, 0x90, 60, 100);
    msg(&s, 0x90, 64, 100);
    msg(&s, 0x90, 67, 100);
    ok = midi_synth_active(&s) == 3 && vfreq(3) == sid_note_to_reg(60) && vfreq(5) == sid_note_to_reg(67) &&
         (vr(4, SID_REG_CTRL) & SID_GATE) && vfreq(0) == 0;
    /* a fourth note takes the oldest voice (3) */
    msg(&s, 0x90, 72, 100);
    ok = ok && vfreq(3) == sid_note_to_reg(72) && midi_synth_active(&s) == 3;
    /* note off and velocity 0 */
    msg(&s, 0x80, 64, 0);
    msg(&s, 0x90, 67, 0);
    ok = ok && midi_synth_active(&s) == 1 && !(vr(4, SID_REG_CTRL) & SID_GATE) && !(vr(5, SID_REG_CTRL) & SID_GATE);
    /* sustain: the note stays until the pedal is up */
    msg(&s, 0xB0, MIDI_CC_SUSTAIN, 127);
    msg(&s, 0x80, 72, 0);
    ok = ok && midi_synth_active(&s) == 1 && (vr(3, SID_REG_CTRL) & SID_GATE);
    msg(&s, 0xB0, MIDI_CC_SUSTAIN, 0);
    ok = ok && midi_synth_active(&s) == 0 && !(vr(3, SID_REG_CTRL) & SID_GATE);
    /* pitch bend up a full 2 semitones, then centre */
    msg(&s, 0x90, 60, 100);
    i = vfreq(3) == sid_note_to_reg(60) ? 3 : vfreq(4) == sid_note_to_reg(60) ? 4 : 5;
    msg(&s, 0xE0, 0x7F, 0x7F);
    ok = ok && vfreq(i) >= sid_note_to_reg(62) - 2 && vfreq(i) <= sid_note_to_reg(62);
    msg(&s, 0xE0, 0x00, 0x40);
    ok = ok && vfreq(i) == sid_note_to_reg(60);
    /* program change: channel 2 plays progs[1] */
    midi_synth_set_programs(&s, progs, 2);
    msg(&s, 0xC2, 1, 0);
    msg(&s, 0x92, 50, 90);
    ok = ok && midi_synth_active(&s) == 2;
    for (i = 3; i < 6; i++)
        if (vfreq(i) == sid_note_to_reg(50)) ok = ok && vr(i, SID_REG_CTRL) == (SID_SAWTOOTH | SID_GATE) &&
                                                  vr(i, SID_REG_AD) == 0x11;
    /* channel volume: the SID master volume */
    msg(&s, 0xB0, MIDI_CC_VOLUME, 80);
    ok = ok && sid_get_reg(0, 0x18) == 10;
    /* all notes off on channel 2 only, then everything */
    msg(&s, 0xB2, MIDI_CC_ALL_NOTES_OFF, 0);
    ok = ok && midi_synth_active(&s) == 1;
    midi_synth_all_off(&s);
    ok = ok && midi_synth_active(&s) == 0;
    return ok && midi_note_to_hz(69) == 440 && midi_note_to_hz(60) == 262 && midi_note_to_hz(81) == 880 &&
           midi_note_to_hz(0) == 8;
}

int main(void) {
    OUT[0] = t_init();
    OUT[1] = t_freq();
    OUT[2] = t_notes();
    OUT[3] = t_shape();
    OUT[4] = t_mirror();
    OUT[5] = t_volume();
    OUT[6] = t_digi();
    OUT[7] = t_parse();
    OUT[8] = t_synth();
    sid_init();
    OUT[9] = 0xAA;
    return 0;
}
