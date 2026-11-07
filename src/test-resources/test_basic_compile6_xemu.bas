1 rem test_basic_compile6_xemu -- basic45 --compile (#369): chaining. this program (rem pragma
2 rem export) dloads test_basic_compile6b_xemu.bas ("bc6b" on the d81), which checks the variables
3 rem it imports. run compiled and interpreted (the interpreter keeps them itself). results at
4 rem $c0c0 (49344): [4] here: a load of a missing file trapped (?file not found); [0]-[3], [5] bc6b's
5 rem pragma export
10 a$="chained":b=1.5:c%=-7:n=42:z$="":p=0
20 trap 100:load "nothere",8
30 if e=4 then poke 49348,1
40 dload "bc6b"
50 print "not reached":end
100 e=er:resume next
