1 rem test_basic_compile18_xemu -- basic45 --compile (#369): run compiled and interpreted (the
2 rem same results; the values are the interpreter's). results at $c0c0 (49344): 1 = pass
3 rem [0] print using (strings, numbers, rounding, *s, the % fields, hex: the screen's codes);
4 rem [1] fgoto, fgosub (computed, ?undef'd statement); [2] run line, run (cleared; the flag
5 rem at $50000 kept); [3] key n, key off / on, key restore, the listing; [4] decbin; [5] char
6 rem def, font; [6] the ranges; [7] $aa
10 trap 900:f=peek($50000):if f=1 then 300
12 if f=2 then 400
20 scnclr:print using "####";"ab":print using ">####";"ab":print using "=######";"abc"
22 print using "###.##";3.14159:print using "###.##";-2.5:print using "##";123:print using "#.#";9.96
24 print using "%5D";42:print using "%05D";42:print using "%8.3F";-1.23456:print using "%4X";255
26 print using "%8X";-1:print using "##.###";1e-5:print using "x=##.#,";5.55,"z"
30 s=0:for i=0 to 13*80-1:s=s+peek(2048+i):next:print s
35 if s=33362 then poke 49344,1
40 n=4:fgosub n*10+100:fgoto 1000/20
45 print "no":end
50 if a=7 then poke 49345,1
55 a=1:poke $50000,1:run 10
140 a=7:return
300 if a=0 then poke $50000,2:run
310 end
400 poke $50000,0:if a=0 then poke 49346,1
410 key 3,"ab"+chr$(13)+"c":k1=peek(4098):key off:k2=peek(247) and 32:key on:k3=peek(247) and 32
420 key restore:k4=peek(4098):if k1=4 and k2=32 and k3=0 and k4=4 then poke 49347,1
430 if decbin("1011")=11 and decbin("")=0 and decbin("12101")=1 then poke 49348,1
440 char def 65,255,129,129,129,129,129,129,255:c1=peek($ff7e000+520)+peek($ff7e000+527)
450 font b:c2=peek($ff7e000+8):font e:c3=peek($ff7e000+9):font a
460 if c1=510 and c2=24 and c3=60 then poke 49349,1
470 e=0:char def 600,1:fgoto 77:key 17,"z":print using "";1:if e=4 then poke 49350,1
480 poke 49351,170:end
900 e=e+1:ee=er:resume next
