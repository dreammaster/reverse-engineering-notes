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

### The loading screen (`FillLoadingScreen`, `TLoadingControl::Init`, `ShowLoadingScreen`; 18 Nov 2026)
Reconstructed from the listing (the three games read the same): `SLoadingScreen` is two `TSprite`s (the picture of the screen, `kLoadingSprite`, +0x00; the status bar, `kLoadingStatusSprite`, +0x50), whether the bar fills (`!kLoadingStatusCover`, +0xA0), the sound (`kLoadingMusic`, +0xA8), the place of the bar (+0xB0) and the window's size (+0xB8/+0xBC, put there by `ShowLoadingScreen`). `Init` loads the two pictures, puts the screen's in the middle of the window, starts the sound (volume 100, looping, music), puts the bar in its place and sets the part of it that is drawn (all of it; none for a bar that fills). The three functions were empty before: the loading screen drew nothing. Checked only that the picture is loaded (`sys_loadingscreen.webp` in the rabbit); the frames of the engine's own screen are not swapped before the game's first frame in the stand-alone player.

### The lists of the console (`PrintScenes`, `PrintCharacters`, `PrintConditions`, `PrintValues`; `PrintSounds` of the rabbit)
`vstables/recordPrints.cpp`, read in the listing (the four functions read the same in all three games: the tables 4, 0, 10 and 20, the fields 0x252/0x124/0x13B/0x13C/0x84, 0x1F6/0x253/0x1F7, 0xC2 and 0x148): each makes the lines of its table, sorted by name (`cmpName`: `TCharHolder::Cmp` < 0): "All Scenes:", for each `Scene: "name" (Id: n), Menu|Playable Scene, Brightness: n%`, the music line and an empty one; the characters (`Active: T|F`) and conditions (`Value: T|F`) and values likewise. An empty table says "This Game Has No ..." (the characters say "Scenes": the original's own message). `tests/deponia1/printtest.cpp` prints them for the real data. The rabbit's `TSoundBase::PrintSounds` has no fade text in its streaming line (`Streamed: T`). `tools/asm_strings.py` lists the strings a function refers to (wide ones too).

### `~TManagedObject` (25 Nov 2026; found by loading a savegame of the first room)
Loading the second savegame (and bookmark) of the first room crashed the stand-alone player: `TGAnimation::ClearAnimations` (called by `TGameControl::Load` through `LoadAnimations`) deleted a running animation, whose destructor tells its owners (`NotifyOwnersAnimationFinished`), and one of them was an object that no longer existed. The destructor of `TManagedObject` was `= default`; the original's (asm 192299-192394) lets the object go of everything that refers to it: `HideAnimation(_currentAnimation, this)`, `TGObjectManager::NotifyObjectRemoved(_objRef)`, `DetachOwner(this)` on each secondary animation and `TGText::DetachOwner` for its text. Now reconstructed that way (`TManagedObject.cpp`); Deponia 4 has the same destructor (asm 417700), the Rabbit's also hides each secondary animation after detaching (rabbit.asm 279496), so all three trees have it. Found with `tests/deponia1/slotload.cpp` (`slotload <b|s> <slot>`, run from the game folder with `GAME_PASSW`; it loads one slot of the real data headless; the savegames must be in `userdata/VisionaireStudio/Savegames`, the stub's directory, not the player's `userdata/Daedalic Entertainment/Deponia/Savegames`). Checked: the four slots of the first room's savegames crash at slot 1 (both kinds) before and load after; `loadtest` and the Rabbit's `plugintest` still pass. Not checked in the SDL player itself.

### The direction on each leg of a way (`TGWaySystem::GetNextPosition`; 25 Nov 2026)
Reported while play-testing: Rufus, loaded in the lower part of his home and sent to the top right, walked up the ramp (and its steeper part) still facing down-right. `GetNextPosition` works out the direction of a leg of the way when the leg starts, but it did so only when its "known" flag (+0x64) was clear. The flag is set when a step does not reach its target and is cleared only when the way goes on to the destination, so a way along several points kept the direction of its first leg. In the original (asm 249540-249794; `-O0` Deponia 4: `if (reachedTarget || !known)`) the direction is also worked out whenever the character stands exactly on the point it was heading for (the path 586618 -> 58664F -> 586670 calls `GetAngle` without looking at the flag). Now the same. Checked with `tests/deponia1/slotload.cpp` (`slotload s 1 1850 1350`: loads the savegame and walks the current character to a scene point, printing the direction every 10 steps): before, 336 on every step up to a later point; now 336, 36, 3, 138, 142, 60, 13, 344 as the legs change (up-right on the ramp, up-left on the steeper part). `tests/deponia1/waylegtest.cpp` checks the angles 0, 90, 180 of a way of three legs (3 failures without the fix). Applied to the Deponia 4 and Rabbit trees too (their originals have the same logic).

### The animation of a character's outfit (`TGAction::ShowAnimation`; 26 Nov 2026)
Reported while play-testing: combining the mousetrap and the provisions closes the inventory, but Rufus stands still instead of playing `combine_items_dv_r` (an animation of the outfit `Rufus_standard`, shared by the combining actions of the whole game). The action ran (the animation was started, its frame 26 started its action, the inventory came back), but on the wrong character: `ShowAnimation` looked the character up with `GetParent()` three steps above the animation (animation, outfit, character, *its* parent), the original two (asm 560610: `GetParent` twice from the animation). The extra step found no character and `GetCharacter` fell back to another one (`1008_Hannek` in this savegame), which played the animation out of sight. Now `GetCharacter(outfit.GetParent())`; `HideAnimation` already had it right. Found by comparing `rufus` (`GetCurrentCharacter`) with the `this` of `StartCharacterAnim` (gdb), and checked in the SDL player: the region of Rufus changes by 20-24 units between frames 220 and 410 where it did by 1-3, and the frames show him turned and working his hands over the items. Deponia 4 and the Rabbit have the same lookup in their originals (two `GetParent` calls before `GetCharacter`) and the same fix.

Debug aids added: `DEPONIA_ACTIONS=<frame>:<id of an action (table 7)>;...` in the Deponia 1 SDL player starts the action at that frame (and turns the traces on); `-s 0` loads `savegame00.dat` of `./userdata/Daedalic Entertainment/Deponia/Savegames` (relative to the current directory, so a scratch directory with a copy of a savegame as slot 0 does not touch the real ones); `tests/deponia1/slotload.cpp` takes `act <id>` (runs an action headless and prints what the current character plays), `tests/deponia1/namefind.cpp <text>...` lists the objects of the tables whose name has the text (items are table 6, actions table 7, animations table 9).
