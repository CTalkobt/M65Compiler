# glamlambs

Glam Lambs, a sheep makeover game.

```
make              Build glamlambs.prg and glamlambs.d81 (the program,
                  GLAMMUSIC, the music, which the game loads into bank 5,
                  and GLAMSALON / GLAMSALON2, the salon's picture: banks 4, 5)
make demo         Build glamdemo.prg and glamdemo.d81, which play by themselves
make demo-studio  The same playing the styling studio (glamstudio: tests)
make gfx          Regenerate gfx.h (gen_gfx.py)
make salon        Regenerate salon.h, salon1.dat, salon2.dat (gen_salon.py, PIL)
make music        Play the music on the PC (tune45 play)
make clean        Remove build artifacts
make run          Run in xemu with the disk in drive 8
```

More, and its screen: [doc/examples](../../../doc/examples/README.md#glamlambs).
