// Not yet assert-confirmed to a specific file; stays at the top level.
#pragma once

#include "WxStub.h"

class TGObjectManager {
public:
    wxString GetActionText() const;

    // Confirmed called in this order from TGameControl::ResetState
    // (Deponia_Linux.asm lines 460910-460953) - not reversed beyond that
    // call shape.
    void ResetCurrentObject();
    void ResetEventInfo();
    void RemoveItem(bool flag);
};
