' test_basic45_mixarr_xemu.bas -- array parameters of compiled procs (#683):
' proc f(a(), n) compiled / call f(x(), 21): the interpreted program's array,
' its elements read and written where they are (bank 1). Built as is and with
' --minify (--checks); not with --compile, which has no interpreted program.
'
' Results POKEd at $C0C0 (1 = pass):
'   [0] a float array summed (21 elements: 315)
'   [1] an integer array summed (negative elements too: 5500)
'   [2] a float array written by the compiled proc
'   [3] an integer array written (high byte first in BASIC's array)
'   [4] the result into an element of the array passed
'   [5] an index past the array's end: ?BAD SUBSCRIPT (--checks) ends the
'       program (the POKE after it never runs)
'   [6] $AA sentinel
start: dim v(20), w%(10)
  for i = 0 to 20 : v(i) = i * 1.5 : next
  for i = 0 to 10 : w%(i) = i * 300 - 1000 : next
  call fsum(v(), 21) to s
  if s = 315 then poke 49344, 1
  call isum(w%(), 11) to t
  if t = 5500 then poke 49345, 1
  call fill(v(), 5)
  if v(0) = 0 and v(4) = 16 and v(5) = 7.5 then poke 49346, 1
  call neg(w%())
  if w%(0) = 1000 and w%(10) = -2000 then poke 49347, 1
  ' (0 + 1 + 4 + 9 + 16 for the first five, 1.5 * (5 + ... + 20) for the rest)
  call fsum(v(), 21) to v(20)
  if v(20) = 330 then poke 49348, 1
  poke 49350, 170
  poke 49349, 1 : call fsum(v(), 22) to s : poke 49349, 0
  end

proc fsum(a(), n) compiled
  z = 0
  for j = 0 to n - 1 : z = z + a(j) : next
  return z
endproc

proc isum(b%(), n) compiled
  z = 0
  for j = 0 to n - 1 : z = z + b%(j) : next
  return z
endproc

proc fill(c(), n) compiled
  for j = 0 to n - 1 : c(j) = j * j : next
endproc

proc neg(d%()) compiled
  for j = 0 to 10 : d%(j) = -d%(j) : next
endproc
