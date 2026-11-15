1 rem test_basic_compile28_xemu -- basic45 --compile --bank=n (#580): the arrays and the
2 rem string variables in bank 1, 4 or 5. results at $c0c0 (49344): 1 = pass. [0] float and
3 rem integer arrays; [1] a string array, concatenated; [2] two dimensions; [3] string
4 rem variables copied and changed; [4] arrays made by their use (11 elements); [5] 45 kb of
5 rem floats and 2001 strings (more than the heap in bank 0 holds); [6] strings assigned
6 rem 3000 times (the old ones freed); [7] string elements in two dimensions, swapped;
7 rem [8] clr, the arrays made again; [9] bad subscript, redimensioned array; [10] $aa
10 trap 900
20 dim a(100):dim b%(50):dim c$(10):dim m(5,6)
21 for i=0 to 100:a(i)=i*1.5:next:for i=0 to 50:b%(i)=i*3:next
22 s=0:for i=0 to 100:s=s+a(i):next:t=0:for i=0 to 50:t=t+b%(i):next
23 if s=7575 and t=3825 then poke 49344,1
30 for i=0 to 10:c$(i)="s"+str$(i):next:x$="":for i=0 to 10:x$=x$+c$(i):next
31 if x$="s 0s 1s 2s 3s 4s 5s 6s 7s 8s 9s 10" and len(x$)=34 then poke 49345,1
40 for i=0 to 5:for j=0 to 6:m(i,j)=i*10+j:next:next
41 if m(5,6)=56 and m(0,6)=6 and m(3,0)=30 then poke 49346,1
50 a$="hello":b$=a$:a$=a$+" there":b$=b$+"!":if a$="hello there" and b$="hello!" then poke 49347,1
60 q(3)=7:q$(4)="z":if q(3)=7 and q(10)=0 and q$(4)="z" and q$(0)="" then poke 49348,1
70 dim g(9000):g(9000)=3:g(0)=1:dim h$(2000):h$(2000)="end"
71 for i=0 to 9000 step 100:g(i)=i:next:s=0:for i=0 to 9000 step 100:s=s+g(i):next
72 if s=409500 and h$(2000)="end" and h$(1999)="" then poke 49349,1
80 for i=1 to 3000:r$="0123456789012345678901234567890123456789"+str$(i):c$(i and 7)=r$+r$:next
81 if r$="0123456789012345678901234567890123456789 3000" and len(c$(0))=90 then poke 49350,1
90 dim w$(3,3):for i=0 to 3:for j=0 to 3:w$(i,j)=chr$(65+i)+chr$(48+j):next:next
91 z$=w$(1,2):w$(1,2)=w$(2,1):w$(2,1)=z$:if w$(1,2)="c1" and w$(2,1)="b2" and w$(3,3)="d3" then poke 49351,1
100 clr:trap 900:dim a(5):dim c$(3):a(5)=2:c$(3)="x":if a$="" and a(5)=2 and a(0)=0 and c$(3)="x" and c$(0)="" then poke 49352,1
110 p=1:a(6)=1
120 p=2:dim a(7)
130 if e1=18 and e2=19 then poke 49353,1
140 poke 49354,170
150 end
900 if p=1 then e1=er
910 if p=2 then e2=er
920 resume next
