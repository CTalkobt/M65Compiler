# tgi_demo

cc65's TGI (`<tgi.h>`) as a cc65 program uses it, at 320x200 in 256 colours and then 640x200 in 16.

```
make              Build tgi_demo.prg
make check        Build tgicheck.prg (both modes, 50 frames each, then reports)
make demo640      Build tgi_demo640.prg (the 640x200 part only)
make patterns     Build tgi_patterns.prg / tgi_patterns640.prg (the fill
                  patterns' page alone, at 320x200 / 640x200)
make clean        Remove build artifacts
make run          Build and run in xemu (if available)
```

More, and its screen: [doc/examples](../../../doc/examples/README.md#tgi_demo).
