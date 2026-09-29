/* stdbool.h — Boolean type support (C99)
 *
 * Provides standard boolean type and constants.
 * In C99, _Bool is a built-in keyword, and this header provides
 * the user-friendly names bool, true, and false.
 */

#pragma once

/* ============================================================================
 * BOOLEAN TYPE AND CONSTANTS
 * ============================================================================ */

/* Map 'bool' to the built-in _Bool type */
#define bool            _Bool

/* Standard boolean constants */
#define true            1
#define false           0

/* Indicates that bool, true, false are defined */
#define __bool_true_false_are_defined   1
