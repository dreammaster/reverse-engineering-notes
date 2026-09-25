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
#include "datastruct/visobjref.h"

class TGInterface : public TPaintControl {
public:
    const TVisObjRef& GetRef() const { return m_ref; }
    void* GetObject(const TVisObjRef& object) const;

private:
    TVisObjRef m_ref;
};
