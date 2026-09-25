// Not yet assert-confirmed to a specific file; stays at the top level.
// Embedded by value in TGameControl (confirmed: TGameControl's ctor calls
// TGDialog::TGDialog() directly on an interior pointer, and TGameControl
// exposes it via GetDialog()/StartDialog()/EndDialog()).
//
// Confirmed from TGameControl::DisplayDialog() (Deponia_Linux.asm lines
// 455780-455803): TGDialog's *first member* is a TVisObjRef (the currently
// active dialog target) - DisplayDialog() calls TVisObjRef::IsEmpty()
// directly on TGDialog's own address, and later TGDialog::Draw() on that
// same address, so a dialog is "active" exactly when this field is
// non-empty. StartDialog()/EndDialog() (not yet reversed) presumably set/
// clear it.
#pragma once

#include "datastruct/visobjref.h"

class TGDialog {
public:
    TGDialog() = default;

    bool IsEmpty() const { return m_target.IsEmpty(); }
    void Draw();

private:
    TVisObjRef m_target;
};
