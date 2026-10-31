/* test_pp_if_ops.c — the #if operators (#375): each #if below is false (or
 * its #if ! is true) in standard C; a wrong one stops with its #error */
#define DRAW 150
#if DRAW & 1
#error "150 & 1 taken as true"
#endif
#if 5 & 2 || 4 ^ 4 || 7 % 7 || 150 % 2
#error "& ^ % evaluated wrongly"
#endif
#if !((1 << 3) == 8 && (64 >> 2) == 16 && -16 >> 2 == -4)
#error "<< >> evaluated wrongly"
#endif
#if !((1 | 2) == 3 && (6 & 3 | 8) == 10 && (1 ^ 3 & 1) == 0 && ~0 == -1)
#error "| ~ or their precedence evaluated wrongly"
#endif
#if (2 | 1 == 1) != 3
#error "== binds tighter than |"
#endif
#if !(1 ? 2 : 0) || (0 ? 1 : 0) || (0 ? 1 : 2) != 2 || (1 ? 0 ? 5 : 6 : 7) != 6
#error "?: evaluated wrongly"
#endif
#if 0 && 1 / 0 || 1 || 1 % 0
#else
#error "&& || ?: evaluate only the side that decides"
#endif
#if 0 ? 1 / 0 : 0
#error "?: evaluated wrongly"
#endif
#if !(-1 > 0u && -1 < 0 && 0xFFFFFFFFFFFFFFFF > 0 && 1UL + 1 == 2)
#error "unsigned arithmetic evaluated wrongly"
#endif
#if !('A' == 65 && '\n' == 10 && '\x41' == 65 && '\101' == 65 && 0b101 == 5 && 010 == 8)
#error "constants evaluated wrongly"
#endif
#if -7 / 2 != -3 || -7 % 2 != -1 || 3 + 4 * 2 != 11 || 10 - 2 - 3 != 5
#error "arithmetic evaluated wrongly"
#endif
#if 0
#if 1 +
#endif
#elif 0 && 1 / 0
#elif 1
#elif 1 / 0
#endif
int x = 1;
