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

## Not yet analysed

The evaluator, the text escape codes, the keyword matching (`F6`/`EF`), the input opcodes and the variable set; the portrait loading
(`PORTRAIT.A/B/Z`, `GAME.EXE` `394A9`).
