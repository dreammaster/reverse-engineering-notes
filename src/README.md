# Cadaver C reimplementation

Work in progress: the data decoders, the full room picture (floor, walls, objects, animations, screen frame) and an exploration
prototype (walk around, collide with objects, leave through the doors into the next room) work; game logic (scripts, creatures,
inventory, combat, sound) is not implemented. Currently:

| File | |
|---|---|
| `engine/mjm.[ch]` | `mjm` container + PKWARE DCL explode + RNC1 unpacker |
| `engine/pc1.[ch]` | `.PC1` picture decoder |
| `engine/objsprites.[ch]` | object sprites (`MP1` section 2) |
| `engine/roomobjs.[ch]` | thing → screen projection (`object_project`), collision boxes, runtime room objects with animation, drawing |
| `engine/anim.[ch]` | sprite animation scripts (the game's step + command table) |
| `engine/collision.[ch]` | the game's box collision test (verified against the original routine) |
| `engine/roomexits.[ch]` | door boxes / exits of a room (verified for all rooms of levels 1-5) |
| `engine/acl.[ch]` | ACL script interpreter (subset: thing flags, variables, conditions, IF blocks; room-start events) |
| `engine/push.[ch]` | pushing objects |
| `engine/panel.[ch]` | `ANDATA.DAT` block 1: the frame around the room view |
| `engine/intro.[ch]` | title scroll: `FONT2.SPR` text, `FLAME.DAT` flames over `LOADING2.PC1` |
| `engine/rooms.[ch]` | floor tiles + wall tiles (`MP3`), floor maps, wall records + palette (`MP1`), room shell (floor + walls) renderer |
| `engine/sprites.[ch]` | `PCMAN.DAT` player sprites (48 frames) and the game palette |
| `engine/mp2.[ch]` | `LEVn.MP2` section tables: script records, room lists, things |
| `engine/lng.[ch]` | `.LNG` language string tables (6-bit packed text) |
| `engine/gamefiles.[ch]` | detects Cadaver / Cadaver: The Payoff, resolves DOS-style paths case-insensitively, loads + unpacks files |
| `engine/screen.h` | 320×200 indexed framebuffer + palette |
| `main.c` | SDL2 front end (title / scroll pictures and the player sprite sheet; Esc quits, any key switches scene; `--scene logo|loading|sprites|sprites2|room|tiles|walk|intro|objects [--level N --room R --tata --scroll Y --sim right:30,down:10]`; in the room scene PgUp/PgDn = room, `[`/`]` = level, `T` = DATA/TATA (Payoff), arrows scroll; in the walk scene the arrow keys move the player along the room axes with the original collision test, exits change room (headless: `--sim right:200`)) |
| `tools/cadaver_unpack.c` | command-line unpacker (`cadaver_unpack in out`, `-t` to test) |
| `tests/` | `test_formats` compares the decoders with vectors from the Python reference (`tools/gen_test_vectors.py`) |

```bash
cmake -S src -B build && cmake --build build
ctest --test-dir build --output-on-failure
./build/cadaver -g cadaver1 [--scene loading] [--screenshot out.bmp]
```

Verification tools (Python, need Unicorn and the unpacked executables): `tools/verify_collision.py`, `tools/oracle_rooms.py` +
`tools/verify_exits.py` / `tools/verify_objects.py` compare the reimplementation with the original CAD.EXE running in an emulator.
See `docs/roadmap.md` for what is missing.
