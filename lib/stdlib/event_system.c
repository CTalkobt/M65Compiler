/* event_system.c — publish / subscribe events (<event_system.h>, #645)
 *
 * A system: its subscriptions (an array of max_subscribers entries, in the
 * order made) and its queue of deferred events (grown with realloc, at most
 * 255). A subscription removed while events are being delivered is marked
 * (its callback NULL) and the array closed up when the outermost delivery
 * ends, so a callback may unsubscribe itself or another.
 */

#include <event_system.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define QUEUE_MAX 255

typedef struct {
    int id;
    sprite_event_callback_t callback;
    void *user_data;
} es_sub;

typedef struct {
    es_sub *subs;
    unsigned int nsubs, maxsubs;
    sprite_event_t *queue;
    unsigned int nqueue, capqueue;
    unsigned char depth;        /* deliveries going on (callbacks publishing) */
    unsigned char dirty;        /* a subscription marked removed meanwhile */
} es_system;

sprite_event_system_t sprite_event_system_create(int max_subscribers) {
    es_system *s;
    if (max_subscribers == 0) return INVALID_EVENT_SYSTEM;
    s = (es_system *)calloc(1, sizeof(es_system));
    if (!s) return INVALID_EVENT_SYSTEM;
    s->subs = (es_sub *)malloc((size_t)max_subscribers * sizeof(es_sub));
    if (!s->subs) {
        free(s);
        return INVALID_EVENT_SYSTEM;
    }
    s->maxsubs = (unsigned int)max_subscribers;
    return s;
}

void sprite_event_system_destroy(sprite_event_system_t system) {
    es_system *s = (es_system *)system;
    if (!s) return;
    free(s->subs);
    free(s->queue);
    free(s);
}

/* the marked subscriptions out (when no delivery is going on) */
static void es_compact(es_system *s) {
    unsigned int i, j = 0;
    if (s->depth || !s->dirty) return;
    for (i = 0; i < s->nsubs; i++)
        if (s->subs[i].callback) s->subs[j++] = s->subs[i];
    s->nsubs = j;
    s->dirty = 0;
}

int sprite_event_system_get_subscriber_count(sprite_event_system_t system, int event_id) {
    es_system *s = (es_system *)system;
    unsigned int i;
    int n = 0;
    if (!s) return 0;
    for (i = 0; i < s->nsubs; i++)
        if (s->subs[i].callback && s->subs[i].id == event_id) n++;
    return n;
}

int sprite_event_subscribe(sprite_event_system_t system, int event_id,
                           sprite_event_callback_t callback, void *user_data) {
    es_system *s = (es_system *)system;
    es_sub *e;
    if (!s || !callback) return 0;
    es_compact(s);
    if (s->nsubs >= s->maxsubs) return 0;
    e = &s->subs[s->nsubs++];
    e->id = event_id;
    e->callback = callback;
    e->user_data = user_data;
    return sprite_event_system_get_subscriber_count(system, event_id);
}

int sprite_event_unsubscribe(sprite_event_system_t system, int event_id,
                             sprite_event_callback_t callback) {
    es_system *s = (es_system *)system;
    unsigned int i;
    if (!s) return 0;
    for (i = 0; i < s->nsubs; i++) {
        if (s->subs[i].callback == callback && callback && s->subs[i].id == event_id) {
            s->subs[i].callback = NULL;
            s->dirty = 1;
            es_compact(s);
            return 1;
        }
    }
    return 0;
}

/* each subscriber of the id (those there when it starts) */
static int es_deliver(es_system *s, int event_id, void *data) {
    unsigned int i, n = s->nsubs;
    int called = 0;
    s->depth++;
    for (i = 0; i < n; i++) {
        es_sub *e = &s->subs[i];
        if (e->callback && e->id == event_id) {
            e->callback(event_id, data, e->user_data);
            called++;
        }
    }
    s->depth--;
    es_compact(s);
    return called;
}

int sprite_event_publish(sprite_event_system_t system, int event_id, void *data, int data_size) {
    (void)data_size;
    if (!system) return 0;
    return es_deliver((es_system *)system, event_id, data);
}

int sprite_event_publish_deferred(sprite_event_system_t system, int event_id, void *data, int data_size) {
    es_system *s = (es_system *)system;
    sprite_event_t *ev;
    if (!s || s->nqueue >= QUEUE_MAX) return 0;
    if (s->nqueue == s->capqueue) {
        unsigned int cap = s->capqueue ? s->capqueue * 2 : 8;
        sprite_event_t *q;
        if (cap > QUEUE_MAX) cap = QUEUE_MAX;
        q = (sprite_event_t *)realloc(s->queue, cap * sizeof(sprite_event_t));
        if (!q) return 0;
        s->queue = q;
        s->capqueue = cap;
    }
    ev = &s->queue[s->nqueue++];
    ev->id = event_id;
    ev->data = data;
    ev->data_size = data_size;
    ev->timestamp = (int)clock();
    return (int)s->nqueue;
}

/* queued event k off the queue, then delivered */
static void es_take(es_system *s, unsigned int k) {
    sprite_event_t ev = s->queue[k];
    s->nqueue--;
    memmove(&s->queue[k], &s->queue[k + 1], (s->nqueue - k) * sizeof(sprite_event_t));
    es_deliver(s, ev.id, ev.data);
}

int sprite_event_system_process(sprite_event_system_t system) {
    es_system *s = (es_system *)system;
    unsigned int n;
    int done = 0;
    if (!s) return 0;
    /* (the events queued now; those the callbacks queue wait) */
    for (n = s->nqueue; n && s->nqueue; n--) {
        es_take(s, 0);
        done++;
    }
    return done;
}

int sprite_event_system_process_immediate(sprite_event_system_t system, int event_id) {
    es_system *s = (es_system *)system;
    unsigned int k = 0, n;
    int done = 0;
    if (!s) return 0;
    for (n = s->nqueue; n && k < s->nqueue; n--) {
        if (s->queue[k].id == event_id) {
            es_take(s, k);
            done++;
        } else {
            k++;
        }
    }
    return done;
}

int sprite_event_system_get_queue_size(sprite_event_system_t system) {
    es_system *s = (es_system *)system;
    return s ? (int)s->nqueue : 0;
}

int sprite_event_system_clear_queue(sprite_event_system_t system) {
    es_system *s = (es_system *)system;
    int n;
    if (!s) return 0;
    n = (int)s->nqueue;
    s->nqueue = 0;
    return n;
}
