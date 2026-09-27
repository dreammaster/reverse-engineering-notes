// Not yet assert-confirmed to a specific file; stays at the top level.
// Concrete TGCharacter constructed from game data (TGameControl::
// InitCharacters, Deponia_Linux.asm lines 466201-466735): for every
// character link TVisionaire::GetList() returns, one of these is
// heap-allocated and added to m_characters. Constructor parameter shape
// (a self-reference and a parent/scene reference) is confirmed by call
// shape only.
#pragma once

#include "TGCharacter.h"
#include "datastruct/visobjref.h"

class THCharacter : public TGCharacter {
public:
    THCharacter(const TVisObjRef& self, const TVisObjRef& parent);
};
