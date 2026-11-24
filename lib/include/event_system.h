/* event_system.h — publish / subscribe events (#645)
 *
 * A system holds up to max_subscribers subscriptions (an event id, a
 * callback, its user data) and a queue of deferred events; both are
 * allocated with malloc by sprite_event_system_create and freed by
 * sprite_event_system_destroy.
 *
 *   sprite_event_publish           calls each subscriber of the id at once,
 *                                  in the order they subscribed; the number
 *                                  called
 *   sprite_event_publish_deferred  queues the event (its data pointer, not a
 *                                  copy of the data); the queue's length,
 *                                  0 when it is full (255 events)
 *   sprite_event_system_process    delivers the queued events in order; an
 *                                  event a callback queues meanwhile waits
 *                                  for the next call; the number delivered
 *   sprite_event_system_process_immediate  delivers (and takes off the
 *                                  queue) only the queued events of one id
 *
 * A callback may subscribe, unsubscribe and publish. event_id is any int;
 * timestamp is clock() (frames) when the event was queued.
 * Library: stdlib/event_system.c. Test: test_event_system_xemu.c.
 */

#ifndef EVENT_SYSTEM_H
#define EVENT_SYSTEM_H

#include <stddef.h>

typedef void *sprite_event_system_t;
typedef void (*sprite_event_callback_t)(int event_id, void *data, void *user_data);

#define INVALID_EVENT_SYSTEM NULL

typedef struct {
    int id;
    void *data;
    int data_size;
    int timestamp;
} sprite_event_t;

/* Event System: NULL when out of memory or max_subscribers is 0 */
sprite_event_system_t sprite_event_system_create(int max_subscribers);
void sprite_event_system_destroy(sprite_event_system_t system);

/* Subscription: the id's number of subscribers (0: the system is full or a
 * NULL callback); unsubscribe: 1 if the callback was subscribed to the id */
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

/* Querying; clear_queue: the number of events dropped */
int sprite_event_system_get_subscriber_count(sprite_event_system_t system, int event_id);
int sprite_event_system_get_queue_size(sprite_event_system_t system);
int sprite_event_system_clear_queue(sprite_event_system_t system);

#endif
