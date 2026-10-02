// Not yet assert-confirmed to a specific file; stays at the top level.
// An in-game character (position, animation, walking state); referenced
// throughout TGameControl (GetCurrentCharacter, GetCharacter,
// ChangeCharacter, etc.) but not itself reversed yet.
#pragma once

#include <list>

#include "TManagedObject.h"
#include "WxStub.h"
#include "datastruct/visobjref.h"

class TGInterface;

// Confirmed to derive from TManagedObject (TGameControl::StartObjectText,
// asm lines 462233-462396): when no scene/interface object is found for the
// target, the code falls back to GetCharacterPointerEx() and calls
// TManagedObject::SetText() directly on the resulting TGCharacter* - only
// possible if TGCharacter IS-A TManagedObject.
//
// GetRef() below was originally modeled against its own separate _ref
// field at "a known offset" (TGameControl::IsTalking/
// SetCharacterActiveCommand) - now that TManagedObject's own layout is
// confirmed (see TManagedObject.h), that offset is almost certainly
// TManagedObject's own inherited _objRef (TGCharacter has no other bases
// ahead of it), so GetRef() delegates to the inherited field instead of
// keeping a second, redundant one.
class TGCharacter : public TManagedObject {
public:
	TGCharacter() = default;
	virtual ~TGCharacter() = default;

	// Confirmed virtual (vtable-indexed call at a fixed slot), returning a
	// wxPoint compared against a {-1,-1} "no valid position" sentinel
	// (TGameControl::CenterScene, asm lines 460533-460715) - name/purpose
	// not resolved.
	virtual wxPoint GetScreenPosition() const;
	// Confirmed virtual, returning a plain-int wxRect (confirmed by the
	// RAX:RDX register-pair return convention rather than a hidden pointer -
	// only valid for a <=16-byte all-integer struct) used for vertical
	// scene-centering at the same call site - name/purpose not resolved.
	virtual wxRect GetVisibleRect() const;
	// Confirmed virtual (vtable-indexed call), called on every character
	// during a save (TGameControl::Save, asm lines 462896-462905) - not
	// reversed beyond that call shape.
	virtual void Save();
	// Confirmed virtual (vtable slot 0xC0, TGameControl::Load, Deponia_Linux.
	// asm lines 476981-476990) - called on every character while loading a
	// save; placed symmetrically with Save() above (name not itself
	// recovered) - not reversed beyond that call shape.
	virtual void Load();

	// Confirmed present at a fixed offset (TGameControl::IsTalking compares
	// a TGText's speaker against a TVisObjRef via this field directly,
	// Deponia_Linux.asm lines 461785-461846) - same "TVisObjRef at a known
	// offset, no accessor in the original" pattern as TGDialog/TSText/TGText.
	const TVisObjRef &GetRef() const {
		return _objRef;
	}
	// Confirmed mutated directly (TGameControl::SetCharacterActiveCommand
	// calls TVisObjRef::SetLink() on this field in place, asm lines
	// 465679-465841).
	TVisObjRef &GetRef() {
		return _objRef;
	}

	// Confirmed static (no implicit `this` - Deponia_Linux.asm lines
	// 178297-178307) and confirmed in full: `direction` is a 0-359 compass
	// value, and the result wraps the same way.
	static int GetOppositeDirection(int direction) {
		int sum = direction + 180;
		return sum >= 360 ? direction - 180 : sum;
	}

	// Confirmed called for every character (TGameControl::
	// SetCharacterInterfaces, Deponia_Linux.asm lines 458260-458285) - not
	// reversed beyond that call shape.
	void SetInterfaces();

	// Confirmed called per-character in TGameControl::HandleCharacters
	// (asm lines 460829-460866), in this order, every frame the scene isn't
	// a menu.
	void WalkWay();
	void UpdateCharacter();

	// Confirmed called for every character (TGameControl::
	// SetAllCharactersOnDestination, asm lines 460874-460902).
	void SetOnDestination();

	// Confirmed called per-character in TGameControl::UpdateRandomTimers
	// (asm lines 463363-463466).
	void CheckRandomTimer();

	// Confirmed call shape only (TGameControl::MoveScene, Deponia_Linux.asm
	// lines 459941, 460022) - not reversed beyond that.
	bool IsWalking() const;

	// Confirmed call shapes only (TGameControl::UpdateWalkingSounds, asm
	// lines 463555, 463597) - not reversed beyond that. A sibling method,
	// TGCharacter::CheckWalkingSound() (not itself called from
	// UpdateWalkingSounds - xref'd only via two shared float constants),
	// presumably updates whatever IsWalkingSoundPlaying() reads; not
	// reversed.
	bool IsWalkingSoundPlaying() const;
	wxFileName GetWalkingSound() const;

	// Confirmed called on both the outgoing and incoming character when
	// switching (TGameControl::ChangeCharacter, asm lines 465849-466072).
	void SetRandomTime();

	// Confirmed a by-value std::list<TGInterface*> (TGameControl::
	// SetInterfaces copies it and iterates the copy, asm lines
	// 465533-465671).
	std::list<TGInterface *> GetInterfaces() const;

	// Confirmed call shapes only (TGameControl::InitCharacters, asm lines
	// 466201-466735) - called once per newly-constructed character, in
	// this order, right after construction.
	void Init();
	void AssignToScene(const TVisObjRef &scene, const wxPoint &pos, int walkSpeed);

	// Confirmed call shape only (TManagedObject::HandlePostExecution,
	// Deponia_Linux.asm line 190824) - not reversed beyond that.
	void ShowComment(const TVisObjRef &comment);
	// Confirmed call shape only (TManagedObject::ExecuteMatchingAction,
	// Deponia_Linux.asm line 55E1C5) - not reversed beyond that.
	void StopWalking(bool flag);
};
