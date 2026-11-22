1 rem test_basic_compile_fns_xemu -- basic45 --compile (#693): log10, log2, hasbit, rdisk, rsprsys.
2 rem compiled only: the rom xemu has (v920413) lacks all of them but log10; the values expected are
3 rem the interpreter's (its str$ of log10 and of log(x)*1.4426950408, log2 in b65.src), or b65.src's.
4 rem an empty d81 as drive 8. results at $c0c0 (49344): 1 = pass
5 rem [0] log10; [1] log2; [2] hasbit (bank 0, far, in an if); [3] ?illegal quantity: hasbit bit 8,
6 rem rdisk(3), rsprsys(1), log10(0), log2(-1); [4] rdisk: bsave (0, as the kernal's save leaves it),
7 rem bload's bytes, unit, secondary address (the kernal's: open's 15 or $60), the argument's low
8 rem byte, 40000 bytes read negative; [5] rsprsys ($d054 bit 4); [6] $aa
10 trap 900
20 a$=str$(log10(2))+str$(log10(1000))+str$(log10(.5)):x=1000
21 if a$=" 0.30103 3-0.30103" and log10(x)=log(x)*.4342944819 then poke 49344,1
30 a$=str$(log2(8))+str$(log2(10))+str$(log2(.25)):x=8
31 if a$=" 3 3.3219281-2" and log2(x)=log(x)*1.4426950408 then poke 49345,1
40 poke 3800,165:poke $40000,2:n=7:b=0:if hasbit(3800,2) then b=1
41 if hasbit(3800,0)=-1 and hasbit(3800,1)=0 and hasbit(3800,n)=-1 and hasbit(3800,6)=0 and hasbit($40000,1)=-1 and b=1 then 43
42 goto 50
43 if hasbit($40000,0)=0 then poke 49346,1
50 c=0:r=0:x=hasbit(3800,8):if r=14 then c=c+1
51 r=0:x=rdisk(3):if r=14 then c=c+1
52 r=0:x=rsprsys(1):if r=14 then c=c+1
53 r=0:x=log10(0):if r=14 then c=c+1
54 r=0:x=log2(-1):if r=14 then c=c+1
55 if c=5 then poke 49347,1
60 bsave "r1",p3500 to p3600:a=rdisk(0)
61 bload "r1",p3700:b=rdisk(0):u=rdisk(1)
62 open 15,8,15:close 15:s=rdisk(2):c=rdisk(256):k=rdisk(1.5)
63 bsave "r2",b4,p0 to p40000:bload "r2",b4,p0:d=rdisk(0)
64 if a=0 and b=100 and u=8 and s=111 and c=100 and k=8 and d=-25536 then poke 49348,1
70 x=peek($d054):poke $d054,x and 239:a=rsprsys(0):poke $d054,x or 16:b=rsprsys(0):poke $d054,x
71 if a=0 and b=1 then poke 49349,1
80 poke 49350,170
90 end
900 r=er:resume next
