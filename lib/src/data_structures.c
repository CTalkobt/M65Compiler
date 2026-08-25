/* sprite_data_structures.c — Data Structures Implementation */

#include "data_structures.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    void *data;
    int element_size;
    int count;
    int capacity;
} vector_impl;

typedef struct {
    void *data;
    int element_size;
    int capacity;
    int head, tail;
    int size;
} queue_impl;

typedef struct {
    char **keys;
    void **values;
    int *used;
    int capacity;
    int size;
} hashmap_impl;

sprite_vector_t sprite_vector_create(int element_size, int initial_capacity) {
    vector_impl *vec = (vector_impl *)malloc(sizeof(vector_impl));
    vec->data = malloc(element_size * initial_capacity);
    vec->element_size = element_size;
    vec->count = 0;
    vec->capacity = initial_capacity;
    return (sprite_vector_t)vec;
}

void sprite_vector_destroy(sprite_vector_t vector) {
    vector_impl *vec = (vector_impl *)vector;
    free(vec->data);
    free(vec);
}

int sprite_vector_push_back(sprite_vector_t vector, void *element) {
    vector_impl *vec = (vector_impl *)vector;
    if (vec->count >= vec->capacity) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, vec->element_size * vec->capacity);
    }
    memcpy((char *)vec->data + vec->count * vec->element_size, element, vec->element_size);
    return ++vec->count;
}

int sprite_vector_pop_back(sprite_vector_t vector) {
    vector_impl *vec = (vector_impl *)vector;
    if (vec->count > 0) vec->count--;
    return vec->count;
}

void *sprite_vector_at(sprite_vector_t vector, int index) {
    vector_impl *vec = (vector_impl *)vector;
    if (index < 0 || index >= vec->count) return NULL;
    return (char *)vec->data + index * vec->element_size;
}

int sprite_vector_size(sprite_vector_t vector) {
    vector_impl *vec = (vector_impl *)vector;
    return vec->count;
}

int sprite_vector_capacity(sprite_vector_t vector) {
    vector_impl *vec = (vector_impl *)vector;
    return vec->capacity;
}

int sprite_vector_clear(sprite_vector_t vector) {
    vector_impl *vec = (vector_impl *)vector;
    vec->count = 0;
    return 1;
}

sprite_queue_t sprite_queue_create(int element_size, int capacity) {
    queue_impl *q = (queue_impl *)malloc(sizeof(queue_impl));
    q->data = malloc(element_size * capacity);
    q->element_size = element_size;
    q->capacity = capacity;
    q->head = q->tail = 0;
    q->size = 0;
    return (sprite_queue_t)q;
}

void sprite_queue_destroy(sprite_queue_t queue) {
    queue_impl *q = (queue_impl *)queue;
    free(q->data);
    free(q);
}

int sprite_queue_enqueue(sprite_queue_t queue, void *element) {
    queue_impl *q = (queue_impl *)queue;
    if (q->size >= q->capacity) return 0;

    memcpy((char *)q->data + q->tail * q->element_size, element, q->element_size);
    q->tail = (q->tail + 1) % q->capacity;
    q->size++;
    return 1;
}

int sprite_queue_dequeue(sprite_queue_t queue, void *element) {
    queue_impl *q = (queue_impl *)queue;
    if (q->size == 0) return 0;

    memcpy(element, (char *)q->data + q->head * q->element_size, q->element_size);
    q->head = (q->head + 1) % q->capacity;
    q->size--;
    return 1;
}

void *sprite_queue_peek(sprite_queue_t queue) {
    queue_impl *q = (queue_impl *)queue;
    if (q->size == 0) return NULL;
    return (char *)q->data + q->head * q->element_size;
}

int sprite_queue_size(sprite_queue_t queue) {
    queue_impl *q = (queue_impl *)queue;
    return q->size;
}

int sprite_queue_is_empty(sprite_queue_t queue) {
    queue_impl *q = (queue_impl *)queue;
    return q->size == 0;
}

int sprite_queue_is_full(sprite_queue_t queue) {
    queue_impl *q = (queue_impl *)queue;
    return q->size >= q->capacity;
}

