/* sprite_procedural_content.h — Procedural Content Generation (Phases 56-60)
 *
 * Phases 56-60: Terrain, dungeons, puzzles, vegetation, population
 */

#ifndef PROCEDURAL_CONTENT_H
#define PROCEDURAL_CONTENT_H

typedef void *sprite_terrain_t;
typedef void *sprite_dungeon_t;
typedef void *sprite_puzzle_t;
typedef void *sprite_vegetation_t;
typedef void *sprite_population_t;

#define INVALID_TERRAIN NULL
#define INVALID_DUNGEON NULL
#define INVALID_PUZZLE NULL
#define INVALID_VEGETATION NULL
#define INVALID_POPULATION NULL

typedef enum {
    TERRAIN_GRASS = 0,
    TERRAIN_ROCK = 1,
    TERRAIN_WATER = 2,
    TERRAIN_SAND = 3,
    TERRAIN_SNOW = 4,
} sprite_terrain_type_t;

/* Phase 56: Terrain Generation */
sprite_terrain_t sprite_terrain_create(int width, int height, int seed);
void sprite_terrain_destroy(sprite_terrain_t terrain);
int sprite_terrain_generate_heightmap(sprite_terrain_t terrain, float scale, float frequency);
int sprite_terrain_get_tile(sprite_terrain_t terrain, int x, int y);
void sprite_terrain_set_tile(sprite_terrain_t terrain, int x, int y, sprite_terrain_type_t type);
float sprite_terrain_get_height(sprite_terrain_t terrain, int x, int y);

/* Phase 57: Dungeon Generation */
sprite_dungeon_t sprite_dungeon_create(int width, int height, int num_rooms);
void sprite_dungeon_destroy(sprite_dungeon_t dungeon);
int sprite_dungeon_generate(sprite_dungeon_t dungeon, int seed);
int sprite_dungeon_get_tile(sprite_dungeon_t dungeon, int x, int y);
int sprite_dungeon_get_room_count(sprite_dungeon_t dungeon);
void sprite_dungeon_connect_rooms(sprite_dungeon_t dungeon);

/* Phase 58: Puzzle Generation */
sprite_puzzle_t sprite_puzzle_create(int difficulty, int seed);
void sprite_puzzle_destroy(sprite_puzzle_t puzzle);
int sprite_puzzle_generate(sprite_puzzle_t puzzle);
int sprite_puzzle_get_solution_length(sprite_puzzle_t puzzle);
int sprite_puzzle_verify_solution(sprite_puzzle_t puzzle, int *moves, int move_count);

/* Phase 59: Vegetation Generation */
sprite_vegetation_t sprite_vegetation_create(sprite_terrain_t terrain);
void sprite_vegetation_destroy(sprite_vegetation_t vegetation);
int sprite_vegetation_populate(sprite_vegetation_t vegetation, int density);
int sprite_vegetation_get_tree_count(sprite_vegetation_t vegetation);
int sprite_vegetation_get_tree_position(sprite_vegetation_t vegetation, int index, int *x, int *y);

/* Phase 60: NPC Population */
sprite_population_t sprite_population_create(int max_npcs);
void sprite_population_destroy(sprite_population_t population);
int sprite_population_spawn_npc(sprite_population_t population, int x, int y);
int sprite_population_update(sprite_population_t population, int delta_ms);
int sprite_population_get_npc_count(sprite_population_t population);

#endif
