/* sprite_data_structures.h — Phase 64: Data Structures & Utilities
 *
 * Generic containers: vectors, queues, hash maps.
 */

#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

typedef void *sprite_vector_t;
typedef void *sprite_queue_t;
typedef void *sprite_hashmap_t;

#define INVALID_VECTOR NULL
#define INVALID_QUEUE NULL
#define INVALID_HASHMAP NULL

/* Dynamic Vector */
sprite_vector_t sprite_vector_create(int element_size, int initial_capacity);
void sprite_vector_destroy(sprite_vector_t vector);
int sprite_vector_push_back(sprite_vector_t vector, void *element);
int sprite_vector_pop_back(sprite_vector_t vector);
void *sprite_vector_at(sprite_vector_t vector, int index);
int sprite_vector_size(sprite_vector_t vector);
int sprite_vector_capacity(sprite_vector_t vector);
int sprite_vector_clear(sprite_vector_t vector);

/* Queue */
sprite_queue_t sprite_queue_create(int element_size, int capacity);
void sprite_queue_destroy(sprite_queue_t queue);
int sprite_queue_enqueue(sprite_queue_t queue, void *element);
int sprite_queue_dequeue(sprite_queue_t queue, void *element);
void *sprite_queue_peek(sprite_queue_t queue);
int sprite_queue_size(sprite_queue_t queue);
int sprite_queue_is_empty(sprite_queue_t queue);
int sprite_queue_is_full(sprite_queue_t queue);
int sprite_queue_clear(sprite_queue_t queue);

/* Hash Map */
sprite_hashmap_t sprite_hashmap_create(int capacity);
void sprite_hashmap_destroy(sprite_hashmap_t map);
int sprite_hashmap_put(sprite_hashmap_t map, const char *key, void *value);
void *sprite_hashmap_get(sprite_hashmap_t map, const char *key);
int sprite_hashmap_remove(sprite_hashmap_t map, const char *key);
int sprite_hashmap_contains(sprite_hashmap_t map, const char *key);
int sprite_hashmap_size(sprite_hashmap_t map);
int sprite_hashmap_clear(sprite_hashmap_t map);

/* Utilities */
int sprite_string_hash(const char *str);
int sprite_compare_ints(const void *a, const void *b);
int sprite_compare_floats(const void *a, const void *b);
void sprite_array_reverse(void *array, int count, int element_size);

#endif
