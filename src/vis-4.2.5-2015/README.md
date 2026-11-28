# Engine 4.2.5, build 1185, built 2015 (notes only: no code reconstructed yet)

| | |
|---|---|
| Engine release | 4.2.5 |
| Build number | 1185 |
| Build date | 6 Dec 2015 |
| Project format version | 0xB2 = 178 |
| Compiler | GCC 4.6.3 / 4.7.2 `-O2`, DynASM 1.3.0 |
| Games of this build | **Deponia 2** (Chaos on Deponia; Linux, GOG, executable `Deponia2`) and **Deponia 3** (Goodbye Deponia; GOG 3.3.3335: **the same executable**, 34 bytes differ: the build id and the password) |
| Compare with | `../vis-5.0.1-2019` (its data loads with that tree; the older engine has no shader or framebuffer classes, no TCP debugger, fewer Lua graphics functions) |

There is no source here yet: the code of this engine is to be made from `../vis-5.0.1-2019` with the older behaviours. What is
known is in `VERSIONS-deponia2.md` and `VERSIONS-deponia3.md` (the games' own data: passwords, sizes, what loads), the comparisons
with Deponia 1 and 4 in `SYMDIFF-D1.md`, `GLOBALDIFF-D4.md` and `ENGINECALLS-D4.md`.

`deponia2.bat` and `deponia3.bat` run these games with the 5.0.1 player until this tree has one of its own.
