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

    // Confirmed present at a fixed offset (TGameControl::IsTalking compares
    // a TGText's speaker against a TVisObjRef via this field directly,
    // Deponia_Linux.asm lines 461785-461846) - same "TVisObjRef at a known
    // offset, no accessor in the original" pattern as TGDialog/TSText/TGText.
    const TVisObjRef& GetRef() const { return m_ref; }
    // Confirmed mutated directly (TGameControl::SetCharacterActiveCommand
    // calls TVisObjRef::SetLink() on this field in place, asm lines
    // 465679-465841).
    TVisObjRef& GetRef() { return m_ref; }

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

    // Confirmed called on both the outgoing and incoming character when
    // switching (TGameControl::ChangeCharacter, asm lines 465849-466072).
    void SetRandomTime();

    // Confirmed a by-value std::list<TGInterface*> (TGameControl::
    // SetInterfaces copies it and iterates the copy, asm lines
    // 465533-465671).
    std::list<TGInterface*> GetInterfaces() const;

private:
    TVisObjRef m_ref;
};
