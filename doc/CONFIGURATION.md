# MEGA65 Toolchain Configuration

This document describes how to configure the MEGA65 C compiler suite using configuration files and command-line options.

## Configuration Files

The MEGA65 toolchain supports persistent configuration via files stored in `~/.config/m65/`. Each tool has its own configuration file:

| Tool | Configuration File | Purpose |
|------|-------------------|---------|
| `cc45` | `~/.config/m65/cc45.conf` | C compiler configuration |
| `ca45` | `~/.config/m65/ca45.conf` | Assembler configuration |
| `ln45` | `~/.config/m65/ln45.conf` | Linker configuration |
| `ar45` | `~/.config/m65/ar45.conf` | Archiver configuration |
| `nm45` | `~/.config/m65/nm45.conf` | Symbol lister configuration |
| `objdump45` | `~/.config/m65/objdump45.conf` | Object file disassembler configuration |
| `disk45` | `~/.config/m65/disk45.conf` | Disk utility configuration |
| `cp45` | `~/.config/m65/cp45.conf` | Preprocessor configuration |

## Configuration File Format

Configuration files use a simple line-based format. Each line contains a single command-line argument or option:

- **Blank lines** are ignored
- **Comments** start with `#` at the beginning of a line and extend to the end of the line
- **Each line** is treated as a single argument (as if passed on the command line)
- **Quotes** are supported for arguments with spaces (e.g., `--pragma "cc45 heap"`)

### Examples

**~/.config/m65/cc45.conf** — C compiler defaults:

```
# Use standard optimizations by default
-O2

# Disable JSR relocation optimization
-PNoJSRRelocate

# Enable inline functions
-finline-functions

# Add default include path
-I./include
```

**~/.config/m65/ca45.conf** — Assembler defaults:

```
# Use standard optimizations
-O2

# Enable optimizer reporting
-Roptimizer

# Verbose mode
-v
```

**~/.config/m65/ln45.conf** — Linker defaults:

```
# Search additional library paths
-L./lib
-L./vendor/lib
```

## Precedence and Override Rules

Configuration file settings and command-line arguments are merged in the following order (highest to lowest precedence):

1. **Command-line arguments** (highest precedence — always win)
2. **Configuration file arguments** (loaded from `~/.config/m65/<tool>.conf`)
3. **Built-in defaults** (lowest precedence)

When both a configuration file and command-line arguments specify the same option, the command-line argument takes precedence.

### Example Scenarios

**Scenario 1: Config file specifies `-O0`, CLI specifies `-O2`**

```bash
$ cat ~/.config/m65/cc45.conf
-O0

$ cc45 -O2 program.c
# Result: -O2 is used (CLI wins)
```

**Scenario 2: Config file enables an optimization, CLI disables it**

```bash
$ cat ~/.config/m65/cc45.conf
-O2
-PTailCall

$ cc45 program.c -PNoTailCall
# Result: Tail call optimization is disabled (CLI wins)
```

**Scenario 3: Config file and CLI both enable the same optimization**

```bash
$ cat ~/.config/m65/cc45.conf
-O2
-PStrengthReduction

$ cc45 program.c -PStrengthReduction
# Result: -O2 is used with strength reduction enabled (consistent)
```

## Optimization Configuration

### Optimization Levels

Optimization levels control which optimization passes are enabled by default:

| Level | Enabled Passes | Use Case |
|-------|-----------------|----------|
| `-O0` | None | Debugging, fastest compilation |
| `-O1` | Basic passes | Balances speed and code size |
| `-O2` | All standard passes (default) | Production code |
| `-O3` | All standard passes (reserved for future) | Aggressive optimization |

### Per-Pass Control

Individual optimization passes can be controlled independently using `-P<Name>` and `-PNo<Name>` flags. These override the baseline set by the `-O` level:

```bash
# Start with -O2 baseline, disable one pass
cc45 -O2 -PNoTailCall program.c

# Start with -O0 baseline, enable one pass
cc45 -O0 -PStrengthReduction program.c

# Enable multiple specific passes
cc45 -O0 -PStrengthReduction -PCSE -PConstantFold program.c
```

### Available Optimizations

#### Compiler (cc45) Optimization Passes

