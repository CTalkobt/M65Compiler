# text_scroller

Horizontal text scroller demonstrating mixed C and assembly (arguments through shared variables, so both calling conventions work), with a bouncing ball (hardware sprite 0 through `<sprites.h>`) that passes in front of and behind the reverse-video message.

```
make              Build scroller.prg
make clean        Remove build artifacts
make run          Build and run in xemu (if available)
```

More, and its screen: [doc/examples](../../../doc/examples/README.md#text_scroller).
