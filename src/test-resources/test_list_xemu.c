/* test_list_xemu.c — <list.h> (#406): intrusive lists against an array model
 *
 * Results at $C0C0 (01 = pass):
 *   [0] dlist: 2000 random push_front / push_back / insert_after / remove /
 *       pop_front / pop_back on 40 nodes against an array of ids, the links
 *       walked both ways after each
 *   [1] container_of, dl_foreach, dl_foreach_safe removing as it goes
 *   [2] dl_splice (into empty, from empty, both full)
 *   [3] slist: push / pop / remove / reverse against a model; a free list over a pool
 *   [4] $AA sentinel
 */
#include <list.h>

#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned int seed = 31337;
static unsigned int rnd(void) { seed = seed * 25173u + 13849u; return seed >> 3; }

struct item { unsigned char id; dlink link; int pad; slink s; };
static struct item items[40];
static unsigned char inlist[40];
static unsigned char order[40];
static unsigned int count;

/* the list holds the ids of order[], in order, both ways */
static int matches(dlist *l) {
    dlink *it;
    unsigned int k = 0;
    dl_foreach(it, l) {
        if (k >= count || container_of(it, struct item, link)->id != order[k]) return 0;
        k++;
    }
    if (k != count) return 0;
    for (it = l->head.prev; it != &l->head; it = it->prev) {
        if (k == 0 || container_of(it, struct item, link)->id != order[--k]) return 0;
    }
    return k == 0 && (count == 0) == dl_empty(l);
}

static void model_insert(unsigned int at, unsigned char id) {
    unsigned int k;
    for (k = count; k > at; k--) order[k] = order[k - 1];
    order[at] = id;
    count++;
    inlist[id] = 1;
}

static void model_remove_at(unsigned int at) {
    unsigned int k;
    inlist[order[at]] = 0;
    for (k = at; k + 1 < count; k++) order[k] = order[k + 1];
    count--;
}

static unsigned int pos_of(unsigned char id) {
    unsigned int k;
    for (k = 0; k < count; k++) if (order[k] == id) return k;
    return 0xFFFF;
}

static int t_dlist(void) {
    dlist l;
    unsigned int n, i;
    int ok = 1;
    for (i = 0; i < 40; i++) { items[i].id = (unsigned char)i; inlist[i] = 0; }
    dl_init(&l);
    count = 0;
    if (!dl_empty(&l) || dl_pop_front(&l) != NULL || dl_pop_back(&l) != NULL || dl_first(&l) != NULL) ok = 0;
    for (n = 0; n < 2000 && ok; n++) {
        unsigned char id = (unsigned char)(rnd() % 40);
        unsigned int op = rnd() % 6;
        if (op < 3 && !inlist[id]) {
            if (op == 0) { dl_push_front(&l, &items[id].link); model_insert(0, id); }
            else if (op == 1) { dl_push_back(&l, &items[id].link); model_insert(count, id); }
            else if (count) {
                unsigned int at = rnd() % count;
                dl_insert_after(&items[order[at]].link, &items[id].link);
                model_insert(at + 1, id);
            }
        } else if (op == 3 && inlist[id]) {
            dl_remove(&items[id].link);
            model_remove_at(pos_of(id));
        } else if (op == 4) {
            dlink *p = dl_pop_front(&l);
            if (count == 0) { if (p) ok = 0; }
            else { if (!p || container_of(p, struct item, link)->id != order[0]) ok = 0; model_remove_at(0); }
        } else if (op == 5) {
            dlink *p = dl_pop_back(&l);
            if (count == 0) { if (p) ok = 0; }
            else { if (!p || container_of(p, struct item, link)->id != order[count - 1]) ok = 0; model_remove_at(count - 1); }
        }
        if (!matches(&l)) ok = 0;
    }
    return ok;
}

