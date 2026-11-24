/* hw_audio_mixer.c — the SIDs' volumes on the MEGA65's audio mixer
 * (<hw_audio_mixer.h>, #645)
 *
 * Coefficient n of output o for input i is at $D6F4 = o + i (two bytes:
 * the gain, 16 bits); the SIDs are inputs $00 (SIDs 1-2) and $02 (SIDs
 * 3-4); the outputs here are the speakers' left $00 / right $20 and the
 * headphones' left $C0 / right $E0. Each write of $D6F5 waits after
 * $D6F4 (the mixer fetches the 16-bit coefficient: the address written
 * five times, as the hypervisor's audiomix_setcoefficient does).
 */

/* Written for standard C signedness (the fades' steps) */
#pragma cc45 signed_default

#include <hw_audio_mixer.h>

#define REG(a) (*(volatile unsigned char *)(a))

typedef struct {
    unsigned char l, r;             /* the volumes set */
    unsigned char muted;
    unsigned char fading;
    unsigned char fl, fr, tl, tr;   /* a fade: from, to */
    unsigned int dur, el;
} mx_pair;

static mx_pair mx[2];

static const unsigned char left_outs[2] = { 0x00, 0xC0 };
static const unsigned char right_outs[2] = { 0x20, 0xE0 };

static void mx_put(unsigned char reg, unsigned char v) {
    REG(0xD6F4) = reg;
    REG(0xD6F4) = reg;
    REG(0xD6F4) = reg;
    REG(0xD6F4) = reg;
    REG(0xD6F4) = reg;
    REG(0xD6F5) = v;
}

static unsigned char mx_get(unsigned char reg) {
    REG(0xD6F4) = reg;
    REG(0xD6F4) = reg;
    REG(0xD6F4) = reg;
    REG(0xD6F4) = reg;
    REG(0xD6F4) = reg;
    return REG(0xD6F5);
}

/* both bytes of the coefficients of input in on the outputs */
static void mx_gain(const unsigned char *outs, unsigned char in, unsigned char g) {
    unsigned char k;
    for (k = 0; k < 2; k++) {
        mx_put(outs[k] + in, g);
        mx_put(outs[k] + in + 1, g);
    }
}

static unsigned char mx_clamp(int v) {
    return v < 0 ? 0 : v > 15 ? 15 : (unsigned char)v;
}

/* the pair's volumes now (a fade's point, 0 when muted) to the mixer */
static void mx_apply(unsigned char p, unsigned char l, unsigned char r) {
    unsigned char in = (unsigned char)(p * 2);
    if (mx[p].muted) l = r = 0;
    mx_gain(left_outs, in, (unsigned char)(l * 17));
    mx_gain(right_outs, in, (unsigned char)(r * 17));
}

void hw_mixer_init(void) {
    unsigned char p;
    for (p = 0; p < 2; p++) {
        /* (a coefficient's high byte, rounded to the nearest volume) */
        mx[p].l = (unsigned char)((mx_get((unsigned char)(p * 2)) + 8) / 17);
        mx[p].r = (unsigned char)((mx_get((unsigned char)(0x20 + p * 2)) + 8) / 17);
        mx[p].muted = 0;
        mx[p].fading = 0;
    }
}

void hw_mixer_set_volume(sid_chip_t chip, int left, int right) {
    unsigned char p = (unsigned char)((chip >> 1) & 1);
    mx[p].l = mx_clamp(left);
    mx[p].r = mx_clamp(right);
    mx[p].fading = 0;
    mx_apply(p, mx[p].l, mx[p].r);
}

stereo_volume_t hw_mixer_get_volume(sid_chip_t chip) {
    stereo_volume_t v;
    unsigned char p = (unsigned char)((chip >> 1) & 1);
    v.left = mx[p].l;
    v.right = mx[p].r;
    return v;
}

void hw_mixer_set_master_volume(int left, int right) {
    hw_mixer_set_volume(SID_1, left, right);
    hw_mixer_set_volume(SID_3, left, right);
}

void hw_mixer_mute(sid_chip_t chip, int mute) {
    unsigned char p = (unsigned char)((chip >> 1) & 1);
    mx[p].muted = mute != 0;
    mx_apply(p, mx[p].l, mx[p].r);
}

void hw_mixer_pan(sid_chip_t chip, int pan) {
    unsigned char p = (unsigned char)((chip >> 1) & 1);
    int level = mx[p].l > mx[p].r ? mx[p].l : mx[p].r;
    int l = level, r = level;
    if (pan < 8) r = level * pan / 8;
    else if (pan > 8) l = level * (15 - (pan > 15 ? 15 : pan)) / 7;
    hw_mixer_set_volume(chip, l, r);
}

void hw_mixer_fade_to(sid_chip_t chip, int target_left, int target_right, int duration_ms) {
    unsigned char p = (unsigned char)((chip >> 1) & 1);
    if (duration_ms <= 0) {
        hw_mixer_set_volume(chip, target_left, target_right);
        return;
    }
    mx[p].fl = mx[p].l;
    mx[p].fr = mx[p].r;
    mx[p].tl = mx_clamp(target_left);
    mx[p].tr = mx_clamp(target_right);
    mx[p].dur = (unsigned int)duration_ms;
    mx[p].el = 0;
    mx[p].fading = 1;
}

/* a fade's point: from + (to - from) * el / dur */
static unsigned char mx_step(unsigned char from, unsigned char to, unsigned int el, unsigned int dur) {
    long d = (long)to - (long)from;
    return (unsigned char)(from + d * (long)el / (long)dur);
}

int hw_mixer_update_fades(int delta_ms) {
    unsigned char p;
    int going = 0;
    for (p = 0; p < 2; p++) {
        mx_pair *m = &mx[p];
        if (!m->fading) continue;
        if ((unsigned int)delta_ms >= m->dur - m->el) {
            m->fading = 0;
            m->l = m->tl;
            m->r = m->tr;
        } else {
            m->el += (unsigned int)delta_ms;
            m->l = mx_step(m->fl, m->tl, m->el, m->dur);
            m->r = mx_step(m->fr, m->tr, m->el, m->dur);
            going++;
        }
        mx_apply(p, m->l, m->r);
    }
    return going;
}

int hw_mixer_is_fading(sid_chip_t chip) {
    return mx[(chip >> 1) & 1].fading;
}

void hw_mixer_reset(void) {
    unsigned char p;
    for (p = 0; p < 2; p++) {
        unsigned char in = (unsigned char)(p * 2);
        /* (the hypervisor's audio_set_stereo: $BE on the SIDs' own side, $40 on the other) */
        mx_gain(p ? right_outs : left_outs, in, 0xBE);
        mx_gain(p ? left_outs : right_outs, in, 0x40);
        mx[p].muted = 0;
        mx[p].fading = 0;
    }
    hw_mixer_init();
}

void hw_mixer_crossfade(sid_chip_t from_chip, sid_chip_t to_chip, int duration_ms) {
    stereo_volume_t v = hw_mixer_get_volume(from_chip);
    if (((from_chip >> 1) & 1) == ((to_chip >> 1) & 1)) return;   /* (the same mixer input) */
    hw_mixer_fade_to(from_chip, 0, 0, duration_ms);
    hw_mixer_fade_to(to_chip, v.left, v.right, duration_ms);
}
