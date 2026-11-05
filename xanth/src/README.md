# Companions of Xanth - standalone source

A readable, hand-written C++ implementation of Legend Entertainment's *Companions of Xanth* (1993, CD version), written so that it can later be
imported into ScummVM and used as the basis for the other Legend games. The goal of this folder is a project that builds on its own and plays the
whole game from the original data files; the ScummVM port starts only once that is true.

## Rules this project follows

* **C++11 at most.** No exceptions, no RTTI, no C++14/17 library features (no `std::make_unique`, `std::string_view`, `std::optional`, generic lambdas ...).
  Plain `std::vector`, `std::string`, `std::array` and similar are fine; each has a ScummVM equivalent (`Common::Array`, `Common::String`, ...), so keep their use
  simple and keep all of it behind the `common/` layer where that is practical.
* **Platform code is isolated** in `platform/` behind small interfaces (video, input, audio, file system, time). SDL2 is the standalone implementation; ScummVM
  replaces it with its `OSystem`, `Audio::Mixer` and `Common::File`. Nothing outside `platform/` includes an SDL header.
* **Sound**: digital audio (voice, sound effects) goes through an audio-stream interface that maps to ScummVM's mixer; music goes through a MIDI/OPL driver
  interface whose standalone implementation is a stub with comments saying where ScummVM's OPL (or MIDI) implementation plugs in.
* **Voice is optional.** `XANTH_01.VOC` is only on the CD version; when it is missing the engine disables speech and plays on.
* **No game data and no game text in the repository.** The engine reads the user's own game files. Structure that was compiled into the original executable
  (object table, attribute descriptors, handler tables, message ids, region tables, layout constants) is generated into `xanth/tables/` by `tools/` from the
  analysis database; strings are never copied.
* **Verified against the original code.** `../engine/host` runs the original executable (lifted to C++ on an emulated PC) and is the oracle: the same scripted
  input must give the same game state and the same screen (state and logic exactly; pixels exactly where practical, see `docs/verification.md`).

## Layout

```
CMakeLists.txt       project build (library + game executable + tests)
common/              types, byte streams, file lookup (maps to Common::SeekableReadStream / Common::File)
legend/              code shared by Legend engines: data formats, graphics, fonts, text, audio interfaces
xanth/               Companions of Xanth: object model, game logic, user interface, conversations, events
platform/            SDL2 implementation of video / input / audio / timing and a headless one for tests
tools/               generators for xanth/tables/ and comparison tools against the oracle
tests/               unit tests (ctest)
docs/                design notes (architecture, verification, milestones)
```

## Build

```
cmake -S xanth/src -B build -DXANTH_SDL2=ON      # needs SDL2 development files; -DXANTH_SDL2=OFF builds the library and tests only
cmake --build build
ctest --test-dir build                           # XANTH_DATA=<game folder> enables the tests that read the game files
build/xanth <game folder>
```

See `docs/milestones.md` for the order of work and the current state.
