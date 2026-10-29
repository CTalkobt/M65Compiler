10 rem test_basic45_numbered_xemu.bas -- a numbered program (the lines
20 rem keep their numbers, as a classic listing) and BASIC 65's two-byte
30 rem functions ($CE: MOD LOG10 WPEEK POINTER) and $FE tokens (WPOKE << >>),
40 rem run by BASIC 65 on xemu. Results at $C0C0 (49344), 1 = pass:
50 rem [0] MOD [1] LOG10 [2] WPOKE/WPEEK (in BANK 0: bank 128 reads $FF
55 rem there) [3] << >> [4] GOSUB/GOTO to numbers [5] POINTER [6] $AA
100 if mod(7,3) = 1 then poke 49344,1
110 if int(log10(1000)+.5) = 3 then poke 49345,1
120 bank 0 : wpoke 49400,4660 : if wpeek(49400) = 4660 then poke 49346,1
130 if (1 << 3) = 8 and (32 >> 2) = 8 then poke 49347,1
140 b = 0 : gosub 300 : goto 160
160 if b = 1 then poke 49348,1
170 a = 5 : if pointer(a) > 0 then poke 49349,1
180 poke 49350,170
190 end
300 b = b + 1 : return
