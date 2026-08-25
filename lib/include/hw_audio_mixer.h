/* hw_audio_mixer.h — Phase 84: Audio Mixer Control
 *
 * Hardware audio mixer for controlling SID chip volumes, panning,
 * and effects routing on MEGA65.
 */

#ifndef HW_AUDIO_MIXER_H
#define HW_AUDIO_MIXER_H

/* SID chip identifiers */
typedef enum {
    SID_1 = 0,
    SID_2 = 1,
    SID_3 = 2,
    SID_4 = 3,
} sid_chip_t;

/* Volume range: 0-15 per channel */
typedef struct {
    int left;   /* 0-15 */
    int right;  /* 0-15 */
} stereo_volume_t;

/* Initialize audio mixer */
void hw_mixer_init(void);

/* Set SID volume (stereo) */
void hw_mixer_set_volume(sid_chip_t chip, int left, int right);

/* Get SID volume */
stereo_volume_t hw_mixer_get_volume(sid_chip_t chip);

/* Set all SIDs to same volume */
void hw_mixer_set_master_volume(int left, int right);

/* Mute/unmute SID chip */
void hw_mixer_mute(sid_chip_t chip, int mute);

/* Pan SID (0=left, 8=center, 15=right) */
void hw_mixer_pan(sid_chip_t chip, int pan);

/* Fade volume over time (call with delta_ms each frame) */
void hw_mixer_fade_to(sid_chip_t chip, int target_left, int target_right, int duration_ms);
int hw_mixer_update_fades(int delta_ms);

/* Get mixer status */
int hw_mixer_is_fading(sid_chip_t chip);

/* Reset mixer to defaults */
void hw_mixer_reset(void);

/* Crossfade between two SID chips */
void hw_mixer_crossfade(sid_chip_t from_chip, sid_chip_t to_chip, int duration_ms);

#endif
