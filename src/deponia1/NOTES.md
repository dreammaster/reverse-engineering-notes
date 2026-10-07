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

## TGameControl batch 25: HandleKeyEvent, and a cross-confirmation of batch 17's guess

`HandleKeyEvent(TKeyboardMessageEnum, const wxString&, int, unsigned short)`
(asm lines 470963-471714) turned out simpler than its size suggested once
the Lua-dispatch boilerplate (per-handler `TArgument`/`LuaExecuteFunction`
construction, same unreversed-contract gap already noted in
`ProcessMessage`/`HandleEngineEvent`) was set aside as boilerplate rather
than transliterated. The real logic: bail if the scene is mid-transition or
the console consumes the event; then, after the (currently inert) handler
dispatch loop, check field 0x279 for an override action link and dispatch
straight to `StartGameAction` if present (subject to a 0x1E0 block-override
flag); otherwise ESC (`a==27`) with `msg==2` skips the current cutscene; and
finally, if field 0x235 (a "game actions enabled" toggle) is set, scans
`_gameActions` with almost exactly `StartGameAction`'s own matching logic
inlined directly rather than calling it.

That `msg==2` comparison is a genuine cross-confirmation of batch 17's
speculative finding: `InitGameActions`' "+10000 = modified variant"
encoding scheme was guessed to be where `TKeyboardMessageEnum`'s otherwise-
unobserved value 2 might come from, and here it is, actually compared
against in real control flow (`TKeyboardMessageEnum` value 3 also appears,
gating whether the Lua dispatch's name argument comes from
`SDL_GetKeyName(a)` instead of the passed-in key name - still not otherwise
observed at any call site).

New stub surface: `TSceneControl::FadingToNewScene()`,
`TConsole::HandleKeyEvent()`, `SDL_GetKeyName()`, and
`TGAction::SkipCutscene()`. Moved `_keyboardEventHandlers` from private to
protected in `TMasterControl` (same reasoning as
`_engineEventHandlerNames`/`_loadingControl` there).

## TGameControl batch 26: MoveScene, approximated like ScrollUpdate, and a latent xspeed bug fixed along the way

`MoveScene()` (asm lines 459288-460538+, TGameControl's largest method by a
wide margin) is a scroll-to-target easing function structurally similar to
`TMasterControl::ScrollUpdate` (already approximated rather than
byte-for-byte transcribed, per that method's own comment) - and, it turns
out, not just similar but sharing the exact same global state:
`ScrollUpdate` eases a global named `xspeed` toward the mouse cursor,
`MoveScene` eases that *same* global toward a stored destination point
(field 0x1D7). The confirmed formulas match exactly: exponential ease
`speed += (target - speed) * startspeed` (algebraically the same as
`ScrollUpdate`'s `target + (speed - target) * kEaseFactor`, just
rearranged) with `startspeed` a real named global (confirmed 0.1, matching
`kEaseFactor`'s already-approximated value), and a max-approach-speed
clamped to `min(1, |distance| / dt * 0.025)` stored in another real global,
`speedDownX`/`speedDownY`.

**Fixed along the way**: `xspeed` had previously been modeled as
`TMasterControl::_xspeed`, a private instance member, reasoned at the time
as "nothing needs it shared." That was wrong even before this batch - it's
a real shared global in the original, and `MoveScene` easing it too (a
method on a *different* class, `TGameControl`) is direct proof two
unrelated call sites read and write the exact same value. Removed
`_xspeed`, added `xspeed`/`yspeed`/`speedDownX`/`speedDownY`/`startspeed` as
real globals in `AppGlobals.h` (matching `movex`/`movey`/`stopped_char`'s
existing pattern of confirmed named-not-anonymous symbols), and updated
`ScrollUpdate` to use the global. Also moved `_easeDirectionFlag` from
private to protected in `TMasterControl`: `MoveScene` reads the exact same
field (0x254) `ScrollUpdate` already uses, as the same "eased value vs.
fixed snap-to value" gate.

As with `ScrollUpdate`, the exact decision tree for which side to approach
from per axis - keyed on fields 0x1D9/0x1DA (0=auto, 1/2=forced left/right,
3/4=forced up/down, matching `ScrollToCharacterIfNeeded`'s own codes,
batch 21) plus a two-tier "is there room to scroll, and if so which side"
structure - is simplified into one merged condition per axis rather than
transcribed branch-by-branch; it's gameplay-feel-specific and can't be
verified without running the original. Field ids 0x1D8 ("unconditional
movement" override) and 0x257 (a character facing-angle check gating one
sub-case) are confirmed but not pursued further. New stub surface:
`TGCharacter::IsWalking()` and `TPaintControl::SetIsScrollable()` (now
wired to a real backing field, same as `GetWorktopWidth/Height` once their
setter existed).

## TGameControl batch 27: PreLoad, a real _gamePath type fix, and a missed call in ReplaceGame caught along the way

`PreLoad(wxString &filePath, wxString &warning, bool isEditor)` (asm lines
463692-464965) resolves a game file to an absolute path, sniffs whether
it's a password-protected container (extension "vis"/"exe"/"ved", or a
"VIS3" magic header read from the first 4 bytes) or a plain data file, then
loads it via `TComposedFileManager` or `TVisionaire::Load` accordingly. Same
manifest-naming correction as `LoadAndInitGame` (batch 24): the first
parameter is read-write throughout, not an error message.

**Fixed a real, independently-confirmable type error along the way**:
`_gamePath` (added in batch 24 as a best-effort `wxString` model for an
offset three different methods touch) is actually a `wxFileName`. The proof
is unambiguous here: `PreLoad` assigns it via `wxFileName::Assign()`, which
a `wxString` cannot support, and `GetGamePath()`'s own asm (a bare
`lea rax,[rdi+0xA80]; retn`) confirms it sits immediately before
`_isClearingAnimations` at +0xA88 with nothing in between - consistent with
an 8-byte handle, the same COW-string-based representation already
established for `wxString`. Changed `_gamePath`'s type and
`GetGamePath()`'s return type, and simplified the two call sites
(`LoadAndInitGame`, `ReplaceGame`) that had been wrapping it in a fresh
`wxFileName` on every use to work around the wrong type.

**Also caught while implementing this**: `ReplaceGame` (batch 23) never
actually called `PreLoad` - the real disassembly calls it right before
`LoadAndInitGame`, threading its by-ref outputs (the resolved file path and
a password string) straight through, but `PreLoad` was still a stub
returning `false` unconditionally at the time `ReplaceGame` was written, so
the call was skipped entirely rather than left in and immediately failing.
Added it back now that `PreLoad` does something real.

New stub surface: `TComposedFileManager::InitMainContainer/
GetMainContainer/Init` (two overloads - one taking a `vector<TCharHolder>`
of extra strings, a second, rarer one taking up to 5 `(path, int)` pairs
whose own field ids are confirmed but not independently verified beyond
that), `TVisObjRef::GetStrings()`, `TVisionaire::Load()` (distinct from
batch 24's `LoadDataGame` - an extra `eSaveGame` parameter and an `int*`
out-param), `TFile::ReadByte()/Close()` (implemented for real against the
already-real `std::FILE*` handle), `FillLoadingScreen()`, real
`wxFileName::GetExt()/SetExt()/IsAbsolute()/Assign()/FileExists()/SetCwd()`
and a `(path, name)`-joining constructor, real `wxString::CmpNoCase()`, and
a new real global, `passw` (a password, matching `xspeed`/`movex`'s own
"confirmed real, not anonymous" pattern - never seen written anywhere
reversed so far). The custom (non-printf) format string used to build a
container's numbered temp-extraction filename (data at address 0xD6CC20)
wasn't fully decoded - approximated with a functionally-equivalent
extension+index concatenation instead.

## TGameControl batch 28: LoadEventHandlers, the last of the giants

`LoadEventHandlers()` (asm lines 475143-476661+) is TGameControl's largest
method by asm line count, but the bulk of that size is manual
`wxStringTokenizer`/`std::vector`/COW-string plumbing around a genuinely
simple idea: parse a `;`-separated `"type:names"` specification string
(field 0x2F7) and register each comma-separated name via whichever
`Register*EventHandler` method matches the entry's type prefix - all four
prefix strings recovered byte-for-byte from the binary's own data:
`"mainLoop"` -> `RegisterEventHandlerMainLoop`, `"mouseEvent"` ->
`RegisterMouseEventHandler`, `"keyEvent"` -> `RegisterKeyboardEventHandler`,
`"engineEvent"` -> `RegisterEngineEventHandler`. Written directly against
those already-implemented, already-deduping `Register*` methods rather than
transcribing the disassembly's own manual vector-append/dedup-scan logic
for each of the four destination containers - the same "trust the
higher-level operation, not the compiler's inlined mechanics" reasoning
used throughout this project for STL-heavy code (`StartTween`, the
character hash table, etc.), just applied one layer up since the higher-
level operation here is this project's own code rather than the STL's.

Two things approximated rather than traced to the end, both flagged in the
method's own comment: the mouse case's real per-name filter-list syntax
(a `TMouseEventHandler` needs one, and building it involves an extra
`std::vector<int>` construction not present for the other three types) -
registered here with an empty filter, a strict superset of whatever the
real filter would have restricted it to; and four further fixed strings
compared against individual names (`"animationStarted"`/
`"animationStopped"`/`"textStarted"`/`"textEnded"`, data at addresses
0xD686F0-0xD687B0) for a special case not traced - every name is
registered identically instead. Added a genuinely real (not stubbed)
`wxStringTokenizer`, matching real `wxWidgets`' default `wxTOKEN_STRTOK`
behavior (skips empty tokens between consecutive delimiters).

This closes out the batch of very large `TGameControl` methods
(`AdjustInterfacesOnScreen`, `LoadAndInitGame`, `MoveScene`, `PreLoad`,
`LoadEventHandlers`) tackled in this session - what's left of the class's
stub surface is smaller, more self-contained methods.

## TGameControl batch 29: InitAfterLoadingScreen

A small one: `InitAfterLoadingScreen()` (asm lines 457468-457503) resumes
the sound manager (a confirmed virtual call, vtable slot 8 - added as
`TSoundFFMPEG::Resume()`, a plausible-from-context guess rather than a
recovered name, clearly flagged as such), then shows the current scene
again if it doesn't already have one set. Attempted `SaveEventHandlers`
next (the natural inverse of batch 28's `LoadEventHandlers`, serializing
the same four handler categories back into the `"type:names;..."` string
format) but stopped partway through: the field-offset-to-container mapping
has more ambiguity than the reversed pass so far resolves with confidence
(narrow vs. wide string element types at a couple of the offsets don't
obviously reconcile with the already-confirmed member types), and this
project's own standard is an honest gap over a confidently wrong
transcription - left as a stub for a future pass with fresher eyes.

## TGameControl batch 30: UpdateWalkingSounds, closing the gap its own comment flagged

