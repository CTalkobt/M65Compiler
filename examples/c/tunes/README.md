# tunes

The pfest game's music (12 tunes, its `music.txt` as it is) played by the library's tune player (`<audio_tune.h>`) from a raster interrupt, as a game plays its music.

```
make              Build tunes.prg (tune45 convert makes music.c of music.txt)
make check        Build tunescheck.prg (plays from the interrupt, reports)
make play         Play the music on the PC (tune45 play)
make clean        Remove build artifacts
make run          Build and run in xemu (if available)
```

More, and its screen: [doc/examples](../../../doc/examples/README.md#tunes).
