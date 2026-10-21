# FOD.EXE — party creation / main menu

`FOD.EXE` is the program that runs first. All 130 functions are named (52 of
them carried over from KEH.EXE by mnemonic-signature matching: CRT, window
text, file I/O, LZSS, dirty-rect blitter — see `ida_scripts/apply_renames_fod.py`).

Flow (`_main`): `loadDisk1` (DISK1 save; if its save counter is non-zero the game
has been started before and the roster is kept, otherwise a default empty roster
is built) → `loadClasses` (BORDERS offset 5000 length 1000 = menu data,
ARCHTYPE = professions, HDSPCT first 0x49C bytes) → `gfxInit` → show the `TPICT`
title picture (LZSS) → menu: **A**dd member, **R**emove member, **E**dit member
(or F1-F3), **P**lay the game. "Play" requires at least one member
("It's tough out there! You should take somebody with you."), calls
`writeInitialDisk1` (save counter := 1, roster ids -6, writes `DISK1`), frees
the picture arena and `_spawnl`s **KEH.EXE**.

Character creation: `chooseProfessionMenu` ("Choose a Profession:") →
`createNewMember`/`editPartyMember`; attributes are raised/lowered with
`increaseAttribute`/`decreaseAttribute` bounded by the profession's min (+14h…)
and max (+20h…) tables, with `Attribute Pts` taken from the profession
(`+33h`); sex toggles when the cursor is on row 7. `rollMemberHitPoints` rolls
Con = random(profession min, max) (profession `+70h`/`+72h`) into `hpMax`, `hp`
and `lastHurt`.

Video modes (`_gfxMode` from DISK1): 0 CGA (INT10h mode 4), 1 Tandy/PCjr (mode 9),
2 EGA (mode 0Eh planar), 3 VGA (mode 13h with a 16-colour DAC palette
`MASTER_PALETTE`); each has its own scanline table and blit loop
(`videoModeCga/Tandy/Ega/Vga`, `blitDirtyRectsCga/Vga`).

## Initial roster/world written for a new game (`loadDisk1` when empty)

* Each of the 5 roster slots: name empty, 6 starting items in slots 0-5
  (item ids 2, 0Ch, 0Ch, 20h, 29h, 37h; the first with ammo 9), item count 6
  (`+52h`), rank 1, exp-to-next 1500 (5DCh), readied weapon/armor none (0FFh),
  sex flag 1, all mutation timers 0.
* World: cash 100, time 09:00, weekday 1, party at (10,6) on map 28 (`_maps[0]`
  = 1Ch), map stack depth 0, facing 0, unique-item bitmask 0.
