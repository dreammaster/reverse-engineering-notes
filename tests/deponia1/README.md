# Tests of the reconstructed engine

There is no game data, so the tests build small games in code (`TVisionaireGame::CreateObject()` ...), save them to a
VBIN file, load them through `THGameControl` and run frames (`TGAction::ContinueRunningActions()`,
`control->Update()`, `control->Draw(true)`, `control->ProcessMessage(msg, pos)`). Pictures have no decoder here: a file
made by `writeFakePicture()` (`fakepictures.h`) is a picture of the size asked for.

```
python tools/tbuild.py <test>.cpp [-32] [-g] [-c] [-r]
```

Builds the sources and the test into `build/tests-<bits>[g|c]/`; `-r` runs it in `tests/deponia1/work`, `-g` builds
for gdb (`PATH=/c/mingw64/bin:$PATH gdb -batch -x cmds build/tests-64g/<test>.exe`), `-c` builds with gcov.
Every test prints `<n> failures`. The 32-bit build (`-32`) counts in SSE (`-msse2 -mfpmath=sse`) like the 64-bit one: the x87 registers round
differently, and the exact comparisons of the geometry (`LineInPolygonSet`) then give another way round a corner.
(The objects are not rebuilt when the flags change: delete `build/tests-32` after changing them.)