int sprite_queue_clear(sprite_queue_t queue) {
    queue_impl *q = (queue_impl *)queue;
    q->head = q->tail = q->size = 0;
    return 1;
}

sprite_hashmap_t sprite_hashmap_create(int capacity) {
    hashmap_impl *map = (hashmap_impl *)malloc(sizeof(hashmap_impl));
    map->keys = (char **)malloc(capacity * sizeof(char *));
    map->values = (void **)malloc(capacity * sizeof(void *));
    map->used = (int *)malloc(capacity * sizeof(int));
    map->capacity = capacity;
    map->size = 0;
    memset(map->used, 0, capacity * sizeof(int));
    return (sprite_hashmap_t)map;
}

void sprite_hashmap_destroy(sprite_hashmap_t map) {
    hashmap_impl *h = (hashmap_impl *)map;
    for (int i = 0; i < h->capacity; i++) {
        if (h->used[i]) free(h->keys[i]);
    }
    free(h->keys);
    free(h->values);
    free(h->used);
    free(h);
}

int sprite_string_hash(const char *str) {
    int hash = 0;
    for (int i = 0; str[i]; i++) {
        hash = hash * 31 + str[i];
    }
    return hash < 0 ? -hash : hash;
}

int sprite_hashmap_put(sprite_hashmap_t map, const char *key, void *value) {
    hashmap_impl *h = (hashmap_impl *)map;
    int hash = sprite_string_hash(key) % h->capacity;

    for (int i = 0; i < h->capacity; i++) {
        int idx = (hash + i) % h->capacity;
        if (!h->used[idx]) {
            h->keys[idx] = (char *)malloc(strlen(key) + 1);
            strcpy(h->keys[idx], key);
            h->values[idx] = value;
            h->used[idx] = 1;
            h->size++;
            return 1;
        }
        if (h->used[idx] && strcmp(h->keys[idx], key) == 0) {
            h->values[idx] = value;
            return 1;
        }
    }
    return 0;
}

void *sprite_hashmap_get(sprite_hashmap_t map, const char *key) {
    hashmap_impl *h = (hashmap_impl *)map;
    int hash = sprite_string_hash(key) % h->capacity;

    for (int i = 0; i < h->capacity; i++) {
        int idx = (hash + i) % h->capacity;
        if (h->used[idx] && strcmp(h->keys[idx], key) == 0) {
            return h->values[idx];
        }
    }
    return NULL;
}

int sprite_hashmap_remove(sprite_hashmap_t map, const char *key) {
    hashmap_impl *h = (hashmap_impl *)map;
    int hash = sprite_string_hash(key) % h->capacity;

    for (int i = 0; i < h->capacity; i++) {
        int idx = (hash + i) % h->capacity;
        if (h->used[idx] && strcmp(h->keys[idx], key) == 0) {
            free(h->keys[idx]);
            h->used[idx] = 0;
            h->size--;
            return 1;
        }
    }
    return 0;
}

int sprite_hashmap_contains(sprite_hashmap_t map, const char *key) {
    return sprite_hashmap_get(map, key) != NULL;
}

int sprite_hashmap_size(sprite_hashmap_t map) {
    hashmap_impl *h = (hashmap_impl *)map;
    return h->size;
}

int sprite_hashmap_clear(sprite_hashmap_t map) {
    hashmap_impl *h = (hashmap_impl *)map;
    for (int i = 0; i < h->capacity; i++) {
        if (h->used[i]) free(h->keys[i]);
        h->used[i] = 0;
    }
    h->size = 0;
    return 1;
}

int sprite_compare_ints(const void *a, const void *b) {
    return *(int *)a - *(int *)b;
}

int sprite_compare_floats(const void *a, const void *b) {
    float fa = *(float *)a, fb = *(float *)b;
    return (fa > fb) - (fa < fb);
}

void sprite_array_reverse(void *array, int count, int element_size) {
    char *arr = (char *)array;
    char *tmp = (char *)malloc(element_size);

    for (int i = 0; i < count / 2; i++) {
        int j = count - 1 - i;
        memcpy(tmp, arr + i * element_size, element_size);
        memcpy(arr + i * element_size, arr + j * element_size, element_size);
        memcpy(arr + j * element_size, tmp, element_size);
    }

    free(tmp);
}
