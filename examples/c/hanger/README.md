# hanger

Hangman with a different victim on the gallows each level.

```
make              Build hanger.prg (tune45 convert makes sounds.c of sounds.txt)
make demo         Build hangerdemo.prg, which plays by itself (DEMO_LEVEL=0-5: the first level)
make gfx          Regenerate gfx.h and words.h (gen_gfx.py; needs Python's PIL)
make preview      preview.png: every stage of every animal
make sounds       Play the sounds on the PC (tune45 play)
make clean        Remove build artifacts
make run          Build and run in xemu (if available)
```

More, and its screen: [doc/examples](../../../doc/examples/README.md#hanger).
