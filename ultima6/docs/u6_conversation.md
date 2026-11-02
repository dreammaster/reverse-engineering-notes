# Ultima VI conversations

Scripts live in `CONVERSE.A` (actors 0-98) and `CONVERSE.B` (actors 99-223 plus three shared scripts), are loaded by `GAME.EXE`'s
`load_conversation` (`395D0`) and run by the interpreter built from `conv_read_value` (`1F1BE`) and `conv_execute_command` (`201EB`).
The C++ for the file layer is `src/files/converse.*` (tests in `tests/test_converse.cpp`).

## Finding and reading a script (read off `395D0`)

* Actors 0-98: `CONVERSE.A`, item = actor. Actors 99-223 (below 0xE0): `CONVERSE.B`, item = actor - 99.
* Actors 224 and up can only be talked to when their object type is 0x175, 0x17E or 0x16B; they use `CONVERSE.B` items 102, 103, 104
  (the scripts of actors 201-203). Any other creature does not talk.
* The files are offset-table archives (`LibN`, no size header); the item is read from its offset: a u32 that is non-zero and below
  0x2800 means an LZW stream follows (that u32 is its size header), otherwise the plain script starts 4 bytes in and at most 0x2800
  bytes are read (CONVERSE.B item 26 is stored that way). Nuvie's `ConvScript::read_script` applies the same test.
* All 200 scripts that exist load, and each one's second byte equals its actor number, which confirms the item numbering.
  24 actor numbers below 224 have no script.

## Script layout

`FF <actor> <name> F1 <description> F2 <code...>`. The code is a byte stream in which printable text and control bytes are mixed;
commands are bytes from 0x9C up and are followed by value expressions.

## The command table (`jpt_20207`)

`conv_execute_command` handles the opcodes 0x9C-0xDF through a 68-entry jump table (unlisted entries print "Unknown command."):

