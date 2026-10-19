#include <far_ptr.h>

/* far_attic_hook: far_malloc(.., BANK_ATTIC), far_free, far_size_long and
 * far_malloc_current reach the attic RAM allocator (far_attic.c) through
 * far_malloc.c's pointers; a member of its own, so a program that calls
 * far_attic_malloc & co. itself takes no address of them */

extern far_ptr_t (*__far_attic_alloc)(unsigned long size);
extern unsigned long (*__far_attic_size)(unsigned long at);
extern void (*__far_attic_free)(unsigned long at);
extern unsigned long (*__far_attic_avail)(void);

static unsigned long size_at(unsigned long a) {
    return far_attic_size(far_of_linear(a));
}

static void free_at(unsigned long a) {
    far_attic_free(far_of_linear(a));
}

void far_attic_hook(void) {
    __far_attic_alloc = far_attic_malloc;
    __far_attic_size = size_at;
    __far_attic_free = free_at;
    __far_attic_avail = far_attic_available;
}
