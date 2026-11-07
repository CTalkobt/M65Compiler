1 rem test_basic_compile8_xemu -- basic45 --compile (#369): run compiled and interpreted (the
2 rem same results), an empty d81 as drive 8. results at $c0c0 (49344): 1 = pass
3 rem [0] line input# (commas, colons, spaces kept); [1] line input (keys in the keyboard buffer);
4 rem [2] fwrite# / fread# (screen memory, far memory); [3] bsave / bload (p, to p, b, far, the
5 rem file's own address); [4] bverify (no message when equal), ?file not found, bsave p10 to p5;
6 rem [5] dir / catalog (the lines on the screen); [6] dir w (columns); [7] $aa
10 trap 900
20 dopen#1,"l",w:print#1,"  a, b: c":print#1,"second":dclose
21 dopen#1,"l":line input#1,a$,b$:dclose
22 if a$="  a, b: c" and b$="second" and e=0 then poke 49344,1
30 poke 688,32:poke 689,65:poke 690,44:poke 691,58:poke 692,13:poke 208,5:line input "";c$
31 if c$=" a,:" then poke 49345,1
40 for i=0 to 9:poke 3800+i,65+i:next
41 dopen#2,"bin",w:fwrite#2,3800,10:dclose
42 for i=0 to 9:poke 3800+i,0:next
43 dopen#2,"bin":fread#2,3802,6:dclose
44 dopen#2,"bin":fread#2,$40000,10:dclose
45 if peek(3802)=65 and peek(3807)=70 and peek(3808)=0 and peek($40003)=68 and wpeek($40008)=19017 then poke 49346,1
50 for i=0 to 15:poke 3500+i,97+i:poke $40020+i,48+i:next
51 bsave "b1",p3500 to p3510:bload "b1",p3520
52 bsave "b2",p($40020) to p($40028):bload "b2",p3540
53 bsave "b3",b4,p($22) to p($26):bload "b3",p3560
54 for i=0 to 3:poke $50000+i,0:next:bload "b1",b5,p0
55 for i=0 to 3:poke 3500+i,0:next:bload "b1"
56 if peek(3520)=97 and peek(3529)=106 and peek(3530)=32 and peek(3540)=48 and peek(3547)=55 then 58
57 goto 60
58 if peek(3560)=50 and peek(3563)=53 and peek($50001)=98 and peek(3500)=97 and e=0 then poke 49347,1
60 bverify "b1",p3500:bload "nofile":r1=r:bsave "b4",p10 to p5:r2=r
61 if r1=4 and r2=14 and e=2 then poke 49348,1
70 e=0:print chr$(147);:catalog "b*"
71 if peek(2048+160+6)=2 and peek(2048+160+7)=49 and peek(2048+400)=48+3 and e=0 then 73
72 goto 80
73 print chr$(147);:dir "q*":if peek(2048+80+5)=2 and e=0 then poke 49349,1
80 for i=1 to 25:dopen#1,"f"+str$(i),w:dclose:next
81 print chr$(147);:dir w:if peek(2048+80+5)=34 and peek(2048+80+27+5)=34 and peek(2048+80+54+5)<>34 then 83
82 goto 90
83 if peek(2048+21*80+27+5)=32 and peek(2048+21*80+5)=34 and e=0 then poke 49350,1
90 print chr$(147);:poke 49351,170:end
900 e=e+1:r=er:resume next
