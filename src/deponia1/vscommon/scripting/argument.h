// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/vscommon/scripting/argument.cpp - see manifest/source_layout.tsv.
//
// TArgument is a tagged-union style value passed to/from Lua script calls
// (see TMasterControl::ProcessMessage, which builds a std::vector<TArgument*>
// of these to call a registered handler). Only the two Set() overloads
// TMasterControl uses are stubbed here.
#pragma once

#include "WxStub.h"

class TArgument {
public:
    void Set(int value);
    void Set(const wxPoint& value);
};
