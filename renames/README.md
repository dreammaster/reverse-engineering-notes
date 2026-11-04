# Rename tables

One TSV per disassembly under `disasm/`, named after the IDB:

| Table | IDB | Executable (unpacked) |
|---|---|---|
| `cadaver1/cad.tsv` | `disasm/cadaver1/cad.idb` | `unpacked/cadaver1/CAD.EXE` |
| `cadaver1/intro.tsv` | `disasm/cadaver1/intro.idb` | `unpacked/cadaver1/INTRO.EXE` |
| `cadaver2/game.tsv` | `disasm/cadaver2/game.idb` | `unpacked/cadaver2/GAME.EXE` |
| `cadaver2/intro.tsv` | `disasm/cadaver2/intro.idb` | `unpacked/cadaver2/INTRO.EXE` |

(New tables for `setup`, `cadaver_com`, the sound drivers … just get the same
`<game>/<stem>.tsv` name.)

The committed `disasm/*.asm`/`*.idc` exports are **not** modified: they still
show the original auto names (`sub_1C7D1`, …). The `old` column records that
name so you can find a routine in the listing, and the tables can be applied to
your local `.idb`s whenever you like.

## Columns

Tab separated; `#` lines are comments.

| Column | |
|---|---|
| `ea` | linear IDA address (`0x1C7D1`). All these IDBs load the image at `1000:0`, so `ea = seg*16 + off` and `1000:C7D1` is the same place the emulator harness prints. |
| `segoff` | the `seg:off` form IDA's listing uses (`seg000:C7D1` ≙ `1000:C7D1`; data segments show their own paragraph, e.g. `1F17:F853`) |
| `kind` | `func`, `data` or `label` |
| `old` | the name in the committed `.asm` (empty = IDA has no name there, e.g. code it did not make a function of) |
| `new` | the new name |
| `conf` | `verified` – confirmed by running the code (tools/dosemu) or by exact content; `likely` – read from the disassembly, role clear but not exercised; `guess` – plausible only (skipped by default) |
| `size` | bytes, for data |
| `sig` | expected bytes at `ea` (`??` = wildcard) |
| `comment` | text for the function comment (or repeatable comment) when none exists yet |

## Applying to your IDBs (Windows, IDA 8.3)

```powershell
.\ida_scripts\run_ida_script.ps1 -Idb cadaver1\cad -ScriptName apply_renames.py
.\ida_scripts\run_ida_script.ps1 -Idb cadaver1\intro -ScriptName apply_renames.py
.\ida_scripts\run_ida_script.ps1 -Idb cadaver2\game -ScriptName apply_renames.py
.\ida_scripts\run_ida_script.ps1 -Idb cadaver2\intro -ScriptName apply_renames.py
```

Dry run first if you have renamed things yourself (note `-NoExport`, and the
environment variable must be set in the same PowerShell session):

```powershell
$env:CADAVER_RENAMES_DRY = 1
.\ida_scripts\run_ida_script.ps1 -Idb cadaver1\cad -ScriptName apply_renames.py -NoExport
Remove-Item Env:CADAVER_RENAMES_DRY
```

The result of each row is written to `ida_scripts\batch_run_and_export.log`
(`OK`, `SKIP … signature mismatch`, `CONFLICT … already named …`, `WOULD rename`)
followed by a summary line. The script is safe to re-run:

* a row whose `sig` bytes differ from the database is skipped (wrong build or
  wrong address) — the Cadaver 1 table cannot be applied to Cadaver 2 by mistake;
* an address you already named yourself is reported as a `CONFLICT` and left
  alone (`$env:CADAVER_RENAMES_FORCE = 1` overrides);
* existing comments are kept;
* `likely` and `verified` rows are applied; add `$env:CADAVER_RENAMES_MIN = 'guess'`
  to include `guess` rows too.

Because the batch driver re-exports `.asm`/`.idc` afterwards, the committed
exports will pick the new names up the next time you export; that is the
intended way to get readable listings to diff against the generated code.

## Getting your own renames back into the repo

```powershell
.\ida_scripts\run_ida_script.ps1 -Idb cadaver1\cad -ScriptName export_renames.py -NoExport
```
```bash
python3 tools/renames.py merge cadaver1/cad     # adds new rows (conf=likely), fills segoff/sig, reports disagreements
python3 tools/renames.py check                  # validate everything
```

## Maintaining the tables (no IDA needed)

```bash
python3 tools/renames.py check              # signatures, segoff, old names vs the .asm exports, duplicates
python3 tools/renames.py fill  cadaver1/cad # fill missing segoff / sig cells after adding rows by hand
python3 tools/renames.py port  cadaver1/cad cadaver2/game   # find the same routines/strings in the other build
python3 tools/renames.py stats
python3 tools/test_renames.py               # tests (apply script against a fake IDA API)
```

`port` matches functions with an instruction-aware signature (Capstone masks
the 16-bit displacements/immediates that move between builds) and data by exact
string content. It cannot port plain variables (their addresses are not
recognisable); those need a manual row once the code that uses them is matched.
Routines that really differ between the games (e.g. `mjm_read`, which gained the
RNC `'2'` branch in Cadaver 2) are matched by hand and flagged in the comment.

**Rule of thumb for new rows:** add a row only for something you have confirmed
(trace it in `tools/dosemu`, or read the code and say so with `likely`), put the
evidence in `comment`, then run `fill` and `check`.
