/* sprite_camera.h — Phase 54: Camera System
 *
 * Camera control with follow, pan, zoom, and shake effects.
 */

#ifndef SPRITE_CAMERA_H
#define SPRITE_CAMERA_H

typedef void *sprite_camera_t;

#define INVALID_CAMERA NULL

typedef enum {
    FOLLOW_NONE = 0,
    FOLLOW_LOCKED = 1,
    FOLLOW_SMOOTH = 2,
    FOLLOW_LERP = 3,
} sprite_camera_follow_mode_t;

typedef struct {
    float x, y;
    float zoom;
    float rotation;
    int viewport_width, viewport_height;
} sprite_camera_state_t;

/* Camera Creation */
sprite_camera_t sprite_camera_create(int viewport_width, int viewport_height);
void sprite_camera_destroy(sprite_camera_t camera);

/* Position Control */
void sprite_camera_set_position(sprite_camera_t camera, float x, float y);
void sprite_camera_get_position(sprite_camera_t camera, float *x, float *y);
void sprite_camera_pan(sprite_camera_t camera, float dx, float dy);
int sprite_camera_pan_to(sprite_camera_t camera, float x, float y, float duration);

/* Zoom Control */
void sprite_camera_set_zoom(sprite_camera_t camera, float zoom);
float sprite_camera_get_zoom(sprite_camera_t camera);
int sprite_camera_zoom_to(sprite_camera_t camera, float target_zoom, float duration);

/* Rotation Control */
void sprite_camera_set_rotation(sprite_camera_t camera, float rotation);
float sprite_camera_get_rotation(sprite_camera_t camera);

/* Follow Target */
void sprite_camera_set_follow_mode(sprite_camera_t camera, sprite_camera_follow_mode_t mode);
void sprite_camera_set_follow_target(sprite_camera_t camera, float *target_x, float *target_y);
void sprite_camera_set_follow_speed(sprite_camera_t camera, float speed);
void sprite_camera_set_follow_bounds(sprite_camera_t camera, float x_min, float x_max,
                                      float y_min, float y_max);

/* Effects */
int sprite_camera_shake(sprite_camera_t camera, float intensity, float duration);
int sprite_camera_update(sprite_camera_t camera, int delta_ms);

/* State */
int sprite_camera_get_state(sprite_camera_t camera, sprite_camera_state_t *state);
int sprite_camera_get_viewport(sprite_camera_t camera, float *x, float *y,
                                float *width, float *height);

/* World to Screen Transformation */
void sprite_camera_world_to_screen(sprite_camera_t camera, float world_x, float world_y,
                                    float *screen_x, float *screen_y);
void sprite_camera_screen_to_world(sprite_camera_t camera, float screen_x, float screen_y,
                                    float *world_x, float *world_y);

#endif
