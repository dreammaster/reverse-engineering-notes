# Deferred work

Items consciously postponed, so they are not forgotten. Each is also marked
with a `TODO` comment at the relevant code. Grep for `TODO (low priority` to
find them all.

## Low priority - do at the end

### TCAnimation: 3D-model and Spine skeleton animation kinds

`src/deponia1/vscommon/canimation.{h,cpp}`; original `src/vscommon/canimation.cpp`.

The original `TCAnimation` plays three kinds of animation: sprite sequences,
3D models (`ModelContainer`/`ModelAnimation`) and Spine skeletons
(`SpineContainer`/`SpineSkeleton`). Only the sprite kind is reconstructed.
Deponia 1 uses nothing else, so it was left out, but the engine is meant to be
reused by other Visionaire games, which may need the other two.

What is missing:

- `IsModelAnimation()` and `IsBonesAnimation()` (currently fixed `false`),
  and the model/bones branches of the ctor, dtor, `Start()`,
  `FirstSprite()`/`NextSprite()`/`EofSprite()`, `SetCurrentSprite()`,
  `GetCurrentSpriteIndexOrTick()` (the model's tick), `GetFrameCount()` (the
  model's ticks / the skeleton's duration) and `GetCurrentSpritePosition()`.
- The fields for them: +0x38 rendered sprite of a model, +0x40
  `ModelAnimation`, +0x48 `SpineSkeleton`, +0x50 its animation, +0x58 a path,
  +0x60 the model's tick.
- The project's own wrapper classes, all still `todo` in the manifest:
  `ModelContainer` (13 methods), `ModelAnimation` (31), `Model` (37),
  `SpineContainer` (5) and `SpineSkeleton` (22). The libraries underneath are
  third-party (`Assimp` and the `Spine runtime` in `vendor_libraries.tsv`) -
  link or stub those, don't reimplement them.

Until this is done, an animation of those kinds plays no sprites, and
`TCAnimation` stays `in-progress` in `manifest/proprietary_classes.tsv`.

### TGAnimation: the model and Spine branches (part of the item above)

`src/deponia1/vsplayer/animationGame.{h,cpp}`; original
`src/vsplayer/animationGame.cpp`.

`TGAnimation` is layered on `TCAnimation` and has its own model/Spine branches,
all left out for the same reason:

- the ctor loading the model (`ModelContainer::GetInstance()->ReadLoadedModel()`);
- `NextSpriteSelected()` matching the frames of a model by tick
  (`ModelAnimation::GetCurrentTick()/GetLastTick()`) and of a skeleton by time
  (`SpineSkeleton::GetTime()`), instead of by sprite index;
- `Prepare()`/`Draw()`/`DrawWithLightMap()` updating and drawing a
  `ModelAnimation`, and `DrawMixed()` (Spine only) updating and mixing
  `SpineSkeleton`s - `DrawMixed()` is an empty function until then.

`TTAnimation::IsModelAnimation()`/`IsBonesAnimation()` (the data-object tests the
start functions use) are reconstructed; `TCAnimation::IsModelAnimation()`/
`IsBonesAnimation()` (the instance tests) are fixed `false`.

### TGAnimation: the debugger overlay

`GetAnimationDetails()`, `PrintRunningAnimations()` and `DrawAnimation()` (asm
lines 154252-156548) list the running animations as text lines and draw a
thumbnail of each in the debugger overlay. Not reconstructed (not declared at
all); the manifest keeps them `todo`.

### TGCharacter and TMCharacter: Spine skeletons and the matrix transform

`src/deponia1/TGCharacter.{h,cpp}`, `src/deponia1/TMCharacter.{h,cpp}`; original
`src/vsplayer/characterManaged.cpp` and the character file of `vsplayer`.

Left out, for the same reason as the model/Spine items above:

- the branches for a character shown as a Spine skeleton (`IsSpineAnimation()`):
  `StartCharacterAnim()` starting a second animation on top of the first (the
  secondary list `_animations` and the kinds `_animKinds[1...]`), `StopCharacterAnim()`
  ending them by kind, `GetCurrentSpriteRect()` (the skeleton's size, scaled by
  `kAnimationSize`), `SetSpritePosition()`/`UpdateSpriteRect()`/`Draw()` positioning
  a model or skeleton animation;
- the matrix transform: `TMCharacter::IsInside()` moves the point back through the
  inverse matrix while a character is drawn through one (`kCharacterMatrixId` 1),
  and `TGCharacter::Draw()` works with a matrix of its own (the part reconstructed
  only turns the global matrices off for a character that has none);
- the Lua hook `CharacterDirectionHook` that `GetDirectionIndex()` asks before its own
  search (needs the Lua bridge, like the other hooks).

`TGCharacter` stays `in-progress` in `manifest/proprietary_classes.tsv` until the
Spine branches are done.

### TGObject: particles, shaders and the matrix transform

`src/deponia1/TGObject.{h,cpp}`; original `src/vsplayer/` (object file, not
assert-confirmed).

