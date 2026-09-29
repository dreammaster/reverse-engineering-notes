// Not yet assert-confirmed to a specific file; stays at the top level.
// A single interface panel/widget. Confirmed to derive from TPaintControl
// (TMasterControl::DrawInterfaces, masterControl.cpp: each list element
// exposes a Draw-like virtual at vtable slot 1, matching the Prepare@0/
// Draw@1 pattern also seen for TCursorControl/TLoadingControl/TGScene) and
// to hold a TVisObjRef identifying it (TGameControl::GetInterface,
// Deponia_Linux.asm lines 456603-456648, compares this field against a
// lookup key) plus a GetObject() lookup (TGameControl::GetObject, asm line
// 456719) - nothing else about this class has been reversed.
#pragma once

#include "TPaintControl.h"
#include "datastruct/vlist.h"
#include "datastruct/visobjref.h"

class TManagedObject;

// Confirmed 6 values, 0-5, from a jump table keyed on an interface's field
// id 0x13A (TGameControl::AdjustInterfacesOnScreen, Deponia_Linux.asm lines
// 465157-465494) - names are a best-effort read of each case's own
// behavior (see AdjustInterfacesOnScreen's comment), not recovered
// identifiers.
enum class TInterfacePositionEnum {
	kDockTopStacked = 0,    // y = accumulated top offset, x = 0; reserves height
	kDockBottomStacked = 1, // y flush against the current bottom edge; reserves height
	kDockTopRow = 2,        // x = accumulated left offset, y = 0; reserves width
	kFixedReserveWidth = 3, // a stored point, but also reserves width like case 2
	kFixed = 4,             // a stored point, no space reserved
	kDraggableClamped = 5,  // follows the mouse while being dragged; always clamped on-screen
};

class TGInterface : public TPaintControl {
public:
	const TVisObjRef &GetRef() const {
		return _ref;
	}
	// Confirmed mutated directly (TGameControl::AdjustInterfacesOnScreen
	// calls TVisObjRef::SetValue() on this field in place, Deponia_Linux.asm
	// line 465175) - same "TVisObjRef at a known offset, no accessor in the
	// original" pattern as TGCharacter's own dual GetRef() overloads.
	TVisObjRef &GetRef() {
		return _ref;
	}
	// Confirmed TManagedObject* (TGameControl::StartObjectText calls
	// TManagedObject::SetText() directly on the result, same as
	// TGScene::GetObject() - asm lines 462233-462396) - the earlier void*
	// was a placeholder guess.
	TManagedObject *GetObject(const TVisObjRef &object) const;

	// Confirmed call shapes only (TGameControl::SetInterfaces, asm lines
	// 465533-465671): called on an interface leaving the active set before
	// it's dropped, and on every interface entering it, respectively.
	void RemoveSpritesAndAnimations();
	void SetObjectsActive(bool active);
	// Confirmed call shape only (TGameControl::AdjustInterfacesOnScreen,
	// Deponia_Linux.asm lines 465036-465040) - called once per interface
	// belonging to the current character, with the character's own field-
	// 0x297 links; not reversed beyond that.
	void UpdateItems(const TVList &items);

private:
	TVisObjRef _ref;
};
