/* sprite_camera.c — Camera System Implementation */

#include "sprite_camera.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#define MAX_CAMERAS 8

typedef struct {
    float x, y;
    float zoom;
    float rotation;
    int viewport_width, viewport_height;
    sprite_camera_follow_mode_t follow_mode;
    float *follow_target_x, *follow_target_y;
    float follow_speed;
    float follow_bounds[4];
    float shake_intensity;
    float shake_duration;
    float shake_time;
    float pan_duration;
    float pan_time;
    float pan_start_x, pan_start_y;
    float pan_target_x, pan_target_y;
    float zoom_duration;
    float zoom_time;
    float zoom_start;
    float zoom_target;
} camera_impl;

static camera_impl cameras[MAX_CAMERAS];
static int camera_count = 0;

sprite_camera_t sprite_camera_create(int viewport_width, int viewport_height) {
    if (camera_count >= MAX_CAMERAS) return INVALID_CAMERA;

    camera_impl *camera = &cameras[camera_count];
    camera->x = camera->y = 0.0f;
    camera->zoom = 1.0f;
    camera->rotation = 0.0f;
    camera->viewport_width = viewport_width;
    camera->viewport_height = viewport_height;
    camera->follow_mode = FOLLOW_NONE;
    camera->follow_target_x = NULL;
    camera->follow_target_y = NULL;
    camera->follow_speed = 5.0f;
    camera->follow_bounds[0] = -9999.0f;
    camera->follow_bounds[1] = 9999.0f;
    camera->follow_bounds[2] = -9999.0f;
    camera->follow_bounds[3] = 9999.0f;
    camera->shake_intensity = 0.0f;
    camera->shake_duration = 0.0f;
    camera->shake_time = 0.0f;
    camera->pan_duration = 0.0f;
    camera->pan_time = 0.0f;

    return (sprite_camera_t)(intptr_t)camera_count++;
}

void sprite_camera_destroy(sprite_camera_t camera) {}

void sprite_camera_set_position(sprite_camera_t camera, float x, float y) {
    intptr_t idx = (intptr_t)camera;
    if (idx < 0 || idx >= camera_count) return;
    cameras[idx].x = x;
    cameras[idx].y = y;
}

void sprite_camera_get_position(sprite_camera_t camera, float *x, float *y) {
    intptr_t idx = (intptr_t)camera;
    if (idx < 0 || idx >= camera_count || !x || !y) return;
    *x = cameras[idx].x;
    *y = cameras[idx].y;
}

void sprite_camera_pan(sprite_camera_t camera, float dx, float dy) {
    intptr_t idx = (intptr_t)camera;
    if (idx < 0 || idx >= camera_count) return;
    cameras[idx].x += dx;
    cameras[idx].y += dy;
}

int sprite_camera_pan_to(sprite_camera_t camera, float x, float y, float duration) {
    intptr_t idx = (intptr_t)camera;
    if (idx < 0 || idx >= camera_count) return 0;

    camera_impl *c = &cameras[idx];
    c->pan_start_x = c->x;
    c->pan_start_y = c->y;
    c->pan_target_x = x;
    c->pan_target_y = y;
    c->pan_duration = duration;
    c->pan_time = 0.0f;
    return 1;
}

void sprite_camera_set_zoom(sprite_camera_t camera, float zoom) {
    intptr_t idx = (intptr_t)camera;
    if (idx < 0 || idx >= camera_count) return;
    if (zoom < 0.1f) zoom = 0.1f;
    cameras[idx].zoom = zoom;
}

float sprite_camera_get_zoom(sprite_camera_t camera) {
    intptr_t idx = (intptr_t)camera;
    if (idx < 0 || idx >= camera_count) return 1.0f;
    return cameras[idx].zoom;
}

int sprite_camera_zoom_to(sprite_camera_t camera, float target_zoom, float duration) {
    intptr_t idx = (intptr_t)camera;
    if (idx < 0 || idx >= camera_count) return 0;

    camera_impl *c = &cameras[idx];
    c->zoom_start = c->zoom;
    c->zoom_target = target_zoom;
    c->zoom_duration = duration;
    c->zoom_time = 0.0f;
    return 1;
}

void sprite_camera_set_rotation(sprite_camera_t camera, float rotation) {
    intptr_t idx = (intptr_t)camera;
    if (idx < 0 || idx >= camera_count) return;
    cameras[idx].rotation = rotation;
}

float sprite_camera_get_rotation(sprite_camera_t camera) {
    intptr_t idx = (intptr_t)camera;
    if (idx < 0 || idx >= camera_count) return 0.0f;
    return cameras[idx].rotation;
}

void sprite_camera_set_follow_mode(sprite_camera_t camera, sprite_camera_follow_mode_t mode) {
    intptr_t idx = (intptr_t)camera;
    if (idx < 0 || idx >= camera_count) return;
    cameras[idx].follow_mode = mode;
}

void sprite_camera_set_follow_target(sprite_camera_t camera, float *target_x, float *target_y) {
    intptr_t idx = (intptr_t)camera;
    if (idx < 0 || idx >= camera_count) return;
    cameras[idx].follow_target_x = target_x;
    cameras[idx].follow_target_y = target_y;
}

