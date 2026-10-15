#!/usr/bin/env python3
"""Checks the songs of a musicgen -DMG_TEST build.

Usage: check_songs.py <memory dump>   (xemu -dumpmem after the test build ran)

The test build logs 14 songs (each style twice) at $A000: style, key, bars,
0, then per section 12 x (chord mask, scale mask) and 192 melody steps
(0 hold, 1 rest, else a MIDI note). Reports per style and checks:
  - every melody note is in the scale (with the chord's changes) or the chord
  - notes on beats 1 and 3 are chord tones (most of them)
  - mostly steps, no leap over an octave, the range
  - section A ends on the tonic
The blues' bars 5-6 repeat the call of bars 1-2 over the IV chord (the AAB
form): they are not checked against the IV chord.
"""
import sys

STYLES = ["pop", "rock", "blues", "jazz", "classical", "ambient", "chiptune"]
RANGES = {"pop": (62, 81), "rock": (60, 79), "blues": (60, 79), "jazz": (62, 81),
          "classical": (62, 84), "ambient": (60, 79), "chiptune": (64, 86)}

mem = open(sys.argv[1], "rb").read()
p = 0xA000
fails = 0
for song in range(14):
    style, key, nbars = mem[p], mem[p + 1], mem[p + 2]
    p += 4
    name = STYLES[style]
    lo, hi = RANGES[name]
    stats = dict(notes=0, outside=0, strong=0, strong_ct=0, steps=0, moves=0, maxleap=0, rng=0)
    tonic_end = None
    for sec in range(2):
        masks = []
        for b in range(12):
            cm = mem[p] | mem[p + 1] << 8
            sm = mem[p + 2] | mem[p + 3] << 8
            masks.append((cm, sm))
            p += 4
        mel = mem[p:p + 192]
        p += 192
        prev = None
        last = None
        for s in range(nbars * 16):
            n = mel[s]
            if n < 24:
                continue
            cm, sm = masks[s // 16]
            if name == "blues" and s // 16 in (4, 5):
                prev = last = n    # the call repeated over the IV chord, as it was
                continue
            pcl = (n - key) % 12
            stats["notes"] += 1
            if not ((sm | cm) >> pcl) & 1:
                stats["outside"] += 1
            if s % 8 == 0:
                stats["strong"] += 1
                if (cm >> pcl) & 1:
                    stats["strong_ct"] += 1
            if not lo <= n <= hi:
                stats["rng"] += 1
            if prev is not None:
                d = abs(n - prev)
                stats["moves"] += 1
                if d <= 2:
                    stats["steps"] += 1
                stats["maxleap"] = max(stats["maxleap"], d)
            prev = n
            last = n
        if sec == 0:
            tonic_end = (last - key) % 12 == 0
    ok = (stats["outside"] == 0 and stats["maxleap"] <= 12 and stats["rng"] == 0 and tonic_end
          and stats["strong_ct"] * 100 >= 80 * stats["strong"]
          and stats["steps"] * 100 >= 40 * stats["moves"])
    if not ok:
        fails += 1
    print("%-9s key %-2d %2d bars: %3d notes, %d outside the scale, strong beats %d/%d chord tones, "
          "%d%% steps, max leap %d, %d out of range, A ends on the tonic: %s  %s" % (
              name, key, nbars, stats["notes"], stats["outside"], stats["strong_ct"], stats["strong"],
              100 * stats["steps"] // max(1, stats["moves"]), stats["maxleap"], stats["rng"],
              "yes" if tonic_end else "no", "ok" if ok else "FAIL"))
print("%d of 14 songs failed" % fails)
sys.exit(1 if fails else 0)
