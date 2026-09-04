# Wizardry reverse-engineering notes

Reverse-engineering the classic **Wizardry** games (the Sir-Tech / Andrew
Greenberg engine, Wizardry **I–V**) from the DOS *Ultimate Wizardry Archives*
release, toward clean reimplementations and eventually a shared ScummVM engine.

Wizardry **VI** (*Bane of the Cosmic Forge*) and later moved to a completely
different engine and are out of scope here.

## Layout

| path | what |
|---|---|
| [`docs/`](docs/) | **engine-wide** reference — the shared UCSD p-System stack, the `.DSK` volume format, `SYSTEM.INTERP` (p-machine), the p-code codefile format, `SCENARIO.DATA` / `ASCII.KRN`, the RANDOM generator. Applies to Wiz I–V (I/II/III share one interpreter byte-for-byte). |
| [`wizardry1/`](wizardry1/) | Wizardry I — the reference port. Tooling, the standalone C++ engine (`engine/`), Apple Pascal sources (`sources/`), and Wiz1-specific findings (`wizardry1/docs/`: game logic, decoded tables, roadmap). |
| `wizardry2/` | *(planned)* Wizardry II — no reconstructed sources; to be derived by diffing its p-code / data against Wiz I. |
| [`wizardry3/`](wizardry3/) | Wizardry III material (disk images, Apple source archives). |

## Key fact

`SYSTEM.INTERP` (the x86 UCSD p-machine) is **byte-identical across
`WIZ1.DSK`, `WIZ2.DSK`, `WIZ3.DSK`** (MD5 `1b83a5e5…`). Everything in
[`docs/pmachine.md`](docs/pmachine.md) — opcode table, CSP handlers, SBIOS
interface, the `0x221E` RNG — therefore applies verbatim to Wizardry II and
III. `WIZ4.DSK` / `WIZ5.DSK` ship a different, revised interpreter.

Start with [`docs/engine.md`](docs/engine.md), then
[`wizardry1/docs/roadmap.md`](wizardry1/docs/roadmap.md).
