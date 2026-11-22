# drums

A SID band and a sampled drum kit, both from one `music.txt` by the library's tune player.

```
make              Build drums.prg (tune45 convert makes music.c of music.txt,
                  the samples too)
make check        Build drumscheck.prg (plays 240 frames, reports)
make play         Play the music on the PC (tune45 play)
make clean        Remove build artifacts
make run          Build and run in xemu (if available)
```

More, and its screen: [doc/examples](../../../doc/examples/README.md#drums).
