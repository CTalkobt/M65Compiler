/* sprite_world.h — Phase 61: World/Level Management
 *
 * Level/world management with entity hierarchies and spatial partitioning.
 */

#ifndef WORLD_H
#define WORLD_H

typedef void *sprite_world_t;
typedef void *sprite_entity_t;
typedef void *sprite_spatial_grid_t;

#define INVALID_WORLD NULL
#define INVALID_ENTITY NULL
#define INVALID_GRID NULL

typedef struct {
    float x, y, z;
    float width, height;
    float rotation;
    int active;
    int layer;
    void *data;
} sprite_entity_info_t;

typedef struct {
    int entity_count;
    int width, height;
    float gravity_x, gravity_y;
} sprite_world_info_t;

/* World Creation */
sprite_world_t sprite_world_create(int width, int height);
void sprite_world_destroy(sprite_world_t world);
int sprite_world_get_info(sprite_world_t world, sprite_world_info_t *info);

/* Entity Management */
sprite_entity_t sprite_world_create_entity(sprite_world_t world, float x, float y);
int sprite_world_destroy_entity(sprite_world_t world, sprite_entity_t entity);
int sprite_world_get_entity_count(sprite_world_t world);
sprite_entity_t sprite_world_get_entity(sprite_world_t world, int index);

/* Entity Properties */
int sprite_entity_set_position(sprite_entity_t entity, float x, float y, float z);
int sprite_entity_get_position(sprite_entity_t entity, float *x, float *y, float *z);
int sprite_entity_set_size(sprite_entity_t entity, float width, float height);
int sprite_entity_get_size(sprite_entity_t entity, float *width, float *height);
int sprite_entity_set_rotation(sprite_entity_t entity, float rotation);
int sprite_entity_set_layer(sprite_entity_t entity, int layer);
int sprite_entity_set_active(sprite_entity_t entity, int active);

/* Spatial Queries */
sprite_spatial_grid_t sprite_spatial_grid_create(sprite_world_t world, int cell_size);
void sprite_spatial_grid_destroy(sprite_spatial_grid_t grid);
int sprite_spatial_grid_update(sprite_spatial_grid_t grid);
int sprite_spatial_grid_query_bounds(sprite_spatial_grid_t grid, float x, float y,
                                      float width, float height,
                                      sprite_entity_t *results, int max_results);

/* Physics */
void sprite_world_set_gravity(sprite_world_t world, float gx, float gy);
int sprite_world_update(sprite_world_t world, int delta_ms);
int sprite_world_raycast(sprite_world_t world, float x1, float y1, float x2, float y2,
                          sprite_entity_t *hit, float *hit_x, float *hit_y);

#endif
