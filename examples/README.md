# m65compiler Examples

These examples demonstrate different features of the m65compiler.

## Available Examples

### C Examples

- **hello_world/** - Classic hello world program that prints to console
- **unit_convert/** - Unit conversion example demonstrating arithmetic operations
- **stdlib/string_h/** - String library examples (strlen, strcmp, strcpy, etc.)
- **stdlib/stdlib_h/** - Standard library examples (atoi, itoa)
- **stdlib/ctype_h/** - Character type examples for classification and conversion
- **palette_fade/** - Demonstrates mixed C and assembly code with calling conventions and MEGA65 hardware register access. C controls fade timing while assembly manipulates the palette.
- **hello_linked/** - Minimal linked C program showing the `cc45 -c` + `ln45` relocatable build workflow.
- **multi_module/** - Multi-file project with shared header, separate compilation of 3 modules, and linking. Demonstrates extern, struct pass/return, and the standard build workflow.
- **bitfield_registers/** - MEGA65 VIC-III/VIC-IV register access using C bitfield structs: `$D031` and `$D054` (layouts per the MEGA65 Book and mega65-core's `iomap.txt`), sprite enables, and VIC-III extended attributes (blink/reverse/bold/underline in colour RAM).
- **game_of_life/** - Conway's Game of Life on the 80x25 text screen (`$0800`) with wrapping edges. Demonstrates 2D arrays, nested loops, `signed int` arithmetic (plain `int` is unsigned in cc45), and direct screen RAM access.
- **memset_screen/** - Fill screen with character codes 0-255, demonstrating memset with direct screen RAM writes.
- **text_scroller/** - Horizontal text scroller demonstrating mixed C and assembly (arguments through shared variables, so both calling conventions work), with a bouncing ball (hardware sprite 0, VIC-IV 16-bit sprite pointers) that passes in front of and behind the reverse-video message.
- **far_scroller/** - A 400x25 landscape map allocated with `far_malloc` in bank 4, scrolled with one `dma_copy_far` per row each frame (parallax sky), colours written straight into colour RAM at `$FF80000` with `dma_fill_far`.
- **vic4_colors/** - Steps the border and background colours through a struct overlay on `$D020`/`$D021`, ending on white with black text.
- **stdlib/math_h/** - Math library examples (min/max, gcd/lcm)

### BASIC Examples

- **basic/c_routines/** - A BASIC 65 program (basic45 source with labels) calling C functions: `basic45 --link routines.c menu.bas`, `SYS {name}, a, x, y` and `RREG`.

### Assembly Examples

- **asm/zpcall_prototype.s45** - Prototype for zero-page calling convention

## Building Examples

### Build all examples
```bash
make -C examples all
```

### Build a specific example
```bash
make -C examples/<example>
```

### Run an example
Individual example directories contain Makefiles with a `run` target:
```bash
make -C examples/<example> run
```

This launches the example in the xemu-xmega65 emulator (if available).

## Emulator

Examples can be executed using the **xemu-xmega65** emulator. The emulator allows you to see the program output and interact with MEGA65 hardware features.
