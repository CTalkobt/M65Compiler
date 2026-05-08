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

### Assembly Examples

- **asm/zpcall_prototype.s** - Prototype for zero-page calling convention

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
