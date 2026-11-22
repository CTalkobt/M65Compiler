# overlays

Code overlays (`#pragma cc45 overlay(name)`).

```
make              Build ovldemo.prg, ovldemo.ov1 and ovldemo.d81
make OVERLAY_MAP=1  The same with mapped overlays (ln45 --overlay-map:
                    the window is mapped to each overlay, not copied)
make clean        Remove build artifacts
make run          Run in xemu with the disk in drive 8
```

More, and its screen: [doc/examples](../../../doc/examples/README.md#overlays).
