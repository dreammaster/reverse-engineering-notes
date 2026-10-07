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
