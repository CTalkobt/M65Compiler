' menu.bas — a BASIC 65 program calling C routines (routines.c)
'
' Build: basic45 --link routines.c menu.bas -o menu.prg
' SYS {name}, a, x, y calls the C function name; RREG A, X reads its result.

start:
  print "{clear}{white}basic 65 + c routines"
  print
  for n = 10 to 100 step 30
    sys {sum_to}, n : rreg lo, hi
    print "sum 1 to"; n; "="; lo + 256 * hi
  next
  sys {call_count} : rreg lo, hi
  print "c was called"; lo + 256 * hi; "times"
  print
  print "press a key to paint the screen"
  getkey k$
  sys {paint}, 7
  print "{home}{down}{down}{down}{down}{down}{down}{down}{down}{down}{yellow}done"
  end
