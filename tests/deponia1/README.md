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
Every test prints `<n> failures`.

| test | what it covers |
| --- | --- |
| upgradetest | `UpdateVersion` (the upgrades of older projects) |
| loadtest | load, save and load again, actions, items, scripts, a text, a dialog |
| savetest | `SaveSaveGame` / `LoadSaveGame` |
| clitest, gfxtest, movietest, soundtest | command line, graphics helpers (and the polygon test), movie, sound |
| animtest | animations of objects (fake pictures, timing) |
| dialogtest | a dialog with answers |
| clicktest | clicks on an object and the floor, keys (`HandleKeyEvent`, key release) |
| fadetest | every fade effect of a scene change |
| interfacetest | an interface with a button, an item in a place holder, the use command, an item used on an object, a command on a statue after a walk |
| waytest | walking around a wall along the way points; the free walk (controller) against a wall |
| chartest | the walk and stand animations of a character for all directions |
| texttest | a character says a text: talk animation, skip by click, end by time |
| charstest | two characters in two scenes: changing the controlled one moves the scene and the interface |
| gamesavetest | a game saved while an action waits and a character walks, and loaded again; the global script variables |
| skiptest | a cutscene (walk, text, wait, change of scene) is skipped with the escape key |
| controltest | IF / ELSE / END IF (nested), GOTO, values, conditions and arithmetic of an action |
| luatest | scripting commands: `object:to()`, event handlers, `isPointInsidePolygon` |

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
`InitGameActions` storing the wrong key for a key release, `ToScene`/`FadeIn`/`MoveScene`/`ScrollUpdate` being
stubs or approximations, an approximated `IsPointInsidePolygon`. The older "Confirmed (asm lines ...)" comments
are not proof: a test that does not do what a script of the game would do is the way to find the rest.
Test data that is wrong looks the same as an engine bug: `kActionPartInt` is not 0 by default (it makes an item be
taken instead of given, a text be shown in the background ...), and a scene point belongs to the way system
(`kWaySystemPoints`), not to the scene.
