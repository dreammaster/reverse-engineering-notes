// Not yet assert-confirmed to a specific file; stays at the top level.
// Concrete TGInterface constructed from game data (TGameControl::
// InitInterfaces, Deponia_Linux.asm lines 458124-458250): for every link
// TVisObjRef::GetLinks() returns, one of these is heap-allocated and added
// to m_allInterfaces. Constructor parameter shape is confirmed by call
// shape only.
#pragma once

#include "TGInterface.h"
#include "datastruct/visobjref.h"

class THInterface : public TGInterface {
public:
    explicit THInterface(const TVisObjRef& object);
};