- `StrengthReduction` — Convert expensive operations to faster equivalents
- `AlgebraicSimplify` — Simplify algebraic expressions
- `TypeNarrowing` — Reduce type width when possible
- `BranchFold` — Fold constant conditions into branches
- `CSE` — Common subexpression elimination
- `LICM` — Loop-invariant code motion
- `CopyChains` — Copy chain propagation
- `AddrElemFusion` — Address element fusion

#### Assembler (ca45) Optimization Passes

- `JSRRelocate` — JSR to BSR conversion
- `TailCall` — Tail call optimization
- `BranchInvert` — Branch inversion
- `JmpBra` — JMP to BRA conversion
- `NoOpBra` — No-op branch elimination
- `CmpElimination` — Compare elimination
- `RedundantLoad` — Redundant load elimination
- `DeadStore` — Dead store elimination
- `TailDedup` — Tail deduplication
- `PreserveXSP` — Preserve XSP optimization
- `SeqExtract` — Sequence extraction (experimental)
- `StoreLoadPair` — Store-load pairing
- `FcmpOpt` — Floating-point compare optimization
- `TsxRedundant` — TSX redundancy elimination

### Configuration File Examples

**Aggressive optimization for performance-critical code:**

```
# ~/.config/m65/cc45.conf
-O2
-PStrengthReduction
-PCSE
-PLICM
```

**Conservative optimization for debugging:**

```
# ~/.config/m65/cc45.conf
-O0
-PDeadStore
```

**Assembler-specific configuration:**

```
# ~/.config/m65/ca45.conf
-O2
-PNoSeqExtract
-Roptimizer
```

## Tool-Specific Configuration

### cc45 (C Compiler)

Common configuration options:

```
# Calling convention
-fzpcall

# Inline functions
-finline-functions

# Include paths
-I/usr/local/include/m65
-I./include

# Optimization settings
-O2
-PNoJSRRelocate

# Breakpoint setting
-fset-bp=0x2000
```

### ca45 (Assembler)

Common configuration options:

```
# Zero page configuration
-Dcc45.zeroPageStart=$20
-D__sp_base=$0101

# Optimization reporting
-Roptimizer

# Verbosity
-vv
```

### ln45 (Linker)

Common configuration options:

```
# Library search paths
-L./lib
-L$(PREFIX)/lib/cc45

# Output format
--prg
```

## Environment Variables

Some tools also support environment variables for configuration:

| Variable | Tools | Description |
|----------|-------|-------------|
| `CC45_INCLUDE` | cc45, cp45 | Colon-separated include search paths |
| `M65_CC_FLAGS` | cc45 | Default compiler flags |
| `M65_ASM_FLAGS` | ca45 | Default assembler flags |

Environment variables are checked after configuration files but before built-in defaults.

## Creating a Configuration File

1. Create the configuration directory:
   ```bash
   mkdir -p ~/.config/m65
   ```

2. Create a configuration file for your tool:
   ```bash
   cat > ~/.config/m65/cc45.conf << 'EOF'
   -O2
   -finline-functions
   -fzpcall
   EOF
   ```

3. Verify the configuration is loaded by running with verbose output:
   ```bash
   cc45 -v program.c
   ```

## Disabling Configuration Files

Configuration files are always loaded automatically. To override all configuration file settings, explicitly specify conflicting options on the command line (which take precedence).

To temporarily disable a configuration file without modifying it, you can use environment variables or rename the file:

```bash
# Rename configuration file
mv ~/.config/m65/cc45.conf ~/.config/m65/cc45.conf.bak

# Restore later
mv ~/.config/m65/cc45.conf.bak ~/.config/m65/cc45.conf
```

## Troubleshooting

### Configuration not being applied

1. Verify the file exists and is readable:
   ```bash
   ls -la ~/.config/m65/cc45.conf
   cat ~/.config/m65/cc45.conf
   ```

2. Check for syntax errors (blank lines and comments are allowed):
   ```bash
   # Ensure each line is a valid command-line argument
   ```

3. Run with verbose output to see what options are being used:
   ```bash
   cc45 -v program.c
   ```

### Conflicting options

If a configuration file and command-line options conflict, the command-line always wins:

```bash
# Config file has: -O0
# CLI has: -O2
# Result: -O2 is used
cc45 -O2 program.c
```

## See Also

- [cc45.md](cc45.md) — C compiler documentation
- [ca45.md](ca45.md) — Assembler documentation
- [ln45.md](ln45.md) — Linker documentation
- [CLAUDE.md](../CLAUDE.md) — Overall toolchain architecture