| test | what it covers |
| --- | --- |
| upgradetest | `UpdateVersion` (the upgrades of older projects) |
| upgradeb3test | the upgrade of version 0xB3: the build rules for the containers |
| particletest | the particles of a scene and of its objects: what the backend is told |
| upgrade63test | the first upgrades (0x63 and after): visibilities, renumbered commands, swapped links, fonts, texts, dialog parts |
| pictest | the pictures in memory: pixels, mirror, parts, paste, scaling, the bitmap of the transparent |
| zlibtest | the zlib stand-in: streams made by the real zlib (fixed, dynamic, stored), refused bad streams |
| upgrade71test | the upgrade of version 0x71: the interfaces of the characters become the game's, with a class each |
| loadtest | load, save and load again, actions, items, scripts, a text, a dialog |
| savetest | `SaveSaveGame` / `LoadSaveGame` |
| clitest, gfxtest, movietest, soundtest | command line, graphics helpers (and the polygon test), movie, sound |
| animtest | animations of objects (fake pictures, timing) |
| dialogtest | a dialog with answers |
| dialog2test | dialogs that lead to others: a part that is used up and returns to the dialog, one that goes to another dialog and one back, one that ends the dialog |
| dialogscrolltest | a dialog of fourteen answers in an area of five: the mouse wheel scrolls a line (up to the first, down to the last), and the last answer is picked |
| replacetest | replaceGame("other.dat") from a script: a file that is not there fails, another game replaces the first and its start action runs |
| areatest | the action areas of a scene: the way in and the way out run once; an area for another character does not |
| charitemtest | items from a character to another (GiveItemTo, ReceiveItem needing an interface, GiveAllItemsTo), StandingAt, GetCurrentDimension, AlignToObject, the turn action |
| cmd3test | the commands SetFont, SetLightMap, SetItem, Save/Execute/ClearSavedObject, SetCursor, SetObjectActive (the active command stays on) and IfLanguage |
| cmd4test | the commands Autosave (saved at or after the part, loaded again, a slot above 99), IfSavegame, DeleteSavegame and GameSave |
| cmd5test | ShowCharacter, PreloadCharacter (a character in the shown scene is left alone), and the lists of running actions (by name, by id, printed, finished) |
| savemenutest | a menu scene with places for savegames: selecting by click or script, saving over (with the screenshot), reading the disk when the menu is entered, scrolling pages, deleting, loading |
| clicktest | clicks on an object and the floor, keys (`HandleKeyEvent`, key release) |
| commenttest | an object without actions clicked with a command: the hero says the comment for it |
| fadetest | every fade effect of a scene change |
| interfacetest | an interface with a button, an item in a place holder, the use command, an item used on an object, a command on a statue after a walk |
| waytest | walking around a wall along the way points; the free walk (controller) against a wall |
| way2test | the way round the end of a bar when no way point can be seen (shortestPath) |
| way3test | a wall that can be passed at both ends: the way goes along the way points (one with two relations); CheckPosition puts a position next to the floor on it |
| polytest | the polygon helpers: the list that owns its polygons, point lists with separators, bounding boxes, lines and points against polygons, the shortest path round corners and holes |
| easingtest | the easing equations of the tweens: 0 to 1, Out mirrors In, the halves of InOut, the known values, the exponential |
| basetest | the string handle (comparisons, ranges, copies), the growing byte buffer (numbers big end first, texts, zlib, the cipher) and the sprite description (reload needed, transparency, Lua text) |
| compmgrtest | the container manager: the main, game, scene, character, interface and movie containers, files by their references, volumes that are not there, export, the movie header, clean up |
| sound2test | the sound engine with streams that play nothing: playing, pausing, muting, stopping and keeping, the volumes, the fades, the console lines |
| matrixtest | the matrices the scripts set (graphics.invMatrix1, graphics.matrix1): the click goes back through the inverse one, points go through the matrix |
| partsimtest | the particle emitter simulation: counts, fractions, box and point geometry, directions and cones, forces, stage colours, size curve, particle kinds, depth sorting, the systems |
| luaparttest | the particle systems the scripts make with particleSystem:new{}: container and emitter settings, the kinds of emitter, assignments, curves (min/max pairs, appended), warm-up, pictures, image emitters |
| datatest | the typed values of the fields: the text of each kind, sizes in memory, comparison, copy, clear, and reading from characters (T/F flags, ints, paths) |
| cmdqueuetest | the commands that can be undone (the editor's): the queue, undo and redo one by one and as a group, deleting, the console text, wrong commands |
| canimtest | the playing of the pictures of an animation: forwards, backwards, by chance, loops, the pause of each picture, the wait of a loop by chance, the questions |
| fonttest | the fonts of pictures: letter widths, the space, kerning of pairs and single letters, the size of texts and lines, a font that stands for another, wrong kerning, the manager |
| chartest | the walk and stand animations of a character for all directions |
| texttest | a character says a text: talk animation, skip by click, end by time |
| charstest | two characters in two scenes: changing the controlled one moves the scene and the interface |
| gamesavetest | a game saved while an action waits and a character walks, and loaded again; the global script variables |
| skiptest | a cutscene (walk, text, wait, change of scene) is skipped with the escape key |
| controltest | IF / ELSE / END IF (nested), GOTO, values, conditions and arithmetic of an action |
| scrolltest | a scene twice as wide as the window: the view follows the walking character to both ends |
| menutest | a menu scene is shown while an action waits: the action is stopped for the time of the menu |
| menu2test | going to a menu freezes the game: a text and a wait go on with the time they had left when the game is entered again |
| cmdtest | the commands of an action that no other test runs: turn, place, walk, outfit, items, follow, scroll, move object, the switches of the game, a cutscene |
| cmdinterfacetest | the commands that work on interfaces: the active command, show / hide / fade an interface, another interface, the object under the mouse |
| talktest | a click on a second character in the scene: the hero walks to it and its action runs on arrival (a command that is used on characters is needed) |
| condtest | objects with conditions (variables, AND / OR, negated, nested): active when the condition is true, following it when a script changes a variable |
| cmdanimtest | the commands for animations: show and wait for the end, move (to, more, less), first and last frame, preload, hide |
| cmdtexttest | the commands for texts: at a point (waiting and in the background), at an object, skip, clear, wait for talking |
| cursortest | the cursor of a command: loaded and linked to the command, active over a button, inactive over the floor, set by id |
| luacmdtest | the Lua commands of the player: start / stop an action and an animation, volumes, cursor position, getObject / getTime / getProperty, hooks, created events; a script that fails |
| texttagtest | the pause tags of a text (`<p2>`, `<p500ms>`, `<pt>`, `<p>`): the time of each part, and the player's text speed |
| containertest | a container written with entries of every kind (plain, compressed, encrypted, both) and read back through the manager (memory file and file object); the wrong password |
| monkeytest | random mouse, key and wheel input on the interface game (600 steps; `MONKEY_SEED` and `MONKEY_STEPS` choose others - seeds 1-8 with 6000 steps were clean) |
| luatest | scripting commands: `object:to()`, event handlers, `isPointInsidePolygon` |
| texttest2 | the single text of a stopped game: `ContinueStoppedText`, `SetOwner`/`DetachOwner` |
| ttexttest | the text of a language (found, made), the languages of text and speech, `TTAnimation::SetMirrored`, `TTSprite::SetPosition` |

## Finding what is not tested

```
python tools/tbuild.py <test>.cpp -c -r      (for every test)
python tools/coverage.py [path-filter]       (the functions no test calls, longest first)
```

## What the tests found (so far)

Reading the disassembly of a function whose result is wrong in a test has found, among others: an inverted test in
`StartDialog`, `TGameControl::UpdateTexts` dropping a text that is still active, `StartText` with the flags of
speech and background swapped, `ExecuteMatchingAction` reading the command from the fixture, `GetActionsToTest`
with the wrong event 4, `ExecuteEvent` without the retry of a press as a click and without the item fallback,
`MD5String` returning the 16 raw bytes where the original returns the 32 hex characters (the key of the whole cipher of the containers), `InitGameActions` storing the wrong key for a key release, the window size being 0 (two members that are one field in the original), script variables never saved (`CreateActiveObject`), `ToScene`/`FadeIn`/`MoveScene`/`ScrollUpdate` being
stubs or approximations, an approximated `IsPointInsidePolygon`, `TGScene::SetCharacter` setting the position and direction without the event, `TGObjectManager::GetDetectInfo` reading the use of the command from the game object instead of the active command, an assertion for an empty polygon list that the original does not make. Later: the upgrade fixes 64/68/6C passing a comparison to the `bool` overload of `SetValue` (refused by an int field), finished object tweens never erased, an "any" link giving nothing (no action area ever ran), the particle container settings of a scene read from the wrong object, `TGAction`'s `format` printing one character for a wide `%s`, `TComposedFileManager::Export` giving the name and the extension the wrong way round, `wxFile::Open` mode 2 not making a file, and (32-bit only) the x87 rounding giving another way round a corner. The older "Confirmed (asm lines ...)" comments
are not proof: a test that does not do what a script of the game would do is the way to find the rest.
Test data that is wrong looks the same as an engine bug: `kActionPartInt` is not 0 by default (it makes an item be
taken instead of given, a text be shown in the background ...), and a scene point belongs to the way system
(`kWaySystemPoints`), not to the scene.
