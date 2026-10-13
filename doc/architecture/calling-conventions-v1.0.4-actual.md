# Calling Conventions (v1.0.4 snapshot) — superseded

This page used to describe the v1.0.4 calling convention. That description no
longer matches the toolchain:

- **Default convention.** The default is now the ZP convention, with
  parameters in `$10-$2F`, plus static allocation (SAC).
- **Frame pointer.** The frame pointer is callee-saved and is set up only
  when a function has a stack frame.
- **Struct and float returns.** These use a caller-provided hidden pointer.
- **Stack.** The stack is 16-bit, with its top at `$BFFF`.
- **Mismatched conventions.** ln45 bridges them in both directions.

See [calling-conventions.md](calling-conventions.md) for the current reference
and [calling-conventions-quick-reference.md](calling-conventions-quick-reference.md)
for a summary.
