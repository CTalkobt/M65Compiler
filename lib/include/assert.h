/* assert.h — MEGA65 C compiler runtime assertion support
 *
 * Provides the assert(expr) macro for runtime expression validation.
 * When NDEBUG is defined, assertions are disabled (no-op).
 * When enabled, a failed assertion prints an error message and halts execution.
 */

#ifndef ASSERT_H
#define ASSERT_H

#ifdef NDEBUG
  /* NDEBUG defined: assertions disabled */
  #define assert(expr) ((void)0)
#else
  /* Assertions enabled: call _assert_fail on failure */
  #define assert(expr) \
      ((expr) ? (void)0 : _assert_fail(#expr, __FILE__, __LINE__))

  /* Runtime function: prints assertion error and halts */
  void _assert_fail(const char *expr, const char *file, int line);
#endif

#endif
