// Not yet assert-confirmed to a specific file; stays at the top level.
// The "currently displayed text" object (confirmed field in TGameControl,
// IsTextActive/IsNoTextDisplayed - Deponia_Linux.asm lines 461674-461777):
// TGameControl holds one via a raw TSText* that's null when no text is
// showing. GetDataObject() and the target field are confirmed by their call
// shapes.
//
// Confirmed polymorphic and sharing a vtable slot (0x28) with TGText
// (ClearCurrentText calls it on a TSText*, ClearObjectText/
// ReattachSceneObjectTexts call the exact same-offset slot on a TGText* -
// asm lines 462001-462228) - modeled as a common base with TGText deriving
// from it, since nothing contradicts that and it's the simplest
// explanation for two unrelated-looking classes sharing one virtual slot.
#pragma once

#include "datastruct/visobjref.h"

class TSText {
public:
    virtual ~TSText() = default;

    // Unconfirmed name/purpose - called right before a text is dropped
    // (ClearCurrentText/ClearObjectText).
    virtual void Discard();

    // A second, distinct unnamed virtual (vtable slot 0x10, vs. Discard's
    // 0x28) called unconditionally just before Discard() when a text is
    // removed by direct reference (TGameControl::ClearText, asm lines
    // 462048-462150) - name/purpose not resolved.
    virtual void OnCleared();

    TVisObjRef GetDataObject() const;
    const TVisObjRef& GetTarget() const { return m_target; }

    // Confirmed called on both a TSText* and list elements of type TGText*
    // (TGameControl::UpdateTexts, asm lines 456183-456282), gating whether
    // the text later needs clearing - not reversed beyond that call shape.
    void CalculateCurrentText();

private:
    TVisObjRef m_target;
};
