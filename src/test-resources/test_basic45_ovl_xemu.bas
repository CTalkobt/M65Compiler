' test_basic45_ovl_xemu.bas -- code overlays in a basic45 --link program (#685)
' (built with -DNOLOAD and --overlay-no-load, #689: the program BLOADs the
' overlay file into bank 4 itself; there is no loader)
'
' Built with --link basic45_ovl/main.c --link basic45_ovl/cold.c
' --overlay-store=0x40000-0x4FFFF --overlay-region=_mod_area+512 --disk:
' cold.c's functions in an overlay kept in bank 4, run in main.c's mod_area.
'
' Results POKEd at $C0C0 (1 = pass):
'   [0] the overlays' region is mod_area
'   [1] a call into the overlay: loaded, its result back
'   [2] the overlay in the region is cold (1)
'   [3] the program reuses the area: mod_wipe clears it, __ovl_current 0
'   [4] the next call loads the overlay again
'   [5] its variable kept between the loads (2 calls)
'   [6] $AA sentinel
#ifdef NOLOAD
' (BLOAD leaves BANK 4 the current bank: POKE needs 128 again)
start: bload "bovl.ov1", b4, u8 : bank 128
  sys {region_ok} : rreg a
#else
start: sys {region_ok} : rreg a
#endif
  if a = 1 then poke 49344, 1
  sys {cold_add}, 40, 2 : rreg a, x
  if a + 256 * x = 142 then poke 49345, 1
  sys {ovl_cur} : rreg a
  if a = 1 then poke 49346, 1
  sys {mod_wipe} : sys {ovl_cur} : rreg a
  if a = 0 then poke 49347, 1
  sys {cold_add}, 200, 7 : rreg a, x
  if a + 256 * x = 307 then poke 49348, 1
  sys {cold_calls} : rreg a, x
  if a = 2 and x = 0 then poke 49349, 1
  poke 49350, 170
  end
