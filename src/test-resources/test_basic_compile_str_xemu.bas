10 rem the string and stack run time's edges (basic_str.s45): interpreted and compiled give the same
20 r=49344:for k=0 to 11:poke r+k,0:next
30 a$="abc":a$=a$:b$="xy":b$=a$+b$:c$="1":d$=c$+(a$+b$):e$=(a$+"-")+b$
40 if a$="abc" and b$="abcxy" and d$="1abcabcxy" and e$="abc-abcxy" then poke r,1
50 h$="hello":x$=left$(h$,0)+"|"+left$(h$,9)+"|"+right$(h$,2)+"|"+right$(h$,9)+"|"+right$(h$,0)
60 if x$="|hello|lo|hello|" then poke r+1,1
70 y$=mid$(h$,2,2)+"|"+mid$(h$,9)+"|"+mid$(h$,5,9)+"|"+mid$(h$,2)+"|"+mid$(h$,1,0)
80 if y$="el||o|ello|" then poke r+2,1
90 n=0:if "ab"<"abc" then n=n+1
100 if "abc">"ab" then n=n+1
110 if "b">"abc" then n=n+1
120 if ""<"a" and "ab"="ab" and not ("ab"<"ab") and "abd">"abc" then n=n+1
130 if n=4 then poke r+3,1
140 s$="":for i=1 to 255:s$=s$+chr$(48+(i and 7)):next:if len(s$)=255 and right$(s$,3)="567" then poke r+4,1
150 trap 600:t$=s$+"y":goto 170
160 trap:goto 180
170 trap:goto 180
180 z=0:for i=1 to 3:for j=1 to 2:z=z+1:next j,i:for i=1 to 2:for j=1 to 2:z=z+10:next:next
190 for i=1 to 4:for i=1 to 2:z=z+100:next i
200 if z=246 then poke r+6,1
210 w=0:do:w=w+1:do:w=w+10:exit:loop:if w>40 then exit
220 loop:if w=44 then poke r+7,1
230 dim z$(5),q$(2,2):for i=0 to 5:z$(i)=chr$(65+i)+"x":next:q$(1,2)=z$(3)+z$(4):clr:r=49344
240 dim z$(5):z$(2)="q":if z$(2)="q" and z$(1)="" then poke r+8,1
250 if str$(12.5)=" 12.5" and str$(-3)="-3" and chr$(65)="A" and len(chr$(0))=1 then poke r+9,1
260 gosub 700:if g=1 then poke r+10,1
270 poke r+11,170:end
600 if er=23 then poke r+5,1
610 resume 160
700 for k=1 to 3:if k=2 then g=1:return
710 next k:return