Left out:

- the particle container built from a script expression (`SetActive()` runs
  `return particleSystem:new(<kParticleContainerSettings>)` through `LuaDoString()` and
  adopts the userdata it returns) - needs the Lua bridge, like `TGScene::BeginScene()`;
- the draw calls of the particle effect through the `graphics` backend (translate by the
  negated float scroll position times the scroll factor, an optional multiply with the
  global matrix, `ParticleContainer::Draw()`/the particle system draw, and the scroll
  fields the particle system is given at +0x2E0/+0x2E4);
- the matrix transform of `IsInside()` and `DrawSnoopAnimation()` while the object is drawn
  through a matrix (`kObjectMatrixId`, the global `invMatrix1`/`matrix1` with 9 entries);
- the second `TPictureIO` at +0x1B8 (cleared when an object with particles is deactivated;
  nothing else is known to use it).

`TGObject` stays `in-progress` in `manifest/proprietary_classes.tsv` until then.

### TGInterface: the matrix transform of the cursor position

`src/deponia1/TGInterface.cpp`; original `src/vsplayer/interfaceGame.cpp`.

`GetObject(const wxPoint &)` and `IsInside()` first move the position back through the global
inverse matrix (`invMatrix1`, nine entries) when the interface is drawn through a matrix
(`kInterfaceMatrixId` not 0, and its parent's `kGameShaderExclude` is not 1); not
reconstructed. `TGInterface` stays `in-progress` until then.

### The text engine: the Lua hooks and the speech

`src/deponia1/TGText.cpp`, `src/deponia1/TSText.cpp`.

- `TGText::CalculateRestText()` calls the Lua function registered by
  `RegisterHookFunctionText()` ("TextTextHook") with the text's record and takes the string it
  returns as the part to show; `CalculateTextPos()` calls the one registered by
  `RegisterHookFunctionSetTextPosition()` ("TextPositionHook", a true answer ends it) and `Draw()`
  the one of `RegisterHookFunctionRender()` ("TextRenderHook": the lines, their widths, the position,
  alignment, alpha and the wrap flag; a true answer means the script drew the text). Needs the Lua
  bridge (`LuaExecuteFunction()` and `TArgument`).
- The sound manager's side of the speech signals (`TSignalData.h`: `kSignalSpeech*`) - `TSoundBase`
  and `TSoundFFMPEG` are not reconstructed.
- Printing the glyphs: `TFontManager::PrintTextLines()` and `TCFont` (the buffers a text keeps are
  `GLCharBuffer`s).

`TGText` stays `in-progress` in `manifest/proprietary_classes.tsv` until the hooks are done.


## The Lua bridge: what is not reconstructed

Done: the Lua state and running scripts (`visLua.cpp`), the conversions (`luaConversion.cpp`, `lua.cpp`), the
constants (`luaGlobals.cpp`), sprites (`luaSprite.cpp`), data objects and the tables of the game
(`visionaireobjectLua.cpp`), the path language (`vscommon/objAccess.cpp`), the command framework and the common
commands, and the 49 commands of the player with `InitPlayerCommands()` (`vsplayer/scripting/playerCommands.cpp`).
Missing:

- `TGameControl::InitScripts()` has to call `InitPlayerCommands()`. That function does not yet call
  `InitDrawLua()` (asm 447452: the `system_*`, `graphics_*`, `sprite_*`, `movie_*`, `particles_*` functions) or open
  the libraries `utf8`, `luacurl`, `rex_pcre` and `lfs` that the original has built in.
- The shader commands (`shaderCompile`, `shaderUniform`) keep the shaders in `shader_list`, but `CreateShader()`
  (`graphicslib/shader.cpp`) makes none: the shader objects belong to the GL backend behind `graphics`
  (`g_subSys`, not reconstructed). `graphics->ToggleWindowMode()`, `SetWindowSize()` and `IsFullscreen()` are
  stubs for the same reason.
- `luaopen_Particles`. `Init()` in `AppFunctions.cpp` (asm 493088, calls `InitPlayerCommands()`) and `CleanUp()` (asm 492124, calls `ClosePlayerCommands()`) are still stubs.
- `CreateObjectPath()` (asm 1387666) is only needed by the editor; `maxlen(const wxString &)` (asm 1400772) is
  never called.

## TGAction: what is not reconstructed

All 104 commands of `TGAction::Execute()` are done (`vstables/eCommand.h`). What they need and is
missing elsewhere:

- The Lua bridge is partly there (see below): the script commands 137 and 138 now run their text through
  `LuaDoString()`, but nothing starts the Lua state yet (`InitPlayerCommands()` is not called), and the drawing
  functions of the scripts (`InitDrawLua()`) are missing.
- The sound engine (`TSoundBase`, `TSoundFFMPEG`): the commands call the sound manager the way the
  original does, but nothing plays (see `TSoundInterface.h` for the virtual surface).
- Command 122 (0x7A) is an if (`IsIFActionPart`) that has no entry in the jump table: it does nothing.
