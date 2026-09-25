// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/datastruct/visionaire.cpp - see manifest/source_layout.tsv.
//
// TVisionaire is the engine's root game-data object; only the one accessor
// TMasterControl needs is stubbed here.
#pragma once

#include "datastruct/vlist.h"
#include "datastruct/visobjref.h"

class TVisionaire {
public:
    TVisObjRef GetGame() const;

    // Confirmed called with a field id, an out-param list, and a bool flag
    // (TGameControl::InitFonts, asm lines 458293-458322) - not reversed
    // beyond that call shape.
    void GetList(int fieldId, TVList& outList, bool flag) const;
};
