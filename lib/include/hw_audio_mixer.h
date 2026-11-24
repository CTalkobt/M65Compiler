/* hw_audio_mixer.h — the MEGA65's audio mixer for the SIDs (#645)
 *
 * The mixer ($D6F4 the coefficient's number, $D6F5 its value: a cross-bar
 * of 16-bit gains, an input on an output) has the SIDs as two inputs: SIDs
 * 1 and 2 ($D400, $D420) are "SID left", SIDs 3 and 4 ($D440, $D460) "SID
 * right". So SID_1 and SID_2 share their volumes, as SID_3 and SID_4 do.
 * A volume (0-15) is the input's gain on the left outputs (the speakers'
 * and the headphones' left) and on the right ones: the coefficient's two
 * bytes both volume * 17 (15: $FFFF), as the hypervisor writes them.
 *
 *   hw_mixer_init             the volumes as the mixer has them now
 *   hw_mixer_pan              0 left only, 8 both at the louder of the two
 *                             volumes, 15 right only (between: the other
 *                             side less)
 *   hw_mixer_mute             0 on both sides; unmuted: the volumes again
 *   hw_mixer_fade_to          to the target over duration_ms, stepped by
 *                             hw_mixer_update_fades(delta_ms) (its result:
 *                             the fades still going)
 *   hw_mixer_crossfade        from_chip down to 0, to_chip up to from_chip's
 *                             volumes, at once over duration_ms
 *   hw_mixer_reset            the hypervisor's stereo setting: each side's
 *                             SIDs on its own side ($BE: 11) and a little on
 *                             the other ($40: 4)
 *
 * The mixer's other inputs (the audio DMA's digi channels, the OPL FM, the
 * microphones) and its master volumes are left as they are. A program that
 * changes the volumes changes them for BASIC too after it ends (the
 * hypervisor sets them at reset). Library: stdlib/hw_audio_mixer.c. Test:
 * test_hw_audio_mixer_xemu.c.
 */

#ifndef HW_AUDIO_MIXER_H
#define HW_AUDIO_MIXER_H

/* SID chip identifiers (1 and 2 one mixer input, 3 and 4 the other) */
typedef enum {
    SID_1 = 0,
    SID_2 = 1,
    SID_3 = 2,
    SID_4 = 3,
} sid_chip_t;

/* Volume range: 0-15 per side */
typedef struct {
    int left;   /* 0-15 */
    int right;  /* 0-15 */
} stereo_volume_t;

/* The volumes as the mixer has them (no fades, nothing muted) */
void hw_mixer_init(void);

/* Set SID volume (stereo; 0-15, more is 15) */
void hw_mixer_set_volume(sid_chip_t chip, int left, int right);

/* Get SID volume (the volumes set, also while muted) */
stereo_volume_t hw_mixer_get_volume(sid_chip_t chip);

/* Set all SIDs to same volume */
void hw_mixer_set_master_volume(int left, int right);

/* Mute (1) / unmute (0) SID chip */
void hw_mixer_mute(sid_chip_t chip, int mute);

/* Pan SID (0=left, 8=center, 15=right) */
void hw_mixer_pan(sid_chip_t chip, int pan);

/* Fade volume over time (call hw_mixer_update_fades with delta_ms each frame) */
void hw_mixer_fade_to(sid_chip_t chip, int target_left, int target_right, int duration_ms);
int hw_mixer_update_fades(int delta_ms);

/* 1 while the chip's volumes are fading */
int hw_mixer_is_fading(sid_chip_t chip);

/* Reset mixer to the hypervisor's defaults */
void hw_mixer_reset(void);

/* Crossfade between two SID chips */
void hw_mixer_crossfade(sid_chip_t from_chip, sid_chip_t to_chip, int duration_ms);

#endif
