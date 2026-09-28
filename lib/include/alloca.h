/*
 * alloca.h — Stack-based dynamic allocation
 *
 * NOTE: The alloca() function is currently incompatible with cc45's
 * software parameter stack. It directly manipulates the hardware SP,
 * which conflicts with how cc45 passes arguments. Using alloca() will
 * cause a BRK/crash at runtime.
 *
 * WORKAROUND: Use local arrays instead, which the compiler manages
 * through its own stack frame mechanism:
 *
 *   // Instead of:
 *   char *buf = (char *)alloca(n);
 *
 *   // Use:
 *   char buf[16];  // fixed-size local array (compile-time constant)
 *
 * A future version will implement alloca as a compiler intrinsic that
 * works with the cc45 parameter stack.
 */

#ifndef _ALLOCA_H
#define _ALLOCA_H

/* Declared but currently broken — see note above */
void *alloca(unsigned int size);
void *__builtin_alloca(unsigned int size);

#endif /* _ALLOCA_H */
