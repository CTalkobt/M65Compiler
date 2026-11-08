1 rem test_basic_compile12_xemu -- basic45 --compile (#369): run compiled and interpreted (the
2 rem same results; the values are the interpreter's). results at $c0c0 (49344): 1 = pass
3 rem [0] circle; [1] circle solid; [2] ellipse sector 0-90 (legs); [3] ellipse solid sector 45-180;
4 rem [4] polygon; [5] polygon solid (triangles), palette of a screen; [6] the bitplanes' bytes
5 rem (sums); [7] $aa
10 trap 900:dim v(40):n=0
20 screen 0,320,200,2:pen 1:circle 60,60,30:v(n)=pixel(90,60):n=n+1:v(n)=pixel(60,30):n=n+1:v(n)=pixel(60,60):n=n+1
30 pen 2:circle 160,60,25,1:v(n)=pixel(160,60):n=n+1:v(n)=pixel(184,60):n=n+1:v(n)=pixel(186,60):n=n+1
40 pen 3:ellipse 260,60,40,20,0,0,90:v(n)=pixel(300,60):n=n+1:v(n)=pixel(260,80):n=n+1:v(n)=pixel(260,40):n=n+1:v(n)=pixel(270,60):n=n+1
50 pen 1:ellipse 60,150,40,30,1,45,180:v(n)=pixel(50,160):n=n+1:v(n)=pixel(70,140):n=n+1
60 pen 2:polygon 160,150,40,40,6:v(n)=pixel(200,150):n=n+1:v(n)=pixel(160,150):n=n+1
70 pen 3:polygon 260,150,40,30,5,0,0,90,1:v(n)=pixel(260,150):n=n+1:v(n)=pixel(260,121):n=n+1
80 palette 0,2,9,8,7:v(n)=rpalette(0,2,0)+16*rpalette(0,2,1):n=n+1:v(n)=rpalette(0,2,2):n=n+1
90 s=0:for i=0 to 7999 step 97:s=s+peek($40000+i):next:v(n)=s and 255:n=n+1:s=0:for i=0 to 7999 step 89:s=s+peek($50000+i):next:v(n)=s and 255:n=n+1
100 screen close:v(n)=e:n=n+1
110 restore 200:for i=0 to n-1:read x:if v(i)<>x then 130
120 next:goto 140
130 print "value";i;v(i);x:goto 190
140 if v(0)=1 and v(1)=1 and v(2)=0 then poke 49344,1
150 if v(3)=2 and v(4)=2 and v(5)=0 then poke 49345,1
160 if v(6)=3 and v(7)=3 and v(8)=0 and v(9)=3 then poke 49346,1
165 if v(10)=1 and v(11)=0 then poke 49347,1
170 if v(12)=2 and v(13)=0 then poke 49348,1
175 if v(14)=3 and v(15)=3 and v(16)=137 and v(17)=7 then poke 49349,1
180 if v(18)=121 and v(19)=253 and v(20)=0 then poke 49350,1
190 poke 49351,170:end
200 data 1,1,0,2,2,0,3,3,0,3,1,0,2,0,3,3,137,7,121,253,0
900 e=e+1:resume next
