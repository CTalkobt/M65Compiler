1 rem test_basic_compile_sad_xemu -- basic45 --compile (#369): a coordinate (basic's getsad:
2 rem rounded down, its low 16 bits) of a constant computed when compiled, of an integer as it is,
3 rem of a float at run time: the same positions as the interpreter's. rsppos's x / y bytes at
4 rem $c0c0 (49344) on; $aa after the last
10 a%=-3:f=257.9:k=0
20 movspr 1,70000,-1:gosub 100
30 movspr 1,65535.7,300:gosub 100
40 movspr 1,a%,a%+500:gosub 100
50 movspr 1,f,-f:gosub 100
60 movspr 1,+10,-20:gosub 100
70 poke 49344+k,170:end
100 x=rsppos(1,0):y=rsppos(1,1)
110 poke 49344+k,x and 255:poke 49345+k,int(x/256) and 255
120 poke 49346+k,y and 255:poke 49347+k,int(y/256) and 255:k=k+4:return
