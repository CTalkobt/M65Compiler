/* sprite_event_system.h — Phase 63: Event System
 *
 * Publish-subscribe event dispatch system.
 */

#ifndef EVENT_SYSTEM_H
#define EVENT_SYSTEM_H

typedef void *sprite_event_system_t;
typedef void (*sprite_event_callback_t)(int event_id, void *data, void *user_data);

#define INVALID_EVENT_SYSTEM NULL

typedef struct {
    int id;
    void *data;
    int data_size;
    int timestamp;
} sprite_event_t;

/* Event System */
sprite_event_system_t sprite_event_system_create(int max_subscribers);
void sprite_event_system_destroy(sprite_event_system_t system);

/* Subscription */
int sprite_event_subscribe(sprite_event_system_t system,
                           int event_id,
                           sprite_event_callback_t callback,
                           void *user_data);
int sprite_event_unsubscribe(sprite_event_system_t system,
                              int event_id,
                              sprite_event_callback_t callback);

/* Publishing */
int sprite_event_publish(sprite_event_system_t system,
                         int event_id,
                         void *data,
                         int data_size);
int sprite_event_publish_deferred(sprite_event_system_t system,
                                   int event_id,
                                   void *data,
                                   int data_size);

/* Processing */
int sprite_event_system_process(sprite_event_system_t system);
int sprite_event_system_process_immediate(sprite_event_system_t system, int event_id);

/* Querying */
int sprite_event_system_get_subscriber_count(sprite_event_system_t system, int event_id);
int sprite_event_system_get_queue_size(sprite_event_system_t system);
int sprite_event_system_clear_queue(sprite_event_system_t system);

#endif
