' test_basic45_mixed_xemu.bas -- compiled procs in an interpreted program
' (basic45 #577): proc name(...) compiled, call name(...) to v, return <expr>.
' Built as is (the compiled procs machine code, the rest interpreted) and with
' --compile (all of it compiled, a compiled proc an ordinary one): the same
' results.
'
' Results POKEd at $C0C0 (1 = pass):
'   [0] floats in, a float back: add(3, 4.5) = 7.5
'   [1] an integer parameter and an integer result: sq(-12) to n%
'   [2] a loop, block IF and a local label in a compiled proc
'   [3] a GOSUB inside a compiled proc (RETURN alone is the GOSUB's)
'   [4] the result into an array element: to x(3)
'   [5] endproc without return: 0
'   [6] a compiled proc changing its parameter (in the interpreted program
'       a copy: the caller's variable stays as the call assigned it; with
'       --compile an ordinary proc's global)
'   [7] an interpreted proc: return <expr> and call ... to v
'   [8] many calls: the sum of 1..k over k = 1..20 (1540)
'   [9] a result out of an integer's range: ?ILLEGAL QUANTITY (--checks) in
'       the compiled proc ends the program (the POKE after it never runs)
'  [10] a string parameter and a string result (#680): reversed
'  [11] a string through a compiled proc's call of another (#680, #681), and
'       the caller's variable unchanged
'  [12] a compiled proc calling compiled procs, results back (#681)
'  [13] a recursive compiled proc (#681): 7! by an accumulator
'  [14] $AA sentinel
start: a = 3 : b = 4.5
  call add(a, b) to r
  if r = 7.5 then poke 49344, 1
  call sq(-12) to n%
  if n% = 144 then poke 49345, 1
  call collatz(27) to cz
  if cz = 111 then poke 49346, 1
  call twice(21) to t
  if t = 42 then poke 49347, 1
  dim x(5) : call add(1.25, 1.75) to x(3)
  if x(3) = 3 then poke 49348, 1
  r = 5 : call nothing to r
  if r = 0 then poke 49349, 1
  call incr(10) to r
  if r = 11 and (pp = 10 or pp = 11) then poke 49350, 1
  call mean(2, 4) to r
  if r = 3 then poke 49351, 1
  s = 0
  for k = 1 to 20 : call tri(k) to t : s = s + t : next
  if s = 1540 then poke 49352, 1
  call rev("hello") to r$
  if r$ = "olleh" then poke 49354, 1
  a$ = "ab" : call wrap(a$, 3) to r$
  if r$ = "<ab:3>" and a$ = "ab" then poke 49355, 1
  call hyp(3, 4) to h
  if h = 25 then poke 49356, 1
  call fact(1, 7) to f
  if f = 5040 then poke 49357, 1
  poke 49358, 170
  poke 49353, 1 : call sq(200) to n% : poke 49353, 0
  end

proc rev(s$) compiled
  t$ = ""
  for i = 1 to len(s$) : t$ = mid$(s$, i, 1) + t$ : next
  return t$
endproc

proc wrap(x$, k) compiled
  call same(x$) to y$
  return "<" + y$ + ":" + mid$(str$(k), 2) + ">"
endproc

proc same(z$) compiled
  return z$
endproc

proc hyp(ha, hb) compiled
  call sq(ha) to s1
  call sq(hb) to s2
  return s1 + s2
endproc

' (arguments are assigned left to right: acc before fk changes)
proc fact(acc, fk) compiled
  if fk <= 1 then return acc
  call fact(acc * fk, fk - 1) to fr
  return fr
endproc

proc add(p, q) compiled
  return p + q
endproc

proc sq(v%) compiled
  return v% * v%
endproc

' the steps of n to 1 (27: 111)
proc collatz(n) compiled
  c = 0
.again:
  if n = 1 then return c
  if int(n / 2) * 2 = n then
    n = n / 2
  else
    n = 3 * n + 1
  endif
  c = c + 1
  goto .again
endproc

proc twice(w) compiled
  gosub .dbl
  return w
.dbl: w = w * 2
  return
endproc

proc nothing compiled
endproc

proc incr(pp) compiled
  pp = pp + 1
  return pp
endproc

' an interpreted proc with a result
proc mean(u, v)
  return (u + v) / 2
endproc

proc tri(m) compiled
  z = 0
  for j = 1 to m : z = z + j : next
  return z
endproc
