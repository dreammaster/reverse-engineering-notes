# Comparing the reconstruction with the original executable

The question: can the sources be compiled for Linux x86-64 and compared with the original `Deponia` executable, to find code that
was written wrongly? **A byte for byte comparison of the whole executable is not possible**, and the reasons decide what is done instead:

- The original was made with GCC 4.7.2 (`readelf -p .comment Deponia`: Debian 7, glibc 2.13, libstdc++ 4.7 with the string with a
  reference count). The cross compiler available on Windows is clang. The two make different code for the same source (the
  instructions for `abs`, how loops and conditions are laid out, what is inlined), so even a perfect source would not be the same bytes.
- The original has wxWidgets, SDL, Lua, Box2D, ffmpeg wrappers, libpng ... linked in. The reconstruction has its own stubs (`WxStub.h`,
  `SdlStub.h`) and ScummVM's Lua; every function that touches a `wxString` is compiled to something else.
- The members of the classes are not at the offsets of the original (`TGameControl::_visionaire` is at `+0xc8` in the original, at
  `+0x58` in the reconstruction), so almost every function that reads a member differs in its bytes.

What does work, because a compiler does not change it and a wrong source does: the **functions that are called (in order), the numbers
(ids of fields, events, commands, sizes of allocations), the texts, the shape of the code**. The original has a full symbol table
(`.symtab` with 33,525 function symbols, 19,964 of them different names in `.text`, all with sizes), so the functions are matched by their
mangled names.

## Setup (once)

```
python tools/crosssetup.py          # C:\dev\crosstools: clang of the ziglang wheel, and the headers of Debian 7 (libstdc++ 4.7, glibc 2.13)
```

The Debian packages of GCC 4.7.2 itself (`gcc-4.7`, `g++-4.7`, `cpp-4.7`) are unpacked into the same `sysroot`. Their `cc1plus` is a Linux
program and cannot run on Windows; with WSL (`wsl --install`, needs a restart) or any Linux machine it could compile the sources with
the very compiler of the original, which would make many functions identical down to the bytes. Everything below also works with
that: change the compiler in `tools/crosscompile.py`.

## Running it

```
python tools/crosscompile.py deponia1            # build/cross/deponia1/*.o  (157 files, about 40 s)
python tools/crosscompare.py deponia1            # the summary, and build/cross/deponia1-compare.tsv (all functions, worst first)
python tools/crosscompare.py deponia1 --classes  # per class: functions in both, how many under 0.5, mean score
python tools/crosscompare.py deponia1 --worst 30 _ZN12TGameControl     # the 30 least alike functions of a class
python tools/crosscompare.py deponia1 --show _ZN12TGameControl14SavegameExistsEi   # both versions side by side
python tools/crosscompare.py deponia1 --missing TGScene                # original functions of a class that the reconstruction lacks
python tools/crosscompare.py deponia1 --callsdiff TGText               # the engine's own calls that the original has more (or fewer) of
```

`crosscompile.py` skips `WxStub.cpp` (Windows) and `luaStdLibs.cpp` (`<filesystem>`, which libstdc++ 4.7 does not have), and uses a
small `std::variant` (`tools/crossshim/variant`) for `argument.h`. `--tag NAME` and `-X flag` make a second set of objects (`-O1`,
`-fno-inline` were tried: `-O2` matches best).

A function gets a score from 0 to 1: calls 0.4, numbers 0.25, texts 0.15, shape 0.2. What the columns mean:

| column | what is compared |
|---|---|
| calls | the names of the functions called, in order (virtual calls by the slot; the constructor and destructor variants are alike) |
| consts | the numbers in the code, without the offsets added to pointers (they are the layout of the classes) |
| strings | the texts the function refers to (narrow and wide) |
| shape | the sequence of the kinds of instruction (the conditional jumps are alike) |

## Where it stood on 21 October 2026

2,801 functions are in both (the original has 19,964 names in `.text`; the reconstruction 3,903, 1,102 of them are helpers, stubs
and inlined functions that the original does not have under that name). Scores: 302 functions are 0.95 or more, 286 are 0.85 to 0.95,
428 are 0.70 to 0.85, 784 are 0.50 to 0.70, 1,001 are under 0.50. 2,400 have the same texts, 1,161 the same numbers, 361 the same calls
in the same order. Most of the low scores are noise of the kind above (inlining, `wxString`, offsets); the list is for a person to look
at with `--show`, not a list of bugs.

What the first looks found (all are in the reconstruction, none changes what the game does as far as is known):

- `TGameControl::SavegameExists` and `DeleteSavegame` make the `TMSavegame` with `new` (the number `0x1e8` is its size, and it is
  `sizeof(TMSavegame)` in the original); the reconstruction has it on the stack.
- The members of the classes are at other places: `_visionaire` `+0xc8` against `+0x58`, `_ownedSceneControl` `+0x298` against `+0x308`.
  If the layouts are to follow the original (they would have to for a drop-in replacement of the binary, not for a ScummVM engine), the
  displacements in the comparison give them directly.
- `TManagedObject::SetText` calls `TGText::SetOwner(TTextOwner *)` in the original, with an `x_assert`; the reconstruction has `SetOwner(TManagedObject *)`: the
  original has a base class `TTextOwner` that is not reconstructed.
- `TGText::SetTextIntern` has the same calls as the original but in another order and fewer `IsSpineAnimation` calls (the original has the
  speaker block twice); `--callsdiff TGText` shows more of the same kind.
- `TGameControl::SetDelay(double, int)` is 23 instructions in the original and 77 here: the original does something simpler than the
  container of the reconstruction.

Look first at the classes at the top of `--classes` that are big (`TVisionaireGame`, `TConsole`, `TXMLNames`, `TPreloadedPicManager`), and at
`--worst` of the classes that the game depends on (`TGCharacter`, `TGScene`, `TGAction`).
