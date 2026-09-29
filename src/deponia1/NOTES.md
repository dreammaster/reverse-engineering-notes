# main() reconstruction notes

## TGameControl batch: reference-return accessors, and a custom character hash table

While reconstructing TGameControl's shortest methods (address-gap analysis,
asm lines ~455750-462554), two patterns worth flagging for future passes:

- **Several "getter" methods return a reference/pointer to a member, not a
  copy**, even though `manifest/proprietary_functions.tsv` had them
  classified with by-value return types (`std::vector<TGCharacter*>`,
  `wxString`). The tell: the function body is a bare `lea rax,[rdi+OFFSET];
  retn` with no second hidden-pointer parameter - a real by-value return of
  a non-trivial type would need the Itanium ABI's RVO calling convention
  (caller-supplied destination pointer in rdi, `this` moved to rsi).
  `GetAllCharacters()` and `GetGamePath()` were both fixed from by-value to
  by-reference for this reason; worth checking any other accessor whose
  manifest signature returns a non-trivial type before assuming it's a copy.
- **TGameControl has a hand-rolled hash table mapping character IDs to
  `m_characters` indices** (`GetCharacter`/`GetCharacterPointer`/
  `GetCharacterPointerEx`, asm lines 456360-456578): `TVisObjRef::GetId()`
  returns a 3-byte identifier that gets packed into a 32-bit hash (byte 0 |
  byte1<<8 | sign-extended byte2<<16-24), divided by a bucket count at
  `+0x378`, and walked as a singly-linked chain (`node+0`: stored hash for
  comparison, `node+4`: a 32-bit index into `m_characters` at `+0x340`,
  `node+8`: next). Left as stubs (falling back to `m_currentCharacter` on an
  empty ref, `nullptr` otherwise) rather than guessing at the node struct
  layout or how entries get inserted - a real implementation would probably
  just use `std::unordered_map<int, TGCharacter*>` for equivalent behavior
  without reproducing the exact bucket mechanics, per the project's
  behavioral-fidelity-over-binary-fidelity stance (see the COW-string-ABI
  note below).
- **Controller buttons map into a custom keysym space starting at
  1000001**: `ConvertControllerButtonToSymKey` (asm lines 457042-457058)
  looks up a 15-entry table (`CSWTCH_876`) that's just `1000001 + button`
  for buttons 0-14, -1 otherwise - presumably reserved above the Unicode
  range used for regular keyboard key codes elsewhere in the engine.

## TGameControl batch 2: TSceneControl::GetScene() really returns TGScene*

Continuing the short-method sweep (asm lines ~456960-461870) turned up one
more manifest-signature correction and two new classes:

