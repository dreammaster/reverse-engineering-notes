---
name: scummvm-events-framework
description: "Deep reference on the ScummVM Events/View dispatch framework (as implemented in engines/got), for porting other old games to new ScummVM engines"
metadata: 
  node_type: memory
  type: reference
  originSessionId: ecc9fbfd-126a-4a61-a3f5-b9ae2ce2c322
  modified: 2026-08-20T01:45:36.965Z
---

# ScummVM Event-Dispatch Framework — Reference Notes

Canonical worked example: `engines/got` (God of Thunder). This framework is **not** a shared
base in `engines/` common code — each engine currently implements its own copy of
`UIElement`/`Events`/`View` (GOT's lives in [events.h](../../../dev/scummvm/engines/got/events.h)
and [events.cpp](../../../dev/scummvm/engines/got/events.cpp), with the `View` specialization in
`engines/got/views/view.h/.cpp`). When starting a new engine, copy/adapt these files rather than
expecting to `#include` a shared version — check first whether a shared version has since been
extracted into `engines/` common code before assuming this.

## Why it exists

Old DOS games relied on the user power-cycling the machine or navigating in-game menus to quit.
In a windowed ScummVM, the user can hit the window's close (X) button at any time, and the game
must unwind cleanly no matter where it is in a nested call stack. The old approach of sprinkling
`shouldQuit()` checks through every function and manually unwinding was unmaintainable. This
framework instead represents the game as a stack of **views** (state machines), with a central
**Events** dispatcher that owns the main loop, polls OS events once, and routes them to whichever
view is currently focused. Quitting is handled once, centrally, in the main loop — not scattered
throughout game logic.

The corollary is that porting a game to this framework means turning sequential, blocking
"draw prompt → block on input → process → draw next prompt" code into **state machines**: a view
holds an enum/mode field, a `msgFocus` sets up the first sub-state, each input message advances
the state and calls `redraw()`, and `draw()` renders whatever the current state calls for.

## Core class hierarchy

- **`UIElement`** — base class for anything that can appear on screen and receive messages:
  both views and finer-grained controls within a view. Holds:
  - `_parent` / `_children` (tree structure — see "Registration" below)
  - `_bounds` (a `Bounds` thunk — see below) and `_innerBounds`
  - `_needsRedraw` flag
  - `_name` (used for lookup via `findView`)
  - a generic countdown timer (`_timeoutCtr`, `isDelayActive()`, `cancelDelay()`, `timeout()`)
- **`Events`** — the dispatcher/engine class. `class GotEngine : public Engine, public Events`.
  Kept deliberately separate from the game-specific engine class so files that only need view
  management (`addView`/`replaceView`) don't have to pull in the whole engine header — saves
  compile time. Owns:
  - the `Graphics::Screen`
  - a `Common::Stack<UIElement *> _views` (the view stack)
  - the main loop (`runGame()`)
  - `shouldQuit()` as a pure virtual the concrete engine implements (typically just
    `Engine::shouldQuit()`)
- **`View`** (namespace `Views`) — a `UIElement` subclass adding conventions for top-level
  screens/dialogs: mouse-focus tracking among children (so a control only gets
  `MouseEnter`/`MouseLeave`/mouse events when the cursor is actually over it, rather than every
  control checking bounds itself), plus helper wrappers for sound/music/fade (see below).
- **`Views` struct** (`engines/got/views/views.h`) — a plain struct that owns one instance of
  every `View`-derived class as a member, e.g. `Game _game; Opening _opening; Dialogs::MainMenu
  _mainMenu;` etc. Constructing this struct (done once in `Events::runGame()`) is what
  registers every view. There's no separate "registry" — registration happens as a side effect
  of each `UIElement` constructor (see below).

## Object registration / parent-child wiring

Two `UIElement` constructors:
```cpp
UIElement(const Common::String &name);                    // implicit parent = g_engine (root)
UIElement(const Common::String &name, UIElement *uiParent); // explicit parent
```
The 1-arg form auto-registers the new object as a child of the engine root (`g_engine->_children.push_back(this)`).
Every top-level `View` (dialogs, screens) uses this form, so `findView(name)` — which does a
recursive DFS over `_children` from the root — can find any view by name from anywhere.

