1 rem test_basic_compile9_xemu -- basic45 --compile (#369): run compiled and interpreted (the
2 rem same results). results at $c0c0 (49344): 1 = pass
3 rem [0] sprite (each value set as read, ",," skipped, ?illegal quantity after those before),
4 rem rsprite, sprite clr; [1] sprcolor, rspcolor; [2] movspr x, y (+ / - relative, a negative
5 rem number's 16 bits), rsppos; [3] movspr ... to (stops at the target; + / - after to: from the
6 rem last target), speeds 0 (?division by zero), 200 (?illegal quantity), -1; [4] movspr
7 rem angle # speed; [5] sprsav (string, sprite, string variable; 64 characters); [6] collision
8 rem (a gosub at the next statement; off), bump; [7] $aa
10 trap 900
20 sprite 2,1,5,1,0,1,0:sprite 2,,,0,1:e=0:sprite 2,0,7,2,1:r1=r
21 a=rsprite(2,0)+2*rsprite(2,2)+4*rsprite(2,3)+8*rsprite(2,4)+16*rsprite(2,5)
22 if a=4+8 and rsprite(2,1)=7 and peek(53288+1)=7 and r1=14 and e=1 then 24
23 goto 30
24 sprite clr:if peek(53269)=0 and peek(53271)=0 and peek(53277)=0 and rsprite(2,1)=7 then poke 49344,1
30 e=0:sprcolor 3,12:sprcolor ,9:a=rspcolor(1):b=rspcolor(2):sprcolor 16:r1=r:c=rspcolor(0):r2=r
31 if a=3 and b=9 and r1=14 and r2=14 and e=2 then poke 49345,1
40 e=0:movspr 1,300,200:x1=rsppos(1,0):y1=rsppos(1,1):movspr 1,+10,-20:x2=rsppos(1,0):y2=rsppos(1,1)
41 movspr 1,(-200),250:x3=rsppos(1,0):p=peek(53264)and 2:movspr 8,1,1:r1=r:n=1:x4=rsppos(n,n-1)
42 if x1=300 and y1=200 and x2=310 and y2=180 and x3=312 and p=2 and r1=14 and x4=312 and e=1 then 44
43 goto 50
44 if rsppos(1,2)=0 then poke 49346,1
50 e=0:sprite 3,1:movspr 3,100,100 to 140,130,4:gosub 800:a=rsppos(3,0):b=rsppos(3,1)
51 movspr 3,50,50 to +10,-10,8:gosub 800:c=rsppos(3,0):d=rsppos(3,1)
52 movspr 3,10,10 to 20,20,0:r1=r:movspr 3,10,10 to 20,20,200:r2=r:movspr 3,10,10 to 10,10,1:gosub 800
53 movspr 3,1,1 to 20,20,-1:s=rsppos(3,2):movspr 3,1,1 to 20,20,2.5:s2=rsppos(3,2):gosub 800
54 if a=140 and b=130 and c=150 and d=120 and r1=20 and r2=14 and s=-1 and s2=2.5 and e=2 then poke 49347,1
60 e=0:sprite 4,1:movspr 4,160,120:movspr 4,45#2:s=rsppos(4,2):sleep .3:x=rsppos(4,0):y=rsppos(4,1)
61 movspr 4,0#0:s2=rsppos(4,2)
62 if s=2 and x>165 and y<115 and x-160>110-y and x-160<130-y and s2=0 and e=0 then poke 49348,1
70 a$="":for i=1 to 64:a$=a$+chr$(i):next:sprsav a$,5:sprsav 5,6:sprsav 6,b$:x$="abc":sprsav 6,x$
71 sprsav "abc",1:r1=r:sprsav 9,1:r2=r:sprsav 1,8:r3=r
72 if len(b$)=64 and b$=a$ and x$=a$ and peek(1536+6*64+63)=64 and r1=14 and r2=14 and r3=14 and e=3 then poke 49349,1
80 e=0:for i=0 to 63:poke 1536+i,255:poke 1600+i,255:next:sprite clr:i=bump(1)
81 sprite 0,1:sprite 1,1:movspr 0,100,100:movspr 1,108,104:collision 1,700:sleep .3
82 c1=c:b=bump(1):collision 1:c0=c:sleep .3:c2=c:sprite clr
83 if c1>=1 and b=3 and c2=c0 and bump(1)=3 and e=0 then poke 49350,1
90 sprite clr:poke 49351,170:end
700 c=c+1:return
800 for w=1 to 100:if rsppos(3,2)<>0 then sleep .02:next
810 return
900 e=e+1:r=er:resume next
