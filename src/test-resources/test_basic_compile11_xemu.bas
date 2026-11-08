1 rem test_basic_compile11_xemu -- basic45 --compile (#369): run compiled and interpreted (the
2 rem same results; the values are the interpreter's). results at $c0c0 (49344): 1 = pass
3 rem [0] screen w, h, d, pen, line, dot (its colour pen 0's), box (solid); [1] dmode complement;
4 rem [2] dmode jam2 with dpat (planes not in pen 0 left); [3] lines clipped; 640x400 (interlaced);
5 rem [4] rgraphic, rpen, rpalette; [5] the bitplane bytes (a sum); [6] ?no graphics area; [7] $aa
10 trap 900:dim v(40):n=0
20 screen 0,320,200,3:pen 1:line 10,10,100,40:dot 50,50,6:pen 0,5:box 20,60,80,90,1
30 v(n)=pixel(10,10):n=n+1:v(n)=pixel(55,25):n=n+1:v(n)=pixel(50,50):n=n+1:v(n)=pixel(30,70):n=n+1:v(n)=pixel(81,91):n=n+1
40 dmode 0,1,0,0,1:box 25,65,40,80,1:v(n)=pixel(30,70):n=n+1:v(n)=pixel(24,64):n=n+1:dmode 0,0,0,0,1
50 dmode 1,0,0,1,1:dpat 2:pen 1,2:line 0,150,30,150:for i=0 to 7:v(n)=pixel(i,150):n=n+1:next:dmode 0,0,0,0,1
60 pen 0,7:line -40,-20,400,250:v(n)=pixel(0,0)+8*pixel(319,199)+64*pixel(160,100):n=n+1
70 screen def 1,1,1,2:screen open 1:screen set 1,0:pen 3:line 0,0,639,399:v(n)=pixel(1,1)+4*pixel(2,2)+16*pixel(639,399):n=n+1
80 v(n)=rgraphic(1,0)+2*rgraphic(1,1)+4*rgraphic(1,2)+8*rgraphic(1,3):n=n+1:v(n)=rgraphic(1,4):n=n+1:v(n)=rgraphic(0,7)*10+rgraphic(1,8):n=n+1
90 v(n)=rpen(0):n=n+1:v(n)=rpalette(0,1,0)+16*rpalette(0,1,1):n=n+1:s=0:for i=0 to 63:s=s+peek($40000+i):next:v(n)=s and 255:n=n+1
100 screen close 1:screen close 0:e=0:line 0,0,1,1:box 0,0,1,1:pen 1:v(n)=e:n=n+1
110 restore 200:for i=0 to n-1:read x:if v(i)<>x then 130
120 next:goto 140
130 print "value";i;v(i);x:goto 190
140 if v(0)+v(1)+v(2)+v(3)+v(4)=13 then poke 49344,1
150 if v(5)=0 and v(6)=5 then poke 49345,1
160 if v(7)=5 and v(9)=0 and v(11)=5 and v(14)=0 then poke 49346,1
170 if v(15)=0 and v(16)=51 then poke 49347,1
180 if v(17)=23 and v(18)=3 and v(19)=10 and v(20)=3 and v(21)=255 then poke 49348,1
185 if v(22)=248 then poke 49349,1
186 if v(23)=3 then poke 49350,1
190 poke 49351,170:end
200 data 1,1,6,5,0,0,5,5,5,0,0,5,5,0,0,0,51,23,3,10,3,255,248,3
900 e=e+1:resume next
