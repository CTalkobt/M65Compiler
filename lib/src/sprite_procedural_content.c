/* sprite_procedural_content.c — Procedural Content Implementation */

#include "sprite_procedural_content.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#define MAX_TERRAINS 8
#define MAX_DUNGEONS 8
#define MAX_PUZZLES 8
#define MAX_VEGETATION 8
#define MAX_POPULATIONS 4

typedef struct {
    int width, height;
    int seed;
    float *heightmap;
    int *tiles;
} terrain_impl;

typedef struct {
    int width, height;
    int num_rooms;
    int *layout;
    int room_count;
} dungeon_impl;

typedef struct {
    int difficulty;
    int seed;
    int *solution;
    int solution_length;
} puzzle_impl;

typedef struct {
    terrain_impl *terrain;
    int tree_count;
    int *tree_x, *tree_y;
} vegetation_impl;

typedef struct {
    int *npc_x, *npc_y;
    int *npc_type;
    int npc_count;
    int max_npcs;
} population_impl;

static terrain_impl terrains[MAX_TERRAINS];
static int terrain_count = 0;

static dungeon_impl dungeons[MAX_DUNGEONS];
static int dungeon_count = 0;

static puzzle_impl puzzles[MAX_PUZZLES];
static int puzzle_count = 0;

static vegetation_impl vegetations[MAX_VEGETATION];
static int vegetation_count = 0;

static population_impl populations[MAX_POPULATIONS];
static int population_count = 0;

/* Simplex noise approximation for terrain */
static float simplex_noise(int x, int y, int seed) {
    int n = seed + x * 73856093 ^ y * 19349663;
    n = (n << 13) ^ n;
    return 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
}

/* Phase 56: Terrain */
sprite_terrain_t sprite_terrain_create(int width, int height, int seed) {
    if (terrain_count >= MAX_TERRAINS) return INVALID_TERRAIN;
    terrain_impl *terrain = &terrains[terrain_count];
    terrain->width = width;
    terrain->height = height;
    terrain->seed = seed;
    terrain->heightmap = (float *)malloc(width * height * sizeof(float));
    terrain->tiles = (int *)malloc(width * height * sizeof(int));
    memset(terrain->tiles, TERRAIN_GRASS, width * height * sizeof(int));
    return (sprite_terrain_t)(intptr_t)terrain_count++;
}

void sprite_terrain_destroy(sprite_terrain_t terrain) {
    intptr_t idx = (intptr_t)terrain;
    if (idx < 0 || idx >= terrain_count) return;
    free(terrains[idx].heightmap);
    free(terrains[idx].tiles);
}

int sprite_terrain_generate_heightmap(sprite_terrain_t terrain, float scale, float frequency) {
    intptr_t idx = (intptr_t)terrain;
    if (idx < 0 || idx >= terrain_count) return 0;

    terrain_impl *t = &terrains[idx];
    for (int y = 0; y < t->height; y++) {
        for (int x = 0; x < t->width; x++) {
            float height = 0.0f;
            for (int i = 0; i < 4; i++) {
                float freq = frequency * (1 << i);
                height += simplex_noise(x, y, t->seed + i) / freq;
            }
            t->heightmap[y * t->width + x] = (height + 1.0f) * 0.5f * scale;
        }
    }
    return 1;
}

int sprite_terrain_get_tile(sprite_terrain_t terrain, int x, int y) {
    intptr_t idx = (intptr_t)terrain;
    if (idx < 0 || idx >= terrain_count) return TERRAIN_GRASS;
    terrain_impl *t = &terrains[idx];
    if (x < 0 || x >= t->width || y < 0 || y >= t->height) return TERRAIN_GRASS;
    return t->tiles[y * t->width + x];
}

void sprite_terrain_set_tile(sprite_terrain_t terrain, int x, int y, sprite_terrain_type_t type) {
    intptr_t idx = (intptr_t)terrain;
    if (idx < 0 || idx >= terrain_count) return;
    terrain_impl *t = &terrains[idx];
    if (x < 0 || x >= t->width || y < 0 || y >= t->height) return;
    t->tiles[y * t->width + x] = type;
}

