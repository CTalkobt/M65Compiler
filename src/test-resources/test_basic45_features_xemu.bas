' test_basic45_features_xemu.bas -- basic45 extensions, run by BASIC 65
'
' Results POKEd at $C0C0 (1 = pass):
'   [0] long variable names: score/scale/store share no variable, and store
'       is not the status variable ST
'   [1] block if / else / endif (nested)
'   [2] proc / endproc / call
'   [3] FN with a long name; integer and string variables with long names
'   [4] ON ... GOSUB to a list of procs
'   [5] $AA sentinel
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
  poke 49349, 170
  end
proc increment
  counter = counter + 1
endproc
pickone: picked = 1 : return
picktwo: picked = 2 : return
