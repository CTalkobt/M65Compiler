1 rem test_basic_compile29_xemu -- basic45 --compile --bank=n (#580): the heap in the bank
2 rem (basic_bank.s45). results at $c0c0 (49344): 1 = pass. [0] strings of every length,
3 rem 0 to 255; [1] 3000 strings of changing lengths in 50 elements (blocks freed, split
4 rem and merged); [2] clr, then one array of nearly the whole bank (every block merged
5 rem back); [3] an array with no room left: out of memory; [4] the heap still usable
6 rem after it; [5] $aa
10 trap 900
20 z$="":for i=1 to 255:z$=z$+chr$(65+(i and 15)):next:dim h$(255)
21 for i=0 to 255:h$(i)=left$(z$,i):next
22 f=1:for i=0 to 255:if len(h$(i))<>i then f=0
23 next:if f=1 and right$(h$(255),1)="p" and h$(1)="b" and h$(0)="" then poke 49344,1
30 dim t$(49):for k=1 to 3000:i=k-int(k/50)*50:l=k*37:l=l-int(l/256)*256:t$(i)=left$(z$,l):next
31 s=0:for i=0 to 49:s=s+len(t$(i)):next:if s=6563 and t$(0)=left$(z$,152) then poke 49345,1
40 clr:trap 900:dim g(10500):g(10500)=7:g(0)=3:if g(10500)=7 and g(0)=3 and g(5000)=0 then poke 49346,1
50 p=1:dim k(4000)
51 if e1=16 then poke 49347,1
60 x$="ok":k$=x$+x$:if k$="okok" and g(10500)=7 then poke 49348,1
70 poke 49349,170
80 end
900 if p=1 then e1=er
920 resume next
