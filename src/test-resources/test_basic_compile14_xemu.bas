1 rem test_basic_compile14_xemu -- basic45 --compile (#369): run compiled and interpreted (the
2 rem same results; the values are the interpreter's). drive 8: an empty d81. results at $c0c0
3 rem (49344): 1 = pass. [0] saveiff, loadiff 320x200x2 (pixels, a colour); [1] the file's
4 rem bytes (a sum); [2] 640x400x4; [3] the bitplanes' bytes (a sum); [4] loadiff errors (a
5 rem sequential file, a program that is no iff); [5] a missing file, no screen; [6] the errors
6 rem trapped; [7] $aa
10 trap 900:dim v(40):n=0
20 screen 0,320,200,2:pen 1:box 10,10,100,80,1:pen 2:circle 200,100,40:pen 3:line 0,199,319,0:palette 0,2,15,3,9
30 saveiff "pic":palette 0,2,0,0,0:scnclr 0:loadiff "pic"
40 v(n)=pixel(50,40):n=n+1:v(n)=pixel(240,100):n=n+1:v(n)=pixel(5,5):n=n+1:v(n)=rpalette(0,2,0)+16*rpalette(0,2,2):n=n+1
50 open 2,8,0,"pic":s=0:for i=1 to 120:get#2,a$:s=s+asc(a$+chr$(0)):next:close 2:v(n)=s:n=n+1
60 screen close 0:screen 0,640,400,4:pen 5:box 20,20,300,200,1:pen 9:circle 400,250,90,1:saveiff "big"
70 scnclr 0:loadiff "big":v(n)=pixel(30,30):n=n+1:v(n)=pixel(400,250):n=n+1:v(n)=pixel(400,161):n=n+1
80 s=0:for i=0 to 7999 step 61:s=s+peek($40000+i)+peek($50000+i):next:v(n)=s and 255:n=n+1
90 open 2,8,2,"junk,s,w":print#2,"hello":close 2:loadiff "junk":v(n)=ee:n=n+1
100 open 2,8,2,"junkp,p,w":print#2,"hello";:close 2:loadiff "junkp":v(n)=ee:n=n+1
110 loadiff "nofile":v(n)=ee:n=n+1:v(n)=e:n=n+1
120 screen close 0:loadiff "big":v(n)=ee:n=n+1
130 if v(0)=1 and v(1)=2 and v(2)=0 and v(3)=159 then poke 49344,1
135 if v(4)=7200 then poke 49345,1
140 if v(5)=5 and v(6)=9 and v(7)=9 then poke 49346,1
145 if v(8)=72 then poke 49347,1
150 if v(9)=2 and v(10)=76 then poke 49348,1
155 if v(11)=2 and v(13)=35 then poke 49349,1
160 if v(12)=3 and e=4 then poke 49350,1
190 poke 49351,170:end
900 e=e+1:ee=er:resume next