A compound view that owns sub-panels (e.g. `Game` owning `GameContent` + `GameStatus`) has those
sub-panels **also** constructed with the 1-arg form (so they self-register with the root too), but
the parent view additionally does `_children.push_back(&_content)` manually in its own
constructor. This means such a sub-panel exists in `_children` under *both* the root and its
logical parent. The practical effect: `draw()`/`tick()`/message propagation recurse through the
logical parent → sub-panel path (because that's the path actually used for rendering/dispatch),
while `findView` can still resolve the sub-panel by name from the root scan. This is a mild wart,
not an elegant design — replicate it because it's the working pattern, but don't be surprised the
child is reachable two ways.

Controls *within* a view (e.g. a hypothetical `TextInput` widget) would use the 2-arg form with
`this` as parent, so they're scoped to that view only and not globally findable — appropriate
since they're not meant to be `replaceView`'d to directly.

## Design pattern: subdivide a complex view into sub-views

A view doesn't have to be one monolithic `draw()`/message-handler blob. When a screen has visually
and logically distinct regions — especially regions that update at different rates or own
different state — split it into several `View` subclasses composed as children of one owning view,
rather than cramming all the logic and drawing into a single class.

GOT's `Game` view is the example: the play area (`GameContent` — actors, tiles, scrolling,
combat/death animation) and the HUD strip (`GameStatus` — health, magic, inventory icons) are
separate `View` subclasses, each with its own `draw()`/`tick()`/state, wired up as:
```cpp
// game.h
class Game : public View {
    GameContent _content;
    GameStatus _status;
    ...
};

// game.cpp
Game::Game() : View("Game") {
    _children.push_back(&_content);
    _children.push_back(&_status);
    _content.setBounds(Common::Rect(0, 0, 320, 240 - 48));   // play area
    _status.setBounds(Common::Rect(0, 240 - 48, 320, 240));  // HUD strip at the bottom
}
```
Each sub-view gets its own non-overlapping `Bounds` rect (set once in the parent's constructor),
so `getSurface()` calls inside each sub-view's `draw()` are automatically clipped to the right
region — a sub-view never needs to know its own screen offset by hand. Because `draw()`/`tick()`/
`redraw()` all recurse through `_children` by default (see "Redraw flagging" and the `MESSAGE`
macro above), simply appending the sub-view to `_children` is enough to get it drawn, ticked, and
offered messages every frame — the parent view (`Game`) doesn't need to manually forward each
message type unless it wants to intercept something first (compare `Game::msgKeypress`/`msgAction`,
which handle a few game-wide cases directly and return `false` to let normal children-broadcast
happen for anything else via the base class).

Reach for this whenever a single view's `draw()` is accumulating unrelated concerns, or when one
region needs its own per-frame animation/timeout state that would otherwise pollute the parent's
fields. It also composes with the registration wart noted above: each sub-view still gets
constructed with the implicit-root-parent form (`View("GameContent")`), so it remains globally
findable/addressable by name (`send("GameStatus", ...)`) even though it's rendered as part of its
parent.

## Message system

`messages.h` defines plain structs deriving from `struct Message {}`:
`FocusMessage` (carries `_priorView`), `UnfocusMessage`, `MouseEnterMessage`, `MouseLeaveMessage`,
`KeypressMessage` (wraps `Common::KeyState`), `MouseDownMessage`/`MouseUpMessage`/`MouseMoveMessage`
(all typedefs/subclasses of `MouseMessage`, carrying button + position), `ActionMessage` (carries
an int action id from the keymapper), `GameMessage` (carries a `_name` string plus optional int or
string value — the generic "tell some other view something" channel), `ValueMessage` (bare int).

Handlers are declared via a macro in `UIElement`:
```cpp
#define MESSAGE(NAME)                                                     \
protected:                                                                \
    virtual bool msg##NAME(const NAME##Message &e) {                     \
        for (child : _children) if (child->msg##NAME(e)) return true;    \
        return false;                                                    \
    }                                                                     \
public:                                                                   \
    bool send(const Common::String &viewName, const NAME##Message &msg); \
    bool send(const NAME##Message &msg) { return msg##NAME(msg); }
MESSAGE(Focus); MESSAGE(Unfocus); MESSAGE(MouseEnter); MESSAGE(MouseLeave);
MESSAGE(Keypress); MESSAGE(MouseDown); MESSAGE(MouseUp); MESSAGE(Action);
MESSAGE(Game); MESSAGE(Value);
```
So every message type gets a `virtual bool msgNAME(const NAMEMessage&)` whose **default
implementation just forwards to children in order until one returns `true`** — i.e. default
behavior is "bubble down the tree, first handler to claim it wins." A concrete `msgNAME` override
typically either handles the message and returns `true` (stopping propagation — siblings and
children below are *not* asked), or returns `false` to let it fall through (or, if it wants both
its own handling AND child forwarding, explicitly calls `UIElement::msgNAME(e)` — see
`View::msgFocus` doing `return UIElement::msgFocus(msg);` after resetting its own state).

`msgMouseMove` is handled specially (not via the macro) — declared directly as a plain virtual
with a no-op default — called out in the header as being for performance, since mouse-move floods
would be expensive to run through the full child-broadcast machinery. It also gets deduplicated by
the engine before dispatch (rapid successive moves collapse to one event) — don't expect
per-pixel granularity in a `msgMouseMove` handler.

`Events` overrides every message handler to route *only* to the focused view (top of `_views`
stack) rather than broadcasting to all its direct children:
```cpp
bool msgKeypress(const KeypressMessage &e) override {
    return !_views.empty() ? focusedView()->msgKeypress(e) : false;
}
```
So the dispatch story end-to-end is: OS event → `Events::processEvent` builds the right Message →
`Events::msgX` (routes to focused view only) → that view's `msgX` (broadcasts to its own children
until one claims it, or handles directly).

