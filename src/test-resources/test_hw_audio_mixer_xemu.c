/* test_hw_audio_mixer_xemu.c — <hw_audio_mixer.h> (#645): the SIDs' gains
 * in the MEGA65's audio mixer, read back through $D6F4 / $D6F5
 *
 * Results at $C0C0 (01 = pass):
 *   [0] hw_mixer_reset: the hypervisor's stereo setting ($BE on the SIDs'
 *       own side, $40 on the other), the volumes 11 and 4
 *   [1] set_volume: both bytes of the coefficients (the speakers' and the
 *       headphones'), 15 -> $FF; SID_1 and SID_2 share an input; SID_3's
 *       untouched; out of range clamped
 *   [2] mute: the gains 0, the volumes kept; unmuted: back
 *   [3] pan: 0 left only, 8 both at the louder, 15 right only, 4 between
 *   [4] fade_to: update's steps, is_fading, the end; duration 0 at once
 *   [5] crossfade, set_master_volume, init reading the mixer's volumes
 *   [6] $AA sentinel
 */
#include <hw_audio_mixer.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define REG(a) (*(volatile unsigned char *)(a))

static unsigned char coef(unsigned char reg) {
    REG(0xD6F4) = reg;
    REG(0xD6F4) = reg;
    REG(0xD6F4) = reg;
    REG(0xD6F4) = reg;
    return REG(0xD6F5);
}

/* input in's gain on the left outputs and on the right ones (all four
 * bytes of each side the same) */
static int gains(unsigned char in, unsigned char l, unsigned char r) {
    return coef(in) == l && coef(in + 1) == l && coef(0xC0 + in) == l && coef(0xC1 + in) == l &&
           coef(0x20 + in) == r && coef(0x21 + in) == r && coef(0xE0 + in) == r && coef(0xE1 + in) == r;
}

static int vol(sid_chip_t c, int l, int r) {
    stereo_volume_t v = hw_mixer_get_volume(c);
    return v.left == l && v.right == r;
}

int main(void) {
    int ok;
    OUT[6] = 0;

    hw_mixer_reset();
    ok = gains(0x00, 0xBE, 0x40) && gains(0x02, 0x40, 0xBE) && vol(SID_1, 11, 4) && vol(SID_4, 4, 11);
    OUT[0] = ok;

    hw_mixer_set_volume(SID_1, 15, 0);
    ok = gains(0x00, 0xFF, 0x00) && vol(SID_2, 15, 0) && gains(0x02, 0x40, 0xBE);
    hw_mixer_set_volume(SID_2, 20, 3);
    ok = ok && vol(SID_1, 15, 3) && gains(0x00, 0xFF, 51);
    hw_mixer_set_volume(SID_1, 15, 0);
    OUT[1] = ok;

    hw_mixer_mute(SID_1, 1);
    ok = gains(0x00, 0, 0) && vol(SID_1, 15, 0);
    hw_mixer_mute(SID_1, 0);
    ok = ok && gains(0x00, 0xFF, 0);
    OUT[2] = ok;

    hw_mixer_pan(SID_3, 0);                 /* (11 and 4: 11 the louder) */
    ok = vol(SID_3, 11, 0);
    hw_mixer_pan(SID_3, 8);
    ok = ok && vol(SID_3, 11, 11);
    hw_mixer_pan(SID_3, 15);
    ok = ok && vol(SID_3, 0, 11) && gains(0x02, 0, 187);
    hw_mixer_pan(SID_3, 4);
    ok = ok && vol(SID_3, 11, 5);
    hw_mixer_pan(SID_3, 15);
    OUT[3] = ok;

    hw_mixer_fade_to(SID_1, 0, 15, 1000);
    ok = hw_mixer_is_fading(SID_1) && !hw_mixer_is_fading(SID_3) && hw_mixer_update_fades(500) == 1;
    ok = ok && vol(SID_1, 8, 7) && gains(0x00, 8 * 17, 7 * 17);
    ok = ok && hw_mixer_update_fades(600) == 0 && !hw_mixer_is_fading(SID_1) && vol(SID_1, 0, 15);
    hw_mixer_fade_to(SID_2, 6, 6, 0);
    ok = ok && !hw_mixer_is_fading(SID_1) && vol(SID_1, 6, 6);
    hw_mixer_set_volume(SID_1, 0, 15);
    OUT[4] = ok;

    hw_mixer_crossfade(SID_1, SID_3, 100);
    ok = hw_mixer_is_fading(SID_1) && hw_mixer_is_fading(SID_3) && hw_mixer_update_fades(50) == 2;
    ok = ok && vol(SID_1, 0, 8) && vol(SID_3, 0, 13);
    ok = ok && hw_mixer_update_fades(50) == 0 && vol(SID_1, 0, 0) && vol(SID_3, 0, 15);
    hw_mixer_set_master_volume(5, 6);
    ok = ok && vol(SID_2, 5, 6) && vol(SID_4, 5, 6) && gains(0x00, 85, 102) && gains(0x02, 85, 102);
    hw_mixer_init();
    ok = ok && vol(SID_1, 5, 6) && vol(SID_3, 5, 6);
    OUT[5] = ok;

    hw_mixer_reset();
    OUT[6] = 0xAA;
    return 0;
}
