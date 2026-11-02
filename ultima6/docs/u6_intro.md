# Ultima VI — `U.EXE`: menu, introduction and character creation

What `U.EXE` (`u6/u6_u`) does, as read from the code. Names are in `ida_scripts/apply_renames_u6_u.py`; the C++ for the character
creation logic is in `src/ultima6/gypsy.*` (namespace `Ultima6::Ultima6`) with tests in `tests/test_character.cpp`.

## Flow

`ULTIMA6.EXE` runs `U.EXE`, whose `_main` (`11D33`) reads `CONFIG.U6` (video/sound choice), loads the matching driver
(`MCGADRV.BIN`, `EGADRV.BIN`, `TGADRV.BIN`, `CGADRV.BIN`), `U6.SET`, the cursors (`CURSORS.*`, `INTRO.PTR`) and the music
(`MIDI.DAT`, `*.M`), seeds the generator with the time and plays `title_sequence` (`17A36`, the `INTRO1-3` screens). It then loops on
`main_menu` (`113A5`, `MAINMENU.*`/`TITLES.*`), whose result indexes a jump table:

| Entry | Menu item | Code |
|---|---|---|
| 0 | Introduction | `intro_story` (`1C678`, `INTRO.*`, `STARPOS.DAT`), then sets `intro_seen` |
| 1 | Create a new character | `create_character` (`152C9`); on success it plays the introduction first if not yet seen, then `start_game` |
| 2 | Transfer a character | `transfer_character_menu` (`13A06`, `transfer_from_disk` `13470`: reads `PARTY.SAV` or `SAVED.GAM` of Ultima V / IV from drive A: or `\ULTIMA`); then the same |
| 3 | Acknowledgements | `acknowledgements` (`16766`) |
| 4 | Journey onward | `start_game` (`16DF9`, `execl GAME.EXE`) |

Story text, and the gypsy's questions (ids 105-132), are shown by `show_text_page(id)` (`19A0B`, 8.6 KB of strings and drawing).

## Character creation

`create_character` asks for the name (up to 13 characters, `enter_name_and_sex`), the sex (`M`/`F`) and one of six portraits
(`MONTAGE.*`), then the gypsy (`GYPSY.*`, `VELLUM1.*`, `WOODS.*`) asks seven questions.

* **Stats**: strength, dexterity and intelligence all start at 15; karma at 75.
* **Questions**: the virtues 0-7 (Honesty, Compassion, Valor, Justice, Sacrifice, Honor, Spirituality, Humility) are shuffled
  (`shuffle_words`: `rand % (n*n) + n` swaps of two random slots) and taken as four pairs; each pair gets the question
  `gypsy_questions[v*8 + w]` (28 questions, one per pair, numbered 105-132). The four winners are shuffled again and form two
  pairs, and the last two winners meet in the final question: 4 + 2 + 1 = 7 questions.
* **Answers**: answer A of question *q* picks virtue `gypsy_answer_a[q-105]`, B picks `gypsy_answer_b[q-105]` (always the two
  virtues of the pair). `gypsy_apply_answer` adds that virtue's row of `gypsy_bonus_str/dex/int`:

  | Virtue | 0 Honesty | 1 Compassion | 2 Valor | 3 Justice | 4 Sacrifice | 5 Honor | 6 Spirituality | 7 Humility |
  |---|---|---|---|---|---|---|---|---|
  | STR | 0 | 0 | 2 | 0 | 1 | 1 | 1 | 0 |
  | DEX | 0 | 2 | 0 | 1 | 1 | 0 | 1 | 0 |
  | INT | 2 | 0 | 0 | 1 | 0 | 1 | 1 | 0 |

  and Humility also adds 3 karma. So each answer adds 2 points of stats (3 for Spirituality, 0 for Humility): between 45 and 66 in total.
* The generator is the Borland run-time one (`_srand` 2195A, `_rand` 2196B: seed = seed * 015A4E35h + 1, result bits 16-30);
  `BorlandRandom` reproduces it (Turbo C's classic 346, 130, 10982, ... after `srand(1)`).
* Nuvie's port (`script/u6/intro_create.cpp`) has identical tables; it differs in running one extra shuffle swap, in numbering
  the vials from 1, and in omitting the Humility karma bonus.

## The new game

`write_new_game_objlist` (`14066`) loads the `savegame\objlist` the installer left (the 7283-byte tail of `LZDNGBLK`; a
shipped save is 256 bytes longer), refuses if byte `+0x1C72` is already set, and patches the Avatar (actor 1):

| Offset | Value |
|---|---|
| `+0x901`, `+0xA01`, `+0xB01` | strength, dexterity, intelligence |
| `+0x13F2` | magic points = 2 * intelligence |
| `+0xC02` | experience 370 (0x172), little-endian word |
| `+0xE01` | hit points 90 |
| `+0xFF2` | level 3 |
| `+0xF00` | the name, 14 bytes |
| `+0x1BF9` | karma |
| `+0x1C71` | sex: 1 for female |
| `+0x1C72` | portrait + 1 (non-zero means a character exists; `GAME.EXE`'s `_main` prints "You must first create or transfer a character." when it is 0) |

then writes the 0x1C73 bytes back. `0x13F1` is therefore the magic points array (the other order in Nuvie's loader is a leftover).
A transferred character takes the same path via `transfer_from_disk`'s helpers (`12863`, `12904`), which derive experience as
`score / 10`, the level from `score / 1000` and hit points as `30 * level`.

## Not yet analysed

The animated parts (`title_sequence`, `intro_story`, the pouring animation and the story pages in `show_text_page`), the transfer
file formats, and `MIDI.DAT`/`*.M` handling. The question and story *texts* are stored as strings in `U.EXE`; whether the new engine
reads them from the executable or carries its own copy is a decision for the ScummVM stage.

## Stand-alone front end (`src/ultima6/front_end.*`)

`FrontEnd` is the menu and character creation of this implementation, driven by the same `GypsyQuiz` logic and ending in `World::load(dir, &character)`, which builds a new game
from `LZOBJBLK` (64 surface blocks one after another) and `LZDNGBLK` (5 dungeon blocks, then the objlist patched by `NewCharacter::applyTo`). Screens use the palettes
`create_character` and `_main` load with `load_intro_palette`: 0 for the main menu (`TITLES.SHP`, `MAINMENU.SHP`), 6 for the gypsy (`GYPSY.SHP` background, `VELLUM1.SHP` for the question), the
portraits (`MONTAGE.SHP`, 6 female then 6 male) with `U6PAL`. The 28 questions are `src/ultima6/gypsy_text.inc`, extracted from `U.EXE` (they follow the story pages as
NUL-terminated lines; `%c` marks a key wait) and check against the A/B virtue tables. Not done: the animated title sequence and introduction, the story pages, character
transfer, acknowledgements, and the original's exact positions on the screens.
