# MM2 tools (plain Python 3, no dependencies)

Game directory defaults to `D:\GOG Games\Might and Magic 2` (`mm2_layout.DEFAULT_GAME_DIR`).

| Script | What |
|---|---|
| `plink_info.py` | dump the EXE's Plink86 layout: MZ header, overlay table, thunks |
| `mm2_layout.py` | parser used by the IDA scripts and tools |
| `mm2_lzw.py` | LZW decoder (`lzw_decode`) |
| `mm2_data.py` | `Game`: `attrib()`, `monsters()`, `strings()`, `map(n)`, `events(kind, n)`, `parse_events`, `split_scripts`; CLI `info` / `unpack DIR` |
| `mm2_gfx.py` | image banks `*.16` / `*.4` -> PNG (`python mm2_gfx.py FILE OUTDIR [sheet]`) |
| `mm2_monsters.py` | monster pictures/animation frames (`MONSTERS.16`, `MONSTERS.4` with `cga=True`) |
| `mm2_rules.py` | experience table and training cost |
| `gen_spell_table.py` | joins `SPELLS.DAT` with the manual text to produce docs/spells.md |
| `condense_asm.py` | shrinks an IDA `.asm` export for reading |
- `mm2_view.py` — software renderer of the indoor first-person view (verifies docs/view.md): `python mm2_view.py MAP X Y N out.png [town|cave|castle]`
