# cbm.h — Commodore KERNAL Interface for MEGA65

`<cbm.h>` provides small C wrappers for the standard Commodore KERNAL jump table, plus PETSCII control codes, device numbers and the jump-table addresses. Each wrapper is a `static` function in the header, one cc45 [`__jsr`](../bin/cc45.md#calling-an-address-__jsr-__jsr_raw-__jmp) call: `cbm_setlfs` is `__jsr(KERNAL_SETLFS, .A = lfn, .X = device, .Y = sa)`, `cbm_open` returns `__jsr(KERNAL_OPEN).Carry`. A KERNAL address goes through `__rom_calli` / `__rom_call`, which calls the routine on the ROM's base page 0 **and on the ROM's page-1 stack** (the caller's 8-bit stack, saved at startup): the KERNAL's disk code crashes on the program's 16-bit stack. Registers and flags pass through both ways, as for a direct `JSR`.

`__rom_call` comes from the library (`stdlib_asm/rom_call.s45`, linked only by programs that call it, #322) and `__jsr`'s register bytes from `c45.lib`/`c45_stack.lib` (`stdlib_asm/cbm_args.s45`), so programs must be linked with the library (a one-step `cc45 prog.c -o prog.prg` does this).

Compatible with C64, C128 and MEGA65 KERNAL jump tables.

## Usage

```c
#include <cbm.h>

int main() {
    cbm_chrout('H');
    cbm_chrout('I');
    cbm_chrout(CH_RETURN);
    return 0;
}
```

## Your own KERNAL calls

A routine the header does not wrap is one `__jsr` (see [cc45.md](../bin/cc45.md#calling-an-address-__jsr-__jsr_raw-__jmp)):

```c
unsigned char row, col;
__jsr(KERNAL_PLOT, .Carry = 1, .X -> row, .Y -> col);   /* where the cursor is */
if (__jsr(KERNAL_SAVE, .A = zp, .XY = end).Carry) ...  /* SAVE failed */
```

Test: `src/test-resources/test_cbm_xemu.c` (the wrappers at run time).

## Character I/O

| Function | Signature | Description |
|----------|-----------|-------------|
| `cbm_chrout` | `void cbm_chrout(char c)` | Output character to the current output channel (KERNAL `$FFD2`) |
| `cbm_chrin` | `int cbm_chrin(void)` | Read character from the current input channel (KERNAL `$FFCF`) |
| `cbm_getin` | `int cbm_getin(void)` | Get character from the keyboard queue, non-blocking (KERNAL `$FFE4`). Returns 0 if the queue is empty |

## I/O Status

| Function | Signature | Description |
|----------|-----------|-------------|
| `cbm_readst` | `int cbm_readst(void)` | Read the I/O status byte (KERNAL `$FFB7`). Bit meanings depend on the device |

## File I/O

| Function | Signature | Description |
|----------|-----------|-------------|
| `cbm_setlfs` | `void cbm_setlfs(char lfn, char device, char sa)` | Set logical file number, device and secondary address (KERNAL `$FFBA`) |
| `cbm_setnam` | `void cbm_setnam(char *name, char len)` | Set filename for the next OPEN/LOAD/SAVE (KERNAL `$FFBD`) |
| `cbm_setbnk` | `void cbm_setbnk(char data_bank, char name_bank)` | The banks (0-5, chip RAM) of the next LOAD/SAVE's data and of its file name (the C65 KERNAL's SETBNK, `$FF6B`) |
| `cbm_load` | `int cbm_load(unsigned int addr)` | LOAD the file set up by `cbm_setbnk`/`cbm_setnam`/`cbm_setlfs` (KERNAL `$FFD5`): with secondary address 0 to `addr` in the SETBNK bank, with 1 to the address the file starts with (its first two bytes, not loaded either way). Returns 0 on success, 1 on error |
| `cbm_load_far` | `int cbm_load_far(char *name, char len, char device, unsigned long lin)` | LOAD file `name` (`len` characters, in bank 0) from `device` to the linear address `lin` in chip RAM (banks 0-5), its two-byte header skipped: `cbm_setbnk`, `cbm_setnam`, `cbm_setlfs` with secondary address 0 and `cbm_load`, then the banks back to 0. Returns 0 on success, 1 on error. `examples/c/glamlambs` loads its music into bank 5 and its salon's picture into banks 4 and 5 with it |
| `cbm_open` | `int cbm_open(void)` | Open the logical file set up by `cbm_setlfs`/`cbm_setnam` (KERNAL `$FFC0`). Returns 0 on success, 1 on error |
| `cbm_close` | `void cbm_close(char lfn)` | Close a logical file (KERNAL `$FFC3`) |
| `cbm_chkin` | `int cbm_chkin(char lfn)` | Set input channel. Returns 0 on success, 1 on error (KERNAL `$FFC6`) |
| `cbm_chkout` | `int cbm_chkout(char lfn)` | Set output channel. Returns 0 on success, 1 on error (KERNAL `$FFC9`) |
| `cbm_clrchn` | `void cbm_clrchn(void)` | Restore default I/O channels (KERNAL `$FFCC`) |
| `cbm_clall` | `void cbm_clall(void)` | Close all open files (KERNAL `$FFE7`) |

`stdio.h` declares a different, library-implemented `cbm_open(device, filename, type)` and `cbm_close(fd)` (cc65 style, see [stdlib.md](stdlib.md#cbm-file-api)). Do not include both headers in one source file if you need the `stdio.h` versions: the `static` definitions in `cbm.h` take over without a diagnostic.

### File I/O Example

```c
#include <cbm.h>

void read_file(void) {
    char ch;
    int st;

    // Open file "DATA" on device 8, secondary address 0
    cbm_setlfs(1, DEV_DISK_8, 0);
    cbm_setnam("data", 4);
    if (cbm_open()) return;      // error

    cbm_chkin(1);                 // set as input channel

    do {
        ch = cbm_chrin();
        cbm_chrout(ch);           // echo to screen
        st = cbm_readst();
    } while (st == 0);

    cbm_clrchn();
    cbm_close(1);
}
```

## Keyboard

| Function | Signature | Description |
|----------|-----------|-------------|
| `cbm_stop` | `int cbm_stop(void)` | Check the STOP key (KERNAL `$FFE1`). Returns 1 if pressed, 0 otherwise |

## Screen

| Function | Signature | Description |
|----------|-----------|-------------|
| `cbm_gotoxy` | `void cbm_gotoxy(char col, char row)` | Set cursor position (0-based). KERNAL PLOT (`$FFF0`) with carry clear |
| `cbm_wherexy` | `int cbm_wherexy(void)` | Get cursor position (PLOT with carry set). Returns col in the low byte, row in the high byte |
| `cbm_screen` | `int cbm_screen(void)` | Get screen size (KERNAL `$FFED`): its X in the low byte, Y in the high byte. The MEGA65's KERNAL gives the last column and row (79 and 24 on the 80x25 screen); the C64's gives the counts |

### Screen Example

```c
#include <cbm.h>

void print_at(char col, char row, char *msg) {
    cbm_gotoxy(col, row);
    while (*msg) {
        cbm_chrout(*msg);
        msg = msg + 1;
    }
}
```

## System Clock

| Function | Signature | Description |
|----------|-----------|-------------|
| `cbm_rdtim` | `int cbm_rdtim(void)` | Read the jiffy clock (1/60 s ticks, KERNAL `$FFDE`). Returns the low 16 bits of the 24-bit clock |

## Serial Bus (IEC)

| Function | Signature | Description |
|----------|-----------|-------------|
| `cbm_listen` | `void cbm_listen(char device)` | Send LISTEN to a device (KERNAL `$FFB1`) |
| `cbm_unlisten` | `void cbm_unlisten(void)` | Send UNLISTEN (KERNAL `$FFAE`) |
| `cbm_talk` | `void cbm_talk(char device)` | Send TALK to a device (KERNAL `$FFB4`) |
| `cbm_untalk` | `void cbm_untalk(void)` | Send UNTALK (KERNAL `$FFAB`) |
| `cbm_second` | `void cbm_second(char sa)` | Send secondary address after LISTEN (KERNAL `$FF93`) |
| `cbm_tksa` | `void cbm_tksa(char sa)` | Send secondary address after TALK (KERNAL `$FF96`) |
| `cbm_ciout` | `void cbm_ciout(char data)` | Send a byte on the serial bus (KERNAL `$FFA8`) |
| `cbm_acptr` | `int cbm_acptr(void)` | Get a byte from the serial bus (KERNAL `$FF13`) |

## PETSCII Control Codes

| Constant | Value | Description |
|----------|-------|-------------|
| `CH_RETURN` | `$0D` | Carriage return |
| `CH_DELETE` | `$14` | Delete character |
| `CH_INSERT` | `$94` | Insert |
| `CH_CLEAR` | `$93` | Clear screen |
| `CH_HOME` | `$13` | Cursor home |
| `CH_CURSOR_UP` | `$91` | Cursor up |
| `CH_CURSOR_DOWN` | `$11` | Cursor down |
| `CH_CURSOR_LEFT` | `$9D` | Cursor left |
| `CH_CURSOR_RIGHT` | `$1D` | Cursor right |
| `CH_REVERSE_ON` | `$12` | Reverse video on |
| `CH_REVERSE_OFF` | `$92` | Reverse video off |

### Color Constants

| Constant | Value |
|----------|-------|
| `CH_COLOR_BLACK` | `$90` |
| `CH_COLOR_WHITE` | `$05` |
| `CH_COLOR_RED` | `$1C` |
| `CH_COLOR_CYAN` | `$9F` |
| `CH_COLOR_PURPLE` | `$9C` |
| `CH_COLOR_GREEN` | `$1E` |
| `CH_COLOR_BLUE` | `$1F` |
| `CH_COLOR_YELLOW` | `$9E` |

## Device Numbers

| Constant | Value | Description |
|----------|-------|-------------|
| `DEV_KEYBOARD` | `0` | Keyboard |
| `DEV_TAPE` | `1` | Datasette |
| `DEV_RS232` | `2` | RS-232 |
| `DEV_SCREEN` | `3` | Screen |
| `DEV_PRINTER` | `4` | Printer |
| `DEV_DISK_8` | `8` | Disk drive (unit 8) |
| `DEV_DISK_9` | `9` | Disk drive (unit 9) |

## KERNAL Jump Table Addresses

All addresses are available as `#define` constants with the `KERNAL_` prefix:

| Constant | Address | Description |
|----------|---------|-------------|
| `KERNAL_ACPTR` | `$FF13` | Get byte from serial bus |
| `KERNAL_CHKIN` | `$FFC6` | Open channel for input |
| `KERNAL_CHKOUT` | `$FFC9` | Open channel for output |
| `KERNAL_CHRIN` | `$FFCF` | Get character from input channel |
| `KERNAL_CHROUT` | `$FFD2` | Output character to channel |
| `KERNAL_CIOUT` | `$FFA8` | Send byte to serial bus |
| `KERNAL_CINT` | `$FF81` | Initialize screen editor |
| `KERNAL_CLALL` | `$FFE7` | Close all open files |
| `KERNAL_CLOSE` | `$FFC3` | Close a logical file |
| `KERNAL_CLRCHN` | `$FFCC` | Clear I/O channels to default |
| `KERNAL_GETIN` | `$FFE4` | Get character from keyboard queue |
| `KERNAL_IOBASE` | `$FFF3` | Get I/O base address |
| `KERNAL_IOINIT` | `$FF84` | Initialize I/O devices |
| `KERNAL_LISTEN` | `$FFB1` | Send LISTEN to serial bus |
| `KERNAL_LOAD` | `$FFD5` | Load from device |
| `KERNAL_MEMBOT` | `$FF9C` | Get/set bottom of memory |
| `KERNAL_MEMTOP` | `$FF99` | Get/set top of memory |
| `KERNAL_OPEN` | `$FFC0` | Open a logical file |
| `KERNAL_PLOT` | `$FFF0` | Get/set cursor position |
| `KERNAL_RAMTAS` | `$FF87` | Initialize RAM, tape buffer, screen |
| `KERNAL_RDTIM` | `$FFDE` | Read system clock |
| `KERNAL_READST` | `$FFB7` | Read I/O status word |
| `KERNAL_RESTOR` | `$FF8A` | Restore default I/O vectors |
| `KERNAL_SAVE` | `$FFD8` | Save to device |
| `KERNAL_SCNKEY` | `$FF9F` | Scan keyboard |
| `KERNAL_SCREEN` | `$FFED` | Get screen size |
| `KERNAL_SECOND` | `$FF93` | Send secondary address after LISTEN |
| `KERNAL_SETLFS` | `$FFBA` | Set logical/first/second address |
| `KERNAL_SETMSG` | `$FF90` | Set KERNAL message control |
| `KERNAL_SETNAM` | `$FFBD` | Set filename |
| `KERNAL_SETTIM` | `$FFDB` | Set system clock |
| `KERNAL_STOP` | `$FFE1` | Check for STOP key |
| `KERNAL_TALK` | `$FFB4` | Send TALK to serial bus |
| `KERNAL_TKSA` | `$FF96` | Send secondary address after TALK |
| `KERNAL_UDTIM` | `$FFEA` | Update system clock |
| `KERNAL_UNLSN` | `$FFAE` | Send UNLISTEN to serial bus |
| `KERNAL_UNTLK` | `$FFAB` | Send UNTALK to serial bus |
| `KERNAL_VECTOR` | `$FF8D` | Get/set KERNAL vectors |

## Implementation Notes

- The functions are `static`, so each source file that uses one gets its own copy (named `_cbm_<name>__<file>`); functions a file does not call are removed. They are called with `JSR` like other functions, not inlined.
- Parameters reach the inline assembly through the `__cbm_*` bytes: inline assembly has no single way to name a parameter in every calling convention (`lda.sp @_p_x` is only right for a stack frame).
- The KERNAL addresses are hardcoded — they are the same on C64, C128 and MEGA65 ROMs.
- Values are returned in A (low) and X (high), cc45's return convention.
- Functions that can fail convert the KERNAL carry flag into the return value: carry set = 1 (error), clear = 0.

## See Also

- [stdlib.md](stdlib.md) — standard C library (including the `stdio.h` CBM file API)
- [../bin/cc45.md](../bin/cc45.md) — C compiler (inline assembly, calling conventions)
