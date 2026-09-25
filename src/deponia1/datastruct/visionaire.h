// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/datastruct/visionaire.cpp - see manifest/source_layout.tsv.
//
// TVisionaire is the engine's root game-data object; only the one accessor
// TMasterControl needs is stubbed here.
#pragma once

#include "datastruct/visobjref.h"

class TVisionaire {
public:
    TVisObjRef GetGame() const;
};
