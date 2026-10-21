# Deponia 1: versions of the engine and of the game

What makes the other builds different is collected in [`/VERSIONS.md`](../../VERSIONS.md); this file has the details of
Deponia 1, the build that `src/deponia1/` reconstructs. Why the code is as it is: `NOTES.md`.

## Deponia 1 Linux (GOG, game version 4.0.1386): the reconstruction target

Data: `D:\Games\ScummVM\Dev\games\Visionnaire\Deponia1-Linux\data\noarch\game`.

| | |
|---|---|
| Game version | 4.0.1386 (`version.txt`; the log of the engine says "Game Version: 4.0.1386") |
| Executable | `Deponia`, ELF x86-64, 18,926,351 bytes, sha1 `f4cad9601c77...`, GCC 4.7.2 (Debian), 4.4.7 and 4.7.2 (Red Hat), 4.6.3 (Ubuntu) |
| Engine build | "Engine Version: 5.0.1 (Build 1189 from Build date: Feb  8 2019)" (the log of the reconstruction, from the data and the constants) |
| Source paths in the assertions | `/home/simon/Documents/jenkins/branchPillars/src/...` (the branch the build came from) |
| Disassembly | `Deponia_Linux.asm` (the IDA export this reconstruction is made from) and `Deponia_Linux.i64` |
| `data.vis` | 81,012,812 bytes, `VIS3`, 3,109 entries, project version 3109 |
| Password of `data.vis` | `ykgT6QsrRNv9XR`, a narrow string next to the texts of the command line options (`AppGlobals.cpp`: `passw`) |
| Other passwords | `SAVEGAMEPWD30` (save games), `VIS4MOVPWS` (movies): both wide strings in the executable, and in Deponia 2's too |
| Volumes | 74 `scenes/scene.vsNNN`, 84 `characters/character.vcNNN`, 42 `videos/video.vvNNN` (Matroska, VP8 and Vorbis, 25 fps; HD and SD) |
| Sound | Opus in Ogg; movie sound Vorbis |
| Rendering device named by `config.ini` | `Device` (Deponia 2 has no such key) |
| Libraries in `libs64` | ffmpeg 56, `libopenal`, `libpng12`, zlib, Galaxy |
| Shaders, particles, matrices | not used by the data (the engine has the code for them) |

## What the reconstruction has been checked on

The whole of it is on this build: the 74 scenes load, the start, the language select, the movies, the new game, the
tutorial and chapter 1 run headless (`tests/deponia1/playthrough.cpp`) and in the SDL2 player. The cursor, the action text and
the dialog fonts of the SDL2 player were compared with a recorded playthrough of the original by the person who
watched it (no automatic comparison exists).

## Differences to other builds

- Deponia 2 Linux: [`../deponia2/VERSIONS.md`](../deponia2/VERSIONS.md).
- Windows and macOS builds of Deponia 1: not looked at. The things to compare first are the graphics layer (Direct3D or
  OpenGL, `Device` in `config.ini`), the sound (OpenAL, XAudio), the movie player (ffmpeg or Media Foundation) and the
  fonts, and the paths of the save games and the logs.