float sprite_terrain_get_height(sprite_terrain_t terrain, int x, int y) {
    intptr_t idx = (intptr_t)terrain;
    if (idx < 0 || idx >= terrain_count) return 0.0f;
    terrain_impl *t = &terrains[idx];
    if (x < 0 || x >= t->width || y < 0 || y >= t->height) return 0.0f;
    return t->heightmap[y * t->width + x];
}

/* Phase 57: Dungeon */
sprite_dungeon_t sprite_dungeon_create(int width, int height, int num_rooms) {
    if (dungeon_count >= MAX_DUNGEONS) return INVALID_DUNGEON;
    dungeon_impl *dungeon = &dungeons[dungeon_count];
    dungeon->width = width;
    dungeon->height = height;
    dungeon->num_rooms = num_rooms;
    dungeon->layout = (int *)malloc(width * height * sizeof(int));
    memset(dungeon->layout, 0, width * height * sizeof(int));
    dungeon->room_count = 0;
    return (sprite_dungeon_t)(intptr_t)dungeon_count++;
}

void sprite_dungeon_destroy(sprite_dungeon_t dungeon) {
    intptr_t idx = (intptr_t)dungeon;
    if (idx < 0 || idx >= dungeon_count) return;
    free(dungeons[idx].layout);
}

int sprite_dungeon_generate(sprite_dungeon_t dungeon, int seed) {
    intptr_t idx = (intptr_t)dungeon;
    if (idx < 0 || idx >= dungeon_count) return 0;

    dungeon_impl *d = &dungeons[idx];
    srand(seed);
    for (int i = 0; i < d->num_rooms; i++) {
        int room_x = rand() % (d->width - 10) + 5;
        int room_y = rand() % (d->height - 10) + 5;
        int room_w = rand() % 5 + 4;
        int room_h = rand() % 5 + 4;

        for (int ry = room_y; ry < room_y + room_h && ry < d->height; ry++) {
            for (int rx = room_x; rx < room_x + room_w && rx < d->width; rx++) {
                d->layout[ry * d->width + rx] = 1;
            }
        }
        d->room_count++;
    }
    return 1;
}

int sprite_dungeon_get_tile(sprite_dungeon_t dungeon, int x, int y) {
    intptr_t idx = (intptr_t)dungeon;
    if (idx < 0 || idx >= dungeon_count) return 0;
    dungeon_impl *d = &dungeons[idx];
    if (x < 0 || x >= d->width || y < 0 || y >= d->height) return 0;
    return d->layout[y * d->width + x];
}

int sprite_dungeon_get_room_count(sprite_dungeon_t dungeon) {
    intptr_t idx = (intptr_t)dungeon;
    if (idx < 0 || idx >= dungeon_count) return 0;
    return dungeons[idx].room_count;
}

void sprite_dungeon_connect_rooms(sprite_dungeon_t dungeon) {
}

/* Phase 58: Puzzle */
sprite_puzzle_t sprite_puzzle_create(int difficulty, int seed) {
    if (puzzle_count >= MAX_PUZZLES) return INVALID_PUZZLE;
    puzzle_impl *puzzle = &puzzles[puzzle_count];
    puzzle->difficulty = difficulty;
    puzzle->seed = seed;
    puzzle->solution_length = difficulty * 3;
    puzzle->solution = (int *)malloc(puzzle->solution_length * sizeof(int));
    srand(seed);
    for (int i = 0; i < puzzle->solution_length; i++) {
        puzzle->solution[i] = rand() % 4;
    }
    return (sprite_puzzle_t)(intptr_t)puzzle_count++;
}

void sprite_puzzle_destroy(sprite_puzzle_t puzzle) {
    intptr_t idx = (intptr_t)puzzle;
    if (idx < 0 || idx >= puzzle_count) return;
    free(puzzles[idx].solution);
}

int sprite_puzzle_generate(sprite_puzzle_t puzzle) {
    return 1;
}

int sprite_puzzle_get_solution_length(sprite_puzzle_t puzzle) {
    intptr_t idx = (intptr_t)puzzle;
    if (idx < 0 || idx >= puzzle_count) return 0;
    return puzzles[idx].solution_length;
}

int sprite_puzzle_verify_solution(sprite_puzzle_t puzzle, int *moves, int move_count) {
    intptr_t idx = (intptr_t)puzzle;
    if (idx < 0 || idx >= puzzle_count) return 0;
    puzzle_impl *p = &puzzles[idx];
    if (move_count != p->solution_length) return 0;
    for (int i = 0; i < move_count; i++) {
        if (moves[i] != p->solution[i]) return 0;
    }
    return 1;
}

