1 rem test_basic_compile20_xemu -- basic45 --compile (#369): run compiled and interpreted (the
2 rem same results; the values are the interpreter's). results at $c0c0 (49344): 1 = pass
3 rem [0] sound: its duration and frequency (basic's own variables, $12be...); [1] a sweep up;
4 rem [2] oscillating (the direction and step turned); [3] down; [4] the ranges (?illegal
5 rem quantity); [5] a duration of 0, sound clr; [6] play and sound together; [7] $aa
10 trap 900
20 t=ti:sound 1,4000,50:do while (peek($12c4) and 128)=0:loop:d=ti-t
25 if d>0.98 and d<1.04 and peek($12f4)=160 and peek($12fa)=15 then poke 49344,1
30 sound 2,5000,40,0,1000,700:do while (peek($12c5) and 128)=0:loop:if peek($12f5)=216 and peek($12fb)=14 then poke 49345,1
40 sound 4,9000,60,2,3000,900,1,2000:do while (peek($12c7) and 128)=0:loop
45 if peek($12f7)=24 and peek($12fd)=21 and peek($12e5)=3 and peek($12eb)=124 and peek($12f1)=252 then poke 49346,1
50 sound 3,2000,30,1,1500,77,3:do while (peek($12c6) and 128)=0:loop:if peek($12f6)=233 and peek($12fc)=6 then poke 49347,1
60 sound 7,1,1:sound 1,1,40000:sound 1,1,1,3:sound 1,1,1,0,0,0,4:sound 1,1,1,,,,,4096:if e=5 and ee=14 then poke 49348,1
70 sound 5,3000,0:a=peek($12c8):sound clr:if a=0 and peek($12c4)=255 then poke 49349,1
80 play "o4qcd":sound 1,3000,20:do while rplay(1):loop:if (peek($12c4) and 128)<>0 then poke 49350,1
90 poke 49351,170:end
900 e=e+1:ee=er:resume next
