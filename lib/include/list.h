/* list.h — intrusive linked lists (#406): the links are members of the
 * program's own structs, so nothing is allocated and nothing copied.
 *
 *   struct sprite { int x, y; dlink link; };
 *   dlist live;
 *   dl_init(&live);
 *   dl_push_back(&live, &s->link);
 *   dl_foreach(it, &live) {
 *       struct sprite *s = container_of(it, struct sprite, link);
 *       ...
 *   }
 *   dl_remove(&s->link);                     no list needed
 *
 * dlist: circular with a sentinel head (an empty list's head points at
 * itself), so insert and remove are four stores with no NULL tests. slist:
 * singly linked, NULL-terminated: a stack, or a free list over a pool.
 * Assembly (stdlib_asm/list.s45), a section per function.
 */
#ifndef _LIST_H
#define _LIST_H

#include <stddef.h>

typedef struct dlink { struct dlink *next, *prev; } dlink;
typedef struct dlist { dlink head; } dlist;
typedef struct slink { struct slink *next; } slink;
typedef struct slist { slink *first; } slist;

#ifndef container_of
#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

__fastcall__ void dl_init(dlist *l);
__fastcall__ void dl_push_front(dlist *l, dlink *n);
__fastcall__ void dl_push_back(dlist *l, dlink *n);
__fastcall__ void dl_insert_after(dlink *pos, dlink *n);
__fastcall__ void dl_remove(dlink *n);
/* the first / last node, out of the list; NULL when empty */
__fastcall__ dlink *dl_pop_front(dlist *l);
__fastcall__ dlink *dl_pop_back(dlist *l);
/* all of src's nodes after dst's last; src empty */
__fastcall__ void dl_splice(dlist *dst, dlist *src);
#define dl_empty(l) ((l)->head.next == &(l)->head)
#define dl_first(l) (dl_empty(l) ? (dlink *)0 : (l)->head.next)
#define dl_last(l)  (dl_empty(l) ? (dlink *)0 : (l)->head.prev)
/* every node; _safe: the current one may be removed (tmp holds the next) */
#define dl_foreach(it, l) for ((it) = (l)->head.next; (it) != &(l)->head; (it) = (it)->next)
#define dl_foreach_safe(it, tmp, l) \
    for ((it) = (l)->head.next, (tmp) = (it)->next; (it) != &(l)->head; (it) = (tmp), (tmp) = (it)->next)

#define sl_init(l)  ((l)->first = (slink *)0)
#define sl_empty(l) ((l)->first == (slink *)0)
__fastcall__ void sl_push(slist *l, slink *n);
/* the first node, out; NULL when empty */
__fastcall__ slink *sl_pop(slist *l);
/* n out of the list (a walk): 1, or 0 when it is not in it */
__fastcall__ int sl_remove(slist *l, slink *n);
__fastcall__ void sl_reverse(slist *l);
#define sl_foreach(it, l) for ((it) = (l)->first; (it); (it) = (it)->next)

#endif
