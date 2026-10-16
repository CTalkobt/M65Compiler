/* setjmp.h — Non-local jumps for cc45 / MEGA65
 *
 * jmp_buf layout (8 bytes):
 *   [0-1]  return address (PC)
 *   [2-3]  stack pointer (SPL/SPH) inside setjmp (return address on top)
 *   [4-5]  frame pointer ($FD/$FE)
 *   [6]    code overlays: the depth of their return stack
 *   [7]    code overlays: the overlay in the region (longjmp brings it back)
 *
 * setjmp(env) saves state, returns 0.
 * longjmp(env, val) restores state, causes setjmp to return val.
 * If val is 0, setjmp returns 1 instead (per C standard).
 */

#pragma once

typedef char jmp_buf[8];

__fastcall__ int setjmp(jmp_buf env);
__fastcall__ void longjmp(jmp_buf env, int val);
