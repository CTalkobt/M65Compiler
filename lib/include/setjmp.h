/* setjmp.h — Non-local jumps for cc45 / MEGA65
 *
 * jmp_buf layout (6 bytes):
 *   [0-1]  return address (PC)
 *   [2-3]  stack pointer (SPL/SPH) inside setjmp (return address on top)
 *   [4-5]  frame pointer ($FD/$FE)
 *
 * setjmp(env) saves state, returns 0.
 * longjmp(env, val) restores state, causes setjmp to return val.
 * If val is 0, setjmp returns 1 instead (per C standard).
 */

#pragma once

typedef char jmp_buf[6];

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);
