# The play screen and command loop of GAME.EXE

C++: `src/ultima6/session.*` (the game), `text_window.*`, `stats_view.*`, `ui.*`, `map_renderer.*`, `talk.*`, `conversation.*`.

## Screen

* `PAPER.BMP` is not a Windows bitmap: LZW (see `lzw.h`) of u16 width, u16 height (320 x 200) and the pixels. It is the whole frame of the play screen
  (parchment, flame border, the dark-edged scroll box). `NEWMAGIC.BMP` (144 x 82) and `WORLDMAP.BMP` (128 x 128) have the same layout. `src/files/picture.*`.
* The map is the 160 x 160 region (8,8)-(167,167) of the 11 x 11 cell buffer, drawn at (8,8). The border tiles 0x1B0-0x1B7 are laid on the outer ring of the buffer
  (corners 1B0 top-left, 1B2 top-right, 1B3 bottom-left, 1B5 bottom-right; edges 1B1 left, 1B4 right, 1B6 top, 1B7 bottom).
* Text windows are defined by `sub_17449(window, x1, y1, x2, y2)` in 8 x 8 character cells: the full screen (0,0)-(39,24), the party panel (22,1)-(38,12)
  (17 x 12 characters at pixel 176,8) and the scroll (22,14)-(38,23) (17 x 10 at pixel 176,112). Text colour 0x48 on the parchment colour 0x31
  (`word_402A6`, `word_402A4`); highlight colours `word_4029A` = 12 (red, nearly dead) and `word_4029C` = 10 (green, poisoned). The font is `U6.CH`.
* The panel mode `word_3DD10`: 0x90 = member statistics (`sub_1D4DA`: name centred on row 0, the 56 x 64 portrait at (176,16), then at column 10 STR/DEX/INT on rows 2-4,
  "Magic" and `mp/max` on rows 6-7, "Health" and `hp/maxhp` on rows 8-9 (`hp` red when below 10, green when poisoned), "Lev/Exp" and `level/exp` on rows 10-11, and the
  four button tiles 0x183, 0x180, 0x182, 0x184 at y = 0x58, x = 0xB0, 0xC0, 0xD0, 0xE0), 0x91, 0x92 and 0x9E the inventory, spell and other views.
* Maximum hit points are 30 x level (1..255, `sub_2B519`); maximum magic points are 2 x INT for the Avatar's type 0x19A, INT for type 0x17A and INT / 2 for types 0x179 and 0x182,
  else 0 (`sub_2B549`).

## Commands

`command_loop` (13C64) prints the prompt `"%s:\n%>"` (the name of the member in control, a colon, '>') and reads a key through `sub_17099`.
Letters map to command codes (table at 1D0D): A 0x81 attack, B 0x8A break off combat, C 0x82 cast, D 0x86 drop, G 0x85 get, L 0x84 look, M 0x87 move, R 0x89 rest,
T 0x83 talk, U 0x88 use, ctrl-S 0xAA save; walking is 0x80 (`sub_2787E`). The names printed as `"%s-"` are the table at ds:03A2 (Attack, Cast, Talk, Look, Get, Drop, Move,
Use, Rest, Save ...). Targeted commands wait for a cursor selection (code 0x8E) and otherwise print "what?"; walking prints nothing (a blocked step only plays a sound).
Strings of the combat and command code live in a table in `seg067` (offsets 0x000-0x519: "Rest!", "Pass!", "Solo mode", "Party mode", "Begin combat!" ...).

Walking (`sub_2787E`): the target is the leader's position plus the direction; blocked if `sub_25D7F` says so; a diagonal step also needs one of the two squares it cuts across to be free.

## Objlist globals (objlist +0x1BF1 on)

quest flag 0x1BF1 (0 = not on a Sacred Quest), rest counter 1BF2, game time 1BF3 (minute, hour, day, month, year u16), karma 1BF9, wind direction 1BFA, timers 1C03,
eclipse 1C12, alcohol 1C17, moonstones 1C1B, Gargish known 1C5F, combat mode 1C69, solo mode 1C6A, command bar 1C6C, gender 1C71 (1 = female), Avatar portrait + 1 at 1C72.

## Talking

`cmd_talk` (35130): "\nNot on screen.\n", "\nNot in solo mode.\n", "\nYou hear a deep moan.\n", "\n\nNo response\n", "\nTalking to yourself?\n", "\nFunny, no response.\n" (no script),
"\n\nYou see " + the script's description, "\nyou say:" before each line of input. The interpreter in `conversation.*` follows Nuvie's and runs all 97 scripts of CONVERSE.A it is given
(`tests/test_conversation.cpp`); Lord British's opening (the Compendium question) comes out as in the game. A `*` in the text waits for a key and is then erased.
