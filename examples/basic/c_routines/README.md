# BASIC 65 calling C routines

`menu.bas` is a BASIC 65 program (basic45 source, with labels instead of line
numbers); `routines.c` holds the C functions it calls.

```bash
basic45 --link routines.c menu.bas -o menu.prg
```

basic45 compiles `routines.c` with cc45, links it (with the C library) right
after the BASIC program, and puts the addresses into the program:

- `SYS {sum_to}, n` calls `sum_to(n)`. The SYS's A, X and Y are the function's
  first three parameters (`unsigned char` or `unsigned int`).
- `RREG LO, HI` reads the result (A = low byte, X = high byte).
- The C code runs with its own zero page and stack (crt0's `__basic_call`):
  globals keep their values from one SYS to the next.

`make run` builds the program and runs it in xemu (`basic45 --emulator`).
