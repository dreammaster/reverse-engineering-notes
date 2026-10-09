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

`TGCharacter` stays `in-progress` in `manifest/proprietary_classes.tsv` until the
Spine branches are done.

### TGObject: particles, shaders and the matrix transform

`src/deponia1/TGObject.{h,cpp}`; original `src/vsplayer/` (object file, not
assert-confirmed).

Left out:

- the draw calls of the particle effect through the `graphics` backend (translate by the
  negated float scroll position times the scroll factor, an optional multiply with the
  global matrix, `ParticleContainer::Draw()`/the particle system draw, and the scroll
  fields the particle system is given at +0x2E0/+0x2E4);
- the matrix transform of `IsInside()` and `DrawSnoopAnimation()` while the object is drawn
  through a matrix (`kObjectMatrixId`, the global `invMatrix1`/`matrix1` with 9 entries);
- the second `TPictureIO` at +0x1B8 (cleared when an object with particles is deactivated;
  nothing else is known to use it).

`TGObject` stays `in-progress` in `manifest/proprietary_classes.tsv` until then.

### The particle systems of the editor (particleSystem.cpp, particlesGame.cpp)

`TParticleEmitter`/`TParticleSystem` (`graphicslib/particleSystem.cpp`) and `TGParticleEmitter`/`TGParticleSystem`
(`vsplayer/particlesGame.cpp`) are done, except `TGParticleEmitter::Save()` and `Load()` (asm 1378389-1378934,
1379800-1380801: the editor writes an emitter to a file and reads it) and the drawing (`TGraphicsOGL::Draw(TParticleSystem&, bool, bool)`
and `Draw(TParticleEmitter&, ...)`, `SetParticleMaterial`: asm 708150, 714258, 719135). A backend draws the particles of
`TParticleEmitter::_particles` with the rules at the top of `particleSystem.h`.

### TGInterface: the matrix transform of the cursor position

`src/deponia1/TGInterface.cpp`; original `src/vsplayer/interfaceGame.cpp`.

`GetObject(const wxPoint &)` and `IsInside()` first move the position back through the global
inverse matrix (`invMatrix1`, nine entries) when the interface is drawn through a matrix
(`kInterfaceMatrixId` not 0, and its parent's `kGameShaderExclude` is not 1); not
reconstructed. `TGInterface` stays `in-progress` until then.

### The text engine: the speech and the glyphs

`src/deponia1/TGText.cpp`, `src/deponia1/TSText.cpp`, `src/deponia1/vscommon/cfont.cpp`.

- The sound manager's side of the speech signals (`TSignalData.h`: `kSignalSpeech*`).
- Printing the glyphs: `TCFont::PrintText()` / `PrintTextLines()` (asm 1396036-1398227: the letters of a picture font through
  `TPictureIO::DrawWithSrcRect`, a TrueType font through `TFreetypeFont::RenderString`, with the text matrix and the scroll of
  the paint control; the buffers a text keeps are `GLCharBuffer`s) and `TCFont::EnsureSpriteLoaded()` (asm 1395566-1396036: the
  letters of the font picture with their transparent edges taken off, packed into a texture).
- `TFreetypeFont` (asm 1582405-1589320, `graphicslib/freetypeFont.h`): no TrueType font can be made, so the width of a text in
  a TrueType font is 0. The metrics of the layout (`TCFont::SplitIntoLines`, `GetTextDimension`) are done for both kinds of fonts.

`TGText` stays `in-progress` in `manifest/proprietary_classes.tsv` until the glyphs are printed (the Lua hooks are done).


## The Lua bridge: what is not reconstructed

Done: the Lua state and running scripts (`visLua.cpp`), the conversions (`luaConversion.cpp`, `lua.cpp`), the
constants (`luaGlobals.cpp`), sprites (`luaSprite.cpp`), data objects and the tables of the game
(`visionaireobjectLua.cpp`), the path language (`vscommon/objAccess.cpp`), the command framework and the common
commands, and the 49 commands of the player with `InitPlayerCommands()` (`vsplayer/scripting/playerCommands.cpp`).
Missing:

- `Init()` (not `InitScripts()`) calls `InitPlayerCommands()`, see "The player's start-up" in `src/deponia1/NOTES.md`. That function calls the part of `InitDrawLua()`
  that is done (`sha1`, `system`, `setDelay`: `luaSystem.cpp`); missing are the rest of it - the Box2D bindings
  (`tolua_b2_open`), the `graphics` object with its sprites, framebuffers, buffers and movies (`graphics_*`, `sprite_*`,
  `framebuffer_*`, `buffer_*`, `movie_*`: asm 438459-448600, all about the GL backend) - the `steam` object (asm
  286850-288180) and the libraries `utf8`, `luacurl`, `rex_pcre` and `lfs` that the original has built in.
- The GL backend (`g_subSys`, 299 uses in the asm; `graphicslib/subsys.h` has the two slots the commands use) is not
  reconstructed and nothing sets `g_subSys`: `shaderCompile` and `shaderUniform` keep the shader list but make no
  shaders, `system.systemInfo().gpu` is empty, `getGPUMem()` has only the base number, `system.cacheContents` is
  empty. `graphics->ToggleWindowMode()`, `SetWindowSize()` and `IsFullscreen()` are stubs for the same reason.
- The particle container's `Draw()` (`particlePipeline` and the vertex buffers), the texture atlas of `images` with more than one picture and the Serialize functions (editor only): `graphicslib/particleHaduken.cpp`, `vscommon/scripting/particles.cpp`. `Init()` in `AppFunctions.cpp` (asm 493088, 4300 lines; its sequence is written down in NOTES.md, it needs the GL window and `LoadAndInitGame` to run), `CleanUp()` (asm 492124, calls `ClosePlayerCommands()`) and `ParseCommandLine` are still stubs; `ShowFrame()` (the SDL event loop and a frame) is done.
- `CreateObjectPath()` (asm 1387666) is only needed by the editor; `maxlen(const wxString &)` (asm 1400772) is
  never called.

## TGAction: what is not reconstructed

All 104 commands of `TGAction::Execute()` are done (`vstables/eCommand.h`). What they need and is
missing elsewhere:

- The Lua bridge is partly there (see below): the script commands 137 and 138 now run their text through
  `LuaDoString()`, but nothing starts the Lua state yet (`InitPlayerCommands()` is not called), and the drawing
  functions of the scripts (`InitDrawLua()`) are missing.
- The sound engine: `TSoundBase` (the list of the sounds, pause/mute, the fades) and the bookkeeping of
  `TSoundFFMPEG` are done (`TSoundBase.cpp`, `TSoundFFMPEG.cpp`), but nothing plays: the streams
  (`soundengine::Stream`, OpenAL buffers, the FFmpeg decoding, `AudioDataSourceFFMPEG`, the thread that calls `Update()`)
  and the audio busses (`soundengine::AudioBus`, `BusActivate`/`BusValuesUpdate`) are not reconstructed.
  `TSoundFFMPEG::CreateStream()` is where a backend (ScummVM's mixer) gives the engine a `TSoundStream`.
- Command 122 (0x7A) is an if (`IsIFActionPart`) that has no entry in the jump table: it does nothing.
