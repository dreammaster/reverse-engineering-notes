// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed in full except HandleEvent()/ObjectReached(TManagedObject*) (and
// the TGEventInfo-dispatching portion of MouseMove()/ExecuteSavedObject()) -
// reaching all 25 manifest-listed methods (Deponia_Linux.asm lines
// 187194-190177): tracks which TManagedObject the mouse is currently
// hovering (_currentObject) and a separate "saved" one
// (SaveCurrentObject()/ExecuteSavedObject(), used once a character has
// finished walking up to something that was clicked out of reach), plus a
// registered hook name for GetActionText().
//
// HandleEvent()/ObjectReached(TManagedObject*) each build a TGEventInfo
// from the game's own saved click/hover state and dispatch it virtually
// through TManagedObject::ExecuteEvent() (vtable slot 0x30) - the exact
// same action-execution-subsystem gap already flagged on TManagedObject
// itself (TGEventInfo's own fields aren't named yet, so transcribing the
// construction would just be moving bytes around under invented names).
// MouseMove() and ExecuteSavedObject() fire the same dispatch as one step
// among several - their other, real bookkeeping is implemented in full,
// with just that one step left as a documented no-op.
//
// GetActionText() (asm lines 189563-190177, by far the largest method here)
// is left as a confirmed-call-shape stub for a different reason: when the
// registered hook name is empty, it falls back to formatting "<button's
// language name> <object's own display text>" by calling an unidentified
// TManagedObject virtual (vtable slot 0xB0 - itself ambiguous due to
// identical-code-folding with an unrelated method, see TManagedObject.h);
// when the hook name is set, it calls LuaExecuteFunction() directly - this
// project's standing, deliberately-unreversed Lua-bridge-contract gap. Both
// paths need their own dedicated follow-up.
#pragma once

#include "TGDetectInfo.h"
#include "WxStub.h"
#include "datastruct/visobjref.h"

class TGCharacter;
class TManagedObject;
enum class TMouseEventEnum;

class TGObjectManager {
public:
	// Confirmed (Deponia_Linux.asm line 55B4A0): assigns straight into the
	// registered hook name.
	void RegisterHookFunctionGetActionText(const wxString &name) {
		_actionTextHookName = name;
	}

	// Confirmed (asm lines 187267-187315): clears the game's own "reached
	// object" link (0x2AE, sending events), releases whatever the cursor is
	// holding, and marks the "held" flag false (without sending events).
	void ResetEventInfo();
	// Confirmed (asm lines 187316-187356): clears the game's "reached
	// object" link the same way, without touching the cursor/held flag.
	void ResetCurrentObject();

	// Confirmed (asm lines 187357-187453): if `item` is the game's current
	// "reached object" link, clears it; if something was also held at that
	// point, releases it via the cursor and clears the "held" flag.
	void RemoveItem(const TVisObjRef &item);
	// Confirmed (asm lines 187455-187541): a second overload - does nothing
	// at all when nothing is held and `keepIfNotHeld` is true; otherwise
	// clears the "reached object" link, and if something was held,
	// releases it via the cursor and clears the "held" flag (same cleanup
	// as the overload above, just unconditionally on the link).
	void RemoveItem(bool keepIfNotHeld);

	// Confirmed (asm lines 187542-187628): sets the game's "reached object"
	// link to `item` and its "held" flag to whether `held` was requested
	// and `item` is non-empty; tells the cursor to pick `item` up or drop
	// whatever it was holding accordingly.
	void SetItem(const TVisObjRef &item, bool held);

	// Confirmed call shape only (asm lines 187629-188249) - see this
	// class's own header comment (the action-execution-subsystem gap).
	void HandleEvent(TMouseEventEnum event);

	// Confirmed call shape only (asm lines 187906-188165) - see this
	// class's own header comment.
	void ObjectReached(TManagedObject *object);
	// Confirmed (asm lines 188166-188249): only forwards to the
	// TManagedObject* overload above when the current character is both in
	// the current scene and is `character` itself; looks up `target`'s own
	// object in the current scene first.
	void ObjectReached(TGCharacter &character, TVisObjRef &target);

	// Confirmed in full (asm lines 188250-188346): snapshots the game's
	// current "action" (0x262), "reached object" (0x2AE), and "held"
	// (0x2DA) state into its own saved fields (0x1E3/0x1E4/0x1E5), plus the
	// mouse event that triggered it (0x267).
	void SaveEventInfo(TMouseEventEnum event);

	// Confirmed in full apart from the TGEventInfo-dispatching step (asm
	// lines 188374-188648) - see this class's own header comment; updates
	// _currentObject and the game's "reached object" link (0x2AF).
	void MouseMove(TManagedObject *object);

	// Confirmed (asm lines 188649-188753): clears whichever of
	// _savedObject/_currentObject refers to the same object as `item` (by
	// TVisObjRef identity, approximating the original's own TId-based
	// comparison - see NOTES.md), plus the matching saved game-data link
	// (0x1E6 for _savedObject, 0x2AF for _currentObject).
	void NotifyObjectRemoved(const TVisObjRef &item);

	// Confirmed (asm lines 188755-188827): snapshots _currentObject into
	// _savedObject, and mirrors it into the game's own "saved object" link
	// (0x1E6, without sending events) - cleared if there's no current
	// object.
	void SaveCurrentObject();

	// Confirmed in full apart from the TGEventInfo-dispatching step (asm
	// lines 188829-189026) - see this class's own header comment; makes
	// _savedObject the new _currentObject, mirrors it into the game's
	// "reached object" link (0x2AF), and fires HandleEvent() to replay it.
	void ExecuteSavedObject();

	// Confirmed (asm lines 189027-189113): resolves the game's "saved
	// object" link (0x1E6) back to a real TManagedObject* via
	// TGameControl::GetObject(), or clears _savedObject if the link is
	// empty.
	void SavedObjectChanged();

	// Confirmed (asm lines 189114-189140): true if there's no current
	// object, or its own game-data reference is empty.
	bool IsCurrentObjectEmpty() const;
	// Confirmed (asm lines 189140-189184): the current object's packed id's
	// 4th byte must be 2, and its "0x129" field non-zero.
	bool IsCurrentObjectDetectable() const;
	// Confirmed (asm lines 189185-189314): the current object itself must
	// report walkable, the game's current "action" button must be a
	// standard command, and nothing must currently be held.
	bool IsCurrentObjectWalkable() const;

	// Confirmed (asm lines 189315-189352): the current object's own
	// reference, or the game's empty-object sentinel if there is none.
	TVisObjRef GetCurrentObject() const;

	// Confirmed in full (asm lines 189352-189567): fills in `info` from the
	// game's own "action" field (0x150) and "held" flag (0x265) - see
	// TGDetectInfo's own header comment for the confirmed flag
	// combinations.
	void GetDetectInfo(TGDetectInfo &info) const;

	// Confirmed (asm lines 189460-189521): the current character, but only
	// if it's the one actually in the current scene right now.
	TGCharacter *GetEventCharacter() const;
	// Confirmed (asm lines 189522-189562): the game's current "action" link
	// (0x262).
	TVisObjRef GetEventCommand() const;
	// Confirmed call shape only (asm lines 189563-190177) - see this
	// class's own header comment.
	wxString GetActionText() const;

private:
	TManagedObject *_currentObject = nullptr;
	TManagedObject *_savedObject = nullptr;
	wxString _actionTextHookName;
};
