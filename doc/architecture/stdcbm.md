# cbm.h — Commodore KERNAL Interface for MEGA65

`<cbm.h>` provides inline C wrappers for the standard Commodore KERNAL jump table, plus PETSCII control codes and device number constants. All functions use `__asm__` to call KERNAL routines directly, with zero overhead beyond the register setup and `JSR`.

Compatible with C64, C128, and MEGA65 KERNAL.

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

## Character I/O

| Function | Signature | Description |
|----------|-----------|-------------|
| `cbm_chrout` | `void cbm_chrout(char c)` | Output character to current channel (KERNAL `$FFD2`) |
| `cbm_chrin` | `int cbm_chrin(void)` | Read character from current input channel (KERNAL `$FFCF`) |
| `cbm_getin` | `int cbm_getin(void)` | Get character from keyboard queue, non-blocking (KERNAL `$FFE4`). Returns 0 if queue is empty |

## I/O Status

| Function | Signature | Description |
|----------|-----------|-------------|
| `cbm_readst` | `int cbm_readst(void)` | Read I/O status byte (KERNAL `$FFB7`). Bit meanings depend on device |

## File I/O

| Function | Signature | Description |
|----------|-----------|-------------|
| `cbm_setlfs` | `void cbm_setlfs(char lfn, char device, char sa)` | Set logical file number, device, and secondary address (KERNAL `$FFBA`) |
| `cbm_setnam` | `void cbm_setnam(char *name, char len)` | Set filename for next OPEN/LOAD/SAVE (KERNAL `$FFBD`) |
| `cbm_open` | `int cbm_open(void)` | Open logical file (call `cbm_setlfs`/`cbm_setnam` first). Returns 0 on success, 1 on error |
| `cbm_close` | `void cbm_close(char lfn)` | Close a logical file (KERNAL `$FFC3`) |
| `cbm_chkin` | `int cbm_chkin(char lfn)` | Set input channel. Returns 0 on success, 1 on error (KERNAL `$FFC6`) |
| `cbm_chkout` | `int cbm_chkout(char lfn)` | Set output channel. Returns 0 on success, 1 on error (KERNAL `$FFC9`) |
| `cbm_clrchn` | `void cbm_clrchn(void)` | Restore default I/O channels (KERNAL `$FFCC`) |
| `cbm_clall` | `void cbm_clall(void)` | Close all open files (KERNAL `$FFE7`) |

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
| `cbm_stop` | `int cbm_stop(void)` | Check if STOP key is pressed. Returns non-zero if pressed (KERNAL `$FFE1`) |

## Screen

| Function | Signature | Description |
|----------|-----------|-------------|
| `cbm_gotoxy` | `void cbm_gotoxy(char col, char row)` | Set cursor position (0-based). Uses KERNAL PLOT (`$FFF0`) with carry clear |
| `cbm_wherexy` | `int cbm_wherexy(void)` | Get cursor position. Returns col in low byte, row in high byte |
| `cbm_screen` | `int cbm_screen(void)` | Get screen size. Returns cols in low byte, rows in high byte (KERNAL `$FFED`) |

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
| `cbm_rdtim` | `int cbm_rdtim(void)` | Read jiffy clock (1/60s ticks). Returns low 16 bits of the 24-bit clock (KERNAL `$FFDE`) |

## Serial Bus (IEC)

| Function | Signature | Description |
|----------|-----------|-------------|
| `cbm_listen` | `void cbm_listen(char device)` | Send LISTEN command to device (KERNAL `$FFB1`) |
| `cbm_unlisten` | `void cbm_unlisten(void)` | Send UNLISTEN (KERNAL `$FFAE`) |
| `cbm_talk` | `void cbm_talk(char device)` | Send TALK command to device (KERNAL `$FFB4`) |
| `cbm_untalk` | `void cbm_untalk(void)` | Send UNTALK (KERNAL `$FFAB`) |
| `cbm_second` | `void cbm_second(char sa)` | Send secondary address after LISTEN (KERNAL `$FF93`) |
| `cbm_tksa` | `void cbm_tksa(char sa)` | Send secondary address after TALK (KERNAL `$FF96`) |
| `cbm_ciout` | `void cbm_ciout(char data)` | Send byte to serial bus (KERNAL `$FFA8`) |
| `cbm_acptr` | `int cbm_acptr(void)` | Get byte from serial bus (KERNAL `$FF13`) |

## PETSCII Control Codes

| Constant | Value | Description |
|----------|-------|-------------|
| `CH_RETURN` | `$0D` | Carriage return |
| `CH_DELETE` | `$14` | Delete character |
| `CH_INSERT` | `$94` | Insert mode |
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

All functions are declared `static` in the header and use `__asm__` inline assembly. This means:

- No separate `.o45` module is needed — functions are inlined at each call site.
- Only functions you actually call generate code (dead code elimination by the compiler).
- The KERNAL addresses are hardcoded — these are stable across C64/C128/MEGA65 ROMs.
- Functions that return values leave the result in A (low) and X (high), matching `cc45`'s return convention.
- Error-returning functions use the KERNAL carry convention: carry set = error, converted to 0/1 return value.

## See Also

- [stdlib.md](stdlib.md) — Standard C library overview
- [cc45.md](cc45.md) — C compiler (inline assembly, calling conventions)
