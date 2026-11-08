1 rem test_basic_compile10_xemu -- basic45 --compile (#369): run compiled and interpreted (the
2 rem same results). results at $c0c0 (49344): 1 = pass
3 rem [0] foreground / color (16 and up: bit 6), background, border, rcolor; [1] highlight
4 rem (errors, rems, keywords; list's flags); [2] cursor (on / off, a column, a row, a style),
5 rem rcursor; [3] window (checks; clear 2 after the window is set), rwindow; [4] speed / fast,
6 rem rspeed, fast on / off; [5] palette color (several), palette restore; [6] vsync; [7] $aa
10 trap 900:print chr$(147);
20 e=0:foreground 7:a=rcolor(1):color 20:b=rcolor(1):foreground 1:background 6:border 6:background 3:border 5
21 c=rcolor(0):d=rcolor(3):x=rcolor(4):background 6:border 6
22 if a=7 and b=68 and c=3 and d=5 and r=14 and e=1 and rcolor(0)=6 then poke 49344,1
30 e=0:h0=peek(85):highlight 4:a=rcolor(2):highlight 9,1:b=peek(680):highlight 10,2:c=peek(681)
31 highlight 11,3:d=peek(680):f=peek(681):g=peek(85)and 24:highlight 8:poke 85,h0:poke 680,13:poke 681,7
32 if a=4 and b=9 and c=10 and d=11 and f=11 and g=24 and rcolor(2)=8 and e=0 then poke 49345,1
40 e=0:cursor 10,2:rcursor c,rr:cursor ,4:rcursor c2,r2:cursor on,20:rcursor c3:cursor off
41 cursor 5,6,1:cursor 5,7,0:rcursor ,r4:cursor 200,1:x=r:print chr$(19);
42 if c=10 and rr=2 and c2=10 and r2=4 and c3=20 and r4=7 and x=14 and e=1 then poke 49346,1
50 e=0:window 2,3,30,10:a=rwindow(0):b=rwindow(1):print "ab";:rcursor c,rw
51 window 79,0,79,24:r1=r:window 10,0,5,24:r2=r:window 2,3,30,10,2:r3=r:d=rwindow(1)
52 window 0,0,79,24,1:f=rwindow(2):g=rwindow(3):x=rwindow(4)
53 if a=29 and b=8 and c=2 and rw=0 and r1=14 and r2=14 and r3=14 and d=8 and f=80 and g=25 and r=14 then 55
54 goto 60
55 if e=4 and rwindow(0)=80 and rwindow(1)=25 then poke 49347,1
60 e=0:speed 1:a=rspeed(0):fast 3:b=rspeed(9):fast 2:c=rspeed(0):speed:d=rspeed(0)
61 s0=peek(150):fast on:f=peek(150)and 2:fast off:g=peek(150)and 2:poke 150,s0
62 if a=1 and b=3 and c=40 and d=40 and f=2 and g=0 and e=0 then poke 49348,1
70 e=0:palette color 1,15,0,0,2,1,2,3:a=peek($d101):b=peek($d102)+16*peek($d202)+256*peek($d302)
71 palette color 3,32,0,0:r1=r:palette color 3,1,16,0:r2=r:palette color 3,1,1,16:r3=r
72 palette restore:c=peek($d101)+peek($d201)+peek($d301):d=peek($d102)
73 if a=15 and b=1+32+768 and r1=14 and r2=14 and r3=14 and e=3 and c=45 and d=15 then poke 49349,1
80 e=0:vsync 100:vsync 300:vsync 600:r1=r:vsync 256+56:r2=r:vsync 0
81 if r1=14 and r2=14 and e=2 then poke 49350,1
90 print chr$(147);:poke 49351,170:end
900 e=e+1:r=er:resume next