`send()` comes in three forms:
- `send(msg)` — dispatch to self (equivalent to calling `msgX` directly, but consistent naming)
- `send(viewName, msg)` — find a view by name anywhere in the tree and dispatch directly to it,
  bypassing the view stack/focus entirely. This is how loosely-coupled cross-view signaling
  happens, e.g. `send("TitleBackground", GameMessage("MAIN_MENU"))` called from `Opening`,
  `SelectGame`, `SaveGame`, etc. — none of those views need to know how `TitleBackground` responds,
  just its name and the message contract.
- Implicit self-forwarding via the macro's child-broadcast default.

**`GameMessage` is the escape hatch for anything that doesn't fit Focus/Keypress/Mouse/Action** —
free-form named signals with an optional payload. Used for: a save/quit dialog being told which
mode to open in (`SaveGame::msgGame` checks `msg._name == "TITLE"` vs `"QUIT"`), a control
reporting user input back to its parent view generically (per the user's original notes — GOT
doesn't have a concrete TextInput example, but the mechanism is exactly `GameMessage`), or a view
telling another view to reset and take over (`TitleBackground::msgGame` handling `"MAIN_MENU"`
by clearing the view stack down to itself and pushing `MainMenu`).

## View stack management (all on `Events`, mirrored on `UIElement` as pass-throughs to `g_events`)

- `addView(ui|name)` — pushes a new view on top without removing the current one; old view gets
  `UnfocusMessage`, new view gets `redraw()` + `FocusMessage(oldView)`. Use for modal-ish overlays
  (menus, confirmation dialogs) where returning to the view underneath makes sense.
- `replaceView(ui|name, replaceAllViews=false, fadeOutIn=false)` — pops the current view (or, if
  `replaceAllViews`, clears the *entire* stack) and pushes the new one. `fadeOutIn` wraps the
  transition in `Gfx::fadeOut()`/`fadeIn()`. Use for hard scene transitions (splash → opening →
  story → game).
- `popView()` — pops the top view, sends `Unfocus` to it, redraws/redraws-and-draws remaining
  views (to erase the removed view's footprint), then sends `Focus` to the newly-exposed view.
- `close()` on a `UIElement` — asserts it's actually the focused view, then calls
  `g_events->popView()`. This is what a dialog calls on itself (e.g. after Escape or after a
  selection animation finishes) rather than the view needing a reference to `Events`.
- `clearViews()` — unfocuses the top view and empties the stack (used internally by
  `replaceView(..., true, ...)`, and directly when the OS sends `EVENT_QUIT`/
  `EVENT_RETURN_TO_LAUNCHER`).
- `focusedView()`, `priorView()` (one below top), `firstView()` (bottom of stack — used to check
  "are we in the base Game view" for save/load eligibility), `isPresent(name)` (anywhere in the
  stack — e.g. `isInCombat()` is just `isPresent("Combat")` for other games with that concept).

The **main loop exits** when `_views` becomes empty (all views popped/cleared) or `shouldQuit()`
returns true — so quitting is just "empty the view stack," achieved uniformly by `EVENT_QUIT`
handling in `processEvent`, and by whatever dialog flow calls `clearViews()`/lets the stack drain.

## Main loop (`Events::runGame()`)

1. Construct the `Views` struct (registers every view as described above).
2. Handle direct-load-from-launcher save slot, else push the initial view (`"SplashScreen"` for
   GOT) via `addView`.
3. Loop while `!_views.empty() && !shouldQuit()`:
   - Drain all pending OS events via `pollEvent`; `EVENT_QUIT`/`EVENT_RETURN_TO_LAUNCHER` clears
     the view stack and breaks out of the poll loop immediately (checked *before* the empty-check
     so leftover queued events don't get processed after quit is requested).
   - `processEvent(ev)` translates each OS/`Common::Event` into the matching `Message` struct and
     calls the corresponding `msgX` entry point on `Events` itself (which, per above, routes to
     the focused view). Keydown/keyup also maintain a raw `_G(keyFlag[])` array for legacy
     polling-style key-state checks alongside the message dispatch.
   - Game-specific periodic effects unrelated to the dispatch pattern itself (GOT: palette
     cycling for water/gem animation) run here too, gated by simple frame counters.
   - Frame pacing: a `nextFrameTime` accumulator only calls `nextFrame()` (which does the
     tick/draw/screen-update triad) once every `FRAME_DELAY` ms (`1000/FRAME_RATE`, GOT uses 60fps)
     — event polling itself runs as fast as the OS delivers, independent of frame rate.
4. `nextFrame()`: update any global per-frame state (RNG values, animation flags), call `tick()`
   (recurses through the focused view's children; returning `true` from a child's `tick()` short-
   circuits checking further siblings that frame — used sparingly), then `drawElements()`
   (recurses `draw()` only on elements with `_needsRedraw` set, clearing the flag after), then
   `_screen->update()` to blit to the OS window.

## Redraw flagging

`UIElement::redraw()` sets `_needsRedraw = true` on itself **and recursively on every child** —
so calling `redraw()` on a view forces its whole subtree to repaint next frame; there's no partial-
subtree invalidation. `drawElements()` is the actual "paint if dirty" driver — it's what `tick()`'s
frame loop calls, not `draw()` directly. A view's own `draw()` is the leaf rendering; `drawElements()`
is the flag-checking wrapper that also recurses into children. Many simple dialogs just call
`redraw()` unconditionally every `tick()` (see `SelectOption::tick()`) rather than tracking finer
dirty state — cheap enough at these resolutions/frame rates, and much simpler than partial
invalidation.

## Action / keybinding integration

Old direct keycodes (arrow keys, etc.) are *also* exposed through ScummVM's rebindable Keymapper
so users can remap them. Pattern (see `engines/got/metaengine.h/.cpp`):
1. Define an enum of abstract action ids, e.g. `KEYBIND_UP, KEYBIND_DOWN, ..., KEYBIND_SELECT,
   KEYBIND_FIRE, KEYBIND_MAGIC, KEYBIND_ESCAPE, KEYBIND_NONE` plus any game-specific ones
   (GOT adds `KEYBIND_THOR_DIES` as a debug hook).
2. A static `KeybindingRecord { action, id-string, description, default-key, default-joy }` table.
3. `MetaEngine::initKeymaps()` builds a `Common::Keymap`, adds a `Common::Action` per record with
   `setCustomEngineActionEvent(action)` and default input mappings — this is what makes the keys
   show up in ScummVM's remap-keys UI.
4. At runtime, a bound key firing produces `Common::EVENT_CUSTOM_ENGINE_ACTION_START`/`_END`;
   `Events::processEvent` turns `_START` into an `ActionMessage(ev.customType)` dispatched via
   `msgAction`, and also mirrors it into the raw `_G(keyFlag[])` table via `actionToKeyFlag()` so
   legacy polling code and the message-based path both see consistent state. `_END` just clears
   the flag (no action message on release).
5. Views implement `msgAction` (not raw `msgKeypress`) for anything the user should be able to
   remap — reserve `msgKeypress` for fixed/debug shortcuts that shouldn't be remappable
   (`Game::msgKeypress` handles F1/hardcoded debug letter-keys directly, separate from `msgAction`
   handling `KEYBIND_FIRE`/`KEYBIND_SELECT`/`KEYBIND_ESCAPE`).

A demo-recording/playback mode (`_G(demo)`) exists in GOT as a parallel event path
(`processDemoEvent`) that synthesizes the same key-flag/action effects from a canned byte stream
instead of real input — worth knowing the hook point exists if a target game had a similar
attract-mode demo, but it's game-specific, not core to the framework.

## `Bounds` helper

`Bounds` is a thunk wrapping a `Common::Rect` so it can be used almost like a plain `Common::Rect`
(implicit conversion operator, `.width()`/`.height()`) while automatically keeping a linked
`_innerBounds` rect in sync whenever it's assigned — `_innerBounds` is `_bounds` shrunk inward by
`_borderSize` (set via `setBorderSize()`, e.g. `Dialog` sets 16px so its border art doesn't overlap
content). `UIElement::getSurface(innerBounds=false)` returns a `GfxSurface` clipped to whichever
rect you ask for — draw code for chrome/borders uses the outer surface, content draw code uses the
inner one.

## Design pattern: reusable base + hook methods (`SelectOption`)

`Dialogs::SelectOption` (extends `Dialog` extends `View`) is the single generic
"bordered box with a title and a vertical list of text options, animated cursor, up/down/select/
escape via Action messages" implementation, reused by `MainMenu`, `Quit`, `SelectGame`, `SaveGame`,
`Ask`, `SkillLevel`, `SetMusic`, `SetSound`, etc. Subclasses **do not override input handling** —
they just call the base's protected `setContent(title, options)` (or pass them to the constructor)
and override two tiny virtual hooks:
```cpp
virtual void selected() {} // called after the "smack" selection animation finishes
virtual void closed() {}   // called when the user pressed Escape (or the ScummVM close callback)
```
`selected()` typically does a `switch (_selectedItem) { ... }` to branch on which option was
picked and call `addView`/`replaceView`/`g_engine->...` accordingly; `closed()` typically just
`addView("MainMenu")` or similar "go back" behavior, or (in `MainMenu::closed()`) forces a
specific choice so Escape has a sane default action. This is the idiomatic way to add a new menu
screen cheaply: subclass `SelectOption`, pass static option arrays, implement `selected`/`closed`.
For a genuinely custom screen (attribute entry, free text entry) you write a full `View` subclass
instead and hand-roll the state machine as described in "Why it exists" above.

`Ask` additionally demonstrates the "static show() convenience + fetch-existing-instance-by-name"
pattern for a dialog that's reused generically by scripted game logic (`Ask::show(title, options)`
looks up the singleton `"Ask"` view via `g_events->findView`, configures it, and opens it) rather
than each call site needing a reference to the instance.

## Sound/music/fade/TTS helpers (on `View`, not `UIElement`)

`View` provides thin wrappers so subclasses don't reach into game globals directly:
`playSound(index, priorityOverride)` / `playSound(GraphicChunk)`, `musicPlay(num|name, override)`,
`musicPause/Resume/Stop`, `musicIsOn()`, `fadeOut()`/`fadeIn(pal=nullptr)`. Optional
(`USE_TTS`-gated) `sayText(text, action=INTERRUPT)` for screen-reader accessibility — worth carrying
into a new port since it's a small addition once the message dispatch skeleton exists; note the
dedup-via-`_previousSaid` trick to avoid re-voicing text every redraw when nothing changed
(see `View::sayText`'s comment about looping).

## Timeout/delay counter

`UIElement` has a built-in single countdown: `_timeoutCtr`, decremented once per `tick()`; hitting
zero calls the virtual `timeout()` (default implementation just calls `redraw()`). `isDelayActive()`
/`cancelDelay()` let a view check/abort a pending timeout. Useful for "flash then auto-advance"
UI beats without hand-rolling a counter field in every view (though several GOT dialogs still do
roll their own counters for multi-stage animations like the "smack" selection effect, since the
built-in mechanism only supports one timeout at a time per element).

## Practical checklist when porting a new game to this framework

1. Copy `events.h`/`events.cpp` and `views/view.h`/`.cpp` from GOT as a starting skeleton; rename
   namespace. Decide the target engine class: `class FooEngine : public Engine, public Events`.
2. Define the game's `Message` payloads if any beyond the standard set (rare — `GameMessage`
   usually suffices for custom signals).
3. Enumerate the game's screens/dialogs as `View` subclasses; for anything that's "title + list of
   choices + up/down/select/escape," subclass `SelectOption` instead of writing input handling
   from scratch.
4. Build the `Views` struct (all view instances as members) and wire `runGame()`'s initial
   `addView(...)`.
5. For each screen that in the original game did sequential blocking input (e.g. a character
   creation screen prompting for stats one at a time), redesign it as: a mode/step enum field, a
   `msgFocus` that sets up step 0, and a message handler (`msgAction`/`msgGame` from a child
   control) that on each input advances the step, updates state, and calls `redraw()`. `draw()`
   renders purely off current state, no side effects.
6. Wire abstract actions through `KeybindingRecord`/`initKeymaps()` for anything the user should be
   able to remap; keep only true fixed/debug keys on raw `msgKeypress`.
7. Get quitting for free by not fighting the framework: don't add manual `shouldQuit()` checks
   inside view logic — trust that `EVENT_QUIT` reaching `Events::processEvent` clears the stack and
   the main loop exits.
