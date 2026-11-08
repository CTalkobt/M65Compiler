1 rem test_basic_compile13_xemu -- basic45 --compile (#369): run compiled and interpreted (the
2 rem same results; the values are the interpreter's). results at $c0c0 (49344): 1 = pass
3 rem [0] paint (mode 0, 1, an empty start); [1] viewport clr; [2] gcopy, paste; [3] cut, paste;
4 rem [4] char (a row's bytes), char reversed and scaled; [5] the bitplanes' bytes (a sum);
5 rem [6] no errors; [7] $aa
10 trap 900:dim v(40):n=0
20 screen 0,320,200,2:pen 1:box 10,10,100,80:circle 200,50,30
30 pen 2:paint 50,40:paint 200,50,1,1:v(n)=pixel(50,40):n=n+1:v(n)=pixel(200,50):n=n+1:v(n)=pixel(5,5):n=n+1
40 pen 3:paint 150,150,0:v(n)=pixel(150,150):n=n+1:v(n)=pixel(50,40):n=n+1
50 viewport 20,100,60,40:viewport clr:v(n)=pixel(20,100):n=n+1:v(n)=pixel(79,139):n=n+1:v(n)=pixel(80,140):n=n+1
60 viewport 0,0,320,200:pen 1:gcopy 5,5,40,30:paste 203,153:v(n)=pixel(205,155):n=n+1:v(n)=pixel(203+30,153+20):n=n+1
70 cut 190,40,20,20:v(n)=pixel(200,50):n=n+1:paste 3,163:v(n)=pixel(13,173):n=n+1
80 pen 2:char 1,184,1,1,2,"hi":s=0:for i=0 to 15:s=s+peek($40000+23*320+8+i):next:v(n)=s and 255:n=n+1
90 pen 1:char 10,120,2,3,2,chr$(18)+"x":v(n)=pixel(80,120):n=n+1:v(n)=pixel(103,135):n=n+1
100 s=0:for i=0 to 7999 step 61:s=s+peek($40000+i)+peek($50000+i):next:v(n)=s and 255:n=n+1:v(n)=e:n=n+1
110 restore 200:for i=0 to n-1:read x:if v(i)<>x then 130
120 next:goto 140
130 print "value";i;v(i);x:goto 190
140 if v(0)=2 and v(1)=2 and v(2)=0 and v(3)=3 and v(4)=2 then poke 49344,1
150 if v(5)=3 and v(6)=3 and v(7)=0 then poke 49345,1
160 if v(8)=3 and v(9)=2 then poke 49346,1
165 if v(10)=1 and v(11)=2 then poke 49347,1
170 if v(12)=140 and v(13)=1 and v(14)=1 then poke 49348,1
175 if v(15)=235 then poke 49349,1
180 if v(16)=0 then poke 49350,1
190 poke 49351,170:end
200 data 2,2,0,3,2,3,3,0,3,2,1,2,140,1,1,235,0
900 e=e+1:resume next
