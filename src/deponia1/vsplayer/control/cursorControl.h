// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/vsplayer/control/cursorControl.cpp - see manifest/source_layout.tsv.
#pragma once

#include "TPaintControl.h"
#include "WxStub.h"

class TCursorControl : public TPaintControl {
public:
    wxPoint GetPositionNextToCursor() const;
};
