/* sprite_world.c — World Management Implementation */

#include "world.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#define MAX_WORLDS 4
#define MAX_ENTITIES_PER_WORLD 1024
#define MAX_GRIDS 8

typedef struct {
    float x, y, z;
    float width, height;
    float rotation;
    int active;
    int layer;
    void *data;
} entity_impl;

typedef struct {
    entity_impl entities[MAX_ENTITIES_PER_WORLD];
    int entity_count;
    int width, height;
    float gravity_x, gravity_y;
} world_impl;

typedef struct {
    world_impl *world;
    int cell_size;
    entity_impl **grid;
    int grid_width, grid_height;
} spatial_grid_impl;

static world_impl worlds[MAX_WORLDS];
static int world_count = 0;

static spatial_grid_impl grids[MAX_GRIDS];
static int grid_count = 0;

sprite_world_t sprite_world_create(int width, int height) {
    if (world_count >= MAX_WORLDS) return INVALID_WORLD;

    world_impl *world = &worlds[world_count];
    world->entity_count = 0;
    world->width = width;
    world->height = height;
    world->gravity_x = 0.0f;
    world->gravity_y = 9.8f;

    return (sprite_world_t)(intptr_t)world_count++;
}

void sprite_world_destroy(sprite_world_t world) {}

int sprite_world_get_info(sprite_world_t world, sprite_world_info_t *info) {
    intptr_t idx = (intptr_t)world;
    if (idx < 0 || idx >= world_count || !info) return 0;

    world_impl *w = &worlds[idx];
    info->entity_count = w->entity_count;
    info->width = w->width;
    info->height = w->height;
    info->gravity_x = w->gravity_x;
    info->gravity_y = w->gravity_y;
    return 1;
}

sprite_entity_t sprite_world_create_entity(sprite_world_t world, float x, float y) {
    intptr_t idx = (intptr_t)world;
    if (idx < 0 || idx >= world_count) return INVALID_ENTITY;

    world_impl *w = &worlds[idx];
    if (w->entity_count >= MAX_ENTITIES_PER_WORLD) return INVALID_ENTITY;

    entity_impl *entity = &w->entities[w->entity_count];
    entity->x = x;
    entity->y = y;
    entity->z = 0.0f;
    entity->width = 1.0f;
    entity->height = 1.0f;
    entity->rotation = 0.0f;
    entity->active = 1;
    entity->layer = 0;
    entity->data = NULL;

    return (sprite_entity_t)(intptr_t)w->entity_count++;
}

int sprite_world_destroy_entity(sprite_world_t world, sprite_entity_t entity) {
    intptr_t idx = (intptr_t)world;
    if (idx < 0 || idx >= world_count) return 0;

    world_impl *w = &worlds[idx];
    intptr_t e_idx = (intptr_t)entity;
    if (e_idx < 0 || e_idx >= w->entity_count) return 0;

    w->entities[e_idx].active = 0;
    return 1;
}

int sprite_world_get_entity_count(sprite_world_t world) {
    intptr_t idx = (intptr_t)world;
    if (idx < 0 || idx >= world_count) return 0;
    return worlds[idx].entity_count;
}

sprite_entity_t sprite_world_get_entity(sprite_world_t world, int index) {
    intptr_t idx = (intptr_t)world;
    if (idx < 0 || idx >= world_count) return INVALID_ENTITY;

    world_impl *w = &worlds[idx];
    if (index < 0 || index >= w->entity_count) return INVALID_ENTITY;
    return (sprite_entity_t)(intptr_t)index;
}

int sprite_entity_set_position(sprite_entity_t entity, float x, float y, float z) {
    intptr_t e_idx = (intptr_t)entity;
    for (int w = 0; w < world_count; w++) {
        if (e_idx < worlds[w].entity_count) {
            worlds[w].entities[e_idx].x = x;
            worlds[w].entities[e_idx].y = y;
            worlds[w].entities[e_idx].z = z;
            return 1;
        }
    }
    return 0;
}

int sprite_entity_get_position(sprite_entity_t entity, float *x, float *y, float *z) {
    intptr_t e_idx = (intptr_t)entity;
    if (!x || !y || !z) return 0;

    for (int w = 0; w < world_count; w++) {
        if (e_idx < worlds[w].entity_count) {
            *x = worlds[w].entities[e_idx].x;
            *y = worlds[w].entities[e_idx].y;
            *z = worlds[w].entities[e_idx].z;
            return 1;
        }
    }
    return 0;
}

int sprite_entity_set_size(sprite_entity_t entity, float width, float height) {
    intptr_t e_idx = (intptr_t)entity;
    for (int w = 0; w < world_count; w++) {
        if (e_idx < worlds[w].entity_count) {
            worlds[w].entities[e_idx].width = width;
            worlds[w].entities[e_idx].height = height;
            return 1;
        }
    }
    return 0;
}

