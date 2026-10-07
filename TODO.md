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