/* Phase 59: Vegetation */
sprite_vegetation_t sprite_vegetation_create(sprite_terrain_t terrain) {
    if (vegetation_count >= MAX_VEGETATION) return INVALID_VEGETATION;
    vegetation_impl *veg = &vegetations[vegetation_count];
    intptr_t idx = (intptr_t)terrain;
    veg->terrain = (idx >= 0 && idx < terrain_count) ? &terrains[idx] : NULL;
    veg->tree_count = 0;
    veg->tree_x = (int *)malloc(1000 * sizeof(int));
    veg->tree_y = (int *)malloc(1000 * sizeof(int));
    return (sprite_vegetation_t)(intptr_t)vegetation_count++;
}

void sprite_vegetation_destroy(sprite_vegetation_t vegetation) {
    intptr_t idx = (intptr_t)vegetation;
    if (idx < 0 || idx >= vegetation_count) return;
    free(vegetations[idx].tree_x);
    free(vegetations[idx].tree_y);
}

int sprite_vegetation_populate(sprite_vegetation_t vegetation, int density) {
    intptr_t idx = (intptr_t)vegetation;
    if (idx < 0 || idx >= vegetation_count) return 0;

    vegetation_impl *veg = &vegetations[idx];
    if (!veg->terrain) return 0;

    srand(12345);
    int width = veg->terrain->width;
    int height = veg->terrain->height;
    int count = 0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if ((rand() % 100) < density) {
                veg->tree_x[count] = x;
                veg->tree_y[count] = y;
                count++;
                if (count >= 1000) break;
            }
        }
        if (count >= 1000) break;
    }
    veg->tree_count = count;
    return count;
}

int sprite_vegetation_get_tree_count(sprite_vegetation_t vegetation) {
    intptr_t idx = (intptr_t)vegetation;
    if (idx < 0 || idx >= vegetation_count) return 0;
    return vegetations[idx].tree_count;
}

int sprite_vegetation_get_tree_position(sprite_vegetation_t vegetation, int index, int *x, int *y) {
    intptr_t idx = (intptr_t)vegetation;
    if (idx < 0 || idx >= vegetation_count) return 0;
    vegetation_impl *veg = &vegetations[idx];
    if (index < 0 || index >= veg->tree_count) return 0;
    *x = veg->tree_x[index];
    *y = veg->tree_y[index];
    return 1;
}

/* Phase 60: Population */
sprite_population_t sprite_population_create(int max_npcs) {
    if (population_count >= MAX_POPULATIONS) return INVALID_POPULATION;
    population_impl *pop = &populations[population_count];
    pop->max_npcs = max_npcs;
    pop->npc_count = 0;
    pop->npc_x = (int *)malloc(max_npcs * sizeof(int));
    pop->npc_y = (int *)malloc(max_npcs * sizeof(int));
    pop->npc_type = (int *)malloc(max_npcs * sizeof(int));
    return (sprite_population_t)(intptr_t)population_count++;
}

void sprite_population_destroy(sprite_population_t population) {
    intptr_t idx = (intptr_t)population;
    if (idx < 0 || idx >= population_count) return;
    free(populations[idx].npc_x);
    free(populations[idx].npc_y);
    free(populations[idx].npc_type);
}

int sprite_population_spawn_npc(sprite_population_t population, int x, int y) {
    intptr_t idx = (intptr_t)population;
    if (idx < 0 || idx >= population_count) return 0;
    population_impl *pop = &populations[idx];
    if (pop->npc_count >= pop->max_npcs) return 0;
    pop->npc_x[pop->npc_count] = x;
    pop->npc_y[pop->npc_count] = y;
    pop->npc_type[pop->npc_count] = 0;
    return pop->npc_count++;
}

int sprite_population_update(sprite_population_t population, int delta_ms) {
    intptr_t idx = (intptr_t)population;
    if (idx < 0 || idx >= population_count) return 0;
    return populations[idx].npc_count;
}

int sprite_population_get_npc_count(sprite_population_t population) {
    intptr_t idx = (intptr_t)population;
    if (idx < 0 || idx >= population_count) return 0;
    return populations[idx].npc_count;
}