int sprite_entity_get_size(sprite_entity_t entity, float *width, float *height) {
    intptr_t e_idx = (intptr_t)entity;
    if (!width || !height) return 0;

    for (int w = 0; w < world_count; w++) {
        if (e_idx < worlds[w].entity_count) {
            *width = worlds[w].entities[e_idx].width;
            *height = worlds[w].entities[e_idx].height;
            return 1;
        }
    }
    return 0;
}

int sprite_entity_set_rotation(sprite_entity_t entity, float rotation) {
    intptr_t e_idx = (intptr_t)entity;
    for (int w = 0; w < world_count; w++) {
        if (e_idx < worlds[w].entity_count) {
            worlds[w].entities[e_idx].rotation = rotation;
            return 1;
        }
    }
    return 0;
}

int sprite_entity_set_layer(sprite_entity_t entity, int layer) {
    intptr_t e_idx = (intptr_t)entity;
    for (int w = 0; w < world_count; w++) {
        if (e_idx < worlds[w].entity_count) {
            worlds[w].entities[e_idx].layer = layer;
            return 1;
        }
    }
    return 0;
}

int sprite_entity_set_active(sprite_entity_t entity, int active) {
    intptr_t e_idx = (intptr_t)entity;
    for (int w = 0; w < world_count; w++) {
        if (e_idx < worlds[w].entity_count) {
            worlds[w].entities[e_idx].active = active;
            return 1;
        }
    }
    return 0;
}

sprite_spatial_grid_t sprite_spatial_grid_create(sprite_world_t world, int cell_size) {
    intptr_t idx = (intptr_t)world;
    if (idx < 0 || idx >= world_count || grid_count >= MAX_GRIDS) return INVALID_GRID;

    spatial_grid_impl *grid = &grids[grid_count];
    grid->world = &worlds[idx];
    grid->cell_size = cell_size;
    grid->grid_width = (grid->world->width + cell_size - 1) / cell_size;
    grid->grid_height = (grid->world->height + cell_size - 1) / cell_size;
    grid->grid = NULL;

    return (sprite_spatial_grid_t)(intptr_t)grid_count++;
}

void sprite_spatial_grid_destroy(sprite_spatial_grid_t grid) {}

int sprite_spatial_grid_update(sprite_spatial_grid_t grid) {
    intptr_t idx = (intptr_t)grid;
    if (idx < 0 || idx >= grid_count) return 0;
    return grids[idx].world->entity_count;
}

int sprite_spatial_grid_query_bounds(sprite_spatial_grid_t grid, float x, float y,
                                      float width, float height,
                                      sprite_entity_t *results, int max_results) {
    intptr_t idx = (intptr_t)grid;
    if (idx < 0 || idx >= grid_count) return 0;

    spatial_grid_impl *g = &grids[idx];
    int count = 0;

    for (int i = 0; i < g->world->entity_count && count < max_results; i++) {
        entity_impl *e = &g->world->entities[i];
        if (!e->active) continue;

        if (e->x < x + width && e->x + e->width > x &&
            e->y < y + height && e->y + e->height > y) {
            results[count++] = (sprite_entity_t)(intptr_t)i;
        }
    }

    return count;
}

void sprite_world_set_gravity(sprite_world_t world, float gx, float gy) {
    intptr_t idx = (intptr_t)world;
    if (idx < 0 || idx >= world_count) return;
    worlds[idx].gravity_x = gx;
    worlds[idx].gravity_y = gy;
}

int sprite_world_update(sprite_world_t world, int delta_ms) {
    intptr_t idx = (intptr_t)world;
    if (idx < 0 || idx >= world_count) return 0;

    world_impl *w = &worlds[idx];
    float dt = delta_ms / 1000.0f;

    for (int i = 0; i < w->entity_count; i++) {
        if (!w->entities[i].active) continue;
    }

    return w->entity_count;
}

int sprite_world_raycast(sprite_world_t world, float x1, float y1, float x2, float y2,
                          sprite_entity_t *hit, float *hit_x, float *hit_y) {
    intptr_t idx = (intptr_t)world;
    if (idx < 0 || idx >= world_count || !hit || !hit_x || !hit_y) return 0;

    world_impl *w = &worlds[idx];
    float min_t = 1e9f;
    int hit_entity = -1;

    for (int i = 0; i < w->entity_count; i++) {
        if (!w->entities[i].active) continue;

        entity_impl *e = &w->entities[i];
        float t = (e->x - x1) / (x2 - x1 + 0.0001f);

        if (t >= 0 && t <= 1 && t < min_t) {
            min_t = t;
            hit_entity = i;
        }
    }

    if (hit_entity >= 0) {
        *hit = (sprite_entity_t)(intptr_t)hit_entity;
        *hit_x = x1 + (x2 - x1) * min_t;
        *hit_y = y1 + (y2 - y1) * min_t;
        return 1;
    }

    return 0;
}
