# dbg45 — Source-Level Debugger

`dbg45` debugs a cc45 program by its C source in xemu: breakpoints by line and function, stepping by line, the call stack, and variables by name and type. It reads the program's debug file (`program.dbg`, from `ln45 -g` or `cc45 -g`: [ln45.md](ln45.md#debug-file)), starts xemu with the program and drives it through xemu's monitor socket. The same debugger speaks the Debug Adapter Protocol (`--dap`), which the VS Code extension in `tools/ide` uses (#202).

## Requirements

- The program built with debug information: `cc45 -g main.c -o prog.prg`, or `ln45 -g ...` (objects always carry it).
- An xemu whose monitor has a **breakpoint list** that stops before the opcode: the `k` command of [CTalkobt/xemu](https://github.com/CTalkobt/xemu), branch `cc45-dev` (the one the Docker images build, with the hardware divider and `XEMU_PREF_DIR` too; `monitor-breakpoints` has the breakpoints alone) (`k` lists, `k+ADDR` adds, `k-ADDR` removes, `k-` clears). The monitor's own `b` holds one breakpoint and stops after the opcode has run. dbg45 says so when the xemu it runs has no `k`. `--xemu`, `$DBG45_XEMU` or `xemu-xmega65` on `PATH`.

## Usage

```bash
cc45 -O2 -g main.c -o prog.prg
dbg45 prog.prg                                  # interactive: (dbg45) prompt
dbg45 --headless --batch -ex "break sum" -ex start -ex bt -ex "print *p" prog.prg
dbg45 --dap                                     # for an editor
```

| Option | Meaning |
|--------|---------|
| `--xemu <path>` | The xemu to run (default `$DBG45_XEMU`, else `xemu-xmega65`) |
| `--xemu-arg <arg>` | One of xemu's options (again for more: `--xemu-arg -8 --xemu-arg disk.d81`) |
| `--headless` | xemu without a window |
| `--fast` | xemu as fast as it can (`-sleepless`) |
| `--timeout <s>` | A run or continue that reaches no breakpoint in s seconds (a fraction is allowed: `0.5`) pauses the program there (`[no stop within the timeout: paused]`). Default: no limit |
| `-ex <command>` | A command to run first (again for more) |
| `-x <file>` | The commands of a file, one a line (empty lines and lines starting with `#` are skipped) |
| `--batch` | Quit after the `-ex` / `-x` commands (each is echoed after the prompt) |
| `--dap` | The Debug Adapter Protocol on stdin/stdout |
| `-v` … `-vvvv`, `-v0` | Log on stderr as `[dbg45:LEVEL] …` (the toolchain's format, `include/Logger.hpp`); cumulative, at most `-vvvv`; `-v0` off: the program and its debug file, xemu's command line and the monitor connection (`-v`), each monitor command (`-vv`) and its reply (`-vvv`); with `--dap` too (the protocol stays on stdout) |
| `-V`, `--version`; `-h`, `--help` | Version; help |

## Commands

As gdb names them; RETURN at the prompt repeats the last command (at a terminal; commands can also be piped in on standard input, without the prompt). Breakpoints can be set before `run`; the commands that read the program (`bt`, `info locals`, `print`, …) answer `error: the program is not running` until it runs.

| Command | Meaning |
|---------|---------|
| `run` (`r`) | Start the program (again: xemu is restarted) and run to a breakpoint or its end |
| `start` | The same, stopping at `main` |
| `continue` (`c`) | On to the next breakpoint or the end |
| `next` (`n`) [N] | The next source line in this function (calls run through) |
| `step` (`s`) [N] | The next source line, into calls (a call into code without lines, a library's assembly, runs through) |
| `finish` | Until the function returns (stops in the caller) |
| `stepi` (`si`) [N] | One instruction |
| `break` (`b`) LOC | A breakpoint: `function`, `file.c:line`, `line` (of the file it stopped in), `*ADDR` (`*$2124`, `*0x2124`, or decimal `*8484`). A line without code moves to the next one with code; a line whose code is in several places (a `for` loop's parts) breaks at each |
| `delete` (`d`) [N] | Delete a breakpoint (all without N) |
| `info breakpoints` | The breakpoints and their addresses (`info` is also `i`, its word any abbreviation: `i b`) |
| `info locals`, `info args` | The frame's local variables, its parameters |
| `info registers` | A X Y Z B SP P (and its flags `NVEBDIZC`), PC |
| `info frame` | The selected frame |
| `backtrace` (`bt`, `where`) | The call stack |
| `frame` (`f`) [N], `up` [N], `down` [N] | Select a frame (variables are read in it); `frame` alone shows the selected one |
| `print` (`p`) EXPR | A value: a variable, `*e`, `&e`, `e.member`, `e->member`, `e[i]`, numbers (`$hex`) |
| `x` ADDR [N] | N bytes of memory (16 without N; ADDR: `$hex` or an expression: a pointer's target, another variable's own address) |
| `list` (`l`) | The source around the selected frame's line (4 lines before it, 5 after; `=>` marks it) |
| `kill` | End xemu |
| `quit` (`q`) | Quit |
| `help` (`h`) | The usage text and this list |

The program's end is reported with its exit status (`[program exited with status 11]`: `main`'s return value or `exit()`'s, at crt0's `__exit`).

## A Session

`sum.c`, built with `cc45 -O2 -g sum.c -o sum.prg`. The build writes `sum.dbg` beside the program; dbg45 looks for
the debug file under the program's name with its extension replaced by `.dbg`, in the program's directory.

```c
#include <stdio.h>

struct point { int x, y; };

int sum(struct point *p, int n) {
    int total = 0;
    for (int i = 0; i < n; i++)
        total += p[i].x + p[i].y;
    return total;
}

int main(void) {
    struct point pts[2] = { {1, 2}, {3, 5} };
    int t = sum(pts, 2);
    printf("%d\n", t);
    return t;
}
```

The session below is `dbg45 --headless --batch` with each command as an `-ex` (batch mode prints the command
after the prompt; at a terminal it is what you type):

```
(dbg45) break sum
Breakpoint 1 at $2124: sum
(dbg45) info breakpoints
1   sum at $2124
(dbg45) run

Breakpoint 1, sum (p=0x2b48, n=2) at sum.c:6
6	    int total = 0;
(dbg45) bt
#0  sum (p=0x2b48, n=2) at sum.c:6
#1  main () at sum.c:14
(dbg45) info args
p = 0x2b48
n = 2
(dbg45) print *p
*p = {x = 1, y = 2}
(dbg45) print p[1].y
p[1].y = 5
(dbg45) next
7	    for (int i = 0; i < n; i++)
(dbg45) next
8	        total += p[i].x + p[i].y;
(dbg45) info locals
total = 0
i = 0
(dbg45) finish
Run till exit from sum (p=0x2b48, n=2) at sum.c:8
main () at sum.c:14
14	    int t = sum(pts, 2);
(dbg45) list
  10	}
  11	
  12	int main(void) {
  13	    struct point pts[2] = { {1, 2}, {3, 5} };
=>14	    int t = sum(pts, 2);
  15	    printf("%d\n", t);
  16	    return t;
  17	}
(dbg45) continue
[program exited with status 11]
```


- **Breakpoints** are xemu's `k` list: they stop before the instruction at the address. A stop at an address whose bytes are not the program's (the ROM's code, mapped there while BASIC runs before the program is loaded) resumes by itself. A function breakpoint covers both entries of a static-allocation function (`f` and `f__sac`, which callers that know it call).
- **next** and **finish** set temporary breakpoints (the function's other line starts and its return address; the return address) and run at full speed, going on past a stop deeper in the stack (recursion). **step** runs one instruction at a time until the line changes (xemu's monitor answers once a frame while paused, so long lines step slowly). A call cc45 inlined (`-O2`'s small functions) is part of its statement's line: `next` and `step` go over it, and the function's own lines are not stepped through.
- **The call stack**: the return addresses on the CPU stack that a `JSR` (or `BSR`) into the frame's function pushed, from SP up to `__stack_top`.
- **Variables**: the debug file gives each one's place: a fixed address (globals, statics, a static-allocation function's locals and parameters, a base-page block's bytes), a byte of the base page the code runs with (the B register: the program's, or an interrupt handler's own), a frame offset from the frame pointer at `$FD/$FE`, or registers. A caller's base-page, frame and register values are not shown (the callee may have changed them). A local kept in storage shared with another the function does not have live at the same time shows that one's value while it is not live. Types: integers by size and signedness (cc45's plain types are unsigned), `char` with its PETSCII character, `bool`, the CBM 5-byte float, pointers (`char *` with its string), arrays (`char` arrays as strings), structs and unions with bit-fields, a struct parameter passed by copy.

## Debug Adapter Protocol

`dbg45 --dap` handles `initialize`, `launch` (`program`, `xemu`, `xemuArgs`, `headless`, `fast`, `stopOnEntry`), `setBreakpoints`, `setFunctionBreakpoints`, `setExceptionBreakpoints` (none), `configurationDone` (starts the program), `threads` (one: the MEGA65), `stackTrace`, `scopes` (Locals, Arguments, Globals), `variables` (structs, unions and arrays expand), `evaluate`, `continue`, `next`, `stepIn`, `stepOut`, `pause` (also while the program runs), `disconnect` and `terminate`; it sends `initialized`, `stopped`, `exited`, `terminated` and `output`.

To `initialize` it answers the capabilities `supportsConfigurationDoneRequest`, `supportsFunctionBreakpoints`, `supportsEvaluateForHovers` and `supportsTerminateRequest`; nothing else is announced (no conditional breakpoints, no setting of variables, no stepping back).

In VS Code, the extension in `tools/ide/vscode-extension` contributes the `cc45` debug type (setting `m65.dbg45Path`, default `${workspaceFolder}/bin/dbg45`):

```json
{
  "type": "cc45",
  "request": "launch",
  "name": "Debug in xemu",
  "program": "${workspaceFolder}/prog.prg",
  "stopOnEntry": true,
  "xemu": "/path/to/xmega65.native"
}
```

## Exit Codes

0, or 1 on an error before it starts (no program, no debug file, an unknown option, an unreadable `-x` file); a session's errors are printed (`error: ...`) and it goes on.

## Tests

`src/test/test_dbg45.sh` (run by `test_xemu.sh`, both calling conventions): a command-line session on `test_debug_file_xemu.c` checked line by line, a DAP session and a DAP pause (`src/test/dap_client.py`), `--timeout`. Without an xemu that has the breakpoint list it is skipped (`DBG45_XEMU` points at one).

## See Also

- [ln45](ln45.md#debug-file) — the debug file (`-g`)
- [cc45](cc45.md) — `-g` in a one-step build
- [objdump45](objdump45.md) — `-g`, an object's debug information
