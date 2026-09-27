// Not yet assert-confirmed to a specific file; stays at the top level.
// Concrete TGText for object/scene-attached text (TGameControl::
// StartObjectText constructs one on the heap and stores it in
// m_sceneTexts, Deponia_Linux.asm lines 462233-462396). Constructor
// parameter shapes are confirmed from the call site; their meaning beyond
// that isn't (character is always passed nullptr at this one call site).
#pragma once

#include "TGCharacter.h"
#include "TGText.h"
#include "datastruct/visobjref.h"
#include "vscommon/fontManager.h"

class THText : public TGText {
public:
    THText(const TVisObjRef& activeObject, const TVisObjRef& object, TGCharacter* character, const TVisObjRef& text,
           TextAlignmentEnum alignment, const TVisObjRef& target, const wxPoint& pos, bool flag1, bool flag2);
};
