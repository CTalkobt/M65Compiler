1 rem test_basic_compile_import_xemu -- basic45 --compile (#369): import in a program is
2 rem ?direct mode only (34) when it runs, as the interpreter's; freezer compiles (not run:
3 rem the freezer would open). results at $c0c0 (49344): 1, er, $aa
10 poke 49344,1
20 if peek(49344)=2 then freezer
30 trap 100:import "x"
40 poke 49345,99:end
100 poke 49345,er:poke 49346,170
