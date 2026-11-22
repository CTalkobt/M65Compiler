#!/usr/bin/env python3
"""test_guides.py -- the tutorials in doc/guide/ (#694, #695, #696), run as written.

Each page is read top to bottom in a directory of its own (build/test/guides/<page>/),
with bin/ first on the PATH and HOME there too (no personal configuration). HTML
comments before a fenced block say what to do with it; blocks without one are only
shown (commands for real hardware, xemu's window, an editor):

  <!-- guide: file NAME -->    the block is written to NAME (directories made)
  <!-- guide: run -->          a console block: each "$ command" line is run with bash
                               and must succeed; the lines after it (up to the next
                               "$") are its output as shown, and each must appear in
                               what the command printed (runs of spaces compared as
                               one; "..." in a line stands for text left out: the
                               pieces around it must appear in order). "$ cd DIR"
                               alone changes the directory for the commands after it
  <!-- guide: run xemu -->     as run, skipped without xemu (dbg45)
  <!-- guide: fails -->        as run, but each command must fail (a non-zero status)
  <!-- guide: xemu PRG [disk=IMAGE] screen=TEXT -->
                               no block: PRG run headless in xemu (IMAGE as drive 8),
                               its screen must show TEXT (the rest of the comment)

Without xemu-xmega65 on the PATH the xemu checks are skipped (and said so).
Usage: test_guides.py [page.md ...]   (default: every doc/guide/*.md but README.md)
"""
import os
import re
import shutil
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "../.."))
GUIDE = os.path.join(ROOT, "doc", "guide")
OUT = os.path.join(ROOT, "build", "test", "guides")
XEMU = shutil.which("xemu-xmega65")
MARK = re.compile(r"^\s*<!--\s*guide:\s*(.*?)\s*-->\s*$")

failures = 0
passes = 0


def fail(page, line, msg):
    global failures
    failures += 1
    print(f"  FAIL: {page}:{line}: {msg}")


def norm(s):
    return " ".join(s.split())


def blocks(lines):
    """(marker, marker line number, block lines or None)"""
    i = 0
    while i < len(lines):
        m = MARK.match(lines[i])
        if not m:
            i += 1
            continue
        mark, at = m.group(1), i + 1
        if mark.startswith("xemu"):
            yield mark, at, None
            i += 1
            continue
        j = i + 1
        while j < len(lines) and not lines[j].strip():
            j += 1
        if j >= len(lines) or not lines[j].lstrip().startswith("```"):
            yield mark, at, []
            i = j
            continue
        fence = lines[j].strip()[:3]
        indent = len(lines[j]) - len(lines[j].lstrip())
        k = j + 1
        body = []
        while k < len(lines) and not lines[k].strip().startswith(fence):
            body.append(lines[k][indent:] if lines[k][:indent].strip() == "" else lines[k])
            k += 1
        yield mark, at, body
        i = k + 1


def shown_ok(line, got):
    """a line of output as shown: in the output; with "...", its pieces in order"""
    if "..." not in line:
        return norm(line) in got
    pos = 0
    for piece in (norm(x) for x in line.split("...")):
        if not piece:
            continue
        pos = got.find(piece, pos)
        if pos < 0:
            return False
        pos += len(piece)
    return True


def run_console(page, at, body, state, env, expect_fail):
    global passes
    cmds = []
    for ln in body:
        if ln.startswith("$ "):
            cmds.append([ln[2:], []])
        elif cmds:
            cmds[-1][1].append(ln)
    if not cmds:
        fail(page, at, "a run block without a '$ ' command")
        return
    for cmd, shown in cmds:
        m = re.fullmatch(r"cd\s+(\S+)", cmd.strip())
        if m:
            # (a cd alone carries over to the commands after it, on the page)
            d = os.path.normpath(os.path.join(state["cwd"], m.group(1)))
            if not os.path.isdir(d):
                fail(page, at, f"'{cmd}': no such directory")
            else:
                state["cwd"] = d
                passes += 1
            continue
        p = subprocess.run(["bash", "-c", cmd], cwd=state["cwd"], env=env, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL, timeout=300)
        out = p.stdout.decode("latin-1")
        if expect_fail and p.returncode == 0:
            fail(page, at, f"'{cmd}' succeeded (it should fail)")
            continue
        if not expect_fail and p.returncode != 0:
            fail(page, at, f"'{cmd}' exit {p.returncode}:\n" + out[-1500:])
            continue
        got = norm(out)
        bad = [s for s in shown if s.strip() and not shown_ok(s, got)]
        if bad:
            fail(page, at, f"'{cmd}': not in its output: {bad[0]!r}\n--- output:\n" + out[-1500:])
            continue
        passes += 1


