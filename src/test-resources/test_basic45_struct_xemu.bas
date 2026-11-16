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
'   [6] $AA sentinel
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
  poke 49350, 170
  end

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
