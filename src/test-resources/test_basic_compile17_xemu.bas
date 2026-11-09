1 rem test_basic_compile17_xemu -- basic45 --compile (#369): run compiled and interpreted (the
2 rem same results; the values are the interpreter's). results at $c0c0 (49344): 1 = pass
3 rem [0] play's length (four quarter notes: 2 s), rplay; [1] voices of different lengths,
4 rem tempo; [2] envelope (the rom's tables); [3] vol, filter (play's copies); [4] l: rplay
5 rem stays 1, play alone stops it and puts the defaults back; [5] the ranges (?illegal
6 rem quantity); [6] dots, sharps, flats, rests, octaves, an empty voice; [7] $aa
10 trap 900
20 t=ti:play "t0o4qcdef":do while rplay(1):loop:d=ti-t:if d>1.98 and d<2.05 then poke 49344,1
30 tempo 24:t=ti:play "icdef","o5wc":do while rplay(1):loop:a=ti-t:b=rplay(2):do while rplay(2):loop:c=ti-t
35 if a>0.48 and a<0.54 and b=1 and c>0.98 and c<1.04 then poke 49345,1
40 envelope 3,2,3,4,5,2,1000:envelope 9,,,,,4:envelope 5,1,2
45 if peek($128c)=35 and peek($1296)=69 and peek($12a0)=65 and peek($12aa)=232 and peek($12b4)=3 and peek($12a6)=21 then poke 49347-1,1
50 vol 9:vol ,3:vol 4,12:filter 1,1500,1,0,1,9:filter 2,,,1
55 s=peek($127b)+peek($127f)*2+peek($1278)*4+peek($1279)*8+peek($127a)*16+peek($127e)*32:if s=4852 then poke 49347,1
60 play "o4scdel":sleep 1:a=rplay(1):play:if a=1 and rplay(1)=0 and peek($1273)=12 and peek($128c)=5 then poke 49348,1
70 e=0:x=rplay(0):x=rplay(7):tempo 0:envelope 10:filter 3:vol 16:if e=6 and ee=14 then poke 49349,1
80 tempo 48:t=ti:play "o2i.c#d$e.rro6b","","q.o1c$c#b":do while rplay(3):loop:d=ti-t:if d>0.4 and d<0.48 then poke 49350,1
90 poke 49351,170:end
900 e=e+1:ee=er:resume next