- **`TSceneControl::GetScene()` (and therefore `TGameControl::GetScene()`,
  which tail-calls it) returns `TGScene*`, not `TPaintControl*`** - every
  caller immediately uses the result as `TGScene::IsMenu()` or
  `TGScene::GetObject()`, which only compile/make sense if the real type is
  the more specific one. `TGScene` was added (deriving from `TPaintControl`,
  matching `TCursorControl`/`TLoadingControl`'s pattern) with just the two
  confirmed methods.
- **`TMasterControl::m_visionaire` needed to move from private to
  protected**, same reasoning as `m_sceneControl`
  (`SkipCurrentText`/`UpdateAspectRatio`/`ResetState` all call
  `m_visionaire->GetGame()` directly from TGameControl, with no
  TMasterControl accessor in between).
- **A small `TSText` class** was added for the "currently displayed text"
  pointer (`IsTextActive`/`IsNoTextDisplayed`, asm lines 461674-461777):
  null when no text is showing, otherwise exposes `GetDataObject()` and a
  target `TVisObjRef` field at a fixed offset (same "first member is a bare
  TVisObjRef" pattern as `TGDialog` - see below).
- **Deferred**: `IsTalking`/`ClearCurrentText` both walk a `std::list<TGText*>
  m_activeTexts` (confirmed present at asm offset +0x388, right before the
  already-known +0x398 list and +0x3A8 `TGDialog` - all three were
  sentinel-initialized together in the constructor). Left unstubbed for a
  future pass since it needs a `TGText` class (`GetSpeaker()` at minimum)
  that hasn't been reversed at all yet, rather than guessing at its shape.
- `TVisObjRef::operator==` was added (confirmed used at several of these
  call sites) - it compares the 4-byte id array from `GetId()`.

## TGameControl batch 3: TKeyboardMessageEnum has more values than first guessed

`HandleMouseHolding`, `ExecuteStartingAction`, and the controller-button
handlers (asm lines 455671-455746, 458052-458116, 471825-471975) all turned
out to be simple once `TGAction::AddRunningAction`/`ContinueRunningActions`/
`ClearActions` existed as stubs. One correction: `TKeyboardMessageEnum` was
guessed as just `{KeyDown, KeyUp}` when `TMasterControl` was first written,
but `HandleControllerButtonHit`/`HandleControllerButtonRelease` pass the
literal values 4 and 5 as this same enum type to `HandleKeyEvent` - so it
has at least 6 members. Extended with explicit values
(`ControllerButtonHit = 4`, `ControllerButtonRelease = 5`) rather than
renumbering `KeyDown`/`KeyUp`, since nothing contradicts those two; values 2
and 3 haven't been observed at any call site yet.

`ClearObjectText`/`IsTalking`/`ClearCurrentText`/`ReattachSceneObjectTexts`
are deferred together - they all walk one of two confirmed
`std::list<TGText*>` members (`+0x388` and `+0x398` in the original,
sentinel-initialized right before `TGDialog` in the constructor) and need a
real `TGText` class (with a `TVisObjRef` field, a `GetSpeaker()`, and at
least one still-unnamed virtual method) plus a `TManagedObject` class
(`TGScene::GetObject`'s real return type, not `void*` as currently stubbed)
- worth reversing `TGText` properly in one pass rather than guessing at its
shape piecemeal across four call sites.

## TGameControl batch 4: the two mystery TMasterControl lists are interface lists

`GetInterface`/`GetAllInterfaces`/`GetActiveInterfaces`/`GetObject` (asm
lines 456603-466193) resolve the two `std::list` members `TMasterControl`
carried since the very first pass without a confirmed element type or full
purpose (see the old "Two std::list members confirmed present" note, now
superseded and removed): they're both `std::list<TGInterface*>` -
`m_allInterfaces` (every registered interface, formerly `m_unknownList`)
and `m_activeInterfaces` (the ones currently drawn, formerly `m_interfaces`,
also used by `DrawInterfaces` - which now actually calls `Draw()` on each
one instead of a no-op placeholder loop). `TGInterface` was added deriving
from `TPaintControl` (confirmed: each list element exposes a Draw-like
virtual at vtable slot 1, the same Prepare@0/Draw@1 pattern as
TCursorControl/TLoadingControl/TGScene) with a `TVisObjRef` id field and a
`GetObject()` lookup.

Both list members moved from private to protected on `TMasterControl` for
the same reason as `m_sceneControl`/`m_visionaire` earlier - TGameControl
reads them directly with no accessor in between.

Also: `TSceneControl::GetScene()` is const in the original
(`_ZNK13TSceneControl8GetSceneEv`) despite returning a non-const `TGScene*`
- a `const_cast` inside it reflects that "logical const, physical mutable
accessor" shape rather than fighting it.

## TGameControl batch 5: finishing the text subsystem (TGText/TSText/TManagedObject)

Closes out the deferred quartet from batch 3 (`IsTalking`, `ClearCurrentText`,
`ReattachSceneObjectTexts`, `ClearObjectText`, asm lines 461785-462228) by
giving `TGText` a real shape instead of leaving it unmodeled:

- **`TGText` derives from `TSText`.** Both classes exhibited the exact same
  two clues independently - a `TVisObjRef` target field accessed at a fixed
  offset with no accessor in the original, and a shared mystery virtual
  method at vtable slot `0x28` that `ClearCurrentText` (on a `TSText*`) and
  `ClearObjectText` (on a `TGText*`) both call right before dropping a text.
  Rather than duplicating that field+virtual in two unrelated classes, the
  simpler and better-supported model is a common base - `TSText` now owns
  `GetTarget()`/the speculative `Discard()` virtual, and `TGText` just adds
  `GetSpeaker()`.
- **`TGCharacter` gained a `TVisObjRef` id field** (`GetRef()`), needed
  because `TGText::GetSpeaker()`'s return value is compared against a
  character reference the same "TVisObjRef at a fixed offset" way.
- **`TManagedObject`** was added as `TGScene::GetObject()`'s real return
  type (confirmed: `ReattachSceneObjectTexts` calls
  `TManagedObject::SetText(TGText*)` directly on it) - the earlier `void*`
  in both `TGScene::GetObject` and `TGameControl::GetObject` was a
  placeholder guess.

Two field ids (`0x2AC` on the scene-text target, `0x1DD` on the current
text) and one type-tag check (`id[3] == 6`) remain unresolved, same as the
several other opaque `TVisObjRef` field ids already catalogued elsewhere in
this file.

## TGameControl batch 6: ConvertControllerAxisToUnicode returns wxString, not int

`ConvertControllerAxisToUnicode` (asm lines 457066-457125) turned out to
write through a hidden return-value pointer at the very start of the
function - a dead giveaway that its real return type is a non-trivial
by-value type, not the `int` the manifest inferred. It's a 6-case switch
over `SDL_GameControllerAxis` returning the axis's name as a `wxString`
("LEFTX"/"LEFTY"/"TRIGGERLEFT"/"TRIGGERRIGHT" confirmed byte-for-byte from
the binary's string data; "RIGHTX"/"RIGHTY" inferred from the same naming
pattern but not individually checked), empty for anything else.
`HandleControllerAxis` (asm lines 471722-471817) uses that name as the
`HandleKeyEvent` key, skipping the call entirely when the axis is
unrecognized - the same "empty name -> skip" shape as the controller-button
handlers' literal `dword_D75F5C` empty string, just non-empty here. This is
`TKeyboardMessageEnum`'s 7th confirmed value (`AxisMove = 6`).

The scaled axis value itself is `value * 100 / 32768` (SDL's +-32768 raw
range down to roughly +-100) - the disassembly computes this two different
ways depending on the sign of `value` (a magic-multiply for the `>0`
branch, a shift-with-rounding-bias for the `<=0` branch), but both are
verified equivalent to that one expression, so the C++ just writes it
once.

## TGameControl batch 7: savegames, dialog cursor handling, the text/action queues

Eleven more methods (asm lines 456183-463466), pulling in a handful of new
pieces:

- **`TMSavegame`**, a per-slot savegame object (`Exists()`, `Delete()`, a
  static `SavegameExists()`), and `TGScene` gained
  `GetSelectedSavegame()`/`GetSavegameAt()`/`DeleteSelectedSavegame()` -
  `SavegameExists`/`DeleteSavegame`'s slot parameter turned out to have 3
  special negative values (-1: scene's selected save, -2: whatever's at a
  new `m_savegameClickPos` field, -3: "does any save exist at all", a
  static query) in addition to real slot numbers.
- **`StartDialog`/`EndDialog`** turned out to share a byte-packing pattern
  (a `TVisObjRef::GetId()`'s 3 bytes packed into a signed 32-bit "cursor id"
  for `TCursorControl::SetCursor()`) also seen in the still-deferred
  `GetCharacter` hash lookup - factored into a shared `PackVisId()` helper
  in gameControl.cpp rather than duplicating the bit-twiddling three times.
  Also surprising: `StartDialog` is a no-op unless a dialog is *already*
  active (`m_dialog` not empty) - it only switches/redirects an existing
  conversation, it doesn't open one from cold.
- **`TSText` gained two more members**: `CalculateCurrentText()` (the
  per-frame text-timing update `UpdateTexts()` drives) and a second unnamed
  virtual `OnCleared()` (vtable slot `0x10`, distinct from `Discard()`'s
  `0x28`) called right before `Discard()` when a text is dropped by direct
  reference (`ClearText`) or during a full flush (`ClearTexts`) - but
  *not* when `ClearCurrentText()` drops the current text on its own.
- **`SGameAction`** (`StartGameAction`'s lookup table entry) got real
  fields: `(a, msg)` to match against, a `flag` selecting between
  always-fire and only-fire-when-not-dialog-or-text-blocked behavior, and
  the `TVisObjRef` action target itself.
- **`PushEngineEvent`** turned out to be backed by a genuinely global (not
  per-`TGameControl`) locked queue (`EngineEventLock`/`EngineEvents` in the
  disassembly) - modeled as file-scope statics in gameControl.cpp since
  nothing else references them yet.

`InitInterfaces` (asm lines 458124-458250) is deferred: it needs a new
`THInterface` class, a `TVisionaireObject` type, `TVisObjRef::GetLinks()`,
and `TVList` growing real `begin()`/`end()` iteration - a bigger unit of
work better done as its own pass, similar to the earlier `TGText` subsystem.

## TGameControl batch 8: DisplayTexts and CenterScene make TGCharacter polymorphic

Finishes `DisplayTexts` (the last of the 8 pure virtuals to get a real
body) and `CenterScene` (asm lines 455890-456037, 460533-460715):

- **`TGCharacter` is polymorphic**, with two new virtuals discovered via
  `CenterScene`'s vtable-indexed calls: `GetScreenPosition()` (returns a
  `wxPoint`, compared against a `{-1,-1}` "no valid position" sentinel) and
  `GetVisibleRect()` (returns a `wxRect` - confirmed to be a plain 4-int
  struct specifically because the disassembly returns it in the RAX:RDX
  register pair rather than through a hidden pointer, which only happens
  for all-integer aggregates of 16 bytes or less). Names/real purpose
  unconfirmed.
- **`TPaintControl::GetVisibleSize()`** is a reference-returning accessor
  (same "logical const, physical mutable" shape as `GetScrollPos()`) - the
  tell was the disassembly dereferencing its return value as `[ptr]`/
  `[ptr+4]` rather than reading a packed register value.
- **`TGScene` gained its own identifying `TVisObjRef`** (`GetRef()`) -
  `CenterScene` only proceeds when the current character's scene-link
  matches it, following the same "bare `TVisObjRef` field, no original
  accessor" pattern as `TGCharacter`/`TGDialog`/`TSText`/`TGText`.
- **`TSText`'s two unnamed virtuals now have a third sibling call pattern**:
  `DisplayTexts()` removes any active text that's stopped being
  "displayed" (field id `0x211`) using the same `OnCleared()`+`Discard()`
  pair as `ClearText`/`ClearTexts`, before drawing what's left.
- Added `wxPoint::operator==`/`!=` and `wxRect::GetHeight()`/`IsEmpty()` to
  `WxStub.h` - needed by the above and not previously used anywhere.

## TGameControl batch 9: SkipCurrentText, and a caught wrong guess (StartTween)

`SkipCurrentText` (asm lines 456762-456956) turned out to be a clean
composition of pieces already built for `ClearCurrentText`: a field id
(0x1E0) normally blocks skipping unless another field id (0x235) overrides
it, then it delegates to `TSText::SkipCurrentText()` and only clears the
text (same 0x1DD field-clear + `Discard()` as `ClearCurrentText`) if the
text actually finished as a result.

While looking at `StartTween(const TVisObjTween&)` next (asm lines
474993-475136), its real body turned out to search a `std::vector` of
176-byte elements for a matching id+target, then erase-or-replace-or-append
- clearly not the vector of `std::pair<Tween, std::string>` guessed for
`m_pendingTweens` early in the project. Worse, checking the *other*
overload, `StartTween(const Tween&, const std::string&)` (asm lines
478477-478598+), showed it operates on a **completely different** vector of
88-byte elements at a different offset, keyed by a string comparison with a
non-trivial erase on a match - meaning the early guess that both overloads
shared one simple `m_pendingTweens.push_back()`-style vector was wrong on
both counts. Reverted `StartTween(const Tween&, const std::string&)`'s body
to a stub rather than keep a confidently wrong `push_back` masquerading as
correct - **a live example of the lesson below**: this project would rather
have an honest gap than a plausible-looking wrong answer. `m_pendingTweens`
stays declared as-is (its exact type doesn't affect anything else) until a
dedicated pass reverses both vectors' real element layouts.

## TGameControl batch 10: StartObjectText/StartBackgroundText reveal TManagedObject is a base of TGCharacter

Both text-starting methods (asm lines 461420-461589, 462233-462396) create
a new `THText` (a concrete `TGText` added for this - constructor parameter
roles are confirmed by call shape only) and need to attach it somewhere:

- **`TGCharacter` derives from `TManagedObject`.** `StartObjectText`'s
  fallback path calls `GetCharacterPointerEx()` and then calls
  `TManagedObject::SetText()` directly on the resulting `TGCharacter*` -
  only possible if `TGCharacter` IS-A `TManagedObject`. This also fixes
  `TGInterface::GetObject()`'s return type the same way `TGScene::
  GetObject()` was fixed earlier (confirmed `TManagedObject*`, not `void*`).
- `StartBackgroundText` dedupes against a character already speaking (via
  `TGText::GetSpeaker()` pointer equality against both `m_currentText` and
  every entry in `m_activeTexts`) before creating a new text - skipping
  entirely if a match is found.
- `TVisionaire` gained `CreateActiveObject(int, const TVisObjRef&)` and
  `GetEmptyObject()`, both confirmed by call shape only.

## TGameControl batch 11: SetInterfaces and SetCharacterActiveCommand

Two more interface/character-command methods (asm lines 465533-465671,
465679-465841):

- `SetInterfaces()` rebuilds `m_activeInterfaces` wholesale from the
  current character's own interface list (`TGCharacter::GetInterfaces()`,
  new), calling `TGInterface::RemoveSpritesAndAnimations()` on any
  interface that's leaving the active set first.
- `SetCharacterActiveCommand()` walks the same per-character interface
  list looking for the first one with a non-empty field-id-`0x25F` link,
  then either records it as the character's active command (field id
  `0x205`) or, if it already matches, pushes it into the game's own
  field-id-`0x262` link instead. Needed a non-const `TGCharacter::GetRef()`
  overload alongside the existing const one, since the original mutates
  that field in place via `TVisObjRef::SetLink()`.

## TGameControl batch 12: ChangeCharacter ties several earlier pieces together

`ChangeCharacter` (asm lines 465849-466072) is a good example of how much
of a payoff the earlier small methods give once enough of them exist: its
body is almost entirely calls to `GetCharacter`, `ResetState`,
`SetInterfaces`, `SetCharacterActiveCommand`, `ScrollToCharacterIfNeeded`,
`AdjustInterfacesOnScreen`, and the same `HandleMouseMove` mouse-replay
tail as `UpdateCurrentObject`/`ChangeCharacter` share - all already built.
Only two small new pieces were needed: `TGCharacter::SetRandomTime()` and
`TSceneControl::ShowScene()` (both call-shape-only stubs), plus a
`m_previousCharacter` field whose exact purpose is unclear (it's set to the
same value as `m_currentCharacter` right when it changes, but never read
again in anything reversed so far - may just be a second cached copy).

## TGameControl batch 13: Save() and LoadGame(int) - the tractable ends of a big family

The savegame methods span a huge size range: `Save()` (189 lines) and
`LoadGame(int)` (83 lines) were tractable, but the real workhorses they
delegate to - `LoadGame(TMSavegame*)` (~970 lines) and `SaveGame(int)`
(also several hundred) - are not, and stay stubbed.

- **`Save()` is `void`, not `bool`**: its one caller (`SaveGame`) discards
  the return value entirely (no `mov`/`test` after the `call`) - another
  instance of the by-value-return tell, just for a scalar this time instead
  of a struct/reference. It composes almost entirely out of existing
  pieces (`m_activeTexts`/`m_sceneTexts`/`m_currentText`'s `Save()`,
  `m_characters`' new virtual `Save()`) plus three new static/free
  entry points modeled the same way as `TGAction`'s: `TGAnimation::
  SaveActions`/`ClearActions` (the latter now actually wired into
  `~TGameControl`, previously just a comment) and the free function
  `SaveGlobalScriptVariables(TVisionaireGame&)`.
- **`TGText::Save()` is a plain (non-virtual) method on `TGText` itself**,
  not inherited from `TSText` - unlike `Discard`/`OnCleared`/
  `CalculateCurrentText`, its call site is a direct `call`, not a
  vtable-indexed one.
- **`LoadGame(TMSavegame*)`** (was declared as `LoadGame(void*)`) confirms
  yet another `void*` manifest placeholder that should have been a real
  type.

## TGameControl batch 14: GetWalkingSounds

A straightforward filter-and-collect (asm lines 474770-474985): for every
character whose scene-link (field id 0x1F7, matching `CenterScene`) points
at the current scene, resolve their walking-sound path (field id 0x110)
and append it if `wxFileName::IsOk()` after normalizing. No new classes
needed - the first entirely self-contained method in a while, safely
smoke-testable without a live `m_currentCharacter`.

## TGameControl batch 15: StartText, and one deliberately left alone

`StartText` (asm lines 461200-461415) is the third and last of the
`StartXText` family, reusing everything the other two already needed
(`THText`, `CreateActiveObject`/`GetEmptyObject`, `GetSpeaker()` dedup):
dedupes against an existing active text from the same speaker, drops any
current text first, creates the new one as `m_currentText`, and keeps it
only if its target reads as "displayed" - otherwise discards it right back.

Looked at `UpdateWalkingSounds` (asm lines 463474-463684) next and decided
to leave it stubbed: underneath a rate-limiting static `TTimer`, it calls
through a raw function pointer read from an unidentified interface
object's vtable (a field this project hasn't seen before, at a `+0x178`
offset) with 7 arguments built from character screen-position, scroll and
viewport math, plus two new `TVisObjRef`/`TGCharacter` accessors
(`GetFloat`, `GetWalkingSound`/`IsWalkingSoundPlaying`). Implementing it
would mean inventing a plausible-looking shape for that callback interface
with no real evidence for its argument meanings - exactly the kind of guess
this project avoids per the lesson below.

## TGameControl batch 16: InitInterfaces - TVList's element type was wrong too

Finally revisiting the `InitInterfaces` deferral from batch 3: re-reading
its full body (Deponia_Linux.asm lines 458124-458250) alongside
`InitGameActions`'s opening (which also calls `TVisObjRef::GetLinks()`)
showed `TVList` actually holds `TVisionaireObject*` elements, not
`TVisObjRef` as first modeled for `TGameControl::InitFonts` - each element
is converted through a new `TVisObjRef(const TVisionaireObject&)`
constructor before use. Architecturally this makes sense in hindsight:
`TVisionaireObject` is the heavier underlying data record, and `TVisObjRef`
is the lightweight handle onto one that the rest of the engine passes
around everywhere. `InitFonts` never actually inspected `TVList`'s elements
(just passed the container through opaquely), so this correction doesn't
change its behavior - but it was still built on a wrong assumption, caught
before it could mislead a later method the way `StartTween`'s did.

With that resolved, `InitInterfaces` itself is simple: fetch the game's
links (field id `0x296`, `eTypeOrder::Value1` - both unresolved) and wrap
each into a new heap-allocated `THInterface`, appended to
`m_allInterfaces`. `InitGameActions` (asm lines 466739-467222+) was looked
at too but left stubbed - it also decodes a literal 12-entry key-action
lookup table that would need its own careful pass.

## Subsystem pass: InitCharacters resolves the deferred character hash table

`InitCharacters` (Deponia_Linux.asm lines 466201-466735) is the method that
actually populates `m_characters` - and, as a bonus, its insertion logic
finally closes out the `GetCharacter`/`GetCharacterPointer`/
`GetCharacterPointerEx` hash lookup left stubbed all the way back in the
first `TGameControl` batch:

- The insertion side confirms the exact same id-packing formula
  (`PackVisId`, already factored out earlier) used as the hash key, with
  last-write-wins semantics on a collision (a new character's index
  overwrites an existing entry with the same hash). This is modeled as a
  plain `std::unordered_map<int, TGCharacter*> m_charactersByHash` rather
  than replicating the original's open-hashing bucket/node/index layout -
  nothing outside this one insert+lookup pair depends on that internal
  shape, so the simpler container is fully behaviorally equivalent.
- `GetCharacter`/`GetCharacterPointer`/`GetCharacterPointerEx` were
  rewritten against the real map instead of being permanently-empty stubs.
- Confirms `THCharacter : TGCharacter` (built from a self + parent
  `TVisObjRef` pair) and `TGCharacter::Init()`/`AssignToScene()`.
- Two real diagnostic strings were recovered byte-for-byte and wired up
  through the same `wxLog::logexpanded` pattern already established in
  `TStandardPaths.cpp`: "There must be at least one character for a valid
  game." and "An active character must be defined for a valid game."
- Fixed two more manifest placeholders now contradicted by real evidence:
  `InitCharacters` returns `bool` (whether a valid starting character was
  found), not `void`.
- Two fields are set but never read anywhere reversed so far -
  `m_previousCharacter` (also touched here, not just `ChangeCharacter`) and
  a new `m_startingCharacter` - both kept for fidelity with an honest
  "purpose unclear" comment rather than dropped or guessed at.

## InitCharacters subsystem, continued: Init() itself was the payoff

With `InitCharacters`/`InitInterfaces`/`SetCharacterActiveCommand` all
reversed, `TGameControl::Init()` (Deponia_Linux.asm lines 467226-467624) -
the master bootstrap the whole engine calls once at startup - turned out to
be almost entirely orchestration over methods already built: set the
starting scene, reset several scroll/centering game-data fields (matching
`CenterScene`/`SetOnScrollDestination`'s field ids exactly), call
`InitCharacters()` and bail if it fails, then `InitInterfaces()` +
per-character `SetInterfaces()` + the instance-level `SetInterfaces()` +
`SetCharacterActiveCommand()`, load fonts (the same `GetList(3, ...)` +
`TFontManager::Initialize()` pair `InitFonts()` already uses), then
`InitGameActions()`/`InitScripts()`/`TConsole::Init()`. Only three small
new pieces were needed: `TSceneControl::Set()`, `TGScene::InitActionAreas()`
(static, matching the `TGAction`/`TGAnimation` pattern), and
`TConsole::Init()`.

Recovered two more diagnostic strings byte-for-byte confirming `TTimer::
GetTime()` is milliseconds: "Interfaces loaded. Needed time: %ld ms" and
"Scripts loaded. Needed time: %ld ms".

## TGameControl batch 17: InitGameActions, and a wx/SDL "unified keysym" table

`InitGameActions` (Deponia_Linux.asm lines 466743-467222) builds
`_gameActions` from the game's action-definition list (field 0x13E), and
turned out to hide a nice confirmation of how the engine's custom key-action
binding scheme works. Its local `arrKeyActionCodes` table
(Deponia_Linux.asm data at address 0xD6D220) decodes byte-for-byte to: `' '`,
`'\r'`, `WXK_ESCAPE`(27), `WXK_BACK`(8), `','`, `'.'`, `'+'`, `'-'`, then the
real SDL2 keycodes `SDLK_F1`-`SDLK_F12` and `SDLK_LEFT/RIGHT/UP/DOWN` (each
is `SDL_SCANCODE_x | (1<<30)`, confirmed against SDL2's own scancode
numbering) - i.e. this engine's "key code" space for action bindings is a
mix of plain ASCII (digits/letters/punctuation), a couple of named wx
constants, and real SDL2 keycodes for non-printable keys, plus
`ConvertControllerButtonToSymKey`'s already-known 1000001-1000015 range for
controller buttons. Added `WXK_ESCAPE`/`WXK_BACK` to `WxStub.h` and
`SDLK_F1`-`SDLK_F12`/`SDLK_LEFT`/`SDLK_RIGHT`/`SDLK_UP`/`SDLK_DOWN` (plus
`SDLK_SCANCODE_MASK`) to `SdlStub.h` to name these - both real, exact API
constants, not invented ones.

Also confirmed: a bound key's code gets classified into `SGameAction::msg`
by checking (a) whether it falls in the controller-button range
(-> `TKeyboardMessageEnum::kControllerButtonHit`), and (b) whether
`code - 10000` names another valid code in the table above (-> `kKeyUp`+1 or
`kControllerButtonRelease`, i.e. a "+10000 = modified variant" encoding).
This is likely where masterControl.h's long-unresolved `TKeyboardMessageEnum`
values 2/3 come from (2 is produced here; 3 never is, in this function at
least). `SGameAction::flag` comes from scanning a second per-action list
(field 0xA2) for a condition of type 0x6B, or type 0x99 with its own field
0xF2 equal to 1 - both cases make the action bypass dialog/text blocking in
`StartGameAction`. The 0xA2/0xB3/0xF2 field ids and the 0x6B/0x99 type
constants are confirmed call/comparison shapes only; what they represent in
Visionaire's data schema is not resolved. Needed two new stub methods to
compile against: `TVisObjRef::GetList(int, TVList&)` (distinct from
`TVisionaire::GetList`, which takes an extra bool) and
`TVisionaireObject::GetInt(int)` (the list elements here are read directly
as raw `TVisionaireObject*`, not converted through a `TVisObjRef` handle
first, unlike everywhere else in this survey).

While tracking down a range-based `for` loop's pointer spacing
(`TGCharacter * character : _characters` - wrong per CLAUDE.md/ScummVM
convention), found that the ScummVM-reformatting pass had missed this
pattern across the whole tree (astyle's `align-pointer`/`align-reference`
don't reach a range-`for` loop variable, just like they already don't reach
function parameters). A supplementary regex pass fixed all 29 instances
(`gameControl.cpp`, `masterControl.cpp`). Re-running astyle afterward turned
up one more related gap it hadn't caught the first time: a pointer/reference
declared inside an `if`/`while` condition (`if (Foo *x = ...)`) is fixed by
astyle when the type is a built-in keyword (`void *x` got fixed) but not when
it's a user-defined type name (`TGCharacter *x`/`TManagedObject *x` didn't) -
fixed the two remaining instances by hand. Documented all of this in
CLAUDE.md so the next reformatting pass doesn't have to rediscover it.

## TGameControl batch 18: StartTween(const TVisObjTween&), and why erase()+push_back() beats transliteration

Revisited `StartTween(const TVisObjTween&)` (asm lines 474993-475136),
previously only characterized in passing (batch 9 above) as "searches a
vector of 176-byte elements for a matching id+target, then
erase-or-replace-or-append." Tracing it in full: it scans `_visObjTweens`
(the new, correctly-typed member replacing the wrong-guess `_pendingTweens`
for this overload - see that member's own comment) for an entry whose
`target` matches the new tween's, and if found, erases it (with the
disassembly manually destroying what look like two `std::function`-style
members via a stored manager-function-pointer call, `_M_manager(dest, src,
3)` - the "destroy" tag in libstdc++'s `std::function` ABI); it then falls
back into the same scan loop again (supporting multiple matches, though in
practice there should only ever be one) before finally appending the new
tween, either by placement-new at `end()` (spare capacity) or by falling
through to the vector's own growth-path emplace helper (at capacity) - i.e.
exactly `std::vector<TVisObjTween>::erase()` then `push_back()`, inlined and
unrolled by -O2. Writing it directly as an erase-loop + push_back reproduces
this without hand-transliterating the placement-new/growth-path branches or
needing to know `Tween`'s real fields (still an empty stub struct) - the
compiler's generated copy/destroy logic for whatever `Tween` turns out to
contain will do the equivalent cleanup on its own. `StartTween(const Tween&,
const std::string&)`'s own 88-byte-element vector (batch 9) is left alone
for a future pass.

## TGameControl batch 19: SaveGame(int) is void, not bool, and a new writer-class stub hierarchy

`SaveGame(int slot)` (asm lines 462981-463264) turned out to be another
manifest return-type error like `Save()` (batch 13): no path through the
function ever sets `eax` before its one `retn`, and every call site
discards the result immediately (`xor eax,eax` right after the `call`,
never reading it) - changed the declared return type from `bool` to `void`.

The real logic: `slot==-1` saves over the scene's currently-selected
savegame (bailing out if there isn't one); any other slot constructs and
owns a new numbered `TMSavegame`, `delete`d again at the end (a plain
`delete savegame` reproduces the disassembly's virtual-dispatch cleanup call
without needing to identify which exact vtable slot it hits). Field id
0x1D5 is the same "last playable scene" field `Save()` itself already sets
(confirmed by that method's own comment) - `SaveGame` overwrites it a second
time with the definitive current-scene reference right before persisting,
via `TGScene::GetRef()`.

Needed a handful of new stub types to compile against, none reversed beyond
their call shapes: a `TXMLWriter`/`TBufferedProjectFileWriter`/
`TProjectFileWriter`/`TXMLStringWriter` chain (`TVisionaire::SaveSaveGame`
takes the first, `TMSavegame::SaveGame` the second, and the one concrete
object constructed here is the last - modeled as a single inheritance chain
since that's the simplest hierarchy satisfying both call shapes),
`TTempFile::DeleteTempFiles()` (a static cleanup entry point, anticipated by
an existing comment in `TCharHolder.h`), `eVisionaireTable`/
`TVisionaire::ResetActiveData` (one confirmed enum value, 0x22), and
`wxFileName::SetFullName` (real wxWidgets semantics: replaces the name+
extension, keeps any existing directory).

Also recovered the exact save-data filename format string byte-for-byte
from the binary's own data (address 0xD6CB90): `L"vtp_saveddata%d.xml"` -
straightforward enough (one `%d`) to translate directly to
`L"vtp_saveddata" + std::to_wstring(nr) + L".xml"` rather than needing a
real `wxString::privFormat`/`vswprintf` implementation.

One loose end left honest rather than papered over: the constructed
`wxFileName` is given a name via `SetFullName` but never read again
anywhere in the function - likely vestigial debug/profiling instrumentation
(two local `TTimer`s are stamped via `SetTime()` in the same way, also never
read) that survived because the compiler couldn't prove those calls have no
side effects. Reproduced faithfully rather than dropped, with a comment
flagging the oddity.

## TGameControl batch 20: HandleEngineEvent dispatches by count, not by name

`HandleEngineEvent(const std::string&, const std::string&)` (asm lines
469160-469504) turned out simpler than its size suggested once the Lua-
dispatch boilerplate (five stack-allocated `TArgument` locals, only two ever
`Set()`, matching the same pattern already noted in `TMasterControl::
ProcessMessage`) was recognized rather than transliterated. The loop walks
`TMasterControl::_engineEventHandlerNames` via raw `begin()`/`end()`
pointers but never dereferences an element - only the vector's *size* is
read, and every iteration dispatches to the same fixed Lua function name,
`"EngineEventHandler"`. So a handler's registered name (see
`RegisterEngineEventHandler`) is apparently just a de-dup key controlling
*whether* a registration is added, not *which* Lua function gets called -
`HandleEngineEvent` just fires the one shared handler once per distinct
registration, passing the same (name, arg) event payload each time. Moved
`_engineEventHandlerNames` from private to protected in `TMasterControl`
(same reasoning already given for `_sceneControl`/`_visionaire`/
`_allInterfaces` there - `TGameControl` reads it directly, not through a
`TMasterControl`-only accessor). Added `TArgument::Set(const wxString&)`,
the one overload that wasn't stubbed yet.

## TGameControl batch 21: ScrollToCharacterIfNeeded reads _previousCharacter, not _currentCharacter

**Correction to this batch's own first pass**: `ScrollToCharacterIfNeeded`
reads a member at offset 0x290 through a couple of virtual calls. Comparing
that offset against the asm lines already cited in `_previousCharacter`'s
own header comment (`ChangeCharacter` asm line 466005, `InitCharacters`'
starting-character resolution asm line 466501 - both inside the exact ranges
`_previousCharacter`'s comment names) showed offset 0x290 **is**
`_previousCharacter`, an existing member - not a new field needing its own
model. The first pass here reasoned from the offset alone, concluded it was
"the same logical value as `_currentCharacter`" (true - they're always set
identically), and modeled it as `_currentCharacter` directly rather than
checking whether an existing member already covered that exact offset. Fixed
throughout, and updated `_previousCharacter`'s own comment: it's no longer
"never read anywhere" - this function reads it via `GetScreenPosition()`/
`GetVisibleRect()`, both methods already confirmed elsewhere (`CenterScene`,
batch 8).

The real logic: bails out unless `character` is the game's currently-active
one (field 0x263, matching `_currentCharacter`'s own 0x1D4/0x263-tracking
already seen in `ChangeCharacter`/`InitCharacters`) and scrolling is enabled
(field 0x231) and that character is actually on the currently-displayed
scene; then checks the character's position/visible-rect against the
scene's worktop size and current scroll position on all four sides,
writing a scroll-direction code into field 0x1D9 (1=left, 2=right) or 0x1DA
(3=up, 4=down) - the same two fields `CenterScene` resets to 0, confirming
they're a horizontal/vertical "pending scroll direction" pair. Preserved one
asymmetry faithfully rather than "cleaning it up": the vertical check
returns immediately on an up-scroll match (skipping the down-scroll check
entirely), but the horizontal left/right checks are NOT mutually exclusive -
both run regardless of the other's outcome. Added `wxRect::GetRight()`/
`GetBottom()`/`SetLeft()`/`SetTop()`/`SetWidth()`/`SetHeight()`, real
wxWidgets API surface that was missing.

## TGameControl batch 22: AdjustInterfacesOnScreen and a 6-mode interface layout algorithm

`AdjustInterfacesOnScreen(bool force, TPaintControl *scene)` (asm lines
464975-465524) is TGameControl's largest method reversed so far by a good
margin, but decomposes cleanly into two independent pieces once traced in
full.

First, a cache check: if `_currentCharacter` has changed since the last call
(tracked by the new `_lastInterfaceCharacter` member), refresh every one of
that character's own interfaces with fresh item lists pulled from field
0x297.

Second - unless a specific `scene` was requested and it isn't one of
`_activeInterfaces` (in which case the whole function is a no-op) - lays out
every active interface not overridden by field 0x1DF ("manual positioning
override," checked once for the whole call, not per interface) according to
a 6-mode position enum read from each interface's own field 0x13A
(`TInterfacePositionEnum`, in `TGInterface.h`), consuming a per-interface
margin (field 0x144) as it goes:
- Modes 0/1 (`kDockTopStacked`/`kDockBottomStacked`) stack interfaces
  vertically from the top or bottom edge, each one consuming `margin` (or
  its own full worktop height, if `margin<=0`) from a shared running
  "remaining height" budget.
- Mode 2 (`kDockTopRow`) does the same horizontally along the top edge.
- Modes 3/4 (`kFixedReserveWidth`/`kFixed`) both just place the interface at
  its own stored point (field 0x12E); mode 3 additionally reserves width
  from the shared budget the same way mode 2 does, mode 4 doesn't touch the
  budget at all.
- Mode 5 (`kDraggableClamped`) is a movable/draggable panel: while being
  dragged (`force` is true and this interface IS the `scene` parameter),
  its position follows the mouse offset from its stored point; otherwise it
  keeps its current origin. Either way the result is clamped to stay fully
  within the window bounds - the only mode that reads `force`/`scene` at
  all, or clamps.

Whatever width/height budget is left over after all active interfaces have
claimed their share becomes the current scene's own origin and visible
size - i.e. this is a "give edge-docked interface panels first claim on
screen space, then let the scene fill whatever's left" layout algorithm,
the same shape as a classic dock-panel UI layout. Added
`TPaintControl::SetWorktopSize/SetVisibleSize/SetOrigin/GetOrigin` (and wired
`GetWorktopWidth/Height` to a real backing field instead of a hardcoded 0,
now that a setter exists), `TGInterface::UpdateItems()` and a second,
mutable `GetRef()` overload (matching `TGCharacter`'s existing dual-overload
pattern - `AdjustInterfacesOnScreen` mutates an interface's own field 0x2B0
in place), and `TInterfacePositionEnum` itself.

## TGameControl batch 23: ReplaceGame, and a new not-yet-integrated THGameControl question

`ReplaceGame(wxFileName file, bool isEditor)` (asm lines 468504-468965)
resolves `file` to an absolute path under the game's own directory,
confirms it exists (logging and bailing if not), then hands off to
`LoadAndInitGame`/`InitAfterLoadingScreen` (neither reversed yet - this
pass only needed their existing call shapes) plus some cache-invalidation
bookkeeping. The path-concatenation step's disassembly is another instance
of batch 18's lesson: a COW-string capacity check deciding between
append-in-place and insert-at-front is a compiler optimization, not
meaningful logic - reproduced as a plain concatenation.

One new, unresolved architectural question surfaced: `LoadAndInitGame`,
`RegisterEventHandler`, and `GetCursorControl` are all called through the
`g_pGameControl` global rather than `this` (unlike the editor-mode branch's
`this->_visionaire`) - reproduced as observed. Worse, IDA resolves the
`RegisterEventHandler` symbol as `THGameControl::RegisterEventHandler()`,
not any method on `TGameControl`/`TMasterControl`. `THGameControl` has only
been seen before as a *caller* (`THGameControl::OnEvent`, xref'd from
`ScrollToCharacterIfNeeded`/`AdjustInterfacesOnScreen`) - its relationship to
`TGameControl`/`TMasterControl` (a base class? a wrapper?) isn't established.
Rather than guess at that relationship, `RegisterEventHandler()` was added
directly to `TMasterControl` (so `g_pGameControl->RegisterEventHandler()`
compiles) with a comment flagging the gap - a dedicated pass on
`THGameControl` itself would be worth doing before this comes up again.

Also unresolved: the member read via `wxFileName::GetPath()` at asm line
468573 (some stored "game directory") is modeled as `_gamePath` for lack of
a better candidate, though its real identity at that exact offset isn't
independently confirmed - `_gamePath` was previously only ever used as a
`wxString`, never as something `wxFileName`-flavored.

New stub surface: `TVisObjRef::GetName()` (returns `TCharHolder`, unlike
`GetStr`/`GetInt`/etc. it takes no field id), `TCursorControl::Clear()`,
`TId`/`UnrefLuaFieldsCache()` (new, in `vscommon/scripting/id.h`), and real
`wxFileName::GetPath()`/`Exists()` (the latter delegating to the already-real
`wxFile::Exists()`).

## TGameControl batch 24: LoadAndInitGame - the manifest's parameter names were wrong, and a second TVisionaire/TVisionaireGame gap

`LoadAndInitGame(wxString&, const wxString&, wxString, bool)` (asm lines
467635-468496) is the largest method reversed so far. Its manifest-derived
parameter names turned out actively misleading, not just approximate: the
first parameter (named `error`) is read-only throughout the traced path -
wrapped in a `wxFileName` and handed to `TVisionaire::LoadDataGame` as the
file to load, never written as an error message - renamed to `filePath`.
The third (`warning`) does double duty: if passed empty, it's populated
from the game's own field-0x132 name as a fallback, and later in the same
function it's compared against a list of language objects' own names to
pick which one to activate via `TTText::SetLanguage` (falling back to the
list's first entry on no match) - genuinely overloaded, not split into two
parameters, since that's what the disassembly does.

A second instance of batch 23's `THGameControl` gap turned up:
`TVisionaireGame::LoadDataGame` is called with `this` confirmed as
`_visionaire` (`TVisionaire*`), not `_visionaireGame` (`TVisionaireGame*`) -
added to `TVisionaire` instead, flagged the same way. A third,
smaller one: `this` gets passed as a `TSignalSlot*` parameter when
`isEditor` is true, implying `TGameControl` derives from (or converts to)
`TSignalSlot` in the original - modeled as an empty placeholder class and a
`reinterpret_cast`, not a real inheritance relationship.

Otherwise a fairly mechanical translation once each piece was identified:
graphics filter/cache setup (`TGraphicsInterface`, new `SetFilters`/
`PreallocateTextures`/`SetCacheSize`), the same aspect-ratio field-0x7E/
`g_unlockAspect` logic `UpdateAspectRatio` already has (duplicated inline
here, not shared), a `TDiagnostic::BeginFixedRegion`/`EndFixedRegion` pair
bracketing the load (asymmetric - `EndFixedRegion` is only called on
success, never on failure, reproduced as observed), then three
`TVisionaire::GetList` calls (0x12 = languages, 0xF = cursor definitions to
load, 2 = buttons whose field-0xE5 link needs `TCursorControl::
LinkButtonCursor` using the same id-packing `PackVisId()` already
implements), a `TGameControl::Init()` call (already implemented), and
finally pointing `TMasterControl::_sceneControl` at `_ownedSceneControl` -
redundant with what the constructor and `ResetState()` already do, kept for
fidelity anyway.

Moved `_loadingControl`/`_soundManager` from private to protected in
`TMasterControl` (same reasoning as the other TGameControl-reads-directly
fields there). New stub surface: `TSoundInterface` (the interface
`TSoundFFMPEG` implements, confirmed distinct from it),
`TLoadingControl::EndLoading()`, `TDiagnostic`, `TLoadingTypeEnum`,
`TVisionaireObject::GetName()/GetLink()/GetId()`, `TCharHolder::
operator==(const wxString&)`, `TTText::SetLanguage()` (static), and
`TCursorControl::LoadCursor()/LinkButtonCursor()`. Also added
`TVList::size()`/`front()`, matching call shapes already needed here.

## Reformatted to ScummVM's code conventions

Since this engine's eventual destination is a ScummVM engine module, the
entire tree was reformatted to match ScummVM's own
[Code Formatting Conventions](https://wiki.scummvm.org/index.php?title=Code_Formatting_Conventions):
see `CLAUDE.md` at the repo root for the full rule reference and the
project's one deliberate exception (recovered class/method identifiers are
NOT renamed to ScummVM's camelCase - they're evidence, not style; see that
file for the full reasoning).

Mechanically: an `astyle` pass (`.astylerc` at the repo root) handled
indentation/braces/most spacing, followed by two supplementary scripted
passes: one fixing pointer/reference spacing in function parameters (an
astyle limitation - it only fixes declarations/return types) that had to be
written comment-aware after an early version corrupted prose mentioning
pointer types (e.g. "the manifest's `void*` was..." or markdown-style
`*emphasis*` in a comment got mangled into "`void *was`"/"`*this *the`" the
first time through - caught by diffing against the pre-format commit and
fixed by making the regex skip `//` and `/* */` text entirely), and one
renaming every invented member variable from `m_foo` to ScummVM's `_foo`.
Enum values with no recovered evidence for their real names (e.g.
`TextAlignmentEnum::Left` -> `::kLeft`) were also renamed to the `kCamelCase`
convention, using their scope-qualified form to avoid collisions with
ordinary English words in comments (`Left`/`Right`/`Center` are common
prose, but `TextAlignmentEnum::Left` is unambiguous).

Left alone deliberately: `TControllerEffectType`/`TControllerEffectDirection`
in `vsplayer/control/gameController.h` still use their own `TCET_`/`TCED_`
prefix convention rather than `kCamelCase` - they're plain (unscoped) enums
with values used throughout `gameController.cpp`, so renaming needs more
call-site verification than this pass covered. Worth revisiting.

## Lesson: don't fill an unconfirmed gap with a plausible-looking guess

While TMasterControl was being written, no evidence was found for where
its `m_sceneControl` pointer gets set (its own constructor, as actually
traced, never touches it) - so it got a `new TSceneControl()` in
TMasterControl's constructor "to make GetMainControl() work," clearly
marked as a guess at the time. Once TGameControl was reversed, its
constructor turned out to embed a *real* `TSceneControl` by value and
point `m_sceneControl` at that instead - so `~TMasterControl()`'s `delete
m_sceneControl` ended up calling `delete` on a non-heap address, silently
corrupting the heap (caught immediately by testing: MinGW's CRT reported
`STATUS_HEAP_CORRUPTION`, no output at all since the crash happened before
stdout - fully buffered when not attached to a console - ever flushed).

Fixed by removing the guess entirely: `m_sceneControl` is left null in
TMasterControl's own constructor, and only the owning subclass (currently
just TGameControl) is responsible for pointing it at whatever TSceneControl
it actually owns. The general lesson: a "fill the gap with something
plausible so it compiles" placeholder is fine short-term, but needs
revisiting - and re-testing - the moment a later class's real evidence
contradicts it, rather than assuming the guess was harmless because it
compiled and ran once.

Source: `Deponia_Linux.asm`, `main` proc, asm lines 499508-500213
(address range 0x62E450-0x62EC85 roughly; see `endp` before local labels
resume at 0x62ED2E).

## Capstone finding: this binary uses the old COW std::string ABI

Confirmed while reversing `TComposedFile`'s entry-growth loop (which copies
an `SEntryInfo`'s `std::wstring` field via a **single 8-byte pointer copy**,
then five more separate qwords): this binary was compiled with
`-D_GLIBCXX_USE_CXX11_ABI=0`, i.e. libstdc++'s pre-C++11 **copy-on-write**
string representation, where a `std::string`/`std::wstring` object is
*literally just one pointer* to a shared, ref-counted buffer (a hidden
`_Rep` header - refcount, length, capacity - living immediately before the
visible character data). Modern libstdc++ (the SSO/"short string
optimization" ABI most of us are used to) makes a `std::wstring` a 32-byte
object with an inline small-buffer; this binary's strings are 8 bytes.

This single fact retroactively explains several things noted independently,
class by class, earlier in this project:

- Why `wxString`/`wxFileName`-typed values kept turning out to be
  reinterpretable as a bare `std::wstring` pointer with no visible
  wrapper overhead (main.cpp's `main+0x37D`, `TComposedFile`'s entries,
  `TMasterControl`'s several string members) - they're COW strings, so a
  "wxString" that wraps one has the exact same 8-byte layout as the
  wstring itself.
- Why so many *different* classes' destructors collapsed under the same
  ICF-folded symbol (`std::pair<std::string const,ulong>::~pair`,
  `std::list<ctdrpc::earlyrole_ip_t>::_M_clear`, etc., see below): a COW
  string's destructor is just "decrement the shared refcount, free the
  `_Rep` if it hit zero" - completely generic machine code that doesn't
  depend on the character type or the wrapping class at all, so the linker
  folds huge numbers of unrelated destructors together.

Not relevant to *our* rebuild: we use ordinary modern `std::wstring` inside
`wxString` (see `WxStub.h`) throughout, since this project targets
behavioral fidelity on a fresh compiler/ABI, not binary compatibility with
the original executable. Recorded here purely because it resolves a
question ("why does this keep looking like just a pointer?") raised
independently in at least three earlier classes' notes.

## Feasibility verdict

`main` itself is tractable: it's a straightforward sequence of calls with a
handful of early-return branches and the usual GCC `-O2` exception-handling
landing pads (each cleanup block is clearly commented by IDA as
`; cleanup() // owned by <addr>`, which made matching them back to their
try-region trivial). No STL containers are iterated, no templates are
instantiated inline other than `std::wstring`/`std::function`, and the two
`std::function` thunks (`_M_invoke`, `_M_manager`) were easy to read directly
because they're tiny (the lambda body is a single `jmp` to
`TPictureIO::RetryFailedPicturesLoad`).

## Recovering the original directory layout from x_assert()

The engine's assert macro, `x_assert(bool cond, const char* expr, const
char* file, int line)`, embeds `__FILE__` as a full build-machine path at
every call site, e.g. `/home/simon/Documents/jenkins/branchPillars/src/
vsplayer/control/gameController.cpp`. `tools/extract_source_layout.py` scans
every one of the 378 `x_assert` call sites in the binary, resolves the
file/expression string operands, and cross-references the enclosing
function - giving a confirmed mapping from many classes to their real
source file, without guessing. Results: `manifest/source_layout.tsv` (raw,
one row per assert) and `manifest/source_layout_summary.tsv` (grouped by
recovered file, with sample functions).

This is now the basis for this project's directory layout: `src/deponia1/`
mirrors the original `src/` root (confirmed via asserts:
`baselib/composedfile.cpp` for `TComposedFile`, `graphicslib/picture.cpp`
for `TPictureIO`, `vsplayer/control/gameController.cpp` for
`TGameController`, `vsplayer/main/mainSDL.cpp` for `main`/`Init`). A class
only gets moved into that structure once an assert (or other hard evidence)
actually confirms its file; `TStandardPaths`, `TMasterControl`, and
`TComposedFileManager` have no assert hits yet, so they stay at the top
level of `src/deponia1/` rather than being placed on a guess. Each moved
header/source has a comment recording which assert confirmed its location.

Re-run the extractor (cheap, a few seconds) whenever a new class's methods
get reversed, in case they resolve to a file not yet seen - some files in
`source_layout_summary.tsv` only have one or two sampled functions and are
likely to grow other members as more classes get worked through.

## Key finding: ICF (identical code folding) is corrupting symbol names

Several call sites in `main` resolve to
`std::pair<std::string const,unsigned long>::~pair()` (with IDA-appended
`_0`/`_1` suffixes to disambiguate), even at addresses that are structurally
required to be destroying a `wxFileName` or a `wxString`. That demangled name
almost certainly does not describe the real type; it is the name the linker
happened to keep for one member of a set of functions whose *machine code*
is byte-identical and got folded into a single symbol (a `pair<string,
unsigned long>` destructor that just tears down two trivial members compiles
to the same instructions as many other small classes' destructors). This is
consistent with what you ran into before with the String class: any type
whose special members are simple enough to collide with another type's under
ICF will get an arbitrary, misleading label, and Hexrays' type propagation
compounds the error once it borrows that symbol's declared type. Cross
referencing *how* a value is used (what it's copy-constructed into, what
it's passed to) proved more reliable than trusting the demangled destructor
name.

Concretely, this is what let me determine `g_logfile` is a `wxFileName`, not
a `wxString`: the call believed to be `wxString::operator=` at main+0x213
assigns into `g_logfile`, but `g_logfile` is later passed as the implicit
`this` to `wxFileName::GetFullPath()` (main+0x22C) - only consistent if
`g_logfile` is a `wxFileName` and the "wxString::operator=" symbol is really
`wxFileName::operator=` folded together with it (a wxFileName whose copy
assignment just copies its one wxString field would compile identically to
wxString's own copy assignment).

Separately, at main+0x2AE (`std::wstring::basic_string(std::wstring
const&)` constructing a local straight from a `wxString*`), the call is only
valid if `wxString`'s layout starts with (or *is*) a `std::wstring` - so
`WxStub.h`'s `wxString` wraps a `std::wstring` as its data, which is
consistent with both observations.

## Things intentionally simplified for the stub build

- **Control flow**: the original has ~15 duplicated
  `wxCmdLineParser::~wxCmdLineParser()` calls, one per exception/early-return
  path, because GCC inlined the destructor at every exit instead of sharing
  an epilogue. Ordinary C++ scoping (`cmdLineParser` going out of scope) is
  semantically equivalent and far more readable, so `main.cpp` uses plain
  `if (...) return -1;` instead of chasing every `jmp`/landing pad 1:1.
- **`fopen64` -> `fopen`**: `fopen64` is glibc's LFS entry point for the
  32-bit-`off_t` ABI; on a from-scratch MinGW build there's no reason to
  keep it, `fopen` is the right call.
- **SDL/wxWidgets are stand-ins** (`SdlStub.h`, `WxStub.h`), not the real
  libraries - neither is installed in this environment yet. Function names
  and signatures were kept identical to the real APIs so swapping in real
  SDL2 headers and a real wxString/wxFileName implementation later should be
  close to a drop-in replacement for the call sites in `main.cpp`.
- **Stub bodies** (`AppFunctions.cpp`, `T*.cpp`) do just enough to let the
  smoke test run to completion: `Init` and `ParseCommandLine` report success,
  `ShowFrame` clears `isProgramLooping` after one call so the "game loop"
  actually exits, `g_pGameControl` is heap-allocated up front since the real
  binary must set it up somewhere before `main` runs and that code hasn't
  been located yet.

## Recovered string literals (verified byte-for-byte against the .asm data)

| Symbol | Value |
|---|---|
| `unk_D6EAD8` | `L"/messages.log"` |
| `dword_D6EB70` | `L"Init failed, could not load game"` |
| `dword_D6EB10` | `L"Unable to open SDL: %s"` |
| `unk_D6EBF8` | `L"gamecontrollerdb.txt"` |
| `dword_D6EC50` | `L"Added Controller mappings from File %s"` |
| `asc_D6F40C` | `L"l"` (the `-l` / `--l` command-line option name) |
| `aInitFailed` | `"Init failed"` |
| `aInitFailedCoul` | `"Init failed, could not load game.\nThere is no game file or the game file is corrupted.\nFor more info view messages.log."` |
| `aSdl` | `"SDL "` |
| `aCouldnTInitSdl` | `"Couldn't init SDL. App will quit now."` |

`ShowMessageBox`'s parameter order was confirmed from both call sites:
`rdi` (first arg) is always the short string ("Init failed" / "SDL "), `rsi`
(second arg) the long descriptive one, so the signature is
`ShowMessageBox(const wxString& title, const wxString& message)`.

The `fopen64` mode string is `(offset aW+1)`, i.e. it points one byte into
the existing `"-w"` literal to reuse its trailing `"w"` - a linker
string-suffix-merging trick, not a real "-w" flag; the actual open mode is
just `"w"`.

`SDL_Init` is called with flags `0x21` = `SDL_INIT_VIDEO (0x20) |
SDL_INIT_TIMER (0x01)`. `SDL_EventState` is called with type `0x303`, which
is `SDL_TEXTINPUT` in real SDL2.

## Not yet resolved

- Exact bit-width/type of `AppStatus`, `isProgramLooping`, `eMouseMessage`,
  `byte_11F8B01`/`byte_11F8B02` (all just `mov [addr], imm8` stores in this
  function - their real type would need to be inferred from other
  reader/writer sites).
- Where `g_pGameControl` and `standardPaths` are really initialized (not in
  `main`).
- `wxString`/`wxFileName`'s true field layout - deferred until we're
  implementing them for real rather than stubbing them.