void sprite_camera_set_follow_speed(sprite_camera_t camera, float speed) {
    intptr_t idx = (intptr_t)camera;
    if (idx < 0 || idx >= camera_count) return;
    cameras[idx].follow_speed = speed;
}

void sprite_camera_set_follow_bounds(sprite_camera_t camera, float x_min, float x_max,
                                      float y_min, float y_max) {
    intptr_t idx = (intptr_t)camera;
    if (idx < 0 || idx >= camera_count) return;
    cameras[idx].follow_bounds[0] = x_min;
    cameras[idx].follow_bounds[1] = x_max;
    cameras[idx].follow_bounds[2] = y_min;
    cameras[idx].follow_bounds[3] = y_max;
}

int sprite_camera_shake(sprite_camera_t camera, float intensity, float duration) {
    intptr_t idx = (intptr_t)camera;
    if (idx < 0 || idx >= camera_count) return 0;

    camera_impl *c = &cameras[idx];
    c->shake_intensity = intensity;
    c->shake_duration = duration;
    c->shake_time = 0.0f;
    return 1;
}

int sprite_camera_update(sprite_camera_t camera, int delta_ms) {
    intptr_t idx = (intptr_t)camera;
    if (idx < 0 || idx >= camera_count) return 0;

    camera_impl *c = &cameras[idx];
    float dt = delta_ms / 1000.0f;

    if (c->pan_duration > 0.0f && c->pan_time < c->pan_duration) {
        c->pan_time += dt;
        float t = c->pan_time / c->pan_duration;
        if (t > 1.0f) t = 1.0f;
        c->x = c->pan_start_x + (c->pan_target_x - c->pan_start_x) * t;
        c->y = c->pan_start_y + (c->pan_target_y - c->pan_start_y) * t;
    }

    if (c->zoom_duration > 0.0f && c->zoom_time < c->zoom_duration) {
        c->zoom_time += dt;
        float t = c->zoom_time / c->zoom_duration;
        if (t > 1.0f) t = 1.0f;
        c->zoom = c->zoom_start + (c->zoom_target - c->zoom_start) * t;
    }

    if (c->follow_mode != FOLLOW_NONE && c->follow_target_x && c->follow_target_y) {
        float target_x = *c->follow_target_x;
        float target_y = *c->follow_target_y;

        if (c->follow_mode == FOLLOW_LOCKED) {
            c->x = target_x;
            c->y = target_y;
        } else if (c->follow_mode == FOLLOW_SMOOTH) {
            float dx = target_x - c->x;
            float dy = target_y - c->y;
            c->x += dx * c->follow_speed * dt;
            c->y += dy * c->follow_speed * dt;
        }

        if (c->x < c->follow_bounds[0]) c->x = c->follow_bounds[0];
        if (c->x > c->follow_bounds[1]) c->x = c->follow_bounds[1];
        if (c->y < c->follow_bounds[2]) c->y = c->follow_bounds[2];
        if (c->y > c->follow_bounds[3]) c->y = c->follow_bounds[3];
    }

    if (c->shake_time < c->shake_duration) {
        c->shake_time += dt;
    }

    return 1;
}

int sprite_camera_get_state(sprite_camera_t camera, sprite_camera_state_t *state) {
    intptr_t idx = (intptr_t)camera;
    if (idx < 0 || idx >= camera_count || !state) return 0;

    camera_impl *c = &cameras[idx];
    state->x = c->x;
    state->y = c->y;
    state->zoom = c->zoom;
    state->rotation = c->rotation;
    state->viewport_width = c->viewport_width;
    state->viewport_height = c->viewport_height;
    return 1;
}

int sprite_camera_get_viewport(sprite_camera_t camera, float *x, float *y,
                                float *width, float *height) {
    intptr_t idx = (intptr_t)camera;
    if (idx < 0 || idx >= camera_count || !x || !y || !width || !height) return 0;

    camera_impl *c = &cameras[idx];
    *width = c->viewport_width / c->zoom;
    *height = c->viewport_height / c->zoom;
    *x = c->x - (*width / 2.0f);
    *y = c->y - (*height / 2.0f);
    return 1;
}

void sprite_camera_world_to_screen(sprite_camera_t camera, float world_x, float world_y,
                                    float *screen_x, float *screen_y) {
    intptr_t idx = (intptr_t)camera;
    if (idx < 0 || idx >= camera_count || !screen_x || !screen_y) return;

    camera_impl *c = &cameras[idx];
    *screen_x = (world_x - c->x) * c->zoom + c->viewport_width / 2.0f;
    *screen_y = (world_y - c->y) * c->zoom + c->viewport_height / 2.0f;
}

void sprite_camera_screen_to_world(sprite_camera_t camera, float screen_x, float screen_y,
                                    float *world_x, float *world_y) {
    intptr_t idx = (intptr_t)camera;
    if (idx < 0 || idx >= camera_count || !world_x || !world_y) return;

    camera_impl *c = &cameras[idx];
    *world_x = c->x + (screen_x - c->viewport_width / 2.0f) / c->zoom;
    *world_y = c->y + (screen_y - c->viewport_height / 2.0f) / c->zoom;
}
