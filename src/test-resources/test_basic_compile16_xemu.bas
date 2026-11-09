1 rem test_basic_compile16_xemu -- basic45 --compile (#369): run compiled and interpreted (the
2 rem same results; the values are the interpreter's). results at $c0c0 (49344): 1 = pass
3 rem [0] joy, pot, lpen idle (headless: no stick, paddle or pen); [1] their ranges (?illegal
4 rem quantity); [2] rmouse without a mouse; [3] mouse on (port, sprite), rmouse; [4] the
5 rem hotspot (the driver's margins) and a position; [5] a relative position; [6] mouse off,
6 rem mouse on's ranges; [7] $aa
10 trap 900
20 s=joy(1)+joy(2)+joy(3)+lpen(0)+lpen(1):p=pot(1)+pot(2)+pot(3)+pot(4):if s=0 and p=1020 then poke 49344,1
30 x=joy(0):x=joy(4):x=pot(0):x=pot(5):x=lpen(2):if e=5 and ee=14 then poke 49345,1
40 rmouse a,b,c:if a=-1 and b=-1 and c=-1 then poke 49346,1
50 sprite 1,1:mouse on,1,1:rmouse a,,c:if a=0 and c=0 and peek(4408)=64 then poke 49347,1
60 mouse on,,3,5,7,100,120:rmouse a,b:q=peek(4412)+peek(4413)*2+peek(4414)*4+peek(4415)*8
65 if a=100 and b=120 and peek(4408)=128 and q=43+486+76+656 then poke 49348,1
70 mouse on,2,0,,,+10,-5:rmouse a,b:if a=10 and b=251 then poke 49349,1
80 e=0:mouse off:rmouse a,b,c:mouse on,4:mouse on,1,8:mouse on,1,1,24:mouse off
85 if a=-1 and c=-1 and peek(4408)=0 and e=3 and ee=14 then poke 49350,1
90 poke 49351,170:end
900 e=e+1:ee=er:resume next
