# far_scroller

A 400x25 landscape map allocated with `far_malloc` in bank 4, scrolled with one `dma_copy_far` per row each frame (parallax sky), colours written straight into colour RAM at `$FF80000` with `dma_fill_far`.

```
make              Build far_scroller.prg
make clean        Remove build artifacts
make run          Build and run in xemu (if available)
```

More, and its screen: [doc/examples](../../../doc/examples/README.md#far_scroller).
