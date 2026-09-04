# `docs/` — engine-wide reference (Wizardry I–V)

Findings common to the whole Sir-Tech UCSD p-System engine. Game-specific
notes live under each game's folder (e.g. `wizardry1/docs/`).

| file | what |
|---|---|
| [`engine.md`](engine.md) | the shared engine: disk / file layout, what's common across I–V, per-game plan, the ScummVM goal |
| [`pmachine.md`](pmachine.md) | `SYSTEM.INTERP` — opcode dispatch table, CSP table, SBIOS (disk/kbd/video), runtime layout, the `RANDOM` generator |
| [`pcode.md`](pcode.md) | `SYSTEM.PASCAL` codefile format — segment dictionary, per-segment proc dictionary + PATs, activation-record model |
| [`file-formats.md`](file-formats.md) | the `.DSK` UCSD volume, `SCENARIO.DATA` container, `ASCII.KRN` cipher, `.CHARSET` / `.TITLE`, the `SAVEn.DSK` save model. Record *field layouts* in here are Wiz1's best-known values — expect II/III to mostly match. |
| [`rng-validation.md`](rng-validation.md) | the PRNG, decoded and **validated bit-exact** against a live DOSBox capture. Applies to Wiz I/II/III (identical interpreter). |

Reminder (see `engine.md`): **`WIZ1.DSK` = `WIZ2.DSK` = `WIZ3.DSK` for
`SYSTEM.INTERP`** — byte-identical — so every p-machine / p-code / RNG
finding here is valid for Wizardry II and III as-is.
