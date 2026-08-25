/* sprite_event_system.c — Event System Implementation */

#include "sprite_event_system.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_EVENT_SYSTEMS 4
#define MAX_EVENT_TYPES 256
#define MAX_SUBSCRIBERS_PER_EVENT 32
#define MAX_QUEUED_EVENTS 512

typedef struct {
    sprite_event_callback_t callback;
    void *user_data;
} subscriber_impl;

typedef struct {
    subscriber_impl subscribers[MAX_SUBSCRIBERS_PER_EVENT];
    int subscriber_count;
} event_type_impl;

typedef struct {
    event_type_impl event_types[MAX_EVENT_TYPES];
    sprite_event_t event_queue[MAX_QUEUED_EVENTS];
    int queue_size;
    int max_subscribers;
} event_system_impl;

static event_system_impl event_systems[MAX_EVENT_SYSTEMS];
static int event_system_count = 0;

sprite_event_system_t sprite_event_system_create(int max_subscribers) {
    if (event_system_count >= MAX_EVENT_SYSTEMS) return INVALID_EVENT_SYSTEM;

    event_system_impl *sys = &event_systems[event_system_count];
    memset(sys->event_types, 0, sizeof(sys->event_types));
    sys->queue_size = 0;
    sys->max_subscribers = max_subscribers;

    return (sprite_event_system_t)(intptr_t)event_system_count++;
}

void sprite_event_system_destroy(sprite_event_system_t system) {}

int sprite_event_subscribe(sprite_event_system_t system,
                           int event_id,
                           sprite_event_callback_t callback,
                           void *user_data) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= event_system_count || event_id < 0 || event_id >= MAX_EVENT_TYPES) return 0;

    event_system_impl *sys = &event_systems[idx];
    event_type_impl *et = &sys->event_types[event_id];

    if (et->subscriber_count >= MAX_SUBSCRIBERS_PER_EVENT) return 0;

    subscriber_impl *sub = &et->subscribers[et->subscriber_count];
    sub->callback = callback;
    sub->user_data = user_data;

    return ++et->subscriber_count;
}

int sprite_event_unsubscribe(sprite_event_system_t system,
                              int event_id,
                              sprite_event_callback_t callback) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= event_system_count || event_id < 0 || event_id >= MAX_EVENT_TYPES) return 0;

    event_system_impl *sys = &event_systems[idx];
    event_type_impl *et = &sys->event_types[event_id];

    for (int i = 0; i < et->subscriber_count; i++) {
        if (et->subscribers[i].callback == callback) {
            et->subscribers[i] = et->subscribers[et->subscriber_count - 1];
            et->subscriber_count--;
            return 1;
        }
    }

    return 0;
}

int sprite_event_publish(sprite_event_system_t system,
                         int event_id,
                         void *data,
                         int data_size) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= event_system_count || event_id < 0 || event_id >= MAX_EVENT_TYPES) return 0;

    event_system_impl *sys = &event_systems[idx];
    event_type_impl *et = &sys->event_types[event_id];

    int published = 0;
    for (int i = 0; i < et->subscriber_count; i++) {
        if (et->subscribers[i].callback) {
            et->subscribers[i].callback(event_id, data, et->subscribers[i].user_data);
            published++;
        }
    }

    return published;
}

int sprite_event_publish_deferred(sprite_event_system_t system,
                                   int event_id,
                                   void *data,
                                   int data_size) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= event_system_count || event_id < 0 || event_id >= MAX_EVENT_TYPES) return 0;

    event_system_impl *sys = &event_systems[idx];

    if (sys->queue_size >= MAX_QUEUED_EVENTS) return 0;

    sprite_event_t *evt = &sys->event_queue[sys->queue_size];
    evt->id = event_id;
    evt->data = data;
    evt->data_size = data_size;
    evt->timestamp = 0;

    return ++sys->queue_size;
}

int sprite_event_system_process(sprite_event_system_t system) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= event_system_count) return 0;

    event_system_impl *sys = &event_systems[idx];
    int processed = 0;

    for (int i = 0; i < sys->queue_size; i++) {
        sprite_event_t *evt = &sys->event_queue[i];
        sprite_event_publish(system, evt->id, evt->data, evt->data_size);
        processed++;
    }

    sys->queue_size = 0;
    return processed;
}

int sprite_event_system_process_immediate(sprite_event_system_t system, int event_id) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= event_system_count || event_id < 0 || event_id >= MAX_EVENT_TYPES) return 0;

    event_system_impl *sys = &event_systems[idx];
    event_type_impl *et = &sys->event_types[event_id];

    int processed = 0;
    for (int i = 0; i < et->subscriber_count; i++) {
        if (et->subscribers[i].callback) {
            et->subscribers[i].callback(event_id, NULL, et->subscribers[i].user_data);
            processed++;
        }
    }

    return processed;
}

int sprite_event_system_get_subscriber_count(sprite_event_system_t system, int event_id) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= event_system_count || event_id < 0 || event_id >= MAX_EVENT_TYPES) return 0;

    event_system_impl *sys = &event_systems[idx];
    return sys->event_types[event_id].subscriber_count;
}

int sprite_event_system_get_queue_size(sprite_event_system_t system) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= event_system_count) return 0;
    return event_systems[idx].queue_size;
}

int sprite_event_system_clear_queue(sprite_event_system_t system) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= event_system_count) return 0;

    event_system_impl *sys = &event_systems[idx];
    int cleared = sys->queue_size;
    sys->queue_size = 0;
    return cleared;
}