`UpdateWalkingSounds()` (asm lines 463474-463680) was explicitly called out
in an earlier pass as "deliberately deferred due to an unidentified callback
interface" - the unresolved interface was a virtual call on `_soundManager`
that this pass identifies well enough to implement: added as
`TSoundFFMPEG::PlaySound(const wxFileName&, int volume, int pan, int, int)`
(vtable slot 14, a plausible name from the call shape, not a recovered
identifier). Rate-limited via a function-local static timer (500ms, same
pattern as `MoveScene`'s own `scrollTimer`), it walks the current scene's
characters and, for each one with a walking sound already playing, computes
a stereo pan from the character's on-screen x position relative to the
viewport - confirmed exactly via its derivation (clamped screen-relative
distance divided by `visibleWidth/200`, then shifted to a `[-100,100]`
range - not approximated, the arithmetic reduces cleanly to that) - and a
volume clamped to `[0,100]` from field 0x2ED, then calls `PlaySound`. A
sibling method, `TGCharacter::CheckWalkingSound()`, shares two of the same
float constants (per their own data xrefs) but is never itself called from
here and presumably maintains whatever `IsWalkingSoundPlaying()` reads -
not reversed.

## TGameControl batch 31: InitScripts, and an "IdStrStd(TId const&) vs. GetId()'s uint8_t*" gap

`InitScripts()` (asm lines 458341-458620+) runs `controller.lua` from disk
if present, then executes every type-1 script object linked from the game
(field 0x28B), skipping any whose name contains `"sha1"` (presumably
signature/checksum files riding along in the same link list, not
executable scripts) - each script's source (field 0x28C) has its `"<"`
placeholder characters replaced with real newlines before being handed to
`LuaDoString()`, using an id-derived string as the Lua chunk name for error
reporting.

That last step surfaced a small type mismatch worth flagging rather than
silently resolving: the real `IdStrStd()` takes a `TId const&`, but it's
called directly on `TVisionaireObject::GetId()`'s result, which is a
confirmed `const std::uint8_t*` (used elsewhere, e.g. `PackVisId()`'s
callers). Since `TId` is still an empty placeholder class (added in batch
23 purely so a different call site would compile) with no known
relationship to that packed-byte representation, `IdStrStd()` is declared
here against the confirmed pointer type instead of guessing at how `TId`
and `GetId()` actually relate.

New stub/real surface: a real (not stubbed) `wxFile` (`Length()`/`Read()`/
`Close()` against a genuine `std::FILE*`, same pattern as `TFile`), real
`wxString::Contains()`/`Replace()`, `TCharHolder::c_str()`,
`TVisionaireObject::GetStr()`, and `LuaDoString()`/`IdStrStd()` (new, in
`vscommon/scripting/lua.h` alongside `argument.h`/`id.h`).

## TGameControl batch 32: implement HandleMouseMove

Reversed `TGameControl::HandleMouseMove(const wxPoint &pos, bool isHolding)`
(Deponia_Linux.asm lines 471985-472461, ~480 lines) - the per-frame mouse
move handler. Confirmed control flow:

- If a scene-mouse-position hook is registered (`_sceneMousePositionHookName`
  non-empty) and the call isn't a holding/drag move, the engine dispatches a
  `"SceneMousePositionHook"` Lua call that may override the position used for
  one of the two position fields written later (see below) - left as an
  unimplemented comment, matching the standing `LuaExecuteFunction`/
  `TArgument` dispatch gap already noted in `ProcessMessage`/
  `HandleEngineEvent`/`HandleKeyEvent`.
- `TCursorControl::SetCursorPosition(x, y)` (new) is always called with the
  **raw** `pos` argument, confirmed never the hook-overridden value (the asm
  reads straight from the original argument register at this point, not the
  local the hook could have rewritten). Returns early if the cursor isn't
  active.
- If a dialog is active (`!_dialog.IsEmpty()`), dispatches to a new
  `TGDialog::HandleMouseMove(pos)` stub and returns.
- Otherwise: snapshots the previous call's hovered-interface-object set
  (`_hoveredInterfaceObjects`, new `TVList` member) into a local, clears the
  member, and writes two position fields - confirmed distinct from each
  other: `_lastMousePos` (already existed; now confirmed written here from
  the raw position) and `_lastHookMousePos` (new; written from the
  possibly-hook-overridden position, write-only in what's reversed so far).
- Rebuild gating (asm lines 472198-472447): while holding, or if the game's
  field 0x1DF is set, the hover set is simply left empty rather than
  rebuilt. If field 0x274 is set instead, gating defers to the current
  scene's own field 0x124. All three flags' real meaning is unresolved.
- When rebuilding: iterates `_activeInterfaces` (already a `TMasterControl`
  member), and for each interface whose new `IsInside(pos)` (stub) is true,
  pushes its `GetRef()` onto the hover set and - for the *first* match only
  - dispatches `TGObjectManager::MouseMove()` (new stub) with that
    interface's `GetObject(pos)` (new stub) result.
- If the (possibly just-rebuilt) hover set ends up empty and the new
  `EngineUpdatePaused` global isn't set, falls back to
  `TGObjectManager::MouseMove()` with the current scene's own `GetObject(pos)`
  (new stub overload).
- Finally, compares the previous hover set against the new one: anything
  present before but absent now fires a "mouse left" action via
  `TGAction::AddRunningAction()` on its field-0x184 link (confirmed called
  through `TVisionaireObject::GetLink()` directly on the raw list element,
  not through a `TVisObjRef` wrapper).

One real behavioral gap worth flagging plainly: `TVList::push_back(const
TVisObjRef&)` (new) is a no-op, because `TVisObjRef` in this reconstruction
doesn't carry a real backing `TVisionaireObject*` (it's a field-value stub,
per `visobjref.h`'s own header comment) - there is nothing genuine to append
to `TVList::items` (a `vector<TVisionaireObject*>`). Practical effect: the
rebuilt hover set is always empty, so the "still hovered" comparison never
finds a match and the scene-level `MouseMove()` fallback always fires when
not paused. This is a data-layer gap, not a control-flow one - the shape
above is faithful to the asm; only the field-schema/object-identity layer
underneath it isn't reversed yet. `TVList::copy(const TVList&)` (new,
alongside `push_back`) is a real, meaningful implementation (a plain vector
copy) since it doesn't depend on that gap.

New stub/real surface: `TGDialog::HandleMouseMove()`, `TGScene::GetObject(
const wxPoint&)`, `TGInterface::GetObject(const wxPoint&)`/`IsInside()`,
`TGObjectManager::MouseMove()`, `TCursorControl::SetCursorPosition()` (all
stubs), `TVList::copy()` (real) and `push_back()` (no-op, see above), and
the `EngineUpdatePaused` global.

## TGameControl batch 33: implement HandleMouseUp

Reversed `TGameControl::HandleMouseUp(const wxPoint &pos, TMouseMessageEnum
msg)` (Deponia_Linux.asm lines 472489-473301, ~810 lines) - the mouse
button-release/wheel handler.

A significant side finding: `TMouseMessageEnum` (declared in
`masterControl.h`) previously carried an unconfirmed `kWheel = 5` guess with
no supporting evidence. This function's jump table (`jpt_61B832`, 14 entries)
proves the enum has at least 14 values (0-13), and that the two wheel
messages are actually **12 and 13**, not 5 - the dialog-active branch (below)
dispatches both straight to a new `TGDialog::HandleMouseWheel()`. 5 is a
real, distinctly-handled value with no resolved name. The enum was corrected
accordingly (`kWheel` removed; 5-11 are `kValueN` placeholders, 12/13 keep
their `kValueN` names too since which is "up" vs "down" isn't resolved,
just that both are wheel-related).

Confirmed control flow:

- If the cursor isn't active: `kLeftUp`(2)/`kRightUp`(4) call
  `SkipCurrentText()`; anything else is a no-op.
- If a dialog is active (`!_dialog.IsEmpty()`): `kLeftUp`/`kRightUp` call a
  new `TGDialog::HandleMouseClick()` stub; the two wheel values (12/13) call
  the new `TGDialog::HandleMouseWheel(msg)` stub; anything else is a no-op.
  Both return without reaching any of the logic below.
- Otherwise (dialog empty): two game-data int fields (0x283, and 0x1FC read
  through `_previousCharacter->GetRef()` - the same "reads _previousCharacter,
  not _currentCharacter" field-access pattern already confirmed for
  `ScrollToCharacterIfNeeded`, batch 21) gate an early return for two specific
  flag combinations; real meaning of both fields/values unresolved.
- A switch on `msg` (the jump table): `kLeftUp`(0x17B)/`kRightUp`(0x233) fire
  a linked action if present and set a `handled` flag; `kValue5`(0x17D)/
  `kValue9`(0x151, plus an `IsActiveMoveObject()`/`RemoveItem()` branch)/
  `kValue11`(0x2FE) additionally dispatch through the object manager
  (`TGAction::ConvertToEvent()` -> `TGObjectManager::HandleEvent()`, both new,
  the latter opaque - `TMouseEventEnum` carries no known values) and derive
  `handled` from a second game-data int field each (0x30C/0x30E/0x30D); the
  wheel values 12/13 (fields 0x2FF/0x300) do the same action+dispatch but
  **return immediately**, skipping everything below. Unhandled values (0, 1,
  3, 6, 7, 8, 10) fall through with `handled = false`.
- The scene-mouse-position Lua hook (same "SceneMousePositionHook" name and
  unimplemented-dispatch-contract gap as `HandleMouseMove`/`ProcessMessage`/
  `HandleEngineEvent`/`HandleKeyEvent`) may override the click position.
- If the current scene `IsMenu()`, calls a new `TGScene::SelectSavegame()`
  stub with that position first, then continues into the same logic below
  regardless.
- One real gap flagged rather than guessed: when a 3x3 transform matrix is
  active on a global matrix stack (gated by two globals, `invMatrix1` and
  `qword_1209B08`, checking whether exactly 9 floats are currently pushed),
  the click position gets transformed through it via `idMat3::operator*
  (idVec3 const&)` before use - none of `invMatrix1`/`idMat3`/`idVec3` are
  reversed or declared anywhere in this codebase (likely a rotatable/
  zoomable scene camera feature, orthogonal to mouse handling itself). The
  untransformed position is used unconditionally instead.
- Finally: if the hover-interface-object set (`_hoveredInterfaceObjects`,
  the same member `HandleMouseMove` maintains) is non-empty, dispatches
  through the object manager and returns. Otherwise, if the current object
  was already empty (`TGObjectManager::IsCurrentObjectEmpty()`, new) and
  the earlier `handled` flag is set and the engine isn't paused, or -
  separately - if the current object is walkable
  (`IsCurrentObjectWalkable()`, new) and the engine isn't paused: checks
  whether `_previousCharacter`'s field-0x1F7 link matches the current
  scene, and if so records the click position (via a new
  `TPaintControl::GetRelativePoint()` stub) into the character's field
  0x201. Otherwise dispatches through the object manager one more time.

New stub/real surface: `TGDialog::HandleMouseClick()`/`HandleMouseWheel()`,
`TGObjectManager::HandleEvent()`/`IsCurrentObjectEmpty()`/
`IsCurrentObjectWalkable()`, `TCursorControl::IsActiveMoveObject()`,
`TGScene::SelectSavegame()`, `TPaintControl::GetRelativePoint()` (all
stubs), `TGAction::ConvertToEvent()` (stub, returns a value-initialized
`TMouseEventEnum`), and the new opaque `TMouseEventEnum` type.

## TGameControl batch 34: SaveEventHandlers, resolved with fresher eyes

Revisited `SaveEventHandlers()` (Deponia_Linux.asm lines 457514-458044,
~530 lines), left as an explicit stub in batch 29 over a "narrow vs. wide
string element type" ambiguity. Separating the function's four handler
loops by their distinct append-call signatures (rather than by stride
alone - a plain 8-byte pointer stride is equally consistent with either a
narrow or wide COW string under this project's old-ABI assumption) resolved
it:

- The `mainLoop:`-prefixed loop appends each element via
  `std::wstring::append(std::wstring const&)` directly - only type-checks
  against wide-string elements. This means `_engineEventHandlerNamesMainLoop`
  (added in an earlier batch as `vector<std::string>`, purely from ICF-
  destructor-symbol inference with no direct type evidence) was **wrong**;
  corrected to `vector<std::wstring>`, with `RegisterEventHandlerMainLoop`/
  `UnregisterEventHandlerMainLoop` updated to match (no more UTF-8 round-
  trip through `toUTF`/`mb_str`, just `wxString::ToStdWstring()`).
- The `engineEvent:`-prefixed loop instead converts each element through
  `toUTF(wxString*, const char*)` - confirms `_engineEventHandlerNames`
  (TMasterControl's member) really is narrow, as already modeled.
- The `keyEvent:` loop appends `_keyboardEventHandlers` elements directly as
  wstrings, consistent with `TKeyboardEventHandler{wxString name}` already
  being a bare 8-byte wrapper.
- The `mouseEvent:` loop (over `_mouseEventHandlers`, moved from private to
  protected - same reasoning as the other TMasterControl containers
  TGameControl reads directly) confirmed the real per-name filter-list
  format `LoadEventHandlers` could only guess at: each entry renders as
  `name|filter1|filter2|...` (pipe-joined, including between the name and
  its first filter), with entries themselves comma-joined - via a new
  `CONVTOSTR(const int&)` free function (recovered name, `WxStub.h`,
  rendering plain decimal).
- Two further single-value categories, `animationStarted`/`animationStopped`/
  `textStarted`/`textStopped`, turned out to be stored on `TGAnimation`/
  `TGText` themselves (four new static getters, all stubs) rather than in
  any `TGameControl` container - explaining why `LoadEventHandlers`' own
  comment couldn't place them among its four regular categories.

The assembled string is written back via `TVisObjRef::SetValue(0x2F7, ...,
TSendEventEnum::kNoEvent)` - the same field `LoadEventHandlers` reads.

## TGameControl batch 35: implement LoadGame(TMSavegame*)

Reversed `TGameControl::LoadGame(TMSavegame *savegame)` (Deponia_Linux.asm
lines 477405-478377, ~970 lines) - loads a savegame slot's data file.
Confirmed control flow:

- Computes the currently-loaded game's own file name (`_visionaire->
  GetGame().GetPath(0x268)`, then `wxFileName::GetFullName()` - new stub) and
  compares it against the savegame's own composed/container file
  (`TMSavegame::GetSavegameComposedFile()`, new stub) - if the savegame names
  a non-empty container that differs, the two "belong to different base
  games/episodes."
- Registers a fixed container id, the literal `"SAVEGAMEPWD30"` (recovered
  byte-for-byte, also referenced by `TMSavegame::Draw()`/`SaveGame()`), as
  the savegame container via a new `TComposedFileManager::SetSavegameFile()`
  stub, keyed by the savegame's own (normalized) file name
  (`TMSavegame::GetFileName()`, new stub). Failure here fires a
  `"LoadingSavegameFailed"` engine event (with the savegame's file name as
  the event argument) and returns false.
- If the container names differed (see above), first calls `ReplaceGame()`
  with the savegame's own composed-file path - swapping the active game/
  container before loading the actual save data, presumably for savegames
  belonging to a different base game.
- Builds a fixed-literal target file name inside that container,
  `"vtp_savedata.xml#g#-01#00000#"` (also recovered byte-for-byte) - the
  `#`-delimited segments look like unsubstituted template placeholders
  (game id? slot number? a counter?), but nothing in this function
  substitutes them, so they're passed through verbatim; presumably resolved
  inside the still-unreversed `SetSavegameFile`/`TVisionaire::LoadSaveGame`
  itself.
- Calls `TVisionaire::LoadSaveGame()` (new - see the note below on why it's
  placed on `TVisionaire` rather than `TVisionaireGame`) with that target
  file and the same container id. Failure fires the same
  `"LoadingSavegameFailed"` event and returns false.
- On success, calls `Load()` (unconditionally discarding its own return
  value - confirmed, no check follows it), then unconditionally fires a
  `"LoadingSavegameSuccess"` engine event and returns true.

Another `TVisionaireGame`/`TVisionaire` mismatch, same shape as
`LoadDataGame`'s already-documented gap (batch 24-ish): IDA resolves
`LoadSaveGame`'s symbol as `TVisionaireGame::LoadSaveGame`, but the
confirmed `this` pointer at the call site is `_visionaire` (`TVisionaire*`),
not `_visionaireGame` - placed on `TVisionaire` to match the confirmed
pointer rather than the resolved symbol's own class name.

New stub/real surface: `TMSavegame::GetSavegameComposedFile()`/
`GetFileName()`, `TComposedFileManager::SetSavegameFile()`,
`TVisionaire::LoadSaveGame()` (all stubs, `false`/empty by default - so the
success path is currently unreachable in this stub build, matching this
project's usual "faithful control flow around not-yet-reversed
dependencies" pattern), plus real `wxString::Cmp()` and
`wxFileName::GetFullName()` in `WxStub.h`.

## TGameControl batch 36: implement Load

Reversed `TGameControl::Load()` (Deponia_Linux.asm lines 476661-477395,
~735 lines) - the savegame-state restoration counterpart to `Save()`,
called only from `LoadGame(TMSavegame*)`.

A useful side effect of this pass: double-checking `GetCurrentCharacterPointer()`'s
own disassembly against Load()'s hash-lookup code (both read the same field)
confirmed it's a plain `mov rax,[rdi+338h]; retn` - a field read, matching
the already-correct existing implementation (`return _currentCharacter;`).
This also gave a clean, independently-confirmed offset map for a cluster of
members this pass depends on:
`_currentCharacter` (0x338), `_characters` (0x340, begin/end pair),
`_currentText`/`_activeTexts`/`_sceneTexts` (0x380/0x388/0x398, matching
`ClearTexts()`'s own asm exactly), `_dialog` (0x3A8), `_allInterfaces`/
`_activeInterfaces` (0x140/0x150), and `_previousCharacter` (0x290,
reconfirming batch 21's fix).

Confirmed control flow:

- Resets `_timingValueSeconds` from field 0xF6 (same derivation as
  `LoadAndInitGame`'s own use of it) and calls `ClearTexts()`.
- Updates `_currentCharacter` from field 0x1D4 - both of the asm's IsEmpty()
  branches turned out to perform the identical hash-lookup-by-id that
  `GetCharacterPointer()` itself already implements (falling back to the
  unchanged `_currentCharacter` on a miss), so this collapses to a single
  `GetCharacterPointer()` call rather than a duplicated inline branch. The
  same collapse applies later to `_previousCharacter`'s own update from
  field 0x263.
- Sets the dialog from field 0x1DC (`TGDialog::SetDialog()` - already
  existed) and remembers field 0x1D5 for a `ShowScene()` call later.
- Rebuilds every text object linked from field 0x18's list (a fresh
  `THText` per entry, via a new 2-argument constructor overload distinct
  from the existing 9-argument one - confirmed by its own mangled
  signature), sorting each into `_currentText` (the one matching field
  0x1DD, which `ClearTexts()` independently confirms is the same field that
  identifies "the current text's target"), a plain `_activeTexts` entry, or
  a `_sceneTexts` entry that also tries to reattach to whatever managed
  object sits at its own field-0x2AC target - falling back, in order, to a
  character lookup (`GetCharacterPointerEx()`, gated by the target id's own
  byte[3]) and then a scan of `_activeInterfaces`.
- Calls `RemoveAllItems()` then (after `TGAction::LoadActions()`, new) each
  character's new `Load()` virtual (vtable slot 0xC0, placed symmetrically
  with the existing `Save()`), then each interface's new `Load()`, then
  `TGObjectManager::SavedObjectChanged()` (new), `SetInterfaces()` (existing),
  `TGAnimation::LoadAnimations()` (new), `LoadEventHandlers()` (existing),
  and a new `LoadGlobalScriptVariables()` free function.
- Clears `_isClearingAnimations`; one further qword field reset (asm line
  477018) wasn't confidently identified with any modeled member and is left
  unimplemented rather than guessed.
- Restores the scroll position (new `TSceneControl::SetNextStartScrollPos()`)
  and shows the remembered scene (`ShowScene()`, existing), then sets
  scrollability either to `false` (field 0x1D8) or from the current scene's
  own field 0xE8.
- Re-touches field 0x132 (fetch, `ClearLink(false)`, then `SetLink(true)`
  with the same value) - presumably to force a change notification without
  an actual value change, given the differing bool flags on the two calls.
- Restores earthquake state from fields 0x275-0x277 (`StartEarthquake()`/
  `StopEarthquake()`, both existing), then finishes with the exact same
  "`_lastMousePos != {-1,-1}` -> re-dispatch `HandleMouseMove`" logic
  `UpdateCurrentObject()` already implements - called directly rather than
  reproducing the inlined vtable dispatch.

Another `TVisionaire`/`TVisionaireGame` data point (see `LoadDataGame`/
`LoadSaveGame`'s own comments for the established gap): the new
`LoadGlobalScriptVariables()` free function's confirmed `TVisionaireGame&`
parameter is passed the SAME field offset (TGameControl+0xC8) that every
other call in this function reads as `_visionaire` (TVisionaire*) - matched
here against the already-shipped `Save()`'s own `SaveGlobalScriptVariables(
*_visionaireGame)` call instead, rather than resolved into one member.

New stub/real surface: `TGCharacter::Load()` (virtual), `TGInterface::
RemoveAllItems()`/`Load()`, `TGObjectManager::SavedObjectChanged()`,
`TGAction::LoadActions()`, `TGAnimation::LoadAnimations()`, `TGText::Load()`,
a second `THText` constructor overload, `TSceneControl::
SetNextStartScrollPos()`, `LoadGlobalScriptVariables()`, and a second
`TVisObjRef::operator==(const TVisionaireObject&)` overload (all stubs
except the real, evidence-backed control-flow logic above). Also adds
`TGameControl::s_stopTime` - a genuinely recovered static member name (its
own linker symbol, unlike a plain instance field), kept as-is per CLAUDE.md's
recovered-global exception.

## TGameControl batch 37: implement Update, the last unreversed method

Reversed `TGameControl::Update()` (Deponia_Linux.asm lines 469514-470955,
~1440 lines) - the per-frame dispatcher, and the single largest method in
the class. With this, every one of `TGameControl`'s ~98 methods has a real
implementation or an explicitly-documented, evidence-backed stub; none are
bare unexamined placeholders anymore.

Confirmed control flow, roughly in order:

- Drains the global `EngineEvents` queue (already-known infrastructure from
  `PushEngineEvent`) under `EngineEventLock`, firing `HandleEngineEvent()`
  for each queued pair, then clears it.
- If not `EngineUpdatePaused`: `MoveScene()`, `UpdateWalkingSounds()`,
  `UpdateRandomTimers()` (all pre-existing).
- `TGAnimation::ContinueAnimations()` (new), then (if not paused)
  `HandleCharacters()` (pre-existing), then updates the cursor's active/
  inactive appearance based on whether a dialog is showing (`TGDialog::
  IsActiveDialogPart()`, new) or, if not, whether the current object is
  detectable (`TGObjectManager::IsCurrentObjectDetectable()`, new) -
  discovered these three unrelated-looking things are all bracketed by one
  `"Animations"` profiler region in the original.
- `UpdateTexts()` (pre-existing), then unconditionally `TGAction::
  ContinueRunningActions(false)` (pre-existing) and `TGAction::
  DeleteFinishedActions()` (new).
- If not paused: `TMasterControl::ScrollUpdate()` (pre-existing); always:
  `TGScene::SortAllObjects()` (new); if not paused: `TGScene::
  UpdateSnoopAnimAlpha()` (new).
- One unresolved pair of virtual calls (vtable slots 0x100/0xE8 on a pointer
  field not confidently matched to any modeled member - `TGameController`
  itself has no vtable, ruling out the obvious guess) - left unimplemented
  and flagged rather than guessed at.
- Iterates every picture the preloader has queued (`TPreloadedPicManager::
  GetPreloadedPictures()`, new - the original's nested bucket/array
  structure is flattened to a plain `vector<TPictureIO*>` here, since
  nothing depends on its exact shape) and calls `TPictureIO::CreateSprite()`
  (already existed) on each - resolving, along the way, that the vector
  holds `TPictureIO*` specifically (confirmed by `TSprite::GetPathNonConst()`,
  new, and `CreateSprite()` both being called on the same pointer, and
  `TPictureIO`'s already-documented `: TPictureMEM : TSprite` chain).
  Refreshes the sprite cache (`TGraphicsInterface::GetCacheSpriteCount()`/
  `UpdateCache()`, both new) when the count has changed.
- A `"Tweens"` region (skipped entirely when paused, but always refreshing
  `_lastUpdateTicks` from `SDL_GetTicks()` at the end) that: updates
  `_visObjTweens` via a new `TVisObjTween::update(double)`; updates
  `_pendingTweens` (see the type-correction below) via new `Tween::Update()`/
  `IsFinished()`; and counts down `_delaysByName`/`_delaysById`, firing
  `LuaDoString()` (a new one-argument overload) or a new `LuaDoRef(int)`
  free function respectively when a delay expires. The original also syncs
  each tween's live interpolated value into Lua globals/tables every frame -
  not reproduced, since `Tween`'s own interpolated-value fields are still
  unknown (see `Tween.h`'s own updated header comment).
- A real, evidence-backed type correction: `_pendingTweens`'s element type
  was a guessed `pair<Tween,string>`, already known wrong. This function's
  own processing of that exact vector confirms the element is just `Tween`
  by value (`Tween` itself now carries a `name` field, used for the Lua
  sync above) - `StartTween(Tween,string)` itself remains a stub (its own
  erase/replace-by-name logic wasn't re-examined this pass).
- The registered "mainLoop" event handlers (`_engineEventHandlerNamesMainLoop`,
  already fixed to `vector<wstring>` in batch 34) get dispatched through Lua
  once per frame when not `MainLoopsPaused` (new global) - left as an
  unimplemented-dispatch comment, the same `LuaExecuteFunction`/`TArgument`
  gap already noted throughout this project.
- Finally checks Steam/Galaxy SDK status and calls their own `Update()` when
  active (four new methods on `TSteamSDK`/`TGalaxySDK`, plus two new
  `TGameClientSDK` accessors for the previously-private pointers).

Also new: `TCPDebuggerClient` (`Diagnostics.h`) - a second, distinct
per-frame profiler from the existing `TDiagnostic`, bracketing every named
section above via a global `debugger` instance; stubbed as pure no-ops since
it's a network-facing dev tool with no gameplay effect, kept only so the
call sites (and their section-boundary information) stay faithful to the
original.

The function's own return value isn't confirmed (the epilogue never sets
`eax` explicitly before returning, unlike a typical `bool`-returning
method) - kept as an unconditional `true`, matching this method's
pre-existing stub behavior rather than guessing at real semantics.

## TLoadingControl: implement Draw/UpdateStatus/EndLoading; Init left as a flagged gap

With `TGameControl` fully covered, moved to `TLoadingControl` (manifest:
stub, 7 methods) - small and directly adjacent (`TMasterControl::
ShowLoadingScreen`/`Signal` already call into it). Reversed the
constructor/destructor's field layout, `Draw()`, `UpdateStatus(int,int)`,
and `EndLoading(TSoundInterface*)` in full; `Init(SLoadingScreen&,
TSoundInterface*)` itself is left as an explicit stub over a genuine, newly-
discovered layout ambiguity (see below) rather than guessed at.

Confirmed fields (from the ctor's zeroing pattern and every other method's
own offsets): two embedded `TPictureIO` objects (`_progressBarPic`,
`_backgroundPic` - `Draw()` draws the background full-surface and the
progress bar through a moving/resizing source rect), a pair of `wxRect`s
(`_totalRect`/`_fillRect` - the bar's full bounds and its filled portion),
a `wxPoint` (`_fillOrigin`, a base position for one of two progress-fill
visual modes), a `bool` (`_progressFillsForward`, which mode is active),
and a `wxFileName` (`_soundPath`, a loading sound EndLoading() plays once if
Init() never managed to). `UpdateStatus()`'s two modes: one grows a filled
rect proportionally from nothing; the other shrinks a "remaining" rect while
also sliding the progress-bar picture's own position via a new `TSprite::
SetPosition()` - both confirmed via careful reading of the float/int math,
not approximated.

The `Init()` gap: its disassembly calls `TSprite::GetPath()`/`GetWidth()`/
`GetHeight()`/`SetPosition()` directly on its `SLoadingScreen&` parameter's
image fields. Cross-checking `TSprite::GetPath()`'s own disassembly (not
previously read) shows it actually reads its path field at offset **+0x30**
within `TSprite` - not +0, contradicting this project's current `TSprite`
model, which has `_path` as the very first field. `GetWidth()`/`GetHeight()`
read +0x18/+0x1C, and (from `TSprite::SetPosition()`/`GetSize()`, also
freshly read) a position lives at +0x20 and a scale float at +0x48 - none of
which reconcile with the current 5-field struct. `TMasterControl::
SetLoadingScreen()`'s own memberwise copy additionally shows each
`SLoadingScreen` image field is 16 bytes wide (the gap between consecutive
copied sub-fields), not the 8 a bare `TCharHolder` occupies. All of this
points at `SLoadingScreen`'s image fields actually being full `TSprite`
objects, not `TCharHolder`s - but fully resolving it means reversing much
more of `TSprite` itself (a stub, ~35 methods, otherwise untouched) and
re-deriving `SLoadingScreen`'s whole layout from `SetLoadingScreen`'s own
copy, which is its own dedicated pass. `TSprite::SetPosition()` was still
added (against its own directly-confirmed, offset-independent behavior) so
`UpdateStatus()` could be implemented for real; `TSprite.h` now carries a
class-level comment flagging the rest of this gap explicitly for whoever
picks it up next.

Also added: `TSoundInterface::Play(const wxFileName&)` - a new virtual,
confirmed distinct from `TSoundFFMPEG::PlaySound`'s own vtable slot,
exercised by `EndLoading()`.

## EventHandler: implement the pending-event queue, add Event

`EventHandler` (manifest: stub, 4 methods) turned out small and
self-contained once its own asm range was isolated from the unrelated
classes interleaved around it in the manifest's line span. Implemented
`ProcessPendingEvents()` (asm lines 1594325-1594376) and
`AddPendingEvent(Event*)` (asm lines 1599448-1599510) for real: both
confirmed to operate on genuinely global state (a `wxCriticalSection` plus a
`std::vector<std::pair<Event*, EventHandler*>>`, mangled type confirmed
directly from `AddPendingEvent`'s own `_M_emplace_back_aux` call) rather
than anything per-instance - the same "global, not per-instance" shape
already established for `TGameControl`'s own `EngineEvents` queue.
`AddPendingEvent` clones the incoming event (a new virtual, `Event::Clone()`)
before queuing it; `ProcessPendingEvents` dispatches each queued event to
its paired handler (a new virtual, `EventHandler::HandleEvent()`) and then
deletes it.

Added a new `Event` class (previously nonexistent) with just the one
confirmed virtual. Both `Clone()`/`HandleEvent()` names are guesses from
their call shapes, not recovered identifiers.

## TMemoryFile: corrected from a TMemoryBuffer wrapper to its own raw buffer

`TMemoryFile` (manifest: stub, 7 methods) turned out to be a real type
correction, not just a fill-in: it was previously modeled as wrapping a
`TMemoryBuffer`, but its own ctor/dtor/GetBuffer/GetSize/ReleaseMemory/
Reserve/DetachBuffer (Deponia_Linux.asm lines 551869-552136) show it's
actually just a plain owned `new[]`-allocated byte buffer plus a size -
unrelated to `TMemoryBuffer`. Nothing else in this codebase depended on the
old modeling (`TMemoryFile` was only ever an untouched stub out-parameter in
`TComposedFile::GetMemoryFile()`), so this was a zero-risk correction rather
than a breaking one.

The original member name, `m_pData`, is recovered byte-for-byte from
`GetBuffer()`'s own `x_assert()` string, along with the confirmed original
path `src/baselib/memfile.cpp` (both now recorded in the header, resolving
its earlier "not yet reconstructed" note).

## TFontManager: implement the whole class; add TCFont as a signature-only stub

Reversed all of `TFontManager` (manifest: stub, 16 methods, Deponia_Linux.asm
lines 1388826-1390289) for real, following the exact same "own state fully
reversed, deep leaf dependency left as a call-shape stub" split already used
for `TGameControl::Update()`'s picture/sprite section: `TFontManager` itself
now has a complete, faithful implementation, while the actual font
rendering/layout it delegates to (`TCFont`, manifest: todo, 16 methods, not
reversed at all) is a brand-new class with only the method signatures
`TFontManager` needs, all stubbed.

Confirmed structure: `TFontManager` owns every loaded `TCFont` in a
positionally-indexable vector (needed by the new `SetCurrentFont(int&)`
overload, which cycles through fonts by wrapping/clamping an index) plus a
hash table keyed by the same packed-id scheme as `TGameControl`'s own
`_charactersByHash` (same simplification: a plain `std::unordered_map`
instead of the original's open-hashing bucket/node layout). Every accessor
(`PrintText`, `PrintTextLines`, `GetTextDimension` x2, `GetLineHeight`,
`PerformAutoLineBreak`) is a thin guard-then-delegate wrapper around a
tracked "current font," which behaves identically to the original's own
pointer-into-the-values-array/sentinel trick as a plain nullable `TCFont*`.
`SplitTexts()` re-splits list entries that don't fit a max width (the
disassembly's manual list-node splicing is just inlined `std::list`
splice/erase codegen, reproduced with the equivalent standard calls) and
`Signal()` handles two new signal types (`kSignalPrintText`/
`kSignalPrintTextLines`, added to `TSignalData.h` along with a `text`/
`point`/`fontId` field set - `TSignalData`'s own header already flags it as
"almost certainly a tagged union," so these are modeled as independently-
named fields rather than reusing `kSignalLoadingProgress`'s overlapping
byte range).

Also extracted `PackVisId()` from `gameControl.cpp`'s anonymous namespace
into `datastruct/visionaireobject.h` as a shared inline free function, since
`TFontManager` now needs the exact same packing scheme in a different
translation unit.

One architectural note left unresolved: `TFontManager`'s constructor writes
a vtable pointer, so it's genuinely polymorphic in the original, and its
`Signal()` signature exactly matches `TMasterControl::Signal()`'s (also
virtual) - hinting at a shared, not-yet-identified signal-handler base
class. Not modeled (`TFontManager` stays non-polymorphic here), since
nothing currently depends on dispatching through it virtually.

## TArgument: implement the whole 57-method tagged union with std::variant

Reversed all of `TArgument` (manifest: stub, 57 methods, Deponia_Linux.asm
lines 1435493-1439510+) - a tagged-union style value passed to/from Lua
script calls. This was the largest single-class undertaking so far after
`TGameControl`/`TFontManager`: 20 distinct type tags (0-19, read directly off
`Clear()`'s/`SetType()`'s/`ToLua()`'s/`CopyTo()`'s own raw switch/cmp chains),
each carrying a different C++ type, with matching `Set`/`Get`/`Add` overloads
and a handful of `Convert*` coercions.

The whole class is modeled with a single `std::variant` instead of the
original's manual per-type `new`/`delete` and reuse-existing-allocation
switch dance (visible in nearly every `Set()`/`Add()` method) - collapsing
~40 methods' worth of heap-management boilerplate into one-line `_value = x;
_type = kY;` assignments, since nothing depends on the original's exact
memory layout (same "behavioral over binary fidelity" reasoning as
`_charactersByHash`/`TFontManager`'s own font table elsewhere in this
project). `kString`/`kPath` share one `wxString` variant alternative (they
differ only in `SetPath()`/`AddPath()`/`GetPath()`'s "vispath:"-prefix
handling, recovered byte-for-byte, not in storage), and `kStringList`/
`kPathList` likewise share one `vector<TCharHolder>` alternative.

Added a new `TTextLanguage` struct (two `TCharHolder` fields plus an int,
confirmed field count/order from `Set(const TTextLanguage&)`'s own copy
logic; field names are a guess from the class name). `ConvertToObject()`/
`ConvertToObjectList()` needed three new dependencies - `TVisionaire::
GetAnyObject()`, `GetLuaGame()`, `FindObjectByNameOrId()` - all call-shape
stubs, plus a real `TId::operator==()` (stubbed false, since `TId`'s own
fields are still unknown) and an `AnyId` sentinel global. `ToLua()` itself
is left as a call-shape stub: its disassembly pushes values straight onto
the Lua stack via the raw C API (`lua_pushstring`/`lua_pushboolean`/etc.) or
a separate `ConvertToLua()` free-function family - the same standing
"Lua bridge contract not reversed" gap already noted throughout this
project (`LuaExecuteFunction`, `LuaDoString`, and others).

Also added two real `wxString` methods needed for the path-prefix handling
(`StartsWith()`, `Mid()`) and a `TCharHolder(const wxString&)` constructor
(confirmed call shape, previously missing).

## TComposedFile::GetMemoryFile

Confirmed in full (Deponia_Linux.asm lines 545837-546088): reads one
archived entry's raw bytes into a caller-supplied `TMemoryFile`. Notable
findings:

- Unlike `Open()` (same class), this method calls `InitEntries()` when
  `_needsInit`/`_hadOpenError` is set but **ignores its return value**,
  always falling through into the main logic - which then fails closed on
  its own via the bounds check if entries genuinely weren't loaded. This is
  a real, confirmed difference between the two methods, not an
  approximation.
- The in-memory `_entries` record stride is confirmed as exactly 48 bytes
  (`(end-begin)>>4` then a reciprocal-multiply `/3`, i.e. `48 = 16*3`) - this
  independently corroborates `SEntryInfo`'s own header comment, which had
  already flagged "48 bytes... plus 5 more qwords" as an open question.
  This method reads two qwords directly off a raw
  `_entries.data() + index*48` pointer at relative offsets +0x10 and +0x18:
  the first (`+0x10`, previously entirely unmodeled) is the entry's **byte
  offset within the container file** (passed to `wxFile::Seek()`); the
  second (`+0x18`, previously the placeholder `field18`) is the entry's
  **byte size** (passed to `TMemoryFile::Reserve()` and used as the
  read-loop's target length). `SEntryInfo` was updated accordingly
  (`field18` renamed to `byteSize`; a new `containerOffset` field added for
  +0x10). `field20`/`field28`'s meaning is still unknown.
- The failing-`Reserve()` path logs via `wxLog::logexpanded()` with a
  message decoded byte-for-byte from the binary's own wide-string constant
  (`unk_D75408`): "Memory for memory file %s with length %d could not be
  reserved".
- The original's own read loop calls `wxFile::Read()` with the *full*
  `byteSize` into the *same* buffer start on every iteration (not resuming
  from the current position/remaining count) - almost certainly harmless in
  practice since a local-file `Read()` satisfies the whole request in one
  call, but not literally reproduced here (see "behavioral over binary
  fidelity"); this implementation reads once and treats any short read as
  failure, matching the original's success/failure outcome either way.
- `wxFile` gained real `Open(const wxString&, int)` and
  `Seek(unsigned long, int)` methods (previously missing) to support this;
  the real wxWidgets `OpenMode` ordinal for the `mode=1` value seen at this
  call site wasn't independently confirmed, so `Open()` always opens for
  reading regardless of `mode` (the only call site reversed so far never
  writes).

`TComposedFile::LoadEntriesFromDisk()` (the on-disk-record unpacking step
that actually populates `_entries`) remains an unreversed placeholder, so
`GetMemoryFile()` is structurally complete but not yet exercised end-to-end
by any real archive - consistent with this project's pattern of
implementing a caller fully while leaving a still-unreversed callee flagged
separately (e.g. `TFontManager`/`TCFont` earlier).

## TCharHolder

Confirmed in full (Deponia_Linux.asm lines 529250-531666, all 32
manifest-listed methods) - a genuinely recovered class name (demangled
byte-for-byte from `_ZN11TCharHolder...` symbols), not a guessed one. This
corrects a significant earlier misunderstanding: `TCharHolder` had been
modeled as a thin `wxString` wrapper, but the real class is its own owned
`char *` + byte-count pair (`new[]`/`delete[]`), always storing narrow
bytes - exactly the same "raw owned buffer" shape as `TMemoryFile`
elsewhere in this project, not a wxString at all. Wide input (wchar_t*,
wxString, wxFileName) is always narrow-converted via `wxString::mb_str()`
before being copied in.

Notable findings:

- `_data == nullptr` iff `_size == 0` is a strict invariant every mutator
  maintains. `Cmp()`/`CmpNoCase()`/the `operator==` family/`SameAs()` all
  treat this "null" state as distinct from - and never equal to - any
  non-null value, even an empty one (confirmed directly from the
  disassembly's branch structure, not inferred).
- `c_str()` was a complete misattribution in the pre-existing stub: an
  earlier pass's "confirmed call shape only" note for it actually pointed
  at a plain `std::string::c_str()` call on an unrelated `std::vector<
  std::string>` field, not at `TCharHolder::c_str()` at all. The REAL
  `TCharHolder::c_str()` (traced directly from its own disassembly this
  time) returns a `wxString` by hidden-pointer value - built via `toUTF()`
  from the raw bytes - not a `const wchar_t*` as the old stub assumed.
  Despite the name, it's behaviorally identical to `GetFullPath()`.
- `mb_str()`, conversely, returns the raw stored `const char*` directly (or
  `""` when unset) - no conversion needed, since the internal storage is
  already narrow. This replaces an earlier placeholder that (reasonably,
  before this class was traced) assumed a `wxString::CharBuffer` return to
  match `wxString::mb_str()`'s own shape.
- `resize()` is a raw reservation, not a content-preserving resize: it
  always discards any existing buffer and allocates a fresh, uninitialized
  one of exactly the requested size (or frees outright when the requested
  size is 0).
- `exchange()` is a move-assign despite its name (frees this's own buffer
  first, steals `other`'s, clears `other`) rather than a symmetric swap.
- `GetFullPath(wxPathFormat)`'s non-`wxPATH_UNIX` branch attempts a
  `wxString::Replace()` of an empty search string for a path separator -
  which real `wxString::Replace()` documents as a no-op (searching for ""
  is guarded against) - so every format observably returns the same value
  as plain `GetFullPath()`.
- Two real `wxString` methods needed adding: `MakeLower()` and
  `ToDouble(double*)`.

Two existing call sites in `gameControl.cpp` (`TGameControl::
LoadAndInitGame`/`ReplaceGame`, both doing `wxString(link.GetName())`) and
one in `TArgument::ConvertToObjectList()` relied on an implicit `operator
wxString()` conversion the old stub provided; at the time this class was
reversed, that conversion had no confirmed call site among TCharHolder's
32 manifest-listed methods, so those 3 sites were switched to explicit
`.GetFullPath()` calls rather than resurrecting an apparently-invented
implicit conversion.

**Correction, found while later reversing TSprite (see that section
below):** `operator wxString() const` and `operator wxFileName() const`
ARE real, confirmed methods (Deponia_Linux.asm lines 531676-531864,
`TSprite::GetName()`/`GetPath()` call them directly) - they just weren't
in that "32" count because the function-level manifest's automated "owner"
extraction couldn't handle their `operator cv...` mangling and miscounted
them as free functions. Both are now implemented (`operator wxString()`
byte-for-byte identical to `GetFullPath()`; `operator wxFileName()`
likewise but through an actual `wxFileName` plus a `NormalizePath()` call).
The 3 explicit-`.GetFullPath()` call sites above were left as-is rather
than reverted, since both spellings are equivalent now that the operator
is real.

## TMemoryBuffer

Confirmed 23 of 26 manifest-listed methods in full (Deponia_Linux.asm lines
549297-551866) - a growable owned byte buffer (`unsigned char *` + used
length + allocated capacity), used both as a scratch read buffer
(`TFile::ReadToBuf`) and as a byte-stream builder (the `operator<<` family
and `AppendStringWithLen`, used by `TXMLWriter` and others). `Reserve()`/
`Init()`/`EnsureBufferSize()` are three subtly different flavors of "ensure
capacity" - only `EnsureBufferSize()` preserves existing content across a
grow (a real memcpy), and only `Init()` unconditionally resets the used
length to 0 even when no reallocation happens. The append family
(`AppendData`/`AppendByte`/every `operator<<`/`AppendStringWithLen`) all
share one growth strategy: growing by `newLength + 0x400000` (4 MiB of
headroom) whenever the current capacity is insufficient - confirmed from a
literal `+0x400000` immediate in the disassembly, not a guess.

The genuinely exciting find: `Decrypt()`/`Encrypt()` are a REAL, fully
confirmed cipher, not a no-op stub as the pre-existing code had it -
`Encrypt()` is just `Decrypt()` (XOR is its own inverse), and `Decrypt()`
XORs the buffer against a repeating 16-byte keystream that is
`MD5(narrow-converted key)`. This directly resolves the encryption gap
`TComposedFile::InitEntries()` and `baselib/composedfile.h`'s header
comment had flagged as unreversed for the archive format's header-table
decryption. A real RFC 1321 MD5 was added (`MD5.h`/`MD5.cpp` - MD5 is a
public algorithm, not proprietary engine logic, so this is a faithful
implementation rather than a recovered one) and self-tested against the
three standard test vectors (`""`, `"abc"`, the pangram) before wiring it
in.

`Compress()` and both `Uncompress()` overloads call the real zlib C API
directly (confirmed by their exact `compress`/`uncompress` symbol names,
plus zlib's own well-known `len*1.001+12` worst-case-output-size formula
appearing literally in `Compress()`'s disassembly) - this project doesn't
currently link zlib, so these 3 methods stay call-shape stubs, the same
"genuine third-party API boundary" treatment already given to the Lua C
API and the Steam/Galaxy SDKs. Adding a zlib dependency is a build-system
decision left for later rather than made unilaterally here.

`GetSize()` in the pre-existing stub is renamed to `GetLen()`, the actual
recovered name (`GetSize()` had no real call sites anywhere in this
codebase).

## TSprite

Confirmed 35 of 35 manifest-listed methods except ToLuaString()/
SetFromLuaString() (Deponia_Linux.asm lines 582998-584513) - a
considerably larger and differently-shaped class than the earlier 5-field
placeholder model (`_path` as the first field) that `TLoadingControl.h`'s
own header had already flagged as contradicted by `TSprite::GetPath()`'s
real offset. The real, confirmed layout: a vtable pointer @0x00 (TSprite is
genuinely polymorphic - the classic deleting/non-deleting virtual-
destructor pair - modeled here with just `virtual ~TSprite() = default`,
matching `TPictureMEM` which already declared its own destructor virtual
in anticipation), `_name` (TCharHolder) @0x08, `_imageWidth`/`_imageHeight`
@0x18/0x1C, `_position` (wxPoint) @0x20/0x24, `_transparentColor` @0x28,
`_path` (TCharHolder) @0x30, a flags byte @0x40 (bit 1 = mirrored; other
bits exist - `SetMirrored()` preserves them - but aren't referenced by any
of TSprite's own methods), `_transparencyMode` @0x44, `_scale` @0x48 (a
float PERCENTAGE, default 100.0 - not the 0-1 fraction range an earlier
guess had assumed), `_pause` @0x4C (a short).

This resolves the exact contradiction `TLoadingControl.h` had flagged:
`GetPath()`/`GetWidth()`/`GetHeight()` really do live at +0x30/+0x18/+0x1C,
confirmed independently from TSprite's own accessors now. What's still
open is `SLoadingScreen`'s own structure (are its image fields full
`TSprite`s? `TMasterControl::SetLoadingScreen`'s 16-byte-per-field
memberwise copy doesn't match `sizeof(TSprite)` either) - that needs its
own dedicated pass, `TLoadingControl.h` updated to describe the remaining
gap precisely.

Two independently-confirmed surprises, found the same way twice (so not a
one-off compiler quirk): `Set()` and the copy constructor both deliberately
skip `_imageWidth`/`_imageHeight` - copying a `TSprite` always zeroes those
two fields regardless of the source, presumably because they're meant to
be recomputed once a copy's image is (re)loaded rather than treated as
part of a sprite's copyable identity. `IsTransparencyEqual()` is also
asymmetric in a way `operator==` isn't: it only inspects the *other*
sprite's transparency mode (a `kAny` sentinel value of -1 always matches;
`kColorKey` compares only the colors; anything else compares modes
directly) - genuinely different logic from `operator==`'s own symmetric
mode-then-color comparison, not an inconsistency introduced here.

`ToLuaString()`/`SetFromLuaString()` are left as call-shape-confirmed
stubs - the same "Lua bridge contract not reversed" gap used throughout
this project (`SetFromLuaString()`'s own disassembly literally calls
`LuaDoString()` + a not-reversed `ConvertFromLua(TSprite&, int)`).
`ToLuaString()`'s per-transparency-mode format strings weren't decoded
byte-for-byte.

Fixed `graphicslib/picture.cpp`'s `TPictureIO::GetSpriteName()`, which had
been written against the old (wrong) TSprite model and referenced fields
(`_id`, `_type`) that never actually existed - its own real disassembly
(asm lines 785997-786106) shows it uses `_path`, `GetTransparency()`, and
`TPictureIO`'s own `_flagC0`, plus a third value that's 0 unless
`s_bTestCacheFileTime` is set (then a file-modification-time value, not
modeled). The exact `wxString::privFormat()` output shape remains an
approximation, as it already was before this fix.

## TTimer

Confirmed in full (Deponia_Linux.asm lines 559372-559478, all 6
manifest-listed methods): a millisecond stopwatch backed by real
wxGetLocalTimeMillis()/wxMilliSleep() (both newly added to WxStub, backed
by `std::chrono::system_clock`/`std::this_thread::sleep_for`) rather than
the pre-existing stub's SDL_GetTicks()-based approximation - functionally
equivalent for elapsed-time purposes, just matching the real backing call
now. `GetTime()`'s real return type is `long` (confirmed via a
`wxLongLong::ToLong()` call), not the `std::int64_t` the earlier stub used.
`WaitUntil(ms)` is a pure tail call to `wxMilliSleep(ms)` that ignores
`this` entirely, despite its name suggesting it references the timer's own
state. The constructor confirms a second qword right after the timestamp,
always zeroed and never read by any of these 6 methods - not modeled,
since nothing depends on it.

## TSpriteHandle (partial - scope turned out bigger than expected)

Confirmed AddRef()/Release()/GetRefCount() operate on a refcount at +0x78
(Deponia_Linux.asm lines 772739-772780) - Release() decrements
UNCONDITIONALLY, with no zero-guard (the pre-existing stub had added a
defensive `if (_refCount > 0)` guard not present in the original; removed
to match). Added GetMemorySize() (asm lines 772788-772797), a plain int
read at +0x00, previously undeclared.

Went looking at the destructor (asm lines 772635-772731) expecting a quick
finish for the remaining ctor/AddSpritePart methods, and found this class
is considerably bigger than its "7 methods" count suggested: the destructor
removes itself from a static global registry (`s_all`, vector-like, paired
with a global end-pointer), frees a dynamically-sized collection of
individually-owned pointers at +0x08 (presumably populated by
`AddSpritePart(TSpritePartHandle const&)` - manifest-listed but not
reversed this pass), a separate owned buffer at +0x20, and a separate owned
object at +0x60. `TSpritePartHandle` itself is an entirely separate,
not-yet-touched type. Given the scope (a new companion type plus non-
trivial ownership/registry bookkeeping), left the ctor/AddSpritePart/dtor
as an honest gap for a dedicated future pass rather than guessed at -
this is a case where "keep going" surfaced more work than it resolved, and
that's fine to record plainly.

## TPictureFormat (base class only)

Confirmed the base class's own 4 methods in full (Deponia_Linux.asm lines
771397-771454): a vtable pointer plus one int format code, default 0,
read directly by `GetFormat()`. `ReadHeader()`/`ReadData()`/`Write()` are
genuinely abstract in the original (only ever reached through a subclass
override, never given a body of their own at this level) - declared pure
virtual here with their confirmed signatures (asm lines 739426-739476) so
a future decoder pass has the right shape to implement against. Gave each
of the 5 format subclasses (PNG/WebP/JPG/GIF/PCX) trivial failing overrides
so they stay instantiable (matching `TPictureIO::GetFormat()`'s existing
`new TPictureWebP()`-style usage) - none of the actual decode logic is
reversed, since that wraps the real vendored libpng/libwebp/jpgd libraries
(`manifest/vendor_libraries.tsv`) rather than anything worth reimplementing
by hand; linking those directly is a separate build-system decision, the
same kind as the zlib one flagged for `TMemoryBuffer::Compress()`.

## TComposedFileManager (also: TFile::DecryptHeader, the real archive-header cipher wired end-to-end)

Confirmed in full except `GetComposedFileInfo()`'s own filename-suffix
grammar and `Export()` (Deponia_Linux.asm lines 516915-519803, all 17
manifest-listed methods reached) - a pure static manager coordinating
several global `TComposedFile` instances built directly on today's earlier
`TComposedFile`/`TMemoryBuffer` work. Confirmed global state: a "main"
container, a "savegame" container, a "game" container, three resizable
arrays (scene/character/interface containers - each either one shared
instance or one per numbered volume), a set of "manual" containers built
from a caller-supplied path list, and a plain movie-container path string
(not a `TComposedFile` at all). The original manages the three
array-backed container sets via raw `operator new[]` + placement-new (an
80-byte-per-`TComposedFile` manual array, stepped by pointer arithmetic) -
modeled here as plain `std::vector<TComposedFile>` instead, and the
original's separately-tracked `s_numXxxContainers` counters are dropped in
favor of each vector's own `.size()`, since they're always equal.

`GetComposedFile(const TComposedFileInfo&)` is a clean 9-case switch on
`TContainerTypeEnum` dispatching to the matching global container(s) -
confirmed in full, including a size-dependent `index == -1` special case
for the array-backed types (falls back to the array's own base address
only when there's exactly one container in it) and manual containers being
1-indexed (`index == 1` is the first one) unlike every other case.

`GetComposedFileInfo()` itself - which parses a trailing "#..." suffix off
a filename's extension to decide which container a given path refers to -
is the one piece NOT reversed to the byte level: it's a genuinely intricate
multi-branch parser (checking suffix length and specific character
positions for '#', distinguishing at least a bare "#NNN#vvv" shape from a
longer "#NNNmSS#vvv" one with an embedded container-type letter) that would
need its own dedicated pass. Stubbed to always return false here, which
means every caller (`GetMemoryFile`/`Open`/`Export`/`DecryptComposedFile`/
`GetComposedMovieFileName`) currently always takes its "plain file, not a
composed-file reference" fallback path - each of those fallback paths is
itself fully confirmed and implemented for real (plain `wxFile`/`TFile`
reads), so this is a real, working, honest subset of the class's behavior
rather than a dead stub.

The exciting part: `TFile::DecryptHeader()` (confirmed in full, asm lines
523658-523820) is a complete, working implementation of the real archive-
header cipher, built directly on today's earlier `TMemoryBuffer::Decrypt()`
work - reads the first 0x12C (300) bytes of a file, decrypts them via the
confirmed MD5-keystream XOR cipher, and writes them straight back.
`EncryptHeader()` is confirmed to be a pure tail call to `DecryptHeader()`
(same cipher, same operation either way). This needed three small, fully
real `wxFile` additions: `Write()`, `Flush()`, and `IsOpened()`, plus a
`TFile::SetOriginalPath()` setter (stores the path; nothing reads it back
yet) and a new `wxString::Find()`/`wxFileName::GetName()` pair for
`FileExists()`/`Export()`. `wxFile::Open()`'s mode parameter is now
confirmed for two distinct values: 1 (read-only, from `TComposedFile::
GetMemoryFile()`) and 2 (read-write, from `DecryptHeader()`'s own
read-then-write-back use) - real wxWidgets ordinals still unconfirmed, but
now backed by the right fopen mode for each.

`TComposedFile` gained a `GetComposedFilePath()` accessor (`GetComposedMovieFileName()`
reads this field directly off a manual container in the original).

## TManagedObject

Confirmed 46 of 50 manifest-listed methods (Deponia_Linux.asm lines
114890-194554) - the shared base for every interactive scene object
(`TGCharacter` already derives from it; other call sites confirm `TGObject`/
`TGItem` do too, but those aren't modeled in this codebase yet). This was
a user-directed, dedicated pass (previously flagged as too large to start
casually - comparable in scope to `TGameControl` itself).

Confirmed real field layout: a `TVisObjRef` game-data reference, a
position/bounding-rect/polygon hit-test area, a primary animation plus a
`std::vector<TGAnimation*>` of secondary overlay animations, an optional
`TPictureIO*` and `TGText*`, a fade-alpha system (`_alpha`/`_alphaFrom`/
`_alphaTarget`/`_alphaDurationMs` driven by an embedded `TTimer`), and an
active flag plus a lifetime counter. The class is genuinely polymorphic
through **two** separate vtables (the constructor writes a second vtable
pointer at +0x10, immediately after the primary one at +0x00, with the
`TVisObjRef` data member sandwiched between the end of the primary vtable
and this second one) - almost certainly two small mixin interfaces the
original multiply inherits from: one `AnimationStopped()`/`GetOwnerId()`/
`GetOwnerName()` are reached through via a `_ZThn72_`-style this-adjusting
thunk from `TCursorControl` (consistent with a primary-base interface),
and a second, informally named `TTextOwner` (after `TGText::SetOwner()`'s
parameter) that `TextFinished()` is reached through via a smaller, outer-
class-independent `_ZThn16_` adjustment (consistent with TManagedObject's
own secondary mixin). Modeled here as one ordinary class with virtual
methods instead of replicating the two-vtable ABI trick, since nothing in
this codebase casts a `TManagedObject*` through a narrower
`TAnimationOwner*`/`TTextOwner*` pointer obtained some other way.

A real, independently-discovered bug in the EXISTING `TGCharacter` model:
its own `_ref` field (previously modeled as TGCharacter's own, "at a known
offset") is almost certainly `TManagedObject`'s own inherited `_objRef` at
that exact same offset - TGCharacter has no other bases ahead of it. Fixed
`TGCharacter::GetRef()` to delegate to the inherited field (now `protected`
on `TManagedObject`) instead of keeping a second, redundant, never-actually-
synchronized one.

Several genuinely non-obvious behaviors, each independently confirmed
rather than assumed:
- `Set()`/the copy constructor pattern seen elsewhere does NOT apply here,
  but a different quirk does: `SetAnimation()` routes an animation to
  either the single "primary" slot or the secondary list based on whether
  its data object matches a specific link (field id 0xAB) on this object's
  own `_objRef` - not based on anything about the animation's own identity.
- `AnimationStopped()` clears the primary slot (if it matches) AND removes
  from the secondary list (if present) - uniformly, regardless of which
  one actually held the stopped animation.
- `RemoveAnimations()` hides the primary animation with `this` as owner,
  but hides every secondary animation with a NULL owner - a confirmed
  asymmetry, not an oversight.
- `Prepare()`'s real disassembly re-derives the same "is the primary
  animation a bones animation" outcome through three different branches
  (checking `IsSpriteIndexValid()`/`IsModelAnimation()` first in two of
  them) that all funnel into one final `IsBonesAnimation()`-based decision -
  simplified here to that one check directly, since the intermediate
  queries are pure and don't change the outcome.
- `SetDestAlpha()`'s `duration == 0` branch computes a full elapsed-time
  interpolation that - since elapsed time from a freshly-reset timer is
  never negative - always resolves to "snap straight to the target",
  simplified accordingly; `UpdateAlpha()` (the real per-frame fade step,
  called from `Draw()` every frame) keeps the genuine lerp.
- `IsInside(pt, TGDetectInfo)` (2-arg) is implemented by tail-calling back
  into the SAME object's own `IsInside(pt)` (1-arg) virtual slot - the base
  simply ignores the detect-info argument; reproduced as a direct call to
  the 1-arg virtual so a subclass override of either still takes effect
  through normal dispatch.

New, deliberately shallow leaf dependencies added to support this (real
`TManagedObject` logic calling into them, but their OWN bodies are
confirmed-call-shape-only stubs for a future pass): `TCAnimation` (a
genuinely distinct recovered class name that `TGAnimation` derives from),
`TGAnimation::HideAnimation()`/`Prepare()`/`Draw()`/`DrawMixed()`,
`TGText::SetOwner()`, `TGObjectManager::SaveEventInfo()`, a real
`wxRect::Contains(wxPoint)`, and `TVisObjRef::GetNameWithParents()`.
`TPolygonList` (modeled as a plain `std::vector<wxPoint>`) and three small
free functions (`CreatePolygonsFromPointList`/`GetBoundingBox`/
`IsPointInsidePolygon`) back the hit-test polygon - the first is a
confirmed-shape stub (always succeeds), but `GetBoundingBox()` and
`IsPointInsidePolygon()` (a standard even-odd ray-casting test) are
implemented for real, since both have well-defined, unambiguous semantics
regardless of how the original's own multi-polygon representation worked.

**What's left as an honest, flagged gap:** `ExecuteMatchingAction()`,
`HandlePostExecution()`, `GetActionsToTest()`, and `ExecuteEvent()` are
Visionaire's actual condition/action-matching engine behind scripted
events. Each branches extensively on opaque byte-offset boolean flags
inside not-yet-modeled `TGEventInfo`/`TGActionInfo` structs and a 26-value
`TypeActionExecution` switch (confirmed to exist via its jump table, but
none of its values are named). Transcribing these byte-for-byte without
knowing what the flags mean would just be moving bytes around under
invented names, violating this project's "honest gap over confidently
wrong transcription" rule - so all four stay confirmed-signature stubs.
This is a large, mostly self-contained subsystem (confirmed call sites
show `TGItem`/`TGObject` also override at least `HandlePostExecution()`/
`AnimationStopped()`, so other subclasses participate in it too) that
needs its own dedicated future pass, starting from naming `TGEventInfo`'s
and `TGActionInfo`'s own fields.

## TGObjectManager, and two TManagedObject corrections found along the way

Implemented 22 of `TGObjectManager`'s 25 manifest-listed methods for real
(the manager that tracks which `TManagedObject` the mouse is hovering -
`_currentObject` - and a separately-saved one used once a character finishes
walking up to something clicked out of reach - `_savedObject`); the other 3
(`HandleEvent()`, `ObjectReached(TManagedObject*)`, and `GetActionText()`)
are confirmed-signature stubs, for two different reasons explained below.
This class was one of three (`TCursorControl`, `TSceneControl` are the
others) explicitly deferred at the end of the `TManagedObject` pass as
needing their own dedicated look once `TManagedObject` itself was done.

**The action-execution gap shows up here too.** `HandleEvent()` and
`ObjectReached(TManagedObject*)` each build a `TGEventInfo` from the game's
own saved click/hover state and dispatch it virtually through
`TManagedObject::ExecuteEvent()` (vtable slot `0x30`) - the exact same
unreversed action-execution subsystem flagged on `TManagedObject` itself.
`MouseMove()` and `ExecuteSavedObject()` each fire that same dispatch as
*one step* among several; for those two, the dispatch step is called out
with a comment and skipped, while the rest of their real bookkeeping
(updating `_currentObject`/`_savedObject` and the matching game-data links)
is implemented in full.

**`GetActionText()`** (asm lines 189563-190177 - by itself longer than every
other method in this class combined) is a stub for a different reason: when
no hook name is registered, it falls back to formatting `"<button's
language name> <object's display text>"`, where "object's display text"
comes from an unidentified `TManagedObject` virtual at vtable slot `0xB0`
(itself ambiguous - see below); when a hook name *is* registered, it calls
`LuaExecuteFunction()` directly, this project's standing, deliberately-
unreversed Lua-bridge-contract gap. Neither path is worth transcribing
without first resolving what it depends on.

**Two errors in `TManagedObject`'s original pass, found while cross-
checking its real vtable layout** (needed here to confirm which slot
`ExecuteEvent()` lives at, since `TGObjectManager` dispatches through it by
raw `this`-pointer vtable call, not a named method call):
- A genuinely missing method, `GetDirection()` (vtable slot `0xE8`, called
  from `Draw()` for the primary animation's per-frame override) - the
  original pass's own `GetAnimationFrameOverride()` name for this was an
  invented placeholder for an "unnamed hook slot" that turned out to have a
  real, recovered name and symbol all along. Fixed - same confirmed `-1`
  default, correct name.
- An outright invented method, `OnAlphaChanged()` - the original pass
  guessed a separate protected hook lived at vtable slot `0xE0` (called from
  `SetDestAlpha()`/`UpdateAlpha()` whenever alpha actually changes), but
  that slot is just `SetAlpha()` itself - a real, public, already-modeled
  virtual (confirmed no-op in the base). Fixed by deleting `OnAlphaChanged()`
  and calling `SetAlpha()` directly; this matters for any future subclass
  that overrides `SetAlpha()` expecting it to fire on alpha changes, which
  the old, fabricated hook would never have reached.

The same cross-check also *confirmed* (rather than corrected) the class's
two secondary mixin interfaces by name, straight from their own vtables/RTTI
sitting right after `TManagedObject`'s own in the binary: `TAnimationOwner`
(3 pure virtuals) and `TTextOwner` (1 pure virtual, `TextFinished()`) - both
already guessed informally in the original pass, now hard evidence rather
than a guess. One vtable slot (`0xB0`, between `RemoveSprites()` and
`SetAnimation()`) remains unidentified: IDA resolves it to
`no_check_checker_t::to_string()`, almost certainly identical-code-folding
with an unrelated trivial method rather than that method's real name/owner.

**New shallow leaf dependencies**, matching this project's usual "just
enough surface to compile and behave correctly" treatment for a class
that isn't getting its own full pass yet:
- `TGDetectInfo` (new, `TGDetectInfo.h`): two flag bytes plus a character
  link, named by position (`flagA`/`flagB`) rather than guessed meaning,
  like `TypeOrder`/`eVisionaireTable` - only 3 of their 4 possible
  combinations are ever written by `GetDetectInfo()`.
- `TTButton` (new, `TTButton.h`): confirmed a zero-overhead `TVisObjRef`
  subclass (every ctor seen is a tail call straight into the matching
  `TVisObjRef` ctor) - modeled as `: public TVisObjRef` directly rather than
  introducing an unneeded `TTObject` base. Only `IsStandardCommand()` is
  implemented (confirmed in full); the manifest's other 11 methods aren't
  touched.
- `TCursorControl::SetMoveObject()`/`ReleaseMoveObject()`: confirmed call
  shapes only - `TCursorControl` itself hasn't had a dedicated pass yet.
- `TVisObjRef::GetVisionaire()`: confirmed call shape only - the real body
  reads a `TVisionaireObject*` field this project's `TVisObjRef` stub
  doesn't model.
- `TVisionaireGame::GetGame()`/`GetEmptyObject()`: both repeatedly confirmed
  called directly on whatever `TGameControl::GetGameSystem()`/
  `GetVisionaire()` returns, with the exact same call shape `TVisionaire`'s
  own methods of the same name already use everywhere else - another data
  point for the standing "`TVisionaire`/`TVisionaireGame` may really be the
  same underlying object" gap (see `visionaireGame.h`'s own comments),
  rather than genuinely new behavior.
- `TGameControl::GetObject()`'s return type corrected from `void *` to
  `TManagedObject *` - the same kind of placeholder the manifest originally
  guessed for `TGScene::GetObject()` too (already fixed, in an earlier
  pass); every internal path here already produces a `TManagedObject*`-
  compatible pointer.
- `TGCharacter::SetActionCharacter`'s own confirmed caller, `TGObjectManager
  ::SaveEventInfo()`, needed no new surface - but it, and several other
  `TGObjectManager` methods, rely on a repeated idiom confirmed across half
  a dozen call sites: "does this character's own field `0x1F7` link match
  the current scene's own `GetRef()`?" (i.e. is this character actually the
  one active in the current scene). Modeled as plain inline comparisons
  rather than a named helper, to keep each method's own confirmed shape
  visible.

`TGObjectManager` is now `in-progress` in the manifest (not `done`, given
the 3 stubs above); `TTButton` moved from `todo` to `stub`.

## TCursorControl

Implemented 22 of `TCursorControl`'s 23 manifest-listed methods for real -
the other dependent flagged alongside `TGObjectManager` at the end of the
`TManagedObject` pass. Only `GetSCursor()` stays a confirmed-call-shape
stub (see below).

**A genuine surprise: `TCursorControl` is not a `TManagedObject`.** The
`_ZThn72_` thunks to `AnimationStopped()`/`GetOwnerId()`/`GetOwnerName()`
that originally suggested a `TManagedObject`-shaped relationship actually
belong to `TCursorControl` implementing the real, named `TAnimationOwner`
interface *independently* - confirmed from its own constructor, which
never calls a `TManagedObject` sub-object ctor at all, just pokes two
vtable pointers directly (one for its own primary interface, one for
`TAnimationOwner` at `+0x48`). `TManagedObject` and `TCursorControl` are
siblings with respect to `TAnimationOwner`, not parent/child - a cursor
isn't a managed scene object, it just also owns an animation and needs to
participate in the same "who owns this, and what happens when it stops"
protocol. Added `TAnimationOwner.h` (the real interface, 3 pure virtuals)
and a parallel set of `TGAnimation::HideAnimation()`/`StartAnimation()`
overloads taking `TAnimationOwner*` instead of `TManagedObject*`, rather
than retrofitting `TManagedObject` itself onto the new interface - lower
risk, since `TManagedObject`'s own design was already shipped and nothing
needs to cast between the two owner kinds polymorphically.

**Confirmed field layout:** a `std::vector<SCursor *> _cursors` (one
heap-allocated entry per loadable cursor) plus an `_activeCursor` iterator
into it, a `wxPoint _position`, the cursor's own `TGAnimation
*_currentAnimation`, and a `TGItem *_heldItem` for whatever's being
dragged. `SCursor` itself (new, `SCursor.h`) was reconstructed field-by-
field by cross-referencing the constructor/destructor/`Clear()` (which
manually inline what amounts to `delete cursor` on each entry) against
`SetCursor()`'s own search loop and `SetActiveCursor()`/
`SetInactiveCursor()`/`ReleaseMoveObject()`'s shared active/inactive-image
toggle: two `TVisObjRef` images (`downImage`/`upImage` - which plays for
"active" vs "inactive" isn't confirmed beyond which SetXCursor() happens
to use which), a `bool active`, a `std::vector<int> linkedIds`, and an
`int id`.

**A corrected parameter order:** `LinkButtonCursor(int, int)`'s own body
(now read directly, rather than inferred from its call site) shows the
*first* argument is matched against an `SCursor`'s own `id` and the
*second* is what gets appended to that entry's `linkedIds` - the opposite
of the manifest's original "linked object's, then the object's own"
guess. Fixed in the (renamed) parameters; `LinkButtonCursor()` and
`LoadCursor()` are both implemented in full now - their own dedup/lookup
logic doesn't depend on what the still-stubbed `GetSCursor()` fills in,
since `LoadCursor()` computes its own id via the already-existing
`PackVisId()` rather than relying on `GetSCursor()` to do it.

**What's left as a confirmed-call-shape stub:** `GetSCursor()` builds a
new `SCursor`'s images and name by string-concatenating two unidentified
wide-string table suffixes onto the source object's own name, and (in one
branch) calls an unidentified `TManagedObject`-family virtual at vtable
slot `0xB0` - the exact same slot left ambiguous by identical-code-folding
during the `TManagedObject` vtable cross-check (see that section above).
`GetActionText()`-sized complexity for a single leaf helper; left for a
future pass once `0xB0`'s real identity is known.

**New shallow dependencies**, same "just enough to compile and behave
correctly" treatment as elsewhere: `TGItem` (new, `TGItem.h`) - confirmed
a `TManagedObject` subclass, but only `SetCenteredPosition()` (real) and
`GetPositionNextToItem()` (stub - its real body reaches into
`TManagedObject`'s own private animation/picture state, which isn't
exposed) are modeled; `TCAnimation::SetPosition()`/`GetCurrentSprite()`
(new stubs); `TGAnimation::UnloadAnimation()`/`PreloadAnimation()`/the new
`HideAnimation()`/`StartAnimation()` overloads (new stubs).

`TCursorControl` is now `in-progress` in the manifest; `TGItem` and
`SCursor` both moved from `todo` to `stub`.

## TSceneControl

Implemented 14 of `TSceneControl`'s 15 manifest-listed methods for real -
the last of the three classes flagged at the end of the `TManagedObject`
pass. Only `ToScene()` - the actual scene-transition implementation
(sound handling, swapping the current/old scene, registering the new
scene's events, by itself longer than every other method here combined)
stays a confirmed-signature stub; every other method is implemented in
full around it, since their own logic doesn't depend on what it actually
does internally.

**Corrects another manifest placeholder guess:** the real class holds two
heap-allocated `THScene` instances (a concrete `TGScene` subclass, by the
same `operator new` + default-ctor shape as `THCharacter`'s own
relationship to `TGCharacter`) - a *current* scene and, during a cross-
fade transition, the one being faded *out* - not a single `TGScene` by
value as the manifest's original guess had it. Modeled as plain
`TGScene *_currentScene`/`_oldScene` rather than introducing a near-empty
`THScene` subclass, since nothing here needs anything beyond `TGScene`'s
own already-confirmed interface.

**Three separate confirmed bool flags**, not one reused three ways:
`_fadingToNewScene` (set by `ToScene()`, read by `FadingToNewScene()`),
`_hasOldScene` (gates whether `Draw()` draws the old scene at all), and
`_drawingOldScene` (set only for the duration of the old scene's own
`Draw()` call, read by `GetScene()`'s const overload and
`GetLastPlayableSceneParams()` so that code running synchronously during
that one draw call still resolves "the scene" to the one actually being
drawn). `GetScene()` turned out to have two genuinely different bodies,
not a const/non-const pair over the same logic - the non-const overload
always returns the current scene outright, skipping that redirect.

New: `TGScene::SetRef()` (`Set()` was already confirmed to mutate `_ref`
directly - mirrors the already-existing `GetRef()`) and
`TGCharacter::GetOppositeDirection()` (confirmed static - no implicit
`this` - and simple enough to implement in full: wraps a 0-359 compass
value by 180).

`TSceneControl` is now `in-progress` in the manifest.

## The action-execution subsystem

Implemented `TManagedObject::HandlePostExecution()`/`ExecuteMatchingAction()`/
`GetActionsToTest()`/`ExecuteEvent()` in full - the condition/action-matching
engine behind Visionaire's scripted events, left as confirmed-signature
stubs at the end of the original `TManagedObject` pass and explicitly
flagged ever since as needing its own dedicated pass. It took one: this is
~2200 lines of disassembly across the four methods, plus follow-on work in
`TGObjectManager` (see below).

**The core insight that made this tractable**: `TGEventInfo` and
`TGActionInfo`'s own *fields* don't need resolved real-world meaning to be
transcribed faithfully - only their structural *role* in the control flow
does, and that's fully confirmed. Both are modeled with positionally-named
fields (`flag4`, `flagA`, etc.), exactly the same convention this project
already uses for `TGDetectInfo`/`TypeOrder`/`eVisionaireTable` when a
value's byte-level behavior is certain but its design intent isn't. This
is what unblocked the "honest gap over confidently wrong transcription"
concern the original stub comments raised - the gap was never really about
the *names*, it was about not yet knowing the fields existed at all.

**New real classes**: `TGEventInfo` (`TGEventInfo.h`) - two TVisObjRef-
shaped slots (`action`, `command`; the first built via a `TTObject` ctor,
the second via `TTButton`'s, at every confirmed call site), a bool, an
int (`mouseEvent`), and a `TGCharacter*`. `TGActionInfo` (`TGActionInfo.h`)
- an int (`matchedType`) plus 8 bools. `TypeActionExecution`
(`TGActionInfo.h`) - confirmed individually-significant values 3-28 (26
cases, `ExecuteMatchingAction()`'s own switch computes `value - 3` and
jump-tables up to 25) plus 0-2 and 34-36 (confirmed via
`GetActionsToTest()`/`IsImmediateExecutionType()`), all named by raw value
per the convention above. `TTAction` (`TTAction.h`, new) -
`IsImmediateExecutionType()` only, confirmed in full: true for exactly 16
literal values.

**Two more TManagedObject fields turned up** while tracing these methods'
own use of `this` (distinct from the 4 found during the TGObjectManager/
TCursorControl/TSceneControl passes): `_bypassReachCheck` (make
`GetActionsToTest()` treat this object as always-reached, and skip the
auto-facing-angle update in `ExecuteEvent()`/`ExecuteMatchingAction()`) and
`_skipFinalPostExecution`/`_hasActionTypeFallback` (gate `ExecuteEvent()`'s
own trailing `HandlePostExecution()` call and a hardcoded candidate-type-
expansion retry respectively). All three are named for their observed
effect, not recovered.

**`GetActionsToTest()`** is a pure `info.mouseEvent`-keyed dispatcher:
depending on the event type (1-9) and a couple of game-data int fields
(0x30D/0x30E, read from this object's own game-data reference), it fills a
candidate list of `TypeActionExecution` values and sets a handful of
`TGActionInfo` flags describing *how* reached/matched this attempt already
looks, before any actual action has been considered.

**`ExecuteMatchingAction()`** then walks the candidate action list,
computing for each one a TVisObjRef-based "required command" and comparing
it against `info.command`/`info.action` and the candidate's own "linked
items" list (field 0x248); the winning combination's own switch (keyed by
each candidate `TypeActionExecution` value) decides whether a match
counts as "reached" (`flag4`) or an "any-object fallback" (`flag5`), and -
only when a character is involved and a confirmed set of 7 specific types
and a game flag (0x246) line up - computes a facing angle via a `GetAngle()`
free function (confirmed call shape only; the real callee's own math isn't
reversed) and calls `TGCharacter::StopWalking()`.

**`ExecuteEvent()`** ties it together: `HandlePreExecution()`, then
`GetActionsToTest()`, an `AlignCharacter()` step when reached, then
`ExecuteMatchingAction()` against this object's own action list - with a
mouse-event-3/4 retry (forcing `TGEventInfo::flag8` true) and, if still
unmatched and `_hasActionTypeFallback` is set, a second retry against a
hardcoded expansion of the original candidate types (3→{27,25}, 20→
{28,26}, 11→{23,21}, 19→{24,22}) and a different action list (field
0xAC). One dead computation was found and deliberately dropped: the
original snapshots the game's "current action" link (0x262) before and
after the main attempt and stores whether it changed, but that value is
never read again anywhere in the function - omitted here as having no
observable effect, not as an oversight.

**`HandlePostExecution()`** decides, once an action attempt is over,
whether to call `TGCharacter::ShowComment()`/`ReceiveItem()` (the "nothing
matched, say something about it" path) or to tag the matched command's
parent object (field 0x25F ← 0x12A) and tell `TGObjectManager::RemoveItem()`
to drop whatever's held - gated by a small bitmask check on the game's own
field 0xF4 whose exact bit meanings aren't resolved beyond "bit 1 is
ignored when `flag4` is set."

**Follow-on in `TGObjectManager`**: now that `TGEventInfo` is real,
`HandleEvent()`/`ObjectReached(TManagedObject*)`/the dispatching steps of
`MouseMove()`/`ExecuteSavedObject()` - all four left as documented no-ops
during the `TGObjectManager` pass pending exactly this - are implemented
in full too. This also pinned down which of `TGEventInfo`'s two TVisObjRef
slots comes from which game-data link: `action` ← field 0x2AE ("reached
object"), `command` ← field 0x262 ("current action"), the *opposite* of
this pass's own first guess (corrected before anything else depended on
it).

`TManagedObject` and `TGEventInfo` are now `done` in the manifest;
`TTAction` moved from `todo` to `stub` (only the one method above).

## TMSavegameArea

A small, clean win while scouting for the next class to tackle: most of
the remaining small `TG*`/`TT*` classes turned out to be UI buttons
(`TGScrollButton`, `TGCommand`, `TGPlaceHolder`, ...) that derive from
`THButton`, which itself pulls in an event-handler-interface bridge
(`TVisObjRef::RegisterEventHandler()`, a `TEventHandlerInterface`, a
`_ZThn648_`-style thunk) - the same kind of standing, deliberately-
unreversed gap as this project's Lua bridge, just a different one. Rather
than start down that hole, found `TMSavegameArea` instead: a
`TManagedObject` directly, no intermediate class, confirmed in full (all
6 manifest-listed methods) - a clickable rectangle over one savegame
slot, with `GetActionList()`/`ExecuteEvent()` both no-ops (it doesn't
participate in the action-execution engine at all) and `IsInside()` doing
a plain rect test instead of the base's polygon one.

Its constructor independently *confirms* two of the three opaque
`TManagedObject` bool fields found during the action-execution-subsystem
pass - it sets `_bypassReachCheck`/`_skipFinalPostExecution` both true
(sensible: a UI slot needs no walking/reach check, and its own
`ExecuteEvent()` is a no-op anyway) - good cross-validation from a
completely independent call site. Both fields moved from `private` to
`protected` on `TManagedObject` so this subclass (and any other direct
subclass that needs them) can set them directly, matching the real
binary's own constructor-time field pokes.

One honest gap of its own: `IsInside()` is gated by a bool (`_active`)
that's never written anywhere in this class's own 6 methods, including
the constructor - presumably set by `TGScene`'s own savegame-slot-picker
methods once an area is actually shown, but no confirmed call site for
that turned up. Defaults to `false` rather than guessing a setter into
existence. Added a matching confirmed-call-shape-only `TGScene::SetScene()`
(the one method that constructs these areas, ~740 bytes, not itself
reversed) so the new class has a real, if still unimplemented, caller
rather than sitting unused.

`TMSavegameArea` is `done` in the manifest.

## TGScene

Implemented all 39 manifest-listed methods (Deponia_Linux.asm lines
166435-173218), with the three exceptions below. TGScene is the concrete
scene drawable behind `TSceneControl::GetScene()`: a TPaintControl that owns
a scene's background and lightmap pictures, its objects and characters, the
depth-sorted draw order built from both, a savegame-slot picker (used by the
"load game" menu scene), a particle effect, and the fading "snoop animation"
overlay. The class's real byte layout is in `TGScene.h`'s own header comment.

**Deliberate gaps** (the same two standing gaps as the rest of the project,
nothing new): the Lua bridge (`GetTint()`'s optional "lightmap callback"
override, `BeginScene()`'s `particleSystem:new(...)` script branch - a scene
with a non-empty field 0x326 gets no particle container here) and the
unmodeled `graphics` backend's virtual slots (`Draw()`'s particle-drawing
translate/matrix/draw calls; `InitialiseBackground()`'s one bool-argument
call when leaving a menu). Everything else around those calls - including
ParticleContainer's own `Update()` and the particle system's fixed 25ms-
step `IncTime()` catch-up loop - is implemented.

**Corrections to earlier passes found along the way:**

- **`TMSavegameArea`'s `_rect`/`_active` were not its own fields.** Its
  constructor stores the rect at +0x28 (TManagedObject's own
  `_boundingRect`, exactly what `TManagedObject::GetBoundingRect()` reads -
  and `TGScene::Prepare()/Draw()` call that very method on each area to
  place its slot's screenshot), and its `IsInside()` tests the +0x38 flag,
  which is TManagedObject's own `_active` (default true; `SetActive()`
  flips it). The earlier "never written anywhere, defaults to false" gap
  was an artefact of modeling those as separate private fields. Both fields
  moved from private to protected on `TManagedObject`; `TMSavegameArea`
  now just sets the base's `_boundingRect`.
- **`TManagedObject::_hasActionTypeFallback` defaults to TRUE**, not false:
  both `TManagedObject` constructors (asm lines 192469/192540) write 1 to
  +0x51, and `TMSavegameArea`'s constructor explicitly writes 0 over it
  (which is what made that class's own "explicitly left false" look
  meaningful). `ExecuteEvent()`'s fallback retry therefore runs for every
  plain `TManagedObject` unless a subclass opts out.
- **`TManagedObject::Prepare()`/`RemoveSprites()` are virtual** (vtable slots
  0x88/0xA8 - `TGScene::Prepare()` calls them through the vtable on
  `TMSavegame`/scene objects).
- **`TPictureIO`'s +0xD0 field is the shader id**, not a LoadRect field -
  `TGScene::Draw()` writes the resolved shader there immediately before
  `Draw()`. Renamed `_shader`, with a `SetShader()` accessor.
- **`TGScene::GetCharacters()` returns the member vector's address** (a
  reference), not a copy. **`GetActionAreas()` and `ClearActionAreas()` are
  static** - the manifest's `this`-taking signature for the former was a
  decompiler artefact (`rdi` is the TVisObjRef argument, not `this`).
- **`TMSavegame` is a TManagedObject subclass** (recovered RTTI, 0x1E8 bytes),
  and its constructor's 3rd/4th ints are not "always 0" as the earlier
  header guessed - `TGScene` passes the savegame slot rectangle's width/
  height (recorded from the first click area in `SetScene()`).

**Behavioral details worth knowing:**

- **`defaultShader` quirk in `Draw()`**: the "no shader (id -1) -> use the
  default" substitution only fires when the global `defaultShader` is
  non-zero (`test eax,eax / cmovnz` - a zero default leaves the id -1).
- **Savegame active-range bound**: `ScrollSavegames()`, `SetActiveSavegames()`
  and `SetSavegames()` all flag a slot active when
  `first <= index < savegameCount + first` - an upper bound that is *not*
  clamped to the click area count, so scrolled-past-the-end slots stay
  "active" (all three call sites use the identical expression).
- **`SortAllObjects()`'s merge is back-to-front**: every character plus
  every currently *moving* scene object is sorted by `GetCenter()`, then
  merged with the stationary scene objects (which stay in their own order);
  on a tie the moving/character side goes first. Each entry's draw-order
  index is recorded in a hash map keyed by its data record's 4-byte id -
  that's what `GetObject(TVisObjRef)` looks up. The original's hash keys
  hash all four id bytes, unlike `PackVisId()`'s 3; modeled as an
  `unordered_map` keyed by all four bytes.
- **`InitActionAreas()`'s registry** is a process-wide `HashMap<int,
  std::list<TSceneActionArea*>*>` keyed by the scene's `PackVisId()` (the
  recovered type name `HashMap<int,std::list<...>*,IntegerHash<int>>` shows
  up in a destructor symbol). `ClearActionAreas()` in the original frees
  the lists' contents but leaves the emptied hash nodes (and their now-
  dangling list pointers) in place; the registry is cleared here instead.
- **`BeforeFade()`** just calls vtable slot 0 (`Prepare()`); the one
  `GetInt(0x223)` read in front of it has its result discarded.
- **`SetCharacters()`'s lifetime protocol**: a character that leaves the
  scene has its walking sound stopped; one that was *in* the scene (or
  whose lifetime counter has just run out) is unloaded - sprites removed,
  animations unloaded - unless the game's "keep loaded" flag (0x282) is
  clear on a menu scene, in which case a leaving character with time left is
  skipped entirely.

New confirmed-call-shape-only stubs this pass: `THObject` (the scene's
object class, 0x340 bytes), `TSceneActionArea` (0x38 bytes),
`TParticleSystem`/`TGParticleSystem`/`ParticleContainer`
(`TGParticleSystem.h`), the `TGAction`/`TGAnimation`/`TGText` pause-and-
resume statics (`StopRunningActions`/`ContinueStoppedActions` etc.),
`TGAction::Execute()` (the engine's whole scripted-action interpreter - tens
of thousands of lines, not reversed), eight `TGCharacter` per-scene
bookkeeping methods, `TPaintControl::SetWorktopArea()`,
`TPictureMEM::GetPixel()/SetMemoryBlock()`, the `defaultShader`/
`ShaderCallback()` globals, `TVisObjRef::GetSprite()/GetRects()/
GetStrHolder()` and the `g_loadingState` global (a recovered symbol - a
one-letter "what's the engine doing" tag, "P" during `TGScene::Prepare()`,
"L" otherwise).

`TGScene` is `done` in the manifest.

## TMSavegame

Implemented all 22 manifest-listed methods (Deponia_Linux.asm lines
159356-164367). One savegame slot (or "bookmark" slot) as shown in the
"load game" menu: a `TManagedObject` subclass (recovered RTTI, 0x1E8 bytes)
that owns the slot's screenshot as an embedded `TPictureIO`, loads the
slot's title/font from the savegame itself, and can write the slot to disk.
`TGScene` creates one per slot found on disk and activates the ones
scrolled into view. The real layout is in `TMSavegame.h`'s header comment.

Savegame file naming: `<savegame dir>/savegame<NN>.dat` (or `bookmark<NN>.dat`;
the slot number is zero-padded to two digits - the original checks `slot <= 9`
and prepends "0"). Inside each composed file the screenshot lives at
`vtp_savepic<slot>.webp#g#-01#00001#` and the title/game record at
`vtp_savedata<slot>.xml#g#-01#00000#` (the `#g#...#` suffix is `TSprite`'s own
embedded-settings path syntax). The composed file's password is
`SAVEGAMEPWD30`; if that doesn't open, loading retries with none (the
recovered global `passwd`, an empty string - `AppGlobals.h`). The savegame
directory is looked up once through `TStandardPaths::GetSavegamePath()`.
Dates in `MakeSaveGameName()` read "name D.M.YYYY, H:MMh" (with the colon
followed by an extra "0" for minutes <= 9).

**Corrections to the earlier header / manifest:**

- The constructor's first bool is "is bookmark", not "numbered slot"
  (`GetFileName()` picks "/bookmark" vs "/savegame" from it); its 3rd/4th
  ints are the slot rectangle's width/height (what `TGScene::SetScene()`
  records from the first click area), not "always 0". A bookmark's
  `SetActive()` is a no-op - it never activates.
- `MakeSaveGameName` is static and takes the scene (the manifest's `this`-
  based listing hid that - the "this" slot is the hidden return buffer);
  `InitSaveGamePath()` is static too.
- Two fields at +0x1C0/+0x1C4 are constructed together via `TId(-1, -1)`
  but used independently - +0x1C0 ends up holding the first four bytes of
  link 0x1D5's id (a font id), +0x1C4 the x offset that centres the title
  across the slot (`slotWidth/2 - titleWidth/2`). Modeled as two ints.

**Deliberate gaps:** the write path is complete against the *call shapes* of
`TComposedFile` (`InitForWrite`/`AddData`/`AddFile`/`WriteToDisk`),
`TTempFile` and the writer's two virtual accessors (recovered from
`TBufferedProjectFileWriter`'s own 3-pure-virtual vtable, named for their
observed role - the buffer and the name it's stored under), all of which are
still unreversed stubs - so `SaveGame()` currently reports success without
writing anything. `AddFile()`'s real 4th parameter (an empty
`StringHashMap<wxString,wxString,...>`) isn't modeled. `SaveSnapShot()`
needs the unmodeled GL backend's "captured frame" virtual slot (+0x128,
named `GetCapturedFrame()` for its role).

New supporting stubs/fixes: `TTScene` (a TVisObjRef-derived scene handle,
`TTScene.h`), `wxDir::Open/GetFirst/GetNext` (real, `std::filesystem`-
backed, with wildcard matching), `wxRemoveFile`, `wxDateTime::GetTmNow`,
`wxString::ToLong`, `wxPoint::operator+`, `TGraphicsInterface::RemoveFromCache/
GetMainMemBlock/GetCapturedFrame`, `TPictureMEM::ResizeImage`,
`TTempFile::AddTempFile`, `TSteamSDK::DeleteCloudSavegame`,
`TVisionaireGame::LoadSaveGame`.

`TMSavegame` is `done` in the manifest.

## TPaintControl

Implemented all 27 manifest-listed methods (Deponia_Linux.asm lines
143284-749311 - the two virtuals sit far from the rest, as COMDAT-folded
empty bodies). The real layout is 0x44 bytes (in `TPaintControl.h`'s header
comment); the earlier model was wrong in three ways: **`_active` defaults to
TRUE** (the constructor writes 1 to +0x08), there is a **worktop area rect**
(+0x2C, distinct from the worktop size) that the scroll position is clamped
into, and **`SetCurrent()`/`GetCurrent()` use a real static**,
`TPictureIO::s_pPaintControl` (recovered symbol; the destructor clears it if
it still points here).

Behavior worth knowing: `AdjustWindowHorizontal/Vertical(float)` take an
*absolute* scroll coordinate despite the name (TGScene::InitialiseBackground
passes a saved scroll position), clamped so the visible window stays inside
the worktop area; `SetWorktopArea()` tests every edge against the caller's
rect but writes the corrections to a copy (a quirk reproduced as-is);
`SetScrollPos(wxPoint)` and `(wxRealPoint)` always keep the int and float
scroll pairs in sync; `GetRelativePoint()` is `(pos + scroll) - origin`.
Adds `wxRealPoint` (a float pair) and `wxPoint::operator-`.

`TPaintControl` is `done` in the manifest.

## TTypeData and TTypeGroup (the data schema)

Implemented in full: `TTypeData` (19 methods, `datastruct/type.h`, original
path confirmed `src/datastruct/type.cpp`) and `TTypeGroup` (30 methods,
`datastruct/typegrp.h`, confirmed `src/datastruct/typegrp.cpp`). These are
the backbone of the game-data schema: a `TTypeGroup` is one *record type* -
the ordered list of `TTypeData` *fields* (field id, value kind, link table,
file-format version ranges, storage offset) - and every `TT*` data-record
class (`TTScene`, `TTEvent`, `TTScript`, ...) owns a static one, built in its
static initializer (`TTypeGroup(int, eVisionaireTable, int, int)` - e.g.
`TTEvent`'s is `(0x26, 0x340, 0xB8, -1)`) and filled by its `InitType()`,
which also installs its `OnCreate`/`OnInit` default-value callbacks
(`SetCreate`/`SetInit`). Layouts are in the two headers' comments.

Field lookup goes through the recovered global `g_lookupTable` (modeled as
`TLookupTable{pEntries, numEntries}` after the original's own assert text;
the real binary keeps the pointer and count as two adjacent globals): one
12-byte entry per field id naming the group that currently owns it and its
index in that group's list. `GetTypeData()`/`GetTypeDataPtr()`/`GetType()`
verify the entry belongs to *this* group, which is how a field id shared
between groups resolves correctly.

**`AddType()`'s version logic** (the interesting part, asm lines 585963-
586100): a field's effective range is its savegame range for a savegame-only
field, or the game range widened to the savegame range for a "both" field.
Against the range the group is currently set up for (`ClearTypes(low,
high)`), a field is added if it spans the low end (starts at or before it
and is in force past it, or open-ended) or, failing that, spans the high end
(starts at or before it and ends past it, or open-ended). It's a *temporary*
field (`IsTempType`, stored only in memory) if it ends before the high end,
and *stored in the file* (`IsInFile`) if it spans the low end. Persistent
fields take storage offsets from `_sizeData`, temporary ones from
`_sizeTempData`.

**Other details:** `SetScrambled(seed)` shuffles field order once (each
field's id + seed picks a partner slot in `1..n-1`; swap if the partner is
later), keeping `g_lookupTable` in step - used to hide the on-disk layout.
`SetupNeededTypes(forSaveGame, linkTypesOnly, skipTemp)` rebuilds the
"needed" list (stored fields fitting the request; link-only keeps types
0x10/0x11). Copying a group (`TTypeGroup(const&)`, `operator=`) is an
intentional `x_assert(false)` trap. `GetTypeData()`'s "not found" log message
is literally the one-character string "T" in the binary (the five arguments
it's passed are never formatted). `eSaveGame` gained its other two values
(0 = game data only, 1 = savegame only, 2 = both).

New confirmed-call-shape stubs: `TData::GetDataSize(eTypeData)`
(`datastruct/data.h`; returns 0, so storage offsets are all 0 until it's
reversed) and `TVedFile::IsSaveGame()/GetVersionOk()` (`datastruct/
vedfile.h`; TVedFile itself is a separate, unreversed class).
`TTypeData`/`TTypeGroup` are `done` in the manifest.

## TFile and TVedFile

`TFile` (37 methods, `baselib/file.h`) and `TVedFile` (10 methods,
`datastruct/vedfile.h`) implemented in full (Deponia_Linux.asm lines
522315-525756), replacing the earlier "FILE* plus a path" stand-in whose
layout bore little relation to the real class. A `TFile` is a `wxFile` plus a
*window* onto it - a start offset, a length (-1 = unbounded) and a position
relative to the window - so one class reads a whole file *and* an entry packed
inside a composed container (`OpenReadFromComposedFile(file, name, offset,
length)` just opens the container and sets the window); every read is clamped
to it. The open mode (0 closed / 1 read / 2 write) at +0x34, and a flag at
+0x30 (0 for a plain read, 1 for a composed entry *or* a file opened for
writing), content flags at +0x38, and the entry name at +0x60 round out the
layout (in the header comment).

Details worth knowing: the 2- and 4-byte reads are big-endian (and the signed
`ReadLong` sign-extends its 32-bit value into the 64-bit `long`);
`ReadToBuf(buf, 0)` (or any size past the window) reads the whole window,
512 KiB at a time, and asserts the length is known; `Seek()` returns false when
closed and handles `wxFromStart/Current/End` against the window, treating any
other mode as "stay put"; `OpenWrite()` makes the path absolute and leaves the
window unbounded; `WriteToFile()` opens its destination *read-write* (so the
file must already exist - consistent with `DecryptHeader()`'s use of the same
mode). Content-flag bits: 1 compressed, 2 encrypted, 4 encrypted header (only
the first 0x12C bytes - a composed file's header), 8 PNG header encrypted,
0x10 compressed chunks (the original names bit 4
`CONTENTFLAG_ENCRYPTED_HEADER` in an assert string).

`PasteFile()`/`PasteData()` are the composed-file writer's appenders. Chunked
mode writes 2 MiB chunks, each compressed (and encrypted if asked) and preceded
by an (uncompressed length, stored length) pair, and *always* ends with a
(0xFFEEFFEE, 0) footer - even if a chunk failed. Whole-file compressed mode
oddly writes nothing when the content isn't encrypted (the original returns
success without touching the destination - presumably its caller writes the
buffer through `PasteData()`); the plain mode optionally encrypts everything
or just the first 0x12C bytes, and never sets its length out-parameter.

`TVedFile` is a `TFile` plus a binary flag ("VBIN" magic, `CheckBinary()`), the
read- and write-side format versions (`SetVersion`; `GetVersionOk(in, out)`
tests the write one for a file open for writing, the read one otherwise,
and never for a mode above 2), the savegame flag and a reference path. That
resolves the stub `TTypeGroup::AppliesToFileVersion()` was using.
`wxFile` gained `Eof()` and a stdio-mode-string `Open()`; `wxFileName` gained
`MakeAbsolute()`. Checked with a standalone test (window reads, big-endian
values, `VBIN` detection, version ranges).

`TFile` and `TVedFile` are `done` in the manifest.

## TComposedFile (the container format, read and write), TTempFile, BuildProgressEvent

All 20 `TComposedFile` methods now implemented (Deponia_Linux.asm lines
541459-548209), plus `TTempFile` (2), `BuildProgressEvent` (3). The earlier pass
stopped short of the directory's record layout - the one thing a shipped game
needs - and had a *wrong endianness*. Verified end-to-end with a standalone
round-trip test (write a container with an in-memory entry and a file entry,
read it back through `Init`/`Open`, wrong password rejected).

**On-disk format** (full layout in `composedfile.h`): "VIS3" + 4-byte entry
count, then `count*16+6` bytes encrypted with the MD5-keystream cipher; the
decrypted block is "HDR", `count` 16-byte records `{offset, stored size,
uncompressed length, content flags}`, "END" - **all values big-endian 32-bit**
(`ByteStreamToLong`/`ULong`, which are the same function and sign-extend into
64 bits). Entry offsets are relative to the end of the directory; `InitEntries()`
adds that end position. Entries carry **no names** - they're addressed by index
through the `<ext>#<type letter>#<volume>#<index>#` reference syntax that
sprite paths embed (`vtp_savepic3.webp#g#-01#00001#` = the 1st entry of an
unnumbered type-4 container; the letter is `scmgio`[type-1], `v` otherwise,
the numbers `%03ld`/`%05ld`).

**Corrections:** `TMemoryBuffer::operator<<(short/long/...)` write **big-endian**
(2 bytes for short, 4 for long - low 32 bits only; an earlier version wrote
native little-endian `sizeof(value)` bytes). `GetComposedFileExtension()` now has
the real strings and the "type 4/7/8 -> empty" cases (the earlier version
asserted on them); `InitForWrite(.., wxFileName, ..)` stores its file in the
*composed path* field, not the exe field; `SEntryInfo`'s unknown fields are
`memoryData` (+0x08, a TMemoryBuffer* for entries added from memory),
`uncompressedLength` (+0x20) and `contentFlags` (+0x28). `TFile`'s +0x60 string,
which `OpenReadFromComposedFile()` stores, is the container's *key*, so a file
can decrypt itself later.

**Writing** (`WriteToDisk`): optional embedded exe (copied verbatim first, its
length recorded for a 7-byte `(length, "COM")` footer), "VIS3"+count header,
a placeholder the size of the directory, each entry's data via
`TFile::PasteData()`/`PasteFile()` per its content flags, then the real
directory overwrites the placeholder. A failed entry logs and is recorded with
zero sizes rather than aborting. A PNG flagged "header encrypted" (flag 8)
has its IHDR width/height (file offsets 0x10/0x14) overwritten with the
encrypted dimensions, keyed by its file name. The optional `wxFile*` log gets
a terse trace; the optional `EventHandler*` a `BuildProgressEvent` (type 0x1D,
0x88 bytes) per entry. `AddFile()` keeps a path -> reference cache
(`StringHashMap`, modeled with `std::unordered_map`) so a file added twice is
stored once; the two embedded-exe container types (7/8) use different
reference formats (`<ext>#<vol>#<index %06d>#` / `<ext>#<vol>#m00000#`).

**`TTempFile`**: `AddTempFile(a, b)` is `tempDir + a + b` (nothing between),
recorded in a static list for `DeleteTempFiles()`. The original concatenates
`wxStandardPaths::GetTempDir()` directly, which has no trailing separator;
a separator is added here when missing (deviation noted in the code).

**zlib:** `TMemoryBuffer::Compress()/Uncompress()` (3 methods, now implemented -
thin wrappers over zlib's `compress()`/`uncompress()`, with the standard
`len*1.001+12` output size) call a stand-in in `zlibShim.h/.cpp` because zlib
isn't available in this build: a complete inflate (stored/fixed/dynamic
Huffman, after "puff" - verified against 28 real zlib streams at levels 0/1/6/9)
plus a "deflate" that writes valid but *stored* blocks (correct, not smaller).
Swap in the real library when it's linked. One quirk reproduced as-is:
`Uncompress(TMemoryBuffer &dest, long)` never updates `dest`'s length.
`TMSavegame::SaveGame()` is a working path for bookmarks; a real savegame's
screenshot still depends on the unmodeled GL backend.

`TComposedFile`, `TTempFile` and `BuildProgressEvent` are `done`.

## The game-data schema, recovered mechanically (field ids, names, record types)

Every property of a game-data record is addressed by a numeric *field id*
(`GetInt(0x124)`, `SetLink(0x1F7, ...)`). The binary names all 840 of them, and
describes every record type's fields, in code that is regular enough to read off
with scripts (all in `tools/`, outputs in `manifest/`):

- `tools/asmdata.py` - reads the .asm's data sections into an addressable byte
  image (IDA only prints addresses inside auto-generated label names; named
  string labels like `aT_263` carry none, so it re-synchronises on hex-suffixed
  labels and tracks `align` padding exactly). Decodes narrow/wide strings by label.
- `tools/extract_xml_names.py` -> `manifest/xml_names.tsv`: the id -> XML-name
  table, read from `TVisionaireGame::InitXMLNames()` (800 `AddXMLName` calls,
  ids 1-840; ids 1-58 are structural XML names such as `x`, `left`, `path`,
  `Link`; 100+ are the data fields: `SceneIsMenu` = 0x124, `CharacterScene` =
  0x1F7, `SceneSavegameAreas` = 0x142, ...). Every guess the earlier passes made
  about a hex id (e.g. 0x124 "is menu", 0x143 "page step", 0x15C "savegame
  font") is confirmed by this table.
- `tools/gen_field_ids.py` -> `src/deponia1/vstables/fieldIds.h`: an `eFieldId`
  enum of `kCamelCase` constants named verbatim after those XML names (a
  duplicated name gets a `_0x<id>` suffix).
- `tools/apply_field_ids.py`: rewrote the 306 accessor call sites in the sources
  (`GetBool(0x124)` -> `GetBool(kSceneIsMenu)`); idempotent, so re-run it after
  adding code.
- `tools/extract_schema.py` -> `manifest/schema_fields.tsv` (583 fields: class,
  field id, name, value kind, link table, file-version range, savegame range) and
  `manifest/schema_groups.tsv` (41 record-type classes with their TTypeGroup
  constructor arguments and callbacks), from each `TT*::InitType()`/static
  initializer - they are straight-line `mov`/`xor` + `TTypeData` ctor + `AddType`.

Value-kind numbers (`eTypeData`) inferred from field names across all classes:
0 bool, 1 int, 2 string, 3 file path, 4 float, 6 rect list, 7 sprite list, 8
point list, 9 string list, 10/12 numeric lists (particle control points/curves),
11 string/path list (model textures), 13 point, 14 rect, 15 sprite, 16 link, 17
link list, 18 text-language list.

## TId (vscommon/scripting/id.h)

Confirmed (asm 586976-587170): a 4-byte value, a signed 24-bit id in bytes 0-2
and a one-byte table in byte 3. `==`/`!=` compare the sign-extended 24-bit id,
then byte 3. `PackId` gives `(id << 7) + (table & 0x7F)` (table 0xFF maps to
0x7F); `UnpackId` is the inverse. Member names (`_id`, `_table`) are invented.
`AnyId`'s real initial value is still unread.

## TXMLNames, TFieldValue, the XML writers and TLink

All confirmed from the asm (see the file headers for line ranges):

- `TXMLNames` (`TXMLNames.h/.cpp`): the id <-> XML-name registry. Ids are dense
  and 1-based (`AddXMLName` refuses anything but count+1 and logs "AddXMLName
  failed: <name>"). The name data is generated (`tools/gen_xml_names.py` ->
  `vstables/xmlNamesData.h`): ids 1-58 are named in `InitXMLNamesIntern`, ids
  59-99 are a loop adding the placeholder "DSunused", ids 100+ come from
  `TVisionaireGame::InitXMLNames()` (now implemented; run from the ctor).
  `manifest/xml_names.tsv` had mistakenly attributed "DSunused" to id 58; fixed.
  `GetNrByUtf8Name` reads a name of the form "T<digit>..." as the number after
  the "T" (`dtol`) instead of looking it up.
- `tools/asmdata.py` used to cut a quoted string at a `;` (so the XML entity
  strings lost their terminator); fixed.
- `TFieldValue`: tag (0 none, 1 int, 2 string, 3 bool, 4 float, 5 path) plus
  side-by-side payloads. `ToString()` escapes `& < > " '` for strings/paths.
- `baselib/xmlWriter.h/.cpp`: `TProjectFileWriter : TVedFile` is the abstract
  interface (22 pure virtuals, in vtable order); `TXMLWriter` writes indented
  XML (CRLF line ends, tab indents up to 10 levels) into a `TMemoryBuffer`;
  `TXMLStringWriter` additionally derives from the (invented-name)
  `TBufferedProjectFileWriter` interface `TMSavegame::SaveGame()` takes.
  `FinishWithContent` (never called) writes into `this` in the original; here
  into the buffer. An empty `vector<TTextLanguage>` writes nothing at all.
- `TLink` (`datastruct/link.h/.cpp`): 8 bytes - TId, field (short), flags (bit 0
  parent link, bit 1 any-object link). Verified end-to-end for the writer with a
  scratch program (TXMLNames lookups, attribute escaping, nested tags).
- `TTextLanguage` moved to its own header; its default language id is -1 (was
  0), per the default constructor.

## TData and the typed field storage (datastruct/data.h/.cpp)

Confirmed (asm 623914-635588): a record's fields live in one raw block, each at
the byte offset `TTypeData::GetOffset()` gave it, and `TData`'s static methods
act on "the value of kind K at this address" through a switch over the 19
kinds. The kinds (now `eTypeData`, datastruct/type.h) are 0 bool, 1 int, 2
string, 3 path, 4 float, 5 (no storage, unused), 6 rect list, 7 sprite list,
8 point list, 9 string list, 10 int list, 11 path list, 12 float list, 13 point,
14 rect, 15 sprite, 16 link, 17 link list, 18 text-language list. Defaults:
bool false, int -1, float -1.0, point/rect zero on creation but -1 once cleared.
The 19 `GetRef...()` helpers return the typed view of an address, logging
"value for <T> is NULL" (verbosity > 1) and asserting on null, and then
answering with a freshly emptied shared dummy.

- `TData::Compare()` returns 0/-1; the asm leaves a stale register on several
  "equal" exits (and inverts the string and string-list kinds), so the evident
  contract is implemented. Kinds 5 and 18 are never equal; link lists compare
  without regard to order.
- `GetDataSize()` returns the sizes of the reconstructed C++ types (rounded up
  to 8) instead of the original's fixed table, since the vectors/TSprite are
  real objects here.
- `DeleteDataInstance()` unregisters links from the visionaire first
  (`TVisionaire::RemoveLink`, unless the byte at +0x89 of the visionaire - name
  guessed, `IsLinkRemovalSuppressed()` - is set); `TDataGroup`/`TVisionaire`/
  `TVisionaireObject` only have the few accessors this needs so far.
- `baselib/xmlCommon.h/.cpp`: the attribute-text converters (`ConvertToInt/
  Float/Bool/String/FileName`) plus `dtol` and `normalizepath`.

## TDataGroup (datastruct/datagrp.h/.cpp)

Confirmed (asm 600389-607108, all 41 methods). One record's field storage: a
persistent block and a temporary block laid out by the TTypeGroup, name,
24-bit last-modified stamp, flags (dirty/temporary/order-contains-position),
event handlers, owner object. Links are kept in step with the visionaire's link
tables (AddLink/RemoveLink); removing a parent link removes the child object.

- `TSendEventEnum` was misnamed: 0 stores unconditionally and notifies, 1
  stores if different and notifies, 2 stores if different and does *not*
  notify. Every call site passes 2, now spelled `kNoEvent`.
- `TEventHandlerInterface::OnEvent(TEventEnum, int, TVisionaireObject *)` (the
  single pure virtual, confirmed via THObject's thunk) and `TDataCompareInfo`
  are new headers.
- `SetValue` mirrors ValueInt (0x148) and ValueFloat (0x325).
- `ForEachCall(kCompare)` and `TData::Compare` use the sane equal/different
  sense; the asm inverts string comparisons (see data.h).
- `ClearLinks` would loop forever in the original on a parent link with an
  empty id; here it is dropped.
- TVisionaire/TVisionaireObject gained stub accessors (SetDirty, GetMappedId,
  AddLink, RemoveObjectByParent, id/order setters, cmpOrder) to be replaced by
  the real implementations.

## TVisionaireObject, TVisObjRef, TVList, TTable

Confirmed (asm 586479-595389 TVisionaireObject, 597612-600388 TVisObjRef,
660697-668540 TTable):

- `TVisionaireObject` is a reference-counted wrapper around one `TDataGroup`
  plus the object's identity: id (+8), parent id (+0xC), 24-bit order (+0x10),
  flags (bit 0 "any object", bit 1 record in the same memory block, bit 2
  removed), parent field, Lua handle. Every forwarding method starts with the
  same "valid" test and answers a default (-1 / 0 / false / empty) otherwise. The
  typed `SetValue`s forward to `TDataGroup::SetValue` with the value's kind;
  `Remove()` and the destructor unlink the object from those linking to it.
  `GetLinkId`'s fallback is an "any" link (odd, but what the asm builds).
- `TVisObjRef` is a single counted pointer to one; all 85 methods forward.
- `TVList` owns one reference per element (`push_back` takes one, `clear`/
  destructor release; the last release deletes the object).
- `TTable`: objects in id order with an id -> position index (an
  `unordered_map` here instead of the 0x1000-bucket chain), a lazily built
  name-ordered list, a 24-bit next-id counter, the "versioned id" for
  new/deleted/changed detection, and the active-object link field. The
  editor-side methods (Import/Merge/FixOrder, partly Changed/Deleted/
  Conflicted and Paste) are only approximated.
- A scratch program built a type group, a table and two objects: ids, ValueInt
  <-> ValueFloat mirroring, lookup by name and id, `TVList` references and
  `Clear()` all behaved.
- TVisionaire is still a stub: GetObjectById, GetTable, RemoveObject,
  GetObjectsLinkedTo, GetMappedId, AddLink/RemoveLink... are next.

## TVisionaireGame and the table registry

`TVisionaireGame` now derives from `TVisionaire` (so TGameControl's `_visionaire`
and `_visionaireGame` are the same object, as the earlier call sites suggested).
Confirmed (asm 1499937-1504534, 1480082-1480237):

- The constructor builds `TVisionaire(TTGame::GetTypeGroup(), 0xBA)`, registers the
  XML names, sizes the registry to 39 tables and calls `AddTable` 39 times (the
  descriptions, table numbers, "eXxx"/singular/plural names, active-link fields
  and flags were read off the binary; table 21 is Loading, 24-26 the active
  text/action/animation tables). Then `CompleteTables`, a once-per-run
  `InitWithVersion(186)` and `NewGame`.
- `InitWithVersion` allocates/clears the 0x348-entry field lookup table and runs
  every record `InitType` (all of which are generated; see vstables/records.cpp).
- `NewGame` creates the main object and runs `TTGame::OnCreate` (still an empty
  stub - editor-only defaults, including a random game id).

Not done yet: loading/saving (LoadDataGame, LoadSaveGame, BeforeSave, the XML and
binary project readers), UpdateVersion, clipboard methods.

## The binary project reader (datastruct/binaryProjectReader)

Confirmed (asm 647056-655295, 619961-620665): `TVisionaire::Load` opens the file
through TComposedFileManager, then goes to `BinaryLoad` (VBIN files) - the XML
reader, `TXMLProjectReader`, is not reconstructed, so XML projects fail to load.

File format: `"VBIN"`, an unknown int, uncompressed size, stored size, then the
payload (zlib if the sizes differ). The payload is read through a global
TMemoryBuffer `membuf` and cursor `offset` (recovered global names). It starts with
a *schema tree* (`bin_structure`: u16 id <= 0x347, u16 child count <= 999, children):
one child per table whose children are the field ids the file stores per record, so
files of other schema versions can be read. Then three ints (project type, revision,
file version - a version other than 0xBA re-runs `InitWithVersion`), then per table:
a skipped int, the record count and per record: name (int length + bytes), id, order,
lastModified and the fields in the file's order as `loadObject` reads them (by
eTypeData kind). Name/id/order/lastModified (field ids 12-14, 55) are record header
data and skipped in the field list; `Version`, `ProjectType`, `Revision` (17, 53, 54)
are not tables; `Game` (116) is the main object, read the same way.

Loading types: 0 = read the file, load only the Loading table (index 21) and keep
the buffer; 1 = load everything from that buffer; 2 = both in one go. A savegame
(`eSaveGame` 1, always type 2) is loaded over an existing project: existing objects
are found by id (active tables always create new ones) and their old links are taken
out of the link table before being overwritten.

Quirks kept: `load(long&)` only skips 8 bytes; `load(TLink&)` reads the "any" flag
before the "parent" flag; the main object's link handling is conditioned on the
file-reading flag rather than the savegame flag. Deviations: reads past the end of
the buffer are detected (the original has no check) and make the load fail; the
buffer's length is set even when the data wasn't compressed (the original leaves it
0, which only spoils the progress ratio). The editor's progress signals to a
TSignalSlot are not reconstructed; LoadSaveProgressEvent is posted to the handler.

Tested with a synthetic file (schema + two Font records) only: no game data exists
locally.

Not done: the save side of the same file (writeStructure/save()/saveObject(),
TVisionaire::BinarySave), TVisionaireGame::LoadDataGame/LoadSaveGame, the XML reader.

## Loading game data: LoadDataGame / UpdateVersion

`TVisionaireGame::LoadDataGame` (asm 1523439) looks the file up by name (setting the
current directory to the file's when asked), posts the "ProgressLoad",
"ProgressUpdateVersion", "ProgressLoadFinalizing" events to the handler, runs
`TVisionaire::Load` (game data), `UpdateVersion` and, for a file of another version,
`InitWithVersion(0xBA)` + `RemoveTempData`. `LoadSaveGame` is `Load` (savegame, type 2)
+ `UpdateVersionGame` (the short cascade of savegame fixes, fully reconstructed).
`TGameControl`'s `_visionaire` is now a TVisionaireGame*.

`UpdateVersion` (asm 1508677, 14000 lines, a cascade of fixes by file version) is only
partly reconstructed: found by walking its control flow with the version fixed at 0xBA
(tools/ scratch script, not kept), which leaves the steps every version gets - objects
at (-1,-1) moved to their way system's first point, random TTValues rolled, the game's
scroll distances derived from the window resolution and, unless the file was loaded in
one go (type 2), mirrored animations rebuilt from their source, minimum pauses, and the
"game needs a first character" check. The conversions of files below version 0xB9
(and the rejection of < 99) are the missing part: such files get a log warning. If the
game data turns out to be older than 0xB9 this needs the other branches.

`TTAnimation::SetMirrored` and `TTValue::SetRandomValue` are written by hand
(TTAnimation.cpp, TTValue.cpp; declared through tools/gen_records.py's EXTRA_METHODS).

## THGameControl and TTText

THGameControl derives from TGameControl and TEventHandlerInterface (second vtable at
+0xA90 of the original); the program instantiates it (AppGlobals: g_pGameControl).
RegisterEventHandler registers it for changes of the game object; OnEvent applies the
settings that have a running effect: scroll speed (_timingValueSeconds), earthquake,
current character/scene/text/cursor visibility, scroll position/character, text and
speech language, hold time (the recovered global GameMinDownTime), hidden interfaces,
the active command and the used item. The TGameControl members are now protected for it.
TTText is now a TVisObjRef-derived handle: SetLanguage/SetSpeechLanguage (only objects of
table 0x12, the Languages), GetTextLanguage (by current/given language id),
GetTextLanguageOrCreate, GetTextString, GetTextProperties, and ReplaceValues
(`<v=name>`, `<vi=name>` integer and `<vs=name>` string placeholders of Value objects,
'?' for an unknown one). GetNameInList (editor label) is still not reconstructed.

## p2t and the polygon helpers

p2t/p2t.h/.cpp is the open-source poly2tri (1.3) constrained Delaunay triangulation the
way system uses; the binary's copy has the same function set (checked by name and by
Orient2d/InScanArea/cmp/MeshClean), so the library's own algorithm is written out rather
than decompiled. The binary's p2t::Triangle has two extra members (a cached area and its
valid flag, used by PointInTriangleTolerant) which the way-system code will add.
Tested: L-shape with a hole, a 24-point star, a comb - n-2 triangles, exact area.
TPolygonList (an owning list of polygons) and the polygon helpers (LinesCut,
PointInPolygon[List], CreatePolygonsFromPointList with its (-10000,-10000) separators,
GetBoundingBox with inclusive width/height, IsPointInsidePolygon) are in TPolygonList.h.

## TCAnimation (vscommon/canimation.cpp)

The sprite animation player that `TGAnimation` derives from. It was left as a
confirmed-call-shape stub through the earlier passes; now real (moved to
`vscommon/canimation.{h,cpp}` once the `x_assert()` in
`GetCurrentSpritePosition` gave its original path: `src/vscommon/canimation.cpp`,
line 0x246).

A `TCAnimation` holds two records - a `TSAnimation` (+0x08, the running state:
loops left, current sprite index, position, scale, waiting) and the
`TTAnimation` data (+0x10) - plus one `TPictureIO` per sprite of the data
(owned, deleted in the dtor) and a `TTimer`. All running state lives in the
`TSAnimation` record rather than in C++ members so that it survives a save;
the class itself keeps only the data reference, the pictures, the current
picture, two flags (`_isOutfit`, `_paused`) and the timer.

Real: ctor/dtor, `Start`, `CanRemoveCurrentSprite`, `GetAnimationLoops`,
`FirstSprite`/`NextSprite`/`EofSprite` (forward, backward and random order),
`GetCurrentPause`/`GetPauseCompletion`, `SetCurrentSprite` (the per-frame
step: random-loop waits, advancing after the pause, loop counting, finishing),
`GetCurrentSpritePosition` (mirrored sprites mirror around the animation's
centre; scaled by the size as a percentage with round-half-away rounding) and
the `Is*` queries. Checked in particular `CanRemoveCurrentSprite` against the
disassembly (vtable slot 0x28 is `GetAnimationLoops`).

Deliberately NOT reconstructed: the 3D-model (`ModelContainer`/`ModelAnimation`)
and Spine skeleton animation kinds the class also plays. Deponia 1 only uses
sprite animations; every model/bones branch is left out, so `IsModelAnimation()`/
`IsBonesAnimation()` are fixed `false`. This is why the manifest status is
`in-progress`, not `done`. Tracked as a low-priority item in the repo-root
`TODO.md`: the engine is meant to be reusable for other Visionaire games, which
may use those kinds, so they are to be done before the project is finished.

Side corrections found along the way: `TSprite::SetPosition(pos, scale)` takes
`-1.0f` (not `1.0f`) as its "leave the scale alone" argument - fixed in
`TTSprite::SetPosition` and the binary project reader; `TCAnimation::SetPosition`
uses the same sentinel. `TPictureIO` gained `SetPreloadPriority()` (+0xB0) and
`TPreloadedPicManager::PreloadPicture()` is a call-shape-only stub.

## TGAnimation and THAnimation (vsplayer/animationGame.cpp)

The game's animation player, layered on `TCAnimation`. Until now only its static
entry points were known as call shapes; now real (moved to
`vsplayer/animationGame.{h,cpp}`, the path the `x_assert()` in `DetachOwner` gives).

What a `TGAnimation` adds to `TCAnimation`: a frame list (+0x90, the data's
`kAnimationPropertyFrames` objects, each with a sound and an action for a sprite
index), the object it belongs to (+0xA8: the data's parent, or an outfit
animation's character), and the owners (+0x78, `TAnimationOwner*`) to tell when it
stops. All running animations live in the static list `RunningAnimations` (the
recovered name; `EventHandlerAnimStarted`/`EventHandlerAnimStopped` are the two
recovered static hook names). Animations are always `new THAnimation(...)`
(0xB8 bytes), which is a `TGAnimation` plus a `TEventHandlerInterface` registered
on the animation's running-state record.

How it fits together:

- **Start** (`StartAnimation`): an animation with no sprites (and not a model/Spine
  one) is refused. If the data object already has a running animation: a
  *preloaded* idle one is restarted (`Start`, then the 1-based `frame` if wanted,
  then `SetCurrentSprite(true)`); a preloaded running one just gains the owner; a
  non-preloaded running one gains the owner; a non-preloaded *ended* one (waiting to
  be deleted) is refused (returns null). Otherwise a new one is created from a new
  TSAnimation record (`CreateActiveObject(0x1A, data)`).
- **Per frame** (`ContinueAnimations`): skip stopped/paused/ended ones; step the
  sprite; one that ends now tells its owners (only if it is preloaded - otherwise
  its destructor does), starts its data's action and calls the stopped hook. A second
  pass deletes ended animations that are not preloaded.
- **Hide** (`HideAnimation`, three overloads sharing one private tail): sets the
  record inactive, calls the stopped hook, then either keeps the animation (preloaded:
  owners are told) or deletes it. With an owner it first detaches that owner and only
  hides when none is left.
- **Preload/Unload**: preloaded animations stay in the list after they end
  (`kAnimationPreloaded`), idle, to be shown again cheaply; `UnloadAnimation` deletes
  one (even a running one).
- **Save/Load**: only preloaded animations, and active ones the player started
  (`kAnimationStartedByUser`), keep their record (with the time since they were
  called); every other record is marked temporary. `LoadAnimations` rebuilds an
  animation from every saved record whose original data object still exists.
- **Menus**: `StopRunningAnimations` sets the stopped flag of every animation except
  those of a scene that is a menu (data -> parent -> parent has `kSceneIsMenu`);
  `ContinueStoppedAnimation` moves the timer on by the time spent stopped.
- **Frames** (`NextSpriteSelected`): for every frame object whose index is the sprite
  just selected, play its sound - only if what the animation belongs to is on the
  shown scene (a character in it; an object that is not on a scene, or on the shown
  one; anything else always) - then rename its action to `"<name>: Frame #<n>"` and
  start it.
- **Drawing** (`Draw`): preloads the next two pictures, then sets the picture's
  rotation, rotation centre, scale, shader and matrix id from the owning object (an
  object of type 6, a button of type 2, a character of type 0 - each with its own
  field ids), and draws it at the animation's position and size.

Things worth knowing:

- `TAnimationOwner` is now a real base of `TManagedObject` (it was modeled as plain
  virtuals): the owner list needs the pointer, and `ReattachAnimations()` adds a
  `TManagedObject` to it.
- `TCAnimation::_flag66` is now `_stopped` (set from construction and while stopped;
  cleared by `Start`/`Load`/`ContinueStoppedAnimation`).
- `g_traceFlags` is a real exported global (`AppGlobals.h`); bit 0 enables the
  animation trace messages (with `wxLog::loglevel > 1`). All those messages are
  reproduced, so they work once the flag is set.
- `DrawWithLightMap` reads the (float) size field with `GetInt`; in the original that
  read fails and gives -1, which leaves the sprite scale alone. Reproduced as is.
- `TTAnimation::IsModelAnimation()`/`IsBonesAnimation()` (the static data-object
  tests) are real; the instance tests of `TCAnimation` stay fixed `false`.
- The three log strings of a few trace messages have extra arguments the format does
  not use (`"Animation loaded: "`): reproduced, minus the unused arguments.
- Call-shape stubs added: `LuaDebugName`, `LuaExecuteEventHandler` (the Lua bridge),
  `TSoundInterface::Play()` (the 7-argument overload),
  `TPreloadedPicManager::StopPreloading(vector&)`/`PreloadPictures()`/
  `HasQueuedPictures()`. `TPictureIO` gained the rotation/rotation centre/scale/
  matrix id setters (its fields at +0xC4..+0xDC were misnamed as a "load rect" before;
  +0xC8/+0xCC is the rotation centre, (-1, -1) when none) and its preload priority now
  starts at -1.
- Not reconstructed (see `TODO.md`): the model/Spine branches, and the debugger
  overlay (`GetAnimationDetails`, `PrintRunningAnimations`, `DrawAnimation`).

Tooling added along the way: `tools/asmview.py` prints a range of the `.asm` without
the noise (labels kept, XREF comments, push/pop and string-refcount code dropped with
`-t`; careful, `-t` also hides `test eax, eax`, which can hide a branch condition)
and `tools/wstr.py` decodes the wide-character strings IDA lists one byte per line.

## TGWaySystem, TPointSize and the way-finding geometry

What a character walks on. A `TGWaySystem` is built from a scene's `TTWaySystem`
(`SetWaySystem`): the outline (`kWaySystemBorder`) is cut into polygons at the separator
points, a polygon whose first point lies inside another is a hole of it, and every
outer polygon is triangulated with its holes by the project's p2t (`p2t::CDT`). The
way points (`kWaySystemPoints`, linked to each other through `kPointRelations`) give
a coarser graph. Original layout (all documented in `TGWaySystem.h`): `TPointSize`
base, outline +0x18, way points +0x30, the way +0x48, the best/current search length
+0x68/+0x6C, the end way point +0x70, the route being searched +0x78,
triangulations/triangles/their points +0x90/+0xA8/+0xC0, destination +0xD8, flags
+0xE0/+0xE1.

Two ways to find a way:

- `CalculateWay(wayPoint, from, to)`: the way points visible from `from`
  (`GetAllPointsNextTo`, a `LineCuts` test against the outline); if the nearest is
  connected to the end way point (`IsConnected`, a breadth first walk over the links)
  a depth first `SearchWay` finds the shortest route (pruned against the best
  length so far, neighbours tried nearest to the end first), which is then trimmed:
  its start moved to the farthest route point that can be seen directly, and its end
  cut at the first point `to` can be seen from. With no way point in sight it walks
  straight (`LineInPolygonSet`) or round the outline's corners (`shortestPath`).
- `CalculateWayTriangles(from, to)`: for a `to` outside the triangulation. A circle
  around `from` through `to` is intersected with the outline's edges
  (`circleLineSegmentIntersection`), a point 2 pixels to either side of each hit is a
  candidate, and the one whose direction from `from` is nearest to the direction of `to`
  (within 80 degrees) that lies in a triangle becomes the way. (A far-away `to`
  finds none: the circle has to cross the outline.) The candidates are left in the
  exported global `drawPoints` for the debug overlay.

`GetNextPosition` is the per-frame step along the way (the direction of a leg is worked
out when it starts; reaching the end of the way heads for the destination if that
is not where it ended). `CheckPosition` keeps a position in the walkable area: inside a
triangle it stays; just outside one (within 0.2 in barycentric terms) it moves to the
nearest point of the triangle; further away it is left alone.

`TPointSize` scales characters by where they stand: the way points that have a size
(`kPointSize`) give (y, size) pairs sorted by y, interpolated linearly (extrapolated
past the ends; 100 with none).

Free functions (geometry): `LineCuts` (two overloads), `GetNumCuts`, `LineInPolygonSet`
(a segment lies in the area: it crosses no edge properly, partly overlaps none that
lie along it, and its middle is inside), `shortestPath` (Dijkstra over start, polygon
vertices and end with visibility as the edges; returns only the corners in between),
`PointInTriangle`/`PointInTriangleTolerant` (barycentric, with the triangle's area
cached in two fields the binary's `p2t::Triangle` has and the library does not),
`closesPointOnTriangle` (the standard region construction in single precision -
including its region 6, which reuses region 1's formula as the original does),
`angleDist`, `argsort`, and `GetAngle` (now real: degrees, 0 to 359, y inverted;
note it truncates, so a 45 degree vector gives 44).

Things worth knowing:

- The original's `PointInTriangle` and `PointInTriangleTolerant` only differ in
  the threshold (0 against -0.2); `PointInPolygonList` in `shortestPath` is called
  for the end point and its result ignored (omitted).
- The log message in `SetWaySystem`'s catch uses a format string that reads as the two
  letters "wa" in the binary; reproduced with a plain format, noted in the code.
- Not reconstructed (nothing calls them): `Len`, `Normalize`, `Distance`,
  `LinePointDistance`, `LinePointPoint`, `dot`, `_point`, `toVec2`, `MapPointToLine`,
  `Rotation`, `PointOnLine`, `GetDotProduct`, `RectHit`.
- `TGCharacter` now has a `TGWaySystem` member; `InitWaySystem` and
  `CheckCharacterPosition` are real. The walking itself (`TGCharacter`'s use of
  `CalculateWay`, `GetNextPosition`) is still to come with the rest of that class.
- `GlmStub.h` is a two-line stand-in for `glm::vec2`. `TManagedObject::_position` is
  now protected.
- Tested with a scratch program (not committed) over real objects built through the
  data layer: a 200x100 area with a hole triangulates into 8 triangles; positions
  inside are kept, a position just in the hole is moved to its edge, a way through
  two way points is walked to the end, and targets just outside the area resolve to a
  point near it.

## TMCharacter and TGCharacter (characterManaged.cpp, the characters of a scene)

`TMCharacter` (asm 137965-138511) is what is specific to a character in the managed object:
the actions are the character's own list, the direction is its own field, it is hit
by the opaque pixels of its sprite (and not by the character that detects), and its name
is a language text. It has one pure virtual of its own, `GetCurrentSpriteRect()`
(vtable slot 0x100). `TGCharacter : TMCharacter` (asm 175318-186239, 76 methods) is the
character that walks, talks, turns and plays animations; `THCharacter` only adds the
constructor.

What was recovered, and not obvious from the code:

- The vtable names: slot 0xB0 is `GetLanguageName()`, 0x68 `GetPosition()` (what was
  first called `GetScreenPosition()`), 0x100 `GetCurrentSpriteRect()` (was
  `GetVisibleRect()`).
- States: `_state` (+0xF0, the field `kCharacterState`) is 2 standing, 3 walking, 4 a
  turn animation plays (`_previousState` remembers a walk that is to go on after it).
  `kCharacterAnimState` (0x1FC) is the kind of the animation shown (`TCharacterAnimEnum`).
- `GetDirectionIndex()` is the nearest-direction search over an outfit's animations: the
  direction lists are ascending, the same direction can repeat (variants, chosen by
  `kCharacterAnimIndex`), and the search wraps at 360. Ties go to the lower direction
  in the middle of the list, to the first when above the last, to the last when below
  the first. **Quirk of the original reproduced**: when the direction is above the last
  one, nearer to it than to the first, and the last direction has variants, the
  comparison is with one less than the size of the group, so the last variant is never
  chosen.
- `CheckTurning()` builds four entries per turn animation, but all with the same
  `(from, to)` pair, so only the first (unmirrored, not replayed) can ever match; the
  mirrored/replayed combinations are dead code in the binary. Also: `SetCurrentOutfit()`
  never clears the turn animation lists, so a second outfit adds to them.
- `SetCurrentOutfit()` writes into the outfit's animations in the game data
  (`kAnimationMove` = true and `kAnimationNumberOfLoops` = 0 on the walk/talk/standing
  ones, 1 loop on the turn ones), and `UnloadAnimations()` zeroes `_state` while the
  animations are unloaded (restoring it after).
- `WalkWay()` has two kinds of walk animation: with `kOutfitSlideWalkAnimation` the
  per-frame values of `kAnimationWalkSteps` are speeds per second (scaled by the time
  since the last step, at most 500 ms); without it they are the distance to step when
  the animation reaches the next frame (and a 0 there means do not step).
- `IsReached()` is two things: for a character that follows another (`strict`), the
  distance depends on both sizes and the follow reach; otherwise it is 1.5 sprite
  widths (50 when the character has no sprite), or an exact place when
  `kCharacterActionDestPosition` is set.
- The walking sound is panned from the character's place on the screen
  (`x / window width * 200 - 100`) and has the volume of its size (0-100).
- Constants: 100.0 (percent), 500.0 (longest step time), 1000.0 (ms per s), 200.0
  (`IsReached`), 1.5 (sprite widths), 0.0 (no step).
- Checked with a scratch program (not committed): `GetDirectionIndex()` over
  `{0, 90, 90, 180, 270}` and `{30, 30, 100}` gives the nearest index at, between and
  beyond the ends, with the tie rules above.

Left out (see `TODO.md`): the model and Spine branches (`IsSpineAnimation()` is real
but the code that plays several animations at once is not), the matrix transform of
`TMCharacter::IsInside()` and `TGCharacter::Draw()`, and the Lua hook
`CharacterDirectionHook` that `GetDirectionIndex()` asks first (the same standing
Lua-bridge gap as the other hooks; the registered name is stored).

Needed from other classes: `TSoundFFMPEG::StopSound()` (call shape only: the virtual
at slot 0x48), `TSoundTypeEnum::kValue3`, and `TManagedObject`'s `_color`, `_lifetime`,
`_animations`, `_text`, the alpha fields and `_timer` made protected.

## TMObject, TGObject and THObject (the objects of a scene)

`TMObject` (asm 116072-116550, vsplayer/objectManaged.cpp) is the managed object for a
scene object: it owns the picture of the object's sprite (a `TPictureIO` the base's
picture pointer points to), shows the sprite at its own position moved by the object's
offset (`kObjectOffset`, scale -1 = keep), and while active shows the object's animation
(`kObjectAnimation`, when it is one of `kObjectAnimations`). `TGObject` (asm
258158-259640) adds the hit polygon, position and centre, the scroll factor (parallax:
`kObjectScrollFactorX/Y`, percent, stored as the value minus 100, with a flag set when
either is not 100), shader/matrix/rotation/scale settings, an optional particle effect and
the snoop animation. `THObject` (asm 113494-114760) is what the game creates: a `TGObject`
that is also a `TEventHandlerInterface` and takes over changes of the data object's fields
and of the variables of its condition.

- The manifest range for `TGObject` stops at 259458; the rest of its constructor runs
  to 259640, after which come unrelated classes (`THAnimation`, `TSceneActionArea`, ...)
  that happen to follow it.
- `THObject::RegisterConditions()` recurses through the two conditions a compound one is
  made of; a condition that is already on the way down is a cycle and is logged, its
  message is `Condition '%s' (id: %d) has a cyclic reference: %s` with the chain
  `'name' (id: n) - ...`.
- The `kObjectRotationCenter` event reads the matrix id too (the code of the two cases
  is shared in the binary), and `kObjectScale` sets both scale fields with `kSendEvent`
  (which then arrive as their own events).
- The object's fields at +0x17C..+0x194 (rotation, rotation centre, shader, matrix id,
  scale) are not the object's: they lie inside the embedded `TPictureIO` (0xE8 bytes
  from +0xB8, which `TMButton` confirms: its second picture is at +0x1A0 and gets the
  same values at +0x264...). They are the picture's draw settings, set by the owner
  through inline setters (`TPictureIO::SetRotation()` etc.), and are modeled so.
- Not reconstructed (see `TODO.md`): the particle effect built from a script expression
  (`kParticleContainerSettings`, the same Lua-bridge gap as in `TGScene::BeginScene()`),
  the graphics-backend calls that draw the particles, and the matrix transform in
  `IsInside()`/`DrawSnoopAnimation()`.

## The button and item family: TMButton, THButton, TGPlaceHolder, TGItem, THItem

`TMButton` (asm 165577-166300) is the managed object of a button of an interface: two
embedded `TPictureIO`s (active sprite from `kButtonActiveSprite`, inactive from
`kButtonInactiveSprite`; `SetActiveSprite()` points the base's picture at one of them), the
button's own polygon and action list, and its animation (`kButtonAnimation`) while it is
active. `THButton` (asm 114929-116060) adds the event handler (a second vtable at +0x288),
the variables of the condition it listens to and the interface it belongs to
(`kConditionValue` makes that interface show or hide its objects). `TGPlaceHolder` (asm
114826-114878) is a `THButton` with nothing of its own (so header-only). `TGItem` (asm
260668-261289) is the item of an interface (a `TMObject`) drawn centred on the position
the interface gives it and `THItem` (asm 218884-219190) the one the game creates, which
follows visibility and scale changes.

- The draw settings (rotation, rotation centre, scale, matrix id, shader) that these
  classes set at +0x17C... / +0x264... are the `TPictureIO`'s own fields, set through
  inline setters (see `TPictureIO::SetRotation()`; `SetScaleX()`/`SetScaleY()`/
  `GetMatrixId()` were added). `TMButton` has the two pictures at +0xB8 and +0x1A0, which
  also fixed `TPictureIO`'s size at 0xE8 bytes.
- `RegisterConditions()`/`UnRegisterConditions()` are the same code in `THObject`,
  `THButton` (and, by the look of the callers, `THItem`'s siblings): written once in
  `ConditionListener.{h,cpp}` and called from each class's own method.
- `TGItem::HandlePostExecution()`: for a click (mouse event 1, 3 or 4) that no action ran
  for, an item becomes the used item (`kGameUsedItem`), and with the game's draggable items
  on and a draggable command also the object that moves with the cursor. Commands 1 and 2
  count as "use".
- `TGInterface::GetPlaceHolder()` is only a call-shape stub for now (null) until
  `TGInterface` itself is reconstructed.

## THScene and TGActionArea

`THScene` (asm 219228-219860) is the scene the game creates: a `TGScene` that is also a
`TEventHandlerInterface` (second vtable at +0x378) and follows the scene's data object:
brightness (kept in 0..100, also in the data), the current way system (every character of
the scene walks on the new one), scrolling on the edges, the lightmap, the scrollable
area, and the music (a change of volume or balance plays the music again with the new
values through the sound manager's `PlaySound`; a new music file is preceded by the sound
manager's virtual at slot 0xD8 and followed by the one at 0xE0 with `(4, 3000, false)`
- both only call shapes, named by their slots). `TSceneControl` now creates `THScene`s.
`TGActionArea` (asm 247238-247290) is a `THButton` by another name, like `TGPlaceHolder`;
header-only. `TGScene`'s data reference and brightness are protected for `THScene`.

## TGInterface, THInterface, TGCommand and TGScrollButton (the interfaces)

`TGInterface` (asm 138535-143198) is a panel of the game made from an interface record
(`kInterface*`): background sprite, border polygon, buttons, and the items of the current
character. Its buttons are made by `Init()` from their `kButtonType`: 0 `TGPlaceHolder` (a
slot for an item), 1 and 2 the `TGScrollButton`s (back, forward), 3 and 6 `TGCommand`,
4 `TGActionArea`, 5 nothing. `THInterface` (asm 246907-247200) is the one the game creates
and follows the record's fields.

- The items (`THItem`) are made by `UpdateItems()` from the items the character has; the
  place holders show the items from the scroll position on, one each, but only the
  place holders that are active (`GetPlaceHolder()`, `GetObject(point)`, `Draw()` all count
  them the same way; `Draw()` goes through the buttons from the last to the first).
- `Draw()` has `UpdateAlpha()` inlined; `SetDestAlpha()` too (an alpha change without a time
  is applied at once). The alpha is given to every button and item as a percent.
- The original keeps two `HashMap<TId, int>`s (button and item by id) with 0x80 buckets; they
  are `std::unordered_map`s over the four bytes of the id here.
- A scroll button scrolls back when its type is 1 (`kInterfaceItemsScrollPosition` minus
  `kInterfaceScrollStepSize`). `UpdateItems()` keeps the scroll position in range by
  taking a step off at a time. `TGCommand::ExecuteEvent()` makes the command the active
  one of its interface; the original builds a copy of the event first (nothing changes).
- Not reconstructed (see `TODO.md`): the matrix transform of the mouse position in
  `GetObject(point)` and `IsInside()`.

## The text engine: TSText, TGText and THText

`TSText` (asm 1454187-1460704) is the text that is shown while a character speaks; it works on
its record (the "ActiveText" table: the text that is left, the part shown now, the time the part
stays, whether it waits for the speech, whether it is active). `TGText` (asm 212566-216250) draws
it, and `THText` (asm 144161-144340) is the one the game creates.

- A text is made of parts cut by pause tags: `<p>` (until skipped), `<pt>` (as long as the text
  so far takes without speech: 130 ms a character), `<pa>` (until the speech ends; no speech: as
  `<pt>`; a number after `a` is the fallback), `<p5>`/`<p5s>` (seconds), `<p500ms>`
  (milliseconds). The times of `<pt>` and the numbers (not `<p>`/`<pa>` with speech) are scaled
  by the player's text speed (`kGameTextSpeed`, percent). A text without any tag is one part that
  stays as long as its speech, or 130 ms a character.
- Reproduced from the binary: a tag that does not end (`<p5` without `>`) leaves the rest of the
  text unchanged, so the text stays on its first part until it is skipped; an invalid number
  (`<p5x>`) is logged and the tag is dropped.
- The speech goes through the sound manager as a `TSignalSlot` (messages: is speech turned off
  `0x1003`, can speech play `0x1012`, load `0x1005`, play `0x1008` with the pan, is it still
  playing `0x1000`, stop `0x100F`). The sound manager is not reconstructed: the default
  `TSignalSlot::Signal()` answers every question with 0, so a text goes by its length. The signal
  fields start at -1 as in the original; `TSignalData` gained the speech file and four values.
- The vtable of the text is: `SetText`, `SetTextIntern(TextOutputEnum, int)`, `ClearText`,
  `CalculateRestText`, then the destructors. What `TGameControl` called `OnCleared()` and
  `Discard()` were `ClearText()` and the deleting destructor.
- `TGText` takes the pan of the speaker's sound from where it stands on the screen
  (`-100...100` scaled by `kSpeakerSoundPanFactor`), keeps the speaker's talking animation going
  while it shows, puts the lines above the speaker's sprite (beside it when there is no room
  above, on the side with more room) inside the part of the scene in view, and tells its owner
  (an object, for the texts of objects) when it is over. A '<' in a text is a line break.
- Not reconstructed (see `TODO.md`): the three Lua hooks of `TGText` (`TextTextHook`,
  `TextPositionHook`, `TextRenderHook`; their registered names are stored), the glyph printing
  (`TCFont`), and the sound manager.

## TGDialog (the dialog the player picks the replies of)

`TGDialog` (asm 220104-223152) is a `TTDialog` - the dialog record itself sits at its address
(so `TGameControl::Save()` hands `&_dialog` over as the current dialog) - with the lines of the
parts the player can pick, their rectangles, the arrows to scroll and the background. The fonts,
the area, the five sprites and the space between lines are the data of the character that is
speaking (fields `0x1C9`..`0x1D0` and `0x245`).

- `SetDialog()` takes the parts that are available (`kDialogPartAvailable`) and whose condition
  holds (negated by `kDialogPartConditionNegate`), cuts each part's text in lines that fit the
  area (`TFontManager::SplitTexts`) and puts the lines one under the other. A character without
  an area (top `-1`, or an empty one) has the whole window.
- Reproduced from the binary: the text goes through `Replace("<", "
")` *before* it is searched
  for `"<p"`, so the cut at a pause tag never finds one (the '<' of every tag is gone by then).
  The value placeholders (`<v=..>`) go through `TTText::ReplaceValues()` after the same replace.
- The mouse: a line is picked only when it is wholly inside the area (the lines scrolled out are
  not shown or picked). The wheel (messages 12 up, 13 down) does what a click on the arrow does,
  at most every 10 ms; it leaves the arrow marked as hovered until the mouse moves.
- Picking a part builds the game's dialog action (`kGameDialogAction`, made if there is none,
  emptied if there is): the actions of the part and of its linked action are copied into it, then
  the character says the part's answer text (command `0x17`, first in the action) and the
  other answers with the part's text (or the alternative text), then the action goes on to the
  next dialog (command `0x0D`, last) when that has a part to pick - else by the part's "return"
  setting: `0` the same dialog again (when it has a part left to pick), `0` with nothing left
  or `1` the dialog above (the grandparent record, when it is a dialog with a part to pick).
  Picking again the part the action already starts with leaves the action as it is. Then the
  action runs (`TGAction::AddRunningAction`) and the dialog ends.
- A part with `kDialogPartRemove` is made unavailable when it is picked.

## TGAction, THAction and the action part commands (the scripted actions)

`TGAction` (asm 194786-212169) runs an action: the list of action parts of a data object (`TTAction`)
with its state in a record of its own (`TSAction`, the "ActiveAction": which part is next, whether it
was started, the depth inside an IF that is not taken, how long it was paused). `THAction` (asm
187010-187150) is the one the game creates: it also listens to its record (and does nothing with what it
hears). The running actions are in a list (`RunningActions`) and a table from the data id to the place in
the list: an action is started once (`AddRunningAction()` returns the one that runs already).

- `Execute(skip, skipInfo)` goes through the parts from the next one: a part that can be done is done and
  the next follows, up to 10000 in one call (then the action is "probably in an endless loop" and is
  stopped, as logged); a part that waits ends the call and is looked at again next frame. A command tells
  the loop three things (the registers `al` and `r12b` of the original): wait, stay on the same part
  (a goto) or go on. The IF/ELSE/END IF parts work by the depth in the record: above 0 only the ifs and
  their ends are counted (`IsIFActionPart`: commands 0x45, 0x4F, 0x54, 0x5C, 0x5F, 0x67, 0x7A, 0x7B,
  0x8C, 0xA0); an IF that is false adds 1, the ELSE and END IF at depth 1 end the skipping.
- Skipping a cutscene (`SkipCutscene()`): the game's cutscene action (`kGameCutsceneAction`, set by the
  command 144 that starts it) is executed with `skip` set. First what the parts did before is undone
  (animations hidden, texts cleared, sounds stopped, started actions run to their end), then the
  commands do their end result at once and tell the `t_SkipCutsceneInfo` (the character and the scene it
  ends with, the scenes whose music changed) so that `SkipCutscene()` can put the game there.
- The commands are in `TGActionPartExecutor` (`TGActionCommands*.cpp`), the numbers in
  `vstables/eCommand.h`: all 104 entries of the jump table `jpt_562CD8` (index = command - 5; the other
  numbers do nothing) are done. The names are invented, from what each one does (and from the
  messages the binary logs: 'Change scene', 'Set value', FADE_INTERFACE, SHOW_HIDE_INTERFACE ...); the
  editor's own names are loaded from a file at run time. `tools/cfg.py` prints the code of one entry of
  such a jump table, `tools/av.py` is a compact reader for long functions and `tools/strs.py` prints
  the strings.
- Reproduced from the binary: the command 12 ('Change character') is not done while a text is shown
  (the part is skipped); `kCommandSetObjectActive` leaves the active command of an interface as it is
  when it is to be shown inactive; a divide by zero of 'Set value' (which stops the original) leaves the
  value as it is; the sound parts (24, 72) call the sound manager the same way and Keep() the sound when
  their last integer is 0; the save commands (145, 146) set `TGAction::s_saveAction` and save the game
  with the index of the part moved on (and the action marked finished when that is past the last).
- The text of a script (commands 137, 138) is made as the original does (the '<' of the data is a new
  line), but the Lua bridge is not reconstructed, so it is not run (`LuaDoString()` and
  `LuaSetCurrentAction()` are call-shape stubs).

## The sound interface and the tweens

`TSoundInterface` (asm 1105963-1107210) is now real: the volumes (music, sounds, speech, movies, all),
whether sounds are turned off, the background music (kept when the same name - what is before a '#' - is asked
for again, faded out when another one comes), and the messages the texts send it (`Signal()`: ids
0x1000-0x1012; what was called "load" for the speech is the speech volume). The sound engine
(`TSoundBase`, `TSoundFFMPEG`) is not reconstructed: its virtual methods are declared in
`TSoundInterface.h` with the names and the slots of `TSoundBase`'s vtable and do nothing. The names
the earlier work guessed for these calls (`PlaySound`, `Resume`, `OnVideoFrameFinished`, `Slot0xD8` ...) were
`SetStats`, `CleanUp`, `ContinueAll`, `FinishSoundFade`.
`Tween` (a number over a time through an easing function) and `TVisObjTween` (it sets the x and y of a
field of a data object each frame) are reconstructed; of the easing functions only the linear one is.

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
