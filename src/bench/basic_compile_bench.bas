1 rem basic_compile_bench -- basic45 --compile's speed (#369): seven loops, each timed in frames
2 rem (the vic-iv's frame counter $d7fa, its turns over 256 from ti). results at $c0c0 (49344),
3 rem two bytes a loop (low, high), then $aa. build it interpreted (basic45), compiled
4 rem (--compile -O2) and with --speed; run in xemu with -dumpmem. doc/bin/basic45.md has the table.
10 a=49344:dim f%(3000):dim a(500)
100 gosub 890:for r=1 to 5:s=0:for i=1 to 3000:s=s+i*2:next i:next r:gosub 900
110 gosub 890:for r=1 to 5:s=1:x=.5:for i=1 to 3000:s=s*.999+x/3-x*x:next i:next r:gosub 900
120 gosub 890:for r=1 to 10:n%=0:c%=0:for i=1 to 3000:n%=n%+1:c%=c%+(n% and 1):next i:next r:gosub 900
130 gosub 890:for r=1 to 10:for i=1 to 300:a$=left$("hello world",5)+chr$(65+(i and 15))+"x":next i:next r:gosub 900
140 gosub 890:for r=1 to 10:x=0:for i=1 to 300:gosub 800:next i:next r:gosub 900
150 gosub 890:for r=1 to 5:for i=2 to 3000:f%(i)=1:next i
151 for i=2 to 54:if f%(i)=0 then 153
152 for j=i*i to 3000 step i:f%(j)=0:next j
153 next i:c=0:for i=2 to 3000:c=c+f%(i):next i:next r:gosub 900
160 gosub 890:for r=1 to 5:for k=1 to 5:for i=0 to 500:a(i)=a(i)+i:next i:next k:next r:gosub 900
170 poke a,170:end
800 x=x+1:return
890 t=peek(55290):u=ti:return
900 d=(peek(55290)-t) and 255:e=(ti-u)*50:d=d+256*int((e-d)/256+.5)
910 poke a,d and 255:poke a+1,int(d/256):a=a+2:return
