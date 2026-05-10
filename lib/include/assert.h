/* assert.h — MEGA65 C compiler runtime assertion support
 *
 * Provides assertion macros for runtime expression validation:
 *   assert(expr) — standard C assertion with condition text
 *   assert_msg(expr, format, ...) — assertion with custom diagnostic message
 *
 * When NDEBUG is defined, all assertions are disabled (no-op).
 * When enabled, failed assertions print an error message and halt execution.
 *
 * Note: assert_msg requires <stdio.h> to be included by the user.
 */

#ifndef ASSERT_H
#define ASSERT_H

#ifdef NDEBUG
  /* NDEBUG defined: all assertions disabled */
  #define assert(expr) ((void)0)
  #define assert_msg(expr, msg) ((void)0)
#else
  /* Assertions enabled: call assert_fail on failure */
  #define assert(expr) \
      ((expr) ? (void)0 : assert_fail(#expr, __FILE__, __LINE__))

  /* assert_msg: assertion with custom diagnostic message
   * Note: For custom formatting, use printf before assert.
   * Usage: assert_msg(expr, "message") */
  #define assert_msg(expr, msg) \
      ((expr) ? (void)0 : assert_msg_fail(#expr, __FILE__, __LINE__, msg))

  /* Runtime functions */
  void assert_fail(const char *expr, const char *file, int line);
  void assert_fail_abort(void);  /* Halts execution with BRK */
  void assert_msg_fail(const char *expr, const char *file, int line, const char *msg);  /* assert_msg handler */
#endif

#endif
