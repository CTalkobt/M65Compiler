/* stddef.h — Standard definitions for cc45 / MEGA65 */

#pragma once

/* NULL pointer constant */
#define NULL 0

typedef unsigned int size_t;
typedef signed int ptrdiff_t;

/* offsetof(type, member) — byte offset of member within type */
#define offsetof(type, member) ((size_t)&((type *)0)->member)
