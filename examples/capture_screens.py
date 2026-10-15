#!/usr/bin/env python3
"""Screenshots of the examples for doc/examples/README.md.

Usage (from the project root, after `make -C examples`):
    python3 examples/capture_screens.py [name ...]

Each program runs in xemu-xmega65 at real speed (no -sleepless: the time must
be the program's own). The script watches xemu's UART monitor (a Unix socket)
until the PRG's bytes are in memory at its load address (xemu has typed RUN
right after loading it), waits 2.5 seconds, and stops xemu, which saves its
-screenshot on exit. The pictures go to doc/examples/images/<name>.png.
"""
import os
import signal
import socket
import subprocess
import sys
import tempfile
import time

DELAY = 2.5
OUT = "doc/examples/images"

# name: the PRG (the examples' build outputs)
EXAMPLES = {
    "hello_world": "examples/c/hello_world/hello.prg",
    "hello_linked": "examples/c/hello_linked/hello.prg",
    "multi_module": "examples/c/multi_module/program.prg",
    "unit_convert": "examples/c/unit_convert/convert.prg",
    "palette_fade": "examples/c/palette_fade/palette_fade.prg",
    "bitfield_registers": "examples/c/bitfield_registers/bitfield_demo.prg",
    "game_of_life": "examples/c/game_of_life/life.prg",
    "memset_screen": "examples/c/memset_screen/memset_screen.prg",
    "text_scroller": "examples/c/text_scroller/scroller.prg",
    "vic4_colors": "examples/c/vic4_colors/vic4_colors.prg",
    "far_scroller": "examples/c/far_scroller/far_scroller.prg",
    "invaders": "examples/c/invaders/invaders.prg",
    "mandelbrot": "examples/c/mandelbrot/mandelbrot.prg",
    "musicgen": "examples/c/musicgen/musicgen.prg",
    "calc_graph": "examples/c/calc_graph/calc_graph.prg",
    "forth": "examples/c/forth/forth.prg",
    "rogue": "examples/c/rogue/rogue.prg",
    "rtc_clock": "examples/c/rtc_clock/rtc_clock.prg",
    "ctype_demo": "examples/c/stdlib/ctype_h/ctype_demo.prg",
    # the library checks print nothing: one picture stands for them all
    "test_strlen": "examples/c/stdlib/string_h/test_strlen.prg",
    "c_routines": "examples/basic/c_routines/menu.prg",
}


def monitor(sock, cmd):
    """Send a monitor command, return its output (up to the next prompt)."""
    sock.sendall((cmd + "\r").encode())
    data = b""
    end = time.time() + 2
    while time.time() < end:
        try:
            chunk = sock.recv(4096)
        except socket.timeout:
            break
        if not chunk:
            break
        data += chunk
        if b"." in chunk.split(b"\n")[-1] or data.rstrip().endswith(b"."):
            break
    return data.decode(errors="replace")


def loaded(sock, addr, want):
    """Are the PRG's first bytes at addr?"""
    out = monitor(sock, "m%x" % addr)
    for line in out.splitlines():
        line = line.strip()
        if line.startswith(":"):
            parts = line[1:].split(":", 1)
            if len(parts) == 2 and int(parts[0], 16) == addr:
                got = bytes.fromhex(parts[1].strip()[:2 * len(want)])
                return got == want
    return False


def capture(name, prg):
    data = open(prg, "rb").read()
    addr = data[0] | data[1] << 8
    want = data[2:18]
    png = os.path.abspath(os.path.join(OUT, name + ".png"))
    if os.path.exists(png):
        os.remove(png)
    tmp = tempfile.mkdtemp()
    sockname = os.path.join(tmp, "umon")
    xemu = subprocess.Popen(
        ["xemu-xmega65", "-headless", "-besure", "-fastboot", "-prgmode", "65",
         "-uartmon", sockname, "-screenshot", png, "-prg", os.path.abspath(prg)],
        stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        sock = None
        deadline = time.time() + 30
        while time.time() < deadline and sock is None:
            try:
                s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                s.connect(sockname)
                s.settimeout(0.3)
                sock = s
            except OSError:
                time.sleep(0.1)
        if sock is None:
            print("%s: no monitor" % name)
            return False
        started = None
        while time.time() < deadline:
            if loaded(sock, addr, want):
                started = time.time()
                break
            time.sleep(0.05)
        if started is None:
            print("%s: the program never appeared at $%04X" % (name, addr))
            return False
        time.sleep(max(0, started + DELAY - time.time()))
        sock.close()
        xemu.send_signal(signal.SIGTERM)
        xemu.wait(timeout=15)
    finally:
        if xemu.poll() is None:
            xemu.kill()
        try:
            os.remove(sockname)
            os.rmdir(tmp)
        except OSError:
            pass
    ok = os.path.exists(png) and os.path.getsize(png) > 0
    print("%s: %s" % (name, png if ok else "no screenshot"))
    return ok


def main():
    os.makedirs(OUT, exist_ok=True)
    names = sys.argv[1:] or list(EXAMPLES)
    bad = 0
    for n in names:
        if not capture(n, EXAMPLES[n]):
            bad += 1
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
