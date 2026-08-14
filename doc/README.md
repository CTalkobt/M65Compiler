# MEGA65 Compiler Suite Documentation

This directory contains the documentation for the MEGA65 C compiler suite (targeting the 45GS02).

## Directory Structure

To keep the documentation clean and organized, documents are separated by their function and scope:

- **[`bin/`](bin/)**: Program-specific user guides, syntax references, and CLI manuals for toolchain binaries.
- **[`architecture/`](architecture/)**: Internal design specifications, calling conventions, file formats, and compiler development details (e.g. coverage, testing, environment setup).
  - **[`architecture/bug/`](architecture/bug/)**: Technical bug reports, root cause analyses, and solutions.

---

## Documentation Specification (Rules & Conventions)

All future documentation must strictly adhere to the following directory rules to ensure cleanliness and avoid broken links. An automated check runs with the test suite to enforce these rules.

### 1. File Placement & Naming
- **No files in the root `doc/` directory** are permitted other than this `README.md`.
- **`doc/bin/`** must contain only documentation files for valid toolchain binaries/tools. Allowed files are:
  - `ar45.md` (Archiver)
  - `ca45.md` (Assembler)
  - `cc45.md` (Compiler)
  - `cp45.md` (Preprocessor)
  - `cvt_asm.md` (Format Converter)
  - `disk45.md` (Disk Utility)
  - `ln45.md` (Linker)
  - `nm45.md` (Symbol Lister)
  - `objdump45.md` (Object Disassembler)
  - `CONFIGURATION.md` (Shared configuration file specification)
- **`doc/architecture/`** must contain only markdown files (`.md`) describing core coding details, ABI conventions, or internal features (such as `calling-conventions.md`, `ir.md`, `lib45.md`, `stdlib.md`, etc.).
  - The only allowed subdirectory is `bug/`.
- **`doc/architecture/bug/`** must contain only bug reports and diagnostic logs ending in `.md`.

### 2. Relative Link Guidelines
Since files are nested in different folders, links between them must use correct relative paths:
- **Inside `doc/bin/` linking to `doc/architecture/`**: Use the `../architecture/` prefix, for example: `[lib45]``(../architecture/lib45.md)`.
- **Inside `doc/architecture/` linking to `doc/bin/`**: Use the `../bin/` prefix, for example: `[cc45]``(../bin/cc45.md)`.
- **Links between files in the same directory**: Use only the filename without directory prefixes, for example: `[stdcbm]``(stdcbm.md)` inside `stdlib.md` (which links to `doc/architecture/stdcbm.md`).

---

## Automated Verification
To ensure compliance and find broken links, you can run the validation script from the project root:
```bash
python3 src/test/check_docs.py
```
This is run automatically as part of the test suite.
