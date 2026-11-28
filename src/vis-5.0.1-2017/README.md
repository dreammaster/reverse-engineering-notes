# Engine 5.0.1, build 1189, built 2017

| | |
|---|---|
| Engine release | 5.0.1 |
| Build number | 1189 |
| Build date | 6 Sep 2017 (older than the 2019 build of the same release in `../vis-5.0.1-2019`) |
| Project format version | 0xBA = 186 |
| Optimisation | `-O0` (every inline function is a symbol; read comparisons with that in mind) |
| Games of this build | **Deponia 4 (Doomsday)** (Linux, GOG 1.2.0267, executable `Deponia4`) |
| Compare with | `../vis-5.0.1-2019` (same release and build number, not the same code: `MERGE.md` lists the differences that need a decision when the engines are merged: the text path with HarfBuzz and the height lock of the fonts, the outline shader, graphics details) |

A copy of the 2019 tree changed where this build's original differs. `VERSIONS.md` has the log (the OpenGL backend with the
shaders of the Shader Toolkit, `glBackend.*`, the matrices of the camera, the effects), `FUNCDIFF.md` the functions compared.

Build and run: `SDL=1 tools/build.sh 5.0.1-2017`, then `deponia4.bat`.