| Opcode | Handler | Opcode | Handler |
|---|---|---|---|
| 9C | `208A1` (horse: puts a horse at the party's position) | C4 | `20676` |
| 9E | `207C8` | C5 | `20696` |
| A1 IF | `2046D` | C8 | `2060F` (three operands: actor, ..., adds an object to an inventory through `1B59D(..., 0x10)`) |
| A2 ENDIF, A7 | `208EB` (no action) | C9 GIVE | `20632` (four operands: object, quality, quantity, actor; creates the object and adds it to an inventory) |
| A3 ELSE | `204F9` | CB | `2058D` |
| A4 SETF, A5 CLEARF | `206DF` | CD | `205A1` |
| A6 DECL | `2033A` | D0 | `206B9` (waits `operand * 20` ticks) |
| B0 JUMP | `20515` | D6 | `2071C` |
| B5 DPRINT | `2029C` (prints text up to a NUL; `D2` introduces a longer form, `D5` ends) | D8 | `20795` |
| B6 BYE | `20294` (sets `byte_3E1DA`, ends the conversation) | D9 | `20763` |
| B9, BA NEW/DELETE | `20534` | DB | `2077F` |
| BE | `20834` | DF | `207AE` |
| BF | `2088D` | | |

This agrees with Nuvie's `U6OP_*` list (`core/converse_interpret.h`) except that the original's table also has 0xC8, 0xD0 and 0xDF,
and `0xA8` (ASSIGN) is not in it (it is handled outside the table). The value opcodes (comparisons, arithmetic, `VAR`, flags, ...)
are evaluated by `conv_read_value` and not yet traced against Nuvie's list.

## Portraits (`load_portrait`, `394A9`; `src/files/portrait.*`)

`PORTRAIT.A` (98 items), `PORTRAIT.B` (96) and `PORTRAIT.Z` (12) are offset-table archives whose items are LZW streams of 56 x 64 bitmaps (all 206
decode to exactly 3584 bytes). The Avatar uses `PORTRAIT.Z` item (portrait number chosen at creation, stored plus one at `objlist +0x1C72`);
other actors use number = actor (item = number - 1 for actors above 0), below 98 in `PORTRAIT.A`, otherwise item number - 98 of `PORTRAIT.B`;
creatures of object types 0x175, 0x17E and 0x16B (actors 224 and up) use numbers 192-194. `MONTAGE.SHP` holds the same twelve Avatar heads
(6 female, then 6 male) as `PORTRAIT.Z`, item for item (they agree on 70-90% of their pixels; the rest differ by palette use).
Nuvie remaps a few numbers (Exodus, shrines) that the original code does not.

## Books (`read_book`, `2FE1F`)

`BOOK.DAT` is an archive with u16 offsets (128 items) of NUL-terminated texts with the same `%` directives as `scroll_printf`; a book's number is
the quality byte of the object and item number - 1 is read into the shared script buffer (`conv_script_ptr`) and printed.

## Expressions (`conv_read_value`, `1F1BE`)

A value is a **postfix expression** evaluated on a stack of ten 32-bit entries and ended by the token `A7` (or `A8`, the assignment form, which
separates the target of an assignment from its value). `D2`, `D3` and `D4` are followed by a u32, u8 or u16 constant; any other token below 0x81 is
itself a small constant; the rest are operators through the 100-entry table `jpt_1F2BF` (0x81-0xE4):

| Operators | |
|---|---|
| 81-86 | greater, greater-or-equal, less, less-or-equal, not-equal, equal |
| 90-95 | add, subtract, multiply, divide, or, and |
| 9A, 9B, 9D, 9F, A0 | can-carry, weight (object weight = `tileflag_block3[type]` times the quantity), horsed, has-object, random(a, b) through `3B8F9` |
| AB | flag(npc, flag) |
| B2, B3, B4, B7, BB | variable, string variable, data, index-of, object count |
| BC, BD | push 100 |
| C0 | prints "Select item:" and lets the player pick an object; yields it, or 0x8001 when cancelled |
| C1, C2, C3 | an object's position word, an object's type (`obj_type_frame & 3FF`), a script-position switch |
| C6, C7, CA, CC | in-party, object-in-party, join, leave |
| D7, DA, DC, DD | npc-nearby, wounded, poisoned, npc |
| E0-E4 | experience, level, strength, intelligence, dexterity |

This is Nuvie's `U6OP_*` list (`core/converse_interpret.h`) plus BC, BD and C0-C3, which the original has and Nuvie does not name. The `weight`
operator settles what `TILEFLAG`'s third block (+0x1000, 1024 bytes) is: the weight of each object type.

Variables live in 16-bit slots; slot *v* is at `0x48F31 + 2 * v` (so `0x10`, the Avatar's sex, is `word_48F51`, `0x14` karma, `0x19` hit points).
`sub_1EA97` fills them for a conversation from the game state (sex, hour, day, month, year, karma, party size, hit points, worktype, ...) and
points the string variables at the player's and the speaker's names and at the words for "milord/milady" and "morning/afternoon/evening".

## Structure of the code

`ConverseScript::scan` (`src/files/converse.*`) walks the code with the grammar below; 199 of the 200 scripts scan to the end of their code (where a
zero pad or the data tables start), and of 4963 jumps only 5 land inside an operand (a mis-classified byte in those, not a real jump):

* a command takes a fixed number of expressions: SETF/CLEARF/DECL/C8/WORKTYPE 2; IF, HORSE, INVENTORY, PORTRAIT, ADDKARMA/SUBKARMA, D0, RESURRECT,
  SETNAME, HEAL, CURE, DF 1; NEW/DELETE/GIVE 4; SLEEP, ENDIF, ELSE, BYE, WAIT, ENDANSWER, ASK, ASKC none; JUMP `B0` a u32 **absolute offset into the
  script** (so conditionals are `IF <expr> JUMP <target> ... ENDIF`); `F9/FB/FC` (input) a variable reference; DPRINT `B5` either `D5` or `D2 <u32>`
  followed by `B1` or an expression;
* `EF <keywords> F6 <answer code>`: the keywords are a comma-separated list (Lord British's script has 55 such entries, `name`, `rep`, `job`, ...);
  `F7` asks for input and the answers end with a `JUMP` back to it; `FF`, `F1`, `F2` are the header and conversation start;
* any other printable byte is text (`$P` player name, `$T` time of day, `*` wait for a key, `@` marks an emphasised keyword in the answer text).

## Not yet analysed

The keyword matching itself (how the input is compared: Nuvie clamps to four characters), the text escape codes beyond the ones above, the data tables
that follow the code (`B4` DATA / `B8`), and the remaining command handlers' exact semantics.
