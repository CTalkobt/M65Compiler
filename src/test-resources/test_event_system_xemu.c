/* test_event_system_xemu.c — <event_system.h> (#645)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] create (0 subscribers: NULL), subscribe (the id's count, full: 0),
 *       get_subscriber_count
 *   [1] publish: each subscriber of the id in order, with its data and user
 *       data; another id's not called; the number called
 *   [2] unsubscribe (another id's, a callback not there: 0), a callback
 *       unsubscribing itself while it is called
 *   [3] publish_deferred: nothing until process, in order, the queue's
 *       length; an event queued by a callback waits for the next process
 *   [4] process_immediate: only the id's queued events (the rest stay, in
 *       order); clear_queue: the number dropped
 *   [5] destroy (with an event queued), NULL system calls: 0
 *   [6] $AA sentinel
 */
#include <event_system.h>
#include <string.h>

#define OUT ((volatile unsigned char *)0xC0C0)

static char log[32];
static unsigned char nlog;
static sprite_event_system_t sys;

static void note(char c) {
    if (nlog < sizeof log - 1) log[nlog++] = c;
    log[nlog] = 0;
}

static void cb_a(int id, void *data, void *user) {
    (void)id;
    note(*(char *)user);
    if (data) note(*(char *)data);
}

static void cb_b(int id, void *data, void *user) {
    (void)data;
    (void)user;
    note('0' + (char)id);
}

static void cb_self(int id, void *data, void *user) {
    (void)data;
    (void)user;
    note('s');
    sprite_event_unsubscribe(sys, id, cb_self);
}

static void cb_queue(int id, void *data, void *user) {
    (void)data;
    (void)user;
    note('q');
    sprite_event_publish_deferred(sys, id + 1, 0, 0);
}

int main(void) {
    static char ux = 'x', uy = 'y', d = 'D';
    int ok;
    OUT[6] = 0;

    ok = sprite_event_system_create(0) == INVALID_EVENT_SYSTEM;
    sys = sprite_event_system_create(4);
    ok = ok && sys != INVALID_EVENT_SYSTEM;
    ok = ok && sprite_event_subscribe(sys, 1, cb_a, &ux) == 1 && sprite_event_subscribe(sys, 1, cb_a, &uy) == 2;
    ok = ok && sprite_event_subscribe(sys, 2, cb_b, 0) == 1 && sprite_event_subscribe(sys, 1, 0, 0) == 0;
    ok = ok && sprite_event_subscribe(sys, 3, cb_b, 0) == 1 && sprite_event_subscribe(sys, 4, cb_b, 0) == 0;
    ok = ok && sprite_event_system_get_subscriber_count(sys, 1) == 2 &&
         sprite_event_system_get_subscriber_count(sys, 9) == 0;
    OUT[0] = ok;

    nlog = 0;
    ok = sprite_event_publish(sys, 1, &d, 1) == 2 && strcmp(log, "xDyD") == 0;
    nlog = 0;
    ok = ok && sprite_event_publish(sys, 2, 0, 0) == 1 && strcmp(log, "2") == 0;
    ok = ok && sprite_event_publish(sys, 7, 0, 0) == 0;
    OUT[1] = ok;

    ok = sprite_event_unsubscribe(sys, 2, cb_a) == 0 && sprite_event_unsubscribe(sys, 3, cb_b) == 1 &&
         sprite_event_unsubscribe(sys, 3, cb_b) == 0 && sprite_event_system_get_subscriber_count(sys, 3) == 0;
    ok = ok && sprite_event_subscribe(sys, 5, cb_self, 0) == 1;
    nlog = 0;
    ok = ok && sprite_event_publish(sys, 5, 0, 0) == 1 && sprite_event_publish(sys, 5, 0, 0) == 0 &&
         strcmp(log, "s") == 0 && sprite_event_system_get_subscriber_count(sys, 5) == 0;
    OUT[2] = ok;

    nlog = 0;
    ok = sprite_event_publish_deferred(sys, 2, 0, 0) == 1 && sprite_event_publish_deferred(sys, 1, &d, 1) == 2;
    ok = ok && nlog == 0 && sprite_event_system_get_queue_size(sys) == 2;
    ok = ok && sprite_event_system_process(sys) == 2 && strcmp(log, "2xDyD") == 0 &&
         sprite_event_system_get_queue_size(sys) == 0;
    /* a callback queueing another event: delivered by the next process */
    sprite_event_unsubscribe(sys, 2, cb_b);
    ok = ok && sprite_event_subscribe(sys, 6, cb_queue, 0) == 1 && sprite_event_subscribe(sys, 7, cb_b, 0) == 1;
    nlog = 0;
    sprite_event_publish_deferred(sys, 6, 0, 0);
    ok = ok && sprite_event_system_process(sys) == 1 && strcmp(log, "q") == 0 &&
         sprite_event_system_get_queue_size(sys) == 1;
    ok = ok && sprite_event_system_process(sys) == 1 && strcmp(log, "q7") == 0;
    OUT[3] = ok;

    nlog = 0;
    sprite_event_publish_deferred(sys, 7, 0, 0);
    sprite_event_publish_deferred(sys, 1, &d, 1);
    sprite_event_publish_deferred(sys, 7, 0, 0);
    sprite_event_publish_deferred(sys, 9, 0, 0);
    ok = sprite_event_system_process_immediate(sys, 7) == 2 && strcmp(log, "77") == 0 &&
         sprite_event_system_get_queue_size(sys) == 2;
    nlog = 0;
    ok = ok && sprite_event_system_clear_queue(sys) == 2 && sprite_event_system_get_queue_size(sys) == 0 &&
         sprite_event_system_process(sys) == 0 && nlog == 0;
    OUT[4] = ok;

    sprite_event_publish_deferred(sys, 9, &d, 5);
    ok = sprite_event_system_get_queue_size(sys) == 1;
    sprite_event_system_destroy(sys);
    ok = ok && sprite_event_publish(0, 1, 0, 0) == 0 && sprite_event_subscribe(0, 1, cb_a, 0) == 0 &&
         sprite_event_system_process(0) == 0 && sprite_event_system_get_queue_size(0) == 0;
    sprite_event_system_destroy(0);
    OUT[5] = ok;
    OUT[6] = 0xAA;
    return 0;
}
