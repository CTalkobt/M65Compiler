# map_decode

What the CPU sees in its 64 KB for a memory configuration you set up, drawn with TGI.

```
make              Build map_decode.prg
make check        Build the tests (-DMAP_CHECK: the decoder and the drawing;
                  -DMAP_KEYS: the editor run by pressed keys), run them in
                  xemu, check $C0C0
make clean        Remove build artifacts
make run          Build and run in xemu (if available)
```

More, and its screen: [doc/examples](../../../doc/examples/README.md#map_decode).
