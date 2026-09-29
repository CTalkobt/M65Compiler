/*
 * alloca.h — Stack-based dynamic allocation
 *
 * For constant sizes, the compiler emits an inline frame buffer
 * allocation (no function call, zero overhead). For variable sizes,
 * the runtime version in alloca.s45 adjusts SP at runtime.
 *
 * Usage:
 *   #include <alloca.h>
 *   char *buf = (char *)alloca(16);  // inline allocation (constant)
 *   char *dyn = (char *)alloca(n);   // runtime allocation (variable)
 *
 * Memory is automatically freed when the calling function returns.
 *
 * Conditional compilation:
 *   #ifdef __CC45_STATIC_ALLOC__
 *     // SAC mode is active — alloca uses frame buffers
 *   #endif
 */

#ifndef _ALLOCA_H
#define _ALLOCA_H

void *alloca(unsigned int size);
void *__builtin_alloca(unsigned int size);

#endif /* _ALLOCA_H */
