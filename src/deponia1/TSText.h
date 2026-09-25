// Not yet assert-confirmed to a specific file; stays at the top level.
// The "currently displayed text" object (confirmed field in TGameControl,
// IsTextActive/IsNoTextDisplayed - Deponia_Linux.asm lines 461674-461777):
// TGameControl holds one via a raw TSText* that's null when no text is
// showing. GetDataObject() and the target field are confirmed by their call
// shapes; nothing else about this class has been reversed.
#pragma once

#include "datastruct/visobjref.h"

class TSText {
public:
    TVisObjRef GetDataObject() const;
    const TVisObjRef& GetTarget() const { return m_target; }

private:
    TVisObjRef m_target;
};