static int t_iterate(void) {
    dlist l;
    dlink *it, *tmp;
    unsigned int i, sum = 0, left = 0;
    dl_init(&l);
    for (i = 0; i < 10; i++) dl_push_back(&l, &items[i].link);
    dl_foreach(it, &l) sum += container_of(it, struct item, link)->id;
    dl_foreach_safe(it, tmp, &l) {
        if (container_of(it, struct item, link)->id & 1) dl_remove(it);
    }
    dl_foreach(it, &l) { left++; if (container_of(it, struct item, link)->id & 1) return 0; }
    return sum == 45 && left == 5 && dl_first(&l) == &items[0].link && dl_last(&l) == &items[8].link;
}

static int t_splice(void) {
    dlist a, b;
    dlink *it;
    unsigned int i, k = 0;
    int ok = 1;
    dl_init(&a); dl_init(&b);
    dl_splice(&a, &b);                      /* empty into empty */
    if (!dl_empty(&a) || !dl_empty(&b)) ok = 0;
    for (i = 0; i < 3; i++) dl_push_back(&b, &items[i].link);
    dl_splice(&a, &b);                      /* into empty */
    if (!dl_empty(&b)) ok = 0;
    for (i = 3; i < 6; i++) dl_push_back(&b, &items[i].link);
    dl_splice(&a, &b);                      /* both full */
    dl_splice(&a, &b);                      /* from empty */
    dl_foreach(it, &a) { if (container_of(it, struct item, link)->id != k) ok = 0; k++; }
    for (it = a.head.prev; it != &a.head; it = it->prev) k--;
    return ok && k == 0 && dl_empty(&b);
}

static int t_slist(void) {
    slist l, pool;
    slink *it;
    unsigned char model[40];
    unsigned int m = 0, n, i, k;
    int ok = 1;
    sl_init(&l);
    if (!sl_empty(&l) || sl_pop(&l) != NULL) ok = 0;
    for (i = 0; i < 40; i++) inlist[i] = 0;
    for (n = 0; n < 1500 && ok; n++) {
        unsigned char id = (unsigned char)(rnd() % 40);
        unsigned int op = rnd() % 4;
        if (op == 0 && !inlist[id]) {
            sl_push(&l, &items[id].s);
            for (k = m; k > 0; k--) model[k] = model[k - 1];
            model[0] = id; m++; inlist[id] = 1;
        } else if (op == 1) {
            slink *p = sl_pop(&l);
            if (m == 0) { if (p) ok = 0; }
            else {
                if (!p || container_of(p, struct item, s)->id != model[0]) ok = 0;
                inlist[model[0]] = 0;
                for (k = 0; k + 1 < m; k++) model[k] = model[k + 1];
                m--;
            }
        } else if (op == 2) {
            int got = sl_remove(&l, &items[id].s);
            if (got != inlist[id]) ok = 0;
            if (inlist[id]) {
                for (k = 0; model[k] != id; k++) ;
                for (; k + 1 < m; k++) model[k] = model[k + 1];
                m--; inlist[id] = 0;
            }
        } else if (op == 3) {
            sl_reverse(&l);
            for (k = 0; k < m / 2; k++) { unsigned char t = model[k]; model[k] = model[m - 1 - k]; model[m - 1 - k] = t; }
        }
        k = 0;
        sl_foreach(it, &l) { if (k >= m || container_of(it, struct item, s)->id != model[k]) ok = 0; k++; }
        if (k != m) ok = 0;
    }
    /* a free list over a pool: every node out, back, out again */
    sl_init(&pool);
    for (i = 0; i < 40; i++) sl_push(&pool, &items[i].s);
    for (i = 0; i < 40; i++) if (sl_pop(&pool) == NULL) ok = 0;
    if (sl_pop(&pool) != NULL || !sl_empty(&pool)) ok = 0;
    return ok;
}

int main(void) {
    OUT[0] = t_dlist();
    OUT[1] = t_iterate();
    OUT[2] = t_splice();
    OUT[3] = t_slist();
    OUT[4] = 0xAA;
    return 0;
}
