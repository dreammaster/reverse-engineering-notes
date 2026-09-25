// Not yet assert-confirmed to a specific file; stays at the top level of
// datastruct/ alongside visobjref.h/visionaire.h, its evident siblings.
//
// TVList is passed by reference into TVisionaire::GetList(int, TVList&,
// bool) and TFontManager::Initialize(TVList&) (confirmed,
// TGameControl::InitFonts - Deponia_Linux.asm lines 458293-458331): the
// caller zero-initializes 24 stack bytes in place of calling a visible
// constructor, matching the 3-pointer (begin/end/capacity) shape of a
// vector-like container - modeled here as a thin wrapper over
// std::vector<TVisObjRef> since nothing contradicts that.
#pragma once

#include <vector>

#include "datastruct/visobjref.h"

class TVList {
public:
    void clear() { items.clear(); }

    std::vector<TVisObjRef> items;
};
