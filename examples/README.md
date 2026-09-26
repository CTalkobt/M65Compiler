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
- **bitfield_registers/** - MEGA65 VIC-IV hardware register access using C bitfield structs. Maps control registers, sprite enables, and screen attributes as typed bitfield overlays.
- **game_of_life/** - Conway's Game of Life on the 40x25 text screen with wrapping edges. Demonstrates 2D arrays, nested loops, and direct screen/color RAM access.

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
