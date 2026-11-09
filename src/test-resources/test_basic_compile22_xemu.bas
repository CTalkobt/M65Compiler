1 rem test_basic_compile22_xemu -- basic45 --compile without --line-numbers (#483): no line
2 rem recorded. results at $c0c0 (49344): 1 = pass. [0] in the trap's handler el is 65535, er
3 rem the error's; [1] resume next goes on; [2] el still 65535 at a second error; [3] $aa
10 trap 100
20 x=1/0:if e=20 and l=65535 then poke 49344,1
30 poke 49345,1
40 dim a(3):a(9)=1:if e=18 and l=65535 then poke 49346,1
50 poke 49347,170:end
100 e=er:l=el:resume next
