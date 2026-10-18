' test_basic45_features_xemu.bas -- basic45 extensions, run by BASIC 65
'
' Results POKEd at $C0C0 (1 = pass):
'   [0] long variable names: score/scale/store share no variable, and store
'       is not the status variable ST
'   [1] block if / else / endif (nested)
'   [2] proc / endproc / call
'   [3] FN with a long name; integer and string variables with long names
'   [4] ON ... GOSUB to a list of procs
'   [5] select / case (values, ranges, is, else; numbers and strings)
'   [6] proc parameters, a local label in a proc
'   [7] string encodings: p"..", s"..", @"..", #pragma encoding, escapes
'   [8] $AA sentinel
start: score = 10 : scale = 20 : store = 30 : sc = 5
  if score = 10 and scale = 20 and store = 30 and sc = 5 then poke 49344, 1
  total = 0
  for value = 1 to 6
    if value > 3 then
      if value = 5 then
        total = total + 100
      else
        total = total + 10
      endif
    else
      total = total + 1
    endif
  next
  if total = 123 then poke 49345, 1
  counter = 0
  call increment
  call increment
  if counter = 2 then poke 49346, 1
  def fn triple(x) = x * 3
  amount% = fn triple(7) : greeting$ = "hello"
  if amount% = 21 and greeting$ = "HELLO" then poke 49347, 1
  picked = 0
  on 2 gosub pickone, picktwo
  if picked = 2 then poke 49348, 1
  hits = 0
  for k = 1 to 12
    select k
    case 1, 2
      hits = hits + 1
    case 3 to 5
      hits = hits + 10
    case is > 10
      hits = hits + 100
    case else
      hits = hits + 1000
    endselect
  next
  word$ = "b"
  select word$
  case "a"
    hits = 0
  case "b", "c"
    hits = hits + 1
  endselect
  ' 2*1 + 3*10 + 2*100 + 5*1000 + 1
  if hits = 5233 then poke 49349, 1
  call addto(5, 7)
  if total2 = 12 and spins = 3 then poke 49350, 1
  ok = asc(p"A") = 193 and asc(p"a") = 65 and asc(s"A") = 1 and asc(@"a") = 97
  ok = ok and asc("a") = 65 and asc("{shift-a}") = 193 and asc("{cbm-a}") = 176
  ok = ok and len("{down*3}") = 3 and asc("{esc-x}") = 27 and asc(mid$("{80col}", 2)) = 88
#pragma encoding petscii
  ok = ok and asc("Q") = 209
#pragma encoding upper
  if ok and asc("Q") = 81 then poke 49351, 1
  poke 49352, 170
  end
proc addto(x, y)
  total2 = x + y
  spins = 0
.again: spins = spins + 1 : if spins < 3 then .again
endproc
proc increment
  counter = counter + 1
endproc
pickone: picked = 1 : return
picktwo: picked = 2 : return
