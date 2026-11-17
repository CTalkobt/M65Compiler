' test_basic45_struct_xemu.bas -- block IF, select, procs with local labels and
' comments, built as is and with --minify (#598), run by BASIC 65 on xemu
'
' Results POKEd at $C0C0 (49344) (1 = pass):
'   [0] block if / else / endif, both ways
'   [1] a proc with a local label and a loop, called twice
'   [2] select / case
'   [3] a ' comment after a statement and between statements
'   [4] nested block if in a proc with parameters
'   [5] a block if in an else part
'   [6] select: several items, a range, is, a string, a case with nothing in
'       it, an IF in a case, a case that ends in a GOTO, case else (#605)
'   [7] procs one after the other, after an END and after a statement that
'       runs on: none is run into (#605)
'   [8] a hex and a binary constant after a keyword (#603)
'   [9] $AA sentinel
  x = 1 : r = 0
' the comment between statements
  if x then
    r = r + 1   ' it's a comment, with a : colon
  else
    r = 100
  endif
  if x = 0 then
    r = 100
  else
    r = r + 1
  endif
  if r = 2 then poke 49344, 1

  n = 0
  call count3
  call count3
  if n = 6 then poke 49345, 1

  k = 2 : s = 0
  select k
    case 1
      s = 10
    case 2
      s = 20
    case else
      s = 30
  endselect
  if s = 20 then poke 49346, 1

  c = 5 ' five
  ' indented comment
  c = c + 1
  if c = 6 then poke 49347, 1

  q = 0
  call pick(3, 4)
  call pick(9, 4)
  if q = 12 then poke 49348, 1

  e = 2 : t = 0
  if e = 1 then
    t = 1
  else
    if e = 2 then
      t = 2 ' the inner block's
    else
      t = 3
    endif
  endif
  if t = 2 then poke 49349, 1

  ' u: one digit per pick(k), each the case that ran
  u = 0
  for j = 1 to 9
    k = val(mid$("002003004007009020200250099", j * 3 - 2, 3))
    call which(k)
    u = u * 10 + w
  next
  ' 2 -> 1; 3, 4 -> 2; 7, 9 -> 3 (9 with the flag: 3); 20 -> 4 (nothing); 200 -> 5; 250 -> 6; 99 -> 7
  w$ = "" : k$ = "b" : gosub strsel : k$ = "zz" : gosub strsel : k$ = "a" : gosub strsel
  if u = 122334567 and w$ = "BEA" then poke 49350, 1

  v = 0
  call first
  v = v * 10
  call second
  v = v * 10 : call third
  if v = 123 then poke 49351, 1

  b = %101 : poke $c0c8, b - 4
  poke 49353, 170
  end

strsel:
  select k$
    case "a"
      w$ = w$ + "A"
    case "b", "c"
      w$ = w$ + "B"
    case else
      w$ = w$ + "E"
  endselect
  return

' which case k takes
proc which(k)
  w = 0 : f = 1
  select k
    case 2
      w = 1
    case 3, 4
      ' a comment in the case
      w = 2
    case 5 to 9
      if f then w = 3
    case 20
    case is >= 250
      w = 6
      goto .out
    case is > 100
      w = 5
    case else
      w = 7
  endselect
  if k = 20 then w = 4
.out:
endproc

proc first
  v = v + 1
endproc
proc second
  v = v + 2
endproc
' (a proc after a line that runs on: its GOTO around it stays)
skipped: v = v + 0
proc third
  v = v + 3
endproc

' counts to three by a local label
proc count3
  i = 0
.again:
  i = i + 1 : n = n + 1
  if i = 3 then goto .done
  goto .again
.done:
endproc

' the larger of two, added to q; a nested block
proc pick(pa, pb)
  if pa > pb then
    if pa > 5 then
      q = q + pa
    else
      q = q + 1
    endif
  else
    q = q + pb - 1
  endif
endproc
