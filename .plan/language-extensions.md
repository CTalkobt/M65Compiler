# Language Extensions & Optimizations for cc45

Brainstormed ideas for C language extensions and optimizations targeting the
45GS02 / MEGA65's constrained memory and speed.

Status: Partially Implemented (2026-05-23)

---

## 1. Memory Placement & Layout Extensions

### `__zp` type qualifier
```c
__zp char *screen = (char *)0x0800;  // Force zero-page allocation for globals
__zp int counter;                     // 2-byte ZP global (not just locals via register)
```
Currently `register` only works on locals. A `__zp` qualifier for globals and
pointers would let the programmer explicitly place frequently-accessed data in
ZP, cutting instruction size (2 bytes vs 3) and cycle counts across the entire
program.

### `__near` / `__far` pointer qualifiers
```c
__near char *zp_ptr;    // 8-bit pointer (ZP only, 0-255)
__far  char *bank_ptr;  // 24-bit or 28-bit pointer (MEGA65 banked memory)
```
The 45GS02 has a 28-bit address space via MAP. An 8-bit `__near` pointer saves
a byte per pointer and enables `(zp),Y` addressing naturally. A `__far` pointer
could emit MAP/bank-switch sequences automatically. Pattern from other embedded
compilers (Keil's `__xdata`, IAR's `__near`/`__far`, cc65's `__fastcall__`).

### `__bank(n)` function/data attribute
```c
__bank(1) void level_loader(void);     // Placed in bank 1
__bank(2) const char tileset[] = {...}; // Data in bank 2
```
The MEGA65 has 384KB+ of RAM in banks. Explicit bank placement with
compiler-generated trampoline stubs for cross-bank calls would be transformative
for larger programs. The linker could auto-generate bank-switch thunks.

### Segment placement pragma
```c
#pragma section("SCREENRAM", 0x0800)
__section("SCREENRAM") char screen[1000];
```
More fine-grained than `.data`/`.bss`/`.code` -- lets the programmer target
specific memory regions (color RAM, screen RAM, I/O shadow areas) without
resorting to inline assembly.

---

## 2. Control Flow Extensions

### Computed / range `case` (GCC extension) -- IMPLEMENTED (v1.0-rc3)
```c
switch (ch) {
    case 'A' ... 'Z':   // Range check: two CMP + branch
        handle_letter();
        break;
    case 0x80 ... 0xFF:
        handle_high();
        break;
}
```
On 6502, compiles to two `CMP` + branch instructions instead of a long jump
table or chain of individual cases. Massively reduces code size for character
classification and similar patterns.

### Multi-level `break`
```c
for (y = 0; y < 25; y++) {
    for (x = 0; x < 40; x++) {
        if (screen[y*40+x] == target)
            break 2;  // Break out of both loops
    }
}
```
Avoids the common `goto found;` pattern or flag variables that waste both memory
and cycles. Eliminating a flag variable and its associated branch is meaningful
when every byte counts.

### `repeat(n)` loop
```c
repeat(8) {
    *dst++ = *src++;  // Unrolled 8 times by the compiler
}
```
Explicit compile-time unrolling. On 6502, loop overhead (DEX/BNE) is 5 cycles
per iteration -- for small, fixed-count loops, full unrolling can be 2-3x
faster. The compiler knows the count is constant, so it can make optimal
decisions about register allocation across iterations.

---

## 3. Type & Data Extensions

### Fixed-point types
```c
__fixed8_8 fx_pos = 1.5;    // 8.8 fixed point (1 byte int, 1 byte fraction)
__fixed16_16 fx_vel = 0.25; // 16.16 fixed point
fx_pos += fx_vel;            // Compiles to ADC/ADC sequence
```
No FPU means floating point is extremely expensive. Fixed-point is the standard
approach for games and signal processing on these machines. Compiler-supported
fixed-point arithmetic would generate tight ADC/SBC sequences and proper
shift-based multiplication.

### `__bit` type / bit-addressable variables
```c
__bit flag_visible;
__bit flag_dirty;
// Packed into a single byte automatically; uses BBS/BBR on 45GS02
if (flag_visible) { ... }  // Compiles to BBR/BBS (bit test & branch)
```
The 45GS02 inherits the 65C02's bit test and branch instructions. A `__bit`
type could pack 8 booleans into one byte and generate BBS/BBR/SMB/RMB
instructions directly -- saving 7 bytes of storage per 8 flags and generating
faster test code than `LDA; AND #mask; BEQ`.

### String literal encoding pragma
```c
#pragma encoding(ascii)      // Keep ASCII
#pragma encoding(petscii)    // Convert to PETSCII (current default)
#pragma encoding(screencode) // MEGA65 screen codes
const char title[] = "HELLO"; // Encoded per current setting
```
MEGA65 programs frequently need strings in different encodings (PETSCII for
KERNAL calls, screen codes for direct screen memory writes). Currently requires
inline assembly or manual lookup tables.

---

## 4. Code Generation Extensions

### `__interrupt` function attribute
```c
__interrupt void timer_isr(void) {
    tick_count++;
}
// Emits: PHA/PHX/PHY/PHZ ... PLZ/PLY/PLX/PLA/RTI
// Also saves/restores any ZP scratch the function body uses
```
Interrupt handlers need to save/restore all registers and use RTI instead of
RTS. Currently requires inline assembly wrappers. The compiler already tracks
which registers the function body clobbers via `.reg_clobbers`, so it can emit
minimal save/restore sequences.

### `__naked` function attribute
```c
__naked void fast_memcpy(void) {
    __asm__("...");  // No prologue/epilogue emitted
}
```
For tiny hand-optimized routines where the compiler's prologue/epilogue overhead
is unacceptable. The function body is 100% inline assembly -- the compiler just
provides the label and `endproc`.

### Intrinsic functions for 45GS02 / MEGA65 features
```c
__builtin_dma_copy(src, dst, len);          // Emit MEGA65 DMA transfer
__builtin_swap(a, b);                       // 45GS02 hardware swap
unsigned char old = __builtin_irq_disable(); // SEI, return old P
__builtin_irq_restore(old);                  // Restore from saved P
```
The MEGA65 has a DMA controller that can copy/fill memory at ~40MB/s (vs ~1MB/s
for CPU loops). An intrinsic that emits the DMA job structure inline would be
enormously useful -- it's the single biggest performance feature of the hardware.

---

## 5. Optimization Hints

### `__likely` / `__unlikely` branch hints
```c
if (__unlikely(error_code)) {
    handle_error();
}
```
On 6502, taken branches cost 1 extra cycle (2 extra if page-crossing). The
compiler could arrange the common path as fall-through and place unlikely code
at the end of the function.

### `__pure` / `__const` function attributes
```c
__pure int abs(int x);  // No side effects, depends only on args
```
Enables the compiler to CSE (common subexpression eliminate) repeated calls and
hoist calls out of loops. On a machine with expensive function calls (JSR = 6
cycles, RTS = 6 cycles, plus arg setup), eliminating even one redundant call
saves significant time.

### `restrict` pointer qualifier
```c
void copy(char * restrict dst, const char * restrict src, int n);
```
Tells the compiler that `dst` and `src` don't alias. On 6502, this matters
because without it, every store through one pointer must assume it invalidates
loads through the other -- preventing register caching of values read through
`src`.

---

## 6. Data Structure Optimizations

### Packed structs
```c
__packed struct SpriteEntry {
    char x;        // offset 0
    char y;        // offset 1
    short tile;    // offset 2 (no alignment padding)
};
```
A `__packed` attribute would suppress padding -- critical for matching hardware
register layouts and binary file formats where every byte is specified.

### `const __rom` for ROM-resident data
```c
const __rom char sin_table[256] = {...};  // Placed in ROM, not copied to RAM
```
On MEGA65, ROM-resident data doesn't consume precious RAM. A qualifier could
ensure lookup tables stay in ROM and generate absolute addressing (not
stack-relative) for access.

---

## 7. Whole-Program Optimizations

### Automatic `register` promotion
The compiler already tracks variable usage. A whole-function analysis could
automatically promote the hottest locals to ZP without programmer annotation.
Criteria: accessed inside loops, no address-of taken, fits in ZP budget.

### Tail-call optimization -- IMPLEMENTED (v1.0-rc3)
```c
int factorial(int n, int acc) {
    if (n <= 1) return acc;
    return factorial(n - 1, n * acc);  // -> JMP, not JSR
}
```
Replace `JSR + RTS` with `JMP` when the last thing a function does is call
another function (or itself). Saves 2 bytes of stack per call depth and 12
cycles (JSR+RTS -> JMP). Critical for recursive algorithms on a 256-byte stack.
Implemented as an assembler optimizer pass (JSR absolute + RTS implied -> JMP).

### Interprocedural register allocation
Already emitting `.reg_clobbers` metadata (Phase 1 complete). The next step: if
the linker knows `helper()` only clobbers A, then the caller doesn't need to
save X/Y across the call. This is the Phase 2-5 roadmap -- the single
highest-impact optimization for call-heavy code.

---

## Priority Matrix

| Extension | Code Size | Speed | Impl. Complexity |
|-----------|:---------:|:-----:|:-----------------:|
| `__interrupt` | medium | medium | low |
| Case ranges (`'A'...'Z'`) | high | medium | **DONE** |
| `__zp` globals | medium | high | medium |
| Tail-call optimization | medium | high | **DONE** |
| `__packed` structs | medium | -- | low |
| `repeat(n)` unrolling | high (trades size) | high | low |
| Fixed-point types | high | high | high |
| DMA intrinsics | -- | very high | medium |
| `__near`/`__far` pointers | high | medium | high |
| Bank placement | high | medium | high |

Low-hanging fruit: `__interrupt`, case ranges, `__packed`, and tail-call
optimization -- all relatively contained changes that deliver immediate,
tangible benefits for the kinds of programs people write on this hardware.