def pref_dir(work):
    """xemu's preferences: links to the real ones but for the two files it rewrites (xemu_pref.sh)"""
    src = os.environ.get("XEMU_PREF_DIR", os.path.expanduser("~/.local/share/xemu-lgb/mega65"))
    d = os.path.join(work, ".xemu-pref")
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(d)
    if os.path.isdir(src):
        for f in os.listdir(src):
            if f.endswith("-template.cfg") or f == "keymap-default.cfg":
                continue
            os.symlink(os.path.join(src, f), os.path.join(d, f))
    return d


def run_xemu(page, at, mark, work):
    global passes
    m = re.match(r"xemu\s+(\S+)((?:\s+(?:disk)=\S+)*)\s+screen=(.*)$", mark)
    if not m:
        fail(page, at, f"bad xemu marker: {mark}")
        return
    if not XEMU:
        print(f"  SKIP: {page}:{at}: xemu-xmega65 not found")
        return
    prg, opts, text = m.group(1), dict(o.split("=", 1) for o in m.group(2).split()), m.group(3)
    screen = os.path.join(work, ".screen.txt")
    if os.path.exists(screen):
        os.remove(screen)
    args = [XEMU, "-headless", "-besure", "-sleepless", "-prgmode", "65", "-prgexit", "-dumpscreen", screen]
    if "disk" in opts:
        args += ["-8", os.path.join(work, opts["disk"])]
    args += ["-prg", os.path.join(work, prg)]
    env = dict(os.environ, XEMU_PREF_DIR=pref_dir(work))
    try:
        subprocess.run(args, cwd=work, env=env, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL, timeout=30)
    except subprocess.TimeoutExpired:
        pass
    shown = open(screen, encoding="latin-1").read() if os.path.exists(screen) else ""
    if norm(text) not in norm(shown):
        fail(page, at, f"{prg}: '{text}' not on the screen:\n" + shown[-1200:])
        return
    passes += 1


def run_page(path):
    page = os.path.relpath(path, ROOT)
    name = os.path.splitext(os.path.basename(path))[0]
    work = os.path.join(OUT, name)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    env = dict(os.environ)
    env["PATH"] = os.path.join(ROOT, "bin") + os.pathsep + env.get("PATH", "")
    env["HOME"] = work
    if XEMU:
        # (xemu run by a command, dbg45's: the ROM and settings from the real preferences)
        env["XEMU_PREF_DIR"] = pref_dir(work)
    for v in ("CC45_INCLUDE", "CC45_LIB", "CA45_OPTIMIZE", "CA45_WARNINGS"):
        env.pop(v, None)
    print(f"{page}:")
    lines = open(path, encoding="utf-8").read().split("\n")
    state = {"cwd": work}
    for mark, at, body in blocks(lines):
        if mark.startswith("file "):
            fn = os.path.join(work, mark[5:].strip())
            os.makedirs(os.path.dirname(fn), exist_ok=True)
            with open(fn, "w", encoding="utf-8") as f:
                f.write("\n".join(body) + "\n")
        elif mark == "run":
            run_console(page, at, body, state, env, False)
        elif mark == "run xemu":
            if XEMU:
                run_console(page, at, body, state, env, False)
            else:
                print(f"  SKIP: {page}:{at}: xemu-xmega65 not found")
        elif mark == "fails":
            run_console(page, at, body, state, env, True)
        elif mark.startswith("xemu"):
            run_xemu(page, at, mark, work)
        else:
            fail(page, at, f"unknown marker '{mark}'")


def main():
    pages = sys.argv[1:] or sorted(os.path.join(GUIDE, f) for f in os.listdir(GUIDE)
                                   if f.endswith(".md") and f != "README.md")
    for p in pages:
        run_page(os.path.abspath(p))
    print(f"Guide checks: {passes} passed, {failures} failed")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
